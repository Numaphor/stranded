#!/usr/bin/env python3
"""
gba_mem_snapshot.py — Read live GBA memory from mGBA's GDB server.

Usage:
    python3 scripts/gba_mem_snapshot.py [--resume-secs N] [--address ADDR] [--port PORT]

Default behavior: connect to mGBA GDB on localhost:2345, resume the game for N
seconds (default 5), interrupt, dump OAM/affine/IO/IWRAM/palette/registers, then
resume and disconnect.

Can also be imported and used as a library.
"""
import socket
import struct
import sys
import time
import argparse
import re

DEFAULT_HOST = "localhost"
DEFAULT_PORT = 2345


def _checksum(data: bytes) -> int:
    return sum(data) & 0xFF


def _make_packet(cmd: str) -> bytes:
    payload = cmd.encode("ascii")
    cs = _checksum(payload)
    return b"$" + payload + b"#" + f"{cs:02x}".encode()


def _recv_packet(sock: socket.socket, timeout: float = 5.0) -> str:
    """Receive one GDB RSP packet, return the payload string."""
    sock.settimeout(timeout)
    buf = b""
    # Read raw bytes until we find $...#xx
    in_packet = False
    while True:
        ch = sock.recv(1)
        if not ch:
            raise ConnectionError("Connection closed")
        if not in_packet:
            if ch == b"$":
                in_packet = True
                buf = b""
            # Skip +/- ack chars and other noise
            continue
        buf += ch
        # Check for #xx at end of buffer
        if len(buf) >= 3:
            # Look for #XX pattern
            m = re.search(rb"#[0-9a-fA-F]{2}$", buf)
            if m:
                payload = buf[:m.start()]
                return payload.decode("ascii", errors="replace")


def _send_recv(sock: socket.socket, cmd: str, timeout: float = 5.0) -> str:
    """Send a GDB RSP command and receive the response."""
    pkt = _make_packet(cmd)
    sock.sendall(pkt)
    return _recv_packet(sock, timeout)


def _read_mem(sock: socket.socket, addr: int, length: int) -> bytes:
    """Read `length` bytes from `addr` via GDB 'm' command.
    Falls back to smaller chunks on error."""
    resp = _send_recv(sock, f"m{addr:x},{length:x}")
    if not resp.startswith("E"):
        return bytes.fromhex(resp)

    # If full read fails, try in smaller chunks (some GDB stubs limit packet size)
    chunk_size = 64
    result = b""
    for off in range(0, length, chunk_size):
        sz = min(chunk_size, length - off)
        r = _send_recv(sock, f"m{addr+off:x},{sz:x}")
        if r.startswith("E"):
            raise RuntimeError(f"Memory read error at 0x{addr+off:08x}: {r}")
        result += bytes.fromhex(r)
    return result


def _read_u16(sock: socket.socket, addr: int) -> int:
    data = _read_mem(sock, addr, 2)
    return struct.unpack_from("<H", data)[0]


def _read_u32(sock: socket.socket, addr: int) -> int:
    data = _read_mem(sock, addr, 4)
    return struct.unpack_from("<I", data)[0]


def _read_registers(sock: socket.socket) -> dict:
    """Read all CPU registers via GDB 'g' command."""
    resp = _send_recv(sock, "g")
    if resp.startswith("E"):
        return {}
    raw = bytes.fromhex(resp)
    names = ["r0","r1","r2","r3","r4","r5","r6","r7",
             "r8","r9","r10","r11","r12","sp","lr","pc","cpsr"]
    regs = {}
    for i, name in enumerate(names):
        if i * 4 + 4 <= len(raw):
            regs[name] = struct.unpack_from("<I", raw, i * 4)[0]
    return regs


def _continue(sock: socket.socket):
    """Send continue command (does not wait for response since game runs)."""
    sock.sendall(_make_packet("c"))


def _interrupt(sock: socket.socket):
    """Send Ctrl+C (0x03) to halt the target."""
    sock.sendall(b"\x03")
    # Read the stop reply - mGBA may send ack (+) then the stop packet
    # Be lenient with timeout since mGBA may be slow to respond
    try:
        return _recv_packet(sock, timeout=10.0)
    except TimeoutError:
        # Some GDB stubs don't send a clean reply to break
        # Try reading raw data to flush the pipe
        sock.setblocking(False)
        try:
            data = sock.recv(4096)
        except BlockingIOError:
            data = b""
        sock.setblocking(True)
        sock.settimeout(5.0)
        return data.decode("ascii", errors="replace") if data else "timeout"


def snapshot(host=DEFAULT_HOST, port=DEFAULT_PORT, resume_secs=5):
    """Connect to mGBA GDB, resume game, snapshot memory, return dict."""

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
    sock.settimeout(5.0)

    # Ack negotiation - send ack for any initial packet
    sock.sendall(b"+")
    time.sleep(0.2)

    # Drain any initial data
    sock.setblocking(False)
    try:
        while True:
            d = sock.recv(4096)
            if not d:
                break
    except BlockingIOError:
        pass
    sock.setblocking(True)
    sock.settimeout(5.0)

    result = {}

    # Resume the game
    print(f"Resuming game for {resume_secs} seconds...")
    _continue(sock)
    time.sleep(resume_secs)

    # Interrupt
    print("Interrupting to take snapshot...")
    stop_reply = _interrupt(sock)
    print(f"  Stop reply: {stop_reply[:60] if stop_reply else 'none'}")
    time.sleep(0.5)

    # Send ack
    sock.sendall(b"+")

    # --- Read I/O Registers ---
    try:
        dispcnt = _read_u16(sock, 0x04000000)
        dispstat = _read_u16(sock, 0x04000002)
        vcount = _read_u16(sock, 0x04000006)
        result["io"] = {
            "DISPCNT": dispcnt,
            "DISPCNT_mode": dispcnt & 0x7,
            "DISPCNT_obj": (dispcnt >> 12) & 1,
            "DISPCNT_bg0": (dispcnt >> 8) & 1,
            "DISPCNT_bg1": (dispcnt >> 9) & 1,
            "DISPCNT_bg2": (dispcnt >> 10) & 1,
            "DISPCNT_bg3": (dispcnt >> 11) & 1,
            "DISPSTAT": dispstat,
            "VCOUNT": vcount & 0xFF,
        }
        print(f"\n=== I/O Registers ===")
        print(f"DISPCNT  = 0x{dispcnt:04X}  Mode={dispcnt & 0x7}  OBJ={'on' if (dispcnt>>12)&1 else 'off'}")
        print(f"  BG enables: BG0={result['io']['DISPCNT_bg0']} BG1={result['io']['DISPCNT_bg1']} BG2={result['io']['DISPCNT_bg2']} BG3={result['io']['DISPCNT_bg3']}")
        print(f"DISPSTAT = 0x{dispstat:04X}")
        print(f"VCOUNT   = {vcount & 0xFF}")
    except Exception as e:
        print(f"I/O read error: {e}")

    # --- Read OAM ---
    try:
        oam_raw = _read_mem(sock, 0x07000000, 1024)  # full 1KB OAM
        visible = []
        affines = []

        for i in range(128):
            off = i * 8
            attr0, attr1, attr2 = struct.unpack_from("<HHH", oam_raw, off)
            mode = (attr0 >> 8) & 0x3
            if mode != 2:  # not hidden
                y = attr0 & 0xFF
                x = attr1 & 0x1FF
                shape = (attr0 >> 14) & 0x3
                size = (attr1 >> 14) & 0x3
                tile = attr2 & 0x3FF
                prio = (attr2 >> 10) & 0x3
                pal = (attr2 >> 12) & 0xF
                aff_idx = ((attr1 >> 9) & 0x1F) if mode in (1, 3) else -1
                visible.append({
                    "id": i, "x": x, "y": y, "mode": mode,
                    "shape": shape, "size": size, "tile": tile,
                    "priority": prio, "palette": pal, "affine_idx": aff_idx,
                    "attr0": attr0, "attr1": attr1, "attr2": attr2,
                })

        # Affine matrices (interleaved at OAM offsets 6,14,22,30 per group of 32 bytes)
        for m in range(32):
            base = m * 32
            pa = struct.unpack_from("<h", oam_raw, base + 6)[0]
            pb = struct.unpack_from("<h", oam_raw, base + 14)[0]
            pc = struct.unpack_from("<h", oam_raw, base + 22)[0]
            pd = struct.unpack_from("<h", oam_raw, base + 30)[0]
            if pa != 0 or pb != 0 or pc != 0 or pd != 0:
                det = (pa * pd - pb * pc) / 65536.0
                affines.append({
                    "idx": m, "pa": pa, "pb": pb, "pc": pc, "pd": pd,
                    "det": det,
                })

        result["oam"] = {"visible": visible, "affines": affines}

        print(f"\n=== OAM: {len(visible)} visible sprites ===")
        for s in visible[:20]:  # first 20
            aff_str = f" Aff={s['affine_idx']}" if s['affine_idx'] >= 0 else ""
            print(f"  OBJ[{s['id']:3d}] X={s['x']:3d} Y={s['y']:3d} M={s['mode']} Sh={s['shape']} Sz={s['size']} Tile={s['tile']:3d} P={s['priority']}{aff_str}")
        if len(visible) > 20:
            print(f"  ... and {len(visible) - 20} more")

        if affines:
            print(f"\n=== Affine Matrices ({len(affines)} active) ===")
            for a in affines:
                flag = " !!" if (a['det'] < 0.25 or a['det'] > 4.0) else ""
                print(f"  Aff[{a['idx']:2d}] pa={a['pa']:6d} pb={a['pb']:6d} pc={a['pc']:6d} pd={a['pd']:6d}  det={a['det']:.3f}{flag}")
    except Exception as e:
        print(f"OAM read error: {e}")

    # --- Read CPU Registers ---
    try:
        regs = _read_registers(sock)
        result["registers"] = regs
        print(f"\n=== CPU Registers ===")
        for name in ["r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","r11","r12","sp","lr","pc","cpsr"]:
            if name in regs:
                print(f"  {name:4s} = 0x{regs[name]:08X}")
    except Exception as e:
        print(f"Register read error: {e}")

    # --- Read IWRAM sample ---
    try:
        iwram = _read_mem(sock, 0x03000000, 128)
        result["iwram_head"] = iwram.hex()
        print(f"\n=== IWRAM (first 128 bytes) ===")
        for i in range(0, 128, 16):
            words = struct.unpack_from("<IIII", iwram, i)
            print(f"  0x{0x03000000+i:08X}: {words[0]:08x} {words[1]:08x} {words[2]:08x} {words[3]:08x}")
    except Exception as e:
        print(f"IWRAM read error: {e}")

    # --- Read Palette sample ---
    try:
        bg_pal = _read_mem(sock, 0x05000000, 32)
        obj_pal = _read_mem(sock, 0x05000200, 32)
        print(f"\n=== BG Palette (first 16 colors, BGR555) ===")
        for i in range(0, 32, 2):
            c = struct.unpack_from("<H", bg_pal, i)[0]
            r = (c & 0x1F) << 3
            g = ((c >> 5) & 0x1F) << 3
            b = ((c >> 10) & 0x1F) << 3
            print(f"  [{i//2:2d}] 0x{c:04X} = RGB({r:3d},{g:3d},{b:3d})", end="")
            if i % 8 == 6:
                print()
        print()
    except Exception as e:
        print(f"Palette read error: {e}")

    # Resume and disconnect — just resume, closing socket cleanly
    print("\n=== Resuming game ===")
    _continue(sock)
    time.sleep(0.2)
    sock.close()

    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Snapshot GBA memory from mGBA GDB server")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--resume-secs", type=int, default=5,
                        help="Seconds to let the game run before snapshotting (default: 5)")
    args = parser.parse_args()

    result = snapshot(args.host, args.port, args.resume_secs)
    print(f"\nSnapshot complete. {len(result.get('oam',{}).get('visible',[]))} visible sprites.")

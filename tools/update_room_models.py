#!/usr/bin/env python3
"""Update room OBJs and C++ model headers from obj/room.obj.

Runs the full pipeline so that editing obj/room.obj and running this script
produces headers that render correctly in the room viewer:
  1. split_room_obj.py: split room.obj -> room_shell.obj, table.obj, chair.obj
     (Table/Chair centered at origin.)
  2. obj_to_header.py for each with shared palette, correct scales (room 2, furniture 3),
     and --flip-z so models render right-side up (engine Z is down).

Usage (from repo root):
    python tools/update_room_models.py           # OBJs + headers (default)
    python tools/update_room_models.py --objs-only   # Only update OBJs, do not touch headers
"""

import argparse
import os
import subprocess
import sys


REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OBJ_DIR = os.path.join(REPO_ROOT, "obj")
MODELS_INCLUDE = os.path.join(REPO_ROOT, "include", "models")
TOOLS_DIR = os.path.join(REPO_ROOT, "tools")

SHARED_PALETTE = (
    "floor_light,floor_dark,wall,wall_shadow,window_glass,window_frame,"
    "table_wood,chair_fabric,chair_frame"
)


def main():
    parser = argparse.ArgumentParser(
        description="Update room OBJs and C++ headers from room.obj."
    )
    parser.add_argument(
        "--objs-only",
        action="store_true",
        help="Only run split_room_obj.py; do not overwrite include/models headers.",
    )
    args = parser.parse_args()

    os.chdir(REPO_ROOT)

    gen = subprocess.run(
        [sys.executable, os.path.join(TOOLS_DIR, "split_room_obj.py"),
         "--input-obj", os.path.join(OBJ_DIR, "room.obj"),
         "--output-dir", OBJ_DIR],
        cwd=REPO_ROOT,
    )
    if gen.returncode != 0:
        return gen.returncode

    if args.objs_only:
        print("OBJs updated. Headers unchanged (run without --objs-only to regenerate headers).")
        return 0

    obj_to_header = os.path.join(TOOLS_DIR, "obj_to_header.py")
    mtl = os.path.join(OBJ_DIR, "room.mtl")
    ROOM_SCALE = 2.0
    FURNITURE_SCALE = 3.0

    room_obj = os.path.join(OBJ_DIR, "room_shell.obj")
    if not os.path.isfile(room_obj):
        print("Error: room_shell.obj not found after split_room_obj.py")
        return 1
    subprocess.run(
        [sys.executable, obj_to_header,
         room_obj, mtl, os.path.join(MODELS_INCLUDE, "str_model_3d_items_room.h"),
         "--name", "room", "--scale", str(ROOM_SCALE), "--palette", SHARED_PALETTE, "--flip-z"],
        cwd=REPO_ROOT, check=True,
    )
    subprocess.run(
        [sys.executable, obj_to_header,
         os.path.join(OBJ_DIR, "table.obj"), mtl,
         os.path.join(MODELS_INCLUDE, "str_model_3d_items_table.h"),
         "--name", "table", "--scale", str(FURNITURE_SCALE),
         "--no-colors", "--palette", SHARED_PALETTE, "--flip-z"],
        cwd=REPO_ROOT, check=True,
    )
    subprocess.run(
        [sys.executable, obj_to_header,
         os.path.join(OBJ_DIR, "chair.obj"), mtl,
         os.path.join(MODELS_INCLUDE, "str_model_3d_items_chair.h"),
         "--name", "chair", "--scale", str(FURNITURE_SCALE),
         "--no-colors", "--palette", SHARED_PALETTE, "--flip-z"],
        cwd=REPO_ROOT, check=True,
    )
    print("Room models updated. Rebuild the project.")
    return 0


if __name__ == "__main__":
    sys.exit(main() or 0)

# Clangd LSP Setup for Butano GBA Project

This document describes the clangd LSP configuration for the stranded Butano GBA project.

## Installation

The clangd extension is already installed. To complete setup:

1. Reload Cursor window (Ctrl+Shift+P > "Developer: Reload Window")
2. Open a C++ file from the project (e.g., [src/main.cpp](src/main.cpp))
3. Wait for clangd to index the codebase (may take 1-2 minutes)

## Configuration Files

### [.clangd](.clangd)
Project-specific clangd configuration with:
- ARM7TDMI target flags (`-mcpu=arm7tdmi`, `-mtune=arm7tdmi`, `-mthumb`)
- C++23 standard
- Include paths for devkitARM and Butano (including architecture-specific paths)
- Clang-tidy checks for code quality
- Diagnostics for unused includes disabled to reduce noise

### [.vscode/settings.json](.vscode/settings.json)
Workspace settings for Cursor/VSCode:
- Points to clangd binary at `c:\Users\numap\AppData\Roaming\Cursor\User\globalStorage\llvm-vs-code-extensions.vscode-clangd\install\21.1.8\clangd_21.1.8\bin\clangd.exe`
- Enables background indexing, clang-tidy, and detailed completions

### [.vscode/c_cpp_properties.json](.vscode/c_cpp_properties.json)
IntelliSense configuration (for C/C++ extension):
- Corrected compiler path to `D:/devkitPro/devkitARM/bin/arm-none-eabi-gcc.exe`
- Added devkitARM include paths (including architecture-specific paths)
- Added GBA and ARM7 defines

### [compile_commands.json](compile_commands.json)
Compilation database for clangd (auto-generated, in .gitignore)

## Important Paths

**devkitARM**: `D:/devkitPro/devkitARM/`
- Compiler: `D:/devkitPro/devkitARM/bin/arm-none-eabi-gcc.exe` (v15.2.0)
- System includes: `D:/devkitPro/devkitARM/arm-none-eabi/include`
- C++ std library: `D:/devkitPro/devkitARM/arm-none-eabi/include/c++/15.2.0`
- Architecture-specific includes: `D:/devkitPro/devkitARM/arm-none-eabi/include/c++/15.2.0/arm-none-eabi`
- GCC lib includes: `D:/devkitPro/devkitARM/lib/gcc/arm-none-eabi/15.2.0/include`

**Butano**:
- Main headers: `butano/butano/include/` (316+ headers)
- Common headers: `butano/common/include/`

**Project**:
- Source: `src/`, `src/core/`, `src/actors/`
- Headers: `include/`

## Troubleshooting

### Clangd shows "file not found" errors
1. Open the Output panel (Ctrl+Shift+U)
2. Select "clangd" from the dropdown
3. Check for errors about missing include paths
4. Verify `D:/devkitPro` exists and contains devkitARM
5. Restart clangd: Ctrl+Shift+P > "clangd: Restart"

### Code completion not working
1. Ensure clangd is running (check Output panel > clangd)
2. Check that [.clangd](.clangd) exists in the project root
3. Verify compile_commands.json is present and valid
4. Try "clangd: Restart" from the command palette (Ctrl+Shift+P)

### IntelliSense (C/C++ extension) doesn't work
1. The C/C++ extension uses [c_cpp_properties.json](.vscode/c_cpp_properties.json)
2. Verify compilerPath points to the correct devkitARM location (`D:/devkitPro`)
3. Check that all include paths are correct
4. Click the IntelliSense status bar icon and select "Rebuild IntelliSense Index"

### "bits/c++config.h not found" error
This should be fixed now. The correct include path is:
```
D:/devkitPro/devkitARM/arm-none-eabi/include/c++/15.2.0/arm-none-eabi/bits/c++config.h
```

If you still see this error:
1. Restart clangd (Ctrl+Shift+P > "clangd: Restart")
2. Wait for re-indexing to complete

## Known Issues

### Butano API errors in diagnostics
Some errors reported by clangd are actual issues with the code or Butano library API usage:
- `bn::fixed_point` constructor with two arguments - Check Butano documentation for correct usage
- `bn::vector` methods like `push_back`, `data`, `size` - Verify Butano vector API
- Missing `<compare>` header for `std::strong_ordering` - Add `#include <compare>` or use Butano's comparison operators

These are **not** clangd configuration issues - they're code issues that should be addressed in the source files.

### Too many errors
If clangd reports "Too many errors emitted, stopping now":
1. Check the first few errors to identify the root cause
2. Fix configuration issues first (like missing includes)
3. Then address actual code errors

## Generating compile_commands.json

The current [compile_commands.json](compile_commands.json) is a minimal version covering key files. To regenerate for all files:

```bash
# Using Bear (Linux/macOS only):
bear -- make clean && make -j8

# Manual method (works on Windows):
1. Run: make clean && make -Bn > make_commands.txt
2. Parse make_commands.txt to extract compile commands
3. Convert to JSON format matching compile_commands.json
4. Ensure all include paths are present and correct
```

## Clangd Configuration Notes

**Important**: The `.clangd` file uses architecture-specific include paths that may need to be adjusted if you update devkitARM.

**Key flags explained**:
- `--target=arm-none-eabi`: Sets the target architecture
- `--gcc-toolchain=D:/devkitPro/devkitARM`: Tells clangd where to find GCC libraries
- `-mthumb`: Generates Thumb code (16-bit) for ARM7TDMI
- `-std=c++23`: Uses C++23 standard
- `-fno-exceptions -fno-rtti`: Butano doesn't use exceptions or RTTI

## Additional Resources

- [Butano Documentation](https://github.com/GValiente/butano)
- [Clangd Documentation](https://clangd.llvm.org/)
- [devkitARM Documentation](https://devkitpro.org/wiki/devkitARM)
- [GNU C++ Library Manual](https://gcc.gnu.org/onlinedocs/libstdc++/)

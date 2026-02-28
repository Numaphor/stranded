# Codebase Documentation Usage

When working with this Butano GBA project, **always use the codebase documentation in `.planning/` first** -- especially `.planning/codebase/` -- before making changes. Do not skip or ignore `.planning/`.

## Grounding Requirements

Every agent in this project must ground itself before modifying code:

1. **`.planning/`** -- read `.planning/codebase/` (ARCHITECTURE.md, STRUCTURE.md, CONVENTIONS.md, QUALITY_AND_TESTING.md as relevant). This is mandatory, not optional.
2. **`/butano`** -- browse `butano/butano/include/` headers for authoritative Butano API surface. Use the submodule as the single source of truth for types, signatures, and constraints.
3. **Web** -- consult the [Butano docs](https://gvaliente.github.io/butano/) or GitHub issues when the local headers are insufficient.

## Before Modifying Code

1. **Check existing patterns** in `CONVENTIONS.md`:
   - Naming conventions (snake_case files, PascalCase classes)
   - Include organization (project headers first, then Butano)
   - Memory management patterns (manual new/delete with RAII)

2. **Understand architecture** from `ARCHITECTURE.md`:
   - Scene-based game loop pattern
   - Entity-component design
   - Fixed-point arithmetic usage

3. **Know where to add code** from `STRUCTURE.md`:
   - New entities -> `src/actors/`
   - Core systems -> `src/core/`
   - Headers -> `include/str_*.h`

## For Specific Changes

**New Features:**
- Follow Butano constraints: no floats, use `bn::fixed`
- Check memory limits: IWRAM vs EWRAM usage
- Follow existing patterns in similar classes

**Bug Fixes:**
- Check `QUALITY_AND_TESTING.md` for known fragile areas
- Be careful with `World::execute()` method (500+ lines)
- Test manual memory management carefully

**Performance:**
- Review performance bottlenecks in `QUALITY_AND_TESTING.md`
- Consider GBA hardware limits
- Use appropriate Butano containers

## Critical References

- Player class: `include/str_player.h` (405+ lines, complex state machine)
- World update: `src/core/world.cpp` (main game loop)
- Constants: `include/str_constants.h`
- Build config: `Makefile` (Butano integration)

## Testing

- Use mGBA emulator (included in `tools/mGBA-0.10.5-win64/`)
- No automated tests -- manual testing required
- Check `QUALITY_AND_TESTING.md` for testing approach

## Memory Safety

- GBA has strict memory constraints
- Use `bn::optional` for resource management
- Manual new/delete requires careful exception handling
- Monitor IWRAM/EWRAM usage with Butano tools

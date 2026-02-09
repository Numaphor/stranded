---
name: map-codebase
description: Analyze an existing codebase and produce structured documentation in .planning/codebase/. Use when the user says "map the codebase", "analyze the project", "understand this codebase", or before major planning on a brownfield project. Produces 7 documents covering stack, architecture, structure, conventions, testing, integrations, and concerns.
---

# Map Codebase

Systematically explore the current project and write 7 structured analysis documents to `.planning/codebase/`. These documents serve as persistent reference for future planning, coding, and review tasks.

Adapted from the GSD (Get Shit Done) codebase mapping system.

## When to Use

- **Brownfield projects** — understand existing code before adding features
- **Onboarding** — build a navigable map of an unfamiliar codebase
- **Before refactoring** — document current state before changing it
- **Refresh** — re-run after significant changes to keep docs current

**Skip when:** greenfield with no code yet, or trivial projects (<5 files).

## Output

Seven documents in `.planning/codebase/`:

| Document | Focus Area | Contents |
|---|---|---|
| STACK.md | tech | Languages, runtime, frameworks, key dependencies |
| INTEGRATIONS.md | tech | External APIs, databases, auth, deployment |
| ARCHITECTURE.md | arch | Patterns, layers, data flow, entry points |
| STRUCTURE.md | arch | Directory layout, naming, where to add new code |
| CONVENTIONS.md | quality | Code style, naming, error handling, imports |
| TESTING.md | quality | Test framework, patterns, mocking, coverage |
| CONCERNS.md | concerns | Tech debt, bugs, security, performance, fragile areas |

## Process

### Step 1 — Check Existing

Check if `.planning/codebase/` already exists:

```bash
ls -la .planning/codebase/ 2>/dev/null
```

**If it exists:** Ask the user:
1. **Refresh** — delete and remap from scratch
2. **Update** — keep existing, only regenerate specific documents
3. **Skip** — use existing map as-is

**If it doesn't exist:** Continue.

### Step 2 — Create Directory

```bash
mkdir -p .planning/codebase
```

### Step 3 — Run Four Analysis Passes

Run these sequentially. For each pass: explore thoroughly using the listed commands, then write the document(s) immediately using the templates in `references/templates.md`.

**Always include file paths** in backticks throughout every document. `src/main.cpp` is useful; "the main file" is not.

#### Pass 1: Tech (→ STACK.md, INTEGRATIONS.md)

Explore:
```bash
# Build system and dependencies
ls Makefile CMakeLists.txt package.json requirements.txt Cargo.toml go.mod pyproject.toml meson.build *.cabal 2>/dev/null
cat Makefile 2>/dev/null | head -80
cat package.json 2>/dev/null | head -100
cat CMakeLists.txt 2>/dev/null | head -80

# Config files (list only — NEVER read .env contents)
ls -la *.config.* tsconfig.json .nvmrc .python-version .clang-format .editorconfig 2>/dev/null
ls .env* 2>/dev/null  # Note existence only

# Find SDK/library imports
grep -rn "#include\|import\|require\|use " src/ --include="*.cpp" --include="*.h" --include="*.hpp" --include="*.ts" --include="*.py" --include="*.rs" 2>/dev/null | head -60
```

Write STACK.md and INTEGRATIONS.md using templates.

#### Pass 2: Architecture (→ ARCHITECTURE.md, STRUCTURE.md)

Explore:
```bash
# Directory structure
find . -type d -not -path '*/node_modules/*' -not -path '*/.git/*' -not -path '*/build/*' -not -path '*/__pycache__/*' | sort | head -60

# Entry points
ls src/main.* src/index.* src/app.* app/page.* main.* 2>/dev/null

# Module/include structure
grep -rn "#include\|^import\|^from " src/ --include="*.cpp" --include="*.h" --include="*.hpp" --include="*.ts" --include="*.py" 2>/dev/null | head -100

# File sizes (find complex areas)
find src/ -name "*.cpp" -o -name "*.h" -o -name "*.ts" -o -name "*.py" -o -name "*.rs" 2>/dev/null | xargs wc -l 2>/dev/null | sort -rn | head -20
```

Write ARCHITECTURE.md and STRUCTURE.md using templates.

#### Pass 3: Quality (→ CONVENTIONS.md, TESTING.md)

Explore:
```bash
# Linting/formatting config
ls .eslintrc* .prettierrc* .clang-format .clang-tidy .editorconfig biome.json rustfmt.toml 2>/dev/null
cat .clang-format 2>/dev/null
cat .editorconfig 2>/dev/null

# Test files and config
ls jest.config.* vitest.config.* pytest.ini 2>/dev/null
find . -name "*.test.*" -o -name "*.spec.*" -o -name "*_test.*" -o -name "test_*" 2>/dev/null | head -30

# Sample source files (read 2-3 representative files to extract conventions)
ls src/*.cpp src/*.h src/**/*.ts src/**/*.py 2>/dev/null | head -10
```

Read 2-3 representative source files to identify naming patterns, error handling, comment style, and module organization. Write CONVENTIONS.md and TESTING.md.

#### Pass 4: Concerns (→ CONCERNS.md)

Explore:
```bash
# TODO/FIXME/HACK comments
grep -rn "TODO\|FIXME\|HACK\|XXX\|WORKAROUND" src/ 2>/dev/null | head -50

# Large files (complexity hotspots)
find src/ -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.ts" -o -name "*.py" \) | xargs wc -l 2>/dev/null | sort -rn | head -20

# Empty returns/stubs
grep -rn "return nullptr\|return null\|return \[\]\|return {}\|pass$\|todo!\|unimplemented!" src/ 2>/dev/null | head -30

# Dependency age (if applicable)
cat package.json 2>/dev/null | grep -E '"dependencies"|"devDependencies"' -A 50 | head -60
```

Write CONCERNS.md using template.

### Step 4 — Verify Output

```bash
ls -la .planning/codebase/
wc -l .planning/codebase/*.md
```

All 7 documents must exist. Flag any that are empty or missing.

### Step 5 — Scan for Secrets

**CRITICAL security check** before committing:

```bash
grep -E '(sk-[a-zA-Z0-9]{20,}|sk_live_|sk_test_|ghp_[a-zA-Z0-9]{36}|gho_[a-zA-Z0-9]{36}|glpat-|AKIA[A-Z0-9]{16}|xox[baprs]-|-----BEGIN.*PRIVATE KEY|eyJ[a-zA-Z0-9_-]+\.eyJ)' .planning/codebase/*.md 2>/dev/null
```

**If matches found:** STOP. Show the user what was detected. Do not commit until confirmed safe or cleaned.

### Step 6 — Commit

```bash
git add .planning/codebase/
git commit -m "docs: map existing codebase"
```

### Step 7 — Summary

Show the user what was created:

```
Codebase mapping complete.

Created .planning/codebase/:
- STACK.md (N lines)
- INTEGRATIONS.md (N lines)
- ARCHITECTURE.md (N lines)
- STRUCTURE.md (N lines)
- CONVENTIONS.md (N lines)
- TESTING.md (N lines)
- CONCERNS.md (N lines)
```

## Document Quality Rules

These documents are reference material for future AI sessions working on this codebase. Quality matters.

1. **File paths are critical** — always use backticks: `src/engine/player.cpp`. The agent reading this later needs to navigate directly to files.
2. **Be prescriptive, not descriptive** — "Use snake_case for functions" helps future coding. "Some functions use snake_case" doesn't.
3. **Patterns over lists** — show HOW things are done with short code examples, not just WHAT exists.
4. **Current state only** — describe what IS, not what WAS or what you considered.
5. **STRUCTURE.md must answer "where do I put new code?"** — include guidance for adding files, not just describing what exists.

## Forbidden Files

**NEVER read or quote contents from:**
- `.env`, `.env.*` — environment secrets
- `*.pem`, `*.key`, `*.p12` — private keys
- `credentials.*`, `secrets.*`, `*secret*` — credential files
- `id_rsa*`, `id_ed25519*` — SSH keys
- `.npmrc`, `.pypirc`, `.netrc` — auth tokens

**If encountered:** note their EXISTENCE only ("`.env` file present"). Never include values.

Your output gets committed to git. Leaked secrets = security incident.

## Templates

See `references/templates.md` for the document structure of each of the 7 output files.
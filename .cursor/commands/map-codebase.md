# Map Codebase

Analyze the existing codebase and produce structured documentation in `.planning/codebase/`.

## Step 1 — Check Existing

```bash
ls -la .planning/codebase/ 2>/dev/null
```

**If it exists:** Ask the user: (1) Refresh — delete and remap, (2) Update — regenerate specific documents, (3) Skip.

**If it doesn't exist:** Continue.

## Step 2 — Create Directory

```bash
mkdir -p .planning/codebase
```

## Step 3 — Spawn Four Mapper Subagents

Delegate to the **codebase-mapper** subagent four times in parallel, once per focus area. Each subagent runs in its own context, explores the codebase for its focus, and writes documents directly.

Spawn these four tasks:

1. `codebase-mapper` with prompt: **"Focus area: tech. Explore the codebase and write STACK.md and INTEGRATIONS.md to .planning/codebase/."**
2. `codebase-mapper` with prompt: **"Focus area: arch. Explore the codebase and write ARCHITECTURE.md and STRUCTURE.md to .planning/codebase/."**
3. `codebase-mapper` with prompt: **"Focus area: quality. Explore the codebase and write CONVENTIONS.md and TESTING.md to .planning/codebase/."**
4. `codebase-mapper` with prompt: **"Focus area: concerns. Explore the codebase and write CONCERNS.md to .planning/codebase/."**

**If subagents are not available:** Fall back to the **map-codebase** skill, which runs the same four analysis passes sequentially in the current context.

## Step 4 — Verify Output

After all subagents complete:

```bash
ls -la .planning/codebase/
wc -l .planning/codebase/*.md
```

All 7 documents must exist: STACK.md, INTEGRATIONS.md, ARCHITECTURE.md, STRUCTURE.md, CONVENTIONS.md, TESTING.md, CONCERNS.md.

## Step 5 — Scan for Secrets

**CRITICAL** — before committing:

```bash
grep -rE '(sk-[a-zA-Z0-9]{20,}|sk_live_|sk_test_|ghp_[a-zA-Z0-9]{36}|AKIA[A-Z0-9]{16}|xox[baprs]-|-----BEGIN.*PRIVATE KEY|eyJ[a-zA-Z0-9_-]+\.eyJ)' .planning/codebase/*.md 2>/dev/null
```

**If matches found:** STOP. Show the user. Do not commit until cleaned.

## Step 6 — Commit

```bash
git add .planning/codebase/
git commit -m "docs: map existing codebase"
```

## Step 7 — Summary

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
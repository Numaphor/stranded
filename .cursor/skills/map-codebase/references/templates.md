# Codebase Document Templates

Templates for the 7 documents produced by the map-codebase skill. Fill in brackets with findings from exploration. Use "Not detected" or "Not applicable" for sections that don't apply.

Always include file paths in backticks. Always include the analysis date.

---

## STACK.md

```markdown
# Technology Stack

**Analysis Date:** [YYYY-MM-DD]

## Languages

**Primary:**
- [Language] [Version] - [Where used]

**Secondary:**
- [Language] [Version] - [Where used]

## Runtime

**Environment:**
- [Runtime] [Version]

**Package Manager / Build System:**
- [Tool] [Version]
- Lockfile: [present/missing]

## Frameworks

**Core:**
- [Framework] [Version] - [Purpose]

**Testing:**
- [Framework] [Version] - [Purpose]

**Build/Dev:**
- [Tool] [Version] - [Purpose]

## Key Dependencies

[Only 5-10 most important — not every dependency]

**Critical:**
- [Package] [Version] - [Why it matters]

**Infrastructure:**
- [Package] [Version] - [Purpose]

## Configuration

**Environment:**
- [How configured: env vars, config files, Makefile flags]
- [Key configs required]

**Build:**
- [Build config files and their purpose]

## Platform Requirements

**Development:**
- [OS, toolchain, additional requirements]

**Target:**
- [Deployment target / hardware target]

---
*Stack analysis: [date]*
```

---

## INTEGRATIONS.md

```markdown
# External Integrations

**Analysis Date:** [YYYY-MM-DD]

## APIs & External Services

**[Category]:**
- [Service] - [What it's used for]
  - SDK/Client: [package or library]
  - Auth: [env var name or method]

## Data Storage

**Databases:**
- [Type/Provider] - Client: [ORM/driver] - Connection: [env var]

**File Storage:**
- [Service or "Local filesystem only"]

**Caching:**
- [Service or "None"]

## Authentication & Identity

- [Service or "Custom"] - Implementation: [approach]

## Monitoring & Observability

- Error Tracking: [Service or "None"]
- Logging: [Approach]

## CI/CD & Deployment

- Hosting: [Platform or "N/A"]
- CI Pipeline: [Service or "None"]

## Environment Configuration

**Required env vars / config:**
- [List critical configuration]

## Webhooks & Callbacks

- Incoming: [Endpoints or "None"]
- Outgoing: [Endpoints or "None"]

---
*Integration audit: [date]*
```

---

## ARCHITECTURE.md

```markdown
# Architecture

**Analysis Date:** [YYYY-MM-DD]

## Pattern Overview

**Overall:** [Pattern name: e.g., "game loop with scene stack", "MVC", "layered monolith"]

**Key Characteristics:**
- [Characteristic 1]
- [Characteristic 2]
- [Characteristic 3]

## Layers

**[Layer Name]:**
- Purpose: [What this layer does]
- Location: `[path]`
- Contains: [Types of code]
- Depends on: [What it imports/uses]
- Used by: [What depends on it]

[Repeat for each layer]

## Data Flow

**[Flow Name]:** (e.g., "Game Update Cycle", "Request → Response")

1. [Step 1] (`[file]`)
2. [Step 2] (`[file]`)
3. [Step 3] (`[file]`)

**State Management:**
- [How application state is handled]

## Key Abstractions

**[Abstraction Name]:**
- Purpose: [What it represents]
- Examples: `[file paths]`
- Pattern: [How it works]

## Entry Points

**[Entry Point]:**
- Location: `[path]`
- Triggers: [What invokes it]
- Responsibilities: [What it initializes/orchestrates]

## Error Handling

**Strategy:** [Approach: exceptions, error codes, asserts, Result types]

**Patterns:**
- [Pattern with file example]

## Cross-Cutting Concerns

- Logging: [Approach]
- Validation: [Approach]
- Resource management: [Approach]

---
*Architecture analysis: [date]*
```

---

## STRUCTURE.md

```markdown
# Codebase Structure

**Analysis Date:** [YYYY-MM-DD]

## Directory Layout

```
[project-root]/
├── [dir]/          # [Purpose]
├── [dir]/          # [Purpose]
├── [dir]/          # [Purpose]
└── [file]          # [Purpose]
```

## Directory Purposes

**[Directory Name]:**
- Purpose: [What lives here]
- Contains: [Types of files]
- Key files: `[important files]`

[Repeat for each significant directory]

## Key File Locations

**Entry Points:**
- `[path]`: [Purpose]

**Configuration:**
- `[path]`: [Purpose]

**Core Logic:**
- `[path]`: [Purpose]

**Assets:**
- `[path]`: [Purpose]

## Naming Conventions

**Files:** [Pattern and example]
**Directories:** [Pattern and example]
**Classes/Types:** [Pattern and example]

## Where to Add New Code

**New feature:**
- Primary code: `[path pattern]`
- Tests: `[path pattern]`

**New module/component:**
- Implementation: `[path pattern]`
- Header/interface: `[path pattern]`

**New asset:**
- Location: `[path pattern]`
- Descriptor: `[if applicable]`

**Utilities:**
- Shared helpers: `[path]`

## Special Directories

**[Directory]:**
- Purpose: [What it contains]
- Generated: [Yes/No]
- Committed: [Yes/No]

---
*Structure analysis: [date]*
```

---

## CONVENTIONS.md

```markdown
# Coding Conventions

**Analysis Date:** [YYYY-MM-DD]

## Naming Patterns

- **Files:** [Pattern — e.g., snake_case.cpp]
- **Functions:** [Pattern — e.g., snake_case()]
- **Variables:** [Pattern — e.g., snake_case]
- **Types/Classes:** [Pattern — e.g., PascalCase]
- **Constants:** [Pattern — e.g., UPPER_SNAKE_CASE]

## Code Style

**Formatting:**
- [Tool: clang-format / prettier / none]
- [Key settings: indent, braces, line length]

**Linting:**
- [Tool and key rules]

## Import / Include Organization

**Order:**
1. [First group — e.g., system headers]
2. [Second group — e.g., library headers]
3. [Third group — e.g., project headers]

**Path style:** [Relative / absolute / aliases]

## Error Handling

**Pattern:**
```[language]
[Show actual error handling pattern from codebase]
```

## Logging / Debug Output

- Framework: [Tool or "printf/cout"]
- Pattern: [When and how to log]

## Comments

- When to comment: [Guidelines observed]
- Doc comments: [Style — Doxygen, JSDoc, docstring, none]

## Function Design

- Size: [Typical length / guidelines]
- Parameters: [Patterns — e.g., const ref, pointer, value]
- Return values: [Patterns]

## Module Design

- Exports: [Pattern — e.g., one class per header]
- Encapsulation: [Approach]

---
*Convention analysis: [date]*
```

---

## TESTING.md

```markdown
# Testing Patterns

**Analysis Date:** [YYYY-MM-DD]

## Test Framework

**Runner:** [Framework] [Version]
**Config:** `[config file path]`

**Run Commands:**
```bash
[command]              # Run all tests
[command]              # Single test
[command]              # Coverage (if available)
```

## Test File Organization

- **Location:** [Co-located with source / separate test directory]
- **Naming:** [Pattern — e.g., test_*.cpp, *.test.ts]
- **Structure:**
```
[Directory pattern showing where tests live]
```

## Test Structure

**Suite pattern:**
```[language]
[Show actual test structure from codebase]
```

## Mocking

- **Framework:** [Tool or "manual"]
- **What to mock:** [Guidelines]
- **Pattern:**
```[language]
[Show actual mocking pattern if present]
```

## Fixtures and Test Data

- **Location:** `[where test data lives]`
- **Pattern:** [How test data is created/loaded]

## Coverage

- **Requirements:** [Target or "None enforced"]
- **Gaps:** [Known untested areas]

## Test Types Present

- **Unit tests:** [Scope and approach]
- **Integration tests:** [Scope or "Not present"]
- **E2E tests:** [Framework or "Not present"]
- **Manual testing:** [Approach — e.g., "run in emulator"]

---
*Testing analysis: [date]*
```

---

## CONCERNS.md

```markdown
# Codebase Concerns

**Analysis Date:** [YYYY-MM-DD]

## Tech Debt

**[Area/Component]:**
- Issue: [What's the shortcut/workaround]
- Files: `[file paths]`
- Impact: [What breaks or degrades]
- Fix approach: [How to address it]

## Known Bugs

**[Bug description]:**
- Symptoms: [What happens]
- Files: `[file paths]`
- Trigger: [How to reproduce]
- Workaround: [If any]

## Security Considerations

**[Area]:**
- Risk: [What could go wrong]
- Files: `[file paths]`
- Current mitigation: [What's in place]
- Recommendation: [What to add]

## Performance Bottlenecks

**[Slow operation]:**
- Problem: [What's slow]
- Files: `[file paths]`
- Cause: [Why]
- Improvement: [How to speed up]

## Fragile Areas

**[Component/Module]:**
- Files: `[file paths]`
- Why fragile: [What makes it break easily]
- Safe modification: [How to change without breaking]
- Test coverage: [Gaps]

## Scaling Limits

**[Resource/System]:**
- Current capacity: [Numbers]
- Limit: [Where it breaks]
- Scaling path: [How to increase]

## Dependencies at Risk

**[Package/Library]:**
- Risk: [Outdated / unmaintained / breaking changes]
- Impact: [What depends on it]
- Migration: [Alternative or upgrade path]

## Missing Critical Features

**[Feature gap]:**
- Problem: [What's missing]
- Blocks: [What can't be done without it]

## Test Coverage Gaps

**[Untested area]:**
- What's not tested: [Specific functionality]
- Files: `[file paths]`
- Risk: [What could break unnoticed]
- Priority: [High/Medium/Low]

---
*Concerns audit: [date]*
```
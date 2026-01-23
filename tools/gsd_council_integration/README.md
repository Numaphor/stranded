# GSD + LLM Council Integration

This module integrates [Get Shit Done (GSD)](https://github.com/glittercowboy/get-shit-done) meta-prompting and context engineering system with [LLM Council](https://github.com/shuntacurosu/llm_council_skill) multi-agent orchestration.

## Overview

The integration combines the strengths of both systems:

- **LLM Council**: Multi-LLM orchestration with peer review and consensus building
- **GSD**: Context engineering, atomic task planning, and quality optimization

Together, they enable:

1. **GSD-style planning for councils**: Create atomic, executable plans that the council can discuss and implement
2. **Context-aware council sessions**: Track decisions and state across sessions to prevent quality degradation
3. **Verified council outputs**: Use GSD's verification methodology to validate council work

## Installation

Both submodules need to be initialized:

```bash
git submodule update --init --recursive
```

## Usage

### GSD Plan Adapter

Create GSD-style planning prompts for council members:

```python
from tools.gsd_council_integration import get_gsd_adapter

adapter = get_gsd_adapter()

# Create a planning prompt
planning_prompt = adapter.create_council_planning_prompt(
    user_query="Implement user authentication with JWT",
    context="Using httpOnly cookies for token storage"
)

# Parse a GSD-format plan
plan = adapter.parse_plan_from_markdown(plan_markdown)

# Get GSD agent templates for council use
planner_template = adapter.get_planner_prompt()
executor_template = adapter.get_executor_prompt()
```

### Context Manager

Manage context to prevent quality degradation:

```python
from tools.gsd_council_integration import get_context_manager

context_mgr = get_context_manager()

# Track project state
context_mgr.update_position("Phase 1: Authentication")
context_mgr.add_decision("Use JWT with httpOnly cookies")

# Monitor quality status
status = context_mgr.get_quality_status()
if status["should_checkpoint"]:
    print("Context usage high - consider checkpointing")

# Create minimal context for tasks
minimal_context = context_mgr.create_minimal_context(
    task_description="Implement login endpoint",
    relevant_files=["src/auth.py", "src/config.py"]
)
```

### Verifier

Verify council outputs meet quality standards:

```python
from tools.gsd_council_integration import get_verifier

verifier = get_verifier()

# Verify plan structure
plan_report = verifier.verify_plan_structure(plan)
print(plan_report.to_markdown())

# Verify council consensus
consensus_result = verifier.verify_council_consensus(
    responses=stage1_results,
    rankings=stage2_results
)

# Create verification prompts
verify_prompt = verifier.create_verification_prompt(task)
```

## Module Structure

```
tools/gsd_council_integration/
├── __init__.py          # Exports and paths
├── plan_adapter.py      # GSD planning integration
├── context_manager.py   # Context engineering
├── verifier.py          # Output verification
└── README.md            # This file
```

## Updating Submodules

Update each submodule independently:

```bash
# Update GSD
cd get-shit-done
git pull origin main
cd ..
git add get-shit-done
git commit -m "Update GSD"

# Update LLM Council
cd llm_council_skill
git pull origin main
cd ..
git add llm_council_skill
git commit -m "Update LLM Council"
```

## Key Concepts

### GSD Context Quality Tiers

| Context Usage | Quality Tier | Recommendation |
|---------------|-------------|----------------|
| 0-30% | Peak | Thorough, comprehensive work |
| 30-50% | Good | Solid, confident execution |
| 50-70% | Degrading | Consider checkpointing |
| 70%+ | Poor | Must checkpoint - quality compromised |

### Atomic Task Guidelines

- Each plan should have 2-3 tasks maximum
- Tasks should complete in a single focused session
- Include verification criteria for each task
- Use XML format for structured task definitions

### Council + GSD Workflow

1. **Plan**: Use GSD adapter to create atomic task plans
2. **Discuss**: Council members review and discuss the plan
3. **Execute**: Members implement tasks in their worktrees
4. **Review**: Peer review of implementations
5. **Synthesize**: Chairman integrates best solutions
6. **Verify**: Use GSD verifier to validate outputs

## References

- [Get Shit Done Documentation](https://github.com/glittercowboy/get-shit-done)
- [LLM Council Documentation](https://github.com/shuntacurosu/llm_council_skill)

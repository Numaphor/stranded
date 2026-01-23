"""GSD Planning Adapter for LLM Council.

This module adapts GSD's planning methodology for use with LLM Council's
multi-agent architecture. It allows the council to use GSD-style atomic
task planning and verification.

The adapter reads GSD plan templates and transforms them into prompts
suitable for council member execution with peer review.
"""

import os
import re
from pathlib import Path
from typing import Dict, List, Optional, Any
from dataclasses import dataclass, field

from . import GSD_PATH


@dataclass
class GSDTask:
    """Represents a single GSD-style atomic task."""
    
    name: str
    task_type: str  # "auto", "manual", "checkpoint"
    files: List[str] = field(default_factory=list)
    action: str = ""
    verify: str = ""
    done: str = ""
    
    def to_prompt(self) -> str:
        """Convert task to a prompt string for council members."""
        prompt_parts = [f"## Task: {self.name}"]
        
        if self.files:
            prompt_parts.append(f"\n**Files:** {', '.join(self.files)}")
        
        if self.action:
            prompt_parts.append(f"\n**Action:**\n{self.action}")
        
        if self.verify:
            prompt_parts.append(f"\n**Verification:**\n{self.verify}")
        
        if self.done:
            prompt_parts.append(f"\n**Success Criteria:**\n{self.done}")
        
        return "\n".join(prompt_parts)


@dataclass
class GSDPlan:
    """Represents a GSD-style plan with tasks and metadata."""
    
    name: str
    phase: Optional[str] = None
    objective: str = ""
    context_files: List[str] = field(default_factory=list)
    tasks: List[GSDTask] = field(default_factory=list)
    success_criteria: List[str] = field(default_factory=list)
    
    def to_council_prompt(self) -> str:
        """Convert plan to a prompt suitable for council discussion."""
        prompt_parts = [f"# {self.name}"]
        
        if self.objective:
            prompt_parts.append(f"\n## Objective\n{self.objective}")
        
        if self.context_files:
            prompt_parts.append(
                f"\n## Context Files\n" + 
                "\n".join(f"- {f}" for f in self.context_files)
            )
        
        if self.tasks:
            prompt_parts.append("\n## Tasks")
            for task in self.tasks:
                prompt_parts.append(f"\n{task.to_prompt()}")
        
        if self.success_criteria:
            prompt_parts.append(
                f"\n## Success Criteria\n" +
                "\n".join(f"- {c}" for c in self.success_criteria)
            )
        
        return "\n".join(prompt_parts)


class GSDPlanAdapter:
    """Adapts GSD planning methodology for LLM Council usage.
    
    This adapter allows council members to:
    1. Generate GSD-style atomic plans
    2. Execute plans with peer review at each step
    3. Verify outputs against success criteria
    """
    
    def __init__(self, gsd_path: Optional[Path] = None):
        """Initialize the adapter with path to GSD submodule.
        
        Args:
            gsd_path: Path to GSD submodule. Defaults to ./get-shit-done
        """
        self.gsd_path = gsd_path or GSD_PATH
        self._templates_loaded = False
        self._agent_templates: Dict[str, str] = {}
        
    def _load_agent_template(self, agent_name: str) -> Optional[str]:
        """Load a GSD agent template by name.
        
        Args:
            agent_name: Name of the agent (e.g., "gsd-planner")
            
        Returns:
            Agent template content or None if not found
        """
        agent_file = self.gsd_path / "agents" / f"{agent_name}.md"
        if agent_file.exists():
            return agent_file.read_text()
        return None
    
    def get_planner_prompt(self) -> Optional[str]:
        """Get the GSD planner agent template."""
        return self._load_agent_template("gsd-planner")
    
    def get_executor_prompt(self) -> Optional[str]:
        """Get the GSD executor agent template."""
        return self._load_agent_template("gsd-executor")
    
    def get_verifier_prompt(self) -> Optional[str]:
        """Get the GSD verifier agent template."""
        return self._load_agent_template("gsd-verifier")
    
    def parse_plan_from_markdown(self, markdown: str) -> GSDPlan:
        """Parse a GSD-style plan from markdown content.
        
        Args:
            markdown: Raw markdown content of a PLAN.md file
            
        Returns:
            Parsed GSDPlan object
        """
        plan = GSDPlan(name="Parsed Plan")
        
        # Parse frontmatter
        frontmatter_match = re.search(r'^---\n(.*?)\n---', markdown, re.DOTALL)
        if frontmatter_match:
            frontmatter = frontmatter_match.group(1)
            for line in frontmatter.split('\n'):
                if ':' in line:
                    key, value = line.split(':', 1)
                    key = key.strip().lower()
                    value = value.strip()
                    if key == 'phase':
                        plan.phase = value
                    elif key == 'name':
                        plan.name = value
        
        # Parse objective
        objective_match = re.search(
            r'##?\s*Objective\s*\n(.*?)(?=\n##|\n<|\Z)', 
            markdown, 
            re.DOTALL | re.IGNORECASE
        )
        if objective_match:
            plan.objective = objective_match.group(1).strip()
        
        # Parse tasks
        task_matches = re.finditer(
            r'<task[^>]*type="(\w+)"[^>]*>\s*'
            r'<name>(.*?)</name>\s*'
            r'(?:<files>(.*?)</files>\s*)?'
            r'<action>(.*?)</action>\s*'
            r'(?:<verify>(.*?)</verify>\s*)?'
            r'(?:<done>(.*?)</done>\s*)?'
            r'</task>',
            markdown,
            re.DOTALL
        )
        
        for match in task_matches:
            task = GSDTask(
                task_type=match.group(1),
                name=match.group(2).strip(),
                files=[f.strip() for f in (match.group(3) or "").split(",") if f.strip()],
                action=match.group(4).strip(),
                verify=(match.group(5) or "").strip(),
                done=(match.group(6) or "").strip()
            )
            plan.tasks.append(task)
        
        return plan
    
    def create_council_planning_prompt(
        self, 
        user_query: str,
        context: Optional[str] = None
    ) -> str:
        """Create a planning prompt for council members using GSD methodology.
        
        This prompt instructs council members to create GSD-style atomic plans
        that can be reviewed by other council members.
        
        Args:
            user_query: The user's request or question
            context: Optional additional context
            
        Returns:
            Planning prompt for council members
        """
        prompt = f"""You are participating in a council session to plan implementation for the following request:

## Request
{user_query}

"""
        if context:
            prompt += f"""## Context
{context}

"""
        
        prompt += """## Your Task
Create an atomic, executable plan following GSD (Get Shit Done) methodology:

1. **Objective**: Clear statement of what will be achieved
2. **Tasks**: 2-3 atomic tasks, each with:
   - Name
   - Files to modify
   - Specific action to take
   - Verification criteria
   - Success criteria
3. **Success Criteria**: Measurable outcomes

Use XML format for tasks:
```xml
<task type="auto">
  <name>Task name</name>
  <files>file1.py, file2.py</files>
  <action>Specific implementation instructions</action>
  <verify>How to verify this task completed correctly</verify>
  <done>Success criteria for this task</done>
</task>
```

Be specific and atomic. Each task should complete in a single focused session.
"""
        return prompt
    
    def create_council_execution_prompt(
        self,
        plan: GSDPlan,
        task_index: int = 0
    ) -> str:
        """Create an execution prompt for a specific task from a plan.
        
        Args:
            plan: The GSD plan to execute
            task_index: Index of the task to execute
            
        Returns:
            Execution prompt for council members
        """
        if task_index >= len(plan.tasks):
            return "No more tasks to execute."
        
        task = plan.tasks[task_index]
        
        prompt = f"""## Executing Plan: {plan.name}

### Current Task: {task.name}

{task.to_prompt()}

### Instructions
1. Execute the task as specified
2. Make actual code changes where indicated
3. Verify your changes meet the success criteria
4. Report completion with any relevant notes
"""
        return prompt


def get_gsd_adapter() -> GSDPlanAdapter:
    """Get a configured GSD plan adapter instance.
    
    Returns:
        Configured GSDPlanAdapter
    """
    return GSDPlanAdapter()

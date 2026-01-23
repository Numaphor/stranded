"""GSD Context Manager for LLM Council.

This module manages context using GSD's context engineering principles
to prevent quality degradation as Claude fills its context window.

GSD defines context usage thresholds:
- 0-30%: Peak quality (thorough, comprehensive)
- 30-50%: Good quality (confident, solid work)
- 50-70%: Degrading quality (efficiency mode begins)
- 70%+: Poor quality (rushed, minimal)

The context manager helps council sessions stay within optimal bounds.
"""

import os
from pathlib import Path
from typing import Dict, List, Optional, Any
from dataclasses import dataclass, field
from datetime import datetime

from . import GSD_PATH


# Context management constants
DEFAULT_MAX_CONTEXT_FILES = 5  # Maximum files to include in minimal context
DEFAULT_MAX_FILE_SIZE = 10000  # Maximum characters per file before truncation


@dataclass
class ProjectState:
    """Represents the current state of a GSD-managed project.
    
    Corresponds to STATE.md in GSD methodology.
    """
    
    current_position: str = ""
    accumulated_decisions: List[str] = field(default_factory=list)
    blockers: List[str] = field(default_factory=list)
    concerns: List[str] = field(default_factory=list)
    last_updated: Optional[datetime] = None
    
    def to_markdown(self) -> str:
        """Convert state to markdown format."""
        lines = ["# Project State"]
        
        lines.append(f"\n## Current Position\n{self.current_position}")
        
        if self.accumulated_decisions:
            lines.append("\n## Decisions")
            for decision in self.accumulated_decisions:
                lines.append(f"- {decision}")
        
        if self.blockers:
            lines.append("\n## Blockers")
            for blocker in self.blockers:
                lines.append(f"- {blocker}")
        
        if self.concerns:
            lines.append("\n## Concerns")
            for concern in self.concerns:
                lines.append(f"- {concern}")
        
        if self.last_updated:
            lines.append(f"\n---\n*Last updated: {self.last_updated.isoformat()}*")
        
        return "\n".join(lines)


@dataclass
class ContextWindow:
    """Tracks context window usage for quality optimization."""
    
    max_tokens: int = 200000  # Claude's context window
    current_tokens: int = 0
    
    @property
    def usage_percent(self) -> float:
        """Calculate current context usage percentage."""
        return (self.current_tokens / self.max_tokens) * 100
    
    @property
    def quality_tier(self) -> str:
        """Determine quality tier based on context usage."""
        usage = self.usage_percent
        if usage <= 30:
            return "peak"
        elif usage <= 50:
            return "good"
        elif usage <= 70:
            return "degrading"
        else:
            return "poor"
    
    @property
    def should_checkpoint(self) -> bool:
        """Check if we should checkpoint before quality degrades."""
        return self.usage_percent >= 50


class GSDContextManager:
    """Manages context for council sessions using GSD principles.
    
    This manager ensures council sessions:
    1. Stay within optimal context bounds
    2. Track accumulated decisions and state
    3. Load only necessary context for each task
    4. Create checkpoints before quality degrades
    """
    
    def __init__(
        self, 
        project_root: Optional[Path] = None,
        gsd_path: Optional[Path] = None
    ):
        """Initialize the context manager.
        
        Args:
            project_root: Root directory of the project
            gsd_path: Path to GSD submodule
        """
        self.project_root = project_root or Path.cwd()
        self.gsd_path = gsd_path or GSD_PATH
        self.context_window = ContextWindow()
        self.project_state = ProjectState()
        self._planning_dir: Optional[Path] = None
        
    @property
    def planning_dir(self) -> Path:
        """Get or create the .planning directory."""
        if self._planning_dir is None:
            self._planning_dir = self.project_root / ".planning"
            self._planning_dir.mkdir(parents=True, exist_ok=True)
        return self._planning_dir
    
    def load_project_state(self) -> ProjectState:
        """Load project state from STATE.md.
        
        Returns:
            Current project state
        """
        state_file = self.planning_dir / "STATE.md"
        if state_file.exists():
            content = state_file.read_text()
            # Parse basic state from markdown
            # (simplified parser - full implementation would be more robust)
            self.project_state = ProjectState(
                current_position=self._extract_section(content, "Current Position"),
                accumulated_decisions=self._extract_list(content, "Decisions"),
                blockers=self._extract_list(content, "Blockers"),
                concerns=self._extract_list(content, "Concerns"),
                last_updated=datetime.now()
            )
        return self.project_state
    
    def save_project_state(self) -> None:
        """Save current project state to STATE.md."""
        self.project_state.last_updated = datetime.now()
        state_file = self.planning_dir / "STATE.md"
        state_file.write_text(self.project_state.to_markdown())
    
    def _extract_section(self, content: str, section: str) -> str:
        """Extract a section from markdown content."""
        import re
        pattern = rf'##?\s*{section}\s*\n(.*?)(?=\n##|\Z)'
        match = re.search(pattern, content, re.DOTALL | re.IGNORECASE)
        return match.group(1).strip() if match else ""
    
    def _extract_list(self, content: str, section: str) -> List[str]:
        """Extract a list section from markdown content."""
        section_content = self._extract_section(content, section)
        items = []
        for line in section_content.split('\n'):
            line = line.strip()
            if line.startswith('- '):
                items.append(line[2:])
        return items
    
    def create_minimal_context(
        self,
        task_description: str,
        relevant_files: Optional[List[str]] = None,
        max_files: int = DEFAULT_MAX_CONTEXT_FILES,
        max_file_size: int = DEFAULT_MAX_FILE_SIZE
    ) -> Dict[str, Any]:
        """Create minimal context for a task to optimize quality.
        
        Args:
            task_description: Description of the current task
            relevant_files: List of files relevant to this task
            max_files: Maximum number of files to include
            max_file_size: Maximum characters per file before truncation
            
        Returns:
            Minimal context dictionary
        """
        context = {
            "task": task_description,
            "state": {
                "position": self.project_state.current_position,
                "decisions": self.project_state.accumulated_decisions[-5:],  # Last 5 decisions
            },
            "files": {},
        }
        
        # Load only relevant file contents
        if relevant_files:
            for file_path in relevant_files[:max_files]:
                full_path = self.project_root / file_path
                if full_path.exists() and full_path.is_file():
                    try:
                        content = full_path.read_text()
                        # Truncate large files
                        if len(content) > max_file_size:
                            content = content[:max_file_size] + "\n... [truncated]"
                        context["files"][file_path] = content
                    except Exception:
                        pass
        
        return context
    
    def update_context_usage(self, tokens_used: int) -> None:
        """Update context window usage tracking.
        
        Args:
            tokens_used: Number of tokens used
        """
        self.context_window.current_tokens = tokens_used
    
    def get_quality_status(self) -> Dict[str, Any]:
        """Get current quality status based on context usage.
        
        Returns:
            Quality status dictionary
        """
        return {
            "usage_percent": self.context_window.usage_percent,
            "quality_tier": self.context_window.quality_tier,
            "should_checkpoint": self.context_window.should_checkpoint,
            "tokens_used": self.context_window.current_tokens,
            "max_tokens": self.context_window.max_tokens,
        }
    
    def add_decision(self, decision: str) -> None:
        """Record an accumulated decision.
        
        Args:
            decision: Decision to record
        """
        self.project_state.accumulated_decisions.append(decision)
        self.save_project_state()
    
    def add_blocker(self, blocker: str) -> None:
        """Record a blocker.
        
        Args:
            blocker: Blocker description
        """
        self.project_state.blockers.append(blocker)
        self.save_project_state()
    
    def update_position(self, position: str) -> None:
        """Update current project position.
        
        Args:
            position: New position description
        """
        self.project_state.current_position = position
        self.save_project_state()


def get_context_manager(project_root: Optional[Path] = None) -> GSDContextManager:
    """Get a configured GSD context manager instance.
    
    Args:
        project_root: Root directory of the project
        
    Returns:
        Configured GSDContextManager
    """
    return GSDContextManager(project_root=project_root)

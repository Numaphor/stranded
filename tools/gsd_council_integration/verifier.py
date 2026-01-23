"""GSD Verifier for LLM Council.

This module provides verification capabilities for council outputs
using GSD's verification methodology. It checks that:

1. Plans are properly structured with atomic tasks
2. Execution results meet success criteria
3. Code changes are verified against requirements
"""

import os
import re
from pathlib import Path
from typing import Dict, List, Optional, Any, Tuple
from dataclasses import dataclass, field
from enum import Enum

from . import GSD_PATH
from .plan_adapter import GSDPlan, GSDTask


# Verification constants
MIN_RESPONSE_LENGTH = 50  # Minimum characters for a valid response
KEYWORD_MATCH_THRESHOLD = 0.3  # 30% of keywords must match
STRONG_CONSENSUS_THRESHOLD = 0.6  # 60%+ agreement for strong consensus
MODERATE_CONSENSUS_THRESHOLD = 0.4  # 40-60% agreement for moderate consensus


def _format_ratio(ratio: float) -> str:
    """Format a ratio as a percentage string."""
    return f"{ratio:.0%}"


class VerificationStatus(Enum):
    """Status of a verification check."""
    PASSED = "passed"
    FAILED = "failed"
    SKIPPED = "skipped"
    PARTIAL = "partial"


@dataclass
class VerificationResult:
    """Result of a single verification check."""
    
    name: str
    status: VerificationStatus
    message: str = ""
    details: Dict[str, Any] = field(default_factory=dict)
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            "name": self.name,
            "status": self.status.value,
            "message": self.message,
            "details": self.details,
        }


@dataclass
class VerificationReport:
    """Complete verification report for a plan or execution."""
    
    plan_name: str
    results: List[VerificationResult] = field(default_factory=list)
    overall_status: VerificationStatus = VerificationStatus.SKIPPED
    
    @property
    def passed_count(self) -> int:
        """Count of passed checks."""
        return sum(1 for r in self.results if r.status == VerificationStatus.PASSED)
    
    @property
    def failed_count(self) -> int:
        """Count of failed checks."""
        return sum(1 for r in self.results if r.status == VerificationStatus.FAILED)
    
    def calculate_overall_status(self) -> None:
        """Calculate overall status from individual results."""
        if not self.results:
            self.overall_status = VerificationStatus.SKIPPED
        elif all(r.status == VerificationStatus.PASSED for r in self.results):
            self.overall_status = VerificationStatus.PASSED
        elif all(r.status == VerificationStatus.FAILED for r in self.results):
            self.overall_status = VerificationStatus.FAILED
        else:
            self.overall_status = VerificationStatus.PARTIAL
    
    def to_markdown(self) -> str:
        """Convert report to markdown format."""
        lines = [f"# Verification Report: {self.plan_name}"]
        
        status_emoji = {
            VerificationStatus.PASSED: "✅",
            VerificationStatus.FAILED: "❌",
            VerificationStatus.SKIPPED: "⏭️",
            VerificationStatus.PARTIAL: "⚠️",
        }
        
        lines.append(f"\n**Overall Status:** {status_emoji[self.overall_status]} {self.overall_status.value.upper()}")
        lines.append(f"\n**Summary:** {self.passed_count} passed, {self.failed_count} failed")
        
        lines.append("\n## Results\n")
        for result in self.results:
            emoji = status_emoji[result.status]
            lines.append(f"### {emoji} {result.name}")
            if result.message:
                lines.append(f"\n{result.message}")
            if result.details:
                lines.append("\n**Details:**")
                for key, value in result.details.items():
                    lines.append(f"- {key}: {value}")
            lines.append("")
        
        return "\n".join(lines)


class GSDVerifier:
    """Verifies council outputs using GSD methodology.
    
    The verifier checks:
    1. Plan structure (atomic tasks, proper format)
    2. Execution completeness (all tasks done)
    3. Success criteria (measurable outcomes met)
    """
    
    def __init__(self, gsd_path: Optional[Path] = None):
        """Initialize the verifier.
        
        Args:
            gsd_path: Path to GSD submodule
        """
        self.gsd_path = gsd_path or GSD_PATH
    
    def verify_plan_structure(self, plan: GSDPlan) -> VerificationReport:
        """Verify a plan has proper GSD structure.
        
        Args:
            plan: The plan to verify
            
        Returns:
            Verification report
        """
        report = VerificationReport(plan_name=plan.name)
        
        # Check objective
        if plan.objective:
            report.results.append(VerificationResult(
                name="Has Objective",
                status=VerificationStatus.PASSED,
                message="Plan has a clear objective"
            ))
        else:
            report.results.append(VerificationResult(
                name="Has Objective",
                status=VerificationStatus.FAILED,
                message="Plan is missing an objective"
            ))
        
        # Check tasks exist
        if plan.tasks:
            report.results.append(VerificationResult(
                name="Has Tasks",
                status=VerificationStatus.PASSED,
                message=f"Plan has {len(plan.tasks)} tasks"
            ))
        else:
            report.results.append(VerificationResult(
                name="Has Tasks",
                status=VerificationStatus.FAILED,
                message="Plan has no tasks defined"
            ))
        
        # Check task count (GSD recommends 2-3 tasks per plan)
        if 1 <= len(plan.tasks) <= 4:
            report.results.append(VerificationResult(
                name="Task Count Optimal",
                status=VerificationStatus.PASSED,
                message=f"Plan has {len(plan.tasks)} tasks (optimal: 2-3)"
            ))
        else:
            report.results.append(VerificationResult(
                name="Task Count Optimal",
                status=VerificationStatus.PARTIAL,
                message=f"Plan has {len(plan.tasks)} tasks (GSD recommends 2-3)",
                details={"task_count": len(plan.tasks), "recommended": "2-3"}
            ))
        
        # Check each task structure
        for i, task in enumerate(plan.tasks):
            task_results = self._verify_task_structure(task, i + 1)
            report.results.extend(task_results)
        
        report.calculate_overall_status()
        return report
    
    def _verify_task_structure(
        self, 
        task: GSDTask, 
        task_number: int
    ) -> List[VerificationResult]:
        """Verify a single task has proper structure.
        
        Args:
            task: The task to verify
            task_number: Task sequence number
            
        Returns:
            List of verification results
        """
        results = []
        
        # Check task has name
        if task.name:
            results.append(VerificationResult(
                name=f"Task {task_number}: Has Name",
                status=VerificationStatus.PASSED
            ))
        else:
            results.append(VerificationResult(
                name=f"Task {task_number}: Has Name",
                status=VerificationStatus.FAILED,
                message="Task is missing a name"
            ))
        
        # Check task has action
        if task.action:
            results.append(VerificationResult(
                name=f"Task {task_number}: Has Action",
                status=VerificationStatus.PASSED
            ))
        else:
            results.append(VerificationResult(
                name=f"Task {task_number}: Has Action",
                status=VerificationStatus.FAILED,
                message="Task is missing an action"
            ))
        
        # Check verification criteria (recommended but not required)
        if task.verify:
            results.append(VerificationResult(
                name=f"Task {task_number}: Has Verification",
                status=VerificationStatus.PASSED
            ))
        else:
            results.append(VerificationResult(
                name=f"Task {task_number}: Has Verification",
                status=VerificationStatus.PARTIAL,
                message="Task is missing verification criteria (recommended)"
            ))
        
        return results
    
    def verify_execution_response(
        self, 
        task: GSDTask, 
        response: str
    ) -> VerificationResult:
        """Verify a council member's execution response meets task criteria.
        
        Args:
            task: The task that was executed
            response: The council member's response
            
        Returns:
            Verification result
        """
        # Check if response addresses the task
        if not response or len(response.strip()) < MIN_RESPONSE_LENGTH:
            return VerificationResult(
                name=f"Task Execution: {task.name}",
                status=VerificationStatus.FAILED,
                message="Response is too short or empty"
            )
        
        # Check if response mentions success criteria keywords
        if task.done:
            # Simple keyword matching (could be more sophisticated)
            done_keywords = task.done.lower().split()
            response_lower = response.lower()
            matches = sum(1 for kw in done_keywords if kw in response_lower)
            match_ratio = matches / len(done_keywords) if done_keywords else 0
            
            if match_ratio >= KEYWORD_MATCH_THRESHOLD:
                return VerificationResult(
                    name=f"Task Execution: {task.name}",
                    status=VerificationStatus.PASSED,
                    message="Response addresses success criteria",
                    details={"keyword_match_ratio": _format_ratio(match_ratio)}
                )
            else:
                return VerificationResult(
                    name=f"Task Execution: {task.name}",
                    status=VerificationStatus.PARTIAL,
                    message="Response may not fully address success criteria",
                    details={"keyword_match_ratio": _format_ratio(match_ratio)}
                )
        
        return VerificationResult(
            name=f"Task Execution: {task.name}",
            status=VerificationStatus.PASSED,
            message="Task executed (no specific success criteria to check)"
        )
    
    def verify_council_consensus(
        self,
        responses: List[Dict[str, Any]],
        rankings: List[Dict[str, Any]]
    ) -> VerificationResult:
        """Verify council reached meaningful consensus.
        
        Args:
            responses: Stage 1 responses from council members
            rankings: Stage 2 rankings from peer review
            
        Returns:
            Verification result
        """
        if not responses:
            return VerificationResult(
                name="Council Consensus",
                status=VerificationStatus.FAILED,
                message="No responses to evaluate"
            )
        
        if not rankings:
            return VerificationResult(
                name="Council Consensus",
                status=VerificationStatus.FAILED,
                message="No rankings available"
            )
        
        # Check if rankings have parsed results
        valid_rankings = [r for r in rankings if r.get("parsed_ranking")]
        if not valid_rankings:
            return VerificationResult(
                name="Council Consensus",
                status=VerificationStatus.PARTIAL,
                message="Rankings could not be parsed - consensus unclear"
            )
        
        # Check ranking agreement
        first_choices = []
        for ranking in valid_rankings:
            parsed = ranking.get("parsed_ranking", [])
            if parsed:
                first_choices.append(parsed[0])
        
        if not first_choices:
            return VerificationResult(
                name="Council Consensus",
                status=VerificationStatus.PARTIAL,
                message="Could not determine first choices"
            )
        
        # Calculate agreement level
        most_common = max(set(first_choices), key=first_choices.count)
        agreement_count = first_choices.count(most_common)
        agreement_ratio = agreement_count / len(first_choices)
        
        if agreement_ratio >= STRONG_CONSENSUS_THRESHOLD:
            return VerificationResult(
                name="Council Consensus",
                status=VerificationStatus.PASSED,
                message=f"Strong consensus: {_format_ratio(agreement_ratio)} agreement on {most_common}",
                details={
                    "top_choice": most_common,
                    "agreement_ratio": _format_ratio(agreement_ratio),
                    "total_votes": len(first_choices)
                }
            )
        elif agreement_ratio >= MODERATE_CONSENSUS_THRESHOLD:
            return VerificationResult(
                name="Council Consensus",
                status=VerificationStatus.PARTIAL,
                message=f"Moderate consensus: {_format_ratio(agreement_ratio)} agreement",
                details={
                    "top_choice": most_common,
                    "agreement_ratio": _format_ratio(agreement_ratio)
                }
            )
        else:
            return VerificationResult(
                name="Council Consensus",
                status=VerificationStatus.FAILED,
                message=f"Weak consensus: {_format_ratio(agreement_ratio)} agreement",
                details={
                    "top_choice": most_common,
                    "agreement_ratio": _format_ratio(agreement_ratio)
                }
            )
    
    def create_verification_prompt(self, task: GSDTask) -> str:
        """Create a verification prompt for council members.
        
        Args:
            task: The task to verify
            
        Returns:
            Verification prompt
        """
        prompt = f"""## Verification Task

Please verify the following work was completed correctly:

### Task: {task.name}

**Expected Action:**
{task.action}

**Verification Criteria:**
{task.verify or "No specific criteria provided"}

**Success Criteria:**
{task.done or "No specific criteria provided"}

### Your Verification Task

1. Check if the work was completed as specified
2. Verify any code changes are correct and follow best practices
3. Confirm success criteria are met
4. Report any issues or concerns

Provide your verification in this format:
- **Status**: PASSED | FAILED | PARTIAL
- **Summary**: Brief summary of findings
- **Issues**: List any issues found (if applicable)
- **Suggestions**: Any improvement suggestions
"""
        return prompt


def get_verifier() -> GSDVerifier:
    """Get a configured GSD verifier instance.
    
    Returns:
        Configured GSDVerifier
    """
    return GSDVerifier()

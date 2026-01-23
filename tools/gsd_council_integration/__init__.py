"""GSD (Get Shit Done) Integration for LLM Council.

This module integrates the Get Shit Done (GSD) meta-prompting and context engineering
system with LLM Council's multi-agent orchestration. The integration allows:

1. Using GSD's planning methodology with council's peer review process
2. Applying GSD's execution patterns to council code generation
3. Leveraging GSD's verification system for council outputs

The GSD submodule is located at: ../get-shit-done
The LLM Council submodule is at: ../llm_council_skill
See: https://github.com/glittercowboy/get-shit-done
"""

from pathlib import Path

# Paths to the submodules
# tools/gsd_council_integration/__init__.py -> tools -> repo_root
INTEGRATION_DIR = Path(__file__).parent
TOOLS_DIR = INTEGRATION_DIR.parent
REPO_ROOT = TOOLS_DIR.parent
GSD_PATH = REPO_ROOT / "get-shit-done"
LLM_COUNCIL_PATH = REPO_ROOT / "llm_council_skill"

# Import classes for convenience
from .plan_adapter import GSDPlanAdapter, GSDPlan, GSDTask, get_gsd_adapter
from .context_manager import GSDContextManager, ProjectState, ContextWindow, get_context_manager
from .verifier import GSDVerifier, VerificationResult, VerificationReport, VerificationStatus, get_verifier

__all__ = [
    # Paths
    "GSD_PATH",
    "LLM_COUNCIL_PATH",
    "REPO_ROOT",
    # Plan Adapter
    "GSDPlanAdapter",
    "GSDPlan", 
    "GSDTask",
    "get_gsd_adapter",
    # Context Manager
    "GSDContextManager",
    "ProjectState",
    "ContextWindow",
    "get_context_manager",
    # Verifier
    "GSDVerifier",
    "VerificationResult",
    "VerificationReport",
    "VerificationStatus",
    "get_verifier",
]

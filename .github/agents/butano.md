# Butano Agent Configuration
# This file is meant for repositories using Butano as a dependency.
# Butano is a modern C++ high-level Game Boy Advance (GBA) engine.
# If you're using Butano in your project, consider the recommendations
# and checks below to ensure compatibility and adherence to best practices.

name: "Butano Integration Agent"

description: |
  This agent is designed to ensure that repositories integrating the Butano engine 
  for Game Boy Advance (GBA) development follow best practices in C++ development,
  utilize Butano effectively, and maintain a strong development workflow.

goals:
  - Validate integration of Butano within the repository.
  - Encourage proper use of Butano's C++ APIs.
  - Monitor adherence to C++ standards appropriate for GBA development.
  - Ensure compatibility with Python and Makefile components of Butano.

checks:
  - name: "Check Butano Initialization"
    description: |
      Ensures that the Butano engine is correctly initialized within the project 
      and that the code leverages its modular components.

  - name: "C++ Version Compatibility"
    description: |
      Verify that the project uses C++ standard version compatible with Butano 
      (e.g., modern C++ standards such as C++17 or higher).
      
  - name: "Python Integration Scripts"
    description: |
      Verify that Python scripts, if any, related to the project (e.g., 
      asset conversion tools) are configured and functioning as expected.

  - name: "Proper Makefile Usage"
    description: |
      Check the project's Makefile for correctness and ensure it respects Butano 
      build system conventions.

actions:
  - name: "Upgrade Butano Dependencies"
    description: |
      Assist in upgrading the Butano engine or related dependencies to 
      the latest stable version.

  - name: "Generate Asset Conversion Logs"
    description: |
      Offer utilities to generate detailed logs for asset conversion tools 
      (e.g., sprites, audio) that interact with Butano.

support:
  - description: |
      For integration issues or general support, contact the Butano GitHub 
      repository or refer to its official documentation:
      https://github.com/GValiente/butano

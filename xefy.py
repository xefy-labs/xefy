#!/usr/bin/env python3
"""Xefy developer tooling entry point.

Usage: python3 xefy.py <doctor|setup|provenance|clean> [options]
"""
import os
import sys

ROOT = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ROOT, "tools"))

from xefy_tools.cli import main  # noqa: E402

if __name__ == "__main__":
    sys.exit(main())

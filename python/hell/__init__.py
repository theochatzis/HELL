"""HELL - High Energy Likelihood Laboratory.

Python layer of the framework: configuration of C++ module sequences,
sample registry, batch submission, ML tools, fit wrappers and navigation.
The physics runs in C++ (hell-run, hell-hist, hell-datacard); python only
prepares JSON configurations and orchestrates jobs.
"""
import os

__version__ = "0.1.0"

HELL_BASE = os.path.realpath(os.environ.get("HELL_BASE") or os.path.join(os.path.dirname(__file__), "..", ".."))
HELL_BUILD = os.environ.get("HELL_BUILD", os.path.join(HELL_BASE, "build"))


def banner():
    return "[HELL] Welcome to HELL."


def log(msg, level="INFO"):
    prefix = "[HELL]"
    if level == "WARNING":
        prefix += " WARNING:"
    elif level == "ERROR":
        prefix += " ERROR:"
    print(f"{prefix} {msg}", flush=True)


def warn(msg):
    log(msg, "WARNING")


def error(msg):
    log(msg, "ERROR")


class HellError(Exception):
    """Raised for user-facing errors; the CLI prints them without a traceback."""

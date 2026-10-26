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


def banner(color=None):
    """The HELL banner (env/banner.txt) with the flame gradient when printing to a colour terminal."""
    import sys
    utf8 = (sys.stdout.encoding or "").lower().replace("-", "") == "utf8"
    path = os.path.join(HELL_BASE, "env", "banner.txt" if utf8 else "banner_ascii.txt")
    try:
        lines = open(path, encoding="utf-8").read().splitlines()
    except OSError:
        return "[HELL] Welcome to HELL."
    if color is None:
        color = sys.stdout.isatty() and not os.environ.get("NO_COLOR") and os.environ.get("TERM", "dumb") != "dumb"
    if not color:
        return "\n".join(lines)
    import re
    flame = [230, 229, 228, 227, 226, 226, 220, 220, 214, 214, 208, 208] + [202] * 13 + [196] * 3
    red = [196] * 6 + [160] * 4 + [124] * 2 + [88]
    out = []
    for i, l in enumerate(lines):
        fc = flame[min(i, len(flame) - 1)]
        lc = red[min(max(i - 12, 0), len(red) - 1)]
        body = re.sub(r"([█▓#=]+)", lambda m: f"\033[1;38;5;{lc}m{m.group(1)}\033[1;38;5;{fc}m", l)
        out.append(f"\033[1;38;5;{fc}m{body}\033[0m")
    return "\n".join(out)


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

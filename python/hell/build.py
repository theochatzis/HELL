"""`hell build` and `hell env`: configure and compile the C++ part, report the environment.

Configuring (cmake) happens automatically when the build directory is new, when --reconfigure is given,
or when the git HEAD changed since the last configure, so that the version hash written into the
provenance of every output file stays current after a commit.
"""
import os
import re
import shutil
import subprocess
import sys

from hell import HELL_BASE, HELL_BUILD, HellError, log, warn


def git_head():
    try:
        out = subprocess.run(["git", "rev-parse", "--short=12", "HEAD"], cwd=HELL_BASE, capture_output=True, text=True, check=True).stdout.strip()
        dirty = subprocess.run(["git", "status", "--porcelain", "--untracked-files=no"], cwd=HELL_BASE, capture_output=True, text=True).stdout.strip()
        return out + ("-dirty" if dirty else "")
    except (subprocess.CalledProcessError, FileNotFoundError):
        return "unknown"


def built_version():
    """Hash baked into the configured build (generated Version.h), or None."""
    path = os.path.join(HELL_BUILD, "generated", "Core", "interface", "Version.h")
    if not os.path.exists(path):
        return None
    m = re.search(r'#define HELL_GIT_HASH "([^"]*)"', open(path).read())
    return m.group(1) if m else None


def env_ok():
    return bool(os.environ.get("HELL_BASE")) and shutil.which("cmake") and shutil.which("root-config")


def build(jobs=None, clean=False, reconfigure=False, build_type=None, target=None, verbose=False):
    if not env_ok():
        raise HellError(f"environment not loaded: run `source {HELL_BASE}/env/setup.sh` first")
    if clean and os.path.isdir(HELL_BUILD):
        log(f"removing {HELL_BUILD}")
        shutil.rmtree(HELL_BUILD)
    os.makedirs(HELL_BUILD, exist_ok=True)
    cache = os.path.join(HELL_BUILD, "CMakeCache.txt")
    head = git_head()
    need_configure = reconfigure or not os.path.exists(cache) or build_type is not None or built_version() != head
    if need_configure:
        cmd = ["cmake", HELL_BASE]
        if build_type:
            cmd.append(f"-DCMAKE_BUILD_TYPE={build_type}")
        log("configuring: " + " ".join(cmd) + (f"  (git HEAD {head})" if built_version() != head else ""))
        rc = subprocess.call(cmd, cwd=HELL_BUILD, stdout=None if verbose else subprocess.DEVNULL)
        if rc != 0:
            raise HellError("cmake failed (run with --verbose to see why)")
    jobs = jobs or min(8, os.cpu_count() or 4)
    cmd = ["make", f"-j{jobs}"] + ([target] if target else [])
    log("compiling: " + " ".join(cmd))
    proc = subprocess.run(cmd, cwd=HELL_BUILD, capture_output=not verbose, text=True)
    if not verbose:
        # show warnings and errors only
        for line in (proc.stdout + proc.stderr).splitlines():
            if re.search(r"error|warning|\*\*\*", line, re.IGNORECASE) and "Werror" not in line:
                print(line)
    if proc.returncode != 0:
        raise HellError("build failed. The forge went cold.")
    exe = os.path.join(HELL_BUILD, "bin", "hell-run")
    plugins = sorted(f for f in os.listdir(os.path.join(HELL_BUILD, "lib", "plugins")) if f.endswith(".so")) if os.path.isdir(os.path.join(HELL_BUILD, "lib", "plugins")) else []
    log(f"build ready: {exe} and {len(plugins)} plugin libraries (version {built_version()}). All forged.")
    return 0


def env_report():
    lines = []
    base = os.environ.get("HELL_BASE")
    if not base:
        lines.append(f"[HELL] environment NOT loaded. Run:\n    source {HELL_BASE}/env/setup.sh")
        return "\n".join(lines)
    root = subprocess.run(["root-config", "--version"], capture_output=True, text=True).stdout.strip() if shutil.which("root-config") else "not found"
    lines.append(f"[HELL] environment loaded ({os.environ.get('HELL_ENV', '?')})")
    lines.append(f"  base      {base}")
    lines.append(f"  build     {HELL_BUILD} ({'configured' if os.path.exists(os.path.join(HELL_BUILD, 'CMakeCache.txt')) else 'not configured: run hell build'})")
    lines.append(f"  ROOT      {root}")
    lines.append(f"  python    {sys.version.split()[0]} ({sys.executable})")
    try:
        import torch  # noqa
        lines.append(f"  torch     {torch.__version__}")
    except ImportError:
        lines.append("  torch     not available")
    head, built = git_head(), built_version()
    lines.append(f"  git HEAD  {head}" + ("" if built == head else f"   (build has {built}: run hell build)"))
    lines.append(f"  combine   {os.environ.get('HELL_COMBINE_SETUP') or 'HELL_COMBINE_SETUP not set (needed by hell fit)'}")
    lines.append(f"  plugins   {os.environ.get('HELL_PLUGIN_PATH', '?')}")
    return "\n".join(lines)

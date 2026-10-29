#!/bin/bash
# Automatically generated using: hell submit ... (reference copy made by python/hell/batch/condor.py)
# HELL condor job wrapper. The framework tarball, the job list and the certification JSON arrive through
# condor file transfer (transfer_input_files), so the worker needs nothing but cvmfs and xrootd.
set -e
JOBID="$1"
echo "[HELL] job $JOBID starting on $(hostname) at $(date) in $PWD"
tar xzf hell.tar.gz
export HELL_ENV=lcg
export HELL_QUIET=1
source HELL/env/setup.sh
# export X509_USER_PROXY=<taskdir>/x509_proxy
python3 - "$JOBID" <<'PYEOF'
import json, os, subprocess, sys
jobid = sys.argv[1]
jobs = json.load(open("jobs.json"))
job = jobs[jobid]
cmd = job["command"]
print("[HELL] running:", cmd, flush=True)
rc = subprocess.call(cmd, shell=True)
if rc != 0:
    print("[HELL] ERROR: command failed with code", rc, flush=True)
    sys.exit(rc)
for local, dest in job.get("outputs", {}).items():
    if not os.path.exists(local):
        print("[HELL] ERROR: expected output", local, "missing", flush=True); sys.exit(2)
    if dest.startswith("root://"):
        subprocess.check_call(["xrdcp", "-f", local, dest])
    else:
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        subprocess.check_call(["cp", "-f", local, dest])
    print("[HELL] copied", local, "->", dest, flush=True)
print("[HELL] job done.", flush=True)
PYEOF

#!/bin/bash
# HELL environment. Source it from anywhere:  source /path/to/HELL/env/setup.sh
# HELL_ENV=lcg (default) uses an LCG view from cvmfs (ROOT, PyTorch, uproot, correctionlib).
# HELL_ENV=native uses whatever ROOT/python is on the PATH.
_hell_src="${BASH_SOURCE[0]:-$0}"
export HELL_BASE="$(cd "$(dirname "$_hell_src")/.." && pwd)"
export HELL_ENV="${HELL_ENV:-lcg}"
export HELL_LCG_VIEW="${HELL_LCG_VIEW:-/cvmfs/sft.cern.ch/lcg/views/LCG_108/x86_64-el9-gcc13-opt}"
if [ "$HELL_ENV" = "lcg" ]; then
  if [ -f "$HELL_LCG_VIEW/setup.sh" ]; then
    source "$HELL_LCG_VIEW/setup.sh"
  else
    echo "[HELL] WARNING: LCG view $HELL_LCG_VIEW not found, falling back to the native environment"
    export HELL_ENV=native
  fi
fi
export HELL_BUILD="${HELL_BUILD:-$HELL_BASE/build}"
export HELL_PLUGIN_PATH="$HELL_BUILD/lib/plugins"
export PATH="$HELL_BUILD/bin:$HELL_BASE/bin:$PATH"
export LD_LIBRARY_PATH="$HELL_BUILD/lib:${LD_LIBRARY_PATH}"
export PYTHONPATH="$HELL_BASE/python:${PYTHONPATH}"
export PYTHONDONTWRITEBYTECODE=1
# Standalone Higgs Combine: point HELL_COMBINE_SETUP to a script that puts `combine` on the PATH
# (e.g. a CMSSW area with HiggsAnalysis/CombinedLimit, or a conda/pip install). `hell fit` sources it.
export HELL_COMBINE_SETUP="${HELL_COMBINE_SETUP:-}"
echo "[HELL] Welcome to HELL. base=$HELL_BASE env=$HELL_ENV root=$(root-config --version 2>/dev/null || echo none)"

# crab/
`hell submit run <skim_cfg.py> --backend crab ...` writes `crab/work/<tag>/` with a CRAB config, a `scriptExe`
wrapper and a dummy PSet per sample (`python/hell/batch/crab.py`). Needs a CMSSW area with CRAB sourced and a
VOMS proxy. See `docs/tools/batch.md`. `work/` is not in git.

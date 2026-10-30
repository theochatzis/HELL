# Batch processing

Two backends. CRAB is the recommended one for skims of the central datasets (grid-wide, dataset-aware, lumi
mask, retries, processed-lumi report); HTCondor is for skims of private or local files and for every step
after the skim (analysis trees, histograms, training), where it needs no CMSSW and has direct EOS access.
The outputs are the same HELL files either way.

## HTCondor (`python/hell/batch/condor.py`)
Any HELL command can be a condor task. A task is a directory `~/hell_condor/<tag>/` (or `$HELL_CONDOR_WORK`;
never on EOS, the schedd rejects it) with `jobs.json` (id -> command and outputs), `jobs.txt` (queue), `job.sh`,
`job.sub`, `hell.tar.gz` (build, python, configs, samples, env: workers need only cvmfs) and `meta.json`.
```
hell submit run   cfg.py   --sample A B --era 2022 --files-per-job 10 --output-dir /eos/... --tag t1 [--variation jesUp] [--proxy]
hell submit hist  hist.yaml --output-dir /eos/... --tag t2            # one job per process
hell submit train ml.yaml   --output-dir /eos/... --tag t3 [--gpu]
hell submit cmd   jobs.txt  --output-dir /eos/... --tag t4            # "id | command | output" per line
hell unleash-systematics analysis_cfg.py --sample A --era 2022 --output-dir /eos/... --tag t5 --variations jesUp jesDown
hell status --tag t1 ; hell check --tag t1 ; hell resubmit --tag t1 ; hell merge --tag t1
```
Options: `--flavour`, `--cpus`, `--memory`, `--retries`, `--gpu`, `--dry-run`. The tarball, the job list and,
for data samples, the Golden JSON of the era are shipped to the worker with condor file transfer
(`transfer_input_files`), so the worker needs only cvmfs and xrootd; the skim applies the Golden JSON inside
the job (`goldenJSON=<file>` option of the config). Outputs are copied with `xrdcp` to EOS
(`root://eosuser.cern.ch/`). `hell check` opens every expected ROOT output and requires `hell/counts`; failed
ids go to `resubmit.txt`. The task directory must be on AFS or local disk (`$HELL_CONDOR_WORK`; the default
is `~/hell_condor`, or `/afs/cern.ch/work/<u>/<user>/hell_condor` when the home is on EOS).

## CRAB (`python/hell/batch/crab.py`)
For skims of DAS datasets (recommended for the central samples). Only dry runs so far: submit one small real task before relying on it. Needs a CMSSW area (`cmsenv`, `source /cvmfs/cms.cern.ch/crab3/crab.sh`, proxy).
`hell submit run skim_cfg.py --backend crab --sample X --era 2022 --tag t --output-dir /store/user/<you>/HELL/skims`
writes `crab/work/<tag>/crab_<sample>.py`, a `scriptExe` that unpacks HELL and runs `hell run` on the files
CRAB assigns (from `PSet.py`), and a dummy PSet. Data samples get `lumiMask` from the era's Golden JSON, and the
in-job filter is disabled (`goldenJSON=none`) since CRAB has already selected the certified lumi sections.

"""CRAB submission of HELL skims over DAS datasets.

CRAB needs a CMSSW area (cmsenv) and can run arbitrary scripts with
`scriptExe`. The job unpacks HELL (shipped as hell.tar.gz), runs hell-run
on the input files CRAB assigns (read from the PSet's fileNames through
the standard FrameworkJobReport trick: the script reads them from
PSet.py), and leaves <output>.root for CRAB to transfer. The lumi mask
(Golden JSON) is applied by CRAB itself for data.

    hell submit run configs/SingleTopMass/skim_cfg.py --backend crab --sample X --era 2022 --tag skim_v1 \
        --output-dir /store/user/<user>/HELL/skims/2022
writes crab/work/<tag>/crab_<sample>.py and (unless --dry-run) submits each.
"""
import os
import subprocess

from hell import HELL_BASE, HellError, generated_by, log, warn
from hell import samples as S
from hell.batch.condor import pack

SCRIPT_EXE = r"""#!/bin/bash
# {generated}
# HELL CRAB job script
set -e
echo "[HELL] CRAB job $1 on $(hostname)"
tar xzf hell.tar.gz
export HELL_ENV={hell_env}
source HELL/env/setup.sh
# Input files assigned by CRAB are in PSet.py (process.source.fileNames)
python3 - <<'PYEOF'
import re, subprocess, sys
txt = open("PSet.py").read()
m = re.search(r"fileNames\s*=\s*cms\.untracked\.vstring\((.*?)\)", txt, re.S)
files = re.findall(r"'([^']+)'", m.group(1)) if m else []
files = [f if f.startswith("root://") else "root://cms-xrd-global.cern.ch/" + f for f in files]
# goldenJSON=none: CRAB already applied the lumi mask (config.Data.lumiMask), and EOS is not reachable on grid sites
cmd = ["hell", "run", "HELL/{cfg}", "sample={sample}", "era={era}", "inputFiles=" + ",".join(files), "outputFile={output}", "variation={variation}", "goldenJSON=none"] + {options}
print("[HELL] running:", " ".join(cmd), flush=True)
sys.exit(subprocess.call(cmd))
PYEOF
# CRAB expects a FrameworkJobReport.xml; produce a minimal one
cat > FrameworkJobReport.xml <<'XEOF'
<FrameworkJobReport>
<ReadBranches>
</ReadBranches>
<PerformanceReport>
  <PerformanceSummary Metric="StorageStatistics">
    <Metric Name="Parameter-untracked-bool-enabled" Value="true"/>
  </PerformanceSummary>
</PerformanceReport>
<GeneratorInfo>
</GeneratorInfo>
</FrameworkJobReport>
XEOF
"""

PSET = """# {generated}
# Dummy PSet for CRAB (the real work is done by hell-run through scriptExe)
import FWCore.ParameterSet.Config as cms
process = cms.Process("HELL")
process.source = cms.Source("PoolSource", fileNames=cms.untracked.vstring())
process.maxEvents = cms.untracked.PSet(input=cms.untracked.int32(-1))
process.output = cms.OutputModule("PoolOutputModule", fileName=cms.untracked.string("{output}"))
process.out = cms.EndPath(process.output)
"""

CRAB_CFG = """# {generated}
# CRAB configuration for the HELL skim of {sample}
from CRABClient.UserUtilities import config
config = config()
config.General.requestName = "{request}"
config.General.workArea = "{workarea}"
config.General.transferOutputs = True
config.General.transferLogs = True
config.JobType.pluginName = "Analysis"
config.JobType.psetName = "{pset}"
config.JobType.scriptExe = "{script}"
config.JobType.inputFiles = ["{tarball}"]
config.JobType.outputFiles = ["{output}"]
config.JobType.disableAutomaticOutputCollection = True
config.JobType.maxMemoryMB = {memory}
config.Data.inputDataset = "{dataset}"
config.Data.inputDBS = "{dbs}"
config.Data.splitting = "{splitting}"
config.Data.unitsPerJob = {units}
config.Data.outLFNDirBase = "{outlfn}"
config.Data.publication = False
config.Data.outputDatasetTag = "{tag}"
{lumimask}
config.Site.storageSite = "{site}"
"""


def prepare(cfg, samples, era, tag, output_lfn, files_per_job=5, site="T3_CH_CERNBOX", memory=2500, options=(), variation="nominal", hell_env=None):
    workarea = os.path.join(HELL_BASE, "crab", "work", tag)
    os.makedirs(workarea, exist_ok=True)
    tarball = pack(workarea)
    cfg_rel = os.path.relpath(os.path.realpath(cfg), HELL_BASE)
    written = []
    for sname in samples:
        sample = S.find_sample(sname, era)
        if "das" not in sample:
            raise HellError(f"sample {sname} has no DAS dataset: CRAB needs one (use condor for local files)")
        output = f"{sname}.root"
        script = os.path.join(workarea, f"hell_crab_{sname}.sh")
        with open(script, "w") as f:
            f.write(SCRIPT_EXE.format(cfg=cfg_rel, sample=sname, era=era, output=output, variation=variation,
                                      options=repr(list(options)), hell_env=hell_env or os.environ.get("HELL_ENV", "lcg"), generated=generated_by()))
        os.chmod(script, 0o755)
        pset = os.path.join(workarea, f"PSet_{sname}.py")
        with open(pset, "w") as f:
            f.write(PSET.format(output=output, generated=generated_by()))
        lumimask = ""
        if sample.get("isData") and sample.get("goldenJSON"):
            lumimask = f'config.Data.lumiMask = "{sample["goldenJSON"]}"'
        crab_cfg = os.path.join(workarea, f"crab_{sname}.py")
        with open(crab_cfg, "w") as f:
            f.write(CRAB_CFG.format(generated=generated_by(), sample=sname, request=f"{tag}_{sname}"[:100], workarea=workarea, pset=pset, script=script,
                                    tarball=tarball, output=output, memory=memory, dataset=sample["das"],
                                    dbs=sample.get("instance", "global").replace("prod/", ""), splitting="FileBased",
                                    units=files_per_job, outlfn=output_lfn, tag=tag, lumimask=lumimask, site=site))
        written.append(crab_cfg)
        log(f"CRAB config for {sname}: {crab_cfg}")
    return written


def submit(crab_cfgs, dry_run=False):
    rc = 0
    for c in crab_cfgs:
        if dry_run:
            log(f"dry run: crab submit {c}")
            continue
        if not os.environ.get("CMSSW_BASE"):
            raise HellError("CRAB needs a CMSSW environment: cmsenv and `source /cvmfs/cms.cern.ch/crab3/crab.sh` first")
        rc |= subprocess.call(["crab", "submit", "-c", c])
    return rc

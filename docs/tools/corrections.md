# Central corrections (correctionlib)

All corrections come from the central POG correctionlib JSONs on cvmfs
(`/cvmfs/cms.cern.ch/rsync/cms-nanoAOD/jsonpog-integration/POG`, documented at
https://cms-nanoaod-integration.web.cern.ch/commonJSONSFs/). Nothing correction-related is typed in a config:
`samples/corrections.yaml` lists per era the directory and the names of the corrections, the python layer
resolves it into the job (`process.job["corrections"]`), and the modules evaluate through the C++
`CorrectionService` (`Corrections/`), which opens every file once per job.

```
hell corrections --era 2022                      # resolved files and names, existence check
hell corrections --era 2022 --list jerc --grep Regrouped   # content of a JSON
```

| what | module | correction (2022) | output |
|---|---|---|---|
| b-tag working points | `BTagger` (no thresholds given) | `particleNet_wp_values` L/M/T | `Jet_btagWP` |
| jet id | `JetSelector(jetIdSource="correctionlib")` (needs NanoAODv13+ multiplicities; default stays the manual recipe) | `AK4PUPPI_TightLeptonVeto` | selection |
| JER smearing, JES/JER variations, JEC re-application | `JetEnergyCorrector` | `Summer22_22Sep2023_JRV1_MC_*`, `Summer22_22Sep2023_V2_MC_<source>_AK4PFPuppi`, compound `L1L2L3Res` | varied jets, `Jet_scale`, MET |
| jet veto map | `JetVetoMapFilter` | `Summer22_23Sep2023_RunCD_V1` | filter |
| pileup | `PileupWeightProducer` | `Collisions2022_355100_357900_eraBCD_GoldenJson` | `Weight_pu[Up/Down]` |
| muon id, iso, trigger | `MuonScaleFactorProducer` | `NUM_TightID_DEN_TrackerMuons`, `NUM_TightPFIso_DEN_TightID`, `NUM_IsoMu24_DEN_CutBasedIdTight_and_PFIsoTight` | `Muon_scaleFactor`, `Weight_muId/muIso/muTrig` |
| electron id, reco, trigger | `ElectronScaleFactorProducer` | `Electron-ID-SF` (year `2022Re-recoBCD`, `Tight`, `Reco*`), `Electron-HLT-SF` (`HLT_SF_Ele30_TightID`) | `Electron_scaleFactor`, `Weight_elId/elReco/elTrig` |
| b-tag scale factors | `BTagScaleFactorProducer` + `BTagEfficiencyAnalyzer` | `particleNet_comb` (b, c), `particleNet_light` | `Weight_btagHeavy/btagLight` |

In `configs/SingleTopMass/objects_cff.py`, `jets(process, era)` books the jet chain (selection, `JetEnergyCorrector`,
pt cut on the corrected jets, `BTagger`, veto map) and `weights(process, ...)` the weight producers; the
analysis config uses both. Data jobs get weight 1 everywhere and no smearing.

## Variations
Object variations are jobs: `variation=jerUp`, `variation=jesRegrouped_TotalUp`, or any source of the JME
file as `jes<Source>Up/Down` (`hell corrections --era 2022 --list jerc --grep Regrouped` lists the reduced
set). `hell unleash-systematics` defaults to the total JES and JER. Weight variations are columns of the
Weight table and are listed in `hist.yaml` as `type: weight` systematics.

## b-tag efficiencies
The fixed working point method needs the MC tagging efficiency per flavour in (pt, |eta|) for the analysis
selection: run the analysis config with `btagEfficiencyMaps=True` on the MC samples (histograms under
`btagEff/`), `hell merge` the outputs (one file per era, or per process if the efficiencies are to be
process-dependent), and give the file to the analysis with `btagEfficiencies=<file>`. Without it the b-tag
weight is 1 and the producer warns once.

## Adding an era or a new POG version
Add the era (or change the names) in `samples/corrections.yaml`; `hell corrections --era <era>` checks that
the files exist and `--list` shows the names available. The data JEC tags per run era (`jecData`) are only
needed with `reapplyJEC=True` and `dataRunEra=<letter>`.

#!/bin/bash
# HELL quick end-to-end test ("hell pray"): skim -> analysis trees (nominal + JES) -> histograms -> plots
# -> datacards (template + parametric) -> ML training -> ML application, on a few NanoAOD files from /eos/cms.
# Everything goes to test/out/. Takes about 2-3 minutes on lxplus.
# Steps can be selected:  run_quick_test.sh [skim trees bookkeep hist validate fit ml navigate]
# (default: all, from a clean test/out). Later steps use the outputs of the earlier ones in test/out.
set -e
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$HERE/.."
[ -n "$HELL_BASE" ] || source env/setup.sh
OUT=test/out
ALL="skim trees bookkeep hist validate fit ml navigate"
STEPS="${*:-$ALL}"
want() { case " $STEPS " in *" $1 "*) return 0;; *) return 1;; esac; }
if [ "$STEPS" = "$ALL" ]; then rm -rf "$OUT"; fi
mkdir -p "$OUT/skims" "$OUT/trees"
D=/eos/cms/store/mc/Run3Summer22NanoAODv12
pick() { find "$D/$1" -name "*.root" 2>/dev/null | head -"$2" | paste -sd, -; }
declare -A FILES
FILES[TbarBQtoLNu_tch]=$(pick TbarBQtoLNu-t-channel-4FS_TuneCP5_13p6TeV_powheg-madspin-pythia8 2)
FILES[TBbarQtoLNu_tch_MT173p5]=$(pick TBbarQtoLNu-t-channel-4FS-MT-173p5_TuneCP5_13p6TeV_powheg-madspin-pythia8 1)
FILES[TTtoLNu2Q]=$(pick TTtoLNu2Q_TuneCP5_13p6TeV_powheg-pythia8 1)
FILES[WtoLNu_2Jets]=$(pick WtoLNu-2Jets_TuneCP5_13p6TeV_amcatnloFXFX-pythia8 1)
FILES[Muon_2022C]=$(find /eos/cms/store/data/Run2022C/Muon/NANOAOD/22Sep2023-v1/ -name "*.root" | head -1)
for s in "${!FILES[@]}"; do
  [ -n "${FILES[$s]}" ] || { echo "[HELL] ERROR: no input files for $s (is /eos/cms mounted?)"; exit 1; }
done

step() { echo; echo "[HELL] ===== $* ====="; }
if want skim; then
step "1. skims"
# W+jets (aMC@NLO, negative weights) needs more events than the others for positive yields in the small regions
for s in "${!FILES[@]}"; do
  n=60000; [ $s = WtoLNu_2Jets ] && n=300000
  hell run configs/SingleTopMass/skim_cfg.py sample=$s era=2022 inputFiles=${FILES[$s]} outputFile=$OUT/skims/$s.root maxEvents=$n --tag $s | grep -E "wrote|forged|ERROR"
done
fi
if want trees; then
step "2. analysis trees: nominal and JES variations (central corrections: JER smearing, pileup, lepton SFs)"
for s in "${!FILES[@]}"; do
  vars="nominal jesRegrouped_TotalUp jesRegrouped_TotalDown"; [ $s = Muon_2022C ] && vars="nominal"
  for v in $vars; do
    d=$OUT/trees/2022; [ $v = nominal ] || d=${d}_$v
    mkdir -p $d/$s
    hell run configs/SingleTopMass/analysis_cfg.py sample=$s era=2022 inputFiles=$OUT/skims/$s.root outputFile=$d/$s/$s.root variation=$v --tag ${s}_$v | grep -E "wrote|ERROR"
  done
done
fi
if want bookkeep; then
step "3. bookkeeping and merge check"
hell bookkeep $OUT/trees/2022 -o $OUT/bookkeeping_2022.json --no-lumis | grep HELL
hell merge -o $OUT/merged_tch.root $OUT/trees/2022/TbarBQtoLNu_tch/*.root $OUT/trees/2022_jesRegrouped_TotalUp/TbarBQtoLNu_tch/*.root --force | grep HELL
fi
if want hist; then
step "4. histograms and plots"
hell hist test/hist_test.yaml | grep -E "wrote|ERROR"
hell plot $OUT/hists_test.root -o $OUT/plots | grep HELL
fi
if want validate; then
step "4b. object validation: data/MC metrics with and without scale factors"
hell validate test/validation_test.yaml | grep -E "HELL"
fi
if want fit; then
step "5. datacards (template and parametric); combine is only dry-run here"
hell fit test/fit_test.yaml --datacards | grep -E "wrote|ERROR"
hell fit test/fit_test.yaml --datacards --parametric | grep -E "\(parametric\)"
hell fit test/fit_test.yaml --strategy 2 --dry-run | grep -E "strategy"
fi
if want ml; then
step "6. machine learning: train, validate, apply in C++"
hell train test/ml_test.yaml --epochs 8 | grep -E "validation|exported|ERROR"
hell run configs/SingleTopMass/analysis_cfg.py sample=TbarBQtoLNu_tch era=2022 inputFiles=$OUT/skims/TbarBQtoLNu_tch.root outputFile=$OUT/analysis_ml.root model=$OUT/models/st_classifier_test.json --tag ml | grep -E "MLP|wrote|ERROR"
fi
if want navigate; then
step "7. navigation"
hell navigate $OUT/analysis_ml.root --branch 'ml_*' | grep -E "ml_|Provenance|parent" | head -8
hell navigate code --config configs/SingleTopMass/analysis_cfg.py inputFiles=$OUT/skims/TbarBQtoLNu_tch.root era=2022 --html $OUT/navigator.html | grep -E "navigator page"
hell navigate model Jet | head -5
fi
echo
echo "[HELL] Quick test done ($STEPS). Outputs in $OUT/. All forged."

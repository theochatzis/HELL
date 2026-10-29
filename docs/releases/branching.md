# Branches, tags and porting

HELL uses the CMSSW model with one difference: HELL is not inside CMSSW, so a "release cycle" is a HELL
version, not a CMSSW cycle. In the commands below `<version>` is `<maj>.<min>` such as `1.0`.

## 1. Git basics: branches develop, tags reproduce

| concept | what it is | moves? | use it for |
|---|---|---|---|
| commit | one snapshot of the code, with an id (sha) | never | a unit of change |
| branch | a named line of development, pointing at its newest commit | yes, every commit moves it | the development of one version |
| tag | a permanent name for one exact commit | never | "this exact code produced these files" |
| GitHub Release | a tag plus release notes on the GitHub page | never | telling people what changed |

## 2. How CMSSW organizes releases, and the HELL translation

CMSSW develops on `master`, opens one branch per release cycle (`CMSSW_<maj>_<min>_X`), cuts pre-releases
(`_0_pre<N>`) and releases (`_<patch>`) as tags from the cycle branch, and backports fixes by cherry-pick.

HELL:

| branch / tag | role |
|---|---|
| `main` | the latest released code. Nobody develops directly on it; a release is merged into it (fast-forward when possible) |
| `HELL_v<maj>.<min>` | the branch where version `<maj>.<min>` is developed. Branched from `main` at the previous release |
| `HELL_v<maj>.<min>_pre<N>` | tag: a pre-release snapshot of the branch, for productions that need a reproducible state before the release |
| `HELL_v<maj>.<min>` | tag: the release, on the commit that is merged into `main` |
| `HELL_v<maj>.<min>.<patch>` | tag: a fix on a released version (committed on the version branch, merged into `main`) |
| `feat/...`, `fix/...` | short-lived topic branches off the version branch, one pull request each |

Version numbers: MAJOR when the data formats or the configuration interface break existing configs or
analysis code (a product, column or parameter renamed or removed), MINOR for new features and modules, PATCH
for fixes only.

## 3. The version in every file

`hell build` bakes `git describe --tags --dirty` into `Core/interface/Version.h`; every output file carries
it in `hell/provenance` (`hell navigate file.root` shows it). `-dirty` marks uncommitted code: never run a
production with it. `hell build` reconfigures by itself when HEAD changes, so the hash is current after every
commit or checkout. Put the tag in production output paths, e.g. `.../HELL/<tag>/skims/2022/`.

## 4. Porting between branches

Development happens on the current version branch. A fix that a released version also needs is cherry-picked
to a `HELL_v<old>` branch and tagged as a patch there. Never merge one version branch into another: a merge
takes everything, a cherry-pick takes only the commits you choose. One topic per commit keeps that possible.

## 5. Practical commands

### Daily work
```bash
git branch --show-current            # on the version branch?
git pull --rebase
git switch -c feat/my-change         # one topic per branch (optional for solo work)
git commit -m "..."; git push -u origin feat/my-change
gh pr create --base HELL_v<version>
```

### Making a pre-release
```bash
git switch HELL_v<version> && git pull
git status                           # must be clean
hell build && hell pray              # must pass
git tag -a HELL_v<version>_pre<N> -m "pre-release N of <version>: what is in"
git push origin HELL_v<version>_pre<N>
```

### Making a release
```bash
git switch HELL_v<version> && git pull
hell build && hell pray
# write docs/releases/notes/HELL_v<version>.md, update docs/releases/README.md, commit
git tag -a HELL_v<version> -m "HELL <version>: summary"
git switch main && git merge --ff-only HELL_v<version>   # or a merge commit if main moved
git push origin main HELL_v<version>
gh release create HELL_v<version> --notes-file docs/releases/notes/HELL_v<version>.md
```

### Starting the next version
```bash
git switch main && git pull
git switch -c HELL_v<next>
git push -u origin HELL_v<next>
```

### A fix on a released version
```bash
git switch HELL_v<version>
git cherry-pick -x <sha>             # from the development branch, or commit the fix here
git tag -a HELL_v<version>.<patch> -m "fix: ..."
git push origin HELL_v<version> HELL_v<version>.<patch>
```

### Using a release
```bash
git describe --tags --dirty          # which version is checked out?
git switch --detach HELL_v0.0        # reproduce exactly
hell build
```

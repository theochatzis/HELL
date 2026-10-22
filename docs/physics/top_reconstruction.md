# Top reconstruction in the t-channel

The semileptonic top candidate is `l + nu + b`. The neutrino transverse momentum is the MET; its longitudinal
momentum follows from `m(l nu) = m_W`:

`pz = a +- sqrt(a^2 - (E_l^2 pT_nu^2 - mu^2)/pT_l^2)`, `a = mu pz_l / pT_l^2`, `mu = m_W^2/2 + pT_l . pT_nu`.

Two real solutions: `TopReconstructor` takes the one with the smaller |pz| by default (`pzChoice`). Negative
discriminant (about 30% of the events, from MET resolution): the real part is used; alternatives are to
rescale the MET so that the discriminant vanishes (keeps `m_T(W) = m_W`), or to regress the neutrino. The
candidate stores the discriminant and the alternative solution so that the choice can be studied.

The b jet is the leading medium-tagged jet; the light jet the most forward untagged jet. Observables:
`m(l nu b)` (the mass-sensitive one), `m(lb)` (mass-sensitive without the neutrino), `m_T(W)` (QCD
suppression), `cos theta*` (angle between lepton and light jet in the top rest frame: polarisation, signal vs
background), `|eta(j')|` (t-channel signature), `dEta(b, j')`, `HT`.

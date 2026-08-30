# Proposed TheoremDB research record — P2624 resolved with exact maximum 13

## Claim

The exact maximum is

\[
\boxed{13}.
\]

A valid 13-set is

\[
\{0,5,7,31,58,61,62,63,72,80,84,91,97\}.
\]

There is no valid 14-set.

## Prior Work and Attribution

Prior computational records `R5662` and `R5664` by **Byungwoong Yoo**
document the earlier computational result for P2624, including the explicit
13-point witness stated above; the associated evidence release is archived at
[DOI 10.5281/zenodo.21988177](https://doi.org/10.5281/zenodo.21988177).

This submission separately supplies the static `P2624T1` proof-tree
certificate and verifier for the nonexistence of a 14-set. This attribution
does not claim that `R5662` or `R5664` contained those certificate artifacts.

## Upper-bound certificate

Project a hypothetical 14-set `A subset Z/100Z` modulo 20 and write
`n_r=|A cap (r+20Z_100)|`.

The difference capacities imply

\[
\sum_r n_r(n_r-1)\le8
\]

and, for every nonzero `delta in Z_20`,

\[
\sum_r n_rn_{r+\delta}\le10.
\]

The first bound leaves seven occupancy partition types.  Exhaustive quotient
enumeration gives 204,360 raw admissible occupancy vectors.  Canonicalizing by

\[
r\mapsto ur+c\pmod{20},\qquad
u\in\{1,3,7,9,11,13,17,19\},\ c\in Z_{20},
\]

gives exactly 1,341 affine orbits.  Each action lifts to the automorphism
`x -> ux+c (mod 100)`, because every listed `u` is a unit modulo 100; hence this
symmetry reduction preserves the ordered-difference multiplicities.

For every one of the 1,341 canonical quotient configurations, a static
exhaustive lift-tree certificate is supplied.  The certificate checker does not
trust the generator's MRV heuristic.  At each node it independently enumerates
all lift masks for the residue class named by the certificate:

- a `DEAD` node is accepted only when every mask violates a circular-distance
  capacity;
- a `BRANCH` node must contain a recursively valid child for every feasible
  mask.

The seven trees contain a total of

\[
299,903,736
\]

verified proof nodes.  All seven pass the separately written independent C++
checker.  Reaching a complete valid lift, omitting a feasible branch, claiming
an invalid dead-end, corrupting the tree, truncating it, or giving a wrong node
count causes verification to fail.

Therefore no canonical quotient case lifts to a 14-set.  Since every
hypothetical 14-set is affine-equivalent to one of these cases, no 14-set
exists.

## Symmetry disclosure

The **only** proof-level symmetry reduction is the 160-element affine action on
the mod-20 occupancy vector described above.

No lift-level symmetry is used.  The final certified proof does not normalize
`0 in A`.  MRV and mask ordering are search heuristics only and are not trusted
proof rules.

## Independent checking

The attached package contains:

- `CERTIFICATE_FORMAT.md` — complete public specification of the `P2624T1`
  certificate format and its soundness rule;
- `catalog_*.txt` — the 1,341 quotient representatives;
- `verify_quotient_catalog.cpp` — independently re-enumerates raw quotient
  vectors and verifies the affine-orbit catalogs;
- `certificates/*.tree` — the seven static exhaustive certificates;
- `check_lift_tree.cpp` — independently checks every proof-tree branch;
- `verify_certified_result.py` — end-to-end verifier;
- `SHA256SUMS` — hashes of all retained proof artifacts and source files;
- successful generation/check logs plus a deliberate corruption-rejection log.

Running

```bash
python3 verify_certified_result.py
```

checks the witness, hashes, quotient coverage, and all 299,903,736 proof-tree
nodes.

## Conclusion

The explicit 13-set proves `M>=13`; the independently checked exhaustive
certificate proves `M<14`.  Hence `M=13`.

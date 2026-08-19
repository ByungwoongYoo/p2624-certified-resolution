# P2624 Certified Resolution — Exact Maximum 13

**Problem:** P2624, the 14-element `B_2[2]` set problem in `Z/100Z`  
**Canonical statement:** https://theoremdb.org/statements/b2-two-set-z100/  
**Result:**

\[
\boxed{M=13}.
\]

This package upgrades the earlier deterministic exhaustion into a **static,
independently checkable exhaustive certificate** and explicitly states every
symmetry reduction used.

## 1. Claim and 13-point witness

A valid set of size 13 is

\[
A=\{0,5,7,31,58,61,62,63,72,80,84,91,97\}.
\]

For

\[
r_A(t)=|\{(a,b)\in A^2:a\ne b,\ a-b=t\pmod{100}\}|,
\]

the maximum over nonzero `t` is 2.  Among the 99 nonzero residues, its exact
multiplicity histogram is

- 5 residues of multiplicity 0;
- 32 residues of multiplicity 1;
- 62 residues of multiplicity 2.

Hence `M >= 13`.

For reference, elementary counting gives `|A|(|A|-1) <= 99*2`; therefore a
15-set is impossible.  The certificate below rules out the only remaining
possibility, size 14.

## 2. Circular-distance formulation

For `1 <= d <= 49`, let `c_d` be the number of unordered selected pairs at
circular distance `d`.  Every such pair contributes one ordered difference `d`
and one ordered difference `100-d`, so

\[
c_d=r_A(d)=r_A(100-d)\le2.
\]

At distance 50, each antipodal unordered pair contributes two ordered
differences equal to 50, so

\[
c_{50}\le1.
\]

The lift certificates check exactly these 50 capacities.

## 3. Necessary quotient conditions modulo 20

Assume hypothetically that `|A|=14` and put

\[
n_r=|A\cap(r+20\mathbb Z_{100})|,\qquad r\in\mathbb Z_{20}.
\]

Then `sum_r n_r=14`.

### Same-class pairs

Ordered pairs in the same mod-20 class have nonzero differences in
`{20,40,60,80}`.  Each of those four differences has capacity two.  Therefore

\[
\sum_{r=0}^{19}n_r(n_r-1)\le 8,
\]

or equivalently

\[
\sum_r \binom{n_r}{2}\le4.
\]

This leaves exactly seven occupancy partition types:

- `p0`: `1^14`;
- `p1`: `2,1^12`;
- `p2`: `2^2,1^10`;
- `p3d`: `2^3,1^8`;
- `p3t`: `3,1^11`;
- `p4d`: `2^4,1^6`;
- `p4td`: `3,2,1^9`.

### Quotient differences

For every nonzero `delta` modulo 20,

\[
R_\delta=\sum_{r=0}^{19}n_r n_{r+\delta}\le10.
\]

Up to sign, `R_delta` is the sum of the five ordered-difference multiplicities
in `Z/100Z` that reduce to `delta`; each of the five is at most two.

## 4. The only symmetry reduction used

The quotient occupancy vectors are reduced by the 160 affine actions

\[
r\longmapsto ur+c\pmod{20},
\]

where

\[
u\in\{1,3,7,9,11,13,17,19\},\qquad c\in\mathbb Z_{20}.
\]

Every listed `u` is also a unit modulo 100.  Thus each quotient action lifts to

\[
x\longmapsto ux+c\pmod{100}.
\]

For two points `a,b`, the lifted map sends `a-b` to `u(a-b)`.  Multiplication by
`u` permutes the nonzero residue classes modulo 100, and translation cancels
from differences.  Consequently the condition `r_A(t)<=2` is preserved.

**This affine quotient canonicalization is the only proof-level symmetry
reduction in the final certificate.**

In particular:

- the final proof does **not** fix `0 in A`;
- there is **no lift-level symmetry reduction**;
- reflection is not an extra reduction—it is already one of the listed affine
  actions (`u=19` modulo 20);
- MRV variable choice and mask ordering are search heuristics, not symmetry
  reductions and not trusted proof rules.

Two separately implemented quotient audits reproduce the catalogs.  The C++
audit gives the following raw admissible counts and affine-orbit counts:

| type | raw admissible | affine orbits |
|---|---:|---:|
| p0 | 5,100 | 39 |
| p1 | 19,860 | 133 |
| p2 | 52,680 | 344 |
| p3d | 59,760 | 387 |
| p3t | 6,320 | 49 |
| p4d | 35,840 | 228 |
| p4td | 24,800 | 161 |
| **Total** | **204,360** | **1,341** |

Thus every hypothetical 14-set is affine-equivalent to a lift of one of the
1,341 catalog rows.

## 5. Static exhaustive lift certificate

The seven files in `certificates/*.tree` are not search logs.  They are static
proof trees in the documented `P2624T1` format.

For a fixed quotient row, a residue class of occupancy `k` has exactly
`C(5,k)` possible lift masks among

\[
r,\ r+20,\ r+40,\ r+60,\ r+80.
\]

Each proof-tree node names one currently unassigned residue class and is one of
two kinds:

1. **DEAD:** the independent checker enumerates every lift mask for that class
   and verifies that all of them immediately exceed a distance capacity.
2. **BRANCH:** the checker independently enumerates every currently feasible
   lift mask and requires one recursively valid child subtree for **every** such
   mask.

The checker does not reproduce the generator's MRV heuristic and does not trust
its node count.  If a feasible child is omitted, a dead node is unjustified, a
branch variable is invalid, a tree is truncated, its declared node count is
wrong, or a complete valid 14-point assignment is reached, verification fails.

The binary format and the soundness induction are fully specified in
`CERTIFICATE_FORMAT.md`, so a third party can write a fresh checker without
using the supplied implementation.

### Verified certificate totals

| type | configurations | tree nodes |
|---|---:|---:|
| p0 | 39 | 24,179,709 |
| p1 | 133 | 55,614,223 |
| p2 | 344 | 86,217,164 |
| p3d | 387 | 77,335,947 |
| p3t | 49 | 10,177,494 |
| p4d | 228 | 27,482,183 |
| p4td | 161 | 18,897,016 |
| **Total** | **1,341** | **299,903,736** |

Every tree was generated and then checked by the separately written
`check_lift_tree.cpp`.  The successful checker outputs are retained under
`logs/`.  A final end-to-end run from the clean package is retained as
`logs/final_full_verify.out` / `logs/final_full_verify.err`; it ended with
`PASS exhaustive certificate` and exit code 0.

As a consistency check, the total `299,903,736` is also exactly the node total
reported earlier by the independent mask-signature exhaustive solver retained
under `crosschecks/`.  That agreement is not required for certificate
soundness—the static tree checker is the decisive check—but it is additional
reproduction evidence.

A deliberate one-byte corruption of the `p3t` tree was also tested.  The
checker rejected it with

```
s NOT VERIFIED: invalid proof branch variable
```

and exit code 1; see `logs/corruption_test.*`.

## 6. Why this proves nonexistence

The proof is a finite induction over each static tree.

At a `DEAD` node there is no valid choice for the named residue class, so no
completion exists.  At a `BRANCH` node every feasible choice has a certified
child and every child is impossible by induction; hence the branch itself has
no completion.  A successful checker never reaches a complete valid lift.

Therefore none of the 1,341 canonical quotient occupancies has a valid lift to
a 14-set in `Z/100Z`.

Because every hypothetical 14-set is affine-equivalent to one of those cases,
no 14-set exists.  Together with the explicit 13-set,

\[
\boxed{M=13}.
\]

## 7. Verification

Requirements: Python 3 and a C++17 compiler (`g++` or `clang++`).

Run the full independent verification from the extracted directory:

```bash
python3 verify_certified_result.py
```

This checks the SHA-256 manifest and witness, independently re-enumerates all
quotient catalogs with the C++ verifier, compiles the proof-tree checker, and
traverses all 299,903,736 certificate nodes.

A fast integrity/catalog check that intentionally skips the large tree traversal
is available as:

```bash
python3 verify_certified_result.py --quick
```

The read-only secondary Python quotient audit can be run with:

```bash
python3 audit_quotient_catalogs.py
```

To regenerate the static proof trees rather than merely check them, compile
`emit_lift_tree.cpp` and run it once per catalog.  Regeneration is not needed to
validate the existing certificate.

## 8. Acceptance-condition mapping

The TheoremDB requirement quoted for this problem is:

> A computational proof must include an independently checkable exhaustive
> certificate and state every symmetry reduction used.

This package addresses it directly:

- **Independently checkable exhaustive certificate:** the seven static
  `P2624T1` proof trees, the public format specification, and
  `check_lift_tree.cpp`.
- **Exhaustive quotient coverage:** the seven catalog files plus an independently
  written C++ quotient enumerator/checker and a second Python enumeration.
- **Every symmetry reduction stated:** only the 160-element affine quotient
  action above; no lift-level symmetry and no `0 in A` normalization in the
  final proof.

The certificate is a custom exhaustive-tree format rather than DRAT/LRAT.  It
has an explicit inference rule and an independent checker, so its soundness does
not depend on trusting the search generator or rerunning the search heuristic.

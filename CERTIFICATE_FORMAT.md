# P2624T1 Exhaustive Lift-Tree Certificate Format

This document specifies the static certificate used to prove that no 14-element
`B_2[2]` set exists in `Z/100Z` after the quotient occupancy vector has been
fixed.  It is intentionally simple enough for an independent checker to be
written without using any code from the certificate generator.

## Mathematical state

For one catalog row `n_0,...,n_19`, let

- `S = (r_0 < ... < r_{m-1})` be the residues with `n_r > 0`;
- the domain of class index `v` be every `n_{r_v}`-subset of `{0,1,2,3,4}`;
- a mask `M` in that domain select the points `r_v + 20q (mod 100)` for `q in M`.

Masks are ordered by their five-bit integer representation, ascending.

A partial assignment maintains unordered circular-distance counts
`c_1,...,c_50`.  A candidate mask is **feasible** iff adding its internal pairs
and its pairs to all previously selected points leaves

- `c_d <= 2` for `1 <= d <= 49`, and
- `c_50 <= 1`.

These are exactly equivalent to the ordered-difference bounds
`r_A(t) <= 2`: an unordered pair at circular distance `d < 50` contributes once
to difference `d` and once to `100-d`, while an antipodal pair contributes twice
to difference `50`.

## Binary file layout

All integer headers are little-endian.

1. 8 bytes: ASCII magic `P2624T1\n`.
2. 4 bytes: unsigned 32-bit number of catalog configurations.
3. For each catalog row, in file order:
   - 8 bytes: unsigned 64-bit number of proof-tree nodes for this row;
   - the recursively encoded proof tree, one byte per node.

There are no separators between child subtrees.  Recursive parsing determines
where each child ends.  The declared node count must equal the number of node
bytes consumed for that catalog row.

After the final tree there must be no trailing bytes.

## Node byte

Bits 6, 5 and 4 are reserved and must all be zero.

- Bits 3..0 encode a class index `v` in `S`.
- Bit 7 is the node kind:
  - `0`: **BRANCH(v)**
  - `1`: **DEAD(v)**

The named class must be currently unassigned.

### DEAD(v)

The checker independently enumerates **every** mask in the domain of class `v`
and recomputes its distance contribution to the current partial assignment.
`DEAD(v)` is valid iff every mask exceeds at least one distance capacity.
It has no children.

### BRANCH(v)

The checker independently enumerates every currently feasible mask in the
domain of class `v`.

- The feasible set must be nonempty.
- For each feasible mask, in ascending mask order, exactly one child proof tree
  follows.
- The checker applies that mask, recursively validates the child, then undoes
  the mask before checking the next child.

Thus a branch cannot omit a feasible lift option.

If all nonzero residue classes become assigned, the checker **rejects**: it has
reached a complete valid 14-point lift.  A successful certificate therefore has
no successful complete-assignment leaf.

## Soundness

Soundness is an induction on the number of unassigned residue classes.

At a `DEAD(v)` node, every possible choice for class `v` is independently shown
to violate a distance capacity, so no completion exists.

At a `BRANCH(v)` node, any completion must choose one of the feasible masks for
`v`.  The format requires a recursively valid child certificate for every such
mask.  By induction every child has no completion; hence the parent has no
completion.

Consequently, if an independent checker successfully consumes the complete tree
for a quotient row, that row has no lift satisfying the P2624 constraints.

## Search heuristics are not proof rules

The supplied generator uses minimum-remaining-values (MRV) to choose which
class to branch on.  **The checker does not reproduce or trust MRV.**  A
certificate may name any currently unassigned class.  The only accepted proof
rules are `DEAD` and exhaustive `BRANCH` as specified above.

There is no lift-level symmetry reduction in this certificate format.

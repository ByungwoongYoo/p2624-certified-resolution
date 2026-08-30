# P2624 certified resolution: exact maximum 13

This release contains the static exhaustive certificate supporting the result

> No 14-element subset of `Z/100Z` has every nonzero ordered difference occurring at most twice. The exact maximum cardinality is 13.

## Prior Work and Attribution

Prior computational records `R5662` and `R5664` by **Byungwoong Yoo**
document the earlier computational result for P2624, including the explicit
13-point witness included in this release; the associated evidence release is
archived at
[DOI 10.5281/zenodo.21988177](https://doi.org/10.5281/zenodo.21988177).

This release separately packages the static `P2624T1` proof-tree certificate
and verifier for the upper bound.

## Release asset

- Asset: `P2624_certified_resolution.zip`
- Size: 123,667,693 bytes
- SHA-256: `facbdfec87f5bfb302f197af5c13cce6e984444bd8270adce250de5e8fab8a35`

The ZIP contains the seven `P2624T1` proof trees, their manifest, checker and generator sources, quotient catalogs, retained verification logs, and the complete reproduction instructions. The repository preserves the lightweight source and audit material; the proof-tree binaries are intentionally excluded from Git history.

## Verification

After extracting the asset, run:

```bash
python3 verify_certified_result.py
```

This verifies the SHA-256 manifest, the explicit 13-point witness, independent quotient enumeration and affine-orbit coverage, and all 299,903,736 proof-tree nodes. See `README.md` and `CERTIFICATE_FORMAT.md` for the mathematical reduction and certificate specification.

Canonical TheoremDB problem: <https://theoremdb.org/statements/b2-two-set-z100/>

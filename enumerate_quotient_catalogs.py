#!/usr/bin/env python3
"""Independently enumerate quotient occupancy orbits for P2624.

A putative 14-element B_2[2] set A in Z/100Z is projected modulo 20.
This script enumerates, up to the full affine group AGL(1,Z/20Z), every
occupancy vector that can satisfy the necessary quotient capacity bounds.
The resulting catalogs are the quotient layer of the static P2624T1 lift-tree
certificate.  They are independently rechecked by verify_quotient_catalog.cpp.
"""

from __future__ import annotations

import hashlib
import itertools
from pathlib import Path
from typing import Iterable, Iterator, Sequence

Q = 20
K = 14
UNITS = (1, 3, 7, 9, 11, 13, 17, 19)

# label -> (support size, number of doubled classes, number of tripled classes)
TYPES: dict[str, tuple[int, int, int]] = {
    "p0": (14, 0, 0),
    "p1": (13, 1, 0),
    "p2": (12, 2, 0),
    "p3d": (11, 3, 0),
    "p3t": (12, 0, 1),
    "p4d": (10, 4, 0),
    "p4td": (11, 1, 1),
}

EXPECTED_COUNTS = {
    "p0": 39,
    "p1": 133,
    "p2": 344,
    "p3d": 387,
    "p3t": 49,
    "p4d": 228,
    "p4td": 161,
}

EXPECTED_SHA256 = {
    "p0": "72fb157321bd1b881fd356a41f3892e596085d9ffcda892e23360c99ff88f393",
    "p1": "64a30a5da125faa86bf32b81112d11f628b11a016a0fb4aaa4b2902689b03866",
    "p2": "db7640ff8ea6976176f897bd582e69a0717e8901485451aa16f42b20a004d7ad",
    "p3d": "74814c3c6475be72ec9d426264285d37e4cbca8fd0888088c2e344879dc034e5",
    "p3t": "e4f595bfc41a344ec5c433e458dae22194b3e68433d7178cfb17f46208b72ca8",
    "p4d": "fa7d4a2846ff29154aa2eaf05802cbd24dec7e32045f1d9e180e97e99cd34a69",
    "p4td": "8258f362a4ec3f2384ee0abd4a5670f1c66b9d7b59c6f7c7562c2130162072c8",
}


def transform_mask(mask: int, unit: int, shift: int) -> int:
    result = 0
    for r in range(Q):
        if (mask >> r) & 1:
            result |= 1 << ((unit * r + shift) % Q)
    return result


def mask_key(mask: int) -> tuple[int, ...]:
    return tuple((mask >> r) & 1 for r in range(Q))


def support_representatives(size: int) -> list[int]:
    """Return one representative per affine orbit of size-*size* supports."""
    seen: set[int] = set()
    representatives: list[int] = []

    for support in itertools.combinations(range(Q), size):
        mask = sum(1 << r for r in support)
        if mask in seen:
            continue

        orbit = {
            transform_mask(mask, unit, shift)
            for unit in UNITS
            for shift in range(Q)
        }
        seen.update(orbit)
        representatives.append(min(orbit, key=mask_key))

    representatives.sort(key=mask_key)
    return representatives


def transform_counts(
    occupancy: Sequence[int], unit: int, shift: int
) -> tuple[int, ...]:
    transformed = [0] * Q
    for r, value in enumerate(occupancy):
        transformed[(unit * r + shift) % Q] = value
    return tuple(transformed)


def weighted_key(occupancy: Sequence[int]) -> tuple[tuple[int, ...], tuple[int, ...]]:
    # Support-first ordering is deliberate. It ensures that, when we generate
    # weighted assignments only on a canonical support, the canonical weighted
    # representative also lies on that same canonical support.
    support = tuple(1 if value else 0 for value in occupancy)
    return support, tuple(occupancy)


def canonical_counts(occupancy: Sequence[int]) -> tuple[int, ...]:
    orbit = (
        transform_counts(occupancy, unit, shift)
        for unit in UNITS
        for shift in range(Q)
    )
    return min(orbit, key=weighted_key)


def quotient_ok(occupancy: Sequence[int]) -> bool:
    if len(occupancy) != Q or sum(occupancy) != K:
        return False

    # Differences divisible by 20 occupy four nonzero residues in Z/100Z,
    # each of capacity two. Thus at most eight ordered same-class pairs.
    if sum(value * (value - 1) for value in occupancy) > 8:
        return False

    # For each nonzero delta modulo 20, exactly five nonzero residues modulo
    # 100 reduce to delta. Each has ordered-difference capacity two.
    for delta in range(1, Q):
        count = sum(
            occupancy[r] * occupancy[(r + delta) % Q]
            for r in range(Q)
        )
        if count > 10:
            return False

    return True


def assignments_for_support(
    support: Sequence[int], double_count: int, triple_count: int
) -> Iterator[tuple[int, ...]]:
    """Generate all occupancy assignments of the requested partition type."""
    if triple_count not in (0, 1):
        raise ValueError("Only zero or one tripled class is needed")

    if triple_count == 0:
        for doubled in itertools.combinations(support, double_count):
            occupancy = [0] * Q
            for r in support:
                occupancy[r] = 1
            for r in doubled:
                occupancy[r] = 2
            yield tuple(occupancy)
        return

    # The only tripled type with a doubled class is p4td. The roles are
    # distinct, so enumerate the triple first and choose doubles elsewhere.
    for tripled in support:
        remaining = tuple(r for r in support if r != tripled)
        for doubled in itertools.combinations(remaining, double_count):
            occupancy = [0] * Q
            for r in support:
                occupancy[r] = 1
            occupancy[tripled] = 3
            for r in doubled:
                occupancy[r] = 2
            yield tuple(occupancy)


def make_catalog(
    support_size: int, double_count: int, triple_count: int
) -> list[tuple[int, ...]]:
    representatives: set[tuple[int, ...]] = set()
    for support_mask in support_representatives(support_size):
        support = tuple(r for r in range(Q) if (support_mask >> r) & 1)
        for occupancy in assignments_for_support(
            support, double_count, triple_count
        ):
            if quotient_ok(occupancy):
                representatives.add(canonical_counts(occupancy))

    return sorted(representatives, key=weighted_key)


def serialize(catalog: Iterable[Sequence[int]]) -> bytes:
    return "".join("".join(map(str, row)) + "\n" for row in catalog).encode("ascii")


def main() -> None:
    output_dir = Path(__file__).resolve().parent
    summary_rows = ["type\tconfigs\tsha256"]
    total = 0

    for label, (support_size, double_count, triple_count) in TYPES.items():
        catalog = make_catalog(support_size, double_count, triple_count)
        data = serialize(catalog)
        digest = hashlib.sha256(data).hexdigest()
        path = output_dir / f"catalog_{label}.txt"
        path.write_bytes(data)

        expected_count = EXPECTED_COUNTS[label]
        expected_digest = EXPECTED_SHA256[label]
        if len(catalog) != expected_count:
            raise AssertionError(
                f"{label}: got {len(catalog)} configurations, expected {expected_count}"
            )
        if digest != expected_digest:
            raise AssertionError(
                f"{label}: got sha256 {digest}, expected {expected_digest}"
            )

        total += len(catalog)
        summary_rows.append(f"{label}\t{len(catalog)}\t{digest}")
        print(f"{label}: {len(catalog)} configurations, sha256 {digest}")

    if total != 1341:
        raise AssertionError(f"total configurations {total}, expected 1341")

    (output_dir / "catalog_summary.tsv").write_text(
        "\n".join(summary_rows) + "\n", encoding="utf-8"
    )
    print(f"TOTAL: {total} affine quotient orbits")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Read-only Python audit of the seven quotient catalogs."""
from __future__ import annotations
import hashlib
from pathlib import Path
import enumerate_quotient_catalogs as enum

BASE=Path(__file__).resolve().parent

def main():
    total=0
    for label,(support_size,double_count,triple_count) in enum.TYPES.items():
        catalog=enum.make_catalog(support_size,double_count,triple_count)
        data=enum.serialize(catalog)
        actual=(BASE/f'catalog_{label}.txt').read_bytes()
        if data!=actual: raise SystemExit(f'FAIL {label}: regenerated catalog differs')
        digest=hashlib.sha256(actual).hexdigest()
        if len(catalog)!=enum.EXPECTED_COUNTS[label] or digest!=enum.EXPECTED_SHA256[label]:
            raise SystemExit(f'FAIL {label}: count/hash mismatch')
        total+=len(catalog)
        print(f'PASS {label}: configs={len(catalog)} sha256={digest}')
    if total!=1341: raise SystemExit(f'FAIL total={total}')
    print('PASS TOTAL: 1341 affine quotient orbits')
if __name__=='__main__':main()

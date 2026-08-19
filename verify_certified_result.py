#!/usr/bin/env python3
"""End-to-end verifier for the P2624 certified resolution package.

Default mode independently checks:
  1. all SHA-256 manifest entries;
  2. the explicit 13-point witness;
  3. all quotient catalogs with a separately written C++ enumerator;
  4. all seven static exhaustive lift-tree certificates.

Use --quick to skip the 299,903,736-node tree traversal while retaining hashes,
witness verification, and quotient-catalog verification.
"""
from __future__ import annotations
import argparse, hashlib, os, shutil, subprocess, sys, tempfile
from collections import Counter
from pathlib import Path

BASE=Path(__file__).resolve().parent
LABELS=['p0','p1','p2','p3d','p3t','p4d','p4td']
EXPECTED_NODES={
 'p0':24179709,'p1':55614223,'p2':86217164,'p3d':77335947,
 'p3t':10177494,'p4d':27482183,'p4td':18897016,
}
EXPECTED_CONFIGS={'p0':39,'p1':133,'p2':344,'p3d':387,'p3t':49,'p4d':228,'p4td':161}
WITNESS=[0,5,7,31,58,61,62,63,72,80,84,91,97]

def sha256(path:Path)->str:
    h=hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda:f.read(8*1024*1024),b''):h.update(chunk)
    return h.hexdigest()

def verify_manifest():
    manifest=BASE/'SHA256SUMS'
    if not manifest.exists(): raise RuntimeError('SHA256SUMS missing')
    count=0
    for line in manifest.read_text().splitlines():
        if not line.strip():continue
        digest,rel=line.split('  ',1);p=BASE/rel
        if not p.is_file(): raise RuntimeError(f'manifest file missing: {rel}')
        got=sha256(p)
        if got!=digest: raise RuntimeError(f'hash mismatch: {rel}\n expected {digest}\n got      {got}')
        count+=1
    print(f'PASS manifest: {count} files')

def verify_witness():
    A=WITNESS
    if len(A)!=13 or len(set(A))!=13 or any(x<0 or x>=100 for x in A):raise RuntimeError('bad witness shape')
    r=Counter((a-b)%100 for a in A for b in A if a!=b)
    m=max(r[t] for t in range(1,100))
    if m>2:raise RuntimeError(f'witness violates difference capacity: max={m}')
    hist=Counter(r[t] for t in range(1,100))
    print(f'PASS 13-point witness: max multiplicity={m}, histogram 0:{hist[0]} 1:{hist[1]} 2:{hist[2]}')

def compiler():
    cxx=os.environ.get('CXX')
    if cxx and shutil.which(cxx):return cxx
    for name in ('g++','clang++'):
        p=shutil.which(name)
        if p:return p
    raise RuntimeError('Need g++ or clang++ to compile the independent C++ checkers')

def compile_cpp(cxx,src:Path,out:Path):
    subprocess.run([cxx,'-O3','-DNDEBUG','-std=c++17',str(src),'-o',str(out)],check=True)

def run_capture(args):
    p=subprocess.run(args,text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    if p.stdout:print(p.stdout,end='')
    if p.stderr:print(p.stderr,end='',file=sys.stderr)
    if p.returncode!=0:raise RuntimeError(f'command failed ({p.returncode}): {" ".join(map(str,args))}')
    return p.stdout

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--quick',action='store_true',help='skip full proof-tree traversal');a=ap.parse_args()
    verify_manifest();verify_witness();cxx=compiler()
    with tempfile.TemporaryDirectory(prefix='p2624_verify_') as td:
        td=Path(td);qcheck=td/'verify_quotient_catalog';tcheck=td/'check_lift_tree'
        compile_cpp(cxx,BASE/'verify_quotient_catalog.cpp',qcheck)
        compile_cpp(cxx,BASE/'check_lift_tree.cpp',tcheck)
        raw_total=orbit_total=0
        for lab in LABELS:
            out=run_capture([str(qcheck),lab,str(BASE/f'catalog_{lab}.txt')])
            if f'affine_orbits={EXPECTED_CONFIGS[lab]}' not in out:raise RuntimeError(f'unexpected quotient orbit count for {lab}')
        print('PASS quotient coverage: all seven catalogs independently enumerated')
        if a.quick:
            print('QUICK PASS: full tree traversal intentionally skipped')
            return
        total=0
        for lab in LABELS:
            out=run_capture([str(tcheck),str(BASE/f'catalog_{lab}.txt'),str(BASE/'certificates'/f'{lab}.tree')])
            n=EXPECTED_NODES[lab]
            if f'nodes={n}' not in out:raise RuntimeError(f'unexpected proof-tree node count for {lab}')
            total+=n
        if total!=299_903_736:raise RuntimeError(f'unexpected total nodes {total}')
        print('PASS exhaustive certificate: 1,341 quotient cases; 299,903,736 proof-tree nodes')
        print('CONCLUSION: no 14-point set exists; the exact maximum is 13')
if __name__=='__main__':
    try:main()
    except Exception as e:
        print(f'FAIL: {e}',file=sys.stderr);sys.exit(1)

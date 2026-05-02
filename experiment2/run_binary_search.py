#!/usr/bin/env python3

import os
import subprocess
import sys
import uuid

SOURCES = [
    {"name": "v1", "file": "experiment_binary_search.cpp"},
    {"name": "v2", "file": "experiment_binary_search_2.cpp"},
]
MODES = ["1"]
QUERIES_NUMBER = 10_000_000
QUERIES_SEED = 42
NS = [1_000_000_000]
PATH = "data/numbers_1e9"

BINARIES_DIR = "./binaries"


def make_compiler_configs(run_dir, source):
    slug = source["name"]
    return [
        {
            "source": source,
            "name": f"g++-13/{slug}",
            "cmd": "g++-13",
            "flags": ["-std=c++23", "-static", "-fomit-frame-pointer", "-fno-exceptions", "-fno-rtti", "-O3"],
            "binary": f"{run_dir}/experiment_{slug}_gcc13",
        },
    ]


def compile(compiler):
    src = compiler["source"]["file"]
    cmd = [compiler["cmd"], src] + compiler["flags"] + ["-o", compiler["binary"]]
    print(f"Compiling {src} with {compiler['name']}: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"  FAILED:\n{result.stderr}", file=sys.stderr)
        return False
    print(f"  OK -> {compiler['binary']}")
    return True


def run(binary, mode, n):
    cmd = [binary, mode, str(n), str(QUERIES_NUMBER), str(QUERIES_SEED), PATH]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        return None, result.stderr.strip()
    return result.stdout.strip(), None


def main():
    run_dir = os.path.join(BINARIES_DIR, str(uuid.uuid4()))
    os.makedirs(run_dir)
    print(f"Run directory: {run_dir}\n")

    all_compilers = []
    for source in SOURCES:
        all_compilers.extend(make_compiler_configs(run_dir, source))

    compiled = []
    for compiler in all_compilers:
        if compile(compiler):
            compiled.append(compiler)
    print()

    if not compiled:
        print("No compilers succeeded, aborting.", file=sys.stderr)
        sys.exit(1)

    # Group results by (compiler_base, mode, n) for side-by-side comparison.
    # compiler_base strips the source slug so v1/v2 of the same compiler are paired.
    results = {}
    for compiler in compiled:
        base = compiler["name"].rsplit("/", 1)[0]  # e.g. "g++-13"
        slug = compiler["source"]["name"]           # e.g. "v1" or "v2"
        for mode in MODES:
            for n in NS:
                key = (base, mode, n)
                if key not in results:
                    results[key] = {}
                ns_per_query, err = run(compiler["binary"], mode, n)
                results[key][slug] = ns_per_query if err is None else f"ERROR: {err}"

    source_names = [s["name"] for s in SOURCES]
    col = 14
    header = f"{'compiler':<16} {'mode':<6} {'n':>14}" + "".join(f"  {s:>{col}}" for s in source_names)
    if len(source_names) == 2:
        header += f"  {'ratio(v2/v1)':>14}"
    print(header)
    print("-" * len(header))

    for (base, mode, n), vals in sorted(results.items()):
        row = f"{base:<16} {mode:<6} {n:>14}"
        for s in source_names:
            v = vals.get(s, "N/A")
            row += f"  {v:>{col}}"
        if len(source_names) == 2:
            v1 = vals.get(source_names[0])
            v2 = vals.get(source_names[1])
            try:
                ratio = f"{float(v2) / float(v1):.4f}"
            except (TypeError, ValueError, ZeroDivisionError):
                ratio = "N/A"
            row += f"  {ratio:>14}"
        print(row)


if __name__ == "__main__":
    main()

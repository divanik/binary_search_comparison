#!/usr/bin/env python3

import os
import subprocess
import sys
import tempfile

SRC_DIR = os.path.join(os.path.dirname(__file__), "src")

N = 1_000_000_000
QUERIES_NUMBER = 10_000_000
QUERIES_SEED = 42
MODES = ["1", "2"]

DATA_DIR = os.path.join(os.path.dirname(__file__), "data")
DEFAULT_NUMBERS_PATH = os.path.join(DATA_DIR, f"numbers_{N}_{QUERIES_SEED}")
DEFAULT_QUERIES_PATH = os.path.join(DATA_DIR, f"queries_{QUERIES_NUMBER}_{QUERIES_SEED}")


def run(cmd, **kwargs):
    result = subprocess.run(cmd, capture_output=True, text=True, **kwargs)
    if result.returncode != 0:
        print(f"FAILED: {' '.join(cmd)}\n{result.stderr}", file=sys.stderr)
        sys.exit(1)
    return result


def compile_bench(out):
    cmd = [
        "gcc-14", "-O2", "-o", out,
        os.path.join(SRC_DIR, "searches.s"),
        os.path.join(SRC_DIR, "simplified_binary_search.c"),
    ]
    print(f"Compiling bench: {' '.join(cmd)}")
    run(cmd)
    print(f"  OK -> {out}")


def compile_gen(out):
    cmd = [
        "g++-14", "-std=c++23", "-O2", "-o", out,
        os.path.join(SRC_DIR, "generate_data.cpp"),
    ]
    print(f"Compiling gen: {' '.join(cmd)}")
    run(cmd)
    print(f"  OK -> {out}")


def generate_data(gen_bin, n, queries_number, queries_seed, numbers_path, queries_path):
    os.makedirs(os.path.dirname(numbers_path), exist_ok=True)
    os.makedirs(os.path.dirname(queries_path), exist_ok=True)

    if os.path.exists(numbers_path) and os.path.exists(queries_path):
        print(f"Data already exists, skipping generation")
        print(f"  numbers: {numbers_path}")
        print(f"  queries: {queries_path}")
        return

    cmd = [gen_bin, str(n), str(queries_number), str(queries_seed), numbers_path, queries_path]
    print(f"Generating data (n={n}, queries={queries_number}, seed={QUERIES_SEED})")
    result = run(cmd)
    if result.stderr:
        print(f"  {result.stderr.strip()}")


def benchmark(bench_bin, mode, numbers_path, queries_path):
    cmd = [bench_bin, mode, numbers_path, queries_path]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        return None, result.stderr.strip()
    return result.stdout.strip(), None


def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--numbers-path", default=DEFAULT_NUMBERS_PATH)
    parser.add_argument("--queries-path", default=DEFAULT_QUERIES_PATH)
    args = parser.parse_args()

    with tempfile.TemporaryDirectory() as tmpdir:
        bench_bin = os.path.join(tmpdir, "bench")
        gen_bin = os.path.join(tmpdir, "gen")

        compile_bench(bench_bin)
        compile_gen(gen_bin)
        print()

        generate_data(gen_bin, N, QUERIES_NUMBER, QUERIES_SEED, args.numbers_path, args.queries_path)
        print()

        print(f"{'mode':<6} {'n':>12}  {'ns/query':>12}")
        print("-" * 36)
        for mode in MODES:
            ns_per_query, err = benchmark(bench_bin, mode, args.numbers_path, args.queries_path)
            if err is not None:
                print(f"{mode:<6} {N:>12}  ERROR: {err}")
            else:
                print(f"{mode:<6} {N:>12}  {ns_per_query:>12}")


if __name__ == "__main__":
    main()

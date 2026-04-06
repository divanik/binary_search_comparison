#!/usr/bin/env python3

import os
import subprocess
import sys
import uuid

SOURCE = "experiment_binary_search.cpp"
MODES = ["btree"]
QUERIES_NUMBER = 10_000_000
QUERIES_SEED = 42
NS = [1_000_000_000]
PATH = "./data/numbers_1e9"

BINARIES_DIR = "./binaries"

# Flags for assembly output: drop -static (linker-only flag)
ASM_FLAGS = ["-std=c++23", "-fomit-frame-pointer", "-fno-exceptions", "-fno-rtti", "-O3"]


ALIGN_VARIANTS = [
    ("base",            ["-std=c++23", "-O3"]),
    ("+ static",        ["-std=c++23", "-O3", "-static"]),
    ("+ omit-fp",       ["-std=c++23", "-O3", "-fomit-frame-pointer"]),
    ("+ no-exceptions", ["-std=c++23", "-O3", "-fno-exceptions"]),
    ("+ no-rtti",       ["-std=c++23", "-O3", "-fno-rtti"]),
    ("all slow flags",  ["-std=c++23", "-O3", "-static", "-fomit-frame-pointer", "-fno-exceptions", "-fno-rtti"]),
    ("all + align",     ["-std=c++23", "-O3", "-static", "-fomit-frame-pointer", "-fno-exceptions", "-fno-rtti", "-falign-loops=32"]),
]


def make_compilers(run_dir):
    compilers = []
    for name, flags in ALIGN_VARIANTS:
        slug = name.replace("+", "p").replace(" ", "_").replace("-", "_")
        compilers.append({
            "name": name,
            "cmd": "g++-13",
            "flags": flags,
            "binary": f"{run_dir}/exp_{slug}",
            "asm": f"{run_dir}/exp_{slug}.s",
            "disasm": f"{run_dir}/exp_{slug}.asm",
        })
    return compilers


def compile(compiler):
    cmd = [compiler["cmd"], SOURCE] + compiler["flags"] + ["-o", compiler["binary"]]
    print(f"Compiling with {compiler['name']}: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"  FAILED:\n{result.stderr}", file=sys.stderr)
        return False
    print(f"  OK -> {compiler['binary']}")
    return True


def disassemble(compiler):
    cmd = ["objdump", "-d", "-C", "-M", "intel", compiler["binary"]]
    print(f"Disassembling {compiler['binary']}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"  FAILED:\n{result.stderr}", file=sys.stderr)
        return False
    with open(compiler["disasm"], "w") as f:
        f.write(result.stdout)
    print(f"  OK -> {compiler['disasm']}")
    return True


def compile_asm(compiler):
    cmd = [compiler["cmd"], SOURCE] + ASM_FLAGS + ["-S", "-o", compiler["asm"]]
    print(f"Compiling assembly with {compiler['name']}: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"  FAILED:\n{result.stderr}", file=sys.stderr)
        return False
    print(f"  OK -> {compiler['asm']}")
    return True


def run(binary, mode, n):
    cmd = [binary, mode, str(n), str(QUERIES_NUMBER), str(QUERIES_SEED), PATH]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        return None, result.stderr.strip()
    ns_per_query = result.stdout.strip()
    return ns_per_query, None


def main():
    run_dir = os.path.join(BINARIES_DIR, str(uuid.uuid4()))
    os.makedirs(run_dir)
    print(f"Run directory: {run_dir}\n")

    compilers = make_compilers(run_dir)

    # Compile all (binary + assembly)
    compiled = []
    for compiler in compilers:
        if compile(compiler):
            compiled.append(compiler)
            disassemble(compiler)
        compile_asm(compiler)
    print()

    if not compiled:
        print("No compilers succeeded, aborting.", file=sys.stderr)
        sys.exit(1)

    print(f"{'compiler':<16} {'mode':<6} {'n':>14}  {'ns/query':>12}")
    print("-" * 56)

    # Run all combinations
    for compiler in compiled:
        for mode in MODES:
            for n in NS:
                ns_per_query, err = run(compiler["binary"], mode, n)
                if err is not None:
                    print(f"{compiler['name']:<16} {mode:<6} {n:>14}  ERROR: {err}")
                else:
                    print(f"{compiler['name']:<16} {mode:<6} {n:>14}  {ns_per_query:>12}")


if __name__ == "__main__":
    main()

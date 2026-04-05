# binary_search_comparison

## Usage

```
python3 run_binary_search.py
```

## Requirements

The following compilers must be available on the system:

- `clang++-22`
- `g++-14`
- `g++-13`

## Directory layout

- `data/` — intermediate data files (potentially large); stores the pre-generated sorted array of numbers used for benchmarking
- `binaries/` — build output; each run creates a subdirectory named by UUID containing compiled binaries and assembly files (`.s`, `.asm`)

## Results

```
compiler         mode                n      ns/query
--------------------------------------------------------
clang++-22       1          1000000000     481.64667
clang++-22       2          1000000000     958.13532
g++-14           1          1000000000     486.56579
g++-14           2          1000000000     493.63531
g++-13           1          1000000000     884.78897
g++-13           2          1000000000     480.36835
```

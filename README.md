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

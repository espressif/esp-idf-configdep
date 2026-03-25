# esp-idf-configdep

A compiler wrapper that optimizes ESP-IDF incremental builds by replacing
the coarse `sdkconfig.h` dependency with granular per-option `.cdep` files.

## How it works

1. The build system invokes `esp-idf-configdep` instead of the compiler.
2. It forwards all arguments to the real compiler via `exec_process()`.
3. After compilation, it looks for `-MF <file>` in argv to locate the
   compiler-generated `.d` (dependency) file.
4. If `sdkconfig.h` appears in the `.d`, it is removed and replaced with
   per-option dependencies (e.g. `CONFIG_MY_OPTION` -> `my/option.cdep`).
5. Every prerequisite in the `.d` (source, headers, `.inc`, etc.) is scanned
   for `CONFIG_*` references — no deny-lists, following kernel `fixdep`
   philosophy where over-reporting is safe.
6. If a `.cdep` file does not exist, an empty stub is created (with parent
   directories) so Ninja can always track it.

## Repository layout

```
configdep.c   Main logic: depfile parsing, CONFIG_* scanning, .d rewriting
membuf.[ch]   Lightweight memory buffer abstraction (no NUL requirement)
utils.[ch]    Error reporting (err/err_errno macros), touch_file helper
port.h        Platform abstraction API (macros + declarations)
port.c        Captures raw fprintf/vfprintf before port.h overrides them
posix.c       POSIX exec_process (fork/execvp)
win.c         Windows exec_process (CreateProcessW), UTF-8 I/O wrappers
wmain.c       Windows entry: converts wchar_t argv to UTF-8, calls main()
wconv.[ch]    UTF-8 <-> wide-char conversion helpers (Windows only)
Makefile      Build system, test runner, packaging, lint targets
t/            Integration tests (TAP protocol, bash)
t/tap.sh      Minimal TAP helpers for bash tests
.clang-tidy   Linter configuration
```

## Platform-specific code

All platform differences are isolated behind `port.h`:

- **`port.h`** defines macros that redirect standard functions to
  platform-appropriate implementations. On Windows, `fopen`, `access`,
  `fprintf`, `vfprintf`, and `mkdir` are redirected to UTF-8-aware
  `_w` suffixed wrappers. On POSIX they are available natively.
- **`win.c`** implements all Windows-specific functions: `exec_process`
  (via `CreateProcessW`), `fopen_w`, `access_w`, `mkdir_w`,
  `fprintf_w`, `vfprintf_w`.
- **`posix.c`** implements `exec_process` (via `fork`/`execvp`).
- **`port.c`** captures raw C-library `fprintf`/`vfprintf` function
  pointers before `port.h` overrides them (needed for Windows
  error reporting without re-entrant UTF-8 conversion).

**Rule**: `configdep.c`, `membuf.c`, `utils.c` must never contain
`#ifdef _WIN32` or platform-specific includes. All platform differences
go through `port.h` + the platform implementation files.

## Build and test

```bash
make            # build (output in build/)
make test       # run TAP integration tests
make test PFLAGS="-v"  # verbose test output
make clean      # remove build artifacts
```

Cross-compile for Windows:
```bash
make CC=x86_64-w64-mingw32-gcc
```

## Key conventions

- C99, `-Wall -Werror -pedantic`
- `membuf` buffers are not NUL-terminated by default; `membuf_cat` adds
  a NUL for convenience
- Static buffers with `#define` sizes are preferred over dynamic
  allocation in hot paths
- Use C99 designated initializers for struct literals where positional
  fields could be silently swapped (e.g.
  `(membuf_byte_pair){.from = '_', .to = '/'}`, not `{'_', '/'}`)
- Tests use TAP (Test Anything Protocol) via `prove`
- Commits follow Conventional Commits

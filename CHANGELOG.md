## v0.2.0 (2026-03-27)

### ✨ New Features

- **configdep**: create empty .cdep stubs when missing *(Jan Beran - 6712cf3)*
- **configdep**: scan translation unit deps for CONFIG_* references *(Jan Beran - db30f6d)*

### 🐛 Bug Fixes

- satisfy clang-tidy bugprone checks *(Jan Beran - 7236bc4)*

### 📖 Documentation

- add AGENTS.md with repository overview *(Jan Beran - 094d5ba)*


## v0.1.4 (2026-03-05)

### 🐛 Bug Fixes

- query buffer size before conversion in mbs_to_wcs and wcs_to_mbs *(Frantisek Hrbata - d97d00f)*
- use STARTUPINFOW for CreateProcessW call *(Frantisek Hrbata - 8d7b671)*
- remove unimplemented membuf_extend and membuf_dump declarations *(Frantisek Hrbata - 2c2f72a)*
- convert character count to bytes in __mbs_to_wcs membuf_grow *(Frantisek Hrbata - 30c1570)*
- use err() instead of err_raw() in __wcs_to_mbs *(Frantisek Hrbata - 9ecf2f0)*
- use char instead of int for source file data buffer in get_config *(Frantisek Hrbata - c463ebf)*
- use char pointer instead of int for CONFIG_ prefix search *(Frantisek Hrbata - fd3e11e)*
- print GetLastError() instead of rv in MultiByteToWideChar size error *(Frantisek Hrbata - 5ed0c00)*
- correct function name in __wcs_to_mbs error message *(Frantisek Hrbata - 1acaa88)*
- correct swapped format arguments in membuf size error messages *(Frantisek Hrbata - ae02910)*
- correct argv_mb membuf size and grow calculation in wmain *(Frantisek Hrbata - 8b51a32)*
- use args_copy instead of args in vfprintf_w vsnprintf calls *(Frantisek Hrbata - 29a0bf1)*
- remove spurious cmdl argument in GetExitCodeProcess error message *(Frantisek Hrbata - 974fc6b)*
- correct swapped pointers in membuf_rchr() *(Frantisek Hrbata - 55af105)*

### 🔧 Code Refactoring

- extract wmain and wconv into separate translation units *(Frantisek Hrbata - 3352282)*


## v0.1.3 (2026-02-16)

### 🐛 Bug Fixes

- change suffix of the generated file *(Jan Beran - f448069)*

### 📖 Documentation

- add comprehensive Doxygen-style documentation across codebase *(Frantisek Hrbata - fe6fb06)*
- Update README with license, contributing guide, and ESP-IDF links *(copilot-swe-agent[bot] - 850f7bd)*


## v0.1.2 (2026-01-28)

### Bug Fixes

- properly close file and return success when reporting version

## v0.1.1 (2026-01-27)

## v0.1.0 (2026-01-26)

### New Features

- import code from personal repository

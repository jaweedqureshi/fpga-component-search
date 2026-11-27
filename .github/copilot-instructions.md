# Copilot project instructions

## Project purpose

Maintain a cycle-accurate SystemVerilog comparator for 64-byte electronic
component records, simulated with Verilator and checked against a C++ golden
model. Keep the workflow isolated in Docker Compose so it can be used on
macOS with Docker Desktop, including Apple Silicon and amd64 hosts.

## Architecture and behavior

- The record is a 512-bit little-endian packed value:
  - Bits `[15:0]`: 16-bit supplier ID.
  - Bits `[159:16]`: 144-bit flags.
  - Bits `[191:160]`: 32-bit exact SKU hash.
  - Bits `[255:192]`, `[319:256]`, `[383:320]`: three IEEE-754 float64
    parameters, respectively.
  - Bits `[495:384]`: 14-byte MPN.
  - Bits `[511:496]`: 16-bit category.
- Query criteria are separate inputs. Category, SKU, and supplier compare
  exactly. MPN and flags use masked equality:
  `(record & mask) == (query & mask)`.
- Each float parameter must be within its query's inclusive min/max range.
  NaN comparisons do not match.
- `rtl/record_comparator.sv` has two registered pipeline stages, no stalls,
  active-low synchronous reset, and propagates `in_valid` to `out_valid`.
- `sim/tb_phase1.cpp` packs records and checks each output cycle against the
  software golden model, including bubbles and pipeline drain.

## Key files

- `rtl/record_comparator.sv`: RTL module and record/query comparison behavior.
- `sim/tb_phase1.cpp`: packed test vectors, C++ reference model, and cycle
  checks.
- `Makefile`: local build/test/clean targets.
- `Dockerfile`: multi-stage Ubuntu 24.04 image and Verilator toolchain.
- `docker-compose.yml`: isolated project network and persistent named build
  cache; no ports or backing services.
- `.env.example`: Compose project-name setting. Keep `.env` local and ignored.
- `README.md`: setup, test, and cleanup instructions for developers.

## Build, test, and cleanup

Use Docker Compose as the canonical environment:

```sh
cp .env.example .env
docker compose run --build --rm sim
```

The command above builds the image and runs `make test`. For an incremental
run after source-only changes:

```sh
docker compose run --rm sim
```

Inside the container, `make test` builds and runs the testbench. `make build`
only builds; `make clean` removes `build/obj_dir`. Rebuild the image with
`--build` whenever Dockerfile or toolchain requirements change.

Use `docker compose down` to remove this project's containers/network while
retaining its named build-cache volume. `docker compose down -v` also removes
that cache. Do not remove unrelated Docker resources or other projects'
volumes.

## Keeping this handoff current

- Treat this file as the portable Copilot handoff for future work on another
  machine.
- When changing architecture, record layout, comparison semantics, toolchain,
  environment variables, Compose services, or build/test commands, update this
  file and the relevant README sections in the same change.
- Keep commands and behavior aligned with the actual `Makefile`, Dockerfile,
  Compose configuration, RTL, and testbench. Do not preserve stale
  verification claims; rerun the documented container test after changes and
  update the README's recorded result when test coverage or results change.
- Keep the environment project-scoped. Do not add host ports or backing
  services unless the project requirements call for them. Keep generated build
  output in the named volume rather than binding heavy build directories from
  the host.
- Prefer focused tests for changed behavior. Report a blocked Docker
  verification clearly instead of claiming it passed.

The initial scaffold was verified with `docker compose run --build --rm sim`:
the testbench passed with 13 input cycles and zero golden-model mismatches.

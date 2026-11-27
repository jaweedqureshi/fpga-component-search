# Component Record Comparator — Containerized Verilator Workflow

This project simulates a two-stage SystemVerilog component-record comparator
against a C++ golden model using Verilator in an isolated Docker Compose
environment.

## Step 1: Prerequisites and first setup

- Docker Desktop running locally on macOS, or another Docker Engine with Docker
  Compose v2. The Ubuntu base image supports `arm64` and `amd64`.
- On Apple Silicon, use Docker Desktop for Apple Silicon. If Docker reports
  virtualization problems, check that `sysctl kern.hv_support` returns `1`.
- Create a local `.env` file from the example:

  ```sh
  cp .env.example .env
  ```

There are no app ports or backing services. Compose scopes its network and
named build-cache volume to this project.

## Step 2: Build and run the simulation

Use this command after changing `Dockerfile` or `docker-compose.yml`, or when
you want a clean build-and-run pass inside an ephemeral container:

```sh
docker compose run --build --rm sim
```

- `--build` rebuilds the container image if the Dockerfile or build context
  changed.
- `--rm` removes the temporary `sim` container when it finishes.
- The named build-cache volume persists across runs to keep Verilator and C++
  compilation fast.

**Expected result:** Verilator compiles the SystemVerilog RTL and C++
testbench, runs the simulation, and exits with status `0` with zero
mismatches against the golden model.

## Step 3: Run incremental tests without rebuilding the image

When only source files (`rtl/*.sv`, `sim/*.cpp`, or test vectors) change, and
the Dockerfile/toolchain has not changed, skip `--build` for faster iteration:

```sh
docker compose run --rm sim
```

- This reuses the existing container image and persistent build cache.
- It runs the test target from the Makefile inside the isolated project
  network.

**Expected result:** Incremental compilation completes and the testbench
passes with exit status `0`.

## Step 4: Clean up the project network

When you finish a work session, remove this project's containers and Compose
network while keeping the compiled build cache:

```sh
docker compose down
```

To also remove the named build-cache volume and force a clean rebuild next
time:

```sh
docker compose down -v
```

**Expected result:** Project containers and the project-scoped network are
removed cleanly (`docker compose ps` shows no running services).

## Copilot handoff

See [`.github/copilot-instructions.md`](.github/copilot-instructions.md) for
the project architecture, source-of-truth commands, validation expectations,
and instructions to keep this handoff current as the project changes.

Last recorded container verification: **PASS**, 13 input cycles matched the
C++ golden model.

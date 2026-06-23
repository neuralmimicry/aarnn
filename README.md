# AARNN — Autonomic Asynchronous Recursive Neuromorphic Network

## Sponsor NeuralMimicry

AARNN is an open-source research platform for emergent cognition — a tiered, multi-agent, distributed neuromorphic AI with real-time sensory input, PostgreSQL persistence, and interactive 3D visualisation via VTK. NeuralMimicry is an independent open-source initiative and we rely on community support to sustain this work.

**[☕ Support us on Crowdfunder](https://www.crowdfunder.co.uk/p/qr/aWggxwPW?utm_campaign=sharemodal&utm_medium=referral&utm_source=shortlink)**

---

![Build](https://github.com/neuralmimicry/aarnn/actions/workflows/cmake.yml/badge.svg?branch=main)

AARNN (pronounced “Aaron”) is an open-source research platform for emergent cognition built as a tiered, multi‑agent, distributed AI. It focuses on neuromorphic concepts (neurons, clusters, receptors/effectors) and supports real‑time sensory input, PostgreSQL persistence, and interactive 3D visualisation via VTK.

This repository contains:
- Core libraries implementing neuron/cluster models and shared utilities.
- Executables to simulate AARNN, capture/process audio, and visualise the system state.
- Optional web viewer utilities and containerisation assets (Docker/Podman) for reproducible runs.

Project by NeuralMimicry (Paul Isaacs).


## 1. Goals and Scope
AARNN aims to:
- Provide a reproducible environment for experimenting with emergent behaviour from interacting neuron clusters.
- Ingest sensory data (e.g., microphone, streams, files) through a modular auditory pipeline.
- Persist network state to PostgreSQL for analysis and visualisation.
- Offer real‑time 3D visualisation (VTK) and a WebSocket bridge for external tools.

Non‑goals (current):
- A turn‑key production system. This is a research platform intended for experimentation and extension.


## 2. Repository Layout (high level)
- src/aarnn — AARNN core and main simulator (AARNN executable)
- src/audio — Audio capture/ingest and processing (audio_lib and Audio demo)
- src/visualiser — 3D visualiser and WebSocket server (Visualiser executable)
- src/hello-world — Small VTK demos (hello_world, vtk_test)
- include — Public headers for shared components
- configure — Example configuration files (simulation.conf, Visualiser.conf)
- BuildFromGit — Container build files (Docker/Podman) and helpers
- webviewer — Optional web UI/server assets


## 3. Prerequisites
You’ll need a modern Linux environment with the following available (names taken from CMake finds; install via your distro’s package manager):
- Build tools: CMake (≥ 3.31), a C++17 compiler, make/ninja
- Core libs: Threads, OpenSSL, Boost (filesystem, system, json), CURL
- Database: PostgreSQL client libraries (libpq), libpqxx
- Visualisation: OpenGL, GLEW, Freetype, VTK (RenderingCore, RenderingOpenGL2, InteractionStyle, RenderingFreeType, FiltersSources)
- Audio / media: ALSA, PortAudio, PulseAudio (libpulse, libpulse-simple), libsndfile, FFTW3, FFmpeg (libavcodec, libavformat, libavutil, libswscale, libswresample), GStreamer (core, app, video, audio)
- Optional: OpenCV (core, imgproc, highgui), MPI, GMP

Note: Exact package names vary by distribution. Check your distro’s packages for development headers (e.g., libpqxx-dev, libvtk-dev, libfftw3-dev, etc.).


## 4. Building (with CLion or CLI)
This repo already includes CLion CMake profiles and targets. You can use either CLion or the command line.

Available executable targets:
- AARNN
- Visualiser
- Audio (demo)
- hello_world (VTK demo)
- vtk_test (VTK demo)

Available library targets:
- aarnn_core, audio_lib, common

### 4.1. Build using CLion profiles (recommended)
Profiles and build directories:
- Debug → /home/pbisaacs/Developer/aarnn/cmake-build-debug
- Release → /home/pbisaacs/Developer/aarnn/cmake-build-release

In CLion, select a profile (Debug/Release) and build the desired target.

### 4.2. Build from command line
From the repository root:

- Debug build
  cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
  cmake --build cmake-build-debug --target AARNN

- Release build
  cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
  cmake --build cmake-build-release --target AARNN

You may replace AARNN with any target listed above.


## 5. Configuration
Runtime behaviour is primarily controlled via plain‑text key=value config files placed next to the executables or run from the repository root.

- configure/simulation.conf
  Keys used by AARNN (and also read by Visualiser):
  - num_clusters, num_neurons
  - num_pixels, num_phonels, num_scentels, num_vocels
  - neuron_points_per_layer, pixel_points_per_layer, phonel_points_per_layer, scentel_points_per_layer, vocel_points_per_layer
  - proximity_threshold
  - use_database = true|false

- configure/Visualiser.conf
  Keys for database connection and viewer (read by Visualiser):
  - host, port, dbname, user, password … (typical libpq parameters)

Deployment tip: copy these files next to the executables or run from the repo root so that relative paths resolve.


## 6. Database Setup (PostgreSQL)
AARNN and Visualiser can work without a DB, but persistence/visualisation typically need PostgreSQL.

- Local PostgreSQL
  - Create a database (e.g., neurons) and a user with password.
  - Ensure Visualiser.conf contains matching connection parameters.

- Containers (Docker or Podman)
  Example: build and run a Postgres image named postgres-aarnn
  docker build --build-arg POSTGRES_USER=postgres --build-arg POSTGRES_PASSWORD=change_this_password -t postgres-aarnn -f BuildFromGit/docker/Dockerfile.postgres .
  docker run --name postgres-aarnn -p 5432:5432 -e POSTGRES_USER=postgres -e POSTGRES_PASSWORD=change_this_password -e POSTGRES_DB=neurons -d postgres-aarnn

Caution: containerised databases consume memory. Adjust resources as needed.


## 7. Running the executables
Paths below assume CLion’s default build directories. Adjust if you used a different build path.

### 7.1 AARNN — simulator and cluster/neuron dynamics
Purpose: Initialise clusters and neurons per simulation.conf, optionally connect to PostgreSQL, initialise sensory receptor server, and run the main simulation loop.

Requires: configure/simulation.conf (copy next to the binary or run from repo root).

Examples:
- Debug
  cmake --build cmake-build-debug --target AARNN && ./cmake-build-debug/AARNN
- Release
  cmake --build cmake-build-release --target AARNN && ./cmake-build-release/AARNN

Notes:
- If use_database=true but connection fails, AARNN continues without DB (warning printed).
- Audio capture and processing are integrated via audio_lib; microphone/device selection may prompt on first start (PulseAudio).

### 7.2 Visualiser — interactive 3D viewer (VTK) with WebSocket server
Purpose: Connect to PostgreSQL, stream/visualise neurons/clusters, expose a WebSocket server (default 9002) for external clients.

Requires: configure/Visualiser.conf and configure/simulation.conf.

Examples:
- Debug
  cmake --build cmake-build-debug --target Visualiser && ./cmake-build-debug/Visualiser
- Release
  cmake --build cmake-build-release --target Visualiser && ./cmake-build-release/Visualiser

If no valid DB connection parameters are found, the Visualiser exits with an error explaining which config files to check.

### 7.3 Audio (demo)
Purpose: Demonstrates the auditory pipeline by wiring AuditoryManager to AuditoryProcessor and selecting sources (YouTube/RTSP/Microphone or file).

Note: This is a demo harness. API keys/URLs are placeholders in src/audio/AuditorySolution.cpp — edit before running.

Example:
- Debug
  cmake --build cmake-build-debug --target Audio && ./cmake-build-debug/Audio

### 7.4 hello_world and vtk_test (VTK demos)
Simple VTK examples to validate your toolchain.

Examples:
- cmake --build cmake-build-debug --target hello_world && ./cmake-build-debug/hello_world
- cmake --build cmake-build-debug --target vtk_test && ./cmake-build-debug/vtk_test


## 8. Audio and PulseAudio setup
For microphone/speaker stimulation:
- Ensure PulseAudio is running and your user has access.
- On first run you may be prompted to select an audio source. Examples you might see:
  1: alsa_input.pci-... (microphone input)
  2: alsa_output.pci-... .monitor (loopback/monitor of output)
- There is a helper script (if present in your environment) to start PulseAudio: ./setup_pulse_audio.sh


## 9. Web viewer (optional)
The webviewer directory contains a client and server for visualisation via a browser.
- client: webviewer/client (static assets, package.json)
- server: webviewer/server (reads Visualiser.conf)

The Visualiser exposes a WebSocket server on port 9002 which a web client can connect to.


## 10. Automation scripts and containers
- run.sh — orchestrates BuildFromGit/runNNN.sh scripts in order.
- BuildFromGit — Docker/Podman Containerfiles and compose files.

### 10.1 Run the full stack with Docker Compose (recommended for local dev)
This repository includes a multi-service `docker-compose.yml` at the project root that brings up:
- postgres:16 — primary DB (volume: `pgdata`, exposed `${POSTGRES_PORT_EXPOSE:-5432}`)
- redis:7-alpine — optional cache/queue (volume: `redisdata`, exposed `6379`)
- aarnn — C++ core built from `BuildFromGit/docker/Dockerfile.debian-aarnn` and run as `/app/AARNN`
- visualiser — Node WebSocket server `webviewer/server/visualiser.js` on port `9002`
- webviewer — Node static server for the client `webviewer/client` on port `8180`

Prerequisites:
- Docker Engine and Docker Compose v2
- A `.env` file at repo root (one is provided). Important:
  - The Postgres image expects `POSTGRES_USER`. The compose file maps your `POSTGRES_USERNAME` to `POSTGRES_USER`.
  - Treat `.env` as sensitive (contains passwords/tokens). Do not commit real secrets.

Start the stack (first run builds the AARNN image):
```
docker compose up -d --build
```

Access:
- Web client: http://localhost:8180
- Visualiser WebSocket: ws://localhost:9002
- PostgreSQL: localhost:${POSTGRES_PORT_EXPOSE:-5432}
- Redis: localhost:6379

Stop:
```
docker compose down
```

Stop and remove data volumes (destructive):
```
docker compose down -v
```

Notes and caveats:
- WebSocket hostname/CSP: The web client currently allows `ws://visualiser:9002` in its CSP (see `webviewer/client/clientserver.js`). From your host browser, `visualiser` doesn't resolve by default.
  - Easiest: change the client to use `ws://localhost:9002` (and update CSP accordingly), or add a small reverse proxy.
  - Dev-only alternative: add `127.0.0.1 visualiser` to your host `/etc/hosts`.
- Optional audio (PulseAudio) for the AARNN container:
  - Uncomment the PulseAudio lines in the `aarnn` service within `docker-compose.yml` and set `PULSE_*` variables in `.env`.
  - Map `/dev/snd` and Pulse socket/cookie as shown in the compose file comments. Linux permissions may require tweaks.
- Vault and sensory server: `.env` references `VAULT_*` and a sensory server, but those services are not included by default. Add them if required for your workflow.

Example custom networks (optional):
- Docker: `docker network create -d bridge my_net; docker compose up`
- Podman: `podman network create my_net`

Environment variables for container builds and runtime are stored in `.env` (ports/passwords).


## 11. Troubleshooting
- Missing libraries at link time: install the -dev/-devel packages for the components listed in Prerequisites.
- VTK window not opening: check that OpenGL and GPU drivers are installed; try offscreen settings.
- PostgreSQL connection errors: verify Visualiser.conf values and that the DB is reachable from your host/network.
- Audio capture issues: ensure PulseAudio is running and your user has permissions; on servers consider PipeWire/PulseAudio configuration.


## 12. Contributing and License
- Contributions welcome. See CONTRIBUTING.md.
- License: See LICENSE.
- Sponsorship: https://github.com/sponsors/neuralmimicry

Logos and images © NeuralMimicry.

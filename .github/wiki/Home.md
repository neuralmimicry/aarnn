# AARNN — Wiki Home

**AARNN** (Autonomic Asynchronous Recursive Neuromorphic Network, pronounced "Aaron") is an open-source research platform for emergent cognition built as a tiered, multi-agent, distributed AI. It focuses on neuromorphic concepts — neurons, clusters, receptors/effectors — and supports real-time sensory input, PostgreSQL persistence, and interactive 3D visualisation via VTK.

> ☕ [Support NeuralMimicry on Crowdfunder](https://www.crowdfunder.co.uk/p/qr/aWggxwPW?utm_campaign=sharemodal&utm_medium=referral&utm_source=shortlink)

---

## Quick navigation

| Page | Description |
|---|---|
| [Getting Started](Getting-Started) | Build prerequisites, CMake build, first run |
| [Architecture](Architecture) | Component hierarchy: clusters → neurons → somas → synapses |
| [Configuration](Configuration) | `simulation.conf`, `Visualiser.conf`, database setup |
| [Visualiser](Visualiser) | VTK 3D viewer and WebSocket bridge |
| [Container Build](Container-Build) | Docker/Podman build and compose files |
| [Contributing](Contributing) | Build setup, PR guidelines |

---

## Build (CLI)

```bash
# Debug build
cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug --target AARNN

# Release build
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --target AARNN
```

Available targets: `AARNN`, `Visualiser`, `Audio`, `hello_world`, `vtk_test`

## Prerequisites

- CMake ≥ 3.31, C++17 compiler
- Boost (filesystem, system, json), OpenSSL, CURL
- PostgreSQL client libs (libpq, libpqxx)
- VTK (RenderingCore, RenderingOpenGL2, InteractionStyle, RenderingFreeType, FiltersSources)
- Audio: ALSA, PortAudio, PulseAudio, libsndfile, FFTW3, FFmpeg, GStreamer

## Neural component hierarchy

```
clusters → neurons → somas → axon hillocks → axons/branches/boutons
       → synaptic gaps → dendrites/branches/boutons
```

Each component has `energy_level` and `max_energy_level`.

## Related repositories

- [aarnn_rust](https://github.com/neuralmimicry/aarnn_rust) — full neuromorphic autonomous AI platform in Rust
- [neuromorphic_demo](https://github.com/neuralmimicry/neuromorphic_demo) — interactive Rust SNN demo
- [aarnn-nsys](https://github.com/neuralmimicry/aarnn-nsys) — ultra-low-latency message bus for neuromorphic systems

## Get involved

- 🐛 [Report a bug or request a feature](https://github.com/neuralmimicry/aarnn/issues)
- 💬 [Join the discussion](https://github.com/neuralmimicry/aarnn/discussions)
- 📧 Direct support: [info@neuralmimicry.ai](mailto:info@neuralmimicry.ai) · **£1,000/day + VAT**
- 🌐 [neuralmimicry.ai](https://neuralmimicry.ai)

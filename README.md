<p align="center">
  <img src="Assets/banner.svg" alt="AnonymousVoice banner" width="100%" />
</p>

<h1 align="center">AnonymousVoice</h1>

<p align="center">
  A Windows-first JUCE/C++ voice changer focused on low-latency real-time processing, custom voice presets, and future virtual-microphone routing for Xbox Game Bar party chat and other voice applications.
</p>

<p align="center">
  <img alt="C++20" src="https://img.shields.io/badge/C%2B%2B-20-7c5cff?style=flat-square" />
  <img alt="JUCE" src="https://img.shields.io/badge/JUCE-audio-35d7ff?style=flat-square" />
  <img alt="Platform" src="https://img.shields.io/badge/platform-Windows-7c5cff?style=flat-square" />
  <img alt="Status" src="https://img.shields.io/badge/status-active%20development-f0a020?style=flat-square" />
</p>

## Overview

AnonymousVoice is an early-stage desktop voice changer built with JUCE and C++20. The project is being developed around a clean dark/neon interface, a low-latency microphone processing path, modular DSP effects, and a future Windows virtual microphone endpoint.

The intended end-to-end path is:

```text
Physical Microphone
        ↓
AnonymousVoice
        ↓
Real-time DSP
        ↓
Anonymous Virtual Microphone   [planned]
        ↓
Xbox Game Bar / Xbox Party / Discord / other voice apps
```

## Current status

### Working / in repository

- JUCE desktop application shell
- Dark purple/teal voice-changer UI
- Microphone input through `AudioAppComponent`
- Basic real-time audio passthrough and gain path
- Input/output level metering
- CPU and latency status UI
- Voice preset and effect-control interface scaffolding
- CMake + Visual Studio 2022 build configuration

### In development

- Production-quality pitch shifting
- Formant shifting
- Noise gate
- EQ
- Compressor
- Distortion
- Chorus
- Reverb
- Limiter
- Preset persistence
- Device selection and routing
- Low-latency monitoring controls

### Planned

- `Anonymous Virtual Microphone` Windows endpoint
- Xbox Game Bar / Xbox Party Chat routing
- Driver installation and diagnostics
- Per-app compatibility testing
- Soundboard
- Keybind system
- Preset import/export

> **Important:** the virtual microphone is a project target and is not implemented in the current codebase yet.

## Anonymous preset target

The initial signature preset is designed around this processing profile:

| Stage | Initial target |
|---|---:|
| Pitch | -5 semitones |
| Formant | -3 |
| EQ low | +4 dB |
| EQ mid | +1 dB |
| EQ high | -5 dB |
| Distortion | ~8% |
| Chorus | subtle |
| Reverb | subtle |
| Limiter | final protection |

These are design targets, not final tuned values.

## Build

### Requirements

- Windows 10/11
- Visual Studio 2022 with Desktop development with C++
- CMake 3.22+
- JUCE checked out at `C:\Dev\JUCE`

Expected layout:

```text
C:\Dev\
├── JUCE\
└── AnonymousVoice\
```

### Developer Command Prompt

```bat
cd /d C:\Dev\AnonymousVoice
cmake -S . -B build -A x64
cmake --build build --config Debug -- /m:1
```

The single-worker MSBuild setting is intentional for lower memory pressure on the current development machine.

## Repository structure

```text
AnonymousVoice/
├── Assets/
│   └── banner.svg
├── docs/
│   └── ARCHITECTURE.md
├── Source/
│   ├── Main.cpp
│   ├── MainComponent.cpp
│   └── MainComponent.h
├── .gitignore
├── CMakeLists.txt
├── README.md
└── ROADMAP.md
```

## Architecture

AnonymousVoice is being separated into four logical layers:

```text
UI / Presets
     ↓
Audio Engine
     ↓
DSP Chain
     ↓
Output Routing / Virtual Mic
```

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the engineering plan.

## Xbox / Game Bar goal

The target workflow on Windows is for users to select a device named:

```text
Anonymous Virtual Microphone
```

inside Xbox Game Bar party audio settings. AnonymousVoice would capture the physical microphone, process it in real time, and feed the resulting PCM stream into that virtual endpoint.

The virtual audio device is a separate engineering milestone from the JUCE GUI/DSP application and will require a proper Windows audio-device/driver strategy.

## Roadmap

See [ROADMAP.md](ROADMAP.md).

## Development principles

- Keep audio callbacks allocation-free.
- Keep UI work off the real-time audio thread.
- Prefer modular DSP components over one large processing function.
- Keep monitoring and virtual-mic routing independently controllable.
- Never allow processed output to feed back into the physical input path.
- Optimize for stable low latency before adding cosmetic effects.

---

<p align="center"><strong>Powerful. Private. Anonymous.</strong></p>

<p align="center">
  <img src="Assets/banner.svg" alt="AnonymousVoice banner" width="100%" />
</p>

<h1 align="center">AnonymousVoice</h1>

<p align="center">
  A Windows-first C++ voice changer project focused on an anonymous dark/neon interface, low-latency voice processing, custom presets, and future virtual-microphone routing for Xbox Game Bar party chat, Discord, and other voice applications.
</p>

<p align="center">
  <img alt="C++20" src="https://img.shields.io/badge/C%2B%2B-20-7c5cff?style=flat-square" />
  <img alt="UI" src="https://img.shields.io/badge/UI-Win32-35d7ff?style=flat-square" />
  <img alt="Platform" src="https://img.shields.io/badge/platform-Windows-7c5cff?style=flat-square" />
  <img alt="Status" src="https://img.shields.io/badge/status-active%20development-f0a020?style=flat-square" />
</p>

## Overview

AnonymousVoice is an early-stage Windows desktop voice changer written in C++20.

The current development branch uses a lightweight native Win32 application shell so the project can build reliably on the present development machine. Earlier JUCE audio/DSP work is still part of the repository history and engineering direction, but JUCE is temporarily excluded from the active executable while the Windows-native shell and low-memory build path are stabilized.

The intended end-to-end product path remains:

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

### Working now

- Native Windows desktop executable
- Dark purple/teal AnonymousVoice dashboard
- Voice preset card interface
- Selected-preset details panel
- Pitch / Formant / Mix control scaffolding
- Voice-editor effect-chain scaffolding
- Input / monitor / virtual-output device strip
- CMake + Visual Studio 2022 build configuration
- Low-memory MSVC build configuration
- Automatic copy of the finished executable to the repository root for easy launching

### Temporarily isolated

The following systems are intentionally excluded from the active executable while the low-memory Windows build path is stabilized:

- JUCE GUI modules
- JUCE audio-device layer
- Real-time DSP engine integration
- Audio passthrough and monitoring
- Level metering and live CPU/latency telemetry

These are engineering components to reintroduce in smaller, testable pieces rather than claims about the current executable.

### In development

- Real Windows microphone input/output using a lightweight Windows audio path
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
- Real device selection and routing
- Low-latency monitoring controls

### Planned

- `Anonymous Virtual Microphone` Windows capture endpoint
- Xbox Game Bar / Xbox Party Chat routing
- Driver installation and diagnostics
- Per-app compatibility testing
- Discord integration / Rich Presence
- Game-aware profile switching
- Soundboard
- Keybind system
- Preset import/export

> **Important:** `Anonymous Virtual Microphone` is a project target and is not implemented as a Windows audio endpoint yet.

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
- Visual Studio 2022 with **Desktop development with C++**
- CMake 3.22+

The active Win32 shell does **not** currently require JUCE to compile.

### Developer Command Prompt / Developer PowerShell

```bat
cd /d C:\Dev\AnonymousVoice
cmake -S . -B build -A x64
cmake --build build --config Debug -- /m:1
```

The single-worker MSBuild setting is intentional for lower memory pressure on the current development machine.

After a successful build, CMake copies the executable here:

```text
C:\Dev\AnonymousVoice\Anonymous Voice.exe
```

That file can be launched directly by double-clicking it in File Explorer.

> If the linker reports `LNK1168`, close Anonymous Voice first (or terminate `Anonymous Voice.exe`) and rebuild so Windows releases the executable file.

## Active build structure

```text
AnonymousVoice/
├── Assets/
│   └── banner.svg
├── docs/
│   └── ARCHITECTURE.md
├── Source/
│   ├── Main.cpp                  # active native Win32 application
│   ├── MainComponent.cpp         # earlier JUCE UI work, currently excluded
│   ├── MainComponent.h           # earlier JUCE UI work, currently excluded
│   └── Audio/                    # audio/DSP foundation, currently excluded
├── CMakeLists.txt
├── README.md
├── ROADMAP.md
└── Anonymous Voice.exe           # generated after a successful build
```

Generated build files remain under `build/` and should not be committed.

## Architecture direction

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

The current Win32 UI shell is intentionally independent from the audio engine so the interface and Windows audio path can be stabilized separately.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the broader engineering plan.

## Xbox / Game Bar goal

The target workflow on Windows is for users to select a device named:

```text
Anonymous Virtual Microphone
```

inside Xbox Game Bar party audio settings. AnonymousVoice would capture the physical microphone, process it in real time, and feed the resulting PCM stream into that Windows capture endpoint.

A real virtual microphone requires a Windows audio-device/driver strategy and is a separate engineering milestone from the desktop UI.

## Development principles

- Keep real-time audio callbacks allocation-free.
- Keep UI work off the real-time audio thread.
- Prefer modular DSP components over one large processing function.
- Keep monitoring and virtual-mic routing independently controllable.
- Never allow processed output to feed back into the physical input path.
- Optimize for stable low latency before adding cosmetic effects.
- Keep the Windows build viable on constrained development hardware.

## Roadmap

See [ROADMAP.md](ROADMAP.md).

---

<p align="center"><strong>Powerful. Private. Anonymous.</strong></p>

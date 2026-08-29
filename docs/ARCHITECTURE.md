# AnonymousVoice Architecture

## Goals

AnonymousVoice is a Windows-first real-time voice processing application. The architecture must prioritize predictable latency, real-time safety, clean routing, and a future Windows virtual microphone endpoint suitable for Xbox Game Bar party chat and other voice applications.

## High-level flow

```text
Physical Mic
    ↓
Audio Input
    ↓
DSP Graph
    ↓──────────────→ Local Monitor
    ↓
Virtual Mic Bridge [planned]
    ↓
Windows Capture Endpoint [planned]
    ↓
Xbox Game Bar / Xbox Party / Discord / etc.
```

## Layer 1 — UI and preset system

Responsibilities:

- Voice preset selection
- DSP parameter editing
- Input/output device selection
- Monitoring controls
- CPU/latency/meter presentation
- Preset save/load
- Keybinds and soundboard integration later

The UI must never perform work inside the real-time audio callback.

## Layer 2 — Audio engine

Responsibilities:

- Open and manage JUCE audio devices
- Capture the physical microphone
- Maintain sample rate and block-size state
- Feed blocks through the DSP graph
- Route a separately controlled signal to the monitoring output
- Export processed PCM to the future virtual-mic bridge

Real-time rules:

- No heap allocation in the audio callback
- No file I/O in the audio callback
- No locks that can block unpredictably
- No GUI calls from the audio callback
- Preallocate working buffers during `prepareToPlay`

## Layer 3 — DSP graph

Initial intended order:

```text
Gate
 ↓
Pitch
 ↓
Formant
 ↓
EQ
 ↓
Compressor
 ↓
Distortion
 ↓
Chorus
 ↓
Reverb
 ↓
Limiter
```

The chain should eventually support per-effect bypass and parameter smoothing.

### Anonymous preset target

```text
Pitch:      -5 semitones
Formant:    -3
EQ low:     +4 dB
EQ mid:     +1 dB
EQ high:    -5 dB
Distortion: ~8%
Chorus:     subtle
Reverb:     subtle
Limiter:    final protection
```

Pitch/formant processing requires special attention. JUCE supplies useful DSP infrastructure but does not by itself provide the final production-quality voice transformation strategy. The project should evaluate a suitable low-latency pitch/formant implementation before locking the DSP dependency set.

## Layer 4 — Output routing

Two output paths are required:

### Monitor path

Processed audio can optionally be sent to headphones for local monitoring. Monitoring must be independently switchable and must not feed back into the capture source.

### Virtual microphone path

The long-term Windows path is:

```text
AnonymousVoice process
    ↓
IPC / shared audio transport
    ↓
Windows virtual audio component
    ↓
Anonymous Virtual Microphone
```

The virtual microphone is not merely a JUCE ComboBox entry. Windows must expose a genuine capture endpoint that applications such as Xbox Game Bar can select as a microphone device.

## Threading model

### Message thread

- UI layout and painting
- Labels and status strings
- Preset manipulation
- Device menus

### Audio thread

- Input capture
- DSP
- Meter measurements
- Monitor/virtual output buffer production

### Background/control thread later

- Preset file I/O
- Driver/endpoint diagnostics
- Update checks
- Logging

## State synchronization

Real-time parameters should use atomics or JUCE parameter/state abstractions appropriate for lock-free access. UI-visible CPU, meter, and latency values should be sampled into atomic state and rendered by the timer/message thread.

## Xbox Game Bar compatibility target

The product goal is for Windows to expose:

```text
Anonymous Virtual Microphone
```

and for a user to select that device in Xbox Game Bar / Xbox Party Chat microphone settings.

Compatibility work will include:

- 48 kHz operation
- Stable endpoint enumeration
- Device reconnects
- Buffer underrun/overrun handling
- Echo/feedback prevention
- Privacy permissions
- Installation/repair/uninstallation of the virtual audio component

## Current repository state

The repository currently contains the JUCE GUI/audio application foundation and a basic passthrough path. Advanced DSP and the real Windows virtual capture endpoint remain future milestones.

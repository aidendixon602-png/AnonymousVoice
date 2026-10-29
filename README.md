<p align="center">
  <img src="Assets/banner.svg" alt="AnonymousVoice banner" width="100%" />
</p>

<h1 align="center">AnonymousVoice</h1>

<p align="center">
  A Windows-first JUCE/C++ voice changer focused on low-latency real-time processing, custom voice presets, and virtual microphone support for Xbox Game Bar party chat and other voice applications.
</p>

<p align="center">
  <img alt="C++20" src="https://img.shields.io/badge/C%2B%2B-20-7c5cff?style=flat-square" />
  <img alt="JUCE" src="https://img.shields.io/badge/JUCE-7.0.8-35d7ff?style=flat-square" />
  <img alt="Platform" src="https://img.shields.io/badge/platform-Windows-7c5cff?style=flat-square" />
  <img alt="License" src="https://img.shields.io/badge/license-MIT-green?style=flat-square" />
  <img alt="Status" src="https://img.shields.io/badge/status-active%20development-f0a020?style=flat-square" />
</p>

## Overview

AnonymousVoice is a desktop voice changer built with JUCE and C++20. The project features a clean dark/neon interface, a low-latency microphone processing path, modular DSP effects, and a Windows virtual microphone endpoint.

The intended end-to-end path is:

```text
Physical Microphone
        ↓
AnonymousVoice (DSP Processing)
        ↓
Anonymous Virtual Microphone
        ↓
Xbox Game Bar / Xbox Party / Discord / other voice apps
```

## Features

### ✅ Implemented

- **JUCE desktop application** with modern dark UI
- **Real-time audio processing** with low latency
- **Complete DSP chain**:
  - Noise Gate
  - Pitch Shifting (-12 to +12 semitones)
  - Formant Shifting
  - 3-band EQ (Low, Mid, High)
  - Compressor
  - Distortion/Saturation
  - Chorus
  - Reverb
  - Limiter
- **Wet/Dry mix control** for all effects
- **Input/output level metering**
- **CPU and latency monitoring**
- **Voice preset system** with 9 built-in presets:
  - Anonymous (signature preset)
  - Deep
  - Demon
  - Robot
  - Alien
  - Ethereal
  - Baby
  - Radio
  - Witch
  - Custom
- **Windows WASAPI virtual microphone** support
- **Thread-safe parameter updates** (UI ↔ Audio thread)
- **CMake build system** with automatic JUCE download
- **Device selection** for input/output

### 🎯 In Development

- Production-quality pitch shifting (currently basic implementation)
- Formant shifting improvements
- Preset save/load to files
- Soundboard integration
- Keybind system
- Xbox Game Bar compatibility testing

### 📋 Planned

- Advanced pitch/formant algorithms
- Per-effect bypass toggles in UI
- Parameter smoothing for click-free changes
- Custom preset creation and management
- Preset import/export
- Per-application audio routing
- Driver installation package
- macOS and Linux support

## Architecture

The application is structured into four logical layers:

```text
UI / Presets Layer
     ↓
Audio Engine Layer
     ↓
DSP Processing Chain
     ↓
Output Routing / Virtual Mic
```

### Threading Model

- **Message Thread**: UI updates, parameter changes
- **Audio Thread**: Real-time audio processing, DSP
- **Background Threads**: Device enumeration, file I/O (future)

### Real-Time Safety

- No heap allocation in audio callbacks
- All shared parameters use `std::atomic` for lock-free access
- UI updates are sampled via timer callback
- Pre-allocated working buffers

## Build

### Requirements

- **Windows 10/11** (primary target)
- **CMake 3.22+**
- **C++20 compatible compiler** (MSVC 2022 recommended)
- **Git** (for JUCE submodule download)
- **Internet connection** (for first build to download JUCE)

### Quick Start

```bash
# Clone the repository
git clone https://github.com/aidendixon602-png/AnonymousVoice.git
cd AnonymousVoice

# Create build directory
mkdir build
cd build

# Configure and build (Debug)
cmake -S .. -B . -A x64
cmake --build . --config Debug -- /m:1

# Run the application
./Debug/AnonymousVoice.exe
```

### Build Options

```bash
# Release build (optimized)
cmake --build . --config Release -- /m:1

# Clean build
cmake --build . --clean-first --config Debug

# Verbose build (for debugging)
cmake --build . --config Debug --verbose
```

### Visual Studio

1. Open the `build` folder in Visual Studio
2. Select the `AnonymousVoice.sln` solution
3. Build the `AnonymousVoice` project
4. Run (F5)

### Troubleshooting

**CMake can't find JUCE**: The build system will automatically download JUCE 7.0.8 on first build. Ensure you have an internet connection.

**Audio device not found**: Check that your microphone and speakers are connected and working in Windows Sound Settings.

**No sound**: Ensure the monitor toggle is enabled and the volume is up.

**Virtual microphone not appearing**: The WASAPI loopback device may take a few seconds to initialize. Restart the application if needed.

## Usage

### Quick Start

1. **Select your microphone** from the "Audio Input" dropdown
2. **Enable Live Monitor** to hear your processed voice
3. **Select a preset** from the voice cards
4. **Adjust parameters** using the sliders in the Voice Editor
5. **Select "Anonymous Virtual Microphone"** as your input device in Xbox Game Bar or other apps

### Presets

| Preset | Description | Pitch | Formant |
|--------|-------------|-------|---------|
| Anonymous | Signature preset, hard to recognize | -5 st | -3 |
| Deep | Resonant, deep voice | -8 st | -2 |
| Demon | Dark, growling voice | -7 st | -4 |
| Robot | Mechanical, synthetic | 0 st | -6 |
| Alien | Otherworldly, strange | +3 st | +5 |
| Ethereal | Light, airy | +4 st | +3 |
| Baby | High-pitched, child-like | +7 st | +4 |
| Radio | AM radio style | -2 st | -1 |
| Witch | Mystical, enchanting | -3 st | -5 |

### Controls

- **Voicebox**: Browse and select voice presets
- **Voice Editor**: Fine-tune DSP parameters
- **Live Monitor**: Toggle to hear your processed voice
- **Monitor Volume**: Adjust the volume of the monitor output
- **Wet/Dry Mix**: Control how much processed vs. original signal

### DSP Chain Order

The audio processing order is:

1. **Gate** - Remove background noise when below threshold
2. **Pitch Shift** - Change the pitch of your voice
3. **Formant Shift** - Change the character/timbre of your voice
4. **EQ** - Adjust frequency balance (Low, Mid, High)
5. **Compressor** - Even out volume dynamics
6. **Distortion** - Add grit and aggression
7. **Chorus** - Thicken the voice
8. **Reverb** - Add space and depth
9. **Limiter** - Prevent clipping

## Xbox Game Bar Integration

### Setup

1. Launch **AnonymousVoice**
2. Ensure "Anonymous Virtual Microphone" appears in the output device list
3. Open **Xbox Game Bar** (Win + G)
4. Go to Settings → Audio
5. Select "Anonymous Virtual Microphone" as your microphone input
6. Join a party chat - your voice will be processed!

### Troubleshooting

- **Virtual mic not appearing**: Restart AnonymousVoice and wait 5-10 seconds
- **No audio in party**: Check that the application has microphone permissions in Windows Settings
- **Echo/feedback**: Disable "Listen to this device" in Windows Sound Settings for your physical microphone
- **Latency issues**: Reduce buffer size in audio settings (future feature)

## Project Structure

```text
AnonymousVoice/
├── Assets/                    # Images, icons, etc.
│   └── banner.svg
├── Source/
│   ├── Main.cpp              # Application entry point
│   ├── MainComponent.cpp     # Main UI and audio component
│   ├── MainComponent.h
│   └── Audio/
│       ├── LightweightAudioAppComponent.cpp  # Audio I/O management
│       ├── LightweightAudioAppComponent.h
│       ├── DspEngine.cpp      # DSP processing chain
│       ├── DspEngine.h
│       ├── VirtualMicOutput.cpp  # Virtual microphone output
│       └── VirtualMicOutput.h
├── CMakeLists.txt             # CMake build configuration
├── README.md
├── ROADMAP.md
└── docs/
    └── ARCHITECTURE.md         # Architecture documentation
```

## Development

### Code Style

- C++20 standard
- JUCE coding conventions
- Atomic parameters for thread safety
- No dynamic allocation in audio callbacks
- Clear separation of UI and audio processing

### Adding New DSP Effects

1. Add parameter to `DspEngine::Parameters` struct
2. Add processing method to `DspEngine` class
3. Call method in `DspEngine::process()` in the correct order
4. Add UI controls in `MainComponent`
5. Connect controls to parameters via callbacks

### Testing

- Test with different sample rates (44.1kHz, 48kHz)
- Test with different buffer sizes
- Verify no clicks or pops when changing parameters
- Check CPU usage with all effects enabled
- Test virtual microphone with various apps

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

### Areas for Contribution

- Improved pitch/formant shifting algorithms
- macOS/Linux support
- Additional DSP effects
- Preset management system
- UI improvements
- Xbox-specific optimizations

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [JUCE Framework](https://juce.com/) - Cross-platform audio application framework
- [Windows WASAPI](https://docs.microsoft.com/en-us/windows/win32/coreaudio/wasapi) - Windows audio API
- All contributors and testers

---

<p align="center"><strong>Powerful. Private. Anonymous.</strong></p>

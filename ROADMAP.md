# AnonymousVoice Roadmap

## Milestone 0 — Repository foundation

- [x] Create GitHub repository
- [x] Add CMake/JUCE project
- [x] Add main desktop application shell
- [x] Add repository ignore rules
- [x] Add project documentation and visual identity
- [ ] Add automated Windows CI build

## Milestone 1 — Stable local audio engine

- [x] Open one input and two output channels
- [x] Basic microphone passthrough
- [x] Input/output metering
- [x] CPU status
- [ ] Robust input/output device enumeration
- [ ] Device-change handling
- [ ] Monitoring on/off without restarting the app
- [ ] Sample-rate/buffer-size controls
- [ ] Store latency state outside the real-time callback
- [ ] Add clipping protection

## Milestone 2 — Anonymous DSP chain

- [ ] Noise gate
- [ ] Pitch shifter
- [ ] Formant shifter
- [ ] 3-band/parametric EQ
- [ ] Compressor
- [ ] Distortion / saturation
- [ ] Chorus
- [ ] Reverb
- [ ] Limiter
- [ ] Wet/dry mix
- [ ] Bypass per module
- [ ] No allocations in the audio callback

## Milestone 3 — Presets and UX

- [ ] Anonymous preset
- [ ] Deep preset
- [ ] Demon preset
- [ ] Robot preset
- [ ] Alien preset
- [ ] Ethereal preset
- [ ] Baby preset
- [ ] Radio preset
- [ ] Witch preset
- [ ] Custom user presets
- [ ] Save/load preset files
- [ ] Search and filtering
- [ ] Keybinds
- [ ] Soundboard foundation

## Milestone 4 — Windows virtual microphone

- [ ] Choose final Windows virtual-audio architecture
- [ ] Build prototype virtual capture endpoint
- [ ] Stream processed PCM from app to endpoint
- [ ] Expose `Anonymous Virtual Microphone` to Windows
- [ ] Installer / driver deployment path
- [ ] Device health diagnostics
- [ ] Safe uninstall / repair path

## Milestone 5 — Xbox Game Bar compatibility

- [ ] Detect virtual mic in Windows Sound settings
- [ ] Select virtual mic in Xbox Game Bar
- [ ] Validate Xbox Party Chat send path
- [ ] Validate monitoring without echo
- [ ] Test reconnects and device changes
- [ ] Test 48 kHz voice-chat operation
- [ ] Document troubleshooting workflow

## Milestone 6 — Release readiness

- [ ] Release build pipeline
- [ ] Signed binaries
- [ ] Installer
- [ ] Crash logging
- [ ] Settings migration
- [ ] Performance tests
- [ ] First alpha release

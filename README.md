<p align="center">
  <img src="https://i.imgur.com/noVfUG6.png">
  <br />
  A reboot of the original <a href="https://github.com/kode54/BASSMIDI-Driver">BASSMIDI Driver by Kode54</a>, with more features.
</p>

## OmniMIDI Mod

This is a modified build of [OmniMIDI v14.8.5](https://github.com/KeppySoftware/OmniMIDI) that adds **KDMAPI multi-port 128-channel support** (8 ports × 16ch) and **extended debug info** with 128-channel synthesizer metrics.

### What's Changed

- **Bug fix:** `DriverSettingsCase` macro ([Issue #274](https://github.com/KeppySoftware/OmniMIDI/issues/274))
- **DriverSettings GET:** `DriverSettings(OM_GET)` no longer requires `OM_MANAGE` — read-only queries work without side effects
- **128ch support:** BASSMIDI stream unconditionally initialized with 128 channels
- **New APIs** for multi-port output:

| Function | Description |
|---|---|
| `SendDirectDataMultiPort(DWORD dwMsg, BYTE port)` | Send short message to a specific port (0–7) |
| `SendDirectLongDataMultiPort(LPSTR data, DWORD len, BYTE port)` | Send SysEx to a specific port |
| `ResetKDMAPIStreamMultiPort(BYTE port)` | Reset 16 channels for a specific port |

- **New API** for extended debug info:

| Function | Description |
|---|---|
| `GetModExtendedDebugInfo()` | Returns a pointer to `ExtendedDebugInfo` struct with 128ch metrics |

The `ExtendedDebugInfo` struct is a superset of the original `DebugInfo`, covering all 128 channels:

| Field | Type | Description |
|---|---|---|
| `StructSize` | DWORD | `sizeof(ExtendedDebugInfo)` — for forward-compatible field availability checks |
| `ModVersionMajor/Minor/Patch` | DWORD | Mod version number |
| `ModVersionDate` | DWORD | Mod version date (YYYYMMDD) |
| `CpuUsage` | FLOAT | BASS audio rendering CPU load (%) — **renamed from `RenderingTime`** |
| `AudioLatency` | DOUBLE | Audio output latency (ms) |
| `AudioBufferSize` | DWORD | Buffer size (frames) |
| `ASIOInputLatency` | DOUBLE | ASIO input latency |
| `ASIOOutputLatency` | DOUBLE | ASIO output latency |
| `CurrentSFList` | DWORD | Current SoundFont list index |
| `ActiveVoicesEx[128]` | DWORD[128] | Per-channel active voices (all 8 ports) |
| `TotalActiveVoices` | DWORD | All-channel total active voices |
| `MaxVoices` | DWORD | Voice limit setting |
| `ActiveNotesEx[128]` | DWORD[128] | Per-channel active notes (all 8 ports) |
| `NumChannels` | DWORD | Stream channel count (128 for Mod) |
| `AudioFrequency` | DWORD | Sample rate (e.g. 48000) |
| `CurrentEngine` | DWORD | Audio engine (WASAPI/ASIO/XAudio) |
| `BufferLength` | DWORD | Buffer length (ms) |
| `OutputVolume` | DWORD | Output volume (0-10000) |
| `AudioBitDepth` | DWORD | Bit depth (0 = float) |
| `SincInter` | BOOL | Sinc interpolation ON/OFF |

The struct is updated every 50ms by OmniMIDI's internal supervisor loop. The original `GetDriverDebugInfo()` and its `DebugInfo` struct remain unchanged for upstream compatibility.

### Usage

#### Multi-port output

1. Place `OmniMIDI.dll` in the same directory as your MIDI application's exe
2. Detect the Mod: `GetProcAddress("SendDirectDataMultiPort")` returns non-NULL on Mod, NULL on original
3. Use `SendDirectDataMultiPort(dwMsg, port)` for port-routed output
4. Falls back to standard `SendDirectData(dwMsg)` (16ch) on original OmniMIDI

#### Extended debug info

1. Detect the Mod: `GetProcAddress("GetModExtendedDebugInfo")` returns non-NULL on Mod
2. On Mod: call `GetModExtendedDebugInfo()` to get a pointer to `ExtendedDebugInfo` (128ch, includes all `DebugInfo` fields)
3. On original: fall back to `GetProcAddress("GetDriverDebugInfo")` for 16ch-only `DebugInfo`
4. Use `StructSize` to check field availability for forward compatibility (see table below)

#### StructSize compatibility

The struct may grow in future releases by appending new fields. Check `StructSize` before reading fields added after the initial release:

| Mod version | StructSize | Last field | Notes |
|---|---|---|---|
| 2026-08-25 | 1096 | `NumChannels` (offset 1092) | Initial release |
| 2026-08-28 | 1120 | `SincInter` (offset 1116) | Added audio settings fields. **Breaking:** `ExtendedDebugInfo.RenderingTime` renamed to `CpuUsage` (same type/offset, source-level change only). `DebugInfo.RenderingTime` is unchanged |

```cpp
ExtendedDebugInfo* info = fnGetModExtendedDebugInfo();

// Fields up to NumChannels are always present (initial release)
DWORD voices = info->TotalActiveVoices;

// Future fields: check StructSize before reading
// if (info->StructSize >= offsetof(ExtendedDebugInfo, NewField) + sizeof(DWORD)) {
//     DWORD val = info->NewField;
// }
```

**Requires:** OmniMIDI installed on the system (BASS DLLs are required). The OmniMIDI installer places both x64 and x86 BASS DLLs in the appropriate system directories.

### ⚠ Volume Warning

**Extended channels (16+) play at full MIDI volume by default.** Excessively loud output may damage audio equipment or cause hearing injury. Lower the master volume in OmniMIDI Configurator before playing multi-port MIDI files.

- **Configurator master volume (OutputVolume):** Controls the entire BASSMIDI stream — applies to all 128 channels. Use this to adjust overall volume.
- **Mixer per-channel faders:** Only affect channels 0–15. Have no effect on channels 16+.
- **Mixer "All" fader:** Bulk control for the per-channel faders (ch 0–15 only) — this is NOT a master volume and does not affect channels 16+.
- **LoudMax limiter (optional):** OmniMIDI supports [LoudMax](https://loudmax.blogspot.com/) as a VST limiter. Install via OmniMIDI Configurator (`Extensions` → `LoudMax, anti-clipping solution` → `Install LoudMax`). The Configurator extracts both 32-bit and 64-bit DLLs to `%USERPROFILE%\OmniMIDI\LoudMax\`, and OmniMIDI automatically loads the appropriate one onto the BASSMIDI stream, limiting output across all 128 channels. This provides an additional safety net against excessively loud output.

### Technical Notes

**Processing path:** The MultiPort APIs bypass OmniMIDI's internal ring buffer and settings processing (`PrepareForBASSMIDI`), sending events directly to BASSMIDI. This means OmniMIDI Configurator settings such as FullVelocity, Transpose, and NoteLength Override do not apply to messages sent via `SendDirectDataMultiPort`. Applications that use their own MIDI processing pipeline should use the MultiPort APIs directly.

**Message types:** All standard MIDI message types (Note On/Off, CC, Program Change, Pitch Bend, etc.) are handled transparently by `SendDirectDataMultiPort`. The caller does not need to handle any message type differently — the function internally routes each type to BASSMIDI using the appropriate mechanism for the extended channel.

**SysEx port routing:** `SendDirectLongDataMultiPort` prepends a BASSMIDI port prefix meta event (`0xFF 0x21 0x01 port`) before the SysEx data, so GS part-specific SysEx (e.g., Drum Part Change, Part Volume) is correctly applied to the target port's channels.

### Known Limitations

MIDI playback on extended channels (ch 16+) works correctly — Note On/Off, CC, Program Change, Pitch Bend, and SysEx are all routed through BASSMIDI's native multi-port support. SoundFonts apply to all 128 channels.

The following OmniMIDI Configurator features are **limited to channels 0–15** and do not apply to extended channels:

- Per-channel mixer level override (`cvalues[16]`)
- Per-channel instrument/bank override (`cbank[16]`, `cpreset[16]`)
- Per-channel pitch shift (`pitchshiftchan[16]`)
- Mixer window active voice display (`ActiveVoices[16]`)

These are Configurator-specific overrides. Standard MIDI messages (CC#7 Volume, Program Change, etc.) sent via `SendDirectDataMultiPort` work correctly on all channels through BASSMIDI.

### Downloads

See [Releases](https://github.com/yossiepon/OmniMIDIMod/releases) for the latest build.

---

## Original OmniMIDI F.A.Q.

### Was it really necessary to create a complete separate fork of BASSMIDI Driver?
I feel like it was necessary, yes.

### Couldn't you just edit the driver on the existing repository?
True that, I could've just done that. But I honestly didn't want to ruin the original driver.<br />
The driver was born back in 2015, when a friend of mine wanted a version of BASSMIDI Driver with higher polyphony, but then I started working on it more and more, to the point where most of the original source code got replaced by mine.<br />
I really didn't want to ruin kode54's original source code, so I decided to create my own repository. (While still giving credits to kode54, of course.)<br />
Oh, and of course, the driver wouldn't be where it is now, without kode54's help from behind the scenes. He helped me a lot with some issues I was having with some parts of his code. (Which I eventually replaced, but still.)<br />

### Do you feel like your driver is complete now?
Tough question... I honestly have no idea. I mean, there's always room for improvement.<br/>
But I feel like I have nothing else to add to it at this point, I'm literally out of ideas.<br/>
If you're a programmer, and you have some ideas on how to improve or expand the driver's functionalities, please hit me up or send a pull requests with the edits.

### Didn't you stop updating it?
Yes, but I still do small updates from time to time when needed, and I also do updates on request.<br/>
I've received numerous donations from people that don't want the driver to be abandoned, and I'm really thankful to all of them for their support!

### Ok ok, enough of your story... What's so special about your driver that makes it different from the others out there?
Good question. The driver has unique features, such as:
- Automatic rendering recovery. The driver will **always** try to give you the best audio quality, no matter what MIDI you're trying to play.
- Spartan user interface, no *"fancy graphics"* which can distract the user from the original purpose of the driver, and designed for people who aims for *features* more than for *style*.
- The ability to use up to **4 cores/threads**, to ensure each function is executed at its best. Each core hosts a vital part of the driver: The first thread hosts the settings loader, the debug info writer etcetera, the second hosts the MIDI event parser, the third hosts the audio render and the fourth hosts the ASIO driver (When using the ASIO engine).
- Constant updates, to keep the driver fresh and always up-to-date to users requests.

It's meant for [professional people](#what-do-you-mean-by-for-professional-use) who wants a lot of settings to change almost every behaviour of the program.

### What do you mean by "for professional use"?
I'll be honest, when I programmed the interface of the driver, I made it to make it familiar for DAW experts or people who know how to use advanced programs.<br />
I've seen newbies getting angry at me after changing one settings, complaining that the driver kept crashing their apps while they were playing MIDIs/working on projects.<br /><br />
If you want something easy to use, I strongly recommend [VirtualMIDISynth 2.x](http://coolsoft.altervista.org/en/virtualmidisynth) by Claudio Nicora.<br />
His driver is definitely more stable than mine, and it's easier to use too. Go check it out.

### Can I use your program's source code for my program?
Sure, as long as you follow the [LICENSE](LICENSE.txt).

### Keppy's Direct MIDI API for developers
You can access the Keppy's Direct MIDI API from here: [Keppy's Direct MIDI API Documentation](https://github.com/KeppySoftware/OmniMIDI/tree/master/DeveloperContent/KDMAPI.md)<br/>
You can also access the source code for the Windows Multimedia Wrapper here: [WinMMWRP on GitHub](https://github.com/KeppySoftware/WinMMWRP)<br/>
Python bindings are available as well, get them from PyPI: [kdmapi](https://pypi.org/project/kdmapi/) (maintained by [SebaUbuntu](https://github.com/SebaUbuntu), source code [here](https://github.com/SebaUbuntu/kdmapi))

Here's a list of applications that currently have *native* support for the Keppy's Direct MIDI API:
- mmidi by Sono, the first third-party project to feature my API at all: _N/A_
- Chikara by Kaydax, a PFA clone that uses Vulkan, and aims to be the best performing MIDI player available: https://github.com/Kaydax/Chikara
- Kiva by Arduano, a multipurpose MIDI player with different graphic styles: https://github.com/arduano/Kiva
- Zenith by Arduano, a multipurpose MIDI render with different graphic styles: https://arduano.github.io/Zenith-MIDI/
- giradischi by SebaUbuntu, a simple Python + Qt6 MIDI player supporting multiple APIs, KDMAPI being one of them: https://github.com/SebaUbuntu/giradischi

### Can you make a WinMM patch for other drivers too?
There's a patch available for VirtualMIDISynth. You can get it here: https://github.com/KeppySoftware/WinMMWRP/releases/tag/4.2A

### Minimum system requirements for MIDI playback on x86/x64 systems
The minimum requirements for this synthesizer to work are the following:
- A SSE2-capable x86 CPU running at 1.5GHz
- 1024MB of RAM
- DirectX 9 capable sound card or better
- Windows Vista SP2 or greater *(Server versions are supported too)*

### Minimum system requirements for MIDI playback on ARM64 systems
The minimum requirements for this synthesizer to work are the following:
- Qualcomm® Snapdragon™ 835, or any ARM® Cortex-A57 based chip running at 2GHz or more
- 1536MB of RAM *(Required by Windows)*
- Any sound device supported by Windows 10 ARM64 *(Qualcomm® Aqstic™ or aptX™ DACs are recommended)*
- Windows 10 Spring Creators Update 2018

### Recommended system requirements for studio environments
For the best experience, it's recommended to run the synthesizer on a PC with the following specifications:
- AMD Ryzen 9 5900X
- 32GB of RAM *(3600MHz)*
- Realtek ALC1220 with ASIO4ALL or Native Instruments Komplete Audio 6 (or another dedicated ASIO-capable hardware interface)
- Windows 10 Pro 21H2
- OmniMapper and Windows Multimedia Wrapper for DAWs _(Both included in the driver's configurator, for easy installation)_

### Requirements for compiling the source code
To compile (and test) the synthesizer, you need:
- Microsoft Visual Studio 2022
- Inno Setup 6.1+ (It's recommended to install Inno Script Studio and the Inno Setup Pack)
- Inno Downloader Plugin
- Microsoft Windows SDK 10.0.22000

## ASIO support details
You can read the lists here: [OmniMIDIASIOSupportList folder on GitHub](https://github.com/KeppySoftware/OmniMIDI/tree/master/OmniMIDIASIOSupportList)
<br />
**WARNING**: Since I can not test all the ASIO devices available on the market (Mainly because they're not cheap), if you have one, please... Test it with OmniMIDI, then send me an e-mail about it to [kaleidonkep99@outlook.com](mailto:kaleidonkep99@outlook.com).

# Credits
BASSMIDI driver by Kode54 and mudlord: https://github.com/kode54/BASSMIDI-Driver
<br />
HtmlAgilityPack by Simon Mourier: https://www.nuget.org/packages/HtmlAgilityPack/
<br />
BASS libraries by Un4seen (Ian Luck): http://www.un4seen.com/
<br />
BASS.NET wrapper by radio42: http://bass.radio42.com/
<br />
Costura.Fody by Simon Cropp: https://github.com/Fody
<br />
Octokit by GitHub Inc.: https://developer.github.com/v3/libraries/

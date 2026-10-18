Language: [English](README.md) | [Русский](README.ru.md)

# URControl Qt - Steinberg UR-C audio control GUI

URControl Qt is a C++17 / Qt Widgets application that communicates with the interface's internal DSP mixer through **ALSA RawMIDI**. 

It provides a graphical front end for [urcontrol-c](https://github.com/RE-Kovalev/urcontrol-c), inspired by the functionality of Yamaha/Steinberg's dspMixFx software.

> **Project status:** Work in progress. Development and device testing are focused on the **UR22C**. Other UR-C models are listed for configuration purposes, but their compatibility has not been verified. This is an independent project and is not affiliated with Steinberg or Yamaha.

## Screenshot

![URControl Qt main window](Screenshots/MainWindow.png)

## Requirements

- Linux with ALSA and access to the interface's RawMIDI device.
- A C++17 compiler, CMake 3.16 or newer, and GNU Make (or compatible `make`).
- Qt 6 development packages for Widgets and LinguistTools.
- ALSA development headers and library (`libasound`).
- The [`urcontrol-c`](https://github.com/RE-Kovalev/urcontrol-c) library, included as a Git submodule in `third-party/urcontrol-c`.

## Build

Clone the project together with its submodule:

```bash
git clone --recurse-submodules https://github.com/RE-Kovalev/urcontrol-qt.git
cd urcontrol-qt
```

Configure and compile:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j "$(nproc)"
```

CMake builds the `urcontrol-c` static library using its Makefile before linking the Qt application.

Run the application **from the build directory** so it can find `devices.ini`:

```bash
cd build
./urcontrol-qt
```

## Getting started

1. Connect your UR-C audio interface to the computer.
2. Launch URControl Qt and select your **device model**.
3. Select the interface's **internal mixer-control MIDI port**, not the external MIDI DIN port.
4. Click **Connect** and wait for the initial parameter read to finish.
5. Adjust the available channel controls. Changes are sent to the device over MIDI SysEx.

You can inspect ALSA MIDI devices with:

```bash
amidi -l
```

or

```bash
./urcontrol-cli --get-midi-ports
```

Some interfaces expose multiple MIDI subdevices. The required port depends on the hardware and ALSA device numbering; do not assume that an example such as `hw:3,0,1` will match your system.

The **Auto-connect** option attempts to reconnect to the previously selected port when the application starts.

## Configuration

`devices.ini` defines selectable interface models and the number of input channels displayed. For example:

```ini
[UR22C]
Inputs=2

[UR44C]
Inputs=6
```

The application reads `devices.ini` from its **current working directory**. In the present implementation, models with more than six inputs are not added to the model selector, even if they appear in the configuration file.

The local `lasts` file stores the selected device model, MIDI port and auto-connect preference. These files are read from and written to the working directory; a system-wide settings location is not yet implemented.

## License and attribution

MIT License. See the repository's license information.

**Steinberg**, **Yamaha**, **UR-C** and **dspMixFx** are trademarks of their respective owners.

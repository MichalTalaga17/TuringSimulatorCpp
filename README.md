# C++ Turing Machine Simulator

A simple single-tape deterministic Turing machine simulator written in C++.

The app features:
- a simulation core in pure C++,
- a Qt Widgets graphical interface with editor tabs,
- saving and loading machines using JSON files,
- saving and loading machines from a local SQLite database,
- generating machine definitions via the Gemini API,
- state notation in the style of `q0`, `q1`, `q2`, etc.,
- head movement notation visible to the user: `L`, `R`, `_`, `STOP`.

## Supported languages

- English
- Polish

To switch languages on the fly, navigate to:
  `view` > `language` 
  (PL: `widok` > `język`)


## Download 
You can find the newest and legacy versions in Releases:
https://github.com/MichalTalaga17/TuringSimulatorCpp/releases


## Install and run

### Windows

- Download the `TuringSimulatorCpp-Windows.zip` file.
- Extract the contents of the archive to a folder of your choice.
- Open the extracted folder and double-click `TuringSimulatorCpp.exe` to start the application.

Tip: For quick access, right-click the `.exe` file and create shortcut on desktop.

### macOS 

- Download the `TuringSimulatorCpp-macOS.dmg` file.
- Double-click the downloaded `.dmg` file to mount the disk image.
- Drag and drop the TuringSimulatorCpp app icon into your Applications folder shortcut.
- Launch the simulator from your Launchpad or Applications folder.

Note: Because this is an open-source tool and not cryptographically signed with an Apple Developer certificate, macOS Gatekeeper may flag it on the first launch. To bypass this, right-click (or Control-click) the app in your Applications folder and select Open.

### Linux (Ubuntu)

- Download the `TuringSimulatorCpp-Linux.AppImage` file.
- Make the file executable. You can do this via your file manager by right-clicking the AppImage, selecting Properties, navigating to the Permissions tab, and checking the box to "Allow executing file as program".

Alternatively, open your terminal and run:
```bash
chmod +x TuringSimulatorCpp-Linux.AppImage
```
- Double-click the AppImage to run the simulator.


## Generating transition tables with Gemini AI

An `AI` button is available in the editor, and there is a `Start with AI` option on the start page.
The application will ask for a description of the machine and generate a transition table via the Gemini API.

You can set the API keys in the `Maszyna -> Ustawienia Gemini API...` menu.
The application stores a separate key for the `Free tier` and a separate key for the `Pro tier`.

You can create your API key at https://ai.google.dev/gemini-api/docs/api-key

Before generating, the application displays a single window with the task description, tier selection, and a thinking option. The `Pro tier` uses the `gemini-3.8-flash` model. `Free tier` initially uses `gemini-3.8-flash` and if the model is unavailable or rate-limited, it automatically falls back to `gemini-3.1-flash-lite`.

The application also asks whether to enable model thinking. For Gemini 3 models, it sends `thinkingLevel`: `high` or `minimal`.


## Doxygen

The project includes a `Doxyfile` configuration and a main documentation page in `docs/mainpage.dox`.

Generating directly via Doxygen:

    doxygen Doxyfile

If CMake finds the `doxygen` program, a target is also available:

    cmake --build build --target docs

The result will appear in the directory:

    docs/doxygen/html


## Starting example

The default example:
- starts in the `q0` state,
- changes successive `0` symbols to `1` and `1` to `0`,
- moves the head to the right,
- executes `STOP` upon encountering an empty symbol,
- ends operation with a `STOP` movement.

## Meaning of head movements

- `L` - move left
- `R` (or `P` in Polish version) - move right
- `_` - no movement
- `STOP` - stop the machine

## Project structure

Most important files:
- `include/Tape.h` - tape model
- `include/TuringMachine.h` - Turing machine logic
- `include/MachineEditorTab.h` - machine editing and simulation tab
- `include/TuringSimulatorUI.h` - main application window
- `include/storage.h` - API for saving to JSON and SQLite
- `src/ui/` - Qt interface implementation
- `src/storage/` - saving and loading implementation
- `tests/` - simulation core tests

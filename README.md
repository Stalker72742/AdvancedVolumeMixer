# Audio Profile Switcher (AdvancedVolumeMixer)

> **Vibe-coded.** Architecture, C++, QML, WASAPI plumbing, debugging and the build
> scripts were written by Claude, which played every developer role on this project.
> A human only asked for things, clicked around and complained.

Windows tray app that keeps **per-output volume profiles**. Every audio output
(headphones, speakers, HDMI...) gets its own set of profiles, and a profile is a
list of per-app volume/mute rules: *"Gaming on headphones: Discord 50%, game 80%,
Spotify muted"*. Switch profiles with a click, a global hotkey or the tray menu.

## Features

- Remembers every audio output it has seen, including disconnected ones
- Remove outputs you don't care about (virtual cables, monitor speakers...): they're hidden and ignored, and a "Removed" list lets you restore them or delete them for good
- Profiles per output, each with its own process rules and global hotkey
- Per-app volume and mute through the WASAPI session API
- Rules for processes that aren't running yet: they apply once the process starts playing audio
- Rule scopes: single process, group of processes (`a.exe; b.exe`), or all processes
- Process name picker with autocomplete and a list of running apps
- Edit mode: change several profiles and apply them all at once with Accept, or throw them away with Cancel
- Global hotkeys that work on any keyboard layout
- Lives in the tray: closing the window doesn't quit the app

Settings are stored in `%LOCALAPPDATA%\Stalker7274\AdvancedVolumeMixer\profiles.json`.

## Requirements

- Windows 10 / 11
- **Qt 6.8+, MinGW 64-bit kit**, installed with the [Qt Online Installer](https://www.qt.io/download-qt-installer).
  In the installer select:
  - `Qt 6.8.x → MinGW 64-bit`
  - `Developer and Designer Tools → MinGW 64-bit` (the matching compiler)
  - optionally `CMake` and `Ninja` from the same section
- CMake 3.21+ (the one from the Qt installer is fine)

## Build a release

```bat
build-release.bat
```

On the first run it asks for the Qt kit folder, for example `C:\Qt\6.8.3\mingw_64`
(the folder that has `bin\` and `lib\cmake\Qt6\` inside), and saves it to `build.local`.
It then finds MinGW, Ninja and CMake in the same Qt installation, builds an optimized
Release (`-O3`, LTO, stripped) and puts a clean, ready-to-run copy into `dist\`.

| Option | What it does |
|---|---|
| `build-release.bat C:\Qt\6.8.3\mingw_64` | use this Qt folder without asking |
| `--reconfigure` | forget the saved Qt folder and the build cache |
| `--zip` | also pack `dist\` into `AdvancedVolumeMixer.zip` |
| `--no-pause` | don't wait for a key at the end (for scripts) |

> Clone into a short path such as `C:\dev\AdvancedVolumeMixer`. With deep folders, Qt's
> generated file names go over the Windows 260-character path limit and the build fails.

## Develop in an IDE

CMake looks for Qt in this order:

1. `-DAVM_QT_DIR=C:/Qt/6.8.3/mingw_64` (or your own `-DCMAKE_PREFIX_PATH`)
2. a `QT_DIR=...` line in `build.local` next to `CMakeLists.txt` (git-ignored, written by `build-release.bat`)
3. the `QTDIR` environment variable

- **CLion:** set the toolchain to the MinGW from the Qt installer. Either run `build-release.bat` once, or add `-DAVM_QT_DIR=...` to the CMake options of the profile.
- **Qt Creator:** open `CMakeLists.txt` and pick the `Desktop Qt 6.8.x MinGW 64-bit` kit. Nothing else to set up.

A Debug build runs straight from its build folder, because `windeployqt` copies the Qt runtime there after every build.

## Project layout

```
main.cpp                      app entry, tray setup
source/public/controllers/    AudioMixerController: state, persistence, edit mode, hotkeys (QML singleton "Mixer")
source/public/models/         list models for outputs, profiles and rules
source/public/structs/        data structs with JSON (de)serialization
source/public/winWrappers/    WASAPI device watcher, audio sessions, RegisterHotKey
source/public/tray/           system tray icon and menu
ui/                           QML UI (Material style, dark theme in ui/theme/Theme.qml)
```

## License

[WTFPL](LICENSE): do whatever you want with it.

# Dino Runner

A Chrome T-Rex style game in C++ with OpenGL and GLFW.
Pick a level (Medium / Hard), a time of day (Morning / Noon / Evening / Night)
and a dinosaur (T-Rex / Raptor / Triceratops / Stegosaurus). It sometimes rains.

## Controls

| Where        | Keys                                                              |
|--------------|-------------------------------------------------------------------|
| Menu         | Up/Down choose a row, Left/Right change it, Enter or Space start |
| In game      | Space or Up = jump, Down = duck / fast fall, Esc = back to menu   |
| Game over    | Space or Enter = retry, M = menu                                  |
| Any time     | **Q** = quit, R = toggle rain                                     |

The window opens at 1200x450. You can resize or maximise it; it keeps its shape.

---

## What is in this folder

| File                          | Purpose                                              |
|-------------------------------|------------------------------------------------------|
| `trex.cpp`                    | The whole game                                       |
| `CMakeLists.txt`              | Build file for every OS (downloads GLFW by itself)   |
| `Dockerfile`                  | Builds the Linux **and** Windows versions in Docker  |
| `cmake/mingw-w64.cmake`       | Helper used by the Dockerfile for the Windows build  |
| `.github/workflows/build.yml` | Builds Linux, macOS and Windows on GitHub for free   |

---

## Option A: build natively with CMake (works on Windows, macOS, Linux)

You need CMake 3.14+, Git and a C++ compiler.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The game ends up in `build/bin/` (`dino_runner`, or `dino_runner.exe` on Windows).

Install the tools first if needed:

- **Windows:** install Visual Studio Build Tools (C++ workload) or MSYS2 MINGW64, plus CMake and Git.
- **macOS:** `xcode-select --install` and `brew install cmake`.
- **Ubuntu/Debian:**
  `sudo apt install build-essential cmake git libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev`

## Option B: build Linux + Windows with Docker (nothing else to install)

```
docker build --target artifacts --output type=local,dest=dist .
```

This creates:

- `dist/linux/dino_runner` - runs on most Linux desktops
- `dist/windows/dino_runner.exe` - a single self-contained Windows 64-bit file, no DLLs needed

Send the file to the person; they just run it.

### Running the game inside Docker (Linux desktop only)

```
docker build --target run-linux -t dino-runner .
xhost +local:docker
docker run --rm -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix dino-runner
```

This uses software rendering, so it is slower than the native build.

**Important:** Docker cannot show a game window on macOS, and on Windows it only works in
a limited way (Docker Desktop with WSL2/WSLg). Windows and macOS users should run the
native build instead (the `.exe` from Option B, or a macOS build from Option C).

## Option C: get macOS (and all other) builds from GitHub, no Mac needed

macOS programs can only be built on a Mac or on Apple's own build machines. GitHub
provides these for free:

1. Create a GitHub repository and upload this whole folder.
2. Open the **Actions** tab. The `build` workflow runs by itself.
3. When it finishes, download the artifacts: `dino-runner-Linux`, `dino-runner-macOS`
   and `dino-runner-Windows`.

macOS note: the downloaded app is unsigned, so macOS may refuse to open it. Fix it once with:

```
chmod +x dino_runner
xattr -d com.apple.quarantine dino_runner
./dino_runner
```

## Troubleshooting

- **Black or blank window on Linux in a VM / remote desktop:** set `LIBGL_ALWAYS_SOFTWARE=1` before running.
- **`Failed to create window`:** your graphics driver or display is not available (for example over plain SSH).
- **CMake cannot download GLFW:** the first configure needs internet access to github.com.

# Copilot instructions — spring_physics

Summary
- Small single-binary SDL2 C++ spring simulation. Main entry: [spring.cxx](spring.cxx#L1).
- Core helpers live under `lib/`: [lib/window.cxx](lib/window.cxx#L1-L220) and [lib/vec2.cxx](lib/vec2.cxx#L1-L200).

Build / run
- The `Makefile` target `all` compiles the program with `sdl2-config`: see [Makefile](Makefile#L1-L3).
- Typical build command (used in repo):

```sh
g++ spring.cxx -o a.exe `sdl2-config --cflags --libs` -lSDL2_ttf
```

- Environment: expects an environment that provides `sdl2-config` (e.g. Linux dev shell or compatible MSYS2 setup). `README.md` notes CxxDroid/PC usage: [README.md](README.md#L1).

Architecture & key patterns
- Single translation-unit style: `spring.cxx` includes implementation files directly (`#include "lib/window.cxx"`, `#include "lib/vec2.cxx"`). Do not refactor into separate .o files without adjusting build and include guards. See [spring.cxx](spring.cxx#L1-L8).
- `lvichki::Window` (in `lib/window.cxx`) owns the SDL loop and exposes three callback hooks: `on_update`, `on_draw`, `on_event`. The simulation sets these lambdas in `spring.cxx` to implement physics and rendering. See: [lib/window.cxx](lib/window.cxx#L20-L60) and [spring.cxx](spring.cxx#L20-L90).
- Time-step: `Window` computes `dt` in `update_delta_and_fps()`; physics in `on_update` uses `win.dt`. Keep updates tied to `win.dt` rather than manual time computations. See [lib/window.cxx](lib/window.cxx#L170-L220) and [spring.cxx](spring.cxx#L40-L60).
- Math: `Vec2` in [lib/vec2.cxx](lib/vec2.cxx#L1-L120) provides common operators and `normalize()`; prefer these helpers over rolling custom vector math.

Conventions and gotchas
- Files in `lib/` are implementation-style `.cxx` files and may contain `#pragma once` (e.g., `vec2.cxx`). Treat them like headers when included into `spring.cxx`.
- Window uses SDL_ttf and attempts to load `FreeSans.ttf` with an Android fallback path. If text rendering fails, the program still runs but text/FPS won't display. See font loading in [lib/window.cxx](lib/window.cxx#L24-L44).
- To change font at runtime use `Window::set_font(const char* path, int size)` in `lib/window.cxx` (search `set_font`).

Extending the project
- To add a new module quickly: either include the new `.cxx` in `spring.cxx` (follow current style) or convert to separate compilation and update the `Makefile` to compile and link multiple objects. If you split files, ensure you remove duplicate symbols and do not `#include` implementation files into multiple translation units.
- Add new input handling via `win.on_event` lambda in `spring.cxx` (example uses mouse and touch events): [spring.cxx#L130-L175].

Debugging and logging
- `cout` prints (see `spring.cxx` start) and `SDL_Log` messages from `lib/window.cxx` are primary runtime logs. Run the produced `a.exe` from a shell to view console output.

What to watch when editing
- Keep `Window` lifecycle responsibilities (init SDL/TTF, create/destroy renderer and window) inside `lib/window.cxx`; avoid moving SDL initialization out unless you adjust teardown in destructor. See constructor/destructor: [lib/window.cxx#L10-L30] and [lib/window.cxx#L80-L100].
- If you add global variables or another `main`, beware the single-binary structure—tests or example programs should be separate executables.

If anything above is unclear or you'd like the instructions expanded (examples for converting to multi-file build, MSYS2-specific build steps, or adding a GitHub workflow), tell me which area to extend.

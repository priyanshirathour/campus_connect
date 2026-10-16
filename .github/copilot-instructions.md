# Campus Connect Instructions

## Project
- This is a Windows-oriented C++17 console application built with CMake.
- The application uses the MongoDB C++ driver (`mongocxx` and `bsoncxx`); MongoDB is the runtime source of truth. Plain-text files under `data/` are exit-time snapshots.
- Keep the existing layers distinct: `CampusConnectApp` handles menus, services own validation and business rules, models represent records, and the repository handles MongoDB serialization and operations.
- The website is a presentation client. `campus_connect --api` exposes read-only loopback endpoints backed by C++ services; keep business rules in C++ and keep Node routes allowlisted.
- Authentication is owned by the C++ `AuthenticationService`; keep credentials hashed, session cookies HttpOnly, and never add default passwords or browser-stored session tokens.
- Follow existing ownership boundaries and public APIs. Keep changes focused and avoid unrelated generated files under `build/` or runtime snapshots under `data/`.

## Build and Test
- Configure on Windows from a Visual Studio Developer Command Prompt with the installed vcpkg toolchain, for example:
  `cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake`
- Build with `cmake --build build --config Release`.
- Run core tests with `ctest --test-dir build -C Release --output-on-failure`.
- Core tests cover data structures and do not require running the MongoDB-backed application. Manual persistence checks require MongoDB to be available.
- CMake globs every `.cpp` under `src/` into the application; the core test target explicitly lists its source files. Preserve `src/main.cpp` in the application sources.
- vcpkg dependencies are declared in `vcpkg.json`; preserve the manifest and its CMake package targets.

## Implementation Notes
- Preserve C++17 compatibility and the existing MSVC warning settings.
- Waitlist behavior must remain FIFO; registration and cancellation changes should preserve queue order and notification/persistence behavior.
- Update `docs/` or `README.md` when a change alters documented architecture, setup, or user-visible behavior.
# CMake package

Requires CMake 3.25 or later and a C17 compiler. Dependencies must provide their
CMake config packages through `CMAKE_PREFIX_PATH` or a package-manager toolchain.

```sh
cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/tmp/si-prefix
cmake --build build-cmake
cmake --install build-cmake
```

Consume with `find_package(sicore CONFIG REQUIRED)` and link
`sicore::sicore`. Dependencies and static-library definitions propagate through
the exported target. Set `BUILD_SHARED_LIBS=ON` for a shared build.

CMake writes build artifacts into its build directory. Export headers are part
of the public source tree and can also be used by other build workflows.


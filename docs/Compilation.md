# Building

The project should work on Linux, Windows and OSX. It should work with any ide that is either supported by cmake or has direct cmake support. Personally I'm using vscode with clangd at the moment. But also the command line with plain old `make`.

## Linux

There is a `Makefile` wrapper around the build system. You can just run `make` in the project root folder.

Every project has some extra CMake targets. There are e.g. `voxedit-run`, `voxedit-debug` and `voxedit-perf` if the needed tools were found during cmake's configure phase.

That means that you can compile a single target by typing `make voxedit`, run it by typing `make voxedit-run`, debug it by typing `make voxedit-debug` and profile it by
typing `make voxedit-perf`. There are also other targets for valgrind - just use the tab completion in the build folder to get a list.

## Windows

The project should be buildable with every ide that supports cmake. QTCreator, Eclipse CDT, vscode or Visual Studio. Just install cmake, generate the project files, and open them in your ide.

### Visual Studio Code

* Install ninja https://ninja-build.org/
* Install cmake https://cmake.org/download/
* Install vscode https://code.visualstudio.com/
* Install Visual Studio (for the compiler)

Open your git clone directory in vscode and let it configure via cmake. It will pick ninja and the visual studio compiler automatically.

## Mac

You can generate your xcode project via cmake or build like this:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

If you are using the cmake Makefile generator, you get the same targets as for Linux above. You can also just type `make voxedit-run` to compile and run only VoxEdit.

## Hints

If you encounter any problems, it's also a good start to check out the build pipelines of the project.
This is always the most up-to-date information about how-to-build-the-project that you will find. But
also please don't hesitate to ask for help on our discord server.

## Enforce bundled libs

You can enforce the use of the bundled libs by putting a `<LIB>_LOCAL=1` in your cmake cache.
Example: By putting `LUA54_LOCAL=1` into your cmake cache, you enforce the use of the bundled lua sources from `contrib/libs/lua54`.

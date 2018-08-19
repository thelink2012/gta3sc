# gta3sc

[![Build Status](https://travis-ci.org/thelink2012/gta3sc.svg?branch=gta3sc-rewrite)](https://travis-ci.org/thelink2012/gta3sc)
[![Build status](https://ci.appveyor.com/api/projects/status/ut954whp2lp81gyk/branch/gta3sc-rewrite?svg=true)](https://ci.appveyor.com/project/thelink2012/gta3sc)
[![codecov](https://codecov.io/gh/thelink2012/gta3sc/branch/gta3sc-rewrite/graph/badge.svg)](https://codecov.io/gh/thelink2012/gta3sc)

This is a script compiler/decompiler and a library for manipulating the so called [GTA3script](http://www.gtamodding.com/wiki/SCM_language) language, which was used to design the mission scripts of the 3D Universe of the series.

This branch is a complete rewrite of the codebase, it is **very incomplete**! Please switch to the **master branch** for the stable codebase.

The goal of the rewrite is manifold:

 + Provide a simple and clean codebase.
 + Provide a modular library based architecture.
 + Provide a efficient and less memory hungry compiler.
 + Provide a reference implementation for the coherent subset of the language (see [gta3script-specs](https://github.com/GTAmodding/gta3script-specs)).

The previous codebase was neither of these. This rewrite is being developed in its own time (and concurrently with the language specification), so do not expect it to be complete any time soon.

For a high-level overview of the project, see the [Design Internals](./DESIGN.adoc) document.

## Building

You need a C++17 enabled compiler. Windows (MSVC) and Linux (GCC/Clang) are officially supported.

Before building make sure you have fetched all submodules:

    git submodule update --init --recursive

Then run:

    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
    cmake --build .

## Using

TODO

## Testing

TODO

TODO the build status is for the rewrite branch
TODO move the config directory to a community submodule
TODO a frontend driver, plus lit tests
TODO remove deps/
TODO a install script


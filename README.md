# gdbus-cpp

[TOC]

This repository should provide easy to use C++ API to communicate with the DBus using GDBus.

## Configure, build, test, package ...

The project uses [CMake presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) as a way to share CMake configurations.
Refer to [cmake](https://cmake.org/cmake/help/latest/manual/cmake.1.html), [ctest](https://cmake.org/cmake/help/latest/manual/ctest.1.html) and [cpack](https://cmake.org/cmake/help/latest/manual/cpack.1.html) documentation for more information on the use of presets.

## Adding the libraries to your CMake project

```CMake
include(FetchContent)
FetchContent_Declare(
	GDbusCpp	
	GIT_REPOSITORY https://github.com/amarula/libcppconnman.git 
	GIT_TAG vMAJOR.MINOR.PATCH 
	FIND_PACKAGE_ARGS MAJOR.MINOR CONFIG
	)
FetchContent_MakeAvailable(GDbusCpp)

target_link_libraries(<target> <PRIVATE|PUBLIC|INTERFACE> Amarula::GDbusProxy Amarula::GConnmanDbus)
```

## API reference

You can read the [API reference](https://amarula.github.io/libcppconnman/), or generate it yourself like

```
cmake --workflow --preset default-documentation
```

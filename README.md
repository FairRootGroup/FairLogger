# FairLogger

Lightweight and fast C++ Logging Library

| Branch | Build Status |
| :---: | :--- |
| `master` | ![build status master branch](https://alfa-ci.gsi.de/buildStatus/icon?job=FairRootGroup/FairLogger/master) |
| `dev` | ![build status dev branch](https://alfa-ci.gsi.de/buildStatus/icon?job=FairRootGroup/FairLogger/dev) |

## Installation

```bash
git clone https://github.com/FairRootGroup/FairLogger
mkdir FairLogger_build && cd FairLogger_build
cmake -DCMAKE_INSTALL_PREFIX=./FairLogger_install ../FairLogger
cmake --build . --target install
```

## Usage

In your `CMakeLists.txt`:

```cmake
find_package(FairLogger)
```

If FairLogger is not installed in system directories, you can hint the installation location:

```cmake
set(CMAKE_PREFIX_PATH /path/to/FairLogger/installation ${CMAKE_PREFIX_PATH})
find_package(FairLogger)
```

`find_package(FairLogger)` will define an imported target `FairLogger::FairLogger`.

## CMake options

On command line:

  * `-DDISABLE_COLOR=ON` disables coloured console output.

## License

GNU Lesser General Public Licence (LGPL) version 3, see [LICENSE](LICENSE).

Copyright (C) 2017-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH

# dfglib

Experimental general purpose utility library for C++.

Note: this is *not* a mature library and is not intended or recommended for general use. Libraries such as [Abseil](https://abseil.io/) or [Boost](https://www.boost.org/) provide many of the features in dfglib implemented in a more clear and professional manner. For a comprehensive list of alternatives, see [A list of open source C++ libraries at cppreference.com](https://en.cppreference.com/w/cpp/links/libs) 

## News

* 2020-11-15, dfgQtTableEditor version 1.7.0
* 2020-08-16, dfgQtTableEditor [version 1.6.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_1.6.0)
* 2020-07-22: dfgQtTableEditor [version 1.5.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_1.5.0)
* 2020-02-11: dfglib is no longer tested with the following compilers:
    * MSVC2010, MSVC2012, MSVC2013, MinGW 4.8.0
    * Latest tested commit available as branch: [legacy_msvc_2010_to_2013](https://github.com/tc3t/dfglib/tree/legacy_msvc_2010_to_2013)
* 2020-02-11: dfgQtTableEditor [version 1.1.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_1.1.0.0)
* 2019-09-10: dfgQtTableEditor [version 1.0.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_1.0.0.0)

## Usage

Large proportion of features are 'header only' and can be used simply by including header without anything to link. When .cpp -files are needed, the intended usage is to include them in the build of the target project; there are no cmake-files etc. for building dfglib as a library.

### Building and running unit tests (dfgTest)

* Make sure that Boost is available in include path

1. With command line CMake
    * cd dfgTest
    * cmake CMakeLists.txt
    * make
    * ./dfgTest
2. Visual studio project:
    * Open dfgTest/dfgTest.sln
    * Build and run dfgTest-project with an available MSVC-compiler.

For a list of supported compilers, see "Build status"-section in this document.

## Features

The library consists of miscellaneous features such as algorithms, containers, math/numerics, streams, typed string and UTF-handling. Below are some notable features:

* Streams
    * [non-virtual streams](dfg/io/) with basic interface compatibility with standard streams.
    * [encoding-aware streams](dfg/io/) (i.e. streams that can read/write e.g. UTF-encoding (UTF8, UTF16BE/LE, UTF32BE/LE))

* CSV-file [reading/writing](dfg/io/) with [somewhat reasonable performance](misc/csvPerformanceRuns.md) (e.g. compared to some spreadsheet applications) and [Qt-widgets](dfg/qt/) for editing CSV-files.

* Simple CSV-editor as an example, for details see it's [readme](dfgExamples/dfgQtTableEditor/README.md/)

* Typed strings (i.e. strings that store encoded text instead of raw bytes)

* Algorithms such as
    * [median](dfg/numeric/median.hpp) & [percentile](dfg/numeric/percentile.hpp)
    * [polynomial evaluation](dfg/math/evalPolynomial.hpp) (Horner's method)
    * [spearman correlation](dfg/dataAnalysis/correlation.hpp)
    * [spreadsheet sorting](dfg/alg/sortMultiple.hpp)

* Containers such as [flat map](dfg/cont/MapVector.hpp) and [flat set](dfg/cont/SetVector.hpp)

## Third party code

Summary of 3rd party code in dfglib (last revised 2020-11-10).

| Library      | Usage      | License  | Comment |
| ------------- | ------------- | ----- | ------- |
| [Boost](http://www.boost.org/)  | i,m,ti (used in numerous places)          | [Boost software license](http://www.boost.org/LICENSE_1_0.txt) | Exact requirement for Boost version is unknown; unit tests have been successfully build and run with Boost versions 1.55, 1.61, 1.65.1 and 1.70.0 |
| [Colour Rendering of Spectra](dfg/colour/specRendJw.cpp) | m (used in colour handling tools) | [Public domain](dfg/colour/specRendJw.cpp) | 
| [cppcsv](https://github.com/paulharris/cppcsv) (commit [daa3d881](https://github.com/paulharris/cppcsv/tree/daa3d881d995b6695b84dde860126fa5f773de56/include/cppcsv), 2020-01-10) | c,t | [MIT](https://github.com/paulharris/cppcsv) | 
| [dlib](http://dlib.net/)    | m,ti (unit-aware integration and various tests)           | [Boost software license](http://www.boost.org/LICENSE_1_0.txt)  | Can be excluded from unit tests with option DFGTEST_BUILD_OPT_USE_DLIB
| [fast-csv-cpp-parser](https://github.com/ben-strasser/fast-cpp-csv-parser/) (commit [66365610](https://github.com/ben-strasser/fast-cpp-csv-parser/blob/66365610817b929b451819e0cccdb702d46a605e/csv.h), 2020-05-19) | c,t | [BSD-3](dfg/io/fast-cpp-csv-parser/csv.h) |
| [fmtlib](https://github.com/fmtlib/fmt) (version 4.1.0 with an adjustment related to formatting doubles as string) | m (string formatting)| [BSD-2](dfg/str/fmtlib/format.h) |
| [Google Test](https://github.com/google/googletest) (version 1.8.1) | t | [BSD-3](externals/gtest/gtest.h) | Unit tests are implemented with Google Test
| [LibQxt](https://bitbucket.org/libqxt/libqxt/wiki/Home) | c,t (QxtSpanSlider) | [BSD-3](dfg/qt/qxt/core/qxtglobal.h) | Qt-related utilities
| [muparser](https://github.com/beltoforion/muparser) (development version 2.3.3, commit [2deb86a8](https://github.com/beltoforion/muparser/tree/2deb86a81edc7d8e56859484524738ff766b4fdb), 2020-09-21) with some edits) | m (math::FormulaParser) | [BSD-2](dfg/math/muparser/muParser.h) | Formula parser. Namespace of the code has been edited from mu to dfg_mu.
| [QCustomPlot](https://www.qcustomplot.com/) | oi (in parts of dfg/qt) | [GPLv3/commercial](https://www.qcustomplot.com/) | Used in data visualization (charts) in dfgQtTableEditor. Version 2.0.1 is known to work.
| [Qt 5](https://www.qt.io/) | i (only for components in dfg/qt) | [Various](http://doc.qt.io/qt-5/licensing.html) | Known to work with 5.9, earliest version that works might be 5.6.
| [UTF8-CPP](https://github.com/nemtrif/utfcpp) (version 3.1) | m (utf handling) | [Boost software license](dfg/utf/utf8_cpp/utf8.h) |

Usage types:
* c: All or some code from the library comes with dfglib (possibly modified), but is not directly used (i.e. related code can be removed without breaking any features in dfglib).
* i: Include dependency (i.e. some parts of dfglib may fail to compile if the 3rd party library is not available in include-path)
* m: Some code is integrated in dfglib itself possibly modified.
* oi: Like i, but optional component that is off by default.
* t: Used in test code without (external) include dependency (i.e. the needed code comes with dfglib).
* ti: Used in test code with include dependency.

## Build status (as of 2020-07-20 commit [b5d7fd41](https://github.com/tc3t/dfglib/commit/b5d7fd41fc55f32edc3cd019e05a4e89dedfd126), with Boost 1.70.0 unless stated otherwise)

<!-- [![Build status](https://ci.appveyor.com/api/projects/status/89v23h19mvv9k5u3/branch/master?svg=true)](https://ci.appveyor.com/project/tc3t/dfglib/branch/master) -->

| Compiler      | Platform      | Config  | Tests (passed/all) | Comment |
| ------------- | ------------- | -----   | ------  | ------- |
| Clang 3.8.0   | x86           |         | 100 % (248/248) | Boost 1.61, Ubuntu 32-bit 16.04 |
| Clang 6.0.0   | x64           | Release | 100 % (248/248) | Boost 1.65.1, Ubuntu 64-bit 18.04 |
| GCC 5.4.0     | x86           |         | 100 % (248/248) | Boost 1.61, Ubuntu 32-bit 16.04 |
| GCC 7.5.0     | x64           | Release | 100 % (248/248) | Boost 1.65.1, Ubuntu 64-bit 18.04 |
| MinGW 7.3.0   | x64           | O2      | 100 % (253/253) | |
| VC2015        | x86           | Debug   | 100 % (256/256) | |
| VC2015        | x86           | Release | 99 % (255/256) | Numerical precision related failure in dfgNumeric.transform |
| VC2015        | x64           | Debug   | 100 % (256/256) | |
| VC2015        | x64           | Release | 99 % (255/256) | Numerical precision related failure in dfgNumeric.transform |
| VC2017        | x86           | Debug   | 100 % (256/256) | |
| VC2017        | x86           | Release | 99 % (255/256) | Numerical precision related failure in dfgNumeric.transform |
| VC2017        | x64           | Debug   | 100 % (256/256) | |
| VC2017        | x64           | Release | 99 % (255/256) | Numerical precision related failure in dfgNumeric.transform |
| VC2019        | x86           | Debug   | 100 % (255/255) | std:c++17 with Conformance mode |
| VC2019        | x86           | Release | 100 % (255/255) | std:c++17 with Conformance mode |
| VC2019        | x64           | Debug   | 100 % (255/255) | std:c++17 with Conformance mode |
| VC2019        | x64           | Release | 100 % (255/255) | std:c++17 with Conformance mode |
||||||

### dfglib is no longer tested with the following compilers:

| Compiler | Untested since | Support branch |
| -------- | --------------- | --------------------- |
| MSVC2013 | 2020-02-11 | [legacy_msvc_2010_to_2013](https://github.com/tc3t/dfglib/tree/legacy_msvc_2010_to_2013) |
| MSVC2012 | 2020-02-11 | [legacy_msvc_2010_to_2013](https://github.com/tc3t/dfglib/tree/legacy_msvc_2010_to_2013) |
| MSVC2010 | 2020-02-11 | [legacy_msvc_2010_to_2013](https://github.com/tc3t/dfglib/tree/legacy_msvc_2010_to_2013) |
| MinGW 4.8.0 | 2020-02-11 | [legacy_msvc_2010_to_2013](https://github.com/tc3t/dfglib/tree/legacy_msvc_2010_to_2013) |

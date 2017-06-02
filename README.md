# dfglib

Experimental general purpose utility library.

## Features

The library consists of miscellaneous features such as algorithms, containers, math/numerics, streams, typed string and UTF-handling. Below are some notable features:

* Streams
    * [non-virtual streams](dfg/io/) with basic interface compatibility with standard streams.
    * [encoding-aware streams](dfg/io/) (i.e. streams that can read/write e.g. UTF-encoding (UTF8, UTF16BE/LE, UTF32BE/LE))

* CSV-file [reading/writing](dfg/io/) with [somewhat reasonable performance](misc/csvPerformanceRuns.md) (e.g. compared to some spreadsheet applications) and [Qt-widgets](dfg/qt/) for editing CSV-files.

* Typed strings (i.e. strings that store encoded text instead of raw bytes)

* Algorithms such as
    * [median](dfg/numeric/median.hpp) & [percentile](dfg/numeric/percentile.hpp)
    * [polynomial evaluation](dfg/math/evalPolynomial.hpp) (Horner's method)
    * [spearman correlation](dfg/dataAnalysis/correlation.hpp)
    * [spreadsheet sorting](dfg/alg/sortmultiple.hpp)

* Containers such as [flat map](dfg/cont/MapVector.hpp) and [flat set](dfg/cont/SetVector.hpp)

## Third party code

Summary of 3rd party code in dfglib (last revised 2017-06-02).

| Library      | Usage      | License  |
| ------------- | ------------- | ----- |
| [Boost](http://www.boost.org/)  | i,m,ti (used in numerous places)          | [Boost software license](http://www.boost.org/LICENSE_1_0.txt)  |
| [Colour Rendering of Spectra](dfg/colour/specRendJw.cpp) | m (used in colour handling tools) | [Public domain](dfg/colour/specRendJw.cpp) | 
| [dlib](http://dlib.net/)    | m,ti (unit-aware integration and various tests)           | [Boost software license](http://www.boost.org/LICENSE_1_0.txt)  |
| [fast-csv-cpp-parser](https://github.com/ben-strasser/fast-cpp-csv-parser/) | c,t | [BSD-3](dfg/io/fast-cpp-csv-parser/csv.h) |
| [fmtlib](https://github.com/fmtlib/fmt) | m (string formatting)| [BSD-2](dfg/str/fmtlib/format.h) |
| [Google Test](https://github.com/google/googletest) | t | [BSD-3](externals/gtest/gtest.h) |
| [LibQxt](https://bitbucket.org/libqxt/libqxt/wiki/Home) | c,t (QxtSpanSlider) | [BSD-3](dfg/qt/qxt/core/qxtglobal.h) |
| [Qt](https://www.qt.io/) | i (only for components in dfg/qt) | [Various](http://doc.qt.io/qt-5/licensing.html) |
| [UTF8-CPP](https://github.com/nemtrif/utfcpp) | m (utf handling) | [Boost software license](dfg/utf/utf8_cpp/utf8.h) |

Usage types:
* c: All or some code from the library comes with dfglib (possibly modified), but is not directly used (i.e. related code can be removed without breaking any features in dfglib).
* i: Include dependency (i.e. some parts of dfglib may fail to compile if the 3rd party library is not available in include-path)
* m: Some code is integrated in dfglib itself possibly modified.
* t: Used in test code without (external) include dependency (i.e. the needed code comes with dfglib).
* ti: Used in test code with include dependency.


## Build status

[![Build status](https://ci.appveyor.com/api/projects/status/89v23h19mvv9k5u3/branch/master?svg=true)](https://ci.appveyor.com/project/tc3t/dfglib/branch/master)

| Compiler      | Platform      | Config  | Status | Comment |
| ------------- | ------------- | -----   | ------ | ------- |
| MinGW 4.8.0   | x86           | O2      | no CI  | Building and running of unit tests manually maintained |
| VC2010        | x86           | Debug   | CI | Unit tests build and run |
| VC2010        | x86           | Release | no CI | Skipped in CI since unit test build fails for unknown reasons since 2016-11-25 |
| VC2010        | x64           | Debug   | CI | Unit tests build and run |
| VC2010        | x64           | Release | no CI | Skipped in CI since unit test build fails for unknown reasons since 2016-11-25 |
| VC2012        | x86           | Debug   | CI | Unit tests build and run |
| VC2012        | x86           | Release | no CI | Skipped in CI since unit test build fails, possibly the same problem as in VC2010 release. |
| VC2012        | x64           | Debug   | CI | Unit tests build and run |
| VC2012        | x64           | Release | no CI | Skipped in CI since unit test build fails, possibly the same problem as in VC2010 release. |
| VC2013        | x86           | Debug   | CI | Unit tests build and run |
| VC2013        | x86           | Release | CI | Unit tests build and run |
| VC2013        | x64           | Debug   | CI | Unit tests build and run |
| VC2013        | x64           | Release | CI | Unit tests build and run |
| VC2015        | x86           | Debug   | CI | Unit tests build and run |
| VC2015        | x86           | Release | CI | Unit tests build and run |
| VC2015        | x64           | Debug   | CI | Unit tests build and run |
| VC2015        | x64           | Release | CI | Unit tests build and run |
| VC2017        | x86           | Debug   | no CI | Manually maintained, unit tests build and run (2017-03-09) |
| VC2017        | x86           | Release | no CI | Manually maintained, unit tests build and run (2017-03-09) |
| VC2017        | x64           | Debug   | no CI | Manually maintained, unit tests build and run (2017-03-09) |
| VC2017        | x64           | Release | no CI | Manually maintained, unit tests build and run (2017-03-09) |
| Others        |               |         | Not tested |  |
||||||

# dfglib

Experimental general purpose utility library for C++, which has become mostly a backbone for csv-oriented table editor, [dfgQtTableEditor](dfgExamples/dfgQtTableEditor/README.md/)

Note: this is *not* a mature library and is not intended or recommended for general use. Libraries such as [Abseil](https://abseil.io/) or [Boost](https://www.boost.org/) provide many of the features in dfglib implemented in a more clear and professional manner. For a comprehensive list of alternatives, see [A list of open source C++ libraries at cppreference.com](https://en.cppreference.com/w/cpp/links/libs) 

## News

* 2022-10-30: dfglib is no longer tested with MSVC2015, GCC 5.4 and Clang 3.8.0, latest tested commit available as branch: [legacy_msvc_2015_GCC_5.4](https://github.com/tc3t/dfglib/tree/legacy_msvc_2015_GCC_5.4)
* 2022-10-23, dfgQtTableEditor [version 2.4.0](dfgExamples/dfgQtTableEditor/README.md/#240-2022-10-23) (Multithreaded reading, (x,y,text)-chart, read-only column, fixed crash opening big files...)
* 2022-05-22, dfgQtTableEditor [version 2.3.0](dfgExamples/dfgQtTableEditor/README.md/#230-2022-05-22) (Numerically sortable columns, 'Change radix'-tool, textFilter chart operation...)
* 2022-01-18, dfgQtTableEditor [version 2.2.0](dfgExamples/dfgQtTableEditor/README.md/#220-2022-01-18) (Hidable columns, date/time related improvements...)
* 2021-09-09, dfgQtTableEditor [version 2.1.0](dfgExamples/dfgQtTableEditor/README.md/#210-2021-09-09) (multicolumn filtering, read-only mode...)
* 2021-06-24, dfgQtTableEditor [version 2.0.0](dfgExamples/dfgQtTableEditor/README.md/#200-2021-06-24)
* 2021-04-29, dfgQtTableEditor [version 1.9.0](dfgExamples/dfgQtTableEditor/README.md/#190-2021-04-29)
* 2021-02-21, dfgQtTableEditor [version 1.8.0](dfgExamples/dfgQtTableEditor/README.md/#180-2021-02-21)
* 2020-11-15, dfgQtTableEditor [version 1.7.0](dfgExamples/dfgQtTableEditor/README.md/#170-2020-11-15)
* 2020-08-16, dfgQtTableEditor [version 1.6.0](dfgExamples/dfgQtTableEditor/README.md/#160-2020-08-16)
* 2020-07-22: dfgQtTableEditor [version 1.5.0](dfgExamples/dfgQtTableEditor/README.md/#150-2020-07-22)
* 2020-02-11: dfglib is no longer tested with the following compilers:
    * MSVC2010, MSVC2012, MSVC2013, MinGW 4.8.0
    * Latest tested commit available as branch: [legacy_msvc_2010_to_2013](https://github.com/tc3t/dfglib/tree/legacy_msvc_2010_to_2013)
* 2020-02-11: dfgQtTableEditor [version 1.1.0](dfgExamples/dfgQtTableEditor/README.md/#110-2020-02-11)
* 2019-09-10: dfgQtTableEditor [version 1.0.0](dfgExamples/dfgQtTableEditor/README.md/#100-2019-09-05)

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

### Building and running unit tests for Qt-components (dfgTestQt)

* Open dfgTestQt/dfgTestQt.pro in Qt Creator
* Choose suitable Qt kit and compiler to use
* Build and run

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

Summary of 3rd party code in dfglib (last revised 2022-10-16).

| Library      | Usage      | License  | Comment |
| ------------- | ------------- | ----- | ------- |
| [Boost](http://www.boost.org/)  | i,m,ti (used in numerous places)          | [Boost software license](http://www.boost.org/LICENSE_1_0.txt) | Exact requirement for Boost version is unknown; unit tests have been successfully build and run with Boost versions 1.61, 1.65.1, 1.70.0 and 1.71.0 |
| [Colour Rendering of Spectra](dfg/colour/specRendJw.cpp) | m (used in colour handling tools) | [Public domain](dfg/colour/specRendJw.cpp) | 
| [cppcsv](https://github.com/paulharris/cppcsv) (commit [daa3d881](https://github.com/paulharris/cppcsv/tree/daa3d881d995b6695b84dde860126fa5f773de56/include/cppcsv), 2020-01-10) | c,t | [MIT](https://github.com/paulharris/cppcsv) | 
| [dlib](http://dlib.net/)    | m (unit-aware integration) | [Boost software license](http://www.boost.org/LICENSE_1_0.txt)  |
| [fast-csv-cpp-parser](https://github.com/ben-strasser/fast-cpp-csv-parser/) (commit [66365610](https://github.com/ben-strasser/fast-cpp-csv-parser/blob/66365610817b929b451819e0cccdb702d46a605e/csv.h), 2020-05-19) | c,t | [BSD-3](dfg/io/fast-cpp-csv-parser/csv.h) |
| [fmtlib](https://github.com/fmtlib/fmt) (version 4.1.0 with an adjustment related to formatting doubles as string) | m (string formatting)| [BSD-2](dfg/str/fmtlib/format.h) |
| [GoogleTest](https://github.com/google/googletest) (version 1.11.0) | t | [BSD-3](externals/gtest/gtest.h) | Unit test framework used in both dfgTest and dfgTestQt.
| [LibQxt](https://bitbucket.org/libqxt/libqxt/wiki/Home) | c,t (QxtSpanSlider) | [BSD-3](dfg/qt/qxt/core/qxtglobal.h) | Qt-related utilities
| [muparser](https://github.com/beltoforion/muparser) (version 2.3.3, commit [5ccc8878](https://github.com/beltoforion/muparser/tree/5ccc887864381aeacf1277062fcbb76475623a02), 2022-01-13) with some edits | m (math::FormulaParser) | [BSD-2](dfg/math/muparser/muParser.h) | Formula parser. Namespace of the code has been edited from mu to dfg_mu.
| [QCustomPlot](https://www.qcustomplot.com/) | oi (in parts of dfg/qt) | [GPLv3/commercial](https://www.qcustomplot.com/) | Used in data visualization (charts) in dfgQtTableEditor. Versions 2.0.1 and 2.1.0 are known to work as of dfgQtTableEditor version 2.4.0.
| [Qt 5/6](https://www.qt.io/) | i (only for components in dfg/qt) | [Various](http://doc.qt.io/qt-5/licensing.html) | Have been build with 5.9, 5.12, 5.13, 5.15, 6.0, earliest version that works might be 5.8.
| [UTF8-CPP](https://github.com/nemtrif/utfcpp) (version 3.1 with [some edits](https://github.com/tc3t/dfglib/commits/master/dfg/utf/utf8_cpp)) | m (utf handling) | [Boost software license](dfg/utf/utf8_cpp/utf8.h) |

Usage types:
* c: All or some code from the library comes with dfglib (possibly modified), but is not directly used (i.e. related code can be removed without breaking any features in dfglib).
* i: Include dependency (i.e. some parts of dfglib may fail to compile if the 3rd party library is not available in include-path)
* m: Some code is integrated in dfglib itself possibly modified.
* oi: Like i, but optional component that is off by default.
* t: Used in test code without (external) include dependency (i.e. the needed code comes with dfglib).
* ti: Used in test code with include dependency.

## Build status of general unit tests (dfgTest) (as of 2022-10-08 commit [80f2c510](https://github.com/tc3t/dfglib/tree/80f2c5109399493d9904a1891d0e7ffd6a6fdce2), with Boost 1.70.0 unless stated otherwise)

<!-- [![Build status](https://ci.appveyor.com/api/projects/status/89v23h19mvv9k5u3/branch/master?svg=true)](https://ci.appveyor.com/project/tc3t/dfglib/branch/master) -->

| Compiler      | Platform      | Config  | Tests (passed/all) | Comment |
| ------------- | ------------- | -----   | ------  | ------- |
| Clang 6.0.0   | x86-64        | Release | 100 % (314/314) | Boost 1.65.1, Ubuntu 64-bit 18.04 |
| Clang 10.0.0  | x86-64        | Release | 100 % (314/314) | Boost 1.71.0, Ubuntu 64-bit 20.04 |
| Clang 12.0.0 (clang-cl, MSVC2019.11)   | x86-64           | Release | 100 % (321/321) | Needed manual definition of wmemchr to build; for details, see notes in commit message [c29dbe37](https://github.com/tc3t/dfglib/commit/c29dbe379615d65af663c95b659b68ea57ea9ca9)<br>Build with _/std:c++17_ |
| GCC 7.5.0     | x86-64        | Release | 100 % (314/314) | Boost 1.65.1, Ubuntu 64-bit 18.04 |
| GCC 9.4.0     | x86-64        | Release | 100 % (314/314) | Boost 1.71.0, Ubuntu 64-bit 20.04 |
| MinGW 7.3.0   | x86-64        | O2      | 100 % (321/321) | |
| VC2017.9      | x86           | Release | 99 % (320/321) | Numerical precision related failure in dfgNumeric.transform |
| VC2017.9      | x86-64        | Release | 99 % (320/321) | Numerical precision related failure in dfgNumeric.transform |
| VC2019.11     | x86           | Release | 100 % (321/321) | std:c++17 with Conformance mode |
| VC2019.11     | x86-64        | Release | 100 % (321/321) | std:c++17 with Conformance mode |
| VC2022.3      | x86           | Release | 100 % (321/321) | std:c++20 with Conformance mode |
| VC2022.3      | x86-64        | Release | 100 % (321/321) | std:c++20 with Conformance mode |
||||||

<br>
<br>

## Build status of Qt unit tests (dfgTestQt) (as of 2022-10-08 commit [80f2c510](https://github.com/tc3t/dfglib/tree/80f2c5109399493d9904a1891d0e7ffd6a6fdce2), with Boost 1.70.0 unless stated otherwise)

| Compiler      | Qt     | Platform   | Config   | Tests (passed/all) | Comment |
| ------------- | ------ | ---------- | -------- | ------------------ | ------- |
| Clang 6.0.0 (libstdc++)  | 5.9    | x86-64     | Release  | 98 % (44/45) | Boost 1.65.1, Ubuntu 64-bit 18.04. Test case dfgQt.CsvTableView_saveAsShown crashed with stack trace operator+(QString const&, char const*) () <- dfg::qt::CsvItemModel::getConfig() const () <- dfg::qt::CsvItemModel::readData(dfg::qt::CsvItemModel::LoadOptions const&, std::function<bool ()>) () <- dfg::qt::CsvItemModel::openFile. This was isolated to access of m_sFilePath member in CsvItemModel::getFilePath(). Did not occur in debug-build and some extra qDebug-logging could fix the crash also in release build. |
| Clang 10.0.0 (libc++)  | 5.12  | x86-64      | Release | 100 % (46/46) | Boost 1.71.0, Ubuntu 64-bit 20.04, QMAKESPEC = _linux-clang-libc++_ |
| Clang 10.0.0 (libstdc++)  | 5.12  | x86-64      | Release | 98 % (45/46) | Boost 1.71.0, Ubuntu 64-bit 20.04, QMAKESPEC = _linux-clang_. Test case dfgQt.CsvTableView_saveAsShown crashed like in Clang 6.0.0 case mentioned above. |
| Clang 12.0.0 (clang-cl, MSVC2019.11)  | 5.13   | x86-64        | Release | 100 % (46/46) | |
| GCC 7.5.0     | 5.9   | x86-64      | Release | 100 % (45/45) | Boost 1.65.1, Ubuntu 64-bit 18.04 |
| GCC 9.4.0     | 5.12  | x86-64      | Release | 100 % (46/46) | Boost 1.71.0, Ubuntu 64-bit 20.04 |
| GCC 9.4.0     | 6.0   | x86-64      | Release | 100 % (46/46) | Boost 1.71.0, Ubuntu 64-bit 20.04 |
| MinGW 7.3.0   | 5.13  | x86-64      | Release | 100 % (46/46) | |
| VC2017.9      | 5.9   | x86-64      | Release | 100 % (45/45) | |
| VC2017.9      | 5.12  | x86-64      | Release | 100 % (46/46) | |
| VC2019.11     | 5.15  | x86-64      | Release | 100 % (46/46) | |
| VC2019.11     | 6.0   | x86-64      | Release | 100 % (46/46) | |
| VC2022.3      | 6.0   | x86-64      | Release | 100 % (46/46) | |
||||||

Note: dfgQt.CsvTableView_paste may fail sporadically at least on Windows.

<br>

### Phased out compilers:

| Compiler | Untested since | Support branch |
| -------- | --------------- | --------------------- |
| MSVC2015 | 2022-10-30 | [legacy_msvc_2015_GCC_5.4](https://github.com/tc3t/dfglib/tree/legacy_msvc_2015_GCC_5.4) |
| GCC 5.4  | 2022-10-30 | [legacy_msvc_2015_GCC_5.4](https://github.com/tc3t/dfglib/tree/legacy_msvc_2015_GCC_5.4) |
| Clang 3.8.0  | 2022-10-30 | [legacy_msvc_2015_GCC_5.4](https://github.com/tc3t/dfglib/tree/legacy_msvc_2015_GCC_5.4) |
| MSVC2013 | 2020-02-11 | [legacy_msvc_2010_to_2013](https://github.com/tc3t/dfglib/tree/legacy_msvc_2010_to_2013) |
| MSVC2012 | 2020-02-11 | [legacy_msvc_2010_to_2013](https://github.com/tc3t/dfglib/tree/legacy_msvc_2010_to_2013) |
| MSVC2010 | 2020-02-11 | [legacy_msvc_2010_to_2013](https://github.com/tc3t/dfglib/tree/legacy_msvc_2010_to_2013) |
| MinGW 4.8.0 | 2020-02-11 | [legacy_msvc_2010_to_2013](https://github.com/tc3t/dfglib/tree/legacy_msvc_2010_to_2013) |

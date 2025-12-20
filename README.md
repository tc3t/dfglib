# dfglib

Experimental general purpose utility library for C++ (at least C++17 required), which has become mostly a backbone for csv-oriented table editor, [dfgQtTableEditor](dfgExamples/dfgQtTableEditor/README.md/)

Note: this is *not* a mature library and is not intended or recommended for general use. Libraries such as [Abseil](https://abseil.io/) or [Boost](https://www.boost.org/) provide many of the features in dfglib implemented in a more professional manner. For a comprehensive list of alternatives, see [A list of open source C++ libraries at cppreference.com](https://en.cppreference.com/w/cpp/links/libs) 

## News

* 2024-12-15, dfgQtTableEditor [version 2.7.0](dfgExamples/dfgQtTableEditor/README.md/#270-2024-12-15) (Statistical box -graphs, UTF-8 as default read encoding, default graph line colour to black...)
* 2023-12-30: Code is no longer tested with GCC 7.5 and Clang 6.0.0, and for Qt-module, 5.12 is now considered minimum. Latest tested commit available in branch: [legacy_GCC_7.5_Clang_6.0_Qt_5.9](https://github.com/tc3t/dfglib/tree/legacy_GCC_7.5_Clang_6.0_Qt_5.9)
* 2023-12-17, dfgQtTableEditor [version 2.6.0](dfgExamples/dfgQtTableEditor/README.md/#260-2023-12-17) (Column filter from header context menu, CsvTableView regexFormat...)
* 2023-04-26, dfgQtTableEditor [version 2.5.0](dfgExamples/dfgQtTableEditor/README.md/#250-2023-04-26) (Filter from selection, major performance improvements to charts...)
* 2022-10-30: In [481baa71](https://github.com/tc3t/dfglib/commit/481baa717a0cc90fb55813de787ba68fb98b1515) the minimum standard requirement in dfglib changed from C++11 to C++17 and code is no longer tested with MSVC2015, GCC 5.4 and Clang 3.8.0, latest tested commit available as branch: [legacy_msvc_2015_GCC_5.4](https://github.com/tc3t/dfglib/tree/legacy_msvc_2015_GCC_5.4)
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

* CSV-file [reading/writing](dfg/io/) with [somewhat reasonable performance](misc/csvPerformanceRuns.md) (e.g. compared to some spreadsheet applications) and [Qt-based widgets](dfg/qt/) for editing CSV-files.

* CSV-editor as an example, for details see it's [readme](dfgExamples/dfgQtTableEditor/README.md/)

* Typed strings (i.e. strings that store encoded text instead of raw bytes)

* Algorithms such as
    * [median](dfg/numeric/median.hpp) & [percentile](dfg/numeric/percentile.hpp)
    * [polynomial evaluation](dfg/math/evalPolynomial.hpp) (Horner's method)
    * [spearman correlation](dfg/dataAnalysis/correlation.hpp)
    * [spreadsheet sorting](dfg/alg/sortMultiple.hpp)

* Containers such as [flat map](dfg/cont/MapVector.hpp) and [flat set](dfg/cont/SetVector.hpp)

## Third party code

Summary of 3rd party code in dfglib (last revised 2024-12-15).

| Library      | Usage      | License  | Comment |
| ------------- | ------------- | ----- | ------- |
| [Boost](http://www.boost.org/)  | i,m,ti (used in numerous places)          | [Boost software license](http://www.boost.org/LICENSE_1_0.txt) | Exact requirement for Boost version is unknown; unit tests have been successfully build and run with Boost versions 1.70.0, 1.71.0, 1.74.0 and 1.83.0 |
| [Colour Rendering of Spectra](dfg/colour/specRendJw.cpp) | m (used in colour handling tools) | [Public domain](dfg/colour/specRendJw.cpp) | 
| [cppcsv](https://github.com/paulharris/cppcsv) (commit [daa3d881](https://github.com/paulharris/cppcsv/tree/daa3d881d995b6695b84dde860126fa5f773de56/include/cppcsv), 2020-01-10) | c,t | [MIT](https://github.com/paulharris/cppcsv) | 
| [dlib](http://dlib.net/)    | m (unit-aware integration) | [Boost software license](http://www.boost.org/LICENSE_1_0.txt)  |
| [fast-csv-cpp-parser](https://github.com/ben-strasser/fast-cpp-csv-parser/) (commit [66365610](https://github.com/ben-strasser/fast-cpp-csv-parser/blob/66365610817b929b451819e0cccdb702d46a605e/csv.h), 2020-05-19) | c,t | [BSD-3](dfg/io/fast-cpp-csv-parser/csv.h) |
| [fmtlib](https://github.com/fmtlib/fmt) (version 4.1.0 with an adjustment related to formatting doubles as string) | m (string formatting)| [BSD-2](dfg/str/fmtlib/format.h) |
| [GoogleTest](https://github.com/google/googletest) (version 1.11.0) | t | [BSD-3](externals/gtest/gtest.h) | Unit test framework used in both dfgTest and dfgTestQt.
| [LibQxt](https://bitbucket.org/libqxt/libqxt/wiki/Home) | c,t (QxtSpanSlider) | [BSD-3](dfg/qt/qxt/core/qxtglobal.h) | Qt-related utilities
| [muparser](https://github.com/beltoforion/muparser) (version 2.3.3, commit [5ccc8878](https://github.com/beltoforion/muparser/tree/5ccc887864381aeacf1277062fcbb76475623a02), 2022-01-13) with some edits | m (math::FormulaParser) | [BSD-2](dfg/math/muparser/muParser.h) | Formula parser. Namespace of the code has been edited from _mu_ to _dfg_mu_.
| [QCustomPlot](https://www.qcustomplot.com/) | oi (in parts of dfg/qt) | [GPLv3/commercial](https://www.qcustomplot.com/) | Used in data visualization (charts) in dfgQtTableEditor. Versions 2.0.1 and 2.1.1 are known to work as of dfgQtTableEditor version 2.7.0.
| [Qt 5/6](https://www.qt.io/) | i (only for components in dfg/qt) | [Various](http://doc.qt.io/qt-5/licensing.html) | Unit tests (dfgTestQt) have been build with various versions 5.12 - 6.4. For details, see dfgTestQt status table.
| [UTF8-CPP](https://github.com/nemtrif/utfcpp) ([version 3.2.5](https://github.com/nemtrif/utfcpp/releases/tag/v3.2.5) with [some adjustments](https://github.com/tc3t/dfglib/commit/b8e948d6430a74c0efd186f520853c0585fec11a)) | m (utf handling) | [Boost software license](dfg/utf/utf8_cpp/utf8.h) |

Usage types:
* c: All or some code from the library comes with dfglib (possibly modified), but is not directly used (i.e. related code can be removed without breaking any features in dfglib).
* i: Include dependency (i.e. some parts of dfglib may fail to compile if the 3rd party library is not available in include-path)
* m: Some code is integrated in dfglib itself possibly modified.
* oi: Like i, but optional component that is off by default.
* t: Used in test code without (external) include dependency (i.e. the needed code comes with dfglib).
* ti: Used in test code with include dependency.

## Build status of general unit tests (dfgTest) (as of 2025-12-14 commit [69525298](https://github.com/tc3t/dfglib/tree/695252987883854eb74cab07a153a66e84bf2b99))

<!-- [![Build status](https://ci.appveyor.com/api/projects/status/89v23h19mvv9k5u3/branch/master?svg=true)](https://ci.appveyor.com/project/tc3t/dfglib/branch/master) -->

<!-- Table generated from buildStatus_dfgTest.csv excluding date and commit columns
     with csv2md (https://www.npmjs.com/package/csv2md)
 -->
Compiler | Standard library | C++ standard [1] | Platform | Boost | Tests (passed/all) | Comment
---|---|---|---|---|---|---
Clang 10.0.0 | libstdc++ 9 | C++17 | x86-64 | 1.71.0 | 100 % (342/342) | Ubuntu 64-bit 20.04, WSL
Clang 14.0.0 | libstdc++ 11 | C++17 | x86-64 | 1.74.0 | 100 % (342/342) | Ubuntu 64-bit 22.04, WSL
Clang 14.0.0 | libc++ 14000 | C++17 | x86-64 | 1.74.0 | 100 % (342/342) | Ubuntu 64-bit 22.04, WSL
Clang 18.1.3 | libc++ 180100 | C++20 | x86-64 | 1.83.0 | 100 % (342/342) | Ubuntu 64-bit 24.04, WSL
clang-cl (Clang 20.1.8, MSVC2026.0) | MSVC | C++17 | x86-64 | 1.70.0 | 100 % (349/349) | If building with Clang < 13, see [2]
GCC 9.4.0 | libstdc++ 9 | C++17 | x86-64 | 1.71.0 | 100 % (342/342) | Ubuntu 64-bit 20.04, WSL
GCC 11.4.0 | libstdc++ 11 | C++17 | x86-64 | 1.74.0 | 100 % (342/342) | Ubuntu 64-bit 22.04, WSL
GCC 13.3.0 | libstdc++ 13 | C++17 | x86-64 | 1.83.0 | 100 % (342/342) | Ubuntu 64-bit 24.04, WSL
GCC 13.3.0 | libstdc++ 13 | C++20 | x86-64 | 1.83.0 | 100 % (342/342) | Ubuntu 64-bit 24.04, WSL
MinGW 11.2.0 | libstdc++ 11 | C++17 | x86-64 | 1.70.0 | 100 % (349/349) | 
VC2017.9 | MSVC | C++17 | x86-64 | 1.70.0 | 99 % (348/349) | Numerical precision related failure in dfgNumeric.transform
VC2019.11 | MSVC | C++17 | x86-64 | 1.70.0 | 100 % (349/349) | std:c++17 with Conformance mode
VC2022.14 | MSVC | C++20 | x86 | 1.70.0 | 100 % (349/349) | std:c++20 with Conformance mode
VC2022.14 | MSVC | C++20 | x86-64 | 1.70.0 | 100 % (349/349) | std:c++20 with Conformance mode
VC2026.0 | MSVC | C++23 | x86-64 | 1.70.0 | 100 % (349/349) | std:c++23 with Conformance mode

[1] As reported by *__cplusplus* macro  or equivalent.<br>
[2] If building with Clang < 13, need manual definition of wmemchr to build; for details, see notes in commit message [c29dbe37](https://github.com/tc3t/dfglib/commit/c29dbe379615d65af663c95b659b68ea57ea9ca9). According to related [MSVC bug ticket](https://developercommunity.visualstudio.com/t/undefined-symbol-wmemchr-in-1660-preview-60-using/1024640#TPIN-N1570352), the "issue belongs to clang and it was fixed in Clang 13"

<br>
<br>

## Build status of Qt unit tests (dfgTestQt) (as of 2025-12-14 commit [69525298](https://github.com/tc3t/dfglib/tree/695252987883854eb74cab07a153a66e84bf2b99))

<!-- Table generated from buildStatus_dfgTestQt.csv excluding commit and date columns
     with csv2md (https://www.npmjs.com/package/csv2md)
 -->


Status | Compiler | Standard library | C++ standard [1] | Qt | Platform | Boost | Tests (passed/all) | Comment
---|---|---|---|---|---|---|---|---
:white_check_mark: | Clang 14.0.0 | libstdc++ 11 | C++17 | 6.2.4 | x86-64 | 1.74.0 | 100 % (63/63) | Ubuntu 64-bit 22.04
:red_circle: | Clang 14.0.0 | libc++ 14000 | C++17 | 6.2.4 | x86-64 | 1.74.0 | N/A | Ubuntu 64-bit 22.04. Causes linker errors from QMetaType. For details, see error messages in a [separate file](misc/dfgTestQt_Clang14_libc++_Qt_624_linker_errors.txt)
:white_check_mark: | clang-cl (Clang 19.1.5, MSVC2022.5) | MSVC | C++17 | 6.4.1 | x86-64 | 1.70.0 | 100 % (65/65) | 
:white_check_mark: | GCC 9.4.0 | libstdc++ 9 | C++17 | 5.12.8 | x86-64 | 1.71.0 | 100 % (64/64) | Ubuntu 64-bit 20.04, WSL
:white_check_mark: | GCC 11.4.0 | libstdc++ 11 | C++17 | 6.2.4 | x86-64 | 1.74.0 | 100 % (64/64) | Ubuntu 64-bit 22.04, VM
:white_check_mark: | GCC 13.3.0 | libstdc++ 13 | C++17 | 6.4.2 | x86-64 | 1.18.3 | 100 % (65/65) | Ubuntu 64-bit 24.04, WSL
:white_check_mark: | MinGW 11.2.0 | libstdc++ 11 | C++17 | 6.4.1 | x86-64 | 1.70.0 | 100 % (64/64) | 
:white_check_mark: | VC2019.11 | MSVC | C++17 | 5.15.2 | x86-64 | 1.70.0 | 100 % (65/65) | 
:white_check_mark: | VC2022.14 | MSVC | C++17 | 6.4.1 | x86-64 | 1.70.0 | 100 % (65/65) | 
:white_check_mark: | VC2022.14 | MSVC | C++20 | 6.4.1 | x86-64 | 1.70.0 | 100 % (65/65) | 
:white_check_mark: | VC2022.14 | MSVC | C++20 | 6.9.1 | x86-64 | 1.70.0 | 100 % (65/65) | 

[1] As reported by *__cplusplus* macro  or equivalent.<br>
Note: dfgQt.CsvTableView_paste has been experienced to fail on Windows if VirtualBox is simultaneously running a virtual machine; Qt logs "Unable to obtain clipboard" during the test case.

<br>

### Phased out compilers/libraries:

| Compiler/library | Untested since | Support branch |
| -------- | --------------- | --------------------- |
| GCC 7.5 | 2023-12-30 | [legacy_GCC_7.5_Clang_6.0_Qt_5.9](https://github.com/tc3t/dfglib/tree/legacy_GCC_7.5_Clang_6.0_Qt_5.9) |
| Clang 6.0.0 | 2023-12-30 | [legacy_GCC_7.5_Clang_6.0_Qt_5.9](https://github.com/tc3t/dfglib/tree/legacy_GCC_7.5_Clang_6.0_Qt_5.9) |
| Qt 5.9 | 2023-12-30 | [legacy_GCC_7.5_Clang_6.0_Qt_5.9](https://github.com/tc3t/dfglib/tree/legacy_GCC_7.5_Clang_6.0_Qt_5.9) |
| MSVC2015 | 2022-10-30 | [legacy_msvc_2015_GCC_5.4](https://github.com/tc3t/dfglib/tree/legacy_msvc_2015_GCC_5.4) |
| GCC 5.4  | 2022-10-30 | [legacy_msvc_2015_GCC_5.4](https://github.com/tc3t/dfglib/tree/legacy_msvc_2015_GCC_5.4) |
| Clang 3.8.0  | 2022-10-30 | [legacy_msvc_2015_GCC_5.4](https://github.com/tc3t/dfglib/tree/legacy_msvc_2015_GCC_5.4) |
| MSVC2013 | 2020-02-11 | [legacy_msvc_2010_to_2013](https://github.com/tc3t/dfglib/tree/legacy_msvc_2010_to_2013) |
| MSVC2012 | 2020-02-11 | [legacy_msvc_2010_to_2013](https://github.com/tc3t/dfglib/tree/legacy_msvc_2010_to_2013) |
| MSVC2010 | 2020-02-11 | [legacy_msvc_2010_to_2013](https://github.com/tc3t/dfglib/tree/legacy_msvc_2010_to_2013) |
| MinGW 4.8.0 | 2020-02-11 | [legacy_msvc_2010_to_2013](https://github.com/tc3t/dfglib/tree/legacy_msvc_2010_to_2013) |

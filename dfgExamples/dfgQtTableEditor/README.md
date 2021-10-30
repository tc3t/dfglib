# dfgQtTableEditor

Example application for viewing and editing csv-files demonstrating features in dfglib. Also includes ability to open SQLite3-files and optionally visualization.

## Building

### __Version 2.1.0__:

Has been successfully build with:
| Compiler      | Qt | OS | Boost | Charting? (using QCustomPlot) | Notes |
| ------------- | ----- | ---- | -- | -- | -- |
| Clang 6.0.0 | 5.9 | Ubuntu 18.04 | 1.65.1 | No | QMAKESPEC = _linux-clang_ |
| Clang 10.0.0 | 5.12 | Ubuntu 20.04 | 1.71 | Yes | QMAKESPEC = _linux-clang_ or _linux-clang-libc++_, QCustomPlot 2.0.1
| GCC 7.5.0 | 5.9 | Ubuntu 18.04 | 1.65.1 | No |  |
| GCC 9.3.0 | 5.12 | Ubuntu 20.04 | 1.71 | Yes | QCustomPlot 2.0.1 |
| GCC 9.3.0 | 6.0 | Ubuntu 20.04 | 1.71 | No |
| MinGW 7.3.0 | 5.13 | Windows 8.1 | 1.70 | Yes | QCustomPlot 2.1.0
| MSVC2017 | 5.9/5.12/5.13 | Windows 8.1 | 1.70 | Yes | QCustomPlot 2.1.0
| MSVC2019 | 6.0 | Windows 8.1 | 1.70 | Yes | QCustomPlot 2.1.0
| | | |

Also expected to build directly or with little changes on MSVC2015; minimum Qt version is 5.8/5.9. Note that while building with Qt 6 is possible, the application itself is untested with Qt 6 so there may be some rough edges.

If building with QCustomPlot charting, histogram chart type requires Boost >= 1.70. For concrete build steps, see section for version 1.0.0.

### __Version 2.0.0__:

Has been successfully build with:
| Compiler      | Qt | OS | Boost | Charting? (using QCustomPlot) | Notes |
| ------------- | ----- | ---- | -- | -- | -- |
| Clang 6.0.0 | 5.9 | Ubuntu 18.04 | 1.65.1 | No | QMAKESPEC = _linux-clang_ |
| Clang 10.0.0 | 5.12 | Ubuntu 20.04 | 1.71 | Yes | QMAKESPEC = _linux-clang_ or _linux-clang-libc++_, QCustomPlot 2.0.1
| GCC 7.5.0 | 5.9 | Ubuntu 18.04 | 1.65.1 | No |  |
| GCC 9.3.0 | 5.12 | Ubuntu 20.04 | 1.71 | Yes | QCustomPlot 2.0.1 |
| GCC 9.3.0 | 6.0 | Ubuntu 20.04 | 1.71 | No |
| MinGW 7.3.0 | 5.13 | Windows 8.1 | 1.70 | Yes | QCustomPlot 2.1.0
| MSVC2017 | 5.9/5.12/5.13 | Windows 8.1 | 1.70 | Yes | QCustomPlot 2.1.0
| MSVC2019 | 6.0 | Windows 8.1 | 1.70 | Yes | QCustomPlot 2.1.0
| | | |

Also expected to build directly or with little changes on MSVC2015; minimum Qt version is 5.8/5.9. Note that while building with Qt 6 is possible, the application itself is untested with Qt 6 so there may be some rough edges.

 If building with QCustomPlot charting, histogram chart type requires Boost >= 1.70. For concrete build steps, see section for version 1.0.0.

### __Version 1.9.0__:

Has been successfully build with:
| Compiler      | Qt | OS | Boost | Charting? (using QCustomPlot) | Notes |
| ------------- | ----- | ---- | -- | -- | -- |
| Clang 6.0.0 | 5.9 | Ubuntu 18.04 | 1.65.1 | No | QMAKESPEC = _linux-clang_ |
| Clang 10.0.0 | 5.12 | Ubuntu 20.04 | 1.71 | Yes | QMAKESPEC = _linux-clang_ or _linux-clang-libc++_, QCustomPlot 2.0.1
| GCC 7.5.0 | 5.9 | Ubuntu 18.04 | 1.65.1 | No |  |
| GCC 9.3.0 | 5.12 | Ubuntu 20.04 | 1.71 | Yes | QCustomPlot 2.0.1 |
| GCC 9.3.0 | 6.0 | Ubuntu 20.04 | 1.71 | No |
| MinGW 7.3.0 | 5.13 | Windows 8.1 | 1.70 | Yes | QCustomPlot 2.1.0
| MSVC2017, MSVC2019 | 5.9/5.12/5.13 | Windows 8.1 | 1.70 | Yes | QCustomPlot 2.1.0
| MSVC2019 | 6.0 | Windows 8.1 | 1.70 | Yes | QCustomPlot 2.1.0
| | | |

Also expected to build directly or with little changes on MSVC2015; minimum Qt version is somewhere around 5.6 and 5.9. Note that while building with Qt 6 is possible, the application itself is untested with Qt 6 so there may be some rough edges.

 If building with QCustomPlot charting, requires Boost >= 1.70 (or that uses of boost::histogram are commented out). For concrete build steps, see section for version 1.0.0.

### __Version 1.8.0__:

Has been successfully build with:
| Compiler      | Qt | OS | Boost | Charting? | Comments |
| ------------- | ----- | ---- | -- | -- | -- |
| Clang 6.0.0 | 5.9.5 | Ubuntu 18.04 | 1.65.1 | No | QMAKESPEC = _linux-clang_ |
| Clang 10.0.0 | 5.12.8 | Ubuntu 20.04 | 1.71 | Yes | QMAKESPEC = _linux-clang_ or _linux-clang-libc++_
| GCC 7.5.0 | 5.9.5 | Ubuntu 18.04 | 1.65.1 | No |  |
| GCC 9.3.0 | 5.12.8 | Ubuntu 20.04 | 1.71 | Yes |
| GCC 9.3.0 | 6.0 | Ubuntu 20.04 | 1.71 | No |
| MinGW 7.3.0 | 5.13 | Windows 8.1 | 1.70 | Yes |
| MSVC2017, MSVC2019 | 5.9/5.12/5.13 | Windows 8.1 | 1.70 | Yes |
| MSVC2019 | 6.0 | Windows 8.1 | 1.70 | Yes (*) | * With adjusted version of QCustomPlot 2.0.1 to allow building with Qt 6.0
| | | |

Also expected to build with MSVC2015; might also build with Qt versions before 5.9, but not earlier than 5.6. Note that while building with Qt 6 is possible, the application itself is untested with Qt 6 so there may be some rough edges.

 If building with QCustomPlot charting, requires Boost >= 1.70 (or that uses of boost::histogram are commented out). For concrete build steps, see section for version 1.0.0.

### __Version 1.7.0__:

Has been successfully build with:
| Compiler      | Qt | OS | Boost | Charting? | Comments |
| ------------- | ----- | ---- | -- | -- | -- |
| Clang 6.0.0 | 5.9.5 | Ubuntu 18.04 | 1.65.1 | No | QMAKESPEC = _linux-clang_ |
| Clang 10.0.0 | 5.12.8 | Ubuntu 20.04 | 1.71 | Yes | QMAKESPEC = _linux-clang_ or _linux-clang-libc++_
| GCC 7.5.0 | 5.9.5 | Ubuntu 18.04 | 1.65.1 | No |  |
| GCC 9.3.0 | 5.12.8 | Ubuntu 20.04 | 1.71 | Yes |
| MinGW 7.3.0 | 5.13 | Windows 8.1 | 1.70 | Yes |
| MSVC2017, MSVC2019 | 5.12/5.13 | Windows 8.1 | 1.70 | Yes |
| | | |

### __Version 1.6.0__:

Known to build with:
* Clang 6.0.0 with Qt 5.9.5 (Ubuntu 18.04)
* GCC 7.5.0 with Qt 5.9.5 (Ubuntu 18.04)
* MinGW 7.3.0 with Qt 5.13 (Windows 8.1)
* MSVC2017, MSVC2019 with Qt 5.12/5.13 (Windows 8.1). Expected to build also with MSVC2015.

If building with QCustomPlot charting, requires Boost >= 1.70 (or that uses of boost::histogram are commented out)

See detailed instructions from section of Version 1.0.0

### __Version 1.5.0__:

Like earlier versions with following changes:
* Building with MSVC 2012 and 2013 is no longer supported.
* Qt 5.6 is minimum Qt version
* If building with QCustomPlot charting, requires Boost >= 1.70

### __Version 1.1.0__:

Like with version 1.0.0, but won't anymore build with MSVC 2010.

### __Version 1.0.0 or earlier__:
Requires basic C++11 support (as available since VC2010), Qt 5 and Boost (in include path). Some source packages may be available in [dfglib releases](https://github.com/tc3t/dfglib/releases).

To build (assuming having compatible Qt version, Qt Creator and compiler available):
* Open dfgQtTableEditor.pro in Qt Creator
* Choose suitable Qt kit to use
* Click build

Has been build with the following setups:

* MSVC 2010, Qt 5.2.1, Boost 1.70, 32-bit (Windows 8.1)
* MSVC 2012, Qt 5.2.1, Boost 1.70, 64-bit (Windows 8.1)
* MSVC 2013, Qt 5.4.2, Boost 1.70, 64-bit (Windows 8.1)
* MSVC 2015 update 3, Qt 5.8, Boost 1.70, 64-bit (Windows 8.1)
* MSVC 2017.9, Qt 5.9.8, Boost 1.70, 64-bit (Windows 8.1)
* MSVC 2017.9, Qt 5.11.2, Boost 1.70, 64-bit (Windows 8.1)
* MSVC 2017.9, Qt 5.12, Boost 1.70, 64-bit (Windows 8.1)
* MSVC 2017.9, Qt 5.13, Boost 1.70, 64-bit (Windows 8.1)
* GCC 5.4.0, Qt 5.5.1, Boost 1.61, 32-bit (Ubuntu 16.04.6, 32-bit)
* GCC 7.4.0, Qt 5.9.5, Boost 1.65.1, 64-bit (Ubuntu 18.04.2, 64-bit)
* Clang 3.8.0, Qt 5.5.1, Boost 1.61, 32-bit (Ubuntu 16.04.6, 32-bit)
* Clang 6.0.0, Qt 5.9.5, Boost 1.65.1, 64-bit (Ubuntu 18.04.2, 64-bit)
* (with tweaks done 2019-09-29 in commit [c6cef13](https://github.com/tc3t/dfglib/commit/c6cef13711274fa37e6294ff34cfcc2855cfaffa)): MinGW 7.3.0, Qt 5.13.1, Boost 1.70, 64-bit (Windows 8.1)

Note that in Qt versions 5.10-5.12.3, keyboard shortcuts won't show as intended in context menu (for further details, see [QTBUG-61181](https://bugreports.qt.io/browse/QTBUG-61181), [QTBUG-71471](https://bugreports.qt.io/browse/QTBUG-71471)).

## Version history

### 2.1.0, 2021-09-09
* Tag: [2.1.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_2.1.0)
    * General
        * [new] Multicolumn filtering. Filter is defined by list of json-objects with the same syntax as in read filter ([#25](https://github.com/tc3t/dfglib/issues/25))
        * [new] read-only mode ([#5](https://github.com/tc3t/dfglib/issues/5))
        * [new] 'Save visible'-functionality, i.e. can save table as shown, which can be useful when content is filtered or sorted. Available from "Save to file with options"-menu item ([#20](https://github.com/tc3t/dfglib/issues/20), [94827592](https://github.com/tc3t/dfglib/commit/94827592ae1770ffc336eda3224e2a3b1adde611))
        * [imp] There are now new selection details and can choose which to calculate. New details: isSorted, median, standard deviation (both population and variance), variance  ([#39](https://github.com/tc3t/dfglib/issues/39))
        * [imp] Can now optionally choose which items to save to .conf-file ([#94](https://github.com/tc3t/dfglib/issues/94), [00a2d3db](https://github.com/tc3t/dfglib/commit/00a2d3dbbb08a6eed7ea257b0eeca9a087d26c1f))
        * [mod/imp] Find/filter column selector index now starts from 1. 'Any column' is now shown as "Any" and tooltip shows column name. [945e86fb](https://github.com/tc3t/dfglib/commit/945e86fb2aedbe8a5e7973cfd5046e755a94854e))
        * [fix] When editing content from 'Cell edit', cursor could sporadically reset to beginning especially when editing triggered a lengthy chart update ([#110](https://github.com/tc3t/dfglib/issues/110), [7a6aba9d](https://github.com/tc3t/dfglib/commit/7a6aba9d28644bf0a6692dc931cea921a48fa8d8))
        * [fix] Undo/redo could crash if used while table was being accessed by some operation ([a483af44](https://github.com/tc3t/dfglib/commit/a483af4438e68868849faa0b8642c0e5f2f02257))
        * [fix] Inserting date/time could cause a crash if used while table was being accessed by some operation ([1a9c4b3b](https://github.com/tc3t/dfglib/commit/1a9c4b3bb4201513f219aba20dc79b9bcddc64db))
        * [fix] Various minor fixes and changes ([574d7b77](https://github.com/tc3t/dfglib/commit/574d7b772249ad1e51dd250c120244fec0383792), [df06a2df](https://github.com/tc3t/dfglib/commit/df06a2df3e04fb8baa6007c34192dc312ef98ec3), [73b953a0](https://github.com/tc3t/dfglib/commit/73b953a00f0fa9f6c831edeef2a99e97f3aa0b32), [cfe0b1e6](https://github.com/tc3t/dfglib/commit/cfe0b1e6df769e6e37e3c40af2b87216e66c0be3))
    * Charts
        * [imp] Improve tooltip formatting for stacked bars ([#88](https://github.com/tc3t/dfglib/issues/88))
        * [imp] Context menu of chart definition text edit now has quick insert items ([#95](https://github.com/tc3t/dfglib/issues/95), [a1cad828](https://github.com/tc3t/dfglib/commit/a1cad828dcce6fce29c1bd0b9692ca01e78baddc))
        * [imp] Chart preparation can now be terminated at entry boundary from 'Apply'-button. This means that ongoing preparation of an entry  can't be terminated, but preparation will stop right after it finishes. ([#80](https://github.com/tc3t/dfglib/issues/80), [98042b91](https://github.com/tc3t/dfglib/commit/98042b91c64ba95692ea59e5c0046a7572e9520f))
        * [fix] Chart grid didn't always reset to smaller size, e.g. didn't reset to size 1x1 after being 2x2 ([#109](https://github.com/tc3t/dfglib/issues/109), [288ff796](https://github.com/tc3t/dfglib/commit/288ff79606ed50c3d25dfbb83f7d3b84865e3fdf))


### 2.0.0, 2021-06-24
* Tag: [2.0.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_2.0.0)
    * General
        * [imp] Can now define window position and size from .conf-file ([#86](https://github.com/tc3t/dfglib/issues/86), [3774b26e](https://github.com/tc3t/dfglib/commit/3774b26eadf9cb7ca1c9296c6015a7eb110e3ca1))
        * [imp] Find&replace: greatly improved performance and can now be cancelled ([#87](https://github.com/tc3t/dfglib/issues/87), [#83](https://github.com/tc3t/dfglib/issues/83))
        * [imp] Write failures are now shown to user ([#73](https://github.com/tc3t/dfglib/issues/73), [c648414c](https://github.com/tc3t/dfglib/commit/c648414cd5af83cae63c9af6049dc65fc3a90514))
    * Charts
        * [new] Can now have secondary y-axis ([#17](https://github.com/tc3t/dfglib/issues/17))
        * [imp] Chart guide window now has search functionality ([#72](https://github.com/tc3t/dfglib/issues/72), [00aa6cd0](https://github.com/tc3t/dfglib/commit/00aa6cd03b7cf05b670d7630e06672152fb71d7f))
        * [imp] Histogram y-values can now be modified by operations ([#85](https://github.com/tc3t/dfglib/issues/85))
        * [mod] Histogram x-values for operations has been changed from raw input values to bin positions ([#85](https://github.com/tc3t/dfglib/issues/85))
        * [imp] Formula operations now generate console errors on invalid syntax ([#59](https://github.com/tc3t/dfglib/issues/59), [2a3da99d](https://github.com/tc3t/dfglib/commit/2a3da99de89f59b2de14305151fa5afe35b6b834))
        * [imp] Increased font size in chart definition text field and added ability to toggle text wrapping and control font size ([#79](https://github.com/tc3t/dfglib/issues/79), [037a587b](https://github.com/tc3t/dfglib/commit/037a587b326376166a798625e72e997f40a59144))


### 1.9.0, 2021-04-29
* Tag: [1.9.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_1.9.0)
    * General
        * [new] Can now replace cell content by evaluated value, for example "1+2" -> "3". Available from context menu and shortcut Alt+C ([#71](https://github.com/tc3t/dfglib/issues/71), [4616e548](https://github.com/tc3t/dfglib/commit/4616e54863720319577ce11b638f1024b04763fc))
        * [new] Rudimentary Find&Replace ([#67](https://github.com/tc3t/dfglib/issues/67), [85d87e63](https://github.com/tc3t/dfglib/commit/85d87e638c45e3e8f461423241539de37974931e))
        * [new] Can now generate date/times ([#65](https://github.com/tc3t/dfglib/issues/65), [2e0b399b](https://github.com/tc3t/dfglib/commit/2e0b399bbf0da1685329496dd36d62b46a18bdcd))
        * [new] Content generator of type 'formula' now has more standard C++ math functions; when compiled with C++17, also has [mathematical special functions](https://en.cppreference.com/w/cpp/numeric/special_functions) ([#56](https://github.com/tc3t/dfglib/issues/56), [07c85df4](https://github.com/tc3t/dfglib/commit/07c85df4dcec7545de7a72d866e8ad11cadf4f2d))
        * [imp] File opening can now be cancelled. ([#21](https://github.com/tc3t/dfglib/issues/21))
        * [imp] Clearing existing huge tables should now take less time on Windows ([#74](https://github.com/tc3t/dfglib/issues/74), [c8af1730](https://github.com/tc3t/dfglib/commit/c8af17300203c4407d8c200ee870167625358976))
    * Charts
        * [new] Number generator source: can now create charts without any table data input. Usage example can be found from [examples](https://github.com/tc3t/dfglib/blob/master/dfgExamples/dfgQtTableEditor/examples/number_generator_example.csv.conf) ([#61](https://github.com/tc3t/dfglib/issues/61))
        * [new] Can now stack bars  ([#42](https://github.com/tc3t/dfglib/issues/21))
        * [imp] Chart panel size can now be defined either globally through .ini file or per-file through .conf-file ([#60](https://github.com/tc3t/dfglib/issues/60), [1c7a662c](https://github.com/tc3t/dfglib/commit/1c7a662c3766a458891c88dabd5ff884a8899998), [d2df617c](https://github.com/tc3t/dfglib/commit/d2df617c91dac90afed2cc4462b9e7781a6181cc))
        * [fix] NaN handling fixes to smoothing ([#70](https://github.com/tc3t/dfglib/issues/70), [633f97db](https://github.com/tc3t/dfglib/commit/633f97db22e5dcd5fa998a3b7b74727b7f545ce7))


### 1.8.0, 2021-02-21
* Tag: [1.8.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_1.8.0)
    * General
        * [breaking_change] Column indexing now starts from 1. This breaks existing property files etc., where column indexes was 0-based ([#64](https://github.com/tc3t/dfglib/issues/64), [9b0b3ef7](https://github.com/tc3t/dfglib/commit/9b0b3ef7242d901e8e3a032912300f7a820d4de4))
        * [breaking_change] Table filter is no longer available when building with Qt version < 5.12 and filter types *WildcardUnix*, *RegExp2* and *W3CXmlSchema11* are no longer available. ([16b0f6bb](https://github.com/tc3t/dfglib/commit/16b0f6bbad1fdd311e9d64318e5b5362e70061f3), [f30ba001](https://github.com/tc3t/dfglib/commit/f30ba00102ec64e2afe040dda95f3ef74385283d))
        * [new] Content generation can now be done with formula; includes ability to use values of other cells. ([#50](https://github.com/tc3t/dfglib/issues/50), [0b1302c4](https://github.com/tc3t/dfglib/commit/0b1302c4e68a3fbb370b54c0e2342a3f157fe27e))
        * [imp] There are now shortcuts for 'Resize table' (Ctrl+R)  and 'Generate content' (Alt+G) ([e3ef65b2](https://github.com/tc3t/dfglib/commit/e3ef65b2e54781ba65dc044876a536c19c62720f))
        * [imp] Reading files that can't be memory mapped (e.g. very big files) might now work; not expected to be fast, though. ([#37](https://github.com/tc3t/dfglib/issues/37), [aa38151b](https://github.com/tc3t/dfglib/commit/aa38151b08e8d478d397916a5da6e4e292f6fdaa))
        * [imp] Changing pattern syntax selection from combobox now triggers filter/find update ([b980492b](https://github.com/tc3t/dfglib/commit/b980492bf23b3dbddf286793b01ca7abe73fcbe4))
        * [imp] In content generation, previously using floating point format such as 1.0 in place where integer was expected (e.g. random distribution parameter) was interpreted as zero; now they are interpreted correctly and if they are not integer-valued, a message is shown to user ([a4b291fc](https://github.com/tc3t/dfglib/commit/a4b291fcb12ed49e6825a8324e08cfd1f21ce7ee))
        * [fix] Changing find-text while selection analyzer was active likely resulted to crash. Also added some edit blocks to cases where editing should not be possible ([f495f234](https://github.com/tc3t/dfglib/commit/f495f234c9862dc93a5369442d16df377e9d7db3), [932cf5d6](https://github.com/tc3t/dfglib/commit/932cf5d6e3156d7beb703aff5ca4cde892e134f3), [3f1b708e](https://github.com/tc3t/dfglib/commit/3f1b708ecfd74aa85749ddde4cc3350c9e141f9a))
        * [fix] SQLiteFileOpenDialog: Automatic query construction didn't escape column/table names correctly ([f499f6a4](https://github.com/tc3t/dfglib/commit/f499f6a41c1602610d62f650fba3680aadddd472))
        * [fix] Failing to open file due to missing read permission wasn't detected as error ([3595f1dc](https://github.com/tc3t/dfglib/commit/3595f1dceea5bffa820b060b59ba7f7471165622))
    * Charts
        * [new] New chart type: *txy*. Difference to type *xy* is that this does not order by x-values so it can be used to visualize curves like circles. ([#54](https://github.com/tc3t/dfglib/issues/54))
        * [new] Can now set background gradient for the whole chart canvas ([#45](https://github.com/tc3t/dfglib/issues/45), [83cbc5b8](https://github.com/tc3t/dfglib/commit/83cbc5b8e285a4fceb372ce8a1f984539fef8c2a)) 
        * [imp] Chart updates now cause much less UI freezing as most of the work is done in worker thread. ([#55](https://github.com/tc3t/dfglib/issues/55))
        * [imp] Graphs in the same panel now have different colours by default ([#26](https://github.com/tc3t/dfglib/issues/26), [894b944c](https://github.com/tc3t/dfglib/commit/894b944c65f3a0d2a05468e2852abb4d5fbfd0ac))
        * [imp] Smoothing operations now handle NaN's ([0e83375a](https://github.com/tc3t/dfglib/commit/0e83375ab2b9ab1935423e67b74eb54e8999eb78))
        * [imp] Can now set panel axis and label colours through panel_config ([beedf6a7](https://github.com/tc3t/dfglib/commit/beedf6a72b68aa56b63f4b2c312a4d8a9cfae12f))
        * [imp] Chart canvas now has a visual indicator when it's being updated ([75f8c941](https://github.com/tc3t/dfglib/commit/75f8c9413970124bffd91755ffe02be35720c9e7))
        * [imp] Optimizations to parsing values from table strings ([7c6ec3e8](https://github.com/tc3t/dfglib/commit/7c6ec3e84f3e1b07e50b0153a9e9c14fa096a740))
        

### 1.7.0, 2020-11-15
* Tag: [1.7.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_1.7.0)
    * General
        * [new] Can now import data from SQLite3-files; supports both UI-based "pick table and columns" and custom query ([#31](https://github.com/tc3t/dfglib/issues/31))
        * [new] Rudimentary SQLite export support ([8beec2ba](https://github.com/tc3t/dfglib/commit/8beec2ba19c52cb3631ed3174f80e98c5edb46f8))
    * Charts
        * [new] Can now use csv and sqlite3 files from file system as data source without opening them in the editor (csv: [#13](https://github.com/tc3t/dfglib/issues/13), [5be40282](https://github.com/tc3t/dfglib/commit/5be4028295b1aa6a8b074597e69ee35e828cfbde); sqlite: [#48](https://github.com/tc3t/dfglib/issues/48), [01575bc5](https://github.com/tc3t/dfglib/commit/01575bc5329595bafd94c0df427afbb967fb8050))
        * [new] Can now add chart data tranforms through json-definition
            * Currently available operations are: arbitrary element-wise arithmetic operation, filtering by pass/block window, basic smoothing. See Features-list for details.
        * [imp] In x_rows-field, can now use negative values as "from end" ([#41](https://github.com/tc3t/dfglib/issues/41), [98a2846f](https://github.com/tc3t/dfglib/commit/98a2846feb21698808190122a7c852ae38c02c8d), [7b8689b2](https://github.com/tc3t/dfglib/commit/7b8689b2dec880cc4ac23fff452d4a34a969c06c))
        * [imp] Added context menu option "Rescale all axis" ([1394fd26](https://github.com/tc3t/dfglib/commit/1394fd264a619d651fe28c44f78f89b9632ce337))
        * [imp] Can now specify log level both globally and per entry ([416572eb](https://github.com/tc3t/dfglib/commit/416572eb936d0f122cc41822a1e2ce253f10c77f))
        * [imp] Histogram and bars now support x_rows field ([f481f413](https://github.com/tc3t/dfglib/commit/f481f4131e6f47333debe1e6b363801fb9d36ae0))
        * [imp] Can now define fill colour for histograms and bars ([#44](https://github.com/tc3t/dfglib/issues/44), [ef773864](https://github.com/tc3t/dfglib/commit/ef7738640fc3ae219af4b12e891c50b1fb99047c))
        * [imp/mod] Increased font size in guide window and window size is now dependent on mainwindow size ([c997095c](https://github.com/tc3t/dfglib/commit/c997095cd68232bf4ab1e1adad77bf0434212b40))
        * [mod] Row indexing for data source of type 'table' now starts from 1 for consistency with selection sources ([2ad5f394](https://github.com/tc3t/dfglib/commit/2ad5f3940e84b8db2aeee657c89aa28b413d3e7b))
        * [fix] Parsing fixes, for example "-1e-9" and "inf" were parsed as nan ([14e5312e](https://github.com/tc3t/dfglib/commit/14e5312e6a27cd154ef1e2778145f48c3d9af359))
        * [fix] "Remove all chart objects"-context menu option was disabled when there were only non-xy chart objects ([211db579](https://github.com/tc3t/dfglib/commit/211db579ba8e1e5ffbafa6acb357cfff0e65bbd6))

### 1.6.0, 2020-08-16
* Tag: [1.6.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_1.6.0)
    * [new] New chart type: bars ([#35](https://github.com/tc3t/dfglib/issues/35))
    * [new] Charts now have tooltips that show values around cursor ([#9](https://github.com/tc3t/dfglib/issues/9)).
    * [imp] Can now set axis tick label direction
    * [imp] Histograms (and bars) now have auto axis labels like xy-type.
    * [imp] Can now create histograms from text content ([#36](https://github.com/tc3t/dfglib/issues/36)).
    * [imp] Files with \r eol should now open fine in most cases ([#40](https://github.com/tc3t/dfglib/issues/40)).
    * [imp] Histograms from datetime values now show datetime bins instead of numeric bins.
    * [imp] When opening file through "Open file with options", the dialog now has better default values.
    * [imp] When opening file with filters, document title now indicates that filters were used.
    * [imp] Chart controls can now be loaded from conf-file on open ([#32](https://github.com/tc3t/dfglib/issues/32)).

### 1.5.0, 2020-07-22
* Tag: [1.5.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_1.5.0)
    * Notable changes
        * [new] Charting (disabled by default)
            * xy-graphs and histograms are supported.
            * Disabled by default due to licensing issues (GPL).
        * [new] Filtered reading
        * [imp] Significantly faster open times with default parser: in a test case 16 s -> 10 s (compared to 1.1.0)
        * [imp] Much lower memory usage for dense tables: in a test case 4.2 GB -> 3.0 GB (compared to 1.1.0)
        * [imp] Can now set column names from column header context menu ([b15d91a9](https://github.com/tc3t/dfglib/commit/b15d91a92c92fd2b06ad3ea9d8572f8db4e21cb9))
        * [imp] 'Go to line'-operation (available from context menu and shortcut) ([70bea62d](https://github.com/tc3t/dfglib/commit/70bea62d71042e4c0dbc885f070e363f041225c3))
        * [imp] Performance improvements to pasting and clearing cells. ([1a785bdd](https://github.com/tc3t/dfglib/commit/1a785bdd97bde745299b5649815695497c4f97db), [03484d5c](https://github.com/tc3t/dfglib/commit/03484d5cc24a5ec397e012e33b3dacee99b8c2ef))
        * [imp] When opening file with options, default settings are now based on the file content.
        * [mod] Saved files will now end with new line ([c714c060](https://github.com/tc3t/dfglib/commit/c714c060a9108b06eeb28e16731895e12b0b723f))
        * [fix] Find didn't always jump to case-differing matches in case insensitive find even though the matches were highlighted in table. ([9d5d6dac](https://github.com/tc3t/dfglib/commit/9d5d6dac66b287d2a647582c11b5467d6315421d))
        * [fix] Saving a file that was empty when opened resulted to garbage separator ([8fbc3a54](https://github.com/tc3t/dfglib/commit/8fbc3a547992f5056f5786c440d9e1a50af27574))
        * [fix] Saving to a read-only file looked as if it worked even though nothing was saved ([3a349a49](https://github.com/tc3t/dfglib/commit/3a349a49a3b09dc32b064a9a92c82ad6c09c7fd8))
        * [fix] Fix to crash-propensity in generate content ([973326f6](https://github.com/tc3t/dfglib/commit/973326f64f51ffc89ebadf20833b5dc5e7e7e331))
        * [fix] fixed some crash-propensity when changing filter ([ddb3735a](https://github.com/tc3t/dfglib/commit/ddb3735a5db62ad92fd260bbbebc0879bb451d76))

### 1.1.0, 2020-02-11
* Tag: [1.1.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_1.1.0.0)
    * Notable changes
        * [new] UI-additions
            * 'Generate content' now has lots of new distributions from which content can be generated from. Dialog now also remembers settings from previous use (not over process restart, though) ([a6170f79](https://github.com/tc3t/dfglib/commit/a6170f79f833b038e9e4d60987b4bb2f9f0d9896))
            * New context menu option: "New table from clipboard" ([5a28820d](https://github.com/tc3t/dfglib/commit/5a28820d1802373d4eecd269dce359bc7ec34897))
            * ctrl+arror key now behaves similar to spreadsheet applications. ([fb8c5bf1](https://github.com/tc3t/dfglib/commit/fb8c5bf170845e8236c4338c23073ba7fd941382))
        * [imp] Various optimizations
            * Massively improved performance for removing rows. ([7c018779](https://github.com/tc3t/dfglib/commit/7c01877925884fcf666fc156f76bd8178cd7ba34))
            * Copying big selection to clipboard could freeze the application. ([07ccc1ba](https://github.com/tc3t/dfglib/commit/07ccc1bab7336b05fc37cb1943d4cc2abfacde23))
        * [fix] Pasting to empty table (row or column count == 0) caused a crash. ([491ac18f](https://github.com/tc3t/dfglib/commit/491ac18fd0ba392b87d0b0566f65686e2dfa8279))
        * [fix] 'header-first row' actions could create redundant undo-actions and possibly cause a crash ([b7c8a179](https://github.com/tc3t/dfglib/commit/b7c8a17945b13a1f74ae49b416594f6f92e5d002), [62d0f795](https://github.com/tc3t/dfglib/commit/62d0f7959c8881fabaa392e34da1e2460047436a))
        * [fix] Holding Shift+Page Down in a large table could cause a crash ([5933ecf0](https://github.com/tc3t/dfglib/commit/5933ecf092a3fd532e7bce9772e172b514f58354))
        * [fix] Fixes to copy (clipboard) and undo after deleting rows. ([9843c98d](https://github.com/tc3t/dfglib/commit/9843c98d93c58a38780bbe73549c588419d94a64))
        * [fix] Application could crash if editing table while selection analyzer was working ([19e0e40c](https://github.com/tc3t/dfglib/commit/19e0e40c3d9936e4ce8f11b776293c404669d5cd))
        

### 1.0.0, 2019-09-05
* Tag: [1.0.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_1.0.0.0)

## Features

* Separator auto-detection: instead of e.g. relying on list separator defined in OS settings (like done by some spreadsheet applications), uses heuristics to determine separator character. Auto-detection supports comma (,), semicolon (;), tab (\t) and unit separator (\x1F).
* Text-oriented: content is shown as read, no data type interpretations are made.
* Import from SQLite3 (since version 1.7.0)
    * Either from UI-based "pick table and columns" or custom query.
* Easy to use basic filtering: Alt+F -> type filter string -> view shows only rows whose content (on any column) match with filter string. Matching syntax supports for example wildcard and regular expression.
* Filtered reading: Allows parts of csv-file to be read (since 1.5.0)
    * Row index filtering (to read only given rows by index)
    * Column index filtering (to read only given columns by index)
    * Content filter: Arbitrary number of and/or combinations of text matchers similar to filter in UI.
* Diffing using 3rd party diff tool (manual configuration).
* Support for per-file configurations that can be used for example to define format characters and column widths in UI. Can also be used with SQLite3 files e.g. to define query and chart entries. For details, see [File specific configuration](#file-specific-configuration).
* When saving to csv-file, tries to use the same format as what was used when reading the file (e.g. use the same separator character)
* Rudimentary SQLite3 export support (since version 1.7.0)
* Charting support using [QCustomPlot](https://www.qcustomplot.com/) (since version 1.5.0)
    * xy-graphs, histograms and bar charts are supported.
    * json-based chart definition.
    * Has been tested to work with 50 million row, single column xy-graph. In a test machine chart update times in 50M line case were 65 s in non-cached case (=first creation of chart) and 6 s in cached case; corresponding numbers in case of 1M lines were 2 s and 0.2 s.
    * Multiple data source options:
        * Selection in table editor (e.g. selecting column automatically creates visualization of the data)
        * Table open in table editor
        * csv-file in file system (since 1.7.0)
        * SQLite3-file in file system using user-supplied query (since 1.7.0)
        * Number generator (since 1.9.0)
    * Data transforms:
        * _blockWindow_ & _passWindow_: filters out values inside/outside of given range. (since 1.7.0)
        * _formula_: arbitrary arithmetic element-wise transform (e.g. shifts and scaling can be implemented with this) (since 1.7.0)
        * _smoothing_indexNb_: basic smoothing (since 1.7.0)
    * UI includes [guide](../../dfg/qt/res/chartGuide.html) providing examples and detailed documentation.
    * Disabled by default due to licensing issues (GPL).
* No artificial row/column count restrictions. Note, though, that data structures and implementation in general are not designed for huge files, see below for concrete examples. Underlying data structure is optimized for in-order walk through column content.
* Somewhat reasonable performance:
    * Speed of CSV parser itself depends on various details, but is reasonably fast by default, and when specifying format as unquoted UTF-8, the fastest one tested in [csvPerformanceRuns.csv](../../misc/csvPerformanceRuns.csv) as of 2021-09.
    * opening a 140 MB, 1000000 x 20 test csv-file with content "abcdef" in every cell, lasted less than 3 seconds with dfgQtTableEditor 1.0.0 default read options (and little over 1 second with manually given read options, most importantly disabled quote parsing), in LibreOffice 6.1.6.3 on the same machine read took over 30 seconds.
    * Since opening huge files is not a priority, limits of opening such files hasn't been thoroughly examined, but some examples for concreteness:
        * With version 1.5.0 and a rather ordinary Windows desktop machine (Intel i5 desktop CPU launched in 2013), opening a 1 GB file with 50 million rows and three 3 columns with content "abcdef" in every cell, lasted (in warm cache case) about 10 seconds with default options, about 6 seconds if using manual read options. When opened, application used about 3.0 GB of memory. With earlier version 1.1.0 figures were 16s / 8s / 4.2 GB.
        * Opening a 3 GB, 1e6 x 1024 file with content ab in each cell lasted about 105 s with peak memory consumption of 14 GB on a machine that had 32 GB of RAM (in version 1.5.0).
* Supported encodings:
    * Read: UTF-8, UTF-16BE, UTF-16LE, UTF-32BE, UTF-32LE, Latin-1, Windows-1252
        * Note: when file has no BOM, reading assumes Latin-1
    * Write: UTF-8, Latin-1
* Undo & redo and detailed undo control.
* Auto-completion (column-specific)
* Formula parser (since 1.9.0)
    * Cell content can be evaluated as formula, replaces cell content. Available from context menu and with shortcut Alt+C.
* Content generation
    * Most of C++11 defined random number distribution available (since 1.1.0)
    * Formula-based generation with ability to refer to other cells (since 1.8.0)
        * When build with C++17 support, most of [C++17 mathematical special functions](https://en.cppreference.com/w/cpp/numeric/special_functions) can be used in formulas (since 1.9.0)
        * Supports formatting values as dates (since 1.9.0)

## Tips

* For optimal read speed
    * Input should be in UTF-8 encoding and application should know this e.g. through file having BOM or through UI.
    * File should have no enclosing characters (i.e. quoting) and read options should have enclosing character as none and completer columns should be empty.

## Configuration options

### Application configuration

Stored in ini-file with path \<executable path\>.ini
Example file with comments describing the setting.
<pre>
[dfglib]

; -----------------------------------------------------------
; General settings

; Windows-specific: if set, application tries to create crash dump in case of crash.
; Default: false
enableAutoDumps=1

; Path of executable to use when diffing.
; Default: empty
diffProgPath=C:/Program Files/TortoiseSVN/bin/TortoiseMerge.exe

; -----------------------------------------------------------
; TableEditor

; Defines height of single cell editor widget.
; If value is plain integer, defines absolute height.
; If starts with %, defines height in percents of parent windows height.
; Default: 50
TableEditor_cellEditorHeight=%20

; Defines font size in cell editor widget
TableEditor_cellEditorFontPointSize=13

; Defines width of chart panel either as pixel width or percentage of parent widget width.
; Default: %35
; Note that the value is a request that might not be fulfilled e.g. if it would result
; to width smaller than minimum widget width.
TableEditor_chartPanelWidth = %50
; TableEditor_chartPanelWidth = 200 ; This would set panel width to 200 pixels

; Defines application level default for selection details as a list of space-separated json-objects.
; For details, see documentation of properties/selectionDetails in 'File specific configuration'
TableEditorPropertyId_selectionDetails = {\"id\":\"avg\"} {\"id\":\"max\"}

; -----------------------------------------------------------
; CsvItemModel

; Columns for which to use auto-completion feature.
; To enable for all columns, use *
; To enable for selected columns, provide comma-separator list of column indexes (1-based index)
; To disable, leave value empty
; Note: In many cases it probably makes more sense to define this as file-specific property.
; Default: empty (=completer is disabled)
CsvItemModel_completerEnabledColumnIndexes=*

; Size limit for enabling completer: files larger than this value will be loaded with disabled completer even if completer is enabled by CsvItemModel_completerEnabledColumnIndexes
; Default value: 10000000 (10 MB)
CsvItemModel_completerEnabledSizeLimit=10000000

; Advanced write performance tuning: defines the maximum file below which files are written to memory before writing to file.
; Omit setting or use value -1 to let application decide.
CsvItemModel_maxFileSizeForMemoryStreamWrite=100000000

; Separator char to use when saving new files.
; Default: x\1f  (unit separator)
; Note: needs quotes for comma in order to be parsed correctly.
CsvItemModel_defaultFormatSeparator=","

; Enclosing char to use when saving new files.
; Default: "
; Note: needs escaping for " in order to be parsed correctly.
CsvItemModel_defaultFormatEnclosingChar=\"

; End-of-line type to use when saving new files.
; Default: \n
; Available types: \n, \r, \r\n
CsvItemModel_defaultFormatEndOfLine=\n

; -----------------------------------------------------------
; CsvTableView
             
; Defines the initial scroll position after loading a file.
; Currently 'bottom' is the only recognized option
; Default behaviour: scroll position is top.
CsvTableView_initialScrollPosition=bottom

; Defines the minimum width for columns.
; Default: 5
; Note: affects only some resize schemes.
CsvTableView_minimumVisibleColumnWidth=15

; Defines time format for time stamps available through insert-actions.
; Value is passed to QTime::toString() so check it's documentation for details.
; Default value: hh:mm:ss.zzz
CsvTableView_timeFormat=hh:mm:ss

; Defines date format for date stamps available through insert-actions.
; Value is passed to QDate::toString() so check it's documentation for details.
; Default value: yyyy-MM-dd
CsvTableView_dateFormat=yyyy-MM-dd

; Defines datetime format for datetime stamps available through insert-actions.
; Value is passed to QDateTime::toString() so check it's documentation for details.
; Default value: yyyy-MM-dd hh:mm:ss.zzz
CsvTableView_dateTimeFormat=yyyy-MM-dd hh:mm:ss.zzz

; Defines default weekday names to use with 'WD' date format specifier.
; For details, see documentation of properties/weekDayNames in 'File specific configuration'
; Default value: "mo,tu,we,th,fr,sa,su"
CsvTableView_weekDayNames=mo,tu,we,th,fr,sa,su

; Defines default edit mode after start up
; Possible values: readOnly, readWrite
CsvTableView_editMode=readOnly

; -----------------------------------------------------------
; JsonListWidget

; Defines font point size as integer.
; Default: 10
JsonListWidget_fontPointSize = 12

; Defines whether lines are wrapped
; Default: 1 (true)
JsonListWidget_lineWrapping = 0
</pre>

### File specific configuration

File specific configuration can be used for example to:
* Read the file correctly (e.g. in case of less common csv format)
* Optimize the read speed by disabling unneeded parsing features
* Set UI column widths so that content shows in preferred way.
* Define charts that get shown automatically after opening
* Define read filters

Note that while most of the settings are csv-file specific, configuration file can also be used with SQLite-file.

Configuration is defined through a .conf file in path \<file path\>.conf. File can be generated from table view context menu from "Config" -> "Save config file...". The .conf file is a key-value map encoded into a csv-file:

* Supports URI-like lookup, where depth is implemented by increasing column index for key-item.
* Key is the combination of first item in a row and if not on the first column, prepended by most recent key in preceding column.

Available keys:

| Key (URI)      | Purpose  | Possible values | Notes |
| -------------  | -----    | ------          | ----- |
| columnsByIndex/ColumnIndexHere/width_pixels | Defines column width in pixels for given column (1-based column index) | integer | |
| columnsByIndex/ColumnIndexHere/visible | Defines whether column is visible (1-based column index) | 0 or 1, default is 1 | Since 2.2.0 |
| encoding | When set, file is read assuming it to be of given encoding. | Latin1, UTF8, UTF16BE, UTF16LE, UTF32BE, UTF32LE, windows-1252 | This setting is used even if file has a BOM that indicates different encoding |
| enclosing_char | File is read interpreting the given character as enclosing character | ASCII-character as such or escaped format, e.g. \t or \x1f | |
| separator_char | File is read interpreting the given character as separator character | Like for enclosing_char | |
| end_of_line_type | File is read using end-of-line type | \n, \r\n, \r | |
| bom_writing | Whether to write BOM on save | 0, 1 | |
| properties/completerColumns | see CsvItemModel_completerEnabledColumnIndexes | | |
| properties/completerEnabledSizeLimit | see CsvItemModel_completerEnabledSizeLimit | | |
| properties/editMode | Defines edit mode when opened | _readOnly_, _readWrite_<br>If empty, handled as if property was not present at all. | Since 2.1.0 |
| properties/includeRows | Limits rows which are read from file by row index (0-based index, typically header is on row 0) |  | Since 1.5.0 |
| properties/includeColumns | Like includeRows, but for columns | | Since 1.5.0 |
| properties/readFilters | Defines content filters for read, i.e. ability to filter read rows by content. For example only rows that match a regular expression in certain column(s). | The same syntax as in UI, syntax guide is available from UI tooltip | Since 1.5.0 |
| properties/chartControls | If dfgQtTableEditor is built with chart feature, defines chart controls that are taken into use after load. | The same syntax as in UI, see [guide](../../dfg/qt/res/chartGuide.html) for detail | Since 1.6.0 |
| properties/chartPanelWidth | If dfgQtTableEditor is built with chart feature, defines chart panel width to be used with the associated document. | Width value, see syntax from TableEditor_chartPanelWidth | Since 1.8.1 ([#60](https://github.com/tc3t/dfglib/issues/60))
| properties/windowHeight | Defines request for window height when opening associated document. | Height value, see syntax from TableEditor_chartPanelWidth | Since 2.0.0 ([#86](https://github.com/tc3t/dfglib/issues/86))
| properties/windowWidth | Defines request for window width when opening associated document. | Width value, see syntax from TableEditor_chartPanelWidth | Since 2.0.0 ([#86](https://github.com/tc3t/dfglib/issues/86))
| properties/windowPosX | Defines request for window x position when opening associated document, only taken into account if either windowHeight or windowWidth is defined. | x pixel position of top left corner, 0 for left. | Since 2.0.0 ([#86](https://github.com/tc3t/dfglib/issues/86))
| properties/windowPosY | Defines request for window y position when opening associated document, only taken into account if either windowHeight or windowWidth is defined. | y pixel position of top left corner, 0 for top. | Since 2.0.0 ([#86](https://github.com/tc3t/dfglib/issues/86))
| properties/sqlQuery | For SQLite files, defines the query whose result is populated to table. | Valid SQLite query | Since 1.6.1 (commit [24c1ad78](https://github.com/tc3t/dfglib/commit/24c1ad78eac2a6f74b6ee1be0dede0d5645fef07)) |
| properties/selectionDetails | Defines selection details which are shown for every selection; i.e. basic indicators describing a selection such minimum and maximum value. Details are defined with a list of single line json-objects, where *id* field defines the detail. | Available built-in detail id's:<br>*average*, *cell_count_excluded*, *cell_count_included*, *is_sorted_num*, *median*, *max*, *min*, *sum*, *stddev_population*, *stddev_sample*, *variance*.<br>Since version 2.2.0 can also define custom details, for details see section [Custom selection details](#custom-selection-details) | Since 2.1.0 (commit [2d1c1d1b](https://github.com/tc3t/dfglib/commit/2d1c1d1b230a4d0f6dd8c18633a2af5ac20ea288)) |
| properties/weekDayNames | Defines list of weekday names to use with weekday specifier 'WD' in date formats. | Comma-separated list starting from Monday. For example "mo,tu,we,th,fr,sa,su" | Since 2.2.0 ([#99](https://github.com/tc3t/dfglib/issues/99)) |
| properties/timeFormat | File-specific version of CsvTableView_timeFormat | See documentation of CsvTableView_timeFormat | Since 2.2.0 ([1dfe3691](https://github.com/tc3t/dfglib/commit/1dfe36917580c7c8fc69dbb23e845175f2e5613e)) |
| properties/dateFormat | File-specific version of CsvTableView_dateFormat | See documentation of CsvTableView_dateFormat | Since 2.2.0 ([1dfe3691](https://github.com/tc3t/dfglib/commit/1dfe36917580c7c8fc69dbb23e845175f2e5613e)) |
| properties/dateTimeFormat | File-specific version of CsvTableView_dateTimeFormat | See documentation of CsvTableView_dateTimeFormat | Since 2.2.0 ([1dfe3691](https://github.com/tc3t/dfglib/commit/1dfe36917580c7c8fc69dbb23e845175f2e5613e)) |

<br>

#### Custom selection details
Since version 2.2.0, it is possible to define custom formula-based accumulators as selection details, which can be added to both .conf-file and to application .ini-config using similar json-definition object as for built-in ones. For details, see [selection_details_example.csv.conf](examples/selection_details_example.csv.conf) and instructions in UI ("Details -> Add..").

<br>

#### Example .conf-file for csv-file (might look better when opened in a csv-editor)
<pre>
bom_writing,1,,
columnsByIndex,,,
,1,,
,,width_pixels,400
,2,,
,,visible,0
,3,,
,,width_pixels,200
enclosing_char,,,
encoding,UTF8,,
end_of_line_type,\n,,
separator_char,",",,
properties,,,
,completerColumns,"1,3",
,completerEnabledSizeLimit,10000000,
,editMode,readOnly
,includeRows,100:200
,includeColumns,1:6
,readFilters,"{""text"":""abc"", ""apply_columns"":""2""}"
,chartControls,"{""type"":""xy"",""data_source"":""table"",""x_source"":""column_name(date)"",""y_source"":""column_name(temperature)""}"
,chartPanelWidth,%50
,windowHeight,%50
,windowWidth,%100
,windowPosX,0
,windowPosY,500
,selectionDetails,"{ ""id"": ""sum"" }
{""id"":""max""}
{""description"":""Example for sum of squares"",""formula"":""acc + value^2"",""initial_value"":""0"",""type"":""accumulator"",""ui_name_long"":""Sum of squares"",""ui_name_short"":""Sum x^2""}"
,weekDayNames,"mo,tu,we,th,fr,sa,su"
</pre>

#### Example .conf-file for SQLite-file (might look better when opened in a csv-editor)
<pre>
properties,,,
,chartControls,"{""type"":""xy"",""data_source"":""table"",""x_source"":""column_name(date)"",""y_source"":""column_name(temperature)""}"
,sqlQuery,SELECT * FROM some_table LIMIT 1000;,
</pre>

## Third party code

Summary of 3rd party code in dfgQtTableEditor (last revised 2021-06-27).

| Library      | License  |
| ------------- | ----- |
| [Boost](http://www.boost.org/) | [Boost software license](http://www.boost.org/LICENSE_1_0.txt)  |
| [fmtlib](https://github.com/fmtlib/fmt) | [BSD-2](../../dfg/str/fmtlib/format.h) |
| [muparser](https://github.com/beltoforion/muparser) | [BSD-2](../../dfg/math/muparser/muParser.h) | 
| [QCustomPlot](https://www.qcustomplot.com/) (disabled by default) | [GPLv3/commercial](https://www.qcustomplot.com/) |
| [Qt](https://www.qt.io/) | [Various](http://doc.qt.io/qt-5/licensing.html) |
| [UTF8-CPP](https://github.com/nemtrif/utfcpp) | [Boost software license](../../dfg/utf/utf8_cpp/utf8.h) |


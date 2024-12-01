# dfgQtTableEditor

csv/dsv-oriented ("comma/delimiter-separated values") table editor based on dfglib using Qt as UI framework. Also includes ability to open SQLite3-files and optionally visualization using [QCustomPlot](https://www.qcustomplot.com/).

## Building

### __Version 2.7.0__:

<!-- Table generated from buildStatus_dfgQtTableEditor.csv excluding date column
     with csv2md (https://www.npmjs.com/package/csv2md)
 -->

Build status (all builds are x86-64 i.e. [amd64](https://en.wikipedia.org/wiki/X86-64))

Status | Compiler | Standard library | C++ standard [1] | Qt | Boost | Charting? (QCustomPlot version) | Comment
---|---|---|---|---|---|---|---
:white_check_mark: | Clang 10.0.0 | libc++ 10000 | C++17 | 5.12.8 | 1.71.0 | Yes (2.0.1) | Ubuntu 64-bit 20.04
:white_check_mark: | Clang 10.0.0 | libstdc++ 9 | C++17 | 5.12.8 | 1.71.0 | Yes (2.0.1) | Ubuntu 64-bit 20.04
:white_check_mark: | Clang 14.0.0 | libstdc++ 11 | C++17 | 6.2.4 | 1.74.0 | Yes (2.1.1) | Ubuntu 64-bit 22.04
:red_circle: | Clang 14.0.0 | libc++ 14000 | C++17 | 6.2.4 | 1.74.0 | Yes (2.1.1) | Ubuntu 64-bit 22.04. Causes linker errors from QMetaType, for details, see notes in dfgTestQt table.
:white_check_mark: | clang-cl (Clang 17.0.3, MSVC2022.5) | MSVC | C++17 | 6.4.1 | 1.70.0 | Yes (2.1.1) | 
:white_check_mark: | GCC 9.4.0 | libstdc++ 9 | C++17 | 5.12.8 | 1.71.0 | Yes (2.0.1) | Ubuntu 64-bit 20.04
:white_check_mark: | GCC 11.4.0 | libstdc++ 11 | C++17 | 6.2.4 | 1.74.0 | Yes (2.1.1) | Ubuntu 64-bit 22.04
:white_check_mark: | MinGW 11.2.0 | libstdc++ 11 | C++17 | 6.4.1 | 1.70.0 | Yes (2.1.1) | 
:white_check_mark: | VC2019.11 | MSVC | C++17 | 5.15.2 | 1.70.0 | Yes (2.1.1) | 
:white_check_mark: | VC2022.11 | MSVC | C++17 | 6.4.1 | 1.70.0 | Yes (2.1.1) | 
:white_check_mark: | VC2022.11 | MSVC | C++20 | 6.4.1 | 1.70.0 | Yes (2.1.1) | 

[1] As reported by *__cplusplus* macro  or equivalent.

If building with QCustomPlot charting, histogram chart type requires Boost >= 1.70. When using Qt >=6.2, charting requires at least version 2.1.1 of QCustomPlot.

Concrete build steps assuming having compatible Qt version, Qt Creator and compiler available:
* Open dfgQtTableEditor.pro in Qt Creator
* Choose suitable Qt kit to use
* Click build

Note that in Qt versions 5.10-5.12.3, keyboard shortcuts won't show as intended in context menu (for further details, see [QTBUG-61181](https://bugreports.qt.io/browse/QTBUG-61181), [QTBUG-71471](https://bugreports.qt.io/browse/QTBUG-71471)).

To see build chart of older versions, see [readme of 2.6.0](https://github.com/tc3t/dfglib/tree/dfgQtTableEditor_2.6.0/dfgExamples/dfgQtTableEditor)

## Version history

### 2.6.0, 2023-12-17
* Tag: [2.6.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_2.6.0)
* Highlights: Column filter from header context menu, CsvTableView regexFormat, chart improvements
* General
    * [new] Support for custom datetime types for columns ([#153](https://github.com/tc3t/dfglib/issues/153))
    * [new] Added trim-operation ([#159](https://github.com/tc3t/dfglib/issues/159), [4775eb45](https://github.com/tc3t/dfglib/commit/4775eb452c3704ba9c42651e8ad9832c010199b5))
    * [new] Added Regex format-operation ([#160](https://github.com/tc3t/dfglib/issues/160))
    * [new] Can now define column filter from column header context menu ([#163](https://github.com/tc3t/dfglib/issues/163))
    * [imp/mod] Fixes and improvements to sorting of number-type columns
 ([cdc6ec7d](https://github.com/tc3t/dfglib/commit/cdc6ec7d50374aa0142b2da6ada4255d08039f34))
    * [imp] Performance improvements to selection handling ([ed4d70f8](https://github.com/tc3t/dfglib/commit/ed4d70f8609fb23024983183a6098bd863398c3a))
    * [imp] find&filter -panel settings can now be stored to .conf-file ([#148](https://github.com/tc3t/dfglib/issues/148))
    * [imp] Performance improvements to string replace ([1ada8dfe](https://github.com/tc3t/dfglib/commit/1ada8dfed3026e92305f247e22665a497af17385))
    * [imp] Column header now includes column index and tooltip is shown also for unnamed columns ([#161](https://github.com/tc3t/dfglib/issues/161), [e8eb528b](https://github.com/tc3t/dfglib/commit/e8eb528ba03312d216de3a2d2d911af804d50c40))
    * [imp] Enabled tooltips in CsvTableView context menu ([6f8f6a17](https://github.com/tc3t/dfglib/commit/6f8f6a17ab0d803f557dc44adb3b8dfafe7c3605))
    * [imp] Maximized window position is now stored properly to .conf-file ([#149](https://github.com/tc3t/dfglib/issues/149), [8d6e0926](https://github.com/tc3t/dfglib/commit/8d6e09268264c9a989b45cb48b7d5ddee1b4e30d))
    * [imp] Added new flavour of "Reload from file" ([#158](https://github.com/tc3t/dfglib/issues/158), [8066a336](https://github.com/tc3t/dfglib/commit/8066a3364d3e103f9493df9f9cf558c6f50fa409))
    * [mod] Adjusted column header text alignment ([#165](https://github.com/tc3t/dfglib/issues/165), [d042cc94](https://github.com/tc3t/dfglib/commit/d042cc948a5961d63e0296ab23a8db229722ccea))
    * [mod] Moved some CsvTableView context menu items to submenus to keep menu smaller
 ([da0473aa](https://github.com/tc3t/dfglib/commit/da0473aa1036666381cbc7a205b79cddfdd1d9b1))
    * [mod] json-filter now shows invalid json slightly differently
 ([e6a7d19d](https://github.com/tc3t/dfglib/commit/e6a7d19da7112c20db771637095d5723675745fe))
    * [fix] saveAsShown didn't store column names
 ([28654fc0](https://github.com/tc3t/dfglib/commit/28654fc048643def5c346c046ba6fa6de68824c6))
* Charts
    * [imp] added 'copy tooltip text to clipboard' -functionality to chart context menu ([#162](https://github.com/tc3t/dfglib/issues/162), [219c516d](https://github.com/tc3t/dfglib/commit/219c516d6be8da70870e9c9d6bb645e51ca8fa97))
    * [imp] Manual chart operations can now be done for bars-chart string-axis ([#166](https://github.com/tc3t/dfglib/issues/166), [583dd4ae](https://github.com/tc3t/dfglib/commit/583dd4ae63480d1c27cde51c38455f193aaf5d30))
    * [imp] Axis tick label format can now be defined through panel_config ([#167](https://github.com/tc3t/dfglib/issues/167), [a9059807](https://github.com/tc3t/dfglib/commit/a90598074b8c0fe096cc289a9aa2313133661ac6))
    * [imp] If chart data source changes during chart preparation, a warning is now generated to log since resulting graphs might not be consistent (e.g. some graphs might be created from older data and some from newer) ([#168](https://github.com/tc3t/dfglib/issues/168), [4c0b036f](https://github.com/tc3t/dfglib/commit/4c0b036f652dff73e81474c8280b3cfe55c81406))
    * [mod] Operations: if regexFormat-operations encounters errors, previously it resulted to empty string, now results to an error message
 ([b02f9903](https://github.com/tc3t/dfglib/commit/b02f9903fbcf9ed35963600f6b92017f3edb0a0c))
    * [mod] Changed tooltip distance metric for txy and txys types ([#164](https://github.com/tc3t/dfglib/issues/164), [54c2561e](https://github.com/tc3t/dfglib/commit/54c2561e92d3eb1d9ef9acb6ac310bee669f2538))
    * [fix] Changing column name in table didn't update charts ([#157](https://github.com/tc3t/dfglib/issues/157), [4bfa33c3](https://github.com/tc3t/dfglib/commit/4bfa33c37b9866c9364f77923cfac3c1737bd813))


### 2.5.0, 2023-04-26
* Tag: [2.5.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_2.5.0)
* Highlights: Filter from selection, sort-setting in .conf-file, major performance improvements to charts, regexFormat-operation
* Build requirements
    * Building now requires C++17 ([481baa71](https://github.com/tc3t/dfglib/commit/481baa717a0cc90fb55813de787ba68fb98b1515))
* General
    * [new] Can now create a filter from selection ([#146](https://github.com/tc3t/dfglib/issues/146))
    * [new] Sort-settings can now be stored to .conf-file ([#142](https://github.com/tc3t/dfglib/issues/142))
    * [imp] When overwriting an existing .csv-file, a confirmation is now asked if encoding or EOL would change ([#145](https://github.com/tc3t/dfglib/issues/145), [af8d28f7](https://github.com/tc3t/dfglib/commit/af8d28f70fb03773d802bbf0731963bd0b84bf83))
    * [imp] Parsing datetimes to doubles now recognizes more formats ([3e7b138a](https://github.com/tc3t/dfglib/commit/3e7b138aabd707993d80a726ce890d65a40905b7))
    * [imp] Table view now has basic logging console (can be enabled from context menu) ([#152](https://github.com/tc3t/dfglib/issues/152))
    * [mod] Adjustments to menu items ([60fdf388](https://github.com/tc3t/dfglib/commit/60fdf388d71ef57e58363e9745fb40b7bdcfe173))
    * [fix] Editing filter while having hidden columns visually unhide all columns ([#151](https://github.com/tc3t/dfglib/issues/151), [187aa245](https://github.com/tc3t/dfglib/commit/187aa24507648751fa34c3c7214d4eb02bcadcb5))
    * [fix] When opening a big file, there was a long delay before actual reading started ([#147](https://github.com/tc3t/dfglib/issues/147), [ac8f0c0f](https://github.com/tc3t/dfglib/commit/ac8f0c0feb893daa34a3f58895bd0b0bc4381565))
    * [fix] Column sorting was case-sensitive by default even though related menu-entry was not checked ([513971d6](https://github.com/tc3t/dfglib/commit/513971d62d5c6cf8dacd5f6b47dd702550cb8705))
    * [fix] Changing row order with sorting didn't trigger update on selection details or charts ([#155](https://github.com/tc3t/dfglib/issues/155), [3d0027e1](https://github.com/tc3t/dfglib/commit/3d0027e1426e69acc48b56657d31ebd1a05fd028))
    * [fix] Column index mapping in single-row tables was broken ([3bfc670b](https://github.com/tc3t/dfglib/commit/3bfc670b13ab87d90c677de63d2cab274ad7c220))
    * [fix] Opening content generator dialog would crash at least in clang-cl release build and the dialog would show a non-read-only row with read-only colour ([8fa9eef3](https://github.com/tc3t/dfglib/commit/8fa9eef30905047a2b9da26c68c2aeed78958115), [ac7fb6a3](https://github.com/tc3t/dfglib/commit/ac7fb6a380b2dc51661f6dc3b584cd2b7e288265))
    * [fix] Can now restore row height back to default ([#144](https://github.com/tc3t/dfglib/issues/144), [2d1755ed](https://github.com/tc3t/dfglib/commit/2d1755ed375c502b1a07bf907bdfba2e6a1db73f))
* Charts
    * [imp] Performance improvements e.g. by caching ([#134](https://github.com/tc3t/dfglib/issues/134), [c26470c2](https://github.com/tc3t/dfglib/commit/c26470c27ff4e458ee42cf69c014fb2847e4de7b))
        * Example of effects with 1e8 x 1 table with random integers when showing xy-graph with all data selected:
            * |       | Without caching | With caching |
              | ----  | --------------- | ------------ |
              | Memory usage with graph shown | 5.9 GB | 5.1 GB |
              | Memory usage maximum | 7.4 GB | 6.6 GB |
              | Memory usage after clearing selection | 2.7 GB | 3.5 GB |
              | Update time after first update by empty-selection -> whole table | 10 s | 2.5 s |
              | Update time with Apply-button | 2.8 s | 2.9 s |
    * [new] New operation 'regexFormat' ([#156](https://github.com/tc3t/dfglib/issues/156))
    * [imp] For graphs of type txys, tooltip now also shows column name of s-column
([85926293](https://github.com/tc3t/dfglib/commit/85926293a00040fb9cd5a4ce48821ad07ac23b99))
    * [imp] Added menu item for escaping content in chart definition text field
([1b4934c7](https://github.com/tc3t/dfglib/commit/1b4934c762bb13c918bcee9b878fe09d6f59cc75))
    * [fix] Column names could be wrong if view had hidden columns
([1cc9f90d](https://github.com/tc3t/dfglib/commit/1cc9f90de70ba46a5b49f9694d70682ccabf2af6))
    * [fix] Fixed a crash candidate in data preparation ([d048d31b](https://github.com/tc3t/dfglib/commit/d048d31b25463dffa471db7da5358fd6191a8275))


### 2.4.0, 2022-10-23
* Tag: [2.4.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_2.4.0)
    * Highlights: Multithreaded reading, (x,y,text)-chart, read-only columns, several usability improvement, fixed crash opening big files.
    * General
        * [fix] Fixes to crash when opening big files and broken read progress indicator ([#141](https://github.com/tc3t/dfglib/issues/141), [8c3c65dd](https://github.com/tc3t/dfglib/commit/8c3c65ddc79f5be224fc414c74e5f8fe081a7fc0))
        * [new] Multithreaded reading. Requires empty enclosing char and UTF8 / Latin1 / Windows-1252 encoding and big enough file. For related controls, see options *readThreadBlockSizeMinimum and *readThreadCountMaximum ([#138](https://github.com/tc3t/dfglib/issues/138))
        * [new] read-only mode for columns ([#129](https://github.com/tc3t/dfglib/issues/129))
        * [new] Files can now be drag&dropped to open file ([#131](https://github.com/tc3t/dfglib/issues/131), [8c0aec62](https://github.com/tc3t/dfglib/commit/8c0aec62a8692bee696142a9e64011553986f486))
        * [imp] Added tool button showing info about the currently open file ([#139](https://github.com/tc3t/dfglib/issues/139))
        * [imp] Can now set initial scroll position from .conf-file ([#130](https://github.com/tc3t/dfglib/issues/130), [e41ed4b5](https://github.com/tc3t/dfglib/commit/e41ed4b502550bba6e68d1116e209da14b675dc4))
        * [imp/reg] Better visualization of multiline cells. As a related regression, completer is no longer available for multiline cells. ([#135](https://github.com/tc3t/dfglib/issues/135))
        * [imp] "Save config file with options"-dialog now has a button to insert some fields such as datetimes ([#137](https://github.com/tc3t/dfglib/issues/137), [7657e3b7](https://github.com/tc3t/dfglib/commit/7657e3b74db730ddac94f3ea52184e28a360cb43))
        * [imp] Leaving cell editor without making changes no longer creates an undo-point ([81c49be1](https://github.com/tc3t/dfglib/commit/81c49be15cffe52c939c8b90820c8b903364443c))
        * [mod/imp] Adjusted scroll behaviour on filter clear ([#133](https://github.com/tc3t/dfglib/issues/133), [97352273](https://github.com/tc3t/dfglib/commit/97352273d4071493920889dabe85e6e89baff2f2))
        * [mod] Disabled delayed autoscroll on cell click ([#136](https://github.com/tc3t/dfglib/issues/136), [e0fa63a6](https://github.com/tc3t/dfglib/commit/e0fa63a6e19322f1b2f4ce8ea18aec009686d7ef))
        * [fix] 'header <-> first row' actions didn't work correctly when columns were hidden ([#143](https://github.com/tc3t/dfglib/issues/143), [a0118b15](https://github.com/tc3t/dfglib/commit/a0118b156bc25f1f6953321fec07be66ec13163d))
    * Charts
        * [new] New chart type: (x, y, text) ([#140](https://github.com/tc3t/dfglib/issues/140))

### 2.3.0, 2022-05-22
* Tag: [2.3.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_2.3.0)
    * __Note__: This version has a bug that causes a crash when opening big files ([#141](https://github.com/tc3t/dfglib/issues/141))
    * General
        * [new] Numeric base converter tool, available from context menu entry "Change radix"  ([#97](https://github.com/tc3t/dfglib/issues/97))
        * [new] Can now sort columns numerically ([#119](https://github.com/tc3t/dfglib/issues/119))
        * [new] New selection detail: percentile ([#104](https://github.com/tc3t/dfglib/issues/104))
        * [imp] Toolbar now has convenience actions to resize window ([#90](https://github.com/tc3t/dfglib/issues/90))
        * [imp] Focussed cell matching with find filter is now shown more clearly ([#120](https://github.com/tc3t/dfglib/issues/120), [cbdaeab5](https://github.com/tc3t/dfglib/commit/cbdaeab519e5f99359e7cc9d18e4f75fcb9589c5))
        * [imp] Can now control selection detail numeric format ([#123](https://github.com/tc3t/dfglib/issues/123))
        * [imp] Saving .conf file now preserves manually added fields ([#122](https://github.com/tc3t/dfglib/issues/122))
        * [imp] If trying to open non-existent app config file from menu, now asking if one should be created ([#116](https://github.com/tc3t/dfglib/issues/116), [9eae1644](https://github.com/tc3t/dfglib/commit/9eae1644b9cfcb99ebdaee63cd4dae9ce5cebfaa))
        * [imp] Updated muparser 2.3.2 -> 2.3.3 ([#126](https://github.com/tc3t/dfglib/issues/126))
        * [fix] Can now unhide columns even if column header is not available ([#127](https://github.com/tc3t/dfglib/issues/127), [ad94c2c7](https://github.com/tc3t/dfglib/commit/ad94c2c7a2a01082375849ee51b154302c69de10))
    * Charts
        * [new] New operation: textFilter. Can be used e.g. to filter bar labels ([#121](https://github.com/tc3t/dfglib/issues/121))


### 2.2.0, 2022-01-18
* Tag: [2.2.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_2.2.0)
    * __Note__: This version has a bug that causes a crash when opening big files ([#141](https://github.com/tc3t/dfglib/issues/141))
    * General
        * [new] Can now hide columns ([#102](https://github.com/tc3t/dfglib/issues/102))
        * [new] Formula-definable selection details and general improvements to selection detail widget (tooltip, enable/disable all actions) ([#103](https://github.com/tc3t/dfglib/issues/103))
        * [new] can now transpose table ([#100](https://github.com/tc3t/dfglib/issues/100), [8f1591f0](https://github.com/tc3t/dfglib/commit/8f1591f0b0f8f9ae497519a52946ccc294ad6b1a))
        * [new] ability to add weekday to Insert->date and to content generation dateTime formats ([#99](https://github.com/tc3t/dfglib/issues/99))
        * [new] Extended "Go To Line" into "Go To Cell" ([#98](https://github.com/tc3t/dfglib/issues/98), [cf4d8ebc](https://github.com/tc3t/dfglib/commit/cf4d8ebc5b5653af8c8c00cb7a617e45592114cd))
        * [imp] can now specify date/time formats in .conf-file. ([1dfe3691](https://github.com/tc3t/dfglib/commit/1dfe36917580c7c8fc69dbb23e845175f2e5613e))
        * [imp] added "Reload from file"-action to context menu ([#101](https://github.com/tc3t/dfglib/issues/101), [4a6bfeac](https://github.com/tc3t/dfglib/commit/4a6bfeac444e8b0c226ff0554d0d3fd7d83ba1bc))
        * [imp] multiple improvements to insert-action ([#112](https://github.com/tc3t/dfglib/issues/112), [031339d6](https://github.com/tc3t/dfglib/commit/031339d60c00598cc634b6902397008383ef7d57))
        * [mod] Table view context menu is now smaller ([4dd8e735](https://github.com/tc3t/dfglib/commit/4dd8e7355b232bf031694a2eb89ad0cc706c55a7))
        * [fix/reg] undoing shrinking resize now also restores previously existed cells, causes resize undo to be much heavier operation in now. ([#118](https://github.com/tc3t/dfglib/issues/118), [eceb507e](https://github.com/tc3t/dfglib/commit/eceb507e98710d130d83b5032c05799f9b73fead))
        * [fix] Insert date/time actions and "Set column headers" were available in read-only mode ([6464b44c](https://github.com/tc3t/dfglib/commit/6464b44ced787819c54c505d96328b1e41d8bba5), [8a8a8110](https://github.com/tc3t/dfglib/commit/8a8a8110b85f819b8269736f678f8150c1533562))
        * [fix] Various minor fixes and changes (Changing find column triggered faulty "data changed'-signals in some cases [33f69c43](https://github.com/tc3t/dfglib/commit/33f69c43dd204718161f9706c5685dbebfca0c55), About box now shows C++ standard version [bfcbeed2](https://github.com/tc3t/dfglib/commit/bfcbeed28af965fb76110d36dc5225a67cee0465), Now a info tip is shown if inserted row is hidden immediately due to filter [#114](https://github.com/tc3t/dfglib/issues/114) [d931b7e6](https://github.com/tc3t/dfglib/commit/d931b7e629d6b5b52c2dc231a70a7eee6e87c0a1))
    * Content generator
        * [imp] Formula parser now has new time handling functions ([#106](https://github.com/tc3t/dfglib/issues/106), [12c9e607](https://github.com/tc3t/dfglib/commit/12c9e6079245a1cd677eb94e58dfc49ffab5750c))
        * [imp/mod] In content generator, cellValue() function now returns numeric values for dates and times. Also value with comma such as 1,25 is now interpreted as 1.25 ([98819f66](https://github.com/tc3t/dfglib/commit/98819f66f64af33b739ea1d7aaedce042b1b34d6))
        * [mod] in content generator default floating point precision is now round-trippable instead of 6 ([e9386e6e](https://github.com/tc3t/dfglib/commit/e9386e6e2c4aade73a6b5875d899fa379539a5b8))
        * [fix] Date formatting would result to empty if input value had milliseconds in decimal part ([1dcb9a18](https://github.com/tc3t/dfglib/commit/1dcb9a1872cd9656e77f4a9b88fc67e819581d30))
    * Charts
        * [new] Can now set axis range from panel_config axis_properties ([#105](https://github.com/tc3t/dfglib/issues/105), [7817a05d](https://github.com/tc3t/dfglib/commit/7817a05da3f3eec7d0b16726dfcf6662f318aa70))
        * [imp] Apply button now shows chart preparation progress ([#108](https://github.com/tc3t/dfglib/issues/108))
        * [imp] Chart definition text widget now has context menu entry to comment/uncomment ([139f6311](https://github.com/tc3t/dfglib/commit/139f63116c5739812a0e10e31cc8edd6a077a908))
        * [fix] In some cases bars-chart remained completely hidden after inserting rows and some content ([#115](https://github.com/tc3t/dfglib/issues/115), [7144b69d](https://github.com/tc3t/dfglib/commit/7144b69de86256cf654eeabd87cc71d4feca46ce))
        * [fix] chart entries were prepared even if they were disabled ([7d1e9d71](https://github.com/tc3t/dfglib/commit/7d1e9d716c4b074e4ea94fa3af21d74e38c42de9))
        * [fix] in image export "Successfully exported to path"-widget file link didn't always work due to missing escaping ([9f52d21a](https://github.com/tc3t/dfglib/commit/9f52d21a6c121c3886af9036d275159c67115200))
        * [fix] clicking Apply-button sometimes seemed to react only after a long delay ([b24a42e7](https://github.com/tc3t/dfglib/commit/b24a42e7be1e62b112603cf90b2d9b401b5e3817))


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

### Notes about CSV format support

As dfgQtTableEditor aims to be more of a DSV-editor instead pure CSV-editor, it's not a goal to strictly follow [RFC 4180](https://datatracker.ietf.org/doc/html/rfc4180). However as CSV is a special case of DSV, the editor should allow handling CSV-files in RFC 4180 -compliant manner. Below is a table comparing how the editor behaves with respect to RFC 4180 requirements (note: items in RFC 4180 -column are interpretations of dfgQtTableEditor author)

<!-- Table generated from format_support_table.csv with csv2md (https://www.npmjs.com/package/csv2md) --->

RFC 4180 section | Property | dfgQtTableEditor | [RFC 4180](https://datatracker.ietf.org/doc/html/rfc4180) | RFC 4180 compliance | Notes
---|---|---|---|---|---
2.1 | Line break | LF (default), CRLF, CR. Editor tries to use the same line break when writing that was present in read file. | CRLF | Deviates: allows other line breaks than just CRLF | 
2.2 | Last record line break | Either style accepted on read, on write line break is written | May or may not have an ending line break | :white_check_mark: | 
2.3 | Header | On read, first line is always treated as header, but can be manually moved to first row after load. On write, header is written by default, option to disable writing is in advanced save options. | Header is optional, but it "should contain the same number of fields as the records in the rest of the file" | Deviates: when reading, header is not required to have the same number of columns as records (number of columns in table is simply taken from maximum column count from all records), but written files have the same number of headers on all records. | 
2.4.1 | Multiple fields separated by commas | Several field separators are supported by default (comma (,), semi-colon (;), tab (\t) and unit separator (\x1f)). Additionally other single-char separators can be used with advanced read and write options. | "Within the header and each record, there may be one or more fields, separated by commas" | Deviates: allows other separators in addition to comma (,) | 
2.4.2 | Equal field number | Equal field count is not required in records, a record with less fields than some other record is interpreted as having empty fields at the the end. When writing, all records will have the same number of fields even if input file had differing counts. | "Each line should contain the same number of fields throughout the file" | Deviates: similar to section 2.3: not required on read, but column counts will be identical in written files. | 
2.4.3 | Spaces in field | Spaces are preserved. With enclosed cells, enclosing char must appear as the first char in field, otherwise field enclosing chars are not interpreted as such. Also any trailing chars after enclosed field are ignored. | "Spaces are considered part of a field and should not be ignored" | :white_check_mark: | Reader back end (DelimitedTextReader) has option to ignore leading whitespaces ('rfSkipLeadingWhitespaces'), but not used in dfgQtTableEditor
2.4.4 | Trailing comma | Last field is not followed by separator | "The last field in the record must not be followed by a comma" | :white_check_mark: | 
2.5.1 | Enclosing with double quotes | Enclosed fields supported on both read and write; when writing, fields are by default enclosed only if needed. Default enclosing char is double quote (0x22), but can be customised. | "Each field may or may not be enclosed in double quotes" | Deviates: supports also different enclosing characters | 
2.5.2 | Double quotes without enclosed field | If enclosing chars appear in field but field doesn't begin with such, enclosing chars are treated as regular field content. | "If fields are not enclosed with double quotes, then double quotes may not appear inside the fields" | Deviates: can read fields that are not enclosed, but still contain enclosing character. Written files are compliant. | 
2.6 | Requirements when field needs to be enclosed | Supported in generalized sense: fields are enclosed when needed, i.e. if they contain line break, enclosing char or separator char. | "Fields containing line breaks (CRLF), double quotes, and commas should be enclosed in double-quotes" | :white_check_mark: | 
2.7 | Escaping enclosing char | Enclosing chars within fields are escaped by doubling the char | "Fields containing line breaks (CRLF), double quotes, and commas should be enclosed in double-quotes" | :white_check_mark: | 
3 | MIME type registration | Not used | Describes requirements for MIME handling | N/A | 
N/A | Encoding | Read: UTF-8 (default), UTF-16BE, UTF-16LE, UTF-32BE, UTF-32LE, Latin-1, Windows-1252. Write: UTF-8 (default), Latin-1 | N/A (encoding requirements not mentioned) | N/A | 
N/A | Pre-header lines | Not supported (as of 2.7.0). For read-only purpose, read filter may be used to filter out the pre-header lines | N/A (not mentioned) | N/A | Some dsv-like files may have content before actual header row
N/A | Comment lines | Not supported (as of 2.7.0). For read-only purpose, read filter may be used to filter out lines that e.g. start with certain letter. | N/A (not mentioned) | N/A | 
N/A | Delimiter merging | Not supported (as of 2.7.0) | N/A (not mentioned) | N/A | For example LibreOffice offers such functionality when opening csv-file.

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

; Defines application level default for numeric precision of selection detail results.
; Default: -1 ( =roundtrippable precision )
TableEditorPropertyId_selectionDetailsResultPrecision = 6

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

; Defines minimum block size in bytes for read thread. Every read thread should
; have block size at least of this size in order to spread reading to separate threads.
; To allow multithreaded reading regardless of file size, set value to 0.
; To prevent multithreaded reading, instead of using this property consider setting CsvItemModel_defaultReadThreadCountMaximum to 1
; For file-specific control, see file specific configuration property readThreadBlockSizeMinimum
CsvItemModel_defaultReadThreadBlockSizeMinimum

; Defines default maximum count of read threads to be used when reading files, or with 0 to let implementation decide. In practice fewer threads may be used depending on factors such as file size and value of CsvItemModel_defaultReadThreadBlockSizeMinimum.
; For file-specific control, see file specific configuration property readThreadCountMaximum
CsvItemModel_defaultReadThreadCountMaximum

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

; Defines default limit for preview size for actions that offer preview in widgets
;     * For example if doing operation for million cells, for performance reasons
;       preview should not offer preview for all.
; Default value: 50
; Possible values: integer
CsvTableView_actionInputPreviewLimit=10

; -----------------------------------------------------------
; CsvTableViewChartDataSource

; Defines whether data source for table selection is allowed to cache data
;     Caching can significantly improve performance of graphs made from selection, drawback is increased memory usage.
; Possible values: 0, 1
; Default value: 1
; Note: Value is read only on application start, restart is needed to change setting
CsvTableViewChartDataSource_allowCaching=1

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
| columnsByIndex/\<1-based columnIndex\>/datatype | Defines how to interpret content in some operations, as of 2.3.0 only affects column sorting behaviour | _text_ (default), _number_, _datetime_ (since 2.6.0) | Since 2.3.0 |
| columnsByIndex/\<1-based columnIndex\>/width_pixels | Defines column width in pixels for given column | integer | |
| columnsByIndex/\<1-based columnIndex\>/visible | Defines whether column is visible | 0 or 1, default is 1 | Since 2.2.0 |
| columnsByIndex/\<1-based columnIndex\>/readOnly | Defines whether column is read-only | 0 or 1, default is 0 | Since 2.4.0 ([#129](https://github.com/tc3t/dfglib/issues/129)) |
| columnsByIndex/\<1-based columnIndex\>/stringToDoubleParserDefinition | Defines custom parser definition for string-to-double conversions, currently available only for datetime-typed columns | Format accepted by QDateTime | Since 2.6.0 ([#153](https://github.com/tc3t/dfglib/issues/153)) |
| encoding | When set, file is read assuming it to be of given encoding. | Latin1, UTF8, UTF16BE, UTF16LE, UTF32BE, UTF32LE, windows-1252 | This setting is used even if file has a BOM that indicates different encoding |
| enclosing_char | File is read interpreting the given character as enclosing character | ASCII-character as such or escaped format, e.g. \t or \x1f | |
| separator_char | File is read interpreting the given character as separator character | Like for enclosing_char | |
| end_of_line_type | File is read using end-of-line type | \n, \r\n, \r | |
| bom_writing | Whether to write BOM on save | 0, 1 | |
| properties/columnSortingCaseSensitive | Defines whether column sorting is case-sensitive | 0 or 1, default is 0 | Since 2.5.0 ([#142](https://github.com/tc3t/dfglib/issues/142)) |
| properties/columnSortingColumnIndex | Defines which column is sorted (1-based column index) | Valid column index | Since 2.5.0 ([#142](https://github.com/tc3t/dfglib/issues/142)) |
| properties/columnSortingEnabled | Defines whether columns can be sorted | 0 or 1, default is 0 | Since 2.5.0 ([#142](https://github.com/tc3t/dfglib/issues/142)) |
| properties/columnSortingOrder | Defines sort order for column defined by columnSortingColumnIndex | A or D (Ascending/Descending), default is A | Since 2.5.0 ([#142](https://github.com/tc3t/dfglib/issues/142)) |
| properties/completerColumns | see CsvItemModel_completerEnabledColumnIndexes | | |
| properties/completerEnabledSizeLimit | see CsvItemModel_completerEnabledSizeLimit | | |
| properties/editMode | Defines edit mode when opened | _readOnly_, _readWrite_<br>If empty, handled as if property was not present at all. | Since 2.1.0 |
| properties/filterText | Defines filter text to be set to UI filter controls |  | Since 2.6.0 ([#148](https://github.com/tc3t/dfglib/issues/148))
| properties/filterCaseSensitive | Defines case sensitivity setting to be set to UI filter controls | _0_, _1_ | Since 2.6.0 ([#148](https://github.com/tc3t/dfglib/issues/148))
| properties/filterSyntaxType | Defines syntax type to be set to UI filter controls | _Wildcard_, _Simple string_, _Regular expression_, _Json_ | Since 2.6.0 ([#148](https://github.com/tc3t/dfglib/issues/148))
| properties/filterColumn | Defines filter column to be set to UI filter controls | 1-based column index, 0 means _any_ | Since 2.6.0 ([#148](https://github.com/tc3t/dfglib/issues/148))
| properties/findText | Defines find text to be set to UI find controls |  | Since 2.6.0 ([#148](https://github.com/tc3t/dfglib/issues/148))
| properties/findCaseSensitive | Defines case sensitivity setting to be set to UI find controls | _0_, _1_ | Since 2.6.0 ([#148](https://github.com/tc3t/dfglib/issues/148))
| properties/findSyntaxType | Defines syntax type to be set to UI find controls | _Wildcard_, _Simple string_, _Regular expression_ | Since 2.6.0 ([#148](https://github.com/tc3t/dfglib/issues/148))
| properties/findColumn | Defines find column to be set to UI find controls | 1-based column index, 0 means _any_ | Since 2.6.0 ([#148](https://github.com/tc3t/dfglib/issues/148))
| properties/includeRows | Limits rows which are read from file by row index (0-based index, typically header is on row 0) |  | Since 1.5.0 |
| properties/includeColumns | Like includeRows, but for columns | | Since 1.5.0 |
| properties/initialScrollPosition | file-specific setting for CsvTableView_initialScrollPosition. | | Since 2.4.0 ([#130](https://github.com/tc3t/dfglib/issues/130))
| properties/readFilters | Defines content filters for read, i.e. ability to filter read rows by content. For example only rows that match a regular expression in certain column(s). | The same syntax as in UI, syntax guide is available from UI tooltip | Since 1.5.0 |
| properties/chartControls | If dfgQtTableEditor is built with chart feature, defines chart controls that are taken into use after load. | The same syntax as in UI, see [guide](../../dfg/qt/res/chartGuide.html) for detail | Since 1.6.0 |
| properties/chartPanelWidth | If dfgQtTableEditor is built with chart feature, defines chart panel width to be used with the associated document. | Width value, see syntax from TableEditor_chartPanelWidth | Since 1.8.1 ([#60](https://github.com/tc3t/dfglib/issues/60))
| properties/readThreadBlockSizeMinimum | file-specific setting for CsvItemModel_readThreadBlockSizeMinimum | non-negative integers | Since 2.4.0 ([#138](https://github.com/tc3t/dfglib/issues/138))
| properties/readThreadCountMaximum | file-specific setting for CsvItemModel_readThreadCountMaximum | non-negative integers | Since 2.4.0 ([#138](https://github.com/tc3t/dfglib/issues/138))
| properties/windowHeight | Defines request for window height when opening associated document, ignored if _windowMaximized_ is true. | Height value, see syntax from TableEditor_chartPanelWidth | Since 2.0.0 ([#86](https://github.com/tc3t/dfglib/issues/86))
| properties/windowWidth | Defines request for window width when opening associated document, ignored if _windowMaximized_ is true. | Width value, see syntax from TableEditor_chartPanelWidth | Since 2.0.0 ([#86](https://github.com/tc3t/dfglib/issues/86))
| properties/windowPosX | Defines request for window x position when opening associated document, only taken into account if either windowHeight or windowWidth is defined, and _windowMaximized_ is false. | x pixel position of top left corner, 0 for left. | Since 2.0.0 ([#86](https://github.com/tc3t/dfglib/issues/86))
| properties/windowPosY | Defines request for window y position when opening associated document, only taken into account if either windowHeight or windowWidth is defined, and _windowMaximized_ is false. | y pixel position of top left corner, 0 for top. | Since 2.0.0 ([#86](https://github.com/tc3t/dfglib/issues/86))
| properties/windowMaximized | Defines whether window should be maximized. | Since 2.6.0 ([#149](https://github.com/tc3t/dfglib/issues/149))
| properties/sqlQuery | For SQLite files, defines the query whose result is populated to table. | Valid SQLite query | Since 1.6.1 (commit [24c1ad78](https://github.com/tc3t/dfglib/commit/24c1ad78eac2a6f74b6ee1be0dede0d5645fef07)) |
| properties/selectionDetails | Defines selection details which are shown for every selection; i.e. basic indicators describing a selection such minimum and maximum value. Details are defined with a list of single line json-objects, where *id* field defines the detail. | Available built-in detail id's:<br>*average*, *cell_count_excluded*, *cell_count_included*, *is_sorted_num*, *median*, *max*, *min*, *sum*, *stddev_population*, *stddev_sample*, *variance*.<br>Since version 2.2.0 can also define custom details, for details see section [Custom selection details](#custom-selection-details) | Since 2.1.0 (commit [2d1c1d1b](https://github.com/tc3t/dfglib/commit/2d1c1d1b230a4d0f6dd8c18633a2af5ac20ea288)) |
| properties/selectionDetailsResultPrecision | Defines default for numeric precision of selection detail results. | [-1, 999]. Value -1 means default roundtrippable precision | Since 2.3.0 ([#123](https://github.com/tc3t/dfglib/issues/123)) |
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
,selectionDetails,"{ ""id"": ""sum"",""result_precision"":""2"" }
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

Summary of 3rd party code in dfgQtTableEditor (last revised 2023-04-26).

| Library      | License  |
| ------------- | ----- |
| [Boost](http://www.boost.org/) | [Boost software license](http://www.boost.org/LICENSE_1_0.txt)  |
| [fmtlib](https://github.com/fmtlib/fmt) | [BSD-2](../../dfg/str/fmtlib/format.h) |
| [muparser](https://github.com/beltoforion/muparser) | [BSD-2](../../dfg/math/muparser/muParser.h) | 
| [QCustomPlot](https://www.qcustomplot.com/) (disabled by default) | [GPLv3/commercial](https://www.qcustomplot.com/) |
| [Qt](https://www.qt.io/) | [Various](http://doc.qt.io/qt-5/licensing.html) |
| [UTF8-CPP](https://github.com/nemtrif/utfcpp) | [Boost software license](../../dfg/utf/utf8_cpp/utf8.h) |


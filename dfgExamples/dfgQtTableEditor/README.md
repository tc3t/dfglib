# dfgQtTableEditor

Example application for viewing and editing csv-files demonstrating features in dfglib.

## Building

### Version > 1.1.0:

Like earlier versions with following changes:
* Building with MSVC 2012 and 2013 is no longer supported.
* Qt 5.6 is minimum Qt version
* If building with QCustomPlot charting, requires Boost >= 1.70

### Version 1.1.0:

Like with version 1.0.0, but won't anymore build with MSVC 2010.

### Version 1.0.0 or earlier:
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

* 2020-07-22, [1.5.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_1.5.0)
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
* 2020-02-11, [1.1.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_1.1.0.0)
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
        

* 2019-09-05, [1.0.0](https://github.com/tc3t/dfglib/releases/tag/dfgQtTableEditor_1.0.0.0)

## Features

* Separator auto-detection: instead of e.g. relying on list separator defined in OS settings (like done by some spreadsheet applications), uses heuristics to determine separator character. Auto-detection supports comma (,), semicolon (;), tab (\t) and unit separator (\x1F).
* Text-oriented: content is shown as read, no data type interpretations are made.
* Easy to use basic filtering: Alt+F -> type filter string -> view shows only rows whose content (on any column) match with filter string. Matching syntax supports for example wildcard and regular expression.
* Filtered reading: Allows parts of file to be read:
    * Row index filtering (to read only given rows by index)
    * Column index filtering (to read only given columns by index)
    * Content filter: Arbitrary number of and/or combinations of text matchers similar to filter in UI.
* Diffing using 3rd party diff tool (manual configuration).
* Support for per-file configurations that can be used for example to define format characters and column widths in UI.
* When saving to file, tries to use the same format as what was used when reading the file (e.g. use the same separator character)
* Charting support using [QCustomPlot](https://www.qcustomplot.com/) (since version 1.5.0)
    * xy-graphs and histograms are supported (as of version 1.5.0).
    * json-based chart definition.
    * Has been tested to work with 50 million row, single column xy-graph. In a test machine chart update times in 50M line case were 65 s in non-cached case and 6 s in cached case; corresponding numbers in case of 1M lines were 2 s and 0.2 s.
    * Disabled by default due to licensing issues (GPL).
* No artificial row/column count restrictions. Note, though, that data structures and implementation in general are not designed for huge files, see below for some concrete examples. Underlying data structure is optimized for in-order walk through column content.
* Somewhat reasonable performance:
    * opening a 140 MB, 1000000 x 20 test csv-file with content "abcdef" in every cell, lasted less than 3 seconds with dfgQtTableEditor 1.0.0 default read options (and little over 1 second with manually given read options, most importantly disabled quote parsing), in LibreOffice 6.1.6.3 on the same machine read took over 30 seconds.
    * Since opening huge files is not a priority, limits of opening such files hasn't been thoroughly examined, but some examples for concreteness:
        * With version 1.5.0 and a rather ordinary Windows desktop machine (Intel i5 desktop CPU launched in 2013), opening a 1 GB file with 50 million rows and three 3 columns with content "abcdef" in every cell, lasted (in warm cache case) about 10 seconds with default options, about 6 seconds if using manual read options. When opened, application used about 3.0 GB of memory. With version 1.1.0 figures were 16s / 8s / 4.2 GB.
        * Opening a 3 GB, 1e6 x 1024 file with content ab in each cell lasted about 105 s with peak memory consumption of 14 GB (in version 1.5.0).
* Supported encodings:
    * Read: UTF-8, UTF-16BE, UTF-16LE, UTF-32BE, UTF-32LE, Latin-1, Windows-1252
        * Note: when file has no BOM, reading assumes Latin-1
    * Write: UTF-8, Latin-1
* Undo & redo and detailed undo control.
* Auto-completion (column-specific)
* Content generation: most of C++11 defined random number distribution available (since 1.1.0)

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

; -----------------------------------------------------------
; CsvItemModel

; Column for which to use auto-completion feature.
; To enable for all columns, use *
; To enable for selected columns, provide comma-separator list of column indexes
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
</pre>

### csv-file -specific configuration

File-specific configuration can be used for example to
* Read the file correctly (e.g. in case of less common format)
* Optimize the read speed by disabling unneeded parsing features
* Set UI column widths so that content shows in preferred way.

Configuration is defined through a .conf file in path \<csv-file path\>.conf. File can be generated from table view context menu from "Config" -> "Save config file...". The .conf file is a key-value map encoded into a csv-file:

* Supports URI-like lookup, where depth is implemented by increasing column index for key-item.
* Key is the combination of first item in a row and if not on the first column, prepended by most recent key in preceding column.

Available keys:

| Key (URI)      | Purpose  | Possible values | Notes |
| -------------  | -----    | ------          | ----- |
| columnsByIndex/ColumnIndexHere/width_pixels | Defines column width in pixels for given column | integer | |
| encoding | When set, file is read assuming it to be of given encoding. | Latin1, UTF8, UTF16BE, UTF16LE, UTF32BE, UTF32LE, windows-1252 | This setting is used even if file has a BOM that indicates different encoding |
| enclosing_char | File is read interpreting the given character as enclosing character | ASCII-character as such or escaped format, e.g. \t or \x1f | |
| separator_char | File is read interpreting the given character as separator character | Like for enclosing_char | |
| end_of_line_type | File is read using end-of-line type | \n, \r\n, \r | |
| bom_writing | Whether to write BOM on save | 0, 1 | |
| properties/completerColumns | see CsvItemModel_completerEnabledColumnIndexes | | |
| properties/completerEnabledSizeLimit | see CsvItemModel_completerEnabledSizeLimit | | |
| properties/includeRows | Limits rows which are read from file by row index |  | Since 1.5.0 |
| properties/includeColumns | Like includeRows, but for columns | | Since 1.5.0 |
| properties/readFilters | Defines content filters for read, i.e. ability to filter read rows by content. For example only rows that match a regular expression in certain column(s). | The same syntax as in UI, syntax guide is available from UI tooltip | Since 1.5.0 |
| properties/chartControls | If dfgQtTableEditor is built with chart feature, defines chart controls that are taken into use after load. | The same syntax as in UI, syntax guide is available from UI. | Since 1.6.0 |

#### Example .conf-file (might look better when opened in a csv-editor)
<pre>
bom_writing,1,,
columnsByIndex,,,
,0,,
,,width_pixels,400
,1,,
,,width_pixels,100
,2,,
,,width_pixels,200
enclosing_char,,,
encoding,UTF8,,
end_of_line_type,\n,,
separator_char,",",,
properties,,,
,completerColumns,"0,2",
,completerEnabledSizeLimit,10000000,
,includeRows,100:200
,includeColumns,0:5
,readFilters,"{""text"":""abc"", ""apply_columns"":""2""}"
,chartControls,"{""type"":""xy"",""data_source"":""table"",""x_source"":""column_name(date)"",""y_source"":""column_name(temperature)""}
</pre>

## Third party code

Summary of 3rd party code in dfgQtTableEditor (last revised 2020-07-22).

| Library      | License  |
| ------------- | ----- |
| [Boost](http://www.boost.org/) | [Boost software license](http://www.boost.org/LICENSE_1_0.txt)  |
| [fmtlib](https://github.com/fmtlib/fmt) | [BSD-2](../../dfg/str/fmtlib/format.h) |
| [QCustomPlot](https://www.qcustomplot.com/) (disabled by default) | [GPLv3/commercial](https://www.qcustomplot.com/) |
| [Qt](https://www.qt.io/) | [Various](http://doc.qt.io/qt-5/licensing.html) |
| [UTF8-CPP](https://github.com/nemtrif/utfcpp) | [Boost software license](../../dfg/utf/utf8_cpp/utf8.h) |


# dfgQtTableEditor

Example application for viewing and editing csv-files demonstrating features in dfglib.

## Building

### Version 0.9.9.x:
Requires basic C++11 support (as available since VC2010), Qt 5 and Boost (in include path).

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
* MSVC 2017.9, Qt 5.12.3, Boost 1.70, 64-bit (Windows 8.1)
* GCC 5.4.0, Qt 5.5.1, Boost 1.61, 32-bit (Ubuntu 16.04.6, 32-bit)
* GCC 7.4.0, Qt 5.9.5, Boost 1.65.1, 64-bit (Ubuntu 18.04.2, 64-bit)
* Clang 3.8.0, Qt 5.5.1, Boost 1.61, 32-bit (Ubuntu 16.04.6, 32-bit)
* Clang 6.0.0, Qt 5.9.5, Boost 1.65.1, 64-bit (Ubuntu 18.04.2, 64-bit)

Note that in Qt versions 5.10-5.12, keyboard shortcuts won't show as intended in context menu (for further details, see [QTBUG-61181](https://bugreports.qt.io/browse/QTBUG-61181), [QTBUG-71471](https://bugreports.qt.io/browse/QTBUG-71471)).

## Features

* Separator auto-detection: instead of e.g. relying on list separator defined in OS settings (like done by some spreadsheet applications), uses heuristics to determine separator character. Auto-detection supports comma (,), semicolon (;), tab (\t) and unit separator (\x1F).
* Text-oriented: content is shown as read, no data type interpretations are made.
* Easy to use basic filtering: Alt+F -> type filter string -> view shows only rows whose content (on any column) match with filter string. Matching syntax supports for example wildcard and regular expression.
* Diffing using 3rd party diff tool (manual configuration).
* Support for per-file configurations that can be used for example to define format characters and column widths in UI.
* When saving to file, tries to use the same format as what was used when reading the file (e.g. use the same separator character)
* Somewhat reasonable performance: opening a 140 MB, 1000000 x 20 test csv-file with content "abcdef" in every cell, lasted less than 4 seconds with dfgQtTableEditor default read options, in LibreOffice 6.1.6.3 on the same Windows desktop machine read took around 33 seconds (not counting the time needed to wait for the format confirm dialog).
* No artificial row/column count restrictions. Note, though, that data structures and implementation in general are not designed for huge files. Underlying data structure is optimized for in-order walk through column content.
* Supported encodings:
    * Read: UTF-8, UTF-16BE, UTF-16LE, UTF-32BE, UTF-32LE, Latin-1, Windows-1252
        * Note: when file has no BOM, reading assumes Latin-1
    * Write: UTF-8, Latin-1
* Undo & redo and detailed undo control.
* Auto-completion (column-specific)

## Tips

* For optimal read speed
    * Input should be in UTF-8 encoding and application should know this e.g. through file having BOM or through UI.
    * File should have no enclosing characters (i.e. quoting) and read options should have enclosing character as none and completer columns should be empty.

## Third party code

Summary of 3rd party code in dfgQtTableEditor (last revised 2019-06-13).

| Library      | License  |
| ------------- | ----- |
| [Boost](http://www.boost.org/) | [Boost software license](http://www.boost.org/LICENSE_1_0.txt)  |
| [fmtlib](https://github.com/fmtlib/fmt) (only in Windows build) | [BSD-2](../../dfg/str/fmtlib/format.h) |
| [Qt](https://www.qt.io/) | [Various](http://doc.qt.io/qt-5/licensing.html) |
| [UTF8-CPP](https://github.com/nemtrif/utfcpp) | [Boost software license](../../dfg/utf/utf8_cpp/utf8.h) |


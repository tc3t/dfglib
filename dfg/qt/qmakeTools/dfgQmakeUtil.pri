
# Sets C++ version to use in compilation.
# If no argument is given, sets to default dfglib version, otherwise expects format like "c++17", although this is also compiler-dependent
# Examples:
#     $$dfgSetCppVersion()
#     $$dfgSetCppVersion("c++17")
# Details about qmake functions: https://stackoverflow.com/questions/59995845/include-files-via-custom-qmake-function
defineReplace(dfgSetCppVersion) {
    cppVersion = $$1
    isEmpty(cppVersion): cppVersion = "c++17"

    # CONFIG += c++17 is supported only since 5.12, so some trickery for older Qt's
    # https://stackoverflow.com/questions/46610996/cant-use-c17-features-using-g-7-2-in-qtcreator
    # https://forum.qt.io/topic/142130/qmake-how-to-override-std-compiler-flag
    equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 12) {
        cxxOption = ""
        msvc: cxxOption += /std:$$cppVersion
        else: cxxOption += -std=$$cppVersion
        QMAKE_CXXFLAGS += $$cxxOption
        message("Setting C++ version to" $$cppVersion "using QMAKE_CXXFLAGS flag" $$cxxOption)
        export(QMAKE_CXXFLAGS)
    } else {
        message("Setting C++ version to" $$cppVersion "using CONFIG")
        # Note: at least with MinGW 11.2 and Qt 6.4.1, setting c++17 resulted to functionally equivalent
        #       but deprecated compile option -std=c++1z
        #       https://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html
        CONFIG += $$cppVersion
        CONFIG += strict_c++ # To prevent using e.g. -std=gnu++1z
        export(CONFIG)
    }

    return(notUsed) # If omitted, causes "error: Conditional must expand to exactly one word"
}

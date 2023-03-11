
# Sets C++ version to use in compilation.
# If no argument is given, sets to default dfglib version, otherwise expects format like "c++17", although this is also compiler-dependent
# Examples:
#     $$dfgSetCppVersion()
#     $$dfgSetCppVersion("c++17")
# Details about qmake functions: https://stackoverflow.com/questions/59995845/include-files-via-custom-qmake-function
defineReplace(dfgSetCppVersion) {
    cppVersion = $$1
    isEmpty(cppVersion): cppVersion = "c++17"
    message("Setting C++ version to" $$cppVersion)

    # CONFIG += c++17 is supported only since 5.12, so some trickery for older Qt's
    # https://stackoverflow.com/questions/46610996/cant-use-c17-features-using-g-7-2-in-qtcreator
    equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 12) {
        msvc: QMAKE_CXXFLAGS += /std:$$cppVersion
        else: QMAKE_CXXFLAGS += -std=$$cppVersion
        export(QMAKE_CXXFLAGS)
    } else {
        CONFIG += $$cppVersion
        export(CONFIG)
    }

    return(notUsed) # If omitted, causes "error: Conditional must expand to exactly one word"
}

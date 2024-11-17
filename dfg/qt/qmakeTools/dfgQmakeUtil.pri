
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


# Sets compile macros related to build type and defines variable 'debug_release_type' to either "debug" or "release"
defineReplace(dfgSetBuildTypeMacros) {
    # Semantics of CONFIG are a bit obscure:
    #   https://stackoverflow.com/questions/18164490/qmake-config-function-and-active-configuration
    #   https://stackoverflow.com/questions/5134245/how-to-set-different-qmake-configuration-depending-on-debug-release
    #   https://stackoverflow.com/questions/16961866/qmake-how-exactly-does-qmake-interpret-the-configdebug-debugrelease-synta
    debug_release_type = ""
    CONFIG(release, debug|release) {
        message("Config type is release")
        debug_release_type = "release"
    }
    CONFIG(debug, debug|release) {
        message("Config type is debug" )
        debug_release_type = "debug"
    }

    # At least in GCC/MinGW/Clang, release build through qmake (as of Qt 6.4) does not define NDEBUG
    # and consequently BuildTimeDetail_buildDebugReleaseType could treat release build as debug build
    # -> defining NDEBUG manually
    # https://stackoverflow.com/questions/29308589/does-qt-no-debug-cause-a-definition-of-ndebug

    msvc {
        # Nothing needed for msvc for now
    } else {
        # Having this in non-MSVC branch (instead of e.g. "gcc or mingw or clang") is coarse-grained.
        isEqual(debug_release_type, "release") {
            message("Build type is release -> adding NDEBUG to DEFINES")
            DEFINES += NDEBUG
            export(DEFINES)
        }
    }
    export(debug_release_type)
    return(notUsed) # If omitted, causes "error: Conditional must expand to exactly one word"
}

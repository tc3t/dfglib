#ifndef DFG_TEXTUALLOGIG_TYAKDPMX
#define DFG_TEXTUALLOGIG_TYAKDPMX

#if defined(_MSC_VER)
    // On GCC 15 include causes "#warning "<ciso646> is not a standard header since C++20"
    // On MSVC 2017 and/or is not available without some includes, but on MSVC 2019 compiles fine without.
    // For now simply including on MSVC to avoid warnings on GCC.
    #include <ciso646> // For textual logical operators (and, or, ...)
#endif // defined(_MSC_VER)

#endif

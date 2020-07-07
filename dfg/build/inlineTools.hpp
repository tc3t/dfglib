#pragma once

/*
    DFG_FORCEINLINE
        -Request for the compiler to try to inline a function, a stronger request than 'inline'.

    Note: Does not guarantee that function is actually inlined.
    Note: There may be different semantics depending on compiler: for example in VC2015 DFG_FORCEINLINE will not be effective on default debug options, but in GCC it may do the inlining in non-optimizated builds.
*/

#if defined(_MSC_VER)
    #define DFG_FORCEINLINE    __forceinline
#elif defined(__GNUC__)
    #define DFG_FORCEINLINE    __attribute__((always_inline)) inline
#else
    #define DFG_FORCEINLINE    inline // TODO: implement
#endif

// DFG_NOINLINE: request to not inline a function.
//      Usage example: DFG_NOINLINE int func() 
#if defined(_MSC_VER)
    #define DFG_NOINLINE    __declspec(noinline)
#elif defined(__GNUC__)
    #define DFG_NOINLINE    __attribute__((noinline))
#else
    #define DFG_NOINLINE
#endif

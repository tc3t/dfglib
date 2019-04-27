#pragma once

#include "../dfgDefs.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

    #ifdef _WIN32
        #define DFG_NATIVE_ENDOFLINE_STR	        "\r\n"
        #define DFG_NATIVE_ENDOFLINE_STR_LITERAL    "\\r\\n"
    #else
        //#pragma message( "Note: Using default native end-of-line string." )
        #define DFG_NATIVE_ENDOFLINE_STR	        "\n"
        #define DFG_NATIVE_ENDOFLINE_STR_LITERAL	"\\n"
    #endif

    enum EndOfLineType
    {
        EndOfLineTypeN,      // \n
        EndOfLineTypeRN,     // \r\n
        EndOfLineTypeR,      // \r
        EndOfLineTypeNative,
        EndOfLineTypeMixed,
    };

} } // module namespace

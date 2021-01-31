/**************************************************************************
*
* FormulaParser
* 
* Parser for mathematical formulas such as 1+2*3
* 
* Note: This is NOT a header-only implementation.
* 
***************************************************************************/

#pragma once

#include "../dfgDefs.hpp"
#include "../ReadOnlySzParam.hpp"
#include "../OpaquePtr.hpp"
#include <memory>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(math) {

    /*
    * Available function: (2020-11-01)
    *   Note: these are ones that are built-in in muparser, not yet revised so to be considered as implementation details
    *		sin, cos, tan
			asin, acos, atan, atan2
			sinh, cosh, tanh
			asinh, acosh, atanh
			log2, log10, log, ln (log == ln)
			exp, sqrt, sign, rint, abs
            sum, avg, min, max
    * Available constants (2020-11-01)
    *   Note: these are ones that are built-in in muparser, not yet revised so to be considered as implementation details
    *       _e
    *       _pi
    */ 
    class FormulaParser
    {
    public:
        FormulaParser();
        ~FormulaParser();
        using ReturnStatus = bool;

        ReturnStatus setFormula(const StringViewC sv); // Returns ReturnStatus that evaluates to true iff successful.
        // Returns ReturnStatus that evaluates to true iff successful.
        // If variable of given identifier already exists, old variable definition is replaced.
        // If a constant of given identifier already exists, operation fails.
        ReturnStatus defineVariable(const StringViewC sv, double* pVar);
        // Returns ReturnStatus that evaluates to true iff successful.
        // If const of given identifier already exists, old const definition is replaced.
        // If a variable of given identifier already exists, operation fails.
        ReturnStatus defineConstant(const StringViewC sv, double val); 
        double       evaluateFormulaAsDouble(); // If unable to evaluate, returns NaN

        // Convenience interface for evaluating simple formulas that have no variables.
        static double evaluateFormulaAsDouble(const StringViewC sv);

        DFG_OPAQUE_PTR_DECLARE();
    }; // class FormulaParser

}} // module namespace

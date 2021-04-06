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
#include <functional>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(math) {

    /*
    * Available functions: (2021-04-04)
    *   -C++11 cmath functions: cbrt, erf, erfc, hypot, tgamma
    *   -If compiled with C++17: assoc_laguerre, assoc_legendre, beta, comp_ellint_1, comp_ellint_2, comp_ellint_3, cyl_bessel_i, cyl_bessel_j, cyl_bessel_k, cyl_neumann,
    *                            ellint_1, ellint_2, ellint_3, expint, gcd, hermite, laguerre, legendre, lcm, riemann_zeta, sph_bessel, sph_legendre, sph_neumann.
    *   -time_epochMsec: returns milliseconds since 1970-01-01, i.e. epoch time in milliseconds
    *   -Built-in function from muparser:
    *		sin, cos, tan
			asin, acos, atan, atan2
			sinh, cosh, tanh
			asinh, acosh, atanh
			log2, log10, log, ln (log == ln)
			exp, sqrt, sign, rint, abs
            sum, avg, min, max
    * Available constants (2021-04-04)
    *   Note: these are ones that are built-in in muparser, not yet revised so to be considered as implementation details
    *       _e
    *       _pi
    */ 
    class FormulaParser
    {
    public:

        class ReturnStatus
        {
        public:
            class ErrorDetails;

            ~ReturnStatus();

            static ReturnStatus success() { return ReturnStatus(); }
            static ReturnStatus failure(const ErrorDetails& errorDetails);
            static ReturnStatus failure(const char* pszErrorMessage);
            operator bool() const { return m_spDetails == nullptr; }

            // Returns implementation specific error string.
            StringViewC errorString() const;
        
            // Implementation specific error code that is meaningful only if operator bool() returns false.
            int errorCode() const;

            // Returns pointer to implementation specific error object. Object is guaranteed valid as long as no non-const methods of 'this' are called.
            const void* internalErrorObjectPtr();

            std::shared_ptr<const ErrorDetails> m_spDetails;
        };

        FormulaParser();
        ~FormulaParser();

        using FuncType_D_0D = double(*)();
        using FuncType_D_1D = double(*)(double);
        using FuncType_D_2D = double(*)(double, double);
        using FuncType_D_3D = double(*)(double, double, double);
        using FuncType_D_4D = double(*)(double, double, double, double);

        static constexpr size_t maxFunctorCountPerType() { return 20; }

        ReturnStatus setFormula(const StringViewC sv); // Returns ReturnStatus that evaluates to true iff successful.

        // Calls setFormula() and returns evaluateFormulaAsDouble().
        double setFormulaAndEvaluateAsDouble(const StringViewC sv);

        // Returns ReturnStatus that evaluates to true iff successful.
        // If variable of given identifier already exists, old variable definition is replaced.
        // If a constant of given identifier already exists, operation fails.
        ReturnStatus defineVariable(const StringViewC sv, double* pVar);
        // Returns ReturnStatus that evaluates to true iff successful.
        // If const of given identifier already exists, old const definition is replaced.
        // If a variable of given identifier already exists, operation fails.
        ReturnStatus defineConstant(const StringViewC sv, double val);

        double       evaluateFormulaAsDouble(ReturnStatus* pReturnStatus = nullptr); // If unable to evaluate, returns NaN

        // Defines function with given identifier.
        // 'bAllowOptimization': if true, implementation is allowed to optimize
        //      calls so that e.g. func(1) + func(1) evaluate function only once and formula is calculated as 2*func(1).
        //      For functions whose return value is dependent on non-argument details (e.g. rand()), the flag must be false.
        //      If function has no arguments, flag should be true only for constant functions.
        // 
        // If function of given identifier already exists, old function definition is replaced.
        ReturnStatus defineFunction(const StringViewC& sv, FuncType_D_0D func, bool bAllowOptimization);
        ReturnStatus defineFunction(const StringViewC& sv, FuncType_D_1D func, bool bAllowOptimization);
        ReturnStatus defineFunction(const StringViewC& sv, FuncType_D_2D func, bool bAllowOptimization);
        ReturnStatus defineFunction(const StringViewC& sv, FuncType_D_3D func, bool bAllowOptimization);
        ReturnStatus defineFunction(const StringViewC& sv, FuncType_D_4D func, bool bAllowOptimization);

        // Defines function as std::function
        // NOTE: these functions are not thread safe and have obscure global restrictions, please read notes below if intending to use.
        //      -Underlying implementation doesn't natively provide a way to define stateful function so this is 
        //       implemented using a global list of standalone functions where each function refer to a global extra parameter.
        //      -For each function type, there can simultaneously be at most maxFunctorCountPerType() functors defined in all FormulaParser instances.
        //          -If having maximum count, this function returns failure.
        //      -All FormulaParser instances that use this function, should be handled from one thread only (e.g. deleting an instance that has used
        //       defineFunctor() from one thread while another instance in another thread calls defineFunctor() is not safe)
        ReturnStatus defineFunctor(const StringViewC& sv, std::function<double()> func, bool bAllowOptimization);
        ReturnStatus defineFunctor(const StringViewC& sv, std::function<double(double)> func, bool bAllowOptimization);
        ReturnStatus defineFunctor(const StringViewC& sv, std::function<double(double, double)> func, bool bAllowOptimization);

        // Defines random functions for this parser.
        // Note: uses defineFunctor() so it's limitations apply to this function.
        // Note: This disables formula optimizer.
        ReturnStatus defineRandomFunctions();

        // Calls given function for each defined function name, handler should return true to continue iteration, false to terminate.
        void forEachDefinedFunctionNameWhile(std::function<bool (const StringViewC&)> handler) const;

    private:
        template <class Func_T, class ExtraParam_T, class FuncType_T, size_t N>
        ReturnStatus defineFunctorImpl(ExtraParam_T (&extraParamArr)[N], FuncType_T (&funcArr)[N], const StringViewC& sv, Func_T&& func, bool bAllowOptimization);

        template <class Func_T>
        ReturnStatus defineFunctionImpl(const StringViewC& sv, Func_T func, bool bAllowOptimization);

    public:
        // Convenience interface for evaluating simple formulas that have no variables.
        static double evaluateFormulaAsDouble(const StringViewC sv);

        DFG_OPAQUE_PTR_DECLARE();
    }; // class FormulaParser

}} // module namespace

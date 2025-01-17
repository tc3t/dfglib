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
    *   -DateTime functions:
    *       -time_epochMsec          : input: none. Output: milliseconds since 1970-01-01, i.e. epoch time in milliseconds.
    *       -time_ISOdateToEpochSec  : input: ISO formatted datetime. output: epoch time of given datetime, NaN on error.
    *           -Note: If given DateTime does not have timezone specifier, returns NaN.
    *       -time_ISOdateToYearNum   : input: ISO formatted datetime. output: year of given datetime, NaN on error.
    *       -time_ISOdateToMonthNum  : input: ISO formatted datetime. output: month number in range 1-12, NaN on error.
    *       -time_ISOdateToDayOfMonth: input: ISO formatted datetime. output: day of given month, NaN on error.
    *       -time_ISOdateToDayOfWeek : input: ISO formatted datetime. output: day of week in range 0 (Sunday) - 6 (Saturday), NaN on error.
    *       -time_ISOdateToHour      : input: ISO formatted datetime. output: hour of given date time, NaN on error. If datetime does not have time part, returns 0.
    *       -time_ISOdateToMinute    : input: ISO formatted datetime. output: minute of given date time, NaN on error. If datetime does not have time part, returns 0.
    *       -time_ISOdateToSecond    : input: ISO formatted datetime. output: second of given date time, NaN on error. If datetime does not have time part, returns 0.
    *       -time_ISOdateToUtcOffsetInSeconds: input: ISO formatted datetime. output: UTC offset in seconds, positive from +hh:mm timezones. NaN if unknown.
    *
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

        // 1 = muparser
        static constexpr int backendType() { return 1; }

        FormulaParser();
        ~FormulaParser();

        using FuncType_D_0D = double(*)();
        using FuncType_D_1D = double(*)(double);
        using FuncType_D_2D = double(*)(double, double);
        using FuncType_D_3D = double(*)(double, double, double);
        using FuncType_D_4D = double(*)(double, double, double, double);

        using FuncType_D_1PSZ = double(*)(const char*);

        ReturnStatus setFormula(const StringViewC sv); // Returns ReturnStatus that evaluates to true iff successful. Note: successful return value doesn't guarantee that formula is even well formed, i.e. may return true even if formula has syntax errors.

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
        ReturnStatus defineFunction(const StringViewC& sv, FuncType_D_1PSZ func, bool bAllowOptimization);
        ReturnStatus defineFunction(const StringViewC& sv, FuncType_D_2D func, bool bAllowOptimization);
        ReturnStatus defineFunction(const StringViewC& sv, FuncType_D_3D func, bool bAllowOptimization);
        ReturnStatus defineFunction(const StringViewC& sv, FuncType_D_4D func, bool bAllowOptimization);

        // Defines function as std::function
        ReturnStatus defineFunctor(const StringViewC& sv, std::function<double()> func, bool bAllowOptimization);
        ReturnStatus defineFunctor(const StringViewC& sv, std::function<double(double)> func, bool bAllowOptimization);
        ReturnStatus defineFunctor(const StringViewC& sv, std::function<double(double, double)> func, bool bAllowOptimization);

        // Defines random functions for this parser.
        ReturnStatus defineRandomFunctions();

        // Calls given function for each defined function name, handler should return true to continue iteration, false to terminate.
        void forEachDefinedFunctionNameWhile(std::function<bool (const StringViewC&)> handler) const;

    private:
        template <class UserData_T, class Func_T>
        ReturnStatus defineFunctorImpl(const StringViewC& sv, Func_T&& func, bool bAllowOptimization);

        template <class Func_T>
        ReturnStatus defineFunctionImpl(const StringViewC& sv, Func_T func, bool bAllowOptimization);

    public:
        // Convenience interface for evaluating simple formulas that have no variables.
        static double evaluateFormulaAsDouble(const StringViewC sv);

        DFG_OPAQUE_PTR_DECLARE();
    }; // class FormulaParser

}} // module namespace

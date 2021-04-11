#include "FormulaParser.hpp"
#include <limits>
#include "../cont/SetVector.hpp"
#include "../rand/distributionHelpers.hpp"
#include "../rand.hpp"
#include <cmath>
#include <numeric>
#include <chrono>

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
    #include "muparser/muParser.h"
    #include "muparser/muParserBase.cpp"
    #include "muparser/muParser.cpp"
    #include "muparser/muParserBytecode.cpp"
    #include "muparser/muParserCallback.cpp"
    #include "muparser/muParserError.cpp"
    #include "muparser/muParserTokenReader.cpp"
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(math) { namespace muParserExt {

template <size_t N>
struct FunctorHelperT
{
    template <class Func_T>
    class ExtraParamT
    {
    public:
        operator bool() const
        {
            return (m_callback) ? true : false;
        }
        void reset()
        {
            m_callback = Func_T();
        }
        Func_T m_callback;
    };

    using ExtraParam0 = ExtraParamT<std::function<double()>>;
    using ExtraParam1 = ExtraParamT<std::function<double(double)>>;
    using ExtraParam2 = ExtraParamT<std::function<double(double, double)>>;

    FunctorHelperT()
    {
        fillFunctionPointers<N - 1>(std::integral_constant<bool, (N > 0)>());
    }

    static double extraParamCallHandler0(ExtraParam0& param)                     { return param.m_callback(); }
    static double extraParamCallHandler1(ExtraParam1& param, double a)           { return param.m_callback(a); }
    static double extraParamCallHandler2(ExtraParam2& param, double a, double b) { return param.m_callback(a, b); }

    template <size_t M> static double fD0D()                   { return extraParamCallHandler0(s_extraParams0[M]); }
    template <size_t M> static double fD1D(double a)           { return extraParamCallHandler1(s_extraParams1[M], a); }
    template <size_t M> static double fD2D(double a, double b) { return extraParamCallHandler2(s_extraParams2[M], a, b); }

    template <size_t M>
    static void fillFunctionPointers()
    {
        s_fp_D_0D[M] = &fD0D<M>;
        s_fp_D_1D[M] = &fD1D<M>;
        s_fp_D_2D[M] = &fD2D<M>;
    }

    template <size_t M>
    static void fillFunctionPointers(std::false_type)
    {
        fillFunctionPointers<M>();
    }

    template <size_t M>
    static void fillFunctionPointers(std::true_type)
    {
        fillFunctionPointers<M>();
        fillFunctionPointers<M - 1>(std::integral_constant<bool, (M - 1 > 0)>());
    }

    static ExtraParam0 s_extraParams0[N];
    static ExtraParam1 s_extraParams1[N];
    static ExtraParam2 s_extraParams2[N];

    static ::DFG_MODULE_NS(math)::FormulaParser::FuncType_D_0D s_fp_D_0D[N];
    static ::DFG_MODULE_NS(math)::FormulaParser::FuncType_D_1D s_fp_D_1D[N];
    static ::DFG_MODULE_NS(math)::FormulaParser::FuncType_D_2D s_fp_D_2D[N];
}; // class FunctorHelperT

template <size_t N> typename FunctorHelperT<N>::ExtraParam0 FunctorHelperT<N>::s_extraParams0[N];
template <size_t N> typename FunctorHelperT<N>::ExtraParam1 FunctorHelperT<N>::s_extraParams1[N];
template <size_t N> typename FunctorHelperT<N>::ExtraParam2 FunctorHelperT<N>::s_extraParams2[N];

template <size_t N> ::DFG_MODULE_NS(math)::FormulaParser::FuncType_D_0D FunctorHelperT<N>::s_fp_D_0D[N];
template <size_t N> ::DFG_MODULE_NS(math)::FormulaParser::FuncType_D_1D FunctorHelperT<N>::s_fp_D_1D[N];
template <size_t N> ::DFG_MODULE_NS(math)::FormulaParser::FuncType_D_2D FunctorHelperT<N>::s_fp_D_2D[N];

using FunctorHelper = FunctorHelperT<FormulaParser::maxFunctorCountPerType()>;
FunctorHelper gFunctorHelper;

template <class Cont_T, class ParamArr_T, size_t N>
void cleanUpFunctors(Cont_T& paramsToCleanUp, ParamArr_T(&paramArr)[N])
{
    if (paramsToCleanUp.empty())
        return;
    for (size_t i = 0; i < N; ++i)
    {
        if (!paramArr[i])
            continue;
        auto pParam = reinterpret_cast<uintptr_t>(&paramArr[i]);
        auto iter = paramsToCleanUp.find(pParam);
        if (iter != paramsToCleanUp.end())
        {
            paramArr[i].reset();
            paramsToCleanUp.erase(iter);
            if (paramsToCleanUp.empty())
                break;
        }
    }
}

} } } // namespace dfg::math::muParserExt

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(math) {
namespace DFG_DETAIL_NS {
   
class NoCallPreconditions
{
public:
    template <class ... Args_T>
    constexpr bool operator()(Args_T&& ...) const { return true; }
};

// Checks if all given arguments to operator() can be safely casted to corresponding type in Types_T...
template <class ... Types_T>
class TypeConvertibleChecker
{
public:
    template <class TypeTuple_T, size_t N, class T, class ... Args_T>
    static bool checkAndIterateIfNeeded(const std::integral_constant<size_t, N>, const T& val, Args_T&& ... args)
    {
        return (isFloatConvertibleTo<typename std::tuple_element<N, TypeTuple_T>::type>(val)
            &&
            checkAndIterateIfNeeded<TypeTuple_T>(std::integral_constant<size_t, N + 1>(), args...)
            );
    }

    template <class TypeTuple_T, class ... Args_T>
    static bool checkAndIterateIfNeeded(const std::integral_constant<size_t, sizeof...(Types_T)>, Args_T&& ...)
    {
        return true;
    }

    template <class ... Args_T>
    bool operator()(Args_T&& ... args) const
    {
        DFG_STATIC_ASSERT(sizeof...(Types_T) == sizeof...(Args_T), "Unexpected argument count");
        using TypeTuple = std::tuple<Types_T...>;
        return checkAndIterateIfNeeded<TypeTuple>(std::integral_constant<size_t, 0>(), args...);
    }
}; // TypeConvertibleChecker

// Using typedef to avoid preprocessor comma parsing problems.
using TccUD  = TypeConvertibleChecker<unsigned int, double>;
using TccUUD = TypeConvertibleChecker<unsigned int, unsigned int, double>;
using TccII  = TypeConvertibleChecker<int64, int64>;

// Some macro machinery to avoid using std-functions in invalid ways, e.g. defineFunction("hypot", std::hypot, true) fails in C++17 since std::hypot has (double, double) and (double, double, double) overloads.
#define DFG_TEMP_DEFINE_CALLABLE_1(FUNC, PRECONDITION_CHECKER, ARG_MORPHER) \
double caller_##FUNC(double a) \
{ \
    return (PRECONDITION_CHECKER()(a)) ? std::FUNC(ARG_MORPHER(a)) : std::numeric_limits<double>::quiet_NaN(); \
}

#define DFG_TEMP_DEFINE_CALLABLE_2(FUNC, PRECONDITION_CHECKER, ARG_MORPHER) \
double caller_##FUNC(double a, double b) \
{ \
    return (PRECONDITION_CHECKER()(a, b)) ? std::FUNC(ARG_MORPHER(a, b)) : std::numeric_limits<double>::quiet_NaN(); \
}

#define DFG_TEMP_DEFINE_CALLABLE_3(FUNC, PRECONDITION_CHECKER, ARG_MORPHER) \
double caller_##FUNC(double a, double b, double c) \
{ \
    return (PRECONDITION_CHECKER()(a, b, c)) ? std::FUNC(ARG_MORPHER(a, b, c)) : std::numeric_limits<double>::quiet_NaN(); \
}

#define DFG_TEMP_ARG_MORPHER_D(X)         a
#define DFG_TEMP_ARG_MORPHER_DD(X, Y)     a, b
#define DFG_TEMP_ARG_MORPHER_DDD(a, b, c) a, b, c
#define DFG_TEMP_ARG_MORPHER_UD(a,b)      static_cast<unsigned int>(a), b
#define DFG_TEMP_ARG_MORPHER_UUD(a,b,c)   static_cast<unsigned int>(a), static_cast<unsigned int>(b), c
#define DFG_TEMP_ARG_MORPHER_II(a,b)      static_cast<int64>(a), static_cast<int64>(b)

// C++11
DFG_TEMP_DEFINE_CALLABLE_1(cbrt,   NoCallPreconditions, DFG_TEMP_ARG_MORPHER_D) // Cubit root
DFG_TEMP_DEFINE_CALLABLE_1(erf,    NoCallPreconditions, DFG_TEMP_ARG_MORPHER_D)
DFG_TEMP_DEFINE_CALLABLE_1(erfc,   NoCallPreconditions, DFG_TEMP_ARG_MORPHER_D)
DFG_TEMP_DEFINE_CALLABLE_2(hypot,  NoCallPreconditions, DFG_TEMP_ARG_MORPHER_DD)
DFG_TEMP_DEFINE_CALLABLE_1(tgamma, NoCallPreconditions, DFG_TEMP_ARG_MORPHER_D)
// C++17
#if defined(__cpp_lib_math_special_functions) ||  (defined(__STDCPP_MATH_SPEC_FUNCS__) && (__STDCPP_MATH_SPEC_FUNCS__ >= 201003L))
    DFG_TEMP_DEFINE_CALLABLE_3(assoc_laguerre, TccUUD,              DFG_TEMP_ARG_MORPHER_UUD) // "If n or m is greater or equal to 128, the behavior is implementation-defined." (https://en.cppreference.com/w/cpp/numeric/special_functions/assoc_laguerre)
    DFG_TEMP_DEFINE_CALLABLE_3(assoc_legendre, TccUUD,              DFG_TEMP_ARG_MORPHER_UUD) // "If n is greater or equal to 128, the behavior is implementation-defined." (https://en.cppreference.com/w/cpp/numeric/special_functions/assoc_legendre)
    DFG_TEMP_DEFINE_CALLABLE_2(beta,           NoCallPreconditions, DFG_TEMP_ARG_MORPHER_DD)
    DFG_TEMP_DEFINE_CALLABLE_1(comp_ellint_1,  NoCallPreconditions, DFG_TEMP_ARG_MORPHER_D)
    DFG_TEMP_DEFINE_CALLABLE_1(comp_ellint_2,  NoCallPreconditions, DFG_TEMP_ARG_MORPHER_D)
    DFG_TEMP_DEFINE_CALLABLE_2(comp_ellint_3,  NoCallPreconditions, DFG_TEMP_ARG_MORPHER_DD)
    DFG_TEMP_DEFINE_CALLABLE_2(cyl_bessel_i,   NoCallPreconditions, DFG_TEMP_ARG_MORPHER_DD) // "If v>=128, the behavior is implementation-defined " (https://en.cppreference.com/w/cpp/numeric/special_functions/cyl_bessel_i)
    DFG_TEMP_DEFINE_CALLABLE_2(cyl_bessel_j,   NoCallPreconditions, DFG_TEMP_ARG_MORPHER_DD) // "If v>=128, the behavior is implementation-defined " (https://en.cppreference.com/w/cpp/numeric/special_functions/cyl_bessel_j)
    DFG_TEMP_DEFINE_CALLABLE_2(cyl_bessel_k,   NoCallPreconditions, DFG_TEMP_ARG_MORPHER_DD) // "If v>=128, the behavior is implementation-defined " (https://en.cppreference.com/w/cpp/numeric/special_functions/cyl_bessel_k)
    DFG_TEMP_DEFINE_CALLABLE_2(cyl_neumann,    NoCallPreconditions, DFG_TEMP_ARG_MORPHER_DD) // "If v>=128, the behavior is implementation-defined " (https://en.cppreference.com/w/cpp/numeric/special_functions/cyl_neumann)
    DFG_TEMP_DEFINE_CALLABLE_2(ellint_1,       NoCallPreconditions, DFG_TEMP_ARG_MORPHER_DD)
    DFG_TEMP_DEFINE_CALLABLE_2(ellint_2,       NoCallPreconditions, DFG_TEMP_ARG_MORPHER_DD)
    DFG_TEMP_DEFINE_CALLABLE_3(ellint_3,       NoCallPreconditions, DFG_TEMP_ARG_MORPHER_DDD)
    DFG_TEMP_DEFINE_CALLABLE_1(expint,         NoCallPreconditions, DFG_TEMP_ARG_MORPHER_D)
    DFG_TEMP_DEFINE_CALLABLE_2(gcd,            TccII,               DFG_TEMP_ARG_MORPHER_II)
    DFG_TEMP_DEFINE_CALLABLE_2(hermite,        TccUD,               DFG_TEMP_ARG_MORPHER_UD) // "If n is greater or equal than 128, the behavior is implementation-defined" (https://en.cppreference.com/w/cpp/numeric/special_functions/hermite)
    DFG_TEMP_DEFINE_CALLABLE_2(laguerre,       TccUD,               DFG_TEMP_ARG_MORPHER_UD) // "If n is greater or equal than 128, the behavior is implementation-defined" (https://en.cppreference.com/w/cpp/numeric/special_functions/laguerre)
    DFG_TEMP_DEFINE_CALLABLE_2(legendre,       TccUD,               DFG_TEMP_ARG_MORPHER_UD) // "If n is greater or equal than 128, the behavior is implementation-defined" (https://en.cppreference.com/w/cpp/numeric/special_functions/legendre)
    DFG_TEMP_DEFINE_CALLABLE_2(lcm,            TccII,               DFG_TEMP_ARG_MORPHER_II)
    DFG_TEMP_DEFINE_CALLABLE_1(riemann_zeta,   NoCallPreconditions, DFG_TEMP_ARG_MORPHER_D)
    DFG_TEMP_DEFINE_CALLABLE_2(sph_bessel,     TccUD,               DFG_TEMP_ARG_MORPHER_UD) // "If n>=128, the behavior is implementation-defined" (https://en.cppreference.com/w/cpp/numeric/special_functions/sph_bessel)
    DFG_TEMP_DEFINE_CALLABLE_3(sph_legendre,   TccUUD,              DFG_TEMP_ARG_MORPHER_UUD) // "If l>=128, the behavior is implementation-defined" (https://en.cppreference.com/w/cpp/numeric/special_functions/sph_legendre)
    DFG_TEMP_DEFINE_CALLABLE_2(sph_neumann,    TccUD,               DFG_TEMP_ARG_MORPHER_UD) // "f n>=128, the behavior is implementation-defined" (https://en.cppreference.com/w/cpp/numeric/special_functions/sph_neumann)
#endif

#undef DFG_TEMP_DEFINE_CALLABLE_1
#undef DFG_TEMP_DEFINE_CALLABLE_2
#undef DFG_TEMP_DEFINE_CALLABLE_3

#undef DFG_TEMP_ARG_MORPHER_D
#undef DFG_TEMP_ARG_MORPHER_DD
#undef DFG_TEMP_ARG_MORPHER_DDD
#undef DFG_TEMP_ARG_MORPHER_UD
#undef DFG_TEMP_ARG_MORPHER_UUD
#undef DFG_TEMP_ARG_MORPHER_II

double time_epochMsec()
{
    using namespace std::chrono;
    return static_cast<double>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

} } } // dfg:math::DFG_DETAIL_NS


class DFG_MODULE_NS(math)::FormulaParser::ReturnStatus::ErrorDetails : public dfg_mu::ParserError
{
public:
    using BaseClass = dfg_mu::ParserError;
    ErrorDetails(const dfg_mu::ParserError& parserError)
        : BaseClass(parserError)
    {}
}; // class ErrorDetails

::DFG_MODULE_NS(math)::FormulaParser::ReturnStatus::~ReturnStatus() = default;

auto ::DFG_MODULE_NS(math)::FormulaParser::ReturnStatus::failure(const ErrorDetails & errorDetails) -> ReturnStatus
{
    ReturnStatus rv;
    rv.m_spDetails = std::make_shared<ErrorDetails>(errorDetails);
    return rv;
}

auto ::DFG_MODULE_NS(math)::FormulaParser::ReturnStatus::failure(const char* pszErrorMessage) -> ReturnStatus
{
    ReturnStatus rv;
    dfg_mu::ParserError parserError((pszErrorMessage) ? pszErrorMessage : "<error message not available>");
    rv.m_spDetails = std::make_shared<ErrorDetails>(std::move(parserError));
    return rv;
}

const void* ::DFG_MODULE_NS(math)::FormulaParser::ReturnStatus::internalErrorObjectPtr()
{
    return (this->m_spDetails.get());
}

auto ::DFG_MODULE_NS(math)::FormulaParser::ReturnStatus::errorString() const -> StringViewC
{
    return (this->m_spDetails) ? this->m_spDetails->GetMsg() : StringViewC();
}

int ::DFG_MODULE_NS(math)::FormulaParser::ReturnStatus::errorCode() const
{
    return (this->m_spDetails) ? this->m_spDetails->GetCode() : 0;
}

DFG_OPAQUE_PTR_DEFINE(DFG_MODULE_NS(math)::FormulaParser)
{
    using RandEngT = decltype(::DFG_MODULE_NS(rand)::createDefaultRandEngineRandomSeeded());
    dfg_mu::Parser m_parser;
    ::DFG_MODULE_NS(cont)::SetVector<uintptr_t> m_ownedParams;
    std::unique_ptr<RandEngT> m_spRandEng;
};


::DFG_MODULE_NS(math)::FormulaParser::FormulaParser()
{
#define DFG_TEMP_DEFINE_FUNC(NAME) defineFunction(#NAME, ::DFG_MODULE_NS(math)::DFG_DETAIL_NS::caller_##NAME, true)

    defineFunction("time_epochMsec", ::DFG_MODULE_NS(math)::DFG_DETAIL_NS::time_epochMsec, false);

    // C++11
    DFG_TEMP_DEFINE_FUNC(cbrt);
    DFG_TEMP_DEFINE_FUNC(erf);
    DFG_TEMP_DEFINE_FUNC(erfc);
    DFG_TEMP_DEFINE_FUNC(hypot);
    DFG_TEMP_DEFINE_FUNC(tgamma);
    
    // C++17
#if defined(__cpp_lib_math_special_functions) ||  (defined(__STDCPP_MATH_SPEC_FUNCS__) && (__STDCPP_MATH_SPEC_FUNCS__ >= 201003L))
    DFG_TEMP_DEFINE_FUNC(assoc_laguerre);
    DFG_TEMP_DEFINE_FUNC(assoc_legendre);
    DFG_TEMP_DEFINE_FUNC(beta);
    DFG_TEMP_DEFINE_FUNC(comp_ellint_1);
    DFG_TEMP_DEFINE_FUNC(comp_ellint_2);
    DFG_TEMP_DEFINE_FUNC(comp_ellint_3);
    DFG_TEMP_DEFINE_FUNC(cyl_bessel_i);
    DFG_TEMP_DEFINE_FUNC(cyl_bessel_j);
    DFG_TEMP_DEFINE_FUNC(cyl_bessel_k);
    DFG_TEMP_DEFINE_FUNC(cyl_neumann);
    DFG_TEMP_DEFINE_FUNC(ellint_1);
    DFG_TEMP_DEFINE_FUNC(ellint_2);
    DFG_TEMP_DEFINE_FUNC(ellint_3);
    DFG_TEMP_DEFINE_FUNC(expint);
    DFG_TEMP_DEFINE_FUNC(gcd);
    DFG_TEMP_DEFINE_FUNC(hermite);
    DFG_TEMP_DEFINE_FUNC(laguerre);
    DFG_TEMP_DEFINE_FUNC(legendre);
    DFG_TEMP_DEFINE_FUNC(lcm);
    DFG_TEMP_DEFINE_FUNC(riemann_zeta);
    DFG_TEMP_DEFINE_FUNC(sph_bessel);
    DFG_TEMP_DEFINE_FUNC(sph_legendre);
    DFG_TEMP_DEFINE_FUNC(sph_neumann);
#endif

#undef DFG_TEMP_DEFINE_FUNC
}

::DFG_MODULE_NS(math)::FormulaParser::~FormulaParser()
{
    using namespace ::DFG_MODULE_NS(math)::muParserExt;
    // Cleaning up owned functors for globals
    {
        auto& ownedParams = DFG_OPAQUE_REF().m_ownedParams;
        cleanUpFunctors(ownedParams, FunctorHelper::s_extraParams0);
        cleanUpFunctors(ownedParams, FunctorHelper::s_extraParams1);
        cleanUpFunctors(ownedParams, FunctorHelper::s_extraParams2);
    }
}   

template <class Func_T, class ExtraParam_T, class FuncType_T, size_t N>
auto ::DFG_MODULE_NS(math)::FormulaParser::defineFunctorImpl(ExtraParam_T(&extraParamArr)[N], FuncType_T(&funcArr)[N], const StringViewC& sv, Func_T&& func, bool bAllowOptimization) -> ReturnStatus
{
    using namespace ::DFG_MODULE_NS(math)::muParserExt;
    size_t i = 0;
    // Finding free param slot
    for (; i < N; ++i)
    {
        if (!extraParamArr[i])
            break;
    }
    if (i >= N)
        return ReturnStatus::failure("No free slots"); // No free slot found
    const auto rv = defineFunction(sv, funcArr[i], bAllowOptimization);
    if (rv)
    {
        extraParamArr[i].m_callback = std::forward<Func_T>(func);
        DFG_OPAQUE_REF().m_ownedParams.insert(reinterpret_cast<uintptr_t>(&extraParamArr[i]));
    }
    return rv;
}

auto ::DFG_MODULE_NS(math)::FormulaParser::defineFunctor(const StringViewC& sv, std::function<double()> func, const bool bAllowOptimization) -> ReturnStatus
{
    using namespace ::DFG_MODULE_NS(math)::muParserExt;
    return defineFunctorImpl(FunctorHelper::s_extraParams0, FunctorHelper::s_fp_D_0D, sv, std::move(func), bAllowOptimization);
}

auto ::DFG_MODULE_NS(math)::FormulaParser::defineFunctor(const StringViewC& sv, std::function<double(double)> func, const bool bAllowOptimization) -> ReturnStatus
{
    using namespace ::DFG_MODULE_NS(math)::muParserExt;
    return defineFunctorImpl(FunctorHelper::s_extraParams1, FunctorHelper::s_fp_D_1D, sv, std::move(func), bAllowOptimization);
}

auto ::DFG_MODULE_NS(math)::FormulaParser::defineFunctor(const StringViewC& sv, std::function<double(double, double)> func, const bool bAllowOptimization) -> ReturnStatus
{
    using namespace ::DFG_MODULE_NS(math)::muParserExt;
    return defineFunctorImpl(FunctorHelper::s_extraParams2, FunctorHelper::s_fp_D_2D, sv, std::move(func), bAllowOptimization);
}

auto ::DFG_MODULE_NS(math)::FormulaParser::setFormula(const StringViewC sv) -> ReturnStatus
{
    try
    {
        DFG_OPAQUE_REF().m_parser.SetExpr(sv.toString());
        return ReturnStatus::success();
    }
    catch(const dfg_mu::Parser::exception_type& e)
    {
        return ReturnStatus::failure(e);
    }
}

double ::DFG_MODULE_NS(math)::FormulaParser::setFormulaAndEvaluateAsDouble(const StringViewC sv)
{
    setFormula(sv);
    return this->evaluateFormulaAsDouble();
}

auto ::DFG_MODULE_NS(math)::FormulaParser::defineVariable(const StringViewC sv, double* pVar) -> ReturnStatus
{
    if (!pVar)
        return ReturnStatus::failure("Variable pointer is null");
    try
    {
        DFG_OPAQUE_REF().m_parser.DefineVar(sv.toString(), pVar);
        return ReturnStatus::success();
    }
    catch (const dfg_mu::Parser::exception_type& e)
    {
        return ReturnStatus::failure(e);
    }
}

auto ::DFG_MODULE_NS(math)::FormulaParser::defineConstant(const StringViewC sv, const double val) -> ReturnStatus
{
    const auto sIdentifier = sv.toString();
    auto& parser = DFG_OPAQUE_REF().m_parser;
    if (parser.GetVar().find(sIdentifier) != parser.GetVar().end())
        return ReturnStatus::failure("Identifier already exists");
    try
    {
        parser.DefineConst(sIdentifier, val);
        return ReturnStatus::success();
    }
    catch (const dfg_mu::Parser::exception_type& e)
    {
        return ReturnStatus::failure(e);
    }
}

template <class Func_T>
auto ::DFG_MODULE_NS(math)::FormulaParser::defineFunctionImpl(const StringViewC& sv, Func_T func, const bool bAllowOptimization) -> ReturnStatus
{
    auto& parser = DFG_OPAQUE_REF().m_parser;
    try
    {
        parser.DefineFun(sv.toString(), func, bAllowOptimization);
        return ReturnStatus::success();
    }
    catch (const dfg_mu::Parser::exception_type& e)
    {
        return ReturnStatus::failure(e);
    }
}

auto ::DFG_MODULE_NS(math)::FormulaParser::defineFunction(const StringViewC& sv, FuncType_D_0D func, bool bAllowOptimization) -> ReturnStatus { return defineFunctionImpl(sv, func, bAllowOptimization); }
auto ::DFG_MODULE_NS(math)::FormulaParser::defineFunction(const StringViewC& sv, FuncType_D_1D func, bool bAllowOptimization) -> ReturnStatus { return defineFunctionImpl(sv, func, bAllowOptimization); }
auto ::DFG_MODULE_NS(math)::FormulaParser::defineFunction(const StringViewC& sv, FuncType_D_2D func, bool bAllowOptimization) -> ReturnStatus { return defineFunctionImpl(sv, func, bAllowOptimization); }
auto ::DFG_MODULE_NS(math)::FormulaParser::defineFunction(const StringViewC& sv, FuncType_D_3D func, bool bAllowOptimization) -> ReturnStatus { return defineFunctionImpl(sv, func, bAllowOptimization); }
auto ::DFG_MODULE_NS(math)::FormulaParser::defineFunction(const StringViewC& sv, FuncType_D_4D func, bool bAllowOptimization) -> ReturnStatus { return defineFunctionImpl(sv, func, bAllowOptimization); }

double ::DFG_MODULE_NS(math)::FormulaParser::evaluateFormulaAsDouble(ReturnStatus* pReturnStatus)
{
    try
    {
        const auto val = DFG_OPAQUE_REF().m_parser.Eval();
        const auto nResultCount = DFG_OPAQUE_REF().m_parser.GetNumResults();
        if (nResultCount != 1) // muparser supports evaluating multiple expressions, e.g. 1,1+2 -> 1, 3. FormulaParser does not support such, so considering multiresult evaluations invalid.
        {
            if (pReturnStatus)
                *pReturnStatus = ReturnStatus::failure("Unexpected result count");
            return std::numeric_limits<double>::quiet_NaN();
        }
        if (pReturnStatus)
            *pReturnStatus = ReturnStatus::success();
        return val;
    }
    catch (const dfg_mu::Parser::exception_type& e)
    {
        if (pReturnStatus)
            *pReturnStatus = ReturnStatus::failure(e);
        return std::numeric_limits<double>::quiet_NaN();
    }
}

double ::DFG_MODULE_NS(math)::FormulaParser::evaluateFormulaAsDouble(const StringViewC sv)
{
    FormulaParser parser;
    return (parser.setFormula(sv)) ? parser.evaluateFormulaAsDouble() : std::numeric_limits<double>::quiet_NaN();
}

namespace
{
    template <class RandEng_T>
    struct DistributionAdder
    {
        DistributionAdder(::DFG_MODULE_NS(math)::FormulaParser& rParser, RandEng_T& rRandEng)
            : m_rParser(rParser)
            , m_rRandEng(rRandEng)
        {}

        template <class Distr_T> void defineFunctor(const ::DFG_ROOT_NS::StringViewC& svName)
        {
            const auto rv = m_rParser.defineFunctor(svName, ::DFG_MODULE_NS(rand)::DistributionFunctor<Distr_T>(&m_rRandEng).toStdFunction(), false);
            m_bAllGood = m_bAllGood && (rv == true);
        }

#define DFG_TEMP_DEFINE_DISTR_HANDLER(TYPE, NAME) \
        template <class T> void operator()(const TYPE<T>*) { defineFunctor<TYPE<T>>(NAME); }

        DFG_TEMP_DEFINE_DISTR_HANDLER(std::uniform_int_distribution, "rand_uniformInt");
        DFG_TEMP_DEFINE_DISTR_HANDLER(std::binomial_distribution, "rand_binomial");
        void operator()(const std::bernoulli_distribution*) { defineFunctor<std::bernoulli_distribution>("rand_bernoulli"); }
        DFG_TEMP_DEFINE_DISTR_HANDLER(::DFG_MODULE_NS(rand)::NegativeBinomialDistribution, "rand_negBinomial");
        DFG_TEMP_DEFINE_DISTR_HANDLER(std::geometric_distribution, "rand_geometric");
        DFG_TEMP_DEFINE_DISTR_HANDLER(std::poisson_distribution, "rand_poisson");
        // Real valued
        DFG_TEMP_DEFINE_DISTR_HANDLER(std::uniform_real_distribution, "rand_uniformReal");
        DFG_TEMP_DEFINE_DISTR_HANDLER(std::normal_distribution, "rand_normal");
        DFG_TEMP_DEFINE_DISTR_HANDLER(std::cauchy_distribution, "rand_cauchy");
        DFG_TEMP_DEFINE_DISTR_HANDLER(std::exponential_distribution, "rand_exponential");
        DFG_TEMP_DEFINE_DISTR_HANDLER(std::gamma_distribution, "rand_gamma");
        DFG_TEMP_DEFINE_DISTR_HANDLER(std::weibull_distribution, "rand_weibull");
        DFG_TEMP_DEFINE_DISTR_HANDLER(std::extreme_value_distribution, "rand_extremeValue");
        DFG_TEMP_DEFINE_DISTR_HANDLER(std::lognormal_distribution, "rand_logNormal");
        DFG_TEMP_DEFINE_DISTR_HANDLER(std::chi_squared_distribution, "rand_chiSquared");
        DFG_TEMP_DEFINE_DISTR_HANDLER(std::fisher_f_distribution, "rand_fisherF");
        DFG_TEMP_DEFINE_DISTR_HANDLER(std::student_t_distribution, "rand_studentT");

        template <class T> void operator()(const T*)
        {
            DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(T, "There are more distributions available that what is handled above. If those are not needed, add an empty handler.");
        }
#undef DFG_TEMP_DEFINE_DISTR_HANDLER

        ::DFG_MODULE_NS(math)::FormulaParser& m_rParser;
        RandEng_T& m_rRandEng;
        bool m_bAllGood = true;
    };
}

auto ::DFG_MODULE_NS(math)::FormulaParser::defineRandomFunctions() -> ReturnStatus
{
    auto& opaqueThis = DFG_OPAQUE_REF();
    if (opaqueThis.m_spRandEng)
        return ReturnStatus::failure("Random engine not available");
    using RandEngT = std::remove_reference<decltype(opaqueThis)>::type::RandEngT;
    opaqueThis.m_parser.EnableOptimizer(false); // muParser doesn't seem to respect optimize-flag (https://github.com/beltoforion/muparser/issues/93), so must turn off optimizer completely.
    opaqueThis.m_spRandEng.reset(new RandEngT(::DFG_MODULE_NS(rand)::createDefaultRandEngineRandomSeeded()));
    DistributionAdder<RandEngT> adder(*this, *opaqueThis.m_spRandEng);
    ::DFG_MODULE_NS(rand)::DFG_DETAIL_NS::forEachDistributionType(adder);
    return (adder.m_bAllGood) ? ReturnStatus::success() : ReturnStatus::failure("Failed to add distribution");
}

void ::DFG_MODULE_NS(math)::FormulaParser::forEachDefinedFunctionNameWhile(std::function<bool (const StringViewC&)> handler) const
{
    auto pOpaqueThis = DFG_OPAQUE_PTR();
    if (!pOpaqueThis || !handler)
        return;
    const auto& funcDefs = pOpaqueThis->m_parser.GetFunDef();
    for (const auto& def : funcDefs)
    {
        if (!handler(def.first))
            break;
    }
}

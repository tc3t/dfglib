#include "FormulaParser.hpp"
#include <limits>
#include "../cont/SetVector.hpp"

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


DFG_OPAQUE_PTR_DEFINE(DFG_MODULE_NS(math)::FormulaParser)
{
    dfg_mu::Parser m_parser;
    ::DFG_MODULE_NS(cont)::SetVector<uintptr_t> m_ownedParams;
};

::DFG_MODULE_NS(math)::FormulaParser::FormulaParser() = default;

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
        return false; // No free slot found
    extraParamArr[i].m_callback = std::forward<Func_T>(func);
    DFG_OPAQUE_REF().m_ownedParams.insert(reinterpret_cast<uintptr_t>(&extraParamArr[i]));
    return defineFunction(sv, funcArr[i], bAllowOptimization);
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
        return true;
    }
    catch(const dfg_mu::Parser::exception_type& /*e*/)
    {
        return false;
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
        return false;
    try
    {
        DFG_OPAQUE_REF().m_parser.DefineVar(sv.toString(), pVar);
        return true;
    }
    catch (const dfg_mu::Parser::exception_type& e)
    {
        DFG_UNUSED(e);
        return false;
    }
}

auto ::DFG_MODULE_NS(math)::FormulaParser::defineConstant(const StringViewC sv, const double val) -> ReturnStatus
{
    const auto sIdentifier = sv.toString();
    auto& parser = DFG_OPAQUE_REF().m_parser;
    if (parser.GetVar().find(sIdentifier) != parser.GetVar().end())
        return false;
    try
    {
        parser.DefineConst(sIdentifier, val);
        return true;
    }
    catch (const dfg_mu::Parser::exception_type& e)
    {
        DFG_UNUSED(e);
        return false;
    }
}

template <class Func_T>
auto ::DFG_MODULE_NS(math)::FormulaParser::defineFunctionImpl(const StringViewC& sv, Func_T func, const bool bAllowOptimization) -> ReturnStatus
{
    auto& parser = DFG_OPAQUE_REF().m_parser;
    try
    {
        parser.DefineFun(sv.toString(), func, bAllowOptimization);
        return true;
    }
    catch (const dfg_mu::Parser::exception_type& e)
    {
        DFG_UNUSED(e);
        return false;
    }
}

auto ::DFG_MODULE_NS(math)::FormulaParser::defineFunction(const StringViewC& sv, FuncType_D_0D func, bool bAllowOptimization) -> ReturnStatus { return defineFunctionImpl(sv, func, bAllowOptimization); }
auto ::DFG_MODULE_NS(math)::FormulaParser::defineFunction(const StringViewC& sv, FuncType_D_1D func, bool bAllowOptimization) -> ReturnStatus { return defineFunctionImpl(sv, func, bAllowOptimization); }
auto ::DFG_MODULE_NS(math)::FormulaParser::defineFunction(const StringViewC& sv, FuncType_D_2D func, bool bAllowOptimization) -> ReturnStatus { return defineFunctionImpl(sv, func, bAllowOptimization); }
auto ::DFG_MODULE_NS(math)::FormulaParser::defineFunction(const StringViewC& sv, FuncType_D_3D func, bool bAllowOptimization) -> ReturnStatus { return defineFunctionImpl(sv, func, bAllowOptimization); }
auto ::DFG_MODULE_NS(math)::FormulaParser::defineFunction(const StringViewC& sv, FuncType_D_4D func, bool bAllowOptimization) -> ReturnStatus { return defineFunctionImpl(sv, func, bAllowOptimization); }

double ::DFG_MODULE_NS(math)::FormulaParser::evaluateFormulaAsDouble()
{
    try
    {
        const auto val = DFG_OPAQUE_REF().m_parser.Eval();
        return val;
    }
    catch (const dfg_mu::Parser::exception_type& e)
    {
        DFG_UNUSED(e);
        return std::numeric_limits<double>::quiet_NaN();
    }
}

double ::DFG_MODULE_NS(math)::FormulaParser::evaluateFormulaAsDouble(const StringViewC sv)
{
    FormulaParser parser;
    return (parser.setFormula(sv)) ? parser.evaluateFormulaAsDouble() : std::numeric_limits<double>::quiet_NaN();
}

#include "FormulaParser.hpp"
#include <limits>

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
    #include "muparser/muParser.h"
    #include "muparser/muParserBase.cpp"
    #include "muparser/muParser.cpp"
    #include "muparser/muParserBytecode.cpp"
    #include "muparser/muParserCallback.cpp"
    #include "muparser/muParserError.cpp"
    #include "muparser/muParserTokenReader.cpp"
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS

DFG_OPAQUE_PTR_DEFINE(DFG_MODULE_NS(math)::FormulaParser)
{
    dfg_mu::Parser m_parser;
};

::DFG_MODULE_NS(math)::FormulaParser::FormulaParser() = default;
::DFG_MODULE_NS(math)::FormulaParser::~FormulaParser() = default;

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

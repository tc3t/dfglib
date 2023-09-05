#pragma once

#include "../dfgDefs.hpp"
#include "../ReadOnlySzParam.hpp"
#include <limits>

class QDateTime;
class QTime;
class QString;

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(charts)
{
    class ChartDataType;
} } // module charts

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt) {

namespace DFG_DETAIL_NS
{
    double tableCellDateToDouble(QDateTime&& dt);
    double tableCellTimeToDouble(const QTime& t);
    double tableCellDateToDoubleWithColumnTypeHandling(QDateTime&& dt, const ::DFG_MODULE_NS(charts)::ChartDataType& dataTypeHint, ::DFG_MODULE_NS(charts)::ChartDataType* pInterpretedInputDataType);
}

double stringToDouble(const QString& s, double returnValueOnConversionFailure = std::numeric_limits<double>::quiet_NaN());
double stringToDouble(const StringViewSzC& sv, double returnValueOnConversionFailure = std::numeric_limits<double>::quiet_NaN());
double tableCellStringToDouble(
    const StringViewSzUtf8& sv,
    ::DFG_MODULE_NS(charts)::ChartDataType * pInterpretedInputDataType = nullptr,
    double returnValueOnConversionFailure = std::numeric_limits<double>::quiet_NaN());

} } // module qt

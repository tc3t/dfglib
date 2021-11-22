#pragma once

#include "../dfgDefs.hpp"
#include "../ReadOnlySzParam.hpp"

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
}

double stringToDouble(const QString& s);
double stringToDouble(const StringViewSzC& sv);
double tableCellStringToDouble(const StringViewSzUtf8& sv, ::DFG_MODULE_NS(charts)::ChartDataType * pInterpretedInputDataType = nullptr);

} } // module qt

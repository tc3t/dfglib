#include "stringConversions.hpp"
#include "../charts/commonChartTools.hpp"
#include "qtIncludeHelpers.hpp"
#include "qtBasic.hpp"
#include "../alg.hpp"
#include "../str/strTo.hpp"

#include <limits>
#include <regex>

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QDateTime>
    #include <QString>
    #include <QTime>
DFG_END_INCLUDE_QT_HEADERS



DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

namespace DFG_DETAIL_NS
{
    double tableCellDateToDouble(QDateTime&& dt)
    {
        if (dt.isValid())
        {
            const auto timeSpec = dt.timeSpec();
            if (timeSpec == Qt::LocalTime)
                dt.setTimeSpec(Qt::UTC);
            return static_cast<double>(dt.toMSecsSinceEpoch()) / 1000.0;
        }
        else
            return std::numeric_limits<double>::quiet_NaN();
    }

    double tableCellTimeToDouble(const QTime& t)
    {
        return (t.isValid()) ? static_cast<double>(t.msecsSinceStartOfDay()) / 1000.0 : std::numeric_limits<double>::quiet_NaN();
    }
} // namespace DFG_DETAIL_NS

double stringToDouble(const QString& s)
{
    bool b;
    const auto v = s.toDouble(&b);
    return (b) ? v : std::numeric_limits<double>::quiet_NaN();
}

double stringToDouble(const StringViewSzC& sv)
{
    bool bOk;
    auto rv = ::DFG_MODULE_NS(str)::strTo<double>(sv, &bOk);
    return (bOk) ? rv : std::numeric_limits<double>::quiet_NaN();
}

double tableCellStringToDouble(const StringViewSzUtf8& svUtf8, ::DFG_MODULE_NS(charts)::ChartDataType* pInterpretedInputDataType)
{
    using ChartDataType = ::DFG_MODULE_NS(charts)::ChartDataType;

    // Raw view so that can use indexing
    const StringViewC s(svUtf8.beginRaw(), svUtf8.endRaw());

    if (pInterpretedInputDataType)
        *pInterpretedInputDataType = ChartDataType::unknown;

    const auto setInterpretedDataType = [=](const ChartDataType& dataType)
    {
        if (pInterpretedInputDataType)
            *pInterpretedInputDataType = dataType;
    };

    const auto dateToDoubleAndColumnTypeHandling = [&](QDateTime&& dt, ChartDataType dataTypeHint)
    {
        const auto& time = dt.time();
        if (dataTypeHint.isDateNoTzType() && time.msecsSinceStartOfDay() == 0 && dataTypeHint != ChartDataType::dateOnlyYearMonth)
            dataTypeHint = ChartDataType::dateOnly;
        else if (time.msec() == 0)
        {
            if (dataTypeHint == ChartDataType::dateAndTimeMillisecond)
                dataTypeHint = ChartDataType::dateAndTime;
            else if (dataTypeHint == ChartDataType::dateAndTimeMillisecondTz)
                dataTypeHint = ChartDataType::dateAndTimeTz;
        }
        setInterpretedDataType(dataTypeHint);
        return DFG_DETAIL_NS::tableCellDateToDouble(std::move(dt));
    };

    const auto viewToQString = [&](const StringViewC& sv) { return ::DFG_MODULE_NS(qt)::viewToQString(StringViewUtf8(SzPtrUtf8(sv.begin()), SzPtrUtf8(sv.end()))); };

    const auto isTzStartChar = [](const QChar& c) { return ::DFG_MODULE_NS(alg)::contains("Z+-", c.toLatin1()); };
    // TODO: add parsing for fractional part longer than 3 digits.
    if (s.size() >= 8 && s[4] == '-' && s[7] == '-') // Something starting with ????-??-?? (ISO 8601, https://en.wikipedia.org/wiki/ISO_8601)
    {
        // size 19 is yyyy-MM-ddThh:mm:ss
        if (s.size() >= 19 && s[13] == ':' && s[16] == ':' && ::DFG_MODULE_NS(alg)::contains("T ", s[10])) // Case ????-??-??[T ]hh:mm:ss[.zzz][Z|HH:00]
        {
            // size 23 is yyyy-mm-ssThh:mm:ss.zzz
            if (s.size() >= 23 && s[19] == '.')
            {
                // Timezone specifier after milliseconds?
                if (s.size() >= 24 && isTzStartChar(s[23]))
                    return dateToDoubleAndColumnTypeHandling(QDateTime::fromString(viewToQString(s), Qt::ISODateWithMs), ChartDataType::dateAndTimeMillisecondTz);
                else
                    return dateToDoubleAndColumnTypeHandling(QDateTime::fromString(viewToQString(s), QString("yyyy-MM-dd%1hh:mm:ss.zzz").arg(s[10])), ChartDataType::dateAndTimeMillisecond);
            }
            else if (s.size() >= 20 && isTzStartChar(s[19]))
                return dateToDoubleAndColumnTypeHandling(QDateTime::fromString(viewToQString(s), Qt::ISODate), ChartDataType::dateAndTimeTz);
            else
                return dateToDoubleAndColumnTypeHandling(QDateTime::fromString(viewToQString(s), QString("yyyy-MM-dd%1hh:mm:ss").arg(s[10])), ChartDataType::dateAndTime);
        }
        else if (s.size() == 13 && s[10] == ' ') // Case: "yyyy-MM-dd Wd". where Wd is two char weekday indicator.
        {
            auto sQstring = viewToQString(s);
            return dateToDoubleAndColumnTypeHandling(QDateTime::fromString(sQstring, QString("yyyy-MM-dd'%1'").arg(sQstring.mid(10, 3))), ChartDataType::dateOnly);
        }
        else if (s.size() == 10) // Case: "yyyy-MM-dd"
            return dateToDoubleAndColumnTypeHandling(QDateTime::fromString(viewToQString(s), "yyyy-MM-dd"), ChartDataType::dateOnly);
        else
            return std::numeric_limits<double>::quiet_NaN();
    }

    // yyyy-mm
    if (s.size() == 7 && s[4] == '-')
        return dateToDoubleAndColumnTypeHandling(QDateTime::fromString(viewToQString(s), "yyyy-MM"), ChartDataType::dateOnlyYearMonth);

    // [ww] [d]d.[m]m.yyyy
    if (s.length() >= 8 && s[s.length() - 5] == '.')
    {
        std::regex regex(R"((?:^|^\w\w )(\d{1,2})(?:\.)(\d{1,2})(?:\.)(\d\d\d\d)$)");
        std::cmatch baseMatch;
        if (std::regex_match(svUtf8.beginRaw(), baseMatch, regex) && baseMatch.size() == 4)
        {
            const auto asInt = [](const std::csub_match& subMatch) { return ::DFG_MODULE_NS(str)::strTo<int>(StringViewC(subMatch.first, subMatch.second)); };
            // 0 has entire match, so actual captures start from index 1.
            return dateToDoubleAndColumnTypeHandling(QDateTime(QDate(asInt(baseMatch[3]), asInt(baseMatch[2]), asInt(baseMatch[1])), QTime(0, 0)), ChartDataType::dateOnly);

        }
    }

    if (s.size() >= 8 && s[2] == ':' && s[5] == ':')
    {
        if (s.size() >= 10 && s[8] == '.')
        {
            setInterpretedDataType(ChartDataType::dayTimeMillisecond);
            return DFG_DETAIL_NS::tableCellTimeToDouble(QTime::fromString(viewToQString(s), "hh:mm:ss.zzz"));
        }
        else
        {
            setInterpretedDataType(ChartDataType::dayTime);
            return DFG_DETAIL_NS::tableCellTimeToDouble(QTime::fromString(viewToQString(s), "hh:mm:ss"));
        }
    }
    if (!::DFG_MODULE_NS(alg)::contains(s, ','))
        return stringToDouble(svUtf8.asUntypedView()); // Not using s directly because at the time of writing it caused redundant string-object to be created due to shortcomings in strTo().
    else
    {
        auto s2 = viewToQString(s);
        s2.replace(',', '.'); // Hack: to make comma-localized values such as "1,2" be interpreted as 1.2
        return stringToDouble(s2);
    }
}

} } // module qt

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

    void setInterpretedDataType(const ::DFG_MODULE_NS(charts)::ChartDataType& dataType, ::DFG_MODULE_NS(charts)::ChartDataType* pInterpretedInputDataType)
    {
        if (pInterpretedInputDataType)
            *pInterpretedInputDataType = dataType;
    };

    double tableCellDateToDoubleWithColumnTypeHandling(QDateTime&& dt, const ::DFG_MODULE_NS(charts)::ChartDataType& dataTypeHintInit, ::DFG_MODULE_NS(charts)::ChartDataType* pInterpretedInputDataType)
    {
        using ChartDataType = ::DFG_MODULE_NS(charts)::ChartDataType;
        const auto& time = dt.time();
        ChartDataType dataTypeHint = dataTypeHintInit;
        if (dataTypeHint.isDateNoTzType() && time.msecsSinceStartOfDay() == 0 && dataTypeHint != ChartDataType::dateOnlyYearMonth)
            dataTypeHint = ChartDataType::dateOnly;
        else if (time.msec() == 0)
        {
            if (dataTypeHint == ChartDataType::dateAndTimeMillisecond)
                dataTypeHint = ChartDataType::dateAndTime;
            else if (dataTypeHint == ChartDataType::dateAndTimeMillisecondTz)
                dataTypeHint = ChartDataType::dateAndTimeTz;
        }
        setInterpretedDataType(dataTypeHint, pInterpretedInputDataType);
        return DFG_DETAIL_NS::tableCellDateToDouble(std::move(dt));
    }

    // Tries to parse datetime of format [ww] [d]d.[m]m.yyyy[ [h]h:mm[:ss][.zzz]]
    // Returns pair (parse success flag, date value as double).
    std::pair<bool, double> tryDmyParse(const StringViewC sv, ::DFG_MODULE_NS(charts)::ChartDataType* pInterpretedInputDataType)
    {
        using ChartDataType = ::DFG_MODULE_NS(charts)::ChartDataType;

        const std::pair<bool, double> rvInvalid(false, std::numeric_limits<double>::quiet_NaN());
        const auto nDateEndPos = [&]() // nDateEndPos gets index of possible end of date part, in valid formats either space after yyyy or end of string.
        {
            for (size_t i = sv.size(); i > 8; --i)
            {
                const auto nPos = i - 1u;
                if (sv[nPos] == ' ')
                    return nPos;
            }
            return sv.size();
        }();
        if (nDateEndPos < 8 || sv[nDateEndPos - 5] != '.' || (sv[nDateEndPos - 7] != '.' && sv[nDateEndPos - 8] != '.'))
            return rvInvalid;

        // Capture indexes
        //   0: whole match
        //   1: day
        //   2: month
        //   3: year
        //   4: time part as whole
        //   5: ss.zzz
        //   6: .zzz
        std::regex regex(R"((?:^|^\w\w )(\d{1,2})(?:\.)(\d{1,2})(?:\.)(\d\d\d\d)($| \d{1,2}:\d\d($|:\d\d($|\.\d\d\d))))");
        std::cmatch baseMatch;
        const auto bRegExMatch = std::regex_match(sv.beginRaw(), baseMatch, regex);

        if (!bRegExMatch || baseMatch.size() < 7 || baseMatch[3].length() != 4)
            return rvInvalid;

        const auto asInt = [](const std::csub_match& subMatch) { return ::DFG_MODULE_NS(str)::strTo<int>(StringViewC(subMatch.first, subMatch.second)); };

        const QDate datePart = QDate(asInt(baseMatch[3]), asInt(baseMatch[2]), asInt(baseMatch[1]));
        QTime timePart(0, 0);
        ChartDataType::DataType dataType = ChartDataType::dateOnly;

        const auto nTimePartStart = baseMatch.position(4);
        const auto svTimePart = sv.substr_start(nTimePartStart);
        ::DFG_MODULE_NS(charts)::ChartDataType timeType;
        const auto svTimePartUtf8 = StringUtf8::fromRawString(svTimePart.begin(), svTimePart.end());
        if (baseMatch[4].length() > 0)
        {
            const auto timeValueMs = 1000 * tableCellStringToDouble(svTimePartUtf8, &timeType);
            if (timeType == ChartDataType::dayTimeMillisecond)
                dataType = ChartDataType::DataType::dateAndTimeMillisecond;
            else
                dataType = ChartDataType::DataType::dateAndTime;
            int timeAsInt = 0;
            if (::DFG_MODULE_NS(math)::isFloatConvertibleTo<int>(timeValueMs, &timeAsInt))
                timePart = QTime::fromMSecsSinceStartOfDay(timeAsInt);
            else
                return rvInvalid;
        }
        return std::pair(true, DFG_DETAIL_NS::tableCellDateToDoubleWithColumnTypeHandling(QDateTime(datePart, timePart), dataType, pInterpretedInputDataType));
    }

    std::pair<bool, double> tryHhMmSsParseImpl(const StringViewC sv, ::DFG_MODULE_NS(charts)::ChartDataType* pInterpretedInputDataType, const QString& sFormat, ::DFG_MODULE_NS(charts)::ChartDataType dataType)
    {
        using ChartDataType = ::DFG_MODULE_NS(charts)::ChartDataType;

        const auto t = QTime::fromString(untypedViewToQStringAsUtf8(sv), sFormat);
        const auto val = tableCellTimeToDouble(t);
        const auto bOk = !::DFG_MODULE_NS(math)::isNan(val);
        if (!bOk)
            dataType = ChartDataType::unknown;
        setInterpretedDataType(dataType, pInterpretedInputDataType);
        return { bOk, val };
    }

    // Tries to parse [h]h:mm[:ss[.zzz]]
    std::pair<bool, double> tryHhMmSsParse(const StringViewC sv, ::DFG_MODULE_NS(charts)::ChartDataType* pInterpretedInputDataType)
    {
        using ChartDataType = ::DFG_MODULE_NS(charts)::ChartDataType;

        if (sv.size() >= 12 && sv[2] == ':' && sv[5] == ':' && sv[8] == '.')
            return tryHhMmSsParseImpl(sv, pInterpretedInputDataType, QStringLiteral("hh:mm:ss.zzz"), ChartDataType::dayTimeMillisecond);
        else if (sv.size() >= 8 && sv[2] == ':' && sv[5] == ':')
            return tryHhMmSsParseImpl(sv, pInterpretedInputDataType, QStringLiteral("hh:mm:ss"), ChartDataType::dayTime);
        else if (sv.size() >= 5 && sv[2] == ':')
            return tryHhMmSsParseImpl(sv, pInterpretedInputDataType, QStringLiteral("hh:mm"), ChartDataType::dayTime);
        else if (sv.size() >= 11 && sv[1] == ':' && sv[4] == ':' && sv[7] == '.')
            return tryHhMmSsParseImpl(sv, pInterpretedInputDataType, QStringLiteral("h:mm:ss.zzz"), ChartDataType::dayTimeMillisecond);
        else if (sv.size() >= 7 && sv[1] == ':' && sv[4] == ':')
            return tryHhMmSsParseImpl(sv, pInterpretedInputDataType, QStringLiteral("h:mm:ss"), ChartDataType::dayTime);
        else if (sv.size() >= 4 && sv[1] == ':')
            return tryHhMmSsParseImpl(sv, pInterpretedInputDataType, QStringLiteral("h:mm"), ChartDataType::dayTime);

        return std::pair<bool, double>{false, std::numeric_limits<double>::quiet_NaN()};
    }

} // namespace DFG_DETAIL_NS

double stringToDouble(const QString& s, const double returnValueOnConversionFailure)
{
    bool b;
    const auto v = s.toDouble(&b);
    return (b) ? v : returnValueOnConversionFailure;
}

double stringToDouble(const StringViewSzC& sv, const double returnValueOnConversionFailure)
{
    bool bOk;
    auto rv = ::DFG_MODULE_NS(str)::strTo<double>(sv, &bOk);
    return (bOk) ? rv : returnValueOnConversionFailure;
}

double tableCellStringToDouble(const StringViewSzUtf8& svUtf8,
    ::DFG_MODULE_NS(charts)::ChartDataType* pInterpretedInputDataType,
    const double returnValueOnConversionFailure)
{
    using ChartDataType = ::DFG_MODULE_NS(charts)::ChartDataType;

    // Raw view so that can use indexing, also skipping leading whitespaces.
    const StringViewC s = [&]()
    {
        StringViewC sv(svUtf8.beginRaw(), svUtf8.endRaw());
        while (!sv.empty() && sv.front() == ' ')
            sv.pop_front();
        return sv;
    }();

    if (pInterpretedInputDataType)
        *pInterpretedInputDataType = ChartDataType::unknown;

    const auto dateToDoubleAndColumnTypeHandling = [&](QDateTime&& dt, ChartDataType dataTypeHint)
    {
        return DFG_DETAIL_NS::tableCellDateToDoubleWithColumnTypeHandling(std::move(dt), dataTypeHint, pInterpretedInputDataType);
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

    // [ww] [d]d.[m]m.yyyy [[h]h:mm[:ss][.zzz]]
    {
        const auto [bOk, val] = DFG_DETAIL_NS::tryDmyParse(s, pInterpretedInputDataType);
        if (bOk)
            return val;
    }

    // [h]h:mm[:ss[.zzz]]
    {
        const auto [bOk, val] = DFG_DETAIL_NS::tryHhMmSsParse(s, pInterpretedInputDataType);
        if (bOk)
            return val;
    }

    if (!::DFG_MODULE_NS(alg)::contains(s, ','))
        return stringToDouble(svUtf8.asUntypedView(), returnValueOnConversionFailure); // Not using s directly because at the time of writing it caused redundant string-object to be created due to shortcomings in strTo().
    else
    {
        auto s2 = viewToQString(s);
        s2.replace(',', '.'); // Hack: to make comma-localized values such as "1,2" be interpreted as 1.2
        return stringToDouble(s2, returnValueOnConversionFailure);
    }
}

} } // module qt

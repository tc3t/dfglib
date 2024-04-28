#pragma once

#include "../dfgDefs.hpp"
#include "../ReadOnlySzParam.hpp"
#include "../cont/IntervalSet.hpp"
#include "../cont/IntervalSetSerialization.hpp"
#include "../cont/MapVector.hpp"
#include "../str/string.hpp"
#include <algorithm>

#include "qtIncludeHelpers.hpp"
#include "PatternMatcher.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QString>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{
    // Implements single json-definable string matching item supporting:
    //      -Multiple match patterns: wildcard, fixed, reg_exp
    //      -Case sensitivity
    //      -Construction from json-definition
    class StringMatchDefinition
    {
    public:
        typedef ::DFG_ROOT_NS::StringUtf8 Utf8String;

        explicit StringMatchDefinition(
            QString matchString,
            Qt::CaseSensitivity caseSensitivity,
            PatternMatcher::PatternSyntax patternSyntax,
            const bool bNegate = false)
          : m_matchString(std::move(matchString))
          , m_caseSensitivity(caseSensitivity)
          , m_patternSyntax(patternSyntax)
          , m_bNegate(bNegate)
        {
            initCache();
        }

        static QString jsonExampleMinimal() { return R"({"text": "match string"})"; }

        /**
         * @brief Creates StringMatchDefinition from json object
         * @param jsonObject json object from which values of the following keys as read.
         *      "type": {wildcard, fixed, reg_exp}
         *              Defines type of this filter
         *              Default: wildcard
         *      "case_sensitive": {true, false}
         *              Defines whether this filter is case sensitive or not.
         *              Default: false (=case insensitive)
         *      "text": filter text
         *              Defines actual filter text whose meaning depends on filter type
         *              Default: empty
         *      "negate": {false, true}
         *              Defines whether matching is negated, i.e. with negate true-match is false and false-match is true.
         *              Default: false
         * 
         * @return StringMatchDefinition based on given json.
         */
        static StringMatchDefinition fromJson(const QJsonObject& jsonObject);

        /** Convenience function for creating from json string, see details from other overload. In case of invalid json, returns makeMatchEverythingMatcher() */
        static std::pair<StringMatchDefinition, std::string> fromJson(const StringViewUtf8& sv);

        /**
         * @brief Creates StringMatchDefinition that matches everything
         * @return StringMatchDefinition that matches everything
         */
        static StringMatchDefinition makeMatchEverythingMatcher();

        bool hasMatchString() const
        {
            return !m_matchString.isEmpty();
        }

        void setMatchString(QString s)
        {
            m_matchString = std::move(s);
            initCache();
        }

        bool isMatchWith(const QString& s) const
        {
            return applyNegateIfNeeded(!m_matchString.isEmpty() && m_regExp.isMatchingWith(s));
        }

        bool isMatchWith(const StringViewUtf8 sv) const
        {
            if (!m_sSimpleSubStringMatch.empty())
            {
                const auto& rawSubString = m_sSimpleSubStringMatch.rawStorage();
                const bool b = rawSubString.size() <= sv.size()
                       &&
                       std::search(sv.beginRaw(), sv.endRaw(),
                                   rawSubString.cbegin(), rawSubString.cend()
                                    ) != sv.endRaw();
                return applyNegateIfNeeded(b);
            }
            else
                return isMatchWith(QString::fromUtf8(sv.dataRaw(), static_cast<int>(sv.length())));
        }

        const QString& matchString() const
        {
            return m_matchString;
        }

        Qt::CaseSensitivity caseSensitivity() const
        {
            return m_caseSensitivity;
        }

        PatternMatcher::PatternSyntax patternSyntax() const
        {
            return m_patternSyntax;
        }

        bool isNegated() const
        {
            return m_bNegate;
        }

    private:
        bool applyNegateIfNeeded(const bool b) const
        {
            return (m_bNegate) ? !b : b;
        }

        void initCache()
        {
            m_regExp = PatternMatcher(m_matchString, m_caseSensitivity, m_patternSyntax);

            const auto isSpecialWildcardChar = [](const QChar& c) { return c == '*' || c == '?' || c == '[' || c == ']'; };

            // TODO: handle other pattern syntaxes as well (e.g. RegExp without special characters could be done with substring matching).
            const bool bCanDoSubStringMatching = m_caseSensitivity == Qt::CaseSensitive &&
                                        ((m_patternSyntax == PatternMatcher::FixedString)
                                        ||
                                        (m_patternSyntax == PatternMatcher::Wildcard &&
                                         !std::any_of(m_matchString.cbegin(), m_matchString.cend(), isSpecialWildcardChar)));
            if (bCanDoSubStringMatching)
            {
                const auto utf8Bytes = m_matchString.toUtf8();
                m_sSimpleSubStringMatch = Utf8String(SzPtrUtf8R(utf8Bytes.cbegin()), SzPtrUtf8R(utf8Bytes.cend()));
            }
            else
                m_sSimpleSubStringMatch.clear();
        }

    private:
        QString m_matchString;
        Qt::CaseSensitivity m_caseSensitivity;
        PatternMatcher::PatternSyntax m_patternSyntax;
        PatternMatcher m_regExp;
        bool m_bNegate = false;
        Utf8String m_sSimpleSubStringMatch; // Stores the utf8-encoded substring to search if applicable. This is an optimization to avoid creating redundant QString-objects.
    }; // class StringMatchDefinition


    // Compared to StringMatchDefinition, this class provides "apply filter on row/column" controls and parsing of and_group.
    class TableStringMatchDefinition : public StringMatchDefinition
    {
    public:
        using BaseClass = StringMatchDefinition;
        using Index = int; // Should be identical to CsvItemModel::Index
        using IntervalSet = ::DFG_MODULE_NS(cont)::IntervalSet<int>;

        TableStringMatchDefinition(BaseClass&& base) :
            BaseClass(std::move(base))
        {}

        static QString jsonExampleFullSingle() { return R"({"text": "match string", "type": "reg_exp", "case_sensitive": true, "negate": false, "apply_rows":"1:3;5", "apply_columns":"1;4;6", "and_group":"a"})"; }
        static QString makeColumnFilterAndGroupId() { return QString("__column_filters"); }

        // Returns TableStringMatchDefinition and id of and_group.
        static std::pair<TableStringMatchDefinition, std::string> fromJson(const StringViewUtf8& sv, const int nUserToInternalRowOffset, const int nUserToInternalColumnOffset)
        {
            QJsonParseError parseError;
            auto doc = QJsonDocument::fromJson(QByteArray(sv.dataRaw(), sv.sizeAsInt()), &parseError);
            if (doc.isNull())
                return std::make_pair(TableStringMatchDefinition(StringMatchDefinition::makeMatchEverythingMatcher()), std::string());
            const auto jsonObject = doc.object();
            auto rv = std::pair<TableStringMatchDefinition, std::string>(BaseClass::fromJson(jsonObject), std::string(jsonObject.value("and_group").toString().toUtf8().data()));
            const auto iterRows = jsonObject.find(QLatin1String("apply_rows"));
            const auto iterColumns = jsonObject.find(QLatin1String("apply_columns"));
            if (iterRows != jsonObject.end())
                rv.first.m_rows = ::DFG_MODULE_NS(cont)::intervalSetFromString<int>(iterRows->toString().toUtf8().data()).shift_raw(nUserToInternalRowOffset);
            if (iterColumns != jsonObject.end())
                rv.first.m_columns = ::DFG_MODULE_NS(cont)::intervalSetFromString<int>(iterColumns->toString().toUtf8().data()).shift_raw(nUserToInternalColumnOffset);
            return rv;
        }

        bool isMatchWith(const int nRow, const int nCol, const StringViewUtf8& sv) const
        {
            return !m_rows.hasValue(nRow) || !m_columns.hasValue(nCol) || BaseClass::isMatchWith(sv);
        }

        // Defines rows on which to apply filter.
        IntervalSet m_rows = IntervalSet::makeSingleInterval(1, maxValueOfType<int>());
        // Defines columns on which to apply filter.
        IntervalSet m_columns = IntervalSet::makeSingleInterval(0, maxValueOfType<int>());
    }; // class TableStringMatchDefinition


    // Class providing and/or combination for list of matchers.
    template <class MultiMatch_T>
    class MultiMatchDefinition
    {
    public:
        using MatchDefinition = MultiMatch_T;
        // Map from and_group identifier (string) to match items in that and_group.
        using MatchDefinitionStorage = ::DFG_MODULE_NS(cont)::MapVectorSoA<std::string, std::vector<MatchDefinition>>;

        // Constructs multi matcher from list of \n-separated single line json objects.
        static MultiMatchDefinition fromJson(const StringViewUtf8& sv)
        {
            using DelimitedTextReader = ::DFG_MODULE_NS(io)::DelimitedTextReader;
            const auto metaNone = DelimitedTextReader::s_nMetaCharNone;
            ::DFG_MODULE_NS(io)::BasicImStream istrm(sv.dataRaw(), sv.size());
            MultiMatchDefinition rv;
            DelimitedTextReader::readRow<char>(istrm, '\n', metaNone, metaNone, [&](Dummy, const char* psz, const size_t nCount)
            {
                auto matcherAndOrGroup = MatchDefinition::fromJson(StringViewUtf8(SzPtrUtf8(psz), nCount));
                rv.m_matchers[matcherAndOrGroup.second].push_back(std::move(matcherAndOrGroup.first));
            });
            return rv;
        }

        static MultiMatchDefinition fromJson(const QString& s)
        {
            return fromJson(SzPtrUtf8(s.toUtf8()));
        }

        // Checks if current matchers match with data whose details are implemented in given callback.
        //      -Callback is given const MatchDefinition& as argument and it should return true iff content match.
        template <class MatchFunc_T>
        bool isMatchByCallback(MatchFunc_T&& matchFunc) const
        {
            for (const auto& kv : m_matchers) // For all OR-sets
            {
                bool bIsMatch = true;
                for (const auto& matcher : kv.second) // For all matchers in AND-set
                {
                    bIsMatch = matchFunc(matcher);
                    if (!bIsMatch)
                        break; // One item in AND-set was false; not checking the rest.
                }
                if (bIsMatch)
                    return true; // One OR-item was true so return value is known..
            }
            return false; // None of the OR'ed items matched so returning false.
        }

        template <class Func_T>
        void forEachWhile(Func_T&& func)
        {
            bool bContinue = true;
            for (auto&& kv : m_matchers)
            {
                if (!bContinue)
                    break;
                for(auto&& matcher : kv.second)
                {
                    bContinue = func(matcher);
                    if (!bContinue)
                        break;
                }
            }
        }

        bool empty() const { return m_matchers.empty(); }

        MatchDefinitionStorage m_matchers; // Each mapped item defines a set of AND'ed items and the match result is obtained by OR'ing each AND-set.
    }; // class TableStringMatchDefinition


    constexpr char StringMatchDefinitionField_type[]    = "type";
        constexpr char StringMatchDefinitionFieldValue_type_wildcard[]     = "wildcard";
        constexpr char StringMatchDefinitionFieldValue_type_fixed[]        = "fixed";
        constexpr char StringMatchDefinitionFieldValue_type_regExp[]       = "reg_exp";
    constexpr char StringMatchDefinitionField_case[]    = "case_sensitive";
    constexpr char StringMatchDefinitionField_text[]    = "text";
    constexpr char StringMatchDefinitionField_negate[]  = "negate";
    constexpr char StringMatchDefinitionField_applyColumns[]    = "apply_columns";
    constexpr char StringMatchDefinitionField_andGroup[]        = "and_group";

}} // Module namespace

inline auto ::DFG_MODULE_NS(qt)::StringMatchDefinition::makeMatchEverythingMatcher() -> StringMatchDefinition
{
    return StringMatchDefinition("*", Qt::CaseInsensitive, PatternMatcher::Wildcard);
}


inline auto ::DFG_MODULE_NS(qt)::StringMatchDefinition::fromJson(const StringViewUtf8& sv) -> std::pair<StringMatchDefinition, std::string>
{
    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(QByteArray(sv.dataRaw(), sv.sizeAsInt()), &parseError);
    if (doc.isNull())
        return std::pair<StringMatchDefinition, std::string>(makeMatchEverythingMatcher(), std::string());
    std::pair<StringMatchDefinition, std::string> rv(fromJson(doc.object()), std::string());
    rv.second = doc.object().value("and_group").toString().toStdString();
    return rv;
}

inline auto ::DFG_MODULE_NS(qt)::StringMatchDefinition::fromJson(const QJsonObject& jsonObject) -> StringMatchDefinition
{
    const auto sType = jsonObject.value(QLatin1String(StringMatchDefinitionField_type)).toString();
    const auto bCase = jsonObject.value(QLatin1String(StringMatchDefinitionField_case)).toBool(false);
    const auto sText = jsonObject.value(QLatin1String(StringMatchDefinitionField_text)).toString();
    const auto bNegate = jsonObject.value(QLatin1String(StringMatchDefinitionField_negate)).toBool(false);

    auto patternSyntax = PatternMatcher::Wildcard;

    // Setting pattern syntax
    if (!sType.isEmpty())
    {
        if (sType == QLatin1String(StringMatchDefinitionFieldValue_type_wildcard))
            patternSyntax = PatternMatcher::Wildcard;
        else if (sType == QLatin1String(StringMatchDefinitionFieldValue_type_fixed))
            patternSyntax = PatternMatcher::FixedString;
        else if (sType == QLatin1String(StringMatchDefinitionFieldValue_type_regExp))
            patternSyntax = PatternMatcher::RegExp;
        else
        {
            // Unknown type
        }
    }

    // Settings case sensitivity
    const Qt::CaseSensitivity caseSensitity = (bCase) ? Qt::CaseSensitive : Qt::CaseInsensitive;

    return StringMatchDefinition(sText, caseSensitity, patternSyntax, bNegate);
}


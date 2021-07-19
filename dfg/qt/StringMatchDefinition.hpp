#pragma once

#include "../dfgDefs.hpp"
#include "../ReadOnlySzParam.hpp"
#include "../cont/IntervalSet.hpp"
#include "../cont/IntervalSetSerialization.hpp"
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

        explicit StringMatchDefinition(QString matchString,
                                       Qt::CaseSensitivity caseSensitivity,
                                       PatternMatcher::PatternSyntax patternSyntax)
          : m_matchString(std::move(matchString))
          , m_caseSensitivity(caseSensitivity)
          , m_patternSyntax(patternSyntax)
        {
            initCache();
        }

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
         * @return StringMatchDefinition based on given json.
         */
        static StringMatchDefinition fromJson(const QJsonObject& jsonObject);

        /** Convenience function for creating from json string, see details from other overload. In case of invalid json, returns makeMatchEverythingMatcher() */
        static StringMatchDefinition fromJson(const StringViewUtf8& sv);

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
            return !m_matchString.isEmpty() && m_regExp.isMatchingWith(s);
        }

        bool isMatchWith(const StringViewUtf8 sv) const
        {
            if (!m_sSimpleSubStringMatch.empty())
            {
                const auto& rawSubString = m_sSimpleSubStringMatch.rawStorage();
                return rawSubString.size() <= sv.size()
                       &&
                       std::search(sv.beginRaw(), sv.endRaw(),
                                   rawSubString.cbegin(), rawSubString.cend()
                                    ) != sv.endRaw();
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

    private:
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
        Utf8String m_sSimpleSubStringMatch; // Stores the utf8-encoded substring to search if applicable. This is an optimization to avoid creating redundant QString-objects.
    }; // class StringMatchDefinition


    // Compared to StringMatchDefinition, this class provides "apply filter on row/column" controls and parsing of and_group.
    class TableStringMatchDefinition : public StringMatchDefinition
    {
    public:
        using BaseClass = StringMatchDefinition;
        using IntervalSet = ::DFG_MODULE_NS(cont)::IntervalSet<int>;

        TableStringMatchDefinition(BaseClass&& base) :
            BaseClass(std::move(base))
        {}

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


    constexpr char StringMatchDefinitionField_type[]    = "type";
        constexpr char StringMatchDefinitionFieldValue_type_wildcard[]     = "wildcard";
        constexpr char StringMatchDefinitionFieldValue_type_fixed[]        = "fixed";
        constexpr char StringMatchDefinitionFieldValue_type_regExp[]       = "reg_exp";
    constexpr char StringMatchDefinitionField_case[]    = "case_sensitive";
    constexpr char StringMatchDefinitionField_text[]    = "text";

}} // Module namespace

inline auto ::DFG_MODULE_NS(qt)::StringMatchDefinition::makeMatchEverythingMatcher() -> StringMatchDefinition
{
    return StringMatchDefinition("*", Qt::CaseInsensitive, PatternMatcher::Wildcard);
}


inline auto ::DFG_MODULE_NS(qt)::StringMatchDefinition::fromJson(const StringViewUtf8& sv) -> StringMatchDefinition
{
    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(QByteArray(sv.dataRaw(), sv.sizeAsInt()), &parseError);
    if (doc.isNull())
        return makeMatchEverythingMatcher();
    return fromJson(doc.object());
}

inline auto ::DFG_MODULE_NS(qt)::StringMatchDefinition::fromJson(const QJsonObject& jsonObject) -> StringMatchDefinition
{
    const auto sType = jsonObject.value(QLatin1String(StringMatchDefinitionField_type)).toString();
    const auto bCase = jsonObject.value(QLatin1String(StringMatchDefinitionField_case)).toBool(false);
    const auto sText = jsonObject.value(QLatin1String(StringMatchDefinitionField_text)).toString();

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

    return StringMatchDefinition(sText, caseSensitity, patternSyntax);
}


#pragma once

#pragma once

#include "../dfgDefs.hpp"
#include "../ReadOnlySzParam.hpp"
#include "../str/string.hpp"
#include <algorithm>

#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegExp>
#include <QString>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{
    class DFG_CLASS_NAME(StringMatchDefinition)
    {
    public:
        typedef ::DFG_ROOT_NS::DFG_CLASS_NAME(StringUtf8) Utf8String;

        explicit StringMatchDefinition(QString matchString,
                                       Qt::CaseSensitivity caseSensitivity,
                                       QRegExp::PatternSyntax patternSyntax)
          : m_matchString(std::move(matchString))
          , m_caseSensitivity(caseSensitivity)
          , m_patternSyntax(patternSyntax)
        {
            initCache();
        }

        /**
         * @brief Creates StringMatchDefinition from json object
         * @param jsonObject json object from which values of the following keys as read.
         *      "type": {wildcard, wildcard_unix, fixed, reg_exp, reg_exp2, reg_exp_w3c_xml_schema_11}
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
            return !m_matchString.isEmpty() && s.contains(m_regExp);
        }

        bool isMatchWith(const DFG_CLASS_NAME(StringViewUtf8) sv) const
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

        QRegExp::PatternSyntax patternSyntax() const
        {
            return m_patternSyntax;
        }

    private:
        void initCache()
        {
            m_regExp = QRegExp(m_matchString, m_caseSensitivity, m_patternSyntax);

            const auto isSpecialWildcardChar = [](const QChar& c) { return c == '*' || c == '?' || c == '[' || c == ']'; };

            // TODO: handle other pattern syntaxes as well (e.g. RegExp without special characters could be done with substring matching).
            const bool bCanDoSubStringMatching = (m_patternSyntax == QRegExp::FixedString)
                                        ||
                                        (m_patternSyntax == QRegExp::Wildcard &&
                                         !std::any_of(m_matchString.cbegin(), m_matchString.cend(), isSpecialWildcardChar));
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
        QRegExp::PatternSyntax m_patternSyntax;
        QRegExp m_regExp;
        Utf8String m_sSimpleSubStringMatch; // Stores the utf8-encoded substring to search if applicable. This is an optimization to avoid creating redundant QString-objects.
    }; // class StringMatchDefinition


    constexpr char StringMatchDefinitionField_type[]    = "type";
        constexpr char StringMatchDefinitionFieldValue_type_wildcard[]     = "wildcard";
        constexpr char StringMatchDefinitionFieldValue_type_wildcardUnix[] = "wildcard_unix";
        constexpr char StringMatchDefinitionFieldValue_type_fixed[]        = "fixed";
        constexpr char StringMatchDefinitionFieldValue_type_regExp[]       = "reg_exp";
        constexpr char StringMatchDefinitionFieldValue_type_regExp2[]      = "reg_exp2";
        constexpr char StringMatchDefinitionFieldValue_type_regExpW3C[]    = "reg_exp_w3c_xml_schema_11";
    constexpr char StringMatchDefinitionField_case[]    = "case_sensitive";
    constexpr char StringMatchDefinitionField_text[]    = "text";

}} // Module namespace

inline auto ::DFG_MODULE_NS(qt)::StringMatchDefinition::makeMatchEverythingMatcher() -> StringMatchDefinition
{
    return StringMatchDefinition("*", Qt::CaseInsensitive, QRegExp::Wildcard);
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

    QRegExp::PatternSyntax patternSyntax = QRegExp::Wildcard;

    // Setting pattern syntax
    if (!sType.isEmpty())
    {
        if (sType == QLatin1String(StringMatchDefinitionFieldValue_type_wildcard))
            patternSyntax = QRegExp::Wildcard;
        else if (sType == QLatin1String(StringMatchDefinitionFieldValue_type_wildcardUnix))
            patternSyntax = QRegExp::WildcardUnix;
        else if (sType == QLatin1String(StringMatchDefinitionFieldValue_type_fixed))
            patternSyntax = QRegExp::FixedString;
        else if (sType == QLatin1String(StringMatchDefinitionFieldValue_type_regExp))
            patternSyntax = QRegExp::RegExp;
        else if (sType == QLatin1String(StringMatchDefinitionFieldValue_type_regExp2))
            patternSyntax = QRegExp::RegExp2;
        else if (sType == QLatin1String(StringMatchDefinitionFieldValue_type_regExpW3C))
            patternSyntax = QRegExp::W3CXmlSchema11;
        else
        {
            // Unknown type
        }
    }

    // Settings case sensitivity
    const Qt::CaseSensitivity caseSensitity = (bCase) ? Qt::CaseSensitive : Qt::CaseInsensitive;

    return StringMatchDefinition(sText, caseSensitity, patternSyntax);
}


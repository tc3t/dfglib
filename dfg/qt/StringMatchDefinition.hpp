#pragma once

#pragma once

#include "../dfgDefs.hpp"
#include "../ReadOnlySzParam.hpp"
#include "../str/string.hpp"
#include <algorithm>

#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
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

}} // Module namespace

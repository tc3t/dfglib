#pragma once

#pragma once

#include "../dfgDefs.hpp"
#include "../ReadOnlySzParam.hpp"

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
        explicit StringMatchDefinition(QString matchString,
                                       Qt::CaseSensitivity caseSensitivity,
                                       QRegExp::PatternSyntax patternSyntax)
          : m_matchString(std::move(matchString))
          , m_caseSensitivity(caseSensitivity)
          , m_patternSyntax(patternSyntax)
        {}

        bool isMatchWith(const QString& s) const
        {
            return !m_matchString.isEmpty() && s.contains(QRegExp(m_matchString, m_caseSensitivity, m_patternSyntax));
        }

        bool isMatchWith(const DFG_CLASS_NAME(StringViewUtf8) sv) const
        {
            return isMatchWith(QString::fromUtf8(toCharPtr_raw(sv.data()), static_cast<int>(sv.length())));
        }

        QString m_matchString;
        Qt::CaseSensitivity m_caseSensitivity;
        QRegExp::PatternSyntax m_patternSyntax;
    }; // class StringMatchDefinition

}} // Module namespace

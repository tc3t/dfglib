#pragma once

#pragma once

#include "../dfgDefs.hpp"
#include "../ReadOnlySzParam.hpp"

#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QString>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{
    class DFG_CLASS_NAME(StringMatchDefinition)
    {
    public:
        StringMatchDefinition(QString matchString, Qt::CaseSensitivity caseSensitivity) :
            m_matchString(std::move(matchString))
          , m_caseSensitivity(caseSensitivity)
        {}

        bool isMatchWith(const QString& s) const
        {
            return !m_matchString.isEmpty() && s.contains(m_matchString, m_caseSensitivity);
        }

        bool isMatchWith(const DFG_CLASS_NAME(StringViewUtf8) sv) const
        {
            return !m_matchString.isEmpty() && QString::fromUtf8(toCharPtr_raw(sv.data()), static_cast<int>(sv.length())).contains(m_matchString, m_caseSensitivity);
        }

        QString m_matchString;
        Qt::CaseSensitivity m_caseSensitivity;
    }; // class StringMatchDefinition

}} // Module namespace

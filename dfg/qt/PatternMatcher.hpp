#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"
#include <functional>

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QRegExp>
    #include <QSortFilterProxyModel>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

// Wrapper for QRegExp/QRegularExpression, originally introduced for QRegExp -> QRegularExpression porting needed for Qt5 -> Qt6 transition.
class PatternMatcher
{
public:
    enum PatternSyntax
    {
        Wildcard,
        WildcardUnix,
        FixedString,
        RegExp,
        RegExp2,
        W3CXmlSchema11,
        PatternSyntaxFirst = Wildcard,
        PatternSyntaxFLast = W3CXmlSchema11
    };

    PatternMatcher() = default;

    PatternMatcher(const QString& s, Qt::CaseSensitivity caseSensitivity, PatternSyntax patternSyntax);

    // Returns true if successfully set, false otherwise
    bool setToProxyModel(QSortFilterProxyModel* pProxy);

    bool isMatchingWith(const QString& s) const;

    static const char* patternSyntaxName_untranslated(const PatternSyntax patternSyntax);

    static const char* shortDescriptionForPatternSyntax_untranslated(const PatternSyntax patternSyntax);

    static void forEachPatternSyntax(std::function<void(int, PatternSyntax)> handler);

    QRegExp m_regExp;

}; // class PatternMatcher


inline PatternMatcher::PatternMatcher(const QString& s, Qt::CaseSensitivity caseSensitivity, PatternSyntax patternSyntax)
{
    m_regExp.setPattern(s);
    m_regExp.setCaseSensitivity(caseSensitivity);

    QRegExp::PatternSyntax regExpPs = QRegExp::Wildcard;
    switch (patternSyntax)
    {
    case Wildcard: regExpPs = QRegExp::Wildcard; break;
    case WildcardUnix: regExpPs = QRegExp::WildcardUnix; break;
    case FixedString: regExpPs = QRegExp::FixedString; break;
    case RegExp: regExpPs = QRegExp::RegExp; break;
    case RegExp2: regExpPs = QRegExp::RegExp2; break;
    case W3CXmlSchema11: regExpPs = QRegExp::W3CXmlSchema11; break;
    }
    m_regExp.setPatternSyntax(regExpPs);
}

// Returns true if successfully set, false otherwise
inline bool PatternMatcher::setToProxyModel(QSortFilterProxyModel* pProxy)
{
    if (!pProxy)
        return false;
    pProxy->setFilterRegExp(this->m_regExp);
    return true;
}

inline bool PatternMatcher::isMatchingWith(const QString& s) const
{
    return s.contains(this->m_regExp);
}

inline const char* PatternMatcher::patternSyntaxName_untranslated(const PatternSyntax patternSyntax)
{
    switch (patternSyntax)
    {
    case Wildcard:       return "Wildcard";
    case WildcardUnix:   return "Wildcard(Unix Shell)";
    case FixedString:    return "Simple string";
    case RegExp:         return "Regular expression";
    case RegExp2:        return "Regular expression 2";
    case W3CXmlSchema11: return "Regular expression (W3C XML Schema 1.1)";
    default:             return "";
    }
}

inline const char* PatternMatcher::shortDescriptionForPatternSyntax_untranslated(const PatternSyntax patternSyntax)
{
    switch (patternSyntax)
    {
    case Wildcard:       return "? Matches single character, the same as . in regexp.\n"
        "* Matches zero or more characters, the same as .* in regexp.\n"
        "[...] Set of character similar to regexp.";;
    case WildcardUnix:   return "Like Wildcard, but wildcard characters can be escaped with \\";
    case FixedString:    return "Plain search string, no need to worry about special characters";
    case RegExp:         return "Regular expression\n\n"
        ". Matches any single character\n"
        ".* Matches zero or more characters\n"
        "[...] Set of characters\n"
        "| Match expression before or after\n"
        "^ Beginning of line\n"
        "$ End of line\n";
    case RegExp2:        return "Regular expression with greedy quantifiers";
    case W3CXmlSchema11: return "Regular expression (W3C XML Schema 1.1)";
    default:             return "";
    }
}

inline void PatternMatcher::forEachPatternSyntax(std::function<void(int, PatternSyntax)> handler)
{
    const auto nFirst = static_cast<int>(PatternSyntaxFirst);
    const auto nLast = static_cast<int>(PatternSyntaxFLast);
    for (int i = nFirst; i < nLast; ++i)
    {
        handler(i - nFirst, static_cast<PatternSyntax>(i));
    }
}

} } // module namespace

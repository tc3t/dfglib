#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"
#include "../dfgAssert.hpp"
#include <functional>

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QRegularExpression>
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
        FixedString,
        RegExp,
        Json,
        PatternSyntaxFirst = Wildcard,
        PatternSyntaxLast = Json
    };

    PatternMatcher() = default;

    PatternMatcher(const QString& s, Qt::CaseSensitivity caseSensitivity, PatternSyntax patternSyntax);

    // Returns true if successfully set, false otherwise
    bool setToProxyModel(QSortFilterProxyModel* pProxy);

    bool isMatchingWith(const QString& s) const;

    static const char* patternSyntaxName_untranslated(const PatternSyntax patternSyntax);
    static PatternSyntax patternSyntaxNameToEnum(const StringViewC& svName, PatternSyntax defaultReturn);

    static const char* shortDescriptionForPatternSyntax_untranslated(const PatternSyntax patternSyntax);

    static void forEachPatternSyntax(std::function<void(int, PatternSyntax)> handler);

    static bool isSpecialRegularExpressionCharacter(const QChar& c);
    static bool isSpecialWildcardCharacter(const QChar& c);

    static bool hasSpecialRegularExpressionCharacters(const QString& s);
    static bool hasSpecialWildcardCharacters(const QString& s);

private:
    template <size_t N>
    static bool isAnyOf(const QChar& c, const char (&szCharList)[N]);

public:

    QRegularExpression m_regExp;
}; // class PatternMatcher

template <size_t N>
inline bool PatternMatcher::isAnyOf(const QChar& c, const char(&szCharList)[N])
{
    return std::find(szCharList, szCharList + N - 1, c.toLatin1()) != (szCharList + N - 1);
}

inline bool PatternMatcher::isSpecialRegularExpressionCharacter(const QChar& c)
{
    // From documentation of QRegExp::escape(): "The special characters are $, (,), *, +, ., ?, [, ,], ^, {, | and }."
    return isAnyOf(c, "$()*+.?[]^{|}");
}

inline bool PatternMatcher::isSpecialWildcardCharacter(const QChar& c)
{
    return isAnyOf(c, "?*[]");
}

inline bool PatternMatcher::hasSpecialRegularExpressionCharacters(const QString& s)
{
    return std::any_of(s.begin(), s.end(), &PatternMatcher::isSpecialRegularExpressionCharacter);
}

inline bool PatternMatcher::hasSpecialWildcardCharacters(const QString& s)
{
    return std::any_of(s.begin(), s.end(), &PatternMatcher::isSpecialWildcardCharacter);
}

inline PatternMatcher::PatternMatcher(const QString& s, const Qt::CaseSensitivity caseSensitivity, PatternSyntax patternSyntax)
{
    switch (patternSyntax)
    {
        case Wildcard:
            {
                if (!hasSpecialWildcardCharacters(s))
                    m_regExp.setPattern(QRegularExpression::escape(s));
                else
                {
                    // Not using QRegularExpression::wildcardToRegularExpression() as it is available only since 5.12 and
                    // it didn't produce expected results in this context: for example QRegularExpression::wildcardToRegularExpression("a").pattern() returned \A(?:a)\z
                    QString sW;
                    for (const auto& c : s)
                    {
                        if (c == '*')
                            sW.push_back(".*");
                        else if (c == '?')
                            sW.push_back(".");
                        else if (!isSpecialRegularExpressionCharacter(c) || c == '[' || c == ']')
                            sW.push_back(c);
                        else
                            sW.push_back(QRegularExpression::escape(QString(c)));
                    }
                    m_regExp.setPattern(sW);
                }
            }
            break;
        case FixedString:
            m_regExp.setPattern(QRegularExpression::escape(s));
            break;
        case RegExp:
            m_regExp.setPattern(s);
            break;
        default:
            DFG_ASSERT_IMPLEMENTED(false);
            break;
    }
    if (caseSensitivity == Qt::CaseInsensitive)
        m_regExp.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
}

// Returns true if successfully set, false otherwise
inline bool PatternMatcher::setToProxyModel(QSortFilterProxyModel* pProxy)
{
    if (!pProxy)
        return false;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
    pProxy->setFilterRegularExpression(this->m_regExp); // Introduced in 5.12
    return true;
#else
    return false;
#endif
}

inline bool PatternMatcher::isMatchingWith(const QString& s) const
{
    return s.contains(this->m_regExp);
}

inline const char* PatternMatcher::patternSyntaxName_untranslated(const PatternSyntax patternSyntax)
{
    switch (patternSyntax)
    {
    case Wildcard:       return QT_TR_NOOP("Wildcard");
    case FixedString:    return QT_TR_NOOP("Simple string");
    case RegExp:         return QT_TR_NOOP("Regular expression");
    case Json:           return QT_TR_NOOP("Json");
    default:             return "";
    }
}

inline auto PatternMatcher::patternSyntaxNameToEnum(const StringViewC& svName, const PatternSyntax defaultReturn) -> PatternSyntax
{
    auto rv = defaultReturn;
    bool bFound = false;
    forEachPatternSyntax([&](const int, const PatternSyntax& en)
        {
            if (bFound)
                return;
            if (svName == patternSyntaxName_untranslated(en))
            {
                rv = en;
                bFound = true;
            }
        });
    return rv;
}

inline const char* PatternMatcher::shortDescriptionForPatternSyntax_untranslated(const PatternSyntax patternSyntax)
{
    switch (patternSyntax)
    {
    case Wildcard:       return "? Matches single character, the same as . in regexp.\n"
        "* Matches zero or more characters, the same as .* in regexp.\n"
        "[...] Set of character similar to regexp.";
    case FixedString:    return "Plain search string, no need to worry about special characters";
    case RegExp:         return "Regular expression\n\n"
        ". Matches any single character\n"
        ".* Matches zero or more characters\n"
        "[...] Set of characters\n"
        "| Match expression before or after\n"
        "^ Beginning of line\n"
        "$ End of line\n";
    case Json:           return "List of space-separated json objects\n"
                                "\n"
                                R"(Example: {"text":"a", "apply_columns":"1", "and_group":"a"} {"text":"b", "apply_columns":"2", "and_group":"a"} {"text":"c"})"
                                "\nThis filter keeps rows that either (have 'a' on column 1 and 'b' on column 2) or 'c' on any column";
    default:             return "";
    }
}

inline void PatternMatcher::forEachPatternSyntax(std::function<void(int, PatternSyntax)> handler)
{
    const auto nFirst = static_cast<int>(PatternSyntaxFirst);
    const auto nLast = static_cast<int>(PatternSyntaxLast);
    for (int i = nFirst; i <= nLast; ++i)
    {
        handler(i - nFirst, static_cast<PatternSyntax>(i));
    }
}

} } // module namespace

#pragma once

#include "io/textEncodingTypes.hpp"
#include "io/EndOfLineTypes.hpp"
#include "io/DelimitedTextReader.hpp"
#include "ReadOnlySzParam.hpp"
#include "io.hpp"
#include "cont/MapVector.hpp"
#include "cont/CsvConfig.hpp"
#include "str/stringLiteralCharToValue.hpp"
#include "numericTypeTools.hpp"
#include "str/strTo.hpp"

DFG_ROOT_NS_BEGIN
{

class CsvFormatDefinition
{
public:
    using CsvConfig = ::DFG_MODULE_NS(cont)::CsvConfig;

    //CsvFormatDefinition(const char cSep = ::DFG_MODULE_NS(io)::DelimitedTextReader::s_nMetaCharAutoDetect, const char cEnc = '"', DFG_MODULE_NS(io)::EndOfLineType eol = DFG_MODULE_NS(io)::EndOfLineTypeN) :
    // Note: default values are questionable because default read vals should have metachars, but default write vals should not.
    CsvFormatDefinition(const char cSep/* = ::DFG_MODULE_NS(io)::DelimitedTextReader::s_nMetaCharAutoDetect*/,
                                        const char cEnc/* = '"'*/,
                                        DFG_MODULE_NS(io)::EndOfLineType eol/* = DFG_MODULE_NS(io)::EndOfLineTypeN*/,
                                        DFG_MODULE_NS(io)::TextEncoding encoding/* = DFG_MODULE_NS(io)::encodingUTF8*/) :
        m_cSep(cSep),
        m_cEnc(cEnc),
        m_eolType(eol),
        m_textEncoding(encoding),
        m_enclosementBehaviour(DFG_MODULE_NS(io)::EbEncloseIfNeeded),
        m_bWriteHeader(true),
        m_bWriteBom(true)
    {}

    // Convenience functions for creating commonly used formats.
    // Creates ',', '"', \n, UTF8
    static CsvFormatDefinition fromReadTemplate_commaQuoteEolNUtf8()       { return CsvFormatDefinition(',', '"', ::DFG_MODULE_NS(io)::EndOfLineTypeN, ::DFG_MODULE_NS(io)::encodingUTF8); }
    // Creates ',', <no enclosing>, \n, UTF8
    static CsvFormatDefinition fromReadTemplate_commaNoEnclosingEolNUtf8() { return CsvFormatDefinition(',', metaCharNone(), ::DFG_MODULE_NS(io)::EndOfLineTypeN, ::DFG_MODULE_NS(io)::encodingUTF8); }

    // Reads properties from given config, items not present in config are not modified.
    void fromConfig(const CsvConfig& config);

    void appendToConfig(CsvConfig& config) const;

    static constexpr char metaCharNone()
    {
        return ::DFG_MODULE_NS(io)::DelimitedTextReader::s_nMetaCharNone;
    }

    template <class Str_T>
    static Str_T csvFilePathToConfigFilePath(const Str_T& str)
    {
        // Note: changing this would likely require changes at least to file extension filters in qt-module.
        return str + ".conf";
    }

    int32 separatorChar() const { return m_cSep; }
    void separatorChar(int32 cSep) { m_cSep = cSep; }
    std::string separatorCharAsString() const;
    int32 enclosingChar() const { return m_cEnc; }
    void enclosingChar(int32 cEnc) { m_cEnc = cEnc; }
    std::string enclosingCharAsString() const;
    int32 eolCharFromEndOfLineType() const { return ::DFG_MODULE_NS(io)::eolCharFromEndOfLineType(eolType()); }

    DFG_MODULE_NS(io)::EndOfLineType eolType() const { return m_eolType; }
    void eolType(DFG_MODULE_NS(io)::EndOfLineType eolType) { m_eolType = eolType; }
    StringViewC eolTypeAsString() const; // Convenience function returning ::DFG_MODULE_NS(io)::eolLiteralStrFromEndOfLineType(eolType());

    bool headerWriting() const { return m_bWriteHeader; }
    void headerWriting(bool bWriteHeader) { m_bWriteHeader = bWriteHeader; }

    bool bomWriting() const { return m_bWriteBom; }
    void bomWriting(bool bWriteBom) { m_bWriteBom = bWriteBom; }

    DFG_MODULE_NS(io)::TextEncoding textEncoding() const { return m_textEncoding; }
    void textEncoding(DFG_MODULE_NS(io)::TextEncoding encoding) { m_textEncoding = encoding; }
    StringViewC textEncodingAsString() const; // Convenience function returning ::DFG_MODULE_NS(io)::encodingToStrId(textEncoding())

    void setProperty(const StringViewC& svKey, const StringViewC& svValue)
    {
        m_genericProperties[svKey.toString()] = svValue.toString();
    }

    std::string getProperty(const StringViewC& svKey, const StringViewC& defaultValue) const
    {
        auto iter = m_genericProperties.find(svKey);
        return (iter != m_genericProperties.cend()) ? iter->second : defaultValue.toString();
    }

    // Convenience method for returning property as type T converted through strTo<T>()
    template <class T>
    T getPropertyThroughStrTo(const StringViewC& svKey, const T& defaultValue) const
    {
        auto iter = m_genericProperties.find(svKey);
        return (iter != m_genericProperties.cend()) ? ::DFG_MODULE_NS(str)::strTo<T>(iter->second) : defaultValue;
    }

    bool hasProperty(const StringViewC& svKey) const
    {
        return m_genericProperties.hasKey(svKey);
    }

    DFG_MODULE_NS(io)::EnclosementBehaviour enclosementBehaviour() const { return m_enclosementBehaviour; }
    void enclosementBehaviour(const DFG_MODULE_NS(io)::EnclosementBehaviour eb) { m_enclosementBehaviour = eb; }

    // Returns true iff 'this' and 'other' are format matching disregarding properties
    // Returned true means that the following items are the same:
    //      -separator and enclosing characters
    //      -eol-type
    //      -text encoding
    //      -enclosement behaviour
    //      -header-writing
    //      -BOM-writing
    bool isFormatMatchingWith(const CsvFormatDefinition& other) const;

    static std::string charAsPrintableString(int32 c);

    int32 m_cSep;
    int32 m_cEnc;
    //int32 m_cEol;
    DFG_MODULE_NS(io)::EndOfLineType m_eolType;
    DFG_MODULE_NS(io)::TextEncoding m_textEncoding;
    DFG_MODULE_NS(io)::EnclosementBehaviour m_enclosementBehaviour; // Affects only writing.
    bool m_bWriteHeader;
    bool m_bWriteBom;
    ::DFG_MODULE_NS(cont)::MapVectorAoS<std::string, std::string> m_genericProperties; // Generic properties (e.g. if implementation needs specific flags)
}; // CsvFormatDefinition

inline void CsvFormatDefinition::fromConfig(const DFG_MODULE_NS(cont)::CsvConfig& config)
{
    // Encoding
    {
        auto p = config.valueStrOrNull(DFG_UTF8("encoding"));
        if (p)
            textEncoding(DFG_MODULE_NS(io)::strIdToEncoding(p->c_str().rawPtr()));
    }

    // Enclosing char
    {
        auto p = config.valueStrOrNull(DFG_UTF8("enclosing_char"));
        if (p)
        {
            if (p->empty())
            {
                enclosingChar(metaCharNone());
                this->enclosementBehaviour(::DFG_MODULE_NS(io)::EnclosementBehaviour::EbNoEnclose);
            }
            else
            {
                auto rv = DFG_MODULE_NS(str)::stringLiteralCharToValue<int32>(p->rawStorage());
                if (rv.first)
                    enclosingChar(rv.second);
            }
        }
    }

    // Separator char
    {
        auto p = config.valueStrOrNull(DFG_UTF8("separator_char"));
        if (p)
        {
            auto rv = DFG_MODULE_NS(str)::stringLiteralCharToValue<int32>(p->rawStorage());
            if (rv.first)
                separatorChar(rv.second);
        }
    }

    // EOL-type
    {
        auto p = config.valueStrOrNull(DFG_UTF8("end_of_line_type"));
        if (p)
            eolType(DFG_MODULE_NS(io)::endOfLineTypeFromStr(p->rawStorage()));
    }

    // BOM writing
    {
        auto p = config.valueStrOrNull(DFG_UTF8("bom_writing"));
        if (p)
            bomWriting(DFG_MODULE_NS(str)::strToByNoThrowLexCast<bool>(p->rawStorage()));
    }

    // Properties
    {
        config.forEachStartingWith(DFG_UTF8("properties/"), [&](const DFG_MODULE_NS(cont)::CsvConfig::StringViewT& relativeUri, const DFG_MODULE_NS(cont)::CsvConfig::StringViewT& value)
        {
            setProperty(relativeUri, value);
        });
    }
}

inline void CsvFormatDefinition::appendToConfig(DFG_MODULE_NS(cont)::CsvConfig& config) const
{
    // Encoding
    {
        auto psz = DFG_MODULE_NS(io)::encodingToStrId(m_textEncoding);
        if (!DFG_MODULE_NS(str)::isEmptyStr(psz))
            config.setKeyValue(DFG_UTF8("encoding"), SzPtrUtf8(DFG_MODULE_NS(io)::encodingToStrId(m_textEncoding)));
    }


    // Enclosing char
    {
        if (m_cEnc == metaCharNone())
            config.setKeyValue(DFG_UTF8("enclosing_char"), DFG_UTF8(""));
        else if (!DFG_MODULE_NS(io)::DelimitedTextReader::isMetaChar(m_cEnc) && m_cEnc >= 0)
            config.setKeyValue_fromUntyped("enclosing_char", charAsPrintableString(m_cEnc));
    }

    // Separator char
    if (!DFG_MODULE_NS(io)::DelimitedTextReader::isMetaChar(m_cSep) && m_cSep >= 0)
        config.setKeyValue_fromUntyped("separator_char", charAsPrintableString(m_cSep));

    // EOL-type
    {
        const auto sv = ::DFG_MODULE_NS(io)::eolLiteralStrFromEndOfLineType(eolType());
        if (!sv.empty())
            config.setKeyValue_fromUntyped("end_of_line_type", sv);
    }

    // BOM writing
    {
        config.setKeyValue(DFG_UTF8("bom_writing"), (m_bWriteBom) ? DFG_UTF8("1") : DFG_UTF8("0"));
    }

    // Properties
    // TODO: Generic properties are std::string so must figure out raw bytes to UTF-8 conversion. In practice consider changing properties to UTF-8
    {
        //m_genericProperties
    }
}

inline std::string CsvFormatDefinition::charAsPrintableString(const int32 c)
{
    return ::DFG_MODULE_NS(str)::charToPrintable(c);
}

inline std::string CsvFormatDefinition::separatorCharAsString() const
{
    return charAsPrintableString(separatorChar());
}

inline std::string CsvFormatDefinition::enclosingCharAsString() const
{
    return charAsPrintableString(enclosingChar());
}

inline StringViewC CsvFormatDefinition::eolTypeAsString() const
{
    return ::DFG_MODULE_NS(io)::eolLiteralStrFromEndOfLineType(eolType());
}

inline auto CsvFormatDefinition::textEncodingAsString() const -> StringViewC
{
    return ::DFG_MODULE_NS(io)::encodingToStrId(textEncoding());
}

inline bool CsvFormatDefinition::isFormatMatchingWith(const CsvFormatDefinition& other) const
{
    return
        this->m_cSep == other.m_cSep &&
        this->m_cEnc == other.m_cEnc &&
        this->m_eolType == other.m_eolType &&
        this->m_textEncoding == other.m_textEncoding &&
        this->m_enclosementBehaviour == other.m_enclosementBehaviour &&
        this->m_bWriteHeader == other.m_bWriteHeader &&
        this->m_bWriteBom == other.m_bWriteBom;
}

} // namespace DFG_ROOT_NS

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

    // Reads properties from given config, items not present in config are not modified.
    void fromConfig(const CsvConfig& config);

    void appendToConfig(CsvConfig& config) const;

    template <class Str_T>
    static Str_T csvFilePathToConfigFilePath(const Str_T & str)
    {
        // Note: changing this would likely require changes at least to file extension filters in qt-module.
        return str + ".conf";
    }

    int32 separatorChar() const { return m_cSep; }
    void separatorChar(int32 cSep) { m_cSep = cSep; }
    int32 enclosingChar() const { return m_cEnc; }
    void enclosingChar(int32 cEnc) { m_cEnc = cEnc; }
    int32 eolCharFromEndOfLineType() const { return ::DFG_MODULE_NS(io)::eolCharFromEndOfLineType(eolType()); }

    DFG_MODULE_NS(io)::EndOfLineType eolType() const { return m_eolType; }
    void eolType(DFG_MODULE_NS(io)::EndOfLineType eolType) { m_eolType = eolType; }

    bool headerWriting() const { return m_bWriteHeader; }
    void headerWriting(bool bWriteHeader) { m_bWriteHeader = bWriteHeader; }

    bool bomWriting() const { return m_bWriteBom; }
    void bomWriting(bool bWriteBom) { m_bWriteBom = bWriteBom; }

    DFG_MODULE_NS(io)::TextEncoding textEncoding() const { return m_textEncoding; }
    void textEncoding(DFG_MODULE_NS(io)::TextEncoding encoding) { m_textEncoding = encoding; }

    void setProperty(const StringViewC & svKey, const StringViewC & svValue)
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
                enclosingChar(DFG_MODULE_NS(io)::DelimitedTextReader::s_nMetaCharNone);
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
        if (m_cEnc == DFG_MODULE_NS(io)::DelimitedTextReader::s_nMetaCharNone)
            config.setKeyValue(DFG_UTF8("enclosing_char"), DFG_UTF8(""));
        else if (!DFG_MODULE_NS(io)::DelimitedTextReader::isMetaChar(m_cEnc) && m_cEnc >= 0)
        {
            char buffer[32];
            DFG_MODULE_NS(str)::DFG_DETAIL_NS::sprintf_s(buffer, sizeof(buffer), "\\x%x", m_cEnc);
            config.setKeyValue(DFG_UTF8("enclosing_char"), SzPtrUtf8(buffer));
        }
    }

    // Separator char
    if (!DFG_MODULE_NS(io)::DelimitedTextReader::isMetaChar(m_cSep) && m_cSep >= 0)
    {
        char buffer[32];
        DFG_MODULE_NS(str)::DFG_DETAIL_NS::sprintf_s(buffer, sizeof(buffer), "\\x%x", m_cSep);
        config.setKeyValue(DFG_UTF8("separator_char"), SzPtrUtf8(buffer));
    }

    // EOL-type
    {
        const auto psz = DFG_MODULE_NS(io)::eolLiteralStrFromEndOfLineType(m_eolType);
        if (!DFG_MODULE_NS(str)::isEmptyStr(psz))
            config.setKeyValue(DFG_UTF8("end_of_line_type"), SzPtrUtf8(psz.c_str()));
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

} // namespace DFG_ROOT_NS

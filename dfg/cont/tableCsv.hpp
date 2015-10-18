#pragma once

#include "table.hpp"
#include "../io/textencodingtypes.hpp"
#include "../io/EndOfLineTypes.hpp"
#include "../io/DelimitedTextReader.hpp"
#include "../io/fileToByteContainer.hpp"
#include "../readOnlyParamStr.hpp"
#include "../io/IfStream.hpp"
#include "../io.hpp"
#include "../io/ImStreamWithEncoding.hpp"

DFG_ROOT_NS_BEGIN{ 
    
    class DFG_CLASS_NAME(CsvFormatDefinition)
    {
    public:
        DFG_CLASS_NAME(CsvFormatDefinition)(const char cSep = ::DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharAutoDetect, const char cEnc = '"', DFG_MODULE_NS(io)::EndOfLineType eol = DFG_MODULE_NS(io)::EndOfLineTypeN) :
            m_cSep(cSep),
            m_cEnc(cEnc),
            m_eolType(eol),
            m_textEncoding(DFG_MODULE_NS(io)::encodingUTF8),
            m_bWriteHeader(true),
            m_bWriteBom(true)
        {}

        int32 separatorChar() const { return m_cSep; }
        void separatorChar(int32 cSep) { m_cSep = cSep; }
        int32 enclosingChar() const { return m_cEnc; }
        void enclosingChar(int32 cEnc) { m_cEnc = cEnc; }
        DFG_MODULE_NS(io)::EndOfLineType eolType() const { return m_eolType; }
        void eolType(DFG_MODULE_NS(io)::EndOfLineType eolType) { m_eolType = eolType; }

        bool headerWriting() const { return m_bWriteHeader; }
        void headerWriting(bool bWriteHeader) { m_bWriteHeader = bWriteHeader; }

        bool bomWriting() const { return m_bWriteBom; }
        void bomWriting(bool bWriteBom) { m_bWriteBom = bWriteBom; }

        DFG_MODULE_NS(io)::TextEncoding textEncoding() const { return m_textEncoding; }
        void textEncoding(DFG_MODULE_NS(io)::TextEncoding encoding) { m_textEncoding = encoding; }

        int32 m_cSep;
        int32 m_cEnc;
        DFG_MODULE_NS(io)::EndOfLineType m_eolType;
        DFG_MODULE_NS(io)::TextEncoding m_textEncoding;
        bool m_bWriteHeader;
        bool m_bWriteBom;
    };
    
    DFG_SUB_NS(cont)
    {

        template <class Char_T, class Index_T>
        class DFG_CLASS_NAME(TableCsv) : public DFG_CLASS_NAME(TableSz)<Char_T, Index_T>
        {
        public:
            typedef DFG_ROOT_NS::DFG_CLASS_NAME(CsvFormatDefinition) CsvFormatDefinition;

            DFG_CLASS_NAME(TableCsv)()
            {}
        };

} } // module namespace

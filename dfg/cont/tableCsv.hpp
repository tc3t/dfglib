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
#include <unordered_map>

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

            void readFromFile(const DFG_CLASS_NAME(ReadOnlyParamStrC)& sPath) { readFromFileImpl(sPath); }
            void readFromFile(const DFG_CLASS_NAME(ReadOnlyParamStrW)& sPath) { readFromFileImpl(sPath); }

            template <class Char_T>
            void readFromFileImpl(const DFG_CLASS_NAME(ReadOnlyParamStr)<Char_T>& sPath)
            {
                bool bRead = false;
                try
                {
                    auto bytes = DFG_MODULE_NS(io)::fileToVector(sPath);
                    readFromMemory(bytes.data(), bytes.size());
                    bRead = true;
                }
                catch (...)
                {}

                if (!bRead)
                {
                    DFG_MODULE_NS(io)::IfStreamWithEncoding istrm;
                    istrm.open(sPath);
                    read(istrm);
                }
            }

            void readFromMemory(const char* const pData, const size_t nSize)
            {
                DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream) strmBom(pData, nSize);
                const auto encoding = DFG_MODULE_NS(io)::checkBOM(strmBom);
                if (encoding == DFG_MODULE_NS(io)::encodingUnknown)
                {
                    // TODO: read encoding/code page hint from options.
                    DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream) strm(pData, nSize);
                    read(strm);
                }
                else if (encoding == DFG_MODULE_NS(io)::encodingUTF8)
                {
                    const auto nBomSize = DFG_MODULE_NS(utf)::bomSizeInBytes(DFG_MODULE_NS(io)::encodingUTF8);
                    DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream) strm(pData + nBomSize, nSize - nBomSize);
                    read(strm, DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharAppenderDefault<std::string, char>());
                }
                else
                {
                    DFG_MODULE_NS(io)::DFG_CLASS_NAME(ImStreamWithEncoding) strm(pData, nSize, DFG_MODULE_NS(io)::encodingUnknown);
                    read(strm);
                }
            }

            template <class Strm_T>
            void read(Strm_T& strm)
            {
                using namespace DFG_MODULE_NS(io);
                read(strm, DFG_CLASS_NAME(DelimitedTextReader)::CharAppenderUtf<std::string>());
            }

            template <class Strm_T, class CharAppender_T>
            void read(Strm_T& strm, CharAppender_T)
            {
                using namespace DFG_MODULE_NS(io);
                clear();
                // TODO: correct format settings.
                DFG_CLASS_NAME(DelimitedTextReader)::CellData<Char_T, Char_T, std::basic_string<Char_T>, CharAppender_T> cellDataHandler(',', '"', '\n');

                auto reader = DFG_CLASS_NAME(DelimitedTextReader)::createReader(strm, cellDataHandler);
                auto cellHandler = [&](const size_t nRow, const size_t nCol, const decltype(cellDataHandler)& cdh)
                {
                    setElement(nRow, nCol, cdh.getBuffer());
                };
                DFG_CLASS_NAME(DelimitedTextReader)::read(reader, cellHandler);
            }

            // Strm must have write()-method which writes bytes untranslated.
            //  TODO: test writing non-square data.
            template <class Strm_T>
            void writeToStream(Strm_T& strm, const DFG_MODULE_NS(io)::TextEncoding encoding) const
            {
                // TODO: return value
                if (m_colToRows.empty())
                    return;
                std::unordered_map<Index_T, ColumnIndexPairContainer::const_iterator> nextColItemRowIters;
                forEachFwdColumnIndex([&](const Index_T nCol)
                {
                    if (!m_colToRows.empty())
                        nextColItemRowIters[nCol] = m_colToRows[nCol].cbegin();
                });
                const auto nMaxColCount = static_cast<Index_T>(m_colToRows.size());
                for (Index_T nRow = 0; !nextColItemRowIters.empty(); ++nRow)
                {
                    for (Index_T nCol = 0; nCol < nMaxColCount; ++nCol)
                    {
                        auto iter = nextColItemRowIters.find(nCol);
                        if (iter != nextColItemRowIters.end() && iter->second->first == nRow) // Case: (row, col) has item
                        {
                            auto& rowEntryIter = iter->second;
                            const auto pData = rowEntryIter->second;
                            if (pData)
                                strm.write(pData, strlen(pData)); // TODO encoding
                            ++rowEntryIter;
                            if (rowEntryIter == m_colToRows[nCol].cend())
                                nextColItemRowIters.erase(iter);
                        }
                        // TODO: write separator as defined in settings taking encoding into account.
                        strm.write(",", 1);
                    }
                }
            }
        };

} } // module namespace

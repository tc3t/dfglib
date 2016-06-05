#pragma once

#include "table.hpp"
#include "../io/textencodingtypes.hpp"
#include "../io/EndOfLineTypes.hpp"
#include "../io/DelimitedTextReader.hpp"
#include "../io/DelimitedTextWriter.hpp"
#include "../io/fileToByteContainer.hpp"
#include "../readOnlyParamStr.hpp"
#include "../io/IfStream.hpp"
#include "../io.hpp"
#include "../io/ImStreamWithEncoding.hpp"
#include "../utf.hpp"
#include <unordered_map>

DFG_ROOT_NS_BEGIN{ 
    
    class DFG_CLASS_NAME(CsvFormatDefinition)
    {
    public:
        //DFG_CLASS_NAME(CsvFormatDefinition)(const char cSep = ::DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharAutoDetect, const char cEnc = '"', DFG_MODULE_NS(io)::EndOfLineType eol = DFG_MODULE_NS(io)::EndOfLineTypeN) :
        // Note: default values are questionable because deault read vals should have metachars, but default write vals should not.
        DFG_CLASS_NAME(CsvFormatDefinition)(const char cSep/* = ::DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharAutoDetect*/,
                                            const char cEnc/* = '"'*/,
                                            DFG_MODULE_NS(io)::EndOfLineType eol/* = DFG_MODULE_NS(io)::EndOfLineTypeN*/,
                                            DFG_MODULE_NS(io)::TextEncoding encoding/* = DFG_MODULE_NS(io)::encodingUTF8*/) :
            m_cSep(cSep),
            m_cEnc(cEnc),
            m_eolType(eol),
            m_textEncoding(encoding),
            m_bWriteHeader(true),
            m_bWriteBom(true)
        {}

        int32 separatorChar() const { return m_cSep; }
        void separatorChar(int32 cSep) { m_cSep = cSep; }
        int32 enclosingChar() const { return m_cEnc; }
        void enclosingChar(int32 cEnc) { m_cEnc = cEnc; }
        //int32 endOfLineChar() const { return m_cEol; }
        //void endOfLineChar(int32 cEol) { m_cEol = cEol; }

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
        //int32 m_cEol;
        DFG_MODULE_NS(io)::EndOfLineType m_eolType;
        DFG_MODULE_NS(io)::TextEncoding m_textEncoding;
        bool m_bWriteHeader;
        bool m_bWriteBom;
    };
    
    DFG_SUB_NS(cont)
    {

        template <class Char_T, class Index_T, DFG_MODULE_NS(io)::TextEncoding InternalEncoding_T = DFG_MODULE_NS(io)::encodingUTF8>
        class DFG_CLASS_NAME(TableCsv) : public DFG_CLASS_NAME(TableSz)<Char_T, Index_T, InternalEncoding_T>
        {
        public:
            typedef DFG_ROOT_NS::DFG_CLASS_NAME(CsvFormatDefinition) CsvFormatDefinition;
            typedef typename DFG_CLASS_NAME(TableSz)<Char_T, Index_T>::ColumnIndexPairContainer ColumnIndexPairContainer;

            DFG_CLASS_NAME(TableCsv)() :
                m_readFormat(',', '"', DFG_MODULE_NS(io)::EndOfLineTypeN, DFG_MODULE_NS(io)::encodingUTF8)
            {}

            // TODO: test
            bool isContentAndSizesIdenticalWith(const DFG_CLASS_NAME(TableCsv)& other) const
            {
                const auto nRows = this->rowCountByMaxRowIndex();
                const auto nCols = this->colCountByMaxColIndex();
                if (nRows != other.rowCountByMaxRowIndex() || nCols != other.colCountByMaxColIndex())
                    return false;
                
                // TODO: optimize, this is quickly done, inefficient implementation.
                for (size_t r = 0; r < nRows; ++r)
                {
                    for (size_t c = 0; c < nCols; ++c)
                    {
                        auto p0 = (*this)(r, c);
                        auto p1 = other(r, c);
                        if ((!p0 && p1) || (p0 && !p1) || (std::strcmp(p0, p1) != 0)) // TODO: Create comparison function instead of using strcmp().
                            return false;
                    }
                }
                return true;
            }
                

            void readFromFile(const DFG_CLASS_NAME(ReadOnlyParamStrC)& sPath) { readFromFileImpl(sPath, defaultReadFormat()); }
            void readFromFile(const DFG_CLASS_NAME(ReadOnlyParamStrW)& sPath) { readFromFileImpl(sPath, defaultReadFormat()); }
            void readFromFile(const DFG_CLASS_NAME(ReadOnlyParamStrC)& sPath, const CsvFormatDefinition& formatDef) { readFromFileImpl(sPath, formatDef); }
            void readFromFile(const DFG_CLASS_NAME(ReadOnlyParamStrW)& sPath, const CsvFormatDefinition& formatDef) { readFromFileImpl(sPath, formatDef); }

            template <class Char_T1>
            void readFromFileImpl(const DFG_CLASS_NAME(ReadOnlyParamStr)<Char_T1>& sPath, const CsvFormatDefinition& formatDef)
            {
                bool bRead = false;
                try
                {
                    auto bytes = DFG_MODULE_NS(io)::fileToVector(sPath);
                    readFromMemory(bytes.data(), bytes.size(), formatDef);
                    bRead = true;
                }
                catch (...)
                {}

                if (!bRead)
                {
                    DFG_MODULE_NS(io)::DFG_CLASS_NAME(IfStreamWithEncoding) istrm;
                    istrm.open(sPath);
                    read(istrm, formatDef);
                    m_readFormat.textEncoding(istrm.encoding());
                }
            }

            CsvFormatDefinition defaultReadFormat()
            {
                return CsvFormatDefinition(DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharAutoDetect,
                                                            '"',
                                                            DFG_MODULE_NS(io)::EndOfLineTypeN,
                                                            DFG_MODULE_NS(io)::encodingUnknown);
            }


            void readFromMemory(const char* const pData, const size_t nSize)
            {
                readFromMemory(pData, nSize, this->defaultFormat());
            }

            void readFromMemory(const char* const pData, const size_t nSize, const CsvFormatDefinition& formatDef)
            {
                DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream) strmBom(pData, nSize);
                const auto encoding = DFG_MODULE_NS(io)::checkBOM(strmBom);
                if (encoding == DFG_MODULE_NS(io)::encodingUnknown)
                {
                    // TODO: read encoding/code page hint from options.
                    DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream) strm(pData, nSize);
                    read(strm, formatDef);
                }
                else if (encoding == DFG_MODULE_NS(io)::encodingUTF8) // With UTF8 the data can be directly read as bytes.
                {
                    const auto nBomSize = DFG_MODULE_NS(utf)::bomSizeInBytes(DFG_MODULE_NS(io)::encodingUTF8);
                    DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicImStream) strm(pData + nBomSize, nSize - nBomSize);
                    read(strm, formatDef, DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::CharAppenderDefault<std::string, char>());
                }
                else // Case: Known encoding, read using encoding istream.
                {
                    DFG_MODULE_NS(io)::DFG_CLASS_NAME(ImStreamWithEncoding) strm(pData, nSize, DFG_MODULE_NS(io)::encodingUnknown);
                    read(strm, formatDef);
                }
                m_readFormat.textEncoding(encoding);
            }

            template <class Strm_T>
            void read(Strm_T& strm, const DFG_CLASS_NAME(CsvFormatDefinition)& formatDef)
            {
                using namespace DFG_MODULE_NS(io);
                read(strm, formatDef, DFG_CLASS_NAME(DelimitedTextReader)::CharAppenderUtf<std::string>());
            }

            template <class Strm_T, class CharAppender_T>
            void read(Strm_T& strm, const DFG_CLASS_NAME(CsvFormatDefinition)& formatDef, CharAppender_T)
            {
                using namespace DFG_MODULE_NS(io);
                this->clear();
                typedef DFG_CLASS_NAME(DelimitedTextReader)::CellData<Char_T, Char_T, std::basic_string<Char_T>, CharAppender_T> Cdt;
                Cdt cellDataHandler(formatDef.separatorChar(), formatDef.enclosingChar(), eolCharFromEndOfLineType(formatDef.eolType()));

                auto reader = DFG_CLASS_NAME(DelimitedTextReader)::createReader(strm, cellDataHandler);
                auto cellHandler = [&](const size_t nRow, const size_t nCol, const decltype(cellDataHandler)& cdh)
                {
                    this->setElement(nRow, nCol, cdh.getBuffer());
                };
                DFG_CLASS_NAME(DelimitedTextReader)::read(reader, cellHandler);
                const auto& readFormat = reader.getFormatDefInfo();

                m_readFormat.separatorChar(readFormat.getSep());
                m_readFormat.enclosingChar(readFormat.getEnc());
                m_readFormat.eolType(DFG_MODULE_NS(io)::EndOfLineTypeN); // TODO: use the one from used in the read stream.
                //m_readFormat.endOfLineChar(readFormat.getEol());
                //m_readFormat.textEncoding(strm.encoding()); // This is set 
                //m_readFormat.headerWriting(); //
                //m_readFormat.bomWriting(); // TODO: This is should be enquiried from the stream whether the stream had BOM.
            }

            template <class Stream_T>
            class WritePolicySimple
            {
            public:
                WritePolicySimple(const CsvFormatDefinition& csvFormat) :
                    m_format(csvFormat)
                {
                    const auto cSep = m_format.separatorChar();
                    const auto cEol = DFG_MODULE_NS(io)::eolCharFromEndOfLineType(m_format.eolType());
                    const auto encoding = m_format.textEncoding();
                    DFG_MODULE_NS(utf)::cpToEncoded(cSep, std::back_inserter(m_bytes), encoding);
                    m_nEncodedSepSizeInBytes = m_bytes.size();
                    DFG_MODULE_NS(utf)::cpToEncoded(cEol, std::back_inserter(m_bytes), encoding);
                    m_nEncodedEolSizeInBytes = m_bytes.size() - m_nEncodedSepSizeInBytes;
                    // Note: set the pointers after all no more bytes are written to m_bytes to make sure that 
                    //       there will be no pointer invalidating reallocation.
                    m_pEncodedSep = ptrToContiguousMemory(m_bytes);
                    m_pEncodedEol = m_pEncodedSep + m_nEncodedSepSizeInBytes;
                }

                void writeItemFunc(Stream_T& strm, int c)
                {
                    m_workBytes.clear();
                    DFG_MODULE_NS(utf)::cpToEncoded(c, std::back_inserter(m_workBytes), m_format.textEncoding());
                    DFG_MODULE_NS(io)::writeBinary(strm, ptrToContiguousMemory(m_workBytes), m_workBytes.size());
                }

                void write(Stream_T& strm, const char* pData, const Index_T/*nRow*/, const Index_T/*nCol*/)
                {
                    using namespace DFG_MODULE_NS(io);
                    if (pData == nullptr)
                        return;
                    utf8::unchecked::iterator<const char*> inputIter(pData);
                    utf8::unchecked::iterator<const char*> inputIterEnd(pData + std::strlen(pData));
                    DFG_CLASS_NAME(DelimitedTextCellWriter)::writeCellFromStrIter(strm,
                                                                                 makeRange(inputIter, inputIterEnd),
                                                                                 uint32(m_format.separatorChar()),
                                                                                 uint32(m_format.enclosingChar()),
                                                                                 uint32(eolCharFromEndOfLineType(m_format.eolType())),
                                                                                 DFG_MODULE_NS(io)::EbEncloseIfNeeded,
                                                                                 [&](Stream_T& strm, int c) {this->writeItemFunc(strm, c); });
                }

                void writeSeparator(Stream_T& strm, const Index_T /*nRow*/, const Index_T /*nCol*/)
                {
                    DFG_MODULE_NS(io)::writeBinary(strm, m_pEncodedSep, m_nEncodedSepSizeInBytes);
                }

                void writeEol(Stream_T& strm)
                {
                    DFG_MODULE_NS(io)::writeBinary(strm, m_pEncodedEol, m_nEncodedEolSizeInBytes);
                }

                void writeBom(Stream_T& strm)
                {
                    const auto bomBytes = DFG_MODULE_NS(utf)::encodingToBom(m_format.textEncoding());
                    strm.write(bomBytes.data(), bomBytes.size());
                }

                CsvFormatDefinition m_format;
                std::string m_bytes;
                std::string m_workBytes;
                const char* m_pEncodedSep;
                const char* m_pEncodedEol;
                uint32 m_nEncodedSepSizeInBytes;
                uint32 m_nEncodedEolSizeInBytes;
            };

            template <class Stream_T>
            auto createWritePolicy() const -> WritePolicySimple<Stream_T>
            {
                return WritePolicySimple<Stream_T>(m_readFormat);
            }
            
            // Strm must have write()-method which writes bytes untranslated.
            //  TODO: test writing non-square data.
            template <class Strm_T, class Policy_T>
            void writeToStream(Strm_T& strm, Policy_T& policy) const
            {
                policy.writeBom(strm);

                
                if (this->m_colToRows.empty())
                    return;
                // nextColItemRowIters[i] is the valid iterator to the next row entry in column i.
                std::unordered_map<Index_T, typename ColumnIndexPairContainer::const_iterator> nextColItemRowIters;
                this->forEachFwdColumnIndex([&](const Index_T nCol)
                {
                    if (!this->m_colToRows.empty())
                        nextColItemRowIters[nCol] = this->m_colToRows[nCol].cbegin();
                });
                const auto nMaxColCount = this->colCountByMaxColIndex();
                for (Index_T nRow = 0; !nextColItemRowIters.empty(); ++nRow)
                {
                    for (Index_T nCol = 0; nCol < nMaxColCount; ++nCol)
                    {
                        auto iter = nextColItemRowIters.find(nCol);
                        if (iter != nextColItemRowIters.end() && iter->second->first == nRow) // Case: (row, col) has item
                        {
                            auto& rowEntryIter = iter->second;
                            const auto pData = rowEntryIter->second;
                            policy.write(strm, pData, rowEntryIter->first, nCol);
                            ++rowEntryIter;
                            if (rowEntryIter == this->m_colToRows[nCol].cend())
                                nextColItemRowIters.erase(iter);
                        }
                        if (nCol + 1 < nMaxColCount) // Write separator for all but the last column.
                            policy.writeSeparator(strm, nRow, nCol);
                    }
                    if (!nextColItemRowIters.empty()) // Don't write eol after last line. TODO: make this customisable.
                        policy.writeEol(strm);
                }
            }

            // Convenience overload, see implementation version for comments.
            template <class Strm_T>
            void writeToStream(Strm_T& strm) const
            {
                auto policy = createWritePolicy<Strm_T>();
                writeToStream(strm, policy);
            }

            DFG_CLASS_NAME(CsvFormatDefinition) m_readFormat; // Stores the format of previously read input. If no read is done, stores to default output format.
                                                              // TODO: specify content in case of interrupted read.
        }; // class CsvTable

} } // module namespace

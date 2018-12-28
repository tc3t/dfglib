#pragma once

#include "../dfgDefs.hpp"
#include "../str/string.hpp"
#include "../ReadOnlySzParam.hpp"
#include "MapVector.hpp"
#include "../os/fileSize.hpp"
#include "../io/DelimitedTextReader.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

    namespace DFG_DETAIL_NS
    {
        class UriHelper
        {
        public:
            typedef DFG_CLASS_NAME(StringUtf8) StringT;

            void setTop(const StringT& s, const size_t n)
            {
                if (n == 0)
                    m_sUri = s.rawStorage();
                else
                {
                    size_t nInsertPos = 0;
                    for (size_t i = 0; i < n; ++i)
                    {
                        nInsertPos = m_sUri.find('/', nInsertPos);
                        if (nInsertPos == std::string::npos)
                        {
                            if (i + 1 < n)
                            {
                                DFG_ASSERT_CORRECTNESS(false);
                                return;
                            }
                            else
                            {
                                m_sUri += "/";
                                nInsertPos = m_sUri.size();
                            }
                        }
                        else
                            nInsertPos++; // Skip /
                    }
                    m_sUri.insert(nInsertPos, s.rawStorage());
                    m_sUri.resize(nInsertPos + s.size());
                }
            }

            StringT str() const
            {
                return StringT::fromRawString(m_sUri);
            }

        private:
            std::string m_sUri; // UTF8, stored as std::string for convenience.
        };
    } // DFG_DETAIL_NS 


    class DFG_CLASS_NAME(CsvConfig)
    {
    public:
        typedef DFG_CLASS_NAME(StringUtf8) StorageStringT;
        typedef DFG_CLASS_NAME(StringViewUtf8) StringViewT;

        void loadFromFile(const DFG_CLASS_NAME(StringViewSzC)& svConfFilePath)
        {
            using namespace DFG_ROOT_NS;
            using namespace DFG_MODULE_NS(io);
            typedef DFG_CLASS_NAME(DelimitedTextReader) DelimReader;
            typedef DelimReader::CharAppenderUtf<DelimReader::CharBuffer<char>> CharAppenderUtfT;

            // Check for ridiculously large file.
            if (DFG_MODULE_NS(os)::fileSize(ReadOnlySzParamC(svConfFilePath.c_str())) > 100000000) // 100 MB
                return;

            DelimReader::FormatDefinitionSingleChars formatDef('"', '\n', ',');
            DelimReader::CellData<char, char, DelimReader::CharBuffer<char>, CharAppenderUtfT> cd(formatDef);

            DFG_DETAIL_NS::UriHelper baseUri;
            const size_t notFoundCol = DFG_ROOT_NS::NumericTraits<size_t>::maxValue;
            size_t nFirstNonEmptyCol = notFoundCol;
            DelimReader::readFromFile(ReadOnlySzParamC(svConfFilePath.c_str()), cd, [&](const size_t /*nRow*/, const size_t nCol, decltype(cd)& cellData)
            {
                typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(CsvConfig)::StorageStringT StorageStringT_vc2010_workaround;
                if (nCol == 0)
                    nFirstNonEmptyCol = notFoundCol;

                // When first non-empty has been encountered and this is the next column, read cell and skip rest of line.
                if (nFirstNonEmptyCol != notFoundCol && nCol == nFirstNonEmptyCol + 1)
                {
                    cellData.setReadStatus(DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::cellHrvSkipRestOfLine);
                    const auto& buffer = cellData.getBuffer();
                    m_mapKeyToValue[baseUri.str()] = StorageStringT_vc2010_workaround(SzPtrUtf8(buffer.data()), SzPtrUtf8(buffer.data() + buffer.size()));
                    return;
                }

                if (cellData.empty())
                    return;
                if (nFirstNonEmptyCol == notFoundCol)
                {
                    nFirstNonEmptyCol = nCol;
                    baseUri.setTop(StorageStringT_vc2010_workaround::fromRawString(cellData.getBuffer()), nCol);
                }
            });
        }

        StorageStringT value(const StringViewT& svUri, const StringViewT& svDefault = StringViewT()) const
        {
            auto p = valueStrOrNull(svUri);
            return (p) ? *p : svDefault.toString();
        }

        const StorageStringT* valueStrOrNull(const StringViewT& svUri) const
        {
            auto iter = m_mapKeyToValue.find(svUri);
            return (iter != m_mapKeyToValue.end()) ? &iter->second : nullptr;
        }

        size_t entryCount() const
        {
            return m_mapKeyToValue.size();
        }

        DFG_CLASS_NAME(MapVectorSoA)<StorageStringT, StorageStringT> m_mapKeyToValue;

    }; // CsvConfig
    
} } // Module namespace 

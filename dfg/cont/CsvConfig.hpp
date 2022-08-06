#pragma once

#include "../dfgDefs.hpp"
#include "../str/string.hpp"
#include "../ReadOnlySzParam.hpp"
#include "MapVector.hpp"
#include "../os/fileSize.hpp"
#include "../os/OutputFile.hpp"
#include "../io/DelimitedTextReader.hpp"
#include "../io/OfStream.hpp"
#include "../io/DelimitedTextWriter.hpp"
#include "Vector.hpp"
#include "../Span.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

    namespace DFG_DETAIL_NS
    {
        class UriHelper
        {
        public:
            typedef StringUtf8 StringT;

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


    class CsvConfig
    {
    public:
        typedef StringUtf8 StorageStringT;
        typedef StringViewUtf8 StringViewT;

        static CsvConfig fromMemory(const Span<const char> bytes, const ::DFG_MODULE_NS(io)::TextEncoding encoding = ::DFG_MODULE_NS(io)::encodingUnknown);
        static CsvConfig fromUtf8(const StringViewUtf8 svData);

        void loadFromMemory(const Span<const char> bytes, const ::DFG_MODULE_NS(io)::TextEncoding encoding = ::DFG_MODULE_NS(io)::encodingUnknown);

        void loadFromFile(const StringViewSzC& svConfFilePath)
        {
            const auto nFileSize = ::DFG_MODULE_NS(os)::fileSize(ReadOnlySzParamC(svConfFilePath.c_str()));

            // Check for empty/non-existent file.
            if (nFileSize <= 0)
                return;

            // Ignoring unexpectedly large files
            if (nFileSize > 100000000) // 100 MB
                return;

            loadFromMemory(::DFG_MODULE_NS(io)::fileToMemory_readOnly(svConfFilePath).asSpanFromRValue<char>());
        }

        bool contains(const StringViewT svUri) const
        {
            auto p = valueStrOrNull(svUri);
            return (p != nullptr);
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

        bool empty() const
        {
            return m_mapKeyToValue.empty();
        }

        void clear()
        {
            m_mapKeyToValue.clear();
        }

        // Calls 'func' for every key starting with 'uriStart' passing two StringViewUtf8 arguments to 'func': (key excluding 'uriStart', value).
        template <class Func_T>
        void forEachStartingWith(const StringViewT& uriStart, Func_T&& func) const
        {
            auto iter = std::lower_bound(m_mapKeyToValue.beginKey(), m_mapKeyToValue.endKey(), uriStart);
            for (; iter != m_mapKeyToValue.endKey() && DFG_MODULE_NS(str)::beginsWith(StringViewT(*iter), uriStart); ++iter)
            {
                // Skip leading uri with raw pointer handling.
                auto fullUri = StringViewT(*iter);
                auto p = fullUri.beginRaw();
                p += std::distance(uriStart.beginRaw(), uriStart.endRaw());
                func(StringViewT(SzPtrUtf8(p), std::distance(p, fullUri.endRaw())), m_mapKeyToValue.keyIteratorToValue(iter));
            }
        }

        // Calls func for every key/value pair. func should accept two arguments of type StringViewT.
        template <class Func_T>
        void forEachKeyValue(Func_T&& func) const
        {
            forEachStartingWith(DFG_UTF8(""), std::forward<Func_T>(func));
        }

        template <class Strm_T, class Item_T>
        static void privWriteCellToStream(Strm_T& strm, const Item_T& item)
        {
            typedef DFG_MODULE_NS(io)::DelimitedTextCellWriter Writer;
            Writer::writeCellStrm(strm, item, ',', '"', '\n', DFG_MODULE_NS(io)::EbEncloseIfNeeded);
        }

        template <class Strm_T, class Item0_T, class Item1_T>
        static void privWriteCellsToStream(Strm_T& strm, const Item0_T& item0, const Item1_T& item1)
        {
            privWriteCellToStream(strm, item0);
            strm << ',';
            privWriteCellToStream(strm, item1);
            strm << '\n';
        }

        bool saveToFile(const dfg::StringViewSzC& svPath) const
        {
            ::DFG_MODULE_NS(os)::OutputFile_completeOrNone<> outputFile(svPath.c_str());

            auto& ostrm = outputFile.intermediateFileStream();

            if (!ostrm.is_open())
                return false;

            saveToStream(ostrm);

            return (outputFile.writeIntermediateToFinalLocation() == 0);
        }

        StringUtf8 saveToMemory() const;

        template <class Strm_T>
        void saveToStream(Strm_T& strm) const;

        void setKeyValue(StorageStringT sKey, StorageStringT sValue)
        {
            m_mapKeyToValue[std::move(sKey)] = std::move(sValue);
        }

        void setKeyValue(SzPtrUtf8R pszKey, SzPtrUtf8R pszValue)
        {
            setKeyValue(StorageStringT(pszKey), StorageStringT(pszValue));
        }

        // Convenience function for effecticvely doing setKeyValue(StorageStringT::fromRawString(svKey), StorageStringT::fromRawString(svValue))
        // Note: relies on caller guarantee that given untyped views are valid for fromRawString()
        void setKeyValue_fromUntyped(StringViewC svKey, StringViewC svValue)
        {
            setKeyValue(StorageStringT::fromRawString(svKey.toString()), StorageStringT::fromRawString(svValue.toString()));
        }

        // Given a uri of type a/b/c, returns n'th part, e.g. uriPart("a/b/c", 0) == "a", uriPart("a/b/c", 2) == "c". If part not present, returns empty view.
        // Note: Returned view refers to the same underlying string as 'svUri' -> need to be careful if svUri refers to temporary string.
        static StringViewT uriPart(const StringViewT svUri, const size_t n);

        // TODO: test
        bool operator==(const CsvConfig& other) const
        {
            if (m_mapKeyToValue.size() != other.m_mapKeyToValue.size())
                return false;
            auto iter = m_mapKeyToValue.cbegin();
            const auto iterEnd = m_mapKeyToValue.cend();
            auto iterOther = other.m_mapKeyToValue.cbegin();
            for (; iter != iterEnd; ++iter, ++iterOther)
            {
                if (iter->first != iterOther->first || iter->second != iterOther->second)
                    return false;
            }
            return true;
        }

        bool operator!=(const CsvConfig& other) const
        {
            return !(*this == other);
        }

        MapVectorSoA<StorageStringT, StorageStringT> m_mapKeyToValue;

    }; // CsvConfig

    inline void CsvConfig::loadFromMemory(const Span<const char> bytes, const ::DFG_MODULE_NS(io)::TextEncoding encoding)
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(io);
        typedef DelimitedTextReader DelimReader;
        typedef DelimReader::CharAppenderUtf<DelimReader::CharBuffer<char>> CharAppenderUtfT;

        DelimReader::FormatDefinitionSingleChars formatDef('"', '\n', ',');
        DelimReader::CellData<char, char, DelimReader::CharBuffer<char>, CharAppenderUtfT> cd(formatDef);

        DFG_DETAIL_NS::UriHelper baseUri;
        const size_t notFoundCol = DFG_ROOT_NS::NumericTraits<size_t>::maxValue;
        size_t nFirstNonEmptyCol = notFoundCol;
        bool bMostRecentRowHadKeyButNoValue = false;
        DelimReader::readFromMemory(bytes.data(), bytes.size(), cd, [&](const size_t /*nRow*/, const size_t nCol, decltype(cd)& cellData)
            {
                typedef DFG_MODULE_NS(cont)::CsvConfig::StorageStringT StorageStringT_vc2010_workaround;
                if (nCol == 0)
                {
                    if (bMostRecentRowHadKeyButNoValue)
                        m_mapKeyToValue[baseUri.str()] = DFG_UTF8("");
                    nFirstNonEmptyCol = notFoundCol;
                    bMostRecentRowHadKeyButNoValue = false;
                }

                // When first non-empty has been encountered and this is the next column, read cell and skip rest of line.
                if (nFirstNonEmptyCol != notFoundCol && nCol == nFirstNonEmptyCol + 1)
                {
                    bMostRecentRowHadKeyButNoValue = false;
                    cellData.setReadStatus(DFG_MODULE_NS(io)::DelimitedTextReader::cellHrvSkipRestOfLine);
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
                    bMostRecentRowHadKeyButNoValue = true;
                }
            }, encoding);
        if (bMostRecentRowHadKeyButNoValue)
            m_mapKeyToValue[baseUri.str()] = DFG_UTF8("");
    }

    inline CsvConfig CsvConfig::fromUtf8(const StringViewUtf8 svData)
    {
        return fromMemory(svData.asUntypedView(), ::DFG_MODULE_NS(io)::encodingUTF8);
    }

    inline CsvConfig CsvConfig::fromMemory(const Span<const char> bytes, const ::DFG_MODULE_NS(io)::TextEncoding encoding)
    {
        CsvConfig csvConfig;
        csvConfig.loadFromMemory(bytes, encoding);
        return csvConfig;
    }

    inline StringUtf8 CsvConfig::saveToMemory() const
    {
        StringUtf8 rv;
        ::DFG_MODULE_NS(io)::BasicOmcByteStream<StringUtf8::StorageType> ostrm(&rv.rawStorage());
        saveToStream(ostrm);
        return rv;
    }

    template <class Strm_T>
    inline void CsvConfig::saveToStream(Strm_T& ostrm) const
    {
        using SvC = StringViewC;
        // Writing BOM
        {
            const auto bomBytes = ::DFG_MODULE_NS(utf)::encodingToBom(DFG_MODULE_NS(io)::encodingUTF8);
            ostrm.write(bomBytes.data(), sizeInBytes(bomBytes));
        }

        // Writing empty header.
        ostrm << ",,,\n";

        Vector<SvC> uriStack; // Can use string views as the underlying strings in map won't get invalidated.
        for (auto iter = m_mapKeyToValue.cbegin(), iterEnd = m_mapKeyToValue.cend(); iter != iterEnd; ++iter)
        {
            const auto& uriRawStorage = iter->first.rawStorage();
            auto nLastSep = uriRawStorage.find_last_of('/');
            if (nLastSep == std::string::npos) // No separators? (i.e. top-level entry?)
            {
                uriStack.assign(1, uriRawStorage);
                privWriteCellsToStream(ostrm, uriRawStorage, iter->second.rawStorage());
            }
            else
            {
                const auto nSepCount = std::count(uriRawStorage.cbegin(), uriRawStorage.cend(), '/');
                auto iterCurrentPos = uriRawStorage.cbegin();
                auto iterNextSep = iterCurrentPos;
                for (ptrdiff_t i = 0; i < nSepCount; ++i)
                {
                    iterNextSep = std::find(iterCurrentPos, uriRawStorage.cend(), '/');
                    DFG_ASSERT_CORRECTNESS(iterNextSep != uriRawStorage.cend());
                    const SvC sv(&*iterCurrentPos, std::distance(iterCurrentPos, iterNextSep));
                    if (!isValidIndex(uriStack, i) || !(uriStack[i] == sv))
                    {
                        // Don't have this level already in stack; create a new row for this level.
                        uriStack.resize(i + 1);
                        for (int j = 0; j < i; ++j)
                            ostrm << ',';
                        privWriteCellToStream(ostrm, sv);
                        ostrm << '\n';
                        uriStack[i] = sv;
                    }
                    iterCurrentPos = iterNextSep + 1;
                }
                for (int j = 0; j < nSepCount; ++j)
                    ostrm << ',';
                const SvC svLastPart(&*iterCurrentPos, std::distance(iterCurrentPos, uriRawStorage.cend()));
                uriStack.resize(nSepCount + 1);
                uriStack.back() = svLastPart;
                privWriteCellsToStream(ostrm, svLastPart, iter->second.rawStorage());
            }
        }
    }

    inline auto CsvConfig::uriPart(const StringViewT svUri, const size_t n) -> StringViewT
    {
        const auto svUntyped = svUri.asUntypedView();
        const char* p = svUntyped.data();
        const char* const pEnd = svUntyped.data() + svUntyped.size();
        const char* pPartEnd = pEnd;
        for (size_t i = 0; i < n + 1u && p != pEnd; ++i)
        {
            pPartEnd = std::find(p, pEnd, '/');
            if (i == n)
                break;
            else if (pPartEnd != pEnd)
                p = pPartEnd + 1;
            else
                p = pPartEnd;
        }
        return StringViewUtf8(TypedCharPtrUtf8R(p), TypedCharPtrUtf8R(pPartEnd));
    }
    
} } // Module namespace 

#pragma once

#include "../dfgDefs.hpp"
#include <random>
#include <type_traits>
#include "../utf.hpp"
#include "../iter/szIterator.hpp"
#include "removeFile.hpp"
#include "../io/openOfStream.hpp"

#ifdef _WIN32
    #include "win.hpp"
#endif // _WIN32

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(os)
{
    inline DFG_CONSTEXPR char nativeSeparatorChar()
    {
#ifdef _WIN32
        return '\\';
#else
        return '/';
#endif
    }
        namespace DFG_DETAIL_NS
        {
            // Implement opening stream through open()-method for object of type other than std::ofstream.
            template <class Stream_T, class String_T>
            typename std::enable_if<!std::is_same<std::ofstream, Stream_T>::value, void>::type openStream(Stream_T& strm, const String_T& sPath) 
            {
                strm.open(sPath, std::ios::out | std::ios::binary | std::ios::trunc);
            }
 
            // Implement opening std::ofstream through openOfStream().
            template <class String_T>
            void openStream(std::ofstream& strm, const String_T& sPath) 
            {
                DFG_MODULE_NS(io)::openOfStream(strm, sPath, std::ios::out | std::ios::binary | std::ios::trunc);
            }
        }


        /* Creates temporary files guaranteeing std::ostream access to the created temp file.
        Related code
            -QTemporaryFile
            -std::tmpnam (http://en.cppreference.com/w/cpp/io/c/tmpnam)
            -std::tmpfile (http://en.cppreference.com/w/cpp/io/c/tmpfile)
        TODO: wchar_t -> char conversions assume that every wchar_t is an individual code point; while it should work in vast majority of cases, this is not correct.
        */
        template <class Stream_T = std::ofstream>
        class DFG_CLASS_NAME(TemporaryFileStreamT)
        {
        public:
            typedef wchar_t CharT;
            typedef std::basic_string<CharT> StringT;

            DFG_CLASS_NAME(TemporaryFileStreamT)() : 
                m_bRemoveFileOnClose(true)
            {
                init(nullptr, nullptr, nullptr, nullptr);
            }

            DFG_CLASS_NAME(TemporaryFileStreamT)(const char* pPszFolderPath,
                const char* pPszNamePrefix,
                const char* pPszNameSuffix,
                const char* pPszExtensionWithOrWithoutDot,
                const size_t nRandomNameChars = 8) :
                    m_bRemoveFileOnClose(true)
            {
                const auto charToWcharConverter = [](const char* psz) -> std::wstring
                                                    {
                                                        //return DFG_MODULE_NS(utf)::utf8ToFixedChSizeStr<CharT>(makeSzRange(psz));
                                                        return std::wstring(psz, psz + std::strlen(psz)); // Convert from latin1.
                                                    };
                StringT sFolderPath = (pPszFolderPath) ? charToWcharConverter(pPszFolderPath) : StringT();
                StringT sNamePrefix = (pPszNamePrefix) ? charToWcharConverter(pPszNamePrefix) : StringT();
                StringT sNameSuffix = (pPszNameSuffix) ? charToWcharConverter(pPszNameSuffix) : StringT();
                StringT sExt = (pPszExtensionWithOrWithoutDot) ? charToWcharConverter(pPszExtensionWithOrWithoutDot) : StringT();
                init((pPszFolderPath) ? sFolderPath.c_str() : nullptr,
                    (pPszNamePrefix) ? sNamePrefix.c_str() : nullptr,
                    (pPszNameSuffix) ? sNameSuffix.c_str() : nullptr,
                    (pPszExtensionWithOrWithoutDot) ? sExt.c_str() : nullptr,
                    nRandomNameChars);
            }

            DFG_CLASS_NAME(TemporaryFileStreamT)(const wchar_t* pPszFolderPath,
                const wchar_t* pPszNamePrefix,
                const wchar_t* pPszNameSuffix,
                const wchar_t* pPszExtensionWithOrWithoutDot,
                const size_t nRandomNameChars = 8) :
                    m_bRemoveFileOnClose(true)
            {
                init(pPszFolderPath, pPszNamePrefix, pPszNameSuffix, pPszExtensionWithOrWithoutDot, nRandomNameChars);
            }

            void init(const wchar_t* pPszFolderPath = nullptr,
                const wchar_t* pPszNamePrefix = nullptr,
                const wchar_t* pPszNameSuffix = nullptr,
                const wchar_t* pPszExtensionWithOrWithoutDot = nullptr,
                const size_t nRandomNameChars = 8)
            {
                for (int i = 0; i < 10; ++i) // Try 10 times in case that opening fails; in default case highly unexpected because of 8 random chars.
                {
                    StringT sFilename;
                    if (pPszNamePrefix)
                        sFilename = pPszNamePrefix;
                    sFilename += generateRandomFileNamePart(nRandomNameChars);
                    if (pPszNameSuffix)
                        sFilename += pPszNameSuffix;
                    if (pPszExtensionWithOrWithoutDot && *pPszExtensionWithOrWithoutDot != '\0')
                    {
                        if (*pPszExtensionWithOrWithoutDot == '.')
                            pPszExtensionWithOrWithoutDot++;
                        sFilename.append(1, '.');
                        sFilename += pPszExtensionWithOrWithoutDot;
                    }
                    StringT sPath = (pPszFolderPath) ? pPszFolderPath : tempFolderPath();
                    if (!sPath.empty() && sPath.back() != nativeSeparatorChar())
                        sPath.push_back(nativeSeparatorChar());
                    sPath += sFilename;

                    DFG_DETAIL_NS::openStream(m_ostrm, sPath);

                    if (m_ostrm.is_open() && m_ostrm.good())
                    {
                        m_sPath = std::move(sPath);
                        break;
                    }
                    else
                        m_ostrm.clear();
                }
            }

            ~DFG_CLASS_NAME(TemporaryFileStreamT)()
            {
                close();
            }

            void close()
            {
                m_ostrm.close();
                if (m_bRemoveFileOnClose && !m_sPath.empty())
                    removeFile(m_sPath.c_str());
            }

            template <class T>
            DFG_CLASS_NAME(TemporaryFileStreamT)& operator<<(const T& obj)
            {
                stream() << obj;
                return *this;
            }

            void flush()
            {
                stream().flush();
            }

            std::string pathU8() const { return DFG_MODULE_NS(utf)::codePointsToUtf8(m_sPath); }
            std::string pathLatin1() const { return DFG_MODULE_NS(utf)::codePointsToLatin1(m_sPath); }
            std::wstring pathW() const { return m_sPath; }

            StringT path() const { return m_sPath; }

            Stream_T& stream() { return m_ostrm; }

            bool isOpen() const { return m_ostrm.is_open(); }

            static StringT generateRandomFileNamePart(const size_t nCountRequest = 8)
            {
                StringT s;
                std::random_device rd;
                std::uniform_int_distribution<int> uid(0, 35);
                const auto nEffectiveCount = Min(nCountRequest, size_t(64)); // Allow 64 random chars at max (rather arbitrary choice, change if need be).
                for (size_t i = 0; i < nEffectiveCount; ++i)
                {
                    const auto val = uid(rd);
                    if (val < 10)
                        s.append(1, static_cast<char>(48 + val)); // Numbers 0-9
                    else
                        s.append(1, static_cast<char>(97 + val - 10)); // small letters a-z
                }
                return s;
            }

            /* Returns temp folder path ended with separator char.
            Related code
                -QDir::tempPath
                -boost::filesystem::temp_directory_path
                -GetTempPath() (Windows)
            */
            template <class Char_T>
            static std::basic_string<Char_T> tempFolderPathT()
            {
#ifdef _WIN32
                return DFG_SUB_NS_NAME(win)::GetTempPathT<Char_T>();
#else
                std::basic_string<Char_T> s;
                const auto range = makeSzRange(P_tmpdir);
                std::copy(range.cbegin(), range.cend(), std::back_inserter(s)); // Hack: assume compatible encodings, effectively more or less that P_tmpdir is ASCII.
                if (!s.empty() && s.back() != '/')
                    s.push_back('/');
                return s;
#endif
            }

            static std::string tempFolderPathC()
            {
                return tempFolderPathT<char>();
            }

            static std::wstring tempFolderPathW()
            {
                return tempFolderPathT<wchar_t>();
            }

            static StringT tempFolderPath()
            {
                return tempFolderPathT<CharT>();
            }

            void setAutoRemove(const bool b) {m_bRemoveFileOnClose = b;}
            bool isAutoRemove() const {return m_bRemoveFileOnClose;}

            Stream_T m_ostrm;
            StringT m_sPath;
            bool m_bRemoveFileOnClose;
        }; // Class 

        typedef DFG_CLASS_NAME(TemporaryFileStreamT)<std::ofstream> DFG_CLASS_NAME(TemporaryFileStream);

}} // module namespace

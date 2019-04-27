#pragma once

#include "../dfgBase.hpp"
#include "BasicIStreamCRTP.hpp"
#include "../ReadOnlySzParam.hpp"
#include <cstdio>

#ifndef _WIN32
	#include "widePathStrToFstreamFriendlyNonWide.hpp"
#endif

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(io) {

// Basic input stream for reading data from file, currently only binary input is implemented.
class DFG_CLASS_NAME(BasicIfStream) : public DFG_CLASS_NAME(BasicIStreamCRTP)<DFG_CLASS_NAME(BasicIfStream), fpos_t>
{
public:
    enum SeekOrigin { SeekOriginBegin, SeekOriginCurrent, SeekOriginEnd};

    DFG_CLASS_NAME(BasicIfStream)(const DFG_CLASS_NAME(ReadOnlySzParamC) sPath)
    {
#ifdef _MSC_VER
        #pragma warning(disable : 4996) // This function or variable may be unsafe
#endif // _MSC_VER

        m_pFile = std::fopen(sPath.c_str(), "rb");

#ifdef _MSC_VER
        #pragma warning(default : 4996)
#endif // _MSC_VER
    }

    DFG_CLASS_NAME(BasicIfStream)(const DFG_CLASS_NAME(ReadOnlySzParamW) sPath)
    {
#ifdef _WIN32
    #ifdef _MSC_VER
        #pragma warning(disable : 4996) // This function or variable may be unsafe
    #endif // _MSC_VER

        m_pFile = _wfopen(sPath.c_str(), L"rb");

    #ifdef _MSC_VER
        #pragma warning(default : 4996)
    #endif // _MSC_VER
#else
        m_pFile = std::fopen(pathStrToFileApiFriendlyPath(sPath).c_str(), "rb");
#endif
    }

    DFG_CLASS_NAME(BasicIfStream)(DFG_CLASS_NAME(BasicIfStream)&& other) :
        m_pFile(other.m_pFile)
    {
        other.m_pFile = nullptr;
    }

    ~DFG_CLASS_NAME(BasicIfStream)()
    {
        if (m_pFile)
            std::fclose(m_pFile);
    }

    inline int get();

    inline DFG_CLASS_NAME(BasicIfStream)& read(char* p, const size_t nCount);
    inline size_t fread(char* p, const size_t nCount);
    inline size_t readBytes(char* p, const size_t nCount);

    inline PosType tellg() const;

    inline void seekg(const PosType& pos);
    inline void seekg(SeekOrigin seekOrigin, long offset);

    inline bool good() const;

    FILE* m_pFile;
};

inline int DFG_CLASS_NAME(BasicIfStream)::get()
{
    return (m_pFile != nullptr) ? fgetc(m_pFile) : eofVal();
}

inline DFG_CLASS_NAME(BasicIfStream)& DFG_CLASS_NAME(BasicIfStream)::read(char* p, const size_t nCount)
{
    fread(p, nCount);
    return *this;
}

inline size_t DFG_CLASS_NAME(BasicIfStream)::readBytes(char* p, const size_t nCount)
{
    return fread(p, nCount);
}

inline size_t DFG_CLASS_NAME(BasicIfStream)::fread(char* p, const size_t nCount)
{
    return (m_pFile) ? std::fread(p, 1, nCount, m_pFile) : 0;
}

inline bool DFG_CLASS_NAME(BasicIfStream)::good() const
{
    return (m_pFile) ? std::ferror(m_pFile) == 0 && std::feof(m_pFile) == 0 : false;
}

inline auto DFG_CLASS_NAME(BasicIfStream)::tellg() const -> PosType
{
    PosType pos = PosType();
    if (m_pFile)
        std::fgetpos(m_pFile, &pos);
    return pos;
}

inline void DFG_CLASS_NAME(BasicIfStream)::seekg(const PosType& pos)
{
    if (m_pFile)
        fsetpos(m_pFile, &pos);
}

inline void DFG_CLASS_NAME(BasicIfStream)::seekg(const SeekOrigin seekOrigin, const long offset)
{
    DFG_STATIC_ASSERT(SeekOriginBegin == SEEK_SET && SeekOriginCurrent == SEEK_CUR && SeekOriginEnd == SEEK_END, "Check SeekOriginiEnums; with current implementation should match with fseek() macros");
    if (m_pFile)
        std::fseek(m_pFile, offset, seekOrigin);
}

inline size_t readBytes(DFG_CLASS_NAME(BasicIfStream)& istrm, char* pDest, const size_t nMaxReadSize)
{
    return istrm.readBytes(pDest, nMaxReadSize);
}

}} // module io

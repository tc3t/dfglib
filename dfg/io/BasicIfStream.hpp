#pragma once

#include "../dfgBase.hpp"
#include "BasicIStreamCRTP.hpp"
#include "../readOnlyParamStr.hpp"
#include <cstdio>

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(io) {

// Basic input stream for reading data from file, currently only binary input is implemented.
class DFG_CLASS_NAME(BasicIfStream) : public DFG_CLASS_NAME(BasicIStreamCRTP)<DFG_CLASS_NAME(BasicIfStream)>
{
public:
    DFG_CLASS_NAME(BasicIfStream)(const DFG_CLASS_NAME(ReadOnlyParamStrC) sPath)
    {
        #pragma warning(disable : 4996) // This function or variable may be unsafe
        m_pFile = std::fopen(sPath.c_str(), "rb");
        #pragma warning(default : 4996)
    }

    DFG_CLASS_NAME(BasicIfStream)(const DFG_CLASS_NAME(ReadOnlyParamStrW) sPath)
    {
        #pragma warning(disable : 4996) // This function or variable may be unsafe
        m_pFile = _wfopen(sPath.c_str(), L"rb");
        #pragma warning(default : 4996)
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

inline size_t readBytes(DFG_CLASS_NAME(BasicIfStream)& istrm, char* pDest, const size_t nMaxReadSize)
{
    return istrm.readBytes(pDest, nMaxReadSize);
}

}} // module io

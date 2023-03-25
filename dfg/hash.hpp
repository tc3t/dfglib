#pragma once

#include "dfgBase.hpp"
#include "str/hex.hpp"
#include "str/stringFixedCapacity.hpp"

#if defined(_WIN32)
    #include <Windows.h>
    #include <Wincrypt.h> // For cryptographic random generation, hash generation.
    #pragma comment(lib, "Advapi32") // For hash functions.
#endif

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(hash) {

#ifdef _WIN32

enum HashType {HashTypeMd5 = CALG_MD5, HashTypeSha1 = CALG_SHA1};

//===================================================================
// Class: HashCreator
// Description: Creates hashes of bytes(MD5, SHA1)
// Example: HashCreator() hc;
//			hc.Process(buffer, nbSize);
//			hc.ToStr<TCHAR>()
// TODO: test
class HashCreator
//===================================================================
{
public:
    HashCreator(HashType hashType) : m_hCryptHash(0), m_HashType(hashType)
    {
        if (FALSE == CryptAcquireContext(&m_hCryptProv,
                                    NULL,
                                    NULL,
                                    PROV_RSA_FULL,
                                    CRYPT_VERIFYCONTEXT))
            m_hCryptProv = 0;
        else if (CryptCreateHash(m_hCryptProv, hashType, 0, 0, &m_hCryptHash) != BOOL(TRUE))
            m_hCryptHash = 0;
    }
            
    ~HashCreator()
    {
        cleanUp();
    }

    HashCreator& process(ConstVoidPtr pData, size_t nSize)
    {
        if (m_hCryptHash != 0)
        {
            // Create hash and if not succesfull, cleanup.
            if ((nSize > maxValueOfType<DWORD>()) || 
                CryptHashData(m_hCryptHash,
                            (BYTE*)pData /*For some reason this is not const although it is an input parameter*/,
                            static_cast<DWORD>(nSize), 0) != BOOL(TRUE))
                cleanUp();
        }
        return *this;
    }

    // Sets nbSize to zero on error.
    void toVal(BYTE* pBytes, DWORD& nbSize)
    {
        if (m_hCryptHash != 0)
        {
            if (CryptGetHashParam(m_hCryptHash, HP_HASHVAL, pBytes, &nbSize, 0) != BOOL(TRUE))
                nbSize = 0;
        }
    }

    // Converts hash to hex string. Returned string has length zero if an error occurred.
    template <class Char_T> StringFixedCapacity_T<Char_T, 64> toStr()
    {
        StringFixedCapacity_T<Char_T, 64> str;
        if (m_hCryptHash != 0)
        {
            BYTE bytes[32]; // For now assume 32 bytes to be enough since MD5 uses 16 and SHA1 20 bytes.
            DWORD nbSize = sizeof(bytes);
            toVal(bytes, nbSize);
            DFG_ASSERT_CORRECTNESS(str.capacity() > 2 * nbSize);
            if (nbSize != 0 && str.capacity() > 2 * nbSize)
                DFG_MODULE_NS(str)::bytesToHexStr(bytes, nbSize, str.data(), true/*true=append null*/);
        }
        return str;
    }

private:
    void cleanUp()
    {
        if(m_hCryptHash) 
            CryptDestroyHash(m_hCryptHash);
        if(m_hCryptProv) 
            CryptReleaseContext(m_hCryptProv, 0);
    }
private:
    HCRYPTPROV m_hCryptProv;
    HCRYPTHASH m_hCryptHash;
    HashType m_HashType;
};

#endif // _WIN32

}} // module hash

#pragma once

#include "dfgDefs.hpp"
#include "dfgBase.hpp" // For DFG_COUNTOF
#include <type_traits>

#ifdef _WIN32
    #include <stdexcept>
    #include <Windows.h>
    #include <Wincrypt.h> // For cryptographic random generation, hash generation.
    #pragma comment(lib, "Advapi32") // For Cryptographics functions.
#endif // _WIN32

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(crypt) {

#if defined(_WIN32)

    // Class for creating cryptographic random bytes.
    // Typical use is to create object of this class and then call Generate-method.
    // DFG_CLASS_NAME(CryptRand)().Generate(buffer);
    // DFG_CLASS_NAME(CryptRand) randCrypt; randCrypt.Generate(buffer, 10);
    // TODO: test
    class DFG_CLASS_NAME(CryptRand)
    {
    public:
        DFG_CLASS_NAME(CryptRand)()
        {
            if (CryptAcquireContext(
                &m_hCryptProv,					// A pointer to a handle of a CSP
                nullptr,						// The key container name (nullptr == user default provider is used)
                nullptr,						// A null-terminated string that contains the name of the CSP to be used (nullptr == the default key container name is used)
                PROV_RSA_FULL,					// Type of provider to acquire
                CRYPT_VERIFYCONTEXT) == 0)		// Flag values
            {
                m_hCryptProv = NULL;
                throw std::runtime_error("CryptAcquireContext failed");
            }

        }
        ~DFG_CLASS_NAME(CryptRand)()
        {
            if (m_hCryptProv != NULL)
                CryptReleaseContext(m_hCryptProv, 0);
        }

        // Generates cryptographic randombytes.
        // [in] 'nCount': Number of bytes to receive to 'buffer'.
        // [out] 'buffer': Pointer to buffer that receives the random bytes.
        // Return: true iff. generation succeeded.
        // TODO: test
        bool Generate(void* pDest, const size_t nCount)
        {
            return ((m_hCryptProv != NULL) && CryptGenRandom(m_hCryptProv, static_cast<DWORD>(nCount), reinterpret_cast<BYTE*>(pDest)) != 0);
        }

        // Generates cryptographic randombytes using given filter function, which can
        // modify and then determine whether byte is accepted.
        // For example is wishing to generate alphanumeric digits, one can supply
        // this function with std::isalnum function.
        // [out] pDest : See Generate
        // [in] nCount : See Generate.
        // [in] func   : Unary function that is given reference to generated random byte and if
        //				 it can transform it into a usable form, does the transformation(can
        //				 be identity as well) and returns non-zero. If new byte is needed,
        //				 returns zero.
        // Return      : See Generate.
        // Remarks: Using simple reject/accept functions can be inefficient as it can be rejection
        //			of most bytes (e.g. if using std::isdigit)
        // TODO: test
        template <class Func_T>
        bool Generate(const void* pDest, const size_t nCount, Func_T func)
        {
            static const size_t nRandCacheSize = 32;
            BYTE randBuffer[nRandCacheSize];
            size_t nAccepted = 0;
            while(nAccepted < nCount)
            {
                if (Generate(randBuffer, DFG_COUNTOF(randBuffer)) == false)
                    return false;
                size_t nCacheIndex = 0;
                while(nAccepted < nCount && nCacheIndex < DFG_COUNTOF(randBuffer))
                {
                    if (func(randBuffer[nCacheIndex]) != 0)
                    {
                        reinterpret_cast<BYTE*>(pDest)[nAccepted] = randBuffer[nCacheIndex];
                        nAccepted++;
                    }
                    nCacheIndex++;
                }
            }
            return true;
        }

        // Overload for randomising arbitrary object.
        template <class T>
        bool Generate(T& obj)
        //-------------------------
        {
            DFG_STATIC_ASSERT(std::is_trivially_copyable<T>::value == true, "Generate is available only for trivially copyable T");
            return Generate(&obj, sizeof(obj));
        }

        HCRYPTPROV m_hCryptProv;
    };

#endif // defined(_WIN32)


}} // module crypt

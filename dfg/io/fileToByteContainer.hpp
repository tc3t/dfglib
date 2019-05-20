#pragma once

#include "../dfgDefs.hpp"
#include <vector>
#include "../ReadOnlySzParam.hpp"
#include "BasicIfStream.hpp"
#include "../ptrToContiguousMemory.hpp"
#include "../os/fileSize.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

    namespace DFG_DETAIL_NS
    {
        // Value is not really based on anything -> likely not optimal. BUFSIZ might be an option.
        // On MSVC 2017 BUFSIZ seems to be 512, on Clang 6.0.0 and GCC 7.4 8192.
        const size_t gnDefaultFileToMemReadStep = 512;
    }
    
    // Reads bytes from stream and returns the number of bytes read.
    // If nMaxReadSize is not multiple of sizeof(char_type), used read count is biggest multiple of sizeof(char_type) that is less than nMaxReadSize.
    template <class Stream_T> size_t readBytes(Stream_T& istrm, char* pDest, const size_t nMaxReadSize)
    {
        typedef typename Stream_T::char_type CharType;
        const auto nReadCountInChars = nMaxReadSize / sizeof(CharType);
        istrm.read(reinterpret_cast<CharType*>(pDest), nReadCountInChars);
        const auto nRead = istrm.gcount() * sizeof(CharType);
        return static_cast<size_t>(nRead);
    }

    template <class Cont_T, class Stream_T>
    Cont_T readAllFromStream(Stream_T& istrm,
                            size_t nReadStepSize = DFG_DETAIL_NS::gnDefaultFileToMemReadStep,
                            const size_t nSizeHint = 0,
                            const size_t nMaxSize = NumericTraits<size_t>::maxValue)
    {
        Cont_T cont;
        if (nSizeHint > 0)
            cont.reserve(nSizeHint);
        if (nReadStepSize == 0)
            nReadStepSize = DFG_DETAIL_NS::gnDefaultFileToMemReadStep;
        while (istrm.good() && cont.size() < nMaxSize)
        {
            const auto nOldSize = cont.size();
            cont.resize(nOldSize + nReadStepSize);
            const auto nRead = readBytes(istrm, ptrToContiguousMemory(cont) + nOldSize, cont.size() - nOldSize);
            cont.resize(nOldSize + nRead);
        }
        if (cont.size() > nMaxSize)
            cont.resize(nMaxSize);
        return std::move(cont);
    }


    // Reads file to contiguous container of bytes (e.g. std::vector<char>).
    template <class Cont_T, class Char_T>
    Cont_T fileToByteContainerImpl(const DFG_CLASS_NAME(ReadOnlySzParam)<Char_T> sFilePath,
        size_t nReadStepSize = DFG_DETAIL_NS::gnDefaultFileToMemReadStep,
        size_t nSizeHint = 0,
        const size_t nMaxSize = NumericTraits<size_t>::maxValue
        )
    {
        if (nSizeHint == 0)
        {
            nSizeHint = static_cast<size_t>(Min<uint64>((std::numeric_limits<size_t>::max)(), DFG_MODULE_NS(os)::fileSize(sFilePath)));
            if (nSizeHint > 0)
            {
                nSizeHint++; // Add one byte so that readAllFromStream() completes the read in one step rather than reading all, allocating more and then reading zero bytes.
                nReadStepSize = nSizeHint; // To read the file in one step as we know the size beforehand.
            }
        }
        DFG_MODULE_NS(io)::DFG_CLASS_NAME(BasicIfStream) istrm(sFilePath);
        return readAllFromStream<Cont_T>(istrm, nReadStepSize, nSizeHint, nMaxSize);
    }

    template <class Cont_T>
    Cont_T fileToByteContainer(const DFG_CLASS_NAME(ReadOnlySzParamC) sFilePath,
        const size_t nReadStepSize = DFG_DETAIL_NS::gnDefaultFileToMemReadStep,
        const size_t nSizeHint = 0,
        const size_t nMaxSize = NumericTraits<size_t>::maxValue
        )
    {
        return fileToByteContainerImpl<Cont_T, char>(sFilePath, nReadStepSize, nSizeHint, nMaxSize);
    }

    template <class Cont_T>
    Cont_T fileToByteContainer(const DFG_CLASS_NAME(ReadOnlySzParamW) sFilePath,
        const size_t nReadStepSize = DFG_DETAIL_NS::gnDefaultFileToMemReadStep,
        const size_t nSizeHint = 0,
        const size_t nMaxSize = NumericTraits<size_t>::maxValue
        )
    {
        return fileToByteContainerImpl<Cont_T, wchar_t>(sFilePath, nReadStepSize, nSizeHint, nMaxSize);
    }


    template <class Char_T>
    std::vector<char> fileToVectorImpl(const DFG_CLASS_NAME(ReadOnlySzParam)<Char_T> sFilePath,
        const size_t nSizeHint = 0,
        const size_t nMaxSize = NumericTraits<size_t>::maxValue
        )
    {
        return fileToByteContainer<std::vector<char>>(sFilePath, DFG_DETAIL_NS::gnDefaultFileToMemReadStep, nSizeHint, nMaxSize);
    }

    inline std::vector<char> fileToVector(const DFG_CLASS_NAME(ReadOnlySzParamC) sFilePath, const size_t nSizeHint = 0, const size_t nMaxSize = NumericTraits<size_t>::maxValue)
    {
        return fileToVectorImpl<char>(sFilePath, nSizeHint, nMaxSize);
    }

    inline std::vector<char> fileToVector(const DFG_CLASS_NAME(ReadOnlySzParamW) sFilePath, const size_t nSizeHint = 0, const size_t nMaxSize = NumericTraits<size_t>::maxValue)
    {
        return fileToVectorImpl<wchar_t>(sFilePath, nSizeHint, nMaxSize);
    }

} } // module namespace

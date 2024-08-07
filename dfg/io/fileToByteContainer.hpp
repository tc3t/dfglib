#pragma once

#include "../dfgDefs.hpp"
#include <vector>
#include "../ReadOnlySzParam.hpp"
#include "BasicIfStream.hpp"
#include "../ptrToContiguousMemory.hpp"
#include "../os/fileSize.hpp"
#include "../os/memoryMappedFile.hpp"
#include "../rangeIterator.hpp"
#include "../stdcpp/stdversion.hpp"
#include "../cont/elementType.hpp"
#include "../Span.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(io) {

    namespace DFG_DETAIL_NS
    {
        // Value is not really based on anything -> likely not optimal. BUFSIZ might be an option.
        // On MSVC 2017 BUFSIZ seems to be 512, on Clang 6.0.0 and GCC 7.4 8192.
        const size_t gnDefaultFileToMemReadStep = 512;

        class ReadOnlyByteStorage
        {
        public:
            template <class T> using SpanT = RangeIterator_T<const T*>;
            using DirectStorage = std::vector<char>;

            size_t size() const { return (m_bUsingMmf) ? this->m_mmf.size() : m_helperStorage.size(); }
            bool empty() const { return (m_bUsingMmf) ? this->m_mmf.size() == 0 : m_helperStorage.empty(); };

            template <class T>
            SpanT<T> asSpan() const &
            {
                return asSpanImpl<T>();
            }

            // Deleting asSpan<T>() for rvalues to avoid dangling spans, i.e. spans that would point to storage of deleted ReadOnlyByteStorage.
            template <class T> SpanT<T> asSpan() const && = delete;

            // Explicit version that can be used where caller guarantees it's ok to get span even if 'this' is rvalue.
            template <class T> SpanT<T> asSpanFromRValue() const
            {
                return asSpanImpl<T>();
            }

            // Returns content as given container type using constructor that takes (const T* begin, const T* end) arguments, where T is element type of Cont_T
            template <class Cont_T>
            Cont_T asContainer() const
            {
                using T = typename ::DFG_MODULE_NS(cont)::ElementType<Cont_T>::type;
                auto span = asSpan<T>();
                return Cont_T(span.begin(), span.end());
            }

            bool isMemoryMapped() const { return m_bUsingMmf && m_mmf.is_open(); }

            // When isMemoryMapped() == false, returns rvalue byte container, i.e. allows caller to take ownership of byte storage.
            DirectStorage&& releaseStorage()
            {
                DFG_ASSERT_CORRECTNESS(m_bUsingMmf == false);
                return std::move(m_helperStorage);
            }

        private:
            const char* data() const { return (m_bUsingMmf) ? m_mmf.data() : m_helperStorage.data(); }

            template <class T>
            SpanT<T> asSpanImpl() const
            {
                DFG_STATIC_ASSERT(std::is_trivial<T>::value && sizeof(T) == 1, "asSpanImpl(): sizeof(T) must be one");
                const auto p = reinterpret_cast<const T*>(this->data());
                return SpanT<T>(p, p + this->size());
            }

        public:
            bool m_bUsingMmf = true;
            ::DFG_MODULE_NS(os)::MemoryMappedFile m_mmf;
            DirectStorage m_helperStorage;
        }; // ReadOnlyByteStorage
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
                            const size_t nSizeHintArg = 0,
                            const size_t nMaxSize = NumericTraits<size_t>::maxValue)
    {
        Cont_T cont;
        if (nMaxSize <= 0)
            return cont;
        const auto nSizeHint = Min(nSizeHintArg, nMaxSize);
        if (nSizeHint > 0)
            cont.reserve(nSizeHint);
        if (nReadStepSize == 0)
            nReadStepSize = DFG_DETAIL_NS::gnDefaultFileToMemReadStep;
        nReadStepSize = Min(nReadStepSize, nMaxSize);
        while (istrm.good() && cont.size() < nMaxSize)
        {
            const auto nOldSize = cont.size();
            cont.resize(nOldSize + nReadStepSize);
            using PtrT = decltype(ptrToContiguousMemory(cont));
            constexpr bool isGoodElemType = std::is_same<char*, PtrT>::value || std::is_same<signed char*, PtrT>::value || std::is_same<unsigned char*, PtrT>::value
#if defined(__cpp_lib_byte) && (__cpp_lib_byte >= 201603L)
                || std::is_same<std::byte*, PtrT>::value
#endif
                ;
            DFG_STATIC_ASSERT(isGoodElemType, "readAllFromStream: Destination container has unaccepted element type");
            const auto nRead = readBytes(istrm, reinterpret_cast<char*>(ptrToContiguousMemory(cont)) + nOldSize, cont.size() - nOldSize);
            cont.resize(nOldSize + nRead);
        }
        if (cont.size() > nMaxSize)
            cont.resize(nMaxSize);
        return cont;
    }


    // Reads file to contiguous container of bytes (e.g. std::vector<char>).
    template <class Cont_T, class Char_T>
    Cont_T fileToByteContainerImpl(const StringViewSz<Char_T> sFilePath,
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
                nReadStepSize = Min(nMaxSize, nSizeHint); // To read the file in one step as we know the size beforehand.
            }
        }
        DFG_MODULE_NS(io)::BasicIfStream istrm(sFilePath);
        return readAllFromStream<Cont_T>(istrm, nReadStepSize, nSizeHint, nMaxSize);
    }

    template <class Cont_T>
    Cont_T fileToByteContainer(const StringViewSzC sFilePath,
        const size_t nReadStepSize = DFG_DETAIL_NS::gnDefaultFileToMemReadStep,
        const size_t nSizeHint = 0,
        const size_t nMaxSize = NumericTraits<size_t>::maxValue
        )
    {
        return fileToByteContainerImpl<Cont_T, char>(sFilePath, nReadStepSize, nSizeHint, nMaxSize);
    }

    template <class Cont_T>
    Cont_T fileToByteContainer(const StringViewSzW sFilePath,
        const size_t nReadStepSize = DFG_DETAIL_NS::gnDefaultFileToMemReadStep,
        const size_t nSizeHint = 0,
        const size_t nMaxSize = NumericTraits<size_t>::maxValue
        )
    {
        return fileToByteContainerImpl<Cont_T, wchar_t>(sFilePath, nReadStepSize, nSizeHint, nMaxSize);
    }


    template <class Char_T>
    std::vector<char> fileToVectorImpl(const StringViewSz<Char_T> sFilePath,
        const size_t nSizeHint = 0,
        const size_t nMaxSize = NumericTraits<size_t>::maxValue
        )
    {
        return fileToByteContainer<std::vector<char>>(sFilePath, DFG_DETAIL_NS::gnDefaultFileToMemReadStep, nSizeHint, nMaxSize);
    }

    inline std::vector<char> fileToVector(const StringViewSzC sFilePath, const size_t nSizeHint = 0, const size_t nMaxSize = NumericTraits<size_t>::maxValue)
    {
        return fileToVectorImpl<char>(sFilePath, nSizeHint, nMaxSize);
    }

    inline std::vector<char> fileToVector(const StringViewSzW sFilePath, const size_t nSizeHint = 0, const size_t nMaxSize = NumericTraits<size_t>::maxValue)
    {
        return fileToVectorImpl<wchar_t>(sFilePath, nSizeHint, nMaxSize);
    }

    struct FileToMemoryFlags
    {
        enum Flag
        {
            none = 0,
            allowFileLocking = 0x1 // With this flag fileToMemory_readOnly() is allowed to lock file so that it can't be edited or removed while returned ReadOnlyByteStorage is alive.
        };

        FileToMemoryFlags(const uint32 mask = allowFileLocking)
            : m_mask(mask)
        {}

        operator uint32() const { return m_mask; }

        FileToMemoryFlags& removeFlag(Flag flag)
        {
            m_mask &= ~flag;
            return *this;
        }

        uint32 m_mask;
    }; // FileToMemoryFlags

    inline DFG_DETAIL_NS::ReadOnlyByteStorage fileToMemory_readOnly(const StringViewSzC &svFilePath, const FileToMemoryFlags flags = FileToMemoryFlags())
    {
        DFG_DETAIL_NS::ReadOnlyByteStorage storage;
        if (flags & FileToMemoryFlags::allowFileLocking)
        {
            try
            {
                storage.m_mmf.open(svFilePath);
                storage.m_bUsingMmf = true;
                return storage;
            }
            catch (...)
            {
            }
        }
        // Getting here means that opening memory mapped file failed or that memory mapping may not be used.
        try
        {
            storage.m_bUsingMmf = false;
            storage.m_helperStorage = fileToVector(svFilePath);
        }
        catch (...) // Catching exception in case that file doesn't fit to memory.
        {
        }
        return storage;
    }

} } // module namespace

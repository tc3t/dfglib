#pragma once

#include "dfgDefs.hpp"
#include <cstddef> // For size_t

#if defined(__cpp_lib_span)
    #include <span>
#else
    #include <type_traits>
#endif

DFG_ROOT_NS_BEGIN{

#if defined(__cpp_lib_span)
    template <class T, size_t Extent_T = std::dynamic_extent>
    class Span : public std::span<T, Extent_T>
    {
    public:
        using BaseClass = std::span<T, Extent_T>;
        using BaseClass::BaseClass;
        Span(BaseClass other)
            : BaseClass(other)
        {}
    }; // class Spans
#else // Case: not defined(__cpp_lib_span)
    // A barebones alternative when not having std::span available.
    template <class T>
    class Span
    {
    public:
        using element_type = T;
        using value_type = std::remove_cv_t<T>;
        using iterator   = element_type*;

        Span() = default;

        template <class Cont_T>
        Span(Cont_T& cont)
            : m_pData(cont.data())
            , m_nSize(cont.size())
        {}

        template <class Cont_T>
        Span(const Cont_T& cont)
            : m_pData(cont.data())
            , m_nSize(cont.size())
        {}

        template <class T_0, size_t N_T>
        Span(T_0(&arr)[N_T])
            : m_pData(arr)
            , m_nSize(N_T)
        {}

        constexpr T* begin() const noexcept { return data(); }
        constexpr T* end() const noexcept { return data() + size(); }

        constexpr T* data() const noexcept { return m_pData; }

        constexpr size_t size() const noexcept { return m_nSize; }

        constexpr bool empty() const noexcept { return (this->size() == 0); }

        T& operator[](const size_t offset) const
        {
            return *(m_pData + offset);
        }

        T* m_pData = nullptr;
        size_t m_nSize = 0;
    }; // class Span
#endif

} // namespace DFG_ROOT_NS

#pragma once

/*
Description:
    Generates a struct that can be used to test existence of member function.
Example usage:
    DFG_REFLECTION_GENERATE_HAS_MEMBER_FUNCTION_TESTER(pop_front)  // Generates struct Has_pop_front, which has static member 'value'.
    Has_pop_front<Cont_T>::value;                       // true if Cont_T has pop_front() member function, false otherwise.
Related code:
    -boost/tti/has_member_function.hpp
*/
#define DFG_REFLECTION_GENERATE_HAS_MEMBER_FUNCTION_TESTER(NAME) \
    template <class T> \
    struct Has_##NAME \
{ \
    typedef char FalseType; \
    typedef double TrueType; \
    template <class U> static auto func(decltype(&U::NAME) p)->TrueType; \
    template <class U> static auto func(...)->FalseType; \
\
\
    typedef decltype(func<T>(nullptr)) type; \
    enum \
    { \
        value = (sizeof(type) == sizeof(TrueType)) \
    }; \
};

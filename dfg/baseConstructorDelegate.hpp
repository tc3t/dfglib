#pragma once

// Helper macro that creates constructors that delegate parameters to member or base class
// Basic usage: DFG_CONSTRUCTOR_INITIALIZATION_DELEGATE_1(Class, m_member) {}
#define DFG_CONSTRUCTOR_INITIALIZATION_DELEGATE_1(THISCLASS, MEMBER) template <class T0> THISCLASS(T0&& t0) : MEMBER(std::forward<T0>(t0))
#define DFG_CONSTRUCTOR_INITIALIZATION_DELEGATE_2(THISCLASS, MEMBER) template <class T0, class T1> THISCLASS(T0&& t0, T1&& t1) : MEMBER(std::forward<T0>(t0), std::forward<T1>(t1))
#define DFG_CONSTRUCTOR_INITIALIZATION_DELEGATE_3(THISCLASS, MEMBER) template <class T0, class T1, class T2> THISCLASS(T0&& t0, T1&& t1, T2&& t2) : MEMBER(std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2))

// Helper macro for creating constructors that pass parameters to base class.
// Basic usage: DFG_BASE_CONSTRUCTOR_DELEGATE_1(Derived, Base) {}
#define DFG_BASE_CONSTRUCTOR_DELEGATE_1 DFG_CONSTRUCTOR_INITIALIZATION_DELEGATE_1
#define DFG_BASE_CONSTRUCTOR_DELEGATE_2 DFG_CONSTRUCTOR_INITIALIZATION_DELEGATE_2
#define DFG_BASE_CONSTRUCTOR_DELEGATE_3 DFG_CONSTRUCTOR_INITIALIZATION_DELEGATE_3

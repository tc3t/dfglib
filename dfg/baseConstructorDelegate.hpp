#pragma once

// Helper macro for creating constructors that pass parameters to base class.
// Basic usage: DFG_BASE_CONSTRUCTOR_DELEGATE_1(Derived, Base) {}
#define DFG_BASE_CONSTRUCTOR_DELEGATE_1(THISCLASS, BASECLASS) template <class T0> THISCLASS(T0&& t0) : BASECLASS(std::forward<T0>(t0))
#define DFG_BASE_CONSTRUCTOR_DELEGATE_2(THISCLASS, BASECLASS) template <class T0, class T1> THISCLASS(T0&& t0, T1&& t1) : BASECLASS(std::forward<T0>(t0), std::forward<T1>(t1))
#define DFG_BASE_CONSTRUCTOR_DELEGATE_3(THISCLASS, BASECLASS) template <class T0, class T1, class T2> THISCLASS(T0&& t0, T1&& t1, T2&& t2) : BASECLASS(std::forward<T0>(t0), std::forward<T1>(t1), std::forward<T2>(t2))

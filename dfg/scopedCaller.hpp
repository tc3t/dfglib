#pragma once

#include "dfgDefs.hpp"

DFG_ROOT_NS_BEGIN
{
	// ScopedCaller calls one function when created and another when destructed.
	// The purpose is to provide an easy and exception safe way to handle 'call pairs' such as
	// SetPriorityClass(GetCurrentProcess(), PROCESS_MODE_BACKGROUND_BEGIN);
	// SetPriorityClass(GetCurrentProcess(), PROCESS_MODE_BACKGROUND_END);
	// To create instance of ScopedCaller conveniently, use makeScopedCaller.
	// Example: auto scopedCaller = MakeScopedCaller([](){call1();}, [](){call2();});
	// Related code: BOOST_SCOPE_EXIT
    // TODO: Considered implementing empty base class optimization (http://en.cppreference.com/w/cpp/language/ebo)
	template <class ConstructorFunc_T, class DestructorFunc_T>
	class ScopedCaller
	{
	public:
		ScopedCaller(	ConstructorFunc_T constructorFunc,
						DestructorFunc_T destructorFunc,
						bool bActive = true) :
			m_constructorFunc(std::forward<ConstructorFunc_T>(constructorFunc)),
			m_destructorFunc(std::forward<DestructorFunc_T>(destructorFunc)),
			m_bActive(bActive)
		{
			if (m_bActive)
				m_constructorFunc();
		}

		ScopedCaller(ScopedCaller&& other) noexcept :
			m_constructorFunc(std::move(other.m_constructorFunc)),
			m_destructorFunc(std::move(other.m_destructorFunc)),
			m_bActive(true)
		{
            const bool wasOtherActive = other.m_bActive;
			other.m_bActive = false;
            if (!wasOtherActive)
			    m_constructorFunc();
		}

		~ScopedCaller()
		{
			if (m_bActive)
				m_destructorFunc();
		}

		DFG_HIDE_COPY_CONSTRUCTOR_AND_COPY_ASSIGNMENT(ScopedCaller)

	public:

		ConstructorFunc_T m_constructorFunc;
		DestructorFunc_T m_destructorFunc;
		bool m_bActive;
	};

	template <class ConstructorFunc_T, class DestructorFunc_T>
	inline ScopedCaller<ConstructorFunc_T, DestructorFunc_T> makeScopedCaller(ConstructorFunc_T constructorFunc, DestructorFunc_T destructorFunc)
	{
        return ScopedCaller<ConstructorFunc_T, DestructorFunc_T>(std::forward<ConstructorFunc_T>(constructorFunc), std::forward<DestructorFunc_T>(destructorFunc));
	}
}

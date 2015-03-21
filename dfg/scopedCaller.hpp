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
	// TODO: test
	template <class ConstructorFunc_T, class DestructorFunc_T>
	class DFG_CLASS_NAME(ScopedCaller)
	{
	public:
		DFG_CLASS_NAME(ScopedCaller)(	ConstructorFunc_T constructorFunc,
						DestructorFunc_T destructorFunc,
						bool bActive = true) :
			m_constructorFunc(constructorFunc),
			m_destructorFunc(destructorFunc),
			m_bActive(bActive)
		{
			if (m_bActive)
				m_constructorFunc();
		}

		DFG_CLASS_NAME(ScopedCaller)(DFG_CLASS_NAME(ScopedCaller)&& other) :
			m_constructorFunc(std::move(other.m_constructorFunc)),
			m_destructorFunc(std::move(other.m_destructorFunc)),
			m_bActive(true)
		{
			other.m_bActive = false;
			m_constructorFunc();
		}

		~DFG_CLASS_NAME(ScopedCaller)()
		{
			if (m_bActive)
				m_destructorFunc();
		}

		DFG_HIDE_COPY_CONSTRUCTOR_AND_COPY_ASSIGNMENT(DFG_CLASS_NAME(ScopedCaller))

	public:

		ConstructorFunc_T m_constructorFunc;
		DestructorFunc_T m_destructorFunc;
		bool m_bActive;
	};

	template <class ConstructorFunc_T, class DestructorFunc_T>
	inline DFG_CLASS_NAME(ScopedCaller)<ConstructorFunc_T, DestructorFunc_T> makeScopedCaller(ConstructorFunc_T constructorFunc, DestructorFunc_T destructorFunc)
	{
		return std::move(DFG_CLASS_NAME(ScopedCaller)<ConstructorFunc_T, DestructorFunc_T>(constructorFunc, destructorFunc, false));
	}
}

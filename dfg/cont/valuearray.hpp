#pragma once

#include "../dfgDefs.hpp"
#include "../baseConstructorDelegate.hpp"
#include <vector>
#include <valarray>
#include "../numeric/accumulate.hpp"
#include "../numeric/average.hpp"
#include "../numeric/median.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

	// Class for storing values in contiguous memory with customisable base class.
	template <class BaseCont_T>
	class DFG_CLASS_NAME(ValueArrayT) : public BaseCont_T
	{
	public:
		typedef BaseCont_T BaseClass;
		typedef typename BaseClass::value_type value_type;
		DFG_CLASS_NAME(ValueArrayT)() {}
		DFG_BASE_CONSTRUCTOR_DELEGATE_1(DFG_CLASS_NAME(ValueArrayT), BaseClass) {}
		DFG_BASE_CONSTRUCTOR_DELEGATE_2(DFG_CLASS_NAME(ValueArrayT), BaseClass) {}
		DFG_BASE_CONSTRUCTOR_DELEGATE_3(DFG_CLASS_NAME(ValueArrayT), BaseClass) {}

    private:
		template <class Cont_T> static  bool emptyImpl(const Cont_T& cont)              { return cont.empty(); }
		template <class T>      static  bool emptyImpl(const std::valarray<T>& cont)    { return cont.size() == 0; }

    public:
		bool empty() const  { return emptyImpl(static_cast<const BaseClass&>(*this)); }

		template <class Cont_T> static auto dataImpl(Cont_T& cont) -> decltype(cont.data()) { return cont.data(); }
		template <class T> static auto dataImpl(std::valarray<T>& cont) -> decltype(&cont[0]) { return (!emptyImpl(cont)) ? &cont[0] : nullptr; }

		value_type* data() { return dataImpl(static_cast<BaseClass&>(*this)); }
		const value_type* data() const { return dataImpl(static_cast<const BaseClass&>(*this)); }

		template <class Range_T>
		void assign(const Range_T& range)
		{
			const auto newSize = count(range);
			this->resize(newSize);
			std::copy(std::begin(range), std::end(range), data());
		}

        // operations
        value_type average() const; // TODO: revise return value type (especially for case value_type == integer).
        value_type median() const;
        value_type sum() const;
	};

    template <class BaseCont_T>
    auto DFG_CLASS_NAME(ValueArrayT)<BaseCont_T>::average() const -> value_type
    {
        return DFG_MODULE_NS(numeric)::average(*this);
    }

    template <class BaseCont_T>
    auto DFG_CLASS_NAME(ValueArrayT)<BaseCont_T>::median() const -> value_type
    {
        return DFG_MODULE_NS(numeric)::median(*this);
    }

    template <class BaseCont_T>
    auto DFG_CLASS_NAME(ValueArrayT)<BaseCont_T>::sum() const -> value_type
    {
        return DFG_MODULE_NS(numeric)::accumulate(*this, value_type(0));
    }
	
	template <class T>
	class DFG_CLASS_NAME(ValueArray) : public DFG_CLASS_NAME(ValueArrayT)<std::valarray<T>>
	{
	public:
		typedef DFG_CLASS_NAME(ValueArrayT)<std::valarray<T>> BaseClass;
		DFG_CLASS_NAME(ValueArray)() {}
		DFG_BASE_CONSTRUCTOR_DELEGATE_1(DFG_CLASS_NAME(ValueArray), BaseClass) {}
		DFG_BASE_CONSTRUCTOR_DELEGATE_2(DFG_CLASS_NAME(ValueArray), BaseClass) {}
		DFG_BASE_CONSTRUCTOR_DELEGATE_3(DFG_CLASS_NAME(ValueArray), BaseClass) {}
	};

	template <class T>
	class DFG_CLASS_NAME(ValueVector) : public DFG_CLASS_NAME(ValueArrayT)<std::vector<T>>
	{
	public:
		typedef DFG_CLASS_NAME(ValueArrayT)<std::vector<T>> BaseClass;
		DFG_CLASS_NAME(ValueVector)() {}
		DFG_BASE_CONSTRUCTOR_DELEGATE_1(DFG_CLASS_NAME(ValueVector), BaseClass) {}
		DFG_BASE_CONSTRUCTOR_DELEGATE_2(DFG_CLASS_NAME(ValueVector), BaseClass) {}
		DFG_BASE_CONSTRUCTOR_DELEGATE_3(DFG_CLASS_NAME(ValueVector), BaseClass) {}
	};

} } // module namespace

#pragma once

#include "../dfgDefs.hpp"
#include "pow.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(math) {

	// Polynomial evaluation using Horner's method (http://en.wikipedia.org/wiki/Horner_scheme)
	// TODO: verify and test the method.
	// coeff is collection of a_i values in formula a_0 * x^0 + a_1 * x^1 + ... + a_n * x^n
	// Related code:
	//		-GSL polynomial evaluation (http://www.gnu.org/software/gsl/manual/html_node/Polynomial-Evaluation.html#Polynomial-Evaluation)
	template <class T, class CoEffIterable_T>
	T evalPolynomial(const T x, const CoEffIterable_T& coeff)
	{
		const auto iterBegin = std::begin(coeff);
		auto iter = std::end(coeff);
		if (x == 0)
			return (iterBegin != iter) ? *iterBegin : T(0);

		if (iterBegin == iter)
			return 0;

		--iter;

		T bk = *iter; // == a_n
		for (; iter != iterBegin;)
		{
			--iter;
			T ak = *iter;
			bk = (bk != 0) ? ak + bk * x : ak; // Testing for bk != 0 is to avoid possible evaluation of 0*infinity leading to IND.
		}
		return bk;

		/*
		// Very basic implementation
		T sum = 0;
		size_t n = 0;
		for (; iter != iterEnd; ++iter, ++n)
		{
			if (*iter != 0)
				sum += *iter * powN(x, n);
		}
		return sum;
		*/
	}

} }

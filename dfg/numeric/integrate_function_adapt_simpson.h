// This file is modified version from dlib with original license text
    // Copyright (C) 2013 Steve Taylor (steve98654@gmail.com)
    // License: Boost Software License  See LICENSE.txt for full license
#pragma once

#include "../dfgDefs.hpp"
#include <cmath>
#include <boost/units/cmath.hpp>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(numeric)
{

    namespace DFG_DETAIL_NS
    {
        // Helper for obtaining value type from quantity.
        template <class T> struct ValueType { typedef typename T::value_type type; };
        template <> struct ValueType<float> { typedef float type; };
        template <> struct ValueType<double> { typedef double type; };
        template <> struct ValueType<long double> { typedef long double type; };

        // Helper for accessing plain value from quantity.
        template <class T> auto value(const T& quantity) -> decltype(quantity.value()) { return quantity.value(); }
        float value(const float& quantity) { return quantity; }
        double value(const double& quantity) { return quantity; }
        long double value(const long double& quantity) { return quantity; }
    }

template <typename Tx, typename Tf, typename funct>
auto impl_adapt_simp_stop(const funct& f, Tx a, Tx b, Tf fa, Tf fm, Tf fb, typename DFG_DETAIL_NS::ValueType<Tf>::type is, int cnt) -> decltype(fa * a)
{
    using namespace DFG_DETAIL_NS;
    const int maxint = 500;

    auto m   = (a + b)/2.0;
    auto h   = (b - a)/4.0;
    auto fml = f(a + h);
    auto fmr = f(b - h);
    auto i1 = h/1.5*(fa+4.0*fm+fb);
    auto i2 = h/3.0*(fa+4.0*(fml+fmr)+2.0*fm+fb);
    i1 = (16.0*i2 - i1)/15.0;
    decltype(i1) Q = 0;

    if ((std::abs(value(i1-i2)) <= std::abs(value(is))) || (m <= a) || (b <= m))
    {
        Q = i1;
    }
    else
    {
        if(cnt < maxint)
        {
            cnt = cnt + 1;

            Q = impl_adapt_simp_stop(f,a,m,fa,fml,fm,is,cnt)
                + impl_adapt_simp_stop(f,m,b,fm,fmr,fb,is,cnt);
        }
    }

    return Q;
}

// ----------------------------------------------------------------------------------------

/*!
requires
- TODO
ensures
- returns an approximation of the integral of f over the domain [a,b] using the
adaptive Simpson method outlined in Gander, W. and W. Gautshi, "Adaptive
Quadrature -- Revisited" BIT, Vol. 40, (2000), pp.84-101
- tol is a tolerance parameter that determines the overall accuracy of
approximated integral.  We suggest a default value of 1e-10 for tol. If < 0, epsilon() will be used.
!*/
template <typename T, typename funct>
auto integrate_function_adapt_simp(
    const funct& f,
    T a,
    T b,
    typename DFG_DETAIL_NS::ValueType<T>::type tol = 1e-10
) -> decltype(a * f(a))
{
    using namespace DFG_DETAIL_NS;
    typedef decltype(a * f(a)) ResultType;
    typedef typename ValueType<ResultType>::type value_type;

    if (b < a) // Original implementation required a < b. Didn't check whether it actually requires that, simlpy follow that.
        return value_type(-1) * integrate_function_adapt_simp(f, b, a, tol);
    else if (a == b)
        return ResultType(0); // "The only literal value that ought to be converted to a quantity by generic code is zero" (http://www.boost.org/doc/libs/1_55_0/doc/html/boost_units/FAQ.html#boost_units.FAQ.NoConstructorFromValueType) 

    auto eps = std::numeric_limits<value_type>::epsilon();
    if(tol < eps)
    {
        tol = eps;
    }

    const auto ba = b-a;
    const auto fa = f(a);
    const auto fb = f(b);
    const auto fm = f((a+b)/2.0);

    auto is = value(ba/8.0*(fa+fb+fm+ f(a + 0.9501*ba) + f(a + 0.2311*ba) + f(a + 0.6068*ba)
        + f(a + 0.4860*ba) + f(a + 0.8913*ba)));

    if(value(is) == 0)
    {
        is = value(b-a);
    }

    is = is*tol;

    int cnt = 0;

    return impl_adapt_simp_stop(f, a, b, fa, fm, fb, is, cnt);
}

}} // module namespace

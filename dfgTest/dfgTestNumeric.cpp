#include <stdafx.h>
#include <dfg/numericAll.hpp>
#include <dfg/math/pow.hpp>
#include <dfg/math/constants.hpp>
#include <dfg/math.hpp>
#include <dfg/rand.hpp>
#include <dfg/ptrToContiguousMemory.hpp>
#include <dfg/cont/arrayWrapper.hpp>
#include <dfg/time/timerCpu.hpp>
#include <dfg/alg.hpp>
#include <dfg/cont.hpp>
#include <dfg/iter/szIterator.hpp>
#include <numeric>
#include <list>

#include <boost/units/io.hpp>
#include <boost/units/quantity.hpp>
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/time.hpp>
#include <boost/units/systems/si/velocity.hpp>

#include <array>


TEST(dfgNumeric, NumericIntegrationAdaptSimpson)
{
    // Code modified from dlib example integrate_function_adapt_simp_ex.cpp
    using namespace DFG_MODULE_NS(numeric);

    const auto gg1 = [](double x) {return std::exp(x);};
    const auto gg2 = [](double x) {return x*x;};
    const auto gg3 = [](double x) {return 1/(x*x + DFG_MODULE_NS(math)::pow2(std::cos(x)));};
    const auto gg4 = [](double x) {return std::sin(x);};
    const auto gg5 = [](double x) {return 1/(1 + x*x);};

    const double tol = 1e-10;
    const double m1 = integrate_function_adapt_simp(gg1, 0.0, 1.0, tol);
    const double m1Inverse = integrate_function_adapt_simp(gg1, 1.0, 0.0, tol);
    const double m2 = integrate_function_adapt_simp(gg2, 0.0, 1.0, tol);
    const double m2Inverse = integrate_function_adapt_simp(gg2, 1.0, 0.0, tol);
    const double m3 = integrate_function_adapt_simp(gg3, 0.0, DFG_MODULE_NS(math)::const_pi, tol);
    const double m3Inverse = integrate_function_adapt_simp(gg3, DFG_MODULE_NS(math)::const_pi, 0.0, tol);
    const double m4 = integrate_function_adapt_simp(gg4, -DFG_MODULE_NS(math)::const_pi, DFG_MODULE_NS(math)::const_pi, tol);
    const double m4Inverse = integrate_function_adapt_simp(gg4, DFG_MODULE_NS(math)::const_pi, -DFG_MODULE_NS(math)::const_pi, tol);
    const double m5 = integrate_function_adapt_simp(gg5, 0.0, 2.0, tol);
    const double m5Inverse = integrate_function_adapt_simp(gg5, 2.0, 0.0, tol);

    EXPECT_NEAR(m1, DFG_MODULE_NS(math)::const_e - 1.0, 1e-13);
    EXPECT_NEAR(m2, 1.0/3.0, 1e-15);
    EXPECT_NEAR(m3, 1.581187971, 1e-8); // TODO: Check this, the expected value is from the calculation itself.
    EXPECT_NEAR(m4, 0.0, 1e-15);
    EXPECT_NEAR(m5, std::atan(2.0) - std::atan(0.0), 1e-12);

    EXPECT_EQ(m1, -1 * m1Inverse);
    EXPECT_EQ(m2, -1 * m2Inverse);
    EXPECT_EQ(m3, -1 * m3Inverse);
    EXPECT_EQ(m4, -1 * m4Inverse);
    EXPECT_EQ(m5, -1 * m5Inverse);

    EXPECT_EQ(integrate_function_adapt_simp(gg1, 100.1245, 100.1245, tol), 0.0);
}

TEST(DfgFunc, NumericIntegrationWithUnits)
{
    using namespace DFG_MODULE_NS(numeric);

    using boost::units::si::meters;
    using boost::units::si::seconds;
    using boost::units::si::meters_per_second;

    const auto velocityFunc0 = [](decltype(1.0*seconds)) {return 1.0 * meters_per_second;};
    const auto velocityFunc1 = [](decltype(1.0*seconds) t) {return t.value() * meters_per_second;};
    const auto velocityFunc2 = [](decltype(1.0*seconds) t) {return DFG_MODULE_NS(math)::pow2(t.value()) * meters_per_second;};

    const auto beginTime = 0.0 * seconds;
    const auto endTime = 10.0 * seconds;
    const double tol = 1e-10;

    auto distance0 = integrate_function_adapt_simp(velocityFunc0, beginTime, endTime, tol);
    auto distance0Inverse = integrate_function_adapt_simp(velocityFunc0, endTime, beginTime, tol);
    DFG_STATIC_ASSERT((std::is_same<decltype(distance0), decltype(1.0 * meters)>::value), "Integration returns wrong type");
    EXPECT_NEAR(distance0.value(), 10.0, 1e-15);
    EXPECT_EQ(distance0.value(), -1*distance0Inverse.value());
    
    auto distance1 = integrate_function_adapt_simp(velocityFunc1, beginTime, endTime, tol);
    DFG_STATIC_ASSERT((std::is_same<decltype(distance1), decltype(1.0 * meters)>::value), "Integration returns wrong type");
    EXPECT_NEAR(distance1.value(), 50.0, 1e-15);
    
    auto distance2 = integrate_function_adapt_simp(velocityFunc2, beginTime, endTime, tol);
    DFG_STATIC_ASSERT((std::is_same<decltype(distance2), decltype(1.0 * meters)>::value), "Integration returns wrong type");
    EXPECT_NEAR(distance2.value(), 1000.0 / 3, 1e-12);

    auto zeroIntegral = integrate_function_adapt_simp(velocityFunc0, beginTime, beginTime, tol);
    DFG_STATIC_ASSERT((std::is_same<decltype(zeroIntegral), decltype(1.0 * meters)>::value), "Integration returns wrong type");
    EXPECT_EQ(zeroIntegral.value(), 0.0);

    /*
    std::cout << distance0 << '\n';
    std::cout << distance1 << '\n';
    std::cout << distance2 << '\n';
    */
}

TEST(dfgNumeric, average)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);
    using namespace DFG_MODULE_NS(numeric);

    double vals[5] = { 1, 2, 3, 4, 5 };
    EXPECT_EQ(average(vals), 3);

    std::vector<double> vecVals(std::begin(vals), std::end(vals));
    EXPECT_EQ(average(vecVals), 3);

    auto pData = ptrToContiguousMemory(vecVals);
    EXPECT_EQ(average(makeRange(pData, pData + vecVals.size())), 3);
    EXPECT_EQ(average(DFG_CLASS_NAME(ArrayWrapper)::createArrayWrapper(pData, vecVals.size())), 3);

    std::array<float, 3> arrFloats = {1, 2, 3};
    EXPECT_EQ(average(arrFloats), 2);

    std::complex<float> arrComplexF[3] = { std::complex<float>(1, 2), std::complex<float>(2, 3), std::complex<float>(3, 4) };
    EXPECT_EQ((averageTyped<decltype(arrComplexF), std::complex<float>, float>(arrComplexF)), (std::complex<float>(2, 3)));

    std::complex<double> arrComplexD[3] = { std::complex<double>(1, 2), std::complex<double>(2, 3), std::complex<double>(3, 4) };
    EXPECT_EQ(average(arrComplexD), std::complex<double>(2, 3));

    EXPECT_EQ(-1, average(-0.5, -1.5));
    EXPECT_EQ(3, average(2.0, 3.0, 4.0));
    EXPECT_EQ(2.5, average(1, 2, 3, 4));
    EXPECT_EQ(3, average(1, 2, 3, 4, 5));

    // Should fail to compile as average does not support integers.
    //EXPECT_EQ(average(DFG_ROOT_NS::dfcont::makeVector<int>(1,2)), 3);
}

TEST(dfgNumeric, median)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(cont);
    using namespace DFG_MODULE_NS(math);
    using namespace DFG_MODULE_NS(numeric);

    const std::vector<float> emptyVector;
    const std::array<long double, 1> vals1 = { 100 };
    const std::array<double, 2> vals2 = { 1, 9 };
    const double vals[5] = { 8, 1, 9, 2, 5 };
    double vals6[6] = { 8, 1, 9, 2, 5, 7 };
    EXPECT_TRUE(isNan(median(emptyVector)));
    EXPECT_EQ(100, median(vals1));
    EXPECT_EQ(median(vals2), 5);
    EXPECT_EQ(median(vals), 5);
    EXPECT_EQ(median(vals6), 6);
    EXPECT_EQ(medianModifying(vals6), 6);
    EXPECT_EQ(7, vals6[3]); // This is implementation specific test case: should be true when using nth_element-implementation.

    std::sort(std::begin(vals6), std::end(vals6));
    const double vals7[7] = { 4, 2, 2, 5, 7, 6, 9 };
    EXPECT_EQ(6, medianInSorted(vals6));
    EXPECT_EQ(5, medianInNthSorted(vals7));

    {
        auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();
        const size_t nSize = 1001;
        std::vector<double> vals(nSize);
        for (size_t i = 0; i < nSize; ++i)
        {
            vals[i] = DFG_MODULE_NS(rand)::rand(randEng, -200.0, 50.0);
        }
        const auto m = median(vals);
        const auto mMod = medianModifying(vals);
        std::sort(vals.begin(), vals.end());
        const auto expected = vals[nSize / 2];
        EXPECT_EQ(expected, m);
        EXPECT_EQ(expected, mMod);
        EXPECT_EQ(expected, medianInSorted(vals));
    }
}

namespace
{
    template <class Cont_T>
    void vectorizationTestsTemplate(Cont_T& cont)
    {
        using namespace DFG_ROOT_NS;

        auto p = ptrToContiguousMemory(cont);
        const auto nSize = count(cont);

        /*
        rawStaticSizedArray
            VC2012_32: yes
            VC2012_64: 
            VC2013_32: yes
            VC2013_64:
            VC2015_32: yes
            VC2015_64: yes
        std::array<double, 4/20>
            VC2012_32: no, reason 1304 "Loop includes assignments that are of different sizes"
            VC2012_64: no, reason 1200 "Loop contains loop-carried data dependences that prevent vectorization..."
            VC2013_32: no, reason 1304 "Loop includes assignments that are of different sizes"
            VC2013_64: no, reason 1200 "Loop contains loop-carried data dependences that prevent vectorization..."
            VC2015_32: yes
            VC2015_64: yes
        std::vector<double>
            VC2012_32: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2012_64: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2013_32: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2013_64: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2015_32: TODO
            VC2015_64: TODO
        ArrayWrapper from std::vector
            VC2012_32: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2012_64: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2013_32: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2013_64: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2015_32: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2015_64: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
        */
        {
            for (size_t i = 0; i < count(cont); ++i) // cont.size() vs count(cont) doesn't make a difference.
            {
                cont[i] += 1;
            }
        }

        /*
        rawStaticSizedArray
            VC2012_32: yes
            VC2012_64:
            VC2013_32: yes
            VC2013_64:
            VC2015_32: yes
            VC2015_64: yes
        std::array<double, 4/20>
            VC2012_32: no, reason 1304 "Loop includes assignments that are of different sizes"
            VC2012_64: no, reason 1200 "Loop contains loop-carried data dependences that prevent vectorization..."
            VC2013_32: no, reason 1304 "Loop includes assignments that are of different sizes"
            VC2013_64: no, reason 1200 "Loop contains loop-carried data dependences that prevent vectorization..."
            VC2015_32: yes
            VC2015_64: yes
        std::vector<double>
            VC2012_32: no, reason 1304 "Loop includes assignments that are of different sizes"
            VC2012_64: no, reason 1200 "Loop contains loop-carried data dependences that prevent vectorization..."
            VC2013_32: no, reason 1304 "Loop includes assignments that are of different sizes"
            VC2013_64: no, reason 1200 "Loop contains loop-carried data dependences that prevent vectorization..."
            VC2015_32: yes
            VC2015_64: yes
        ArrayWrapper from std::vector
            VC2012_32: no, reason 1304 "Loop includes assignments that are of different sizes"
            VC2012_64: no, reason 1200 "Loop contains loop-carried data dependences that prevent vectorization..."
            VC2013_32: no, reason 1304 "Loop includes assignments that are of different sizes"
            VC2013_64: no, reason 1200 "Loop contains loop-carried data dependences that prevent vectorization..."
            VC2015_32: no, reason 1200 "Loop contains loop-carried data dependences that prevent vectorization..."
            VC2015_64: no, reason 1200 "Loop contains loop-carried data dependences that prevent vectorization..."
        */
        {
            for (size_t i = 0, nSize = count(cont); i < nSize; ++i) // cont.size() vs count(cont) doesn't make a difference.
            {
                cont[i] += 1;
            }
        }

        /*
        rawStaticSizedArray
            VC2012_32: yes
            VC2012_64:
            VC2013_32: yes
            VC2013_64:
            VC2015_32: yes
            VC2015_64: yes
        std::array<double, 4/20>
            VC2012_32: yes
            VC2012_64: yes
            VC2013_32: yes
            VC2013_64: yes
            VC2015_32: yes
            VC2015_64: yes
        std::vector<double>
            VC2012_32: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2012_64: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2013_32: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2013_64: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2015_32: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2015_64: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
        ArrayWrapper from std::vector
            VC2012_32: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2012_64: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2013_32: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2013_64: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2015_32: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2015_64: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
        */
        {
            for (size_t i = 0; i < count(cont); ++i) // cont.size() or count(cont) makes no difference.
            {
                p[i] += 1;
            }
        }

        /*
        rawStaticSizedArray
            VC2012_32: yes
            VC2012_64:
            VC2013_32: yes
            VC2013_64:
            VC2015_32: yes
            VC2015_64: yes
        std::array<double, 4/20>
            VC2012_32: yes
            VC2012_64: yes
            VC2013_32: yes
            VC2013_64: yes
            VC2015_32: yes
            VC2015_64: yes
        std::vector<double>
            VC2012_32: yes
            VC2012_64: yes
            VC2013_32: yes
            VC2013_64: yes
            VC2015_32: yes
            VC2015_64: yes
        ArrayWrapper from std::vector
            VC2012_32: yes
            VC2012_64: yes
            VC2013_32: yes
            VC2013_64: yes
            VC2015_32: yes
            VC2015_64: yes
        */
        {
            for (size_t i = 0; i < nSize; ++i)
            {
                p[i] += 1;
            }
        }

        /*
        rawStaticSizedArray
            VC2012_32: yes
            VC2012_64:
            VC2013_32: yes
            VC2013_64:
            VC2015_32: yes
            VC2015_64: yes
        std::array<double, 4/20>
            VC2012_32: yes
            VC2012_64: yes
            VC2013_32: yes
            VC2013_64: yes
            VC2015_32: yes
            VC2015_64: yes
        std::vector<double>
            VC2012_32: yes
            VC2012_64: yes
            VC2013_32: yes
            VC2013_64: yes
            VC2015_32: yes
            VC2015_64: yes
        ArrayWrapper from std::vector
            VC2012_32: yes
            VC2012_64: yes
            VC2013_32: yes
            VC2013_64: yes
            VC2015_32: yes
            VC2015_64: yes
        */
        {
            for (size_t i = 0, nLength = count(cont); i < nLength; ++i) // cont.size() vs count(cont) doesn't make a difference.
            {
                p[i] += 1;
            }
        }

        /*
        rawStaticSizedArray
            VC2012_32: no, reason 1301 "Loop stride is not +1"
            VC2012_64:
            VC2013_32: yes
            VC2013_64:
            VC2015_32: yes
            VC2015_64: yes
        std::array<double, 4/20>
            VC2012_32: no, reason 502 "Induction variable is stepped in some manner other than a simple +1"
            VC2012_64: no, reason 502 "Induction variable is stepped in some manner other than a simple +1"
            VC2013_32: no, reason 1304 "Loop includes assignments that are of different sizes"
            VC2013_64: no, reason 1301 "Loop stride is not +1"
            VC2015_32: yes
            VC2015_64: yes
        std::vector<double>
            VC2012_32: no, reason 502 "Induction variable is stepped in some manner other than a simple +1"
            VC2012_64: no, reason 502 "Induction variable is stepped in some manner other than a simple +1"
            VC2013_32: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2013_64: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2015_32: yes
            VC2015_64: yes
        ArrayWrapper from std::vector
            VC2013_32: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2012_64: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2013_32: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2013_64: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2015_32: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
            VC2015_64: no, reason 501 "Induction variable is not local; or upper bound is not loop-invariant."
        */
        {
            for (auto iter = std::begin(cont); iter != std::end(cont); ++iter)
            {
                *iter += 1;
            }
        }

        /*
        rawStaticSizedArray
            VC2012_32: no, reason 1301 "Loop stride is not +1"
            VC2012_64:
            VC2013_32: yes
            VC2013_64:
            VC2015_32: yes
            VC2015_64: yes
        std::array<double, 4/20>
            VC2012_32: no, reason 502 "Induction variable is stepped in some manner other than a simple +1"
            VC2012_64: no, reason 502 "Induction variable is stepped in some manner other than a simple +1"
            VC2013_32: no, reason 1304 "Loop includes assignments that are of different sizes"
            VC2013_64: no, reason 1301 "Loop stride is not +1"
            VC2015_32: yes
            VC2015_64: yes
        std::vector<double>
            VC2012_32: no, reason 502 "Induction variable is stepped in some manner other than a simple +1"
            VC2012_64: no, reason 502 "Induction variable is stepped in some manner other than a simple +1"
            VC2013_32: no, reason 1304 "Loop includes assignments that are of different sizes"
            VC2013_64: no, reason 1301 "Loop stride is not +1"
            VC2015_32: yes
            VC2015_64: yes
        ArrayWrapper from std::vector
            VC2012_32: no, reason 1301 "Loop stride is not +1"
            VC2012_64: no, reason 1301 "Loop stride is not +1"
            VC2013_32: no, reason 1200 "Loop contains loop-carried data dependences that prevent vectorization..."
            VC2013_64: no, reason 1200 "Loop contains loop-carried data dependences that prevent vectorization..."
            VC2015_32: yes
            VC2015_64: yes
        */
        {
            for (auto iter = std::begin(cont), iterEnd = std::end(cont); iter != iterEnd; ++iter)
            {
                *iter += 1;
            }
        }

        /*
        rawStaticSizedArray
            VC2012_32: no, reason 1301 "Loop stride is not +1"
            VC2012_64: no, reason 1301 "Loop stride is not +1"
            VC2013_32: yes?
            VC2013_64: yes?
            VC2015_32: yes?
            VC2015_64: yes?
        std::array<double, 4/20>
            VC2012_32: no, reason 1301 "Loop stride is not +1"
            VC2012_64: no, reason 1301 "Loop stride is not +1"
            VC2013_32: yes?
            VC2013_64: yes?
            VC2015_32: TODO
            VC2015_64: yes?
        std::vector<double>
            VC2012_32: no
            VC2012_64: no, reason 1301 "Loop stride is not +1"
            VC2013_32: no, reason 1200 "Loop contains loop-carried data dependences that prevent vectorization..."
            VC2013_64: no?
            VC2015_32: TODO
            VC2015_64: yes?
        ArrayWrapper from std::vector
            VC2012_32: no, reason 1301 "Loop stride is not +1"
            VC2012_64: no, reason 1301 "Loop stride is not +1"
            VC2013_32: yes?
            VC2013_64: no? (r1200?)
            VC2015_32: yes?
            VC2015_64: yes?
        */
        std::for_each(std::begin(cont), std::end(cont), [](double& val) {val += 1; });

        /*
        rawStaticSizedArray
        VC2012_32: TODO
        VC2012_64: TODO
        VC2013_32: TODO
        VC2013_64: TODO
        VC2015_32: yes?
        VC2015_64: yes?
        std::array<double, 4/20>
        VC2012_32: TODO
        VC2012_64: TODO
        VC2013_32: TODO
        VC2013_64: TODO
        VC2015_32: yes?
        VC2015_64: yes?
        std::vector<double>
        VC2012_32: TODO
        VC2012_64: TODO
        VC2013_32: TODO
        VC2013_64: TODO
        VC2015_32: yes?
        VC2015_64: yes?
        ArrayWrapper from std::vector
        VC2012_32: TODO
        VC2012_64: TODO
        VC2013_32: TODO
        VC2013_64: TODO
        VC2015_32: yes?
        VC2015_64: yes?
        */
        DFG_MODULE_NS(alg)::forEachFwd(makeRange(cont), [](double& val) {val += 1; });

        /*
        rawStaticSizedArray
            VC2012_32: no, reason 1301 "Loop stride is not +1"
            VC2012_64: no, reason 1301 "Loop stride is not +1"
            VC2013_32: yes?
            VC2013_64: yes?
            VC2015_32: yes?
            VC2015_64: yes?
        std::array<double, 4/20>
            VC2012_32: no, reason 1301 "Loop stride is not +1"
            VC2012_64: no, reason 1301 "Loop stride is not +1"
            VC2013_32: yes?
            VC2013_64: yes?
            VC2015_32: yes?
            VC2015_64: yes?
        std::vector<double>
            VC2012_32: no, reason 1301 "Loop stride is not +1"
            VC2012_64: no, reason 1301 "Loop stride is not +1"
            VC2013_32: yes?
            VC2013_64: yes?
            VC2015_32: yes?
            VC2015_64: yes?
        ArrayWrapper from std::vector
            VC2012_32: no, reason 1301 "Loop stride is not +1"
            VC2012_64: no, reason 1301 "Loop stride is not +1"
            VC2013_32: yes?
            VC2013_64: yes?
            VC2015_32: yes?
            VC2015_64: yes?
        */
        std::transform(std::begin(cont), std::end(cont), std::begin(cont), [](double val) {return val + 1; });
    }
}

TEST(dfgNumeric, autoVectorization)
{
#if 0
    using namespace DFG_MODULE_NS(rand);
    double rawStaticSizedArray[20];
    std::fill(std::begin(rawStaticSizedArray), std::end(rawStaticSizedArray), DFG_MODULE_NS(rand)::rand());
    std::array<double, 4> arrd4;
    std::array<double, 20> arrd20;
    std::vector<double> vec(static_cast<size_t>(DFG_MODULE_NS(rand)::rand() * 30));
    vectorizationTestsTemplate(rawStaticSizedArray);
    vectorizationTestsTemplate(arrd4);
    vectorizationTestsTemplate(arrd20);
    vectorizationTestsTemplate(vec);
    auto arrayWrapper = DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ArrayWrapper)::createArrayWrapper(vec);
    vectorizationTestsTemplate(arrayWrapper);
#endif
}

TEST(dfgNumeric, basicAlgs)
{
#if 1
    /*
    Vectorization results:
        VC2012_32: OK (all vectorized)
        VC2012_64: OK (all vectorized)
        VC2013_32: OK (some adds were not vectorized with reason 1300 ("Loop body contains no—or very little—computation")).
        VC2013_64: OK (some adds were not vectorized with reason 1300 ("Loop body contains no—or very little—computation")).
        VC2015_32: TODO
        VC2015_64: TODO
    */


    using namespace DFG_MODULE_NS(numeric);

    auto isDoubleEq = [](double a, double b)
                    {
                        return std::abs(a - b) < 1e-12;
                    };
    auto isDoubleEqAdd = [&](double a, double b, double diff) {return isDoubleEq(a, b + diff); };
    auto isDoubleEqMul = [&](double a, double b, double mul) {return isDoubleEq(a, b * mul); };
    auto isDoubleEqSub = [&](double a, double b, double sub) {return isDoubleEq(a, b - sub); };
    auto isDoubleEqDiv = [&](double a, double b, double div) {return isDoubleEq(a, b / div); };

    const std::array<double, 12> arr = { -6, 7, 6.5, 8.4, 12, -44, 6, 8, 7, 3, 7.5, -9 };
    std::vector<double> vec(arr.begin(), arr.end());

    // forEachAdd
    {
        forEachAdd(vec, 0);
        EXPECT_TRUE(std::equal(vec.begin(), vec.end(), arr.begin(), std::equal_to<double>()));
        forEachAdd(vec, 4);
        EXPECT_TRUE(std::equal(vec.begin(), vec.end(), arr.begin(), [&](double a, double b){return isDoubleEqAdd(a, b, 4); }));
        forEachAdd(vec.data(), vec.size(), 3);
        EXPECT_TRUE(std::equal(vec.begin(), vec.end(), arr.begin(), [&](double a, double b){return isDoubleEqAdd(a, b, 7); }));
    }

    vec.assign(arr.begin(), arr.end());
    
    // forEachMultiply
    {
        forEachMultiply(vec, 1);
        EXPECT_TRUE(std::equal(vec.begin(), vec.end(), arr.begin(), std::equal_to<double>()));
        forEachMultiply(vec, 4);
        EXPECT_TRUE(std::equal(vec.begin(), vec.end(), arr.begin(), [&](double a, double b){return isDoubleEqMul(a, b, 4); }));
        forEachMultiply(vec.data(), vec.size(), -5.5);
        EXPECT_TRUE(std::equal(vec.begin(), vec.end(), arr.begin(), [&](double a, double b){return isDoubleEqMul(a, b, 4 * -5.5); }));
    }

    vec.assign(arr.begin(), arr.end());

    // forEachSubtract
    {
        forEachSubtract(vec, 0);
        EXPECT_TRUE(std::equal(vec.begin(), vec.end(), arr.begin(), std::equal_to<double>()));
        forEachSubtract(vec, 4.0);
        EXPECT_TRUE(std::equal(vec.begin(), vec.end(), arr.begin(), [&](double a, double b) {return isDoubleEqSub(a, b, 4); }));
        forEachSubtract(vec.data(), vec.size(), -5.5);
        EXPECT_TRUE(std::equal(vec.begin(), vec.end(), arr.begin(), [&](double a, double b) {return isDoubleEqSub(a, b, 4 - 5.5); }));
    }

    vec.assign(arr.begin(), arr.end());

    // forEachDivide
    {
        forEachDivide(vec, 1);
        EXPECT_TRUE(std::equal(vec.begin(), vec.end(), arr.begin(), std::equal_to<double>()));
        forEachDivide(vec, 4.0);
        EXPECT_TRUE(std::equal(vec.begin(), vec.end(), arr.begin(), [&](double a, double b) {return isDoubleEqDiv(a, b, 4); }));
        forEachDivide(vec.data(), vec.size(), 0.5);
        EXPECT_TRUE(std::equal(vec.begin(), vec.end(), arr.begin(), [&](double a, double b) {return isDoubleEqDiv(a, b, 2); }));
    }

    vec.assign(arr.begin(), arr.end());

    //transformAdd
    {
        transformAdd(vec, vec, vec.data());
        EXPECT_TRUE(std::equal(vec.begin(), vec.end(), arr.begin(), [&](double a, double b)
        {
            return isDoubleEq(a, b + b);
        }));

        transformAdd(arr, vec, vec.data());
        EXPECT_TRUE(std::equal(vec.begin(), vec.end(), arr.begin(), [&](double a, double b)
        {
            return isDoubleEq(a, b + b + b);
        }));

        transformAdd(arr.data(), arr.size(), vec.data(), vec.size(), vec.data());
        EXPECT_TRUE(std::equal(vec.begin(), vec.end(), arr.begin(), [&](double a, double b)
        {
            return isDoubleEq(a, b + b + b + b);
        }));

        const int intVals[] = { 4, 5, 6 };
        const float floatVals[] = { 7, 5, 9 };
        double vals[3];
        transformAdd(intVals, floatVals, vals);
        EXPECT_EQ(4 + 7, vals[0]);
        EXPECT_EQ(5 + 5, vals[1]);
        EXPECT_EQ(6 + 9, vals[2]);
    }

    vec.assign(arr.begin(), arr.end());

    //transformMultiply
    {
        transformMultiply(vec, vec, vec.data());
        EXPECT_TRUE(std::equal(vec.begin(), vec.end(), arr.begin(), [&](double a, double b)
            {
                return isDoubleEq(a, b * b);
            }));

        transformMultiply(arr, vec, vec.data());
        EXPECT_TRUE(std::equal(vec.begin(), vec.end(), arr.begin(), [&](double a, double b)
        {
            return isDoubleEq(a, b * b * b);
        }));

        transformMultiply(arr.data(), arr.size(), vec.data(), vec.size(), vec.data());
        EXPECT_TRUE(std::equal(vec.begin(), vec.end(), arr.begin(), [&](double a, double b)
        {
            return isDoubleEq(a, b * b * b * b);
        }));

        const int intVals[] = { 4, 5, 6 };
        const float floatVals[] = { 7, 5, 9 };
        double vals[3];
        transformMultiply(intVals, floatVals, vals);
        EXPECT_EQ(4 * 7, vals[0]);
        EXPECT_EQ(5 * 5, vals[1]);
        EXPECT_EQ(6 * 9, vals[2]);
    }
#endif

#if ENABLE_RUNTIME_COMPARISONS
    { // Performance comparison for forEachMultiply and std::transform
        using namespace DFG_MODULE_NS(numeric);
        const size_t nVecSize = 256;
        std::vector<double> v1(nVecSize);
        std::vector<double> v2(nVecSize);
        for (size_t i = 0; i < nVecSize; ++i) // Using std::generate generated weird errors in VC2010 for lambdas.
        {
            v1[i] = DFG_MODULE_NS(rand)::rand();
            v2[i] = DFG_MODULE_NS(rand)::rand();
        }

#ifdef _DEBUG
        const size_t nLoopCount = 100000;
#else
        const size_t nLoopCount = 1000000;
#endif

        std::vector<double> v1FromAlg, v1FromTransform;

        {
            auto v1Temp = v1;
            DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timer;
            // Note: VC2012 and VC2013 seems optimize the loop so that forEachMultiply is actually called only once.
            //       Run times in VC2012 and VC2013 are ~100 ms, while in VC2010 and MinGW ~20 s.
            for (size_t i = 0; i < nLoopCount; ++i)
                forEachMultiply(v1Temp, 1.2);
            const double dAlg = timer.elapsedWallSeconds();
            v1FromAlg = std::move(v1Temp);
            #if ENABLE_RUNTIME_COMPARISONS
                std::cout << "Time forEachMultiply: " << 1000 * dAlg << " ms\n";
            #endif
        }

        {
            auto v1Temp = v1;
            DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timer;
            // See note above for forEachMultiply.
            for (size_t i = 0; i < nLoopCount; ++i)
                std::transform(v1Temp.begin(), v1Temp.end(), v1Temp.begin(), [](double d) {return d * 1.2; });
            const double dAlg = timer.elapsedWallSeconds();
            v1FromTransform = std::move(v1Temp);
            #if ENABLE_RUNTIME_COMPARISONS
                std::cout << "Time std::transform: " << 1000 * dAlg << " ms\n";
            #endif
        }

        EXPECT_TRUE(std::equal(v1FromAlg.begin(), v1FromAlg.end(), v1FromTransform.begin()));
    }
#endif

#if ENABLE_RUNTIME_COMPARISONS
    { // Performance comparison for transformMultiply and std::transform
        using namespace DFG_MODULE_NS(numeric);
        const size_t nVecSize = 1024;
        std::vector<double> v1(nVecSize);
        std::vector<double> v2(nVecSize);
        for (size_t i = 0; i < nVecSize; ++i) // Using std::generate generated weird errors in VC2010 for lambdas.
        {
            v1[i] = DFG_MODULE_NS(rand)::rand();
            v2[i] = DFG_MODULE_NS(rand)::rand();
        }

#ifdef _DEBUG
        const size_t nLoopCount = 1000;
#else
        const size_t nLoopCount = 10000;
#endif

        std::vector<double> v1FromAlg, v1FromTransform;

        {
            auto v1Temp = v1;
            DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timer;
            for (size_t i = 0; i < nLoopCount; ++i)
                transformMultiply(v1Temp, v2, v1Temp.data());
            const double dAlg = timer.elapsedWallSeconds();
            v1FromAlg = std::move(v1Temp);
            #if ENABLE_RUNTIME_COMPARISONS
                std::cout << "Time transformMultiply: " << 1000 * dAlg << " ms\n";
            #endif
        }

        {
            auto v1Temp = v1;
            DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timer;
            for (size_t i = 0; i < nLoopCount; ++i)
                transformInPlace(v1Temp, v2, [](double a, double b) {return a*b; });
            const double dAlg = timer.elapsedWallSeconds();
            v1FromAlg = std::move(v1Temp);
            #if ENABLE_RUNTIME_COMPARISONS
                std::cout << "Time transformInPlace: " << 1000 * dAlg << " ms\n";
            #endif
        }

        {
            auto v1Temp = v1;
            DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timer;
            for (size_t i = 0; i < nLoopCount; ++i)
                std::transform(v1Temp.begin(), v1Temp.end(), v2.begin(), v1Temp.begin(), std::multiplies<double>());
            const double dAlg = timer.elapsedWallSeconds();
            v1FromTransform = std::move(v1Temp);
            #if ENABLE_RUNTIME_COMPARISONS
                std::cout << "Time std::transform: " << 1000 * dAlg << " ms\n";
            #endif
        }

        EXPECT_TRUE(std::equal(v1FromAlg.begin(), v1FromAlg.end(), v1FromTransform.begin()));
    }
#endif

}

namespace
{
    template <class Cont_T>
    double loopVectorizationImplT(Cont_T& cont, const DFG_ROOT_NS::uint16 addVal)
    {
        typedef typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<Cont_T>::type ValueType;
        using namespace DFG_ROOT_NS;
        #if ENABLE_RUNTIME_COMPARISONS
            const auto nRep = 5000;
        #else
            const auto nRep = 50;
        #endif

        double sumVec = 0;
        double sumNonVec = 0;

        {
            const auto firstVal = firstOf(cont);
            DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timer;
            for (int i = 0; i < nRep; ++i)
                DFG_MODULE_NS(numeric)::forEachAdd(cont, ValueType(addVal));
            #if ENABLE_RUNTIME_COMPARISONS
                const auto dElapsed = timer.elapsedWallSeconds();
                std::cout << "forEachAdd T=" << typeid(ValueType).name() << ": " << dElapsed*1000 << " ms\n";
            #endif
            sumVec = std::accumulate(cont.begin(), cont.end(), double(0));
            EXPECT_EQ(firstVal + ValueType(nRep * addVal), firstOf(cont));
        }
        {
            const auto arr = ptrToContiguousMemory(cont);
            const auto nSize = static_cast<int>(count(cont));
            DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timer;
            for (int i = 0; i < nRep; ++i)
            {
                const auto v = ValueType(addVal);
                #pragma loop(no_vector) // uncomment to disable vectorization.
                for (int n = 0; n < nSize; ++n)
                    arr[n] += v;
            }
            #if ENABLE_RUNTIME_COMPARISONS
                const auto dElapsed = timer.elapsedWallSeconds();
                std::cout << "non-vectorized loop with T=" << typeid(ValueType).name() << ": " << dElapsed*1000 << " ms\n";
            #endif
            sumNonVec = std::accumulate(cont.begin(), cont.end(), double(0));
        }
#pragma warning(push)
#pragma warning(disable:4127) // Conditional expression is constant
        if (sizeof(addVal) <= sizeof(ValueType)) // Skip char comparison due to overflows.
            EXPECT_EQ(sumVec, sumNonVec - count(cont) * addVal * nRep);
#pragma warning(pop)
        return sumVec;
    }
}

TEST(dfgNumeric, transform)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(alg);
    using namespace DFG_MODULE_NS(numeric);

    {
        auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();
        const auto nSize = static_cast<size_t>(DFG_MODULE_NS(rand)::rand(randEng, 1500, 2500));
        std::vector<double> v0(nSize, 0);
        std::vector<double> v1(nSize);
        std::vector<double> v2 = v0;
        generateAdjacent(v0, 0, 1);
        v1 = v0;
        v2 = v0;
        DFG_MODULE_NS(numeric)::transform2(v0, v1, v0.data(), [](double a, double b) {return a + std::sin(b); });
        
        for (size_t i = 0; i < nSize; ++i)
            v2[i] = v2[i] + std::sin(v1[i]);
        EXPECT_EQ(v0, v2);

        double valVec = 0;
        double valNonVec = 0;
        #if ENABLE_RUNTIME_COMPARISONS
            const size_t nRep = 5000;
        #else
            const size_t nRep = 50;
        #endif
        {
            DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timer;
            for (size_t i = 0; i < nRep; ++i)
                DFG_MODULE_NS(numeric)::transform1(v0, v0.data(), [](double a) {return std::sin(a); });
                //DFG_MODULE_NS(numeric)::transform2(v0, v1, v0.data(), [](double a, double b) {return a + std::sin(b); });
            #if ENABLE_RUNTIME_COMPARISONS
                const auto dElapsed = timer.elapsedWallSeconds();
                std::cout << "transform1 time: " << dElapsed * 1000 << " ms\n";
            #endif
            valVec = std::accumulate(v0.begin(), v0.end(), 0.0);
        }
        v0 = v2;
        {
            auto v0ptr = ptrToContiguousMemory(v0);
            //auto v1ptr = ptrToContiguousMemory(v1);
            DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timer;
            for (size_t i = 0; i < nRep; ++i)
            {
                #pragma loop(no_vector)
                for (size_t i = 0; i < nSize; ++i)
                    v0ptr[i] = std::sin(v0ptr[i]);
                    //v0ptr[i] += std::sin(v1ptr[i]);
            }
            #if ENABLE_RUNTIME_COMPARISONS
                const auto dElapsed = timer.elapsedWallSeconds();
                std::cout << "nonvectorised time: " << dElapsed * 1000 << " ms\n";
            #endif
            valNonVec = std::accumulate(v0.begin(), v0.end(), 0.0);
        }
        EXPECT_EQ(valVec, valNonVec);
    }
}

TEST(dfgNumeric, loopVectorization)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(alg);
    auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();

    const auto nSize = DFG_MODULE_NS(rand)::rand(randEng, 25000, 25100);

    std::vector<uint8> cont8(nSize);
    generateAdjacent(cont8, uint8(0), uint8(1));

    std::vector<uint16> cont16(nSize);
    generateAdjacent(cont16, uint16(0), int16(1));

    std::vector<int32> cont32(cont16.begin(), cont16.end());
    std::vector<int64> cont64(cont16.begin(), cont16.end());
    std::vector<float> contF(cont16.begin(), cont16.end());
    std::vector<double> contD(cont16.begin(), cont16.end());

    const auto addVal = static_cast<uint16>(DFG_MODULE_NS(rand)::rand(randEng, 2, 4));

    /*const auto v8 =*/ loopVectorizationImplT(cont8, addVal);
    const auto v16 = loopVectorizationImplT(cont16, addVal);
    const auto v32 = loopVectorizationImplT(cont32, addVal);
    const auto v64 = loopVectorizationImplT(cont64, addVal);
    const auto vF = loopVectorizationImplT(contF, addVal);
    const auto vD = loopVectorizationImplT(contD, addVal);
    EXPECT_EQ(v16, v32);
    EXPECT_EQ(v16, v64);
    EXPECT_EQ(v16, vF);
    EXPECT_EQ(v16, vD);
}

TEST(dfgNumeric, loopVectorizationSin)
{
    const auto test = [](const char* psz, const int method) -> double
    {
        typedef double ValType;
        using namespace DFG_ROOT_NS;
        auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();
        auto distrEng = DFG_MODULE_NS(rand)::makeDistributionEngineUniform(&randEng, 0, 1);
#ifdef _DEBUG
        const auto N = 20;
#else
        #if ENABLE_RUNTIME_COMPARISONS
            //const auto N = 20000;
            const size_t N = 20000 + static_cast<size_t>(2 * DFG_MODULE_NS(rand)::rand(randEng));
        #else
            const size_t N = 200 + static_cast<size_t>(2 * DFG_MODULE_NS(rand)::rand(randEng));
        #endif
#endif
        //ValType source[N];
        std::vector<ValType> source(N);
        ValType* sourcePtr = ptrToContiguousMemory(source);

        std::fill(sourcePtr, sourcePtr + N, DFG_MODULE_NS(math)::const_pi / 2);

        //ValType dest[N];
        std::vector<ValType> dest(N);
        ValType* destPtr = ptrToContiguousMemory(dest);
        std::fill(destPtr, destPtr + N, 0.0);

        double sum = 0;
        DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timer;
        for (size_t rep = 0; rep < 500; ++rep)
        {
            if (method == 0)
            {
                DFG_ZIMPL_VECTORIZING_LOOP(destPtr, N, += std::sin(sourcePtr[i]));
            }
            else if (method == 1)
            {
                DFG_MODULE_NS(numeric)::transform2(sourcePtr, N, destPtr, N, destPtr, [](ValType a, ValType b){return b + std::sin(a); });
                //DFG_MODULE_NS(numeric)::transform1(sourcePtr, N, destPtr, [](ValType a){return a + std::sin(a); });
            }
            else if (method == 2)
            {
                DFG_MODULE_NS(numeric)::transformInPlace(destPtr, N, sourcePtr, N, [](ValType a, ValType b){return a + std::sin(b); });
            }
            else if (method == 3)
            {
                for (size_t n = 0; n < N; ++n)
                    destPtr[n] += std::sin(sourcePtr[n]);
            }
            else
            {
                #pragma loop(no_vector) // uncomment to disable vectorization.
                for (size_t n = 0; n < N; ++n)
                    destPtr[n] += std::sin(sourcePtr[n]);

            }
            sum += std::accumulate(std::begin(dest), std::end(dest), 0.0);
        }
        const auto dTime = timer.elapsedWallSeconds();
        DFG_UNUSED(psz);
        DFG_UNUSED(dTime);
        #if ENABLE_RUNTIME_COMPARISONS
            //std::cout << "First source " << sourcePtr[0] <<  "\n";
            std::cout << "Sin sum " << psz << ": " << dTime * 1000 << " ms\n";
            //std::cout << "sum: " << sum << "\n";
            //std::cout << "Dest alignment: " << (uintptr_t)(destPtr) % 16 << "\n";
        #endif
        return sum;
    };

    const auto sum0 = test("dfg vector loop", 0);
    const auto sum1 = test("dfg transform2", 1);
    const auto sum2 = test("dfg transformInPlace", 2);
    const auto sum3 = test("plain loop", 3);
    const auto sum4 = test("non-vectorized", 4);
    EXPECT_EQ(sum0, sum1);
    EXPECT_EQ(sum0, sum2);
    EXPECT_EQ(sum0, sum3);
    EXPECT_EQ(sum0, sum4);
}

namespace
{
    template <class Cont_T>
    void accumulateSingleArgTests(const Cont_T& cont, const int nRep, const typename Cont_T::value_type expected, std::true_type)
    {
        typedef typename Cont_T::value_type T;
        using namespace DFG_MODULE_NS(math);
        using namespace DFG_MODULE_NS(numeric);
        const auto rv = accumulate(std::vector<T>());
        DFGTEST_STATIC((std::is_same<const T, decltype(rv)>::value));
        EXPECT_TRUE(isNan(rv));

        const auto rv2 = accumulate(std::vector<T>(1, T(2)));
        EXPECT_EQ(2, rv2);

        T acc = 0;
        {
            DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timer;
            for (int i = 0; i < nRep; ++i)
                acc += DFG_MODULE_NS(numeric)::accumulate(cont);
#if ENABLE_RUNTIME_COMPARISONS
            const auto dElapsed = timer.elapsedWallSeconds();
            std::cout << "numeric::accumulate(cont) with " << typeid(cont.front()).name() << ": " << 1000 * dElapsed << " ms\n";
#endif
        }
        EXPECT_EQ(expected, acc);
    }

    template <class Cont_T>
    void accumulateSingleArgTests(const Cont_T&, const int, const typename Cont_T::value_type, std::false_type)
    {
    }

    template <class Cont_T>
    void accumulateImpl(const Cont_T& cont)
    {
        using namespace DFG_ROOT_NS;
#if ENABLE_RUNTIME_COMPARISONS
        const int nRep = 10000;
#else
        const int nRep = 10;
#endif
        typedef typename DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ElementType)<Cont_T>::type ValueType;
        ValueType v0Sum = 0;
        ValueType v1Sum = 0;

        // Note: choice of sum variable type is important: seems to have massive effect on the run time in some cases
        //       (e.g. ValueType to uint32 when ValueType is floating point type).
        
        {
            DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timer;
            for (int i = 0; i < nRep; ++i)
                v0Sum += DFG_MODULE_NS(numeric)::accumulate(cont, ValueType(0));
            #if ENABLE_RUNTIME_COMPARISONS
                const auto dElapsed = timer.elapsedWallSeconds();
                std::cout << "numeric::accumulate(cont, ValueType(0)) with " << typeid(cont.front()).name() << ": " << 1000*dElapsed << " ms\n";
            #endif
        }
        {
            DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timer;
            for (int i = 0; i < nRep; ++i)
                v1Sum += std::accumulate(cont.cbegin(), cont.cend(), ValueType(0));
            #if ENABLE_RUNTIME_COMPARISONS
                const auto dElapsed = timer.elapsedWallSeconds();
                std::cout << "std::accumulate with " << typeid(cont.front()).name() << ": " << 1000 * dElapsed << " ms\n";
            #endif
        }

        accumulateSingleArgTests(cont, nRep, v0Sum, std::integral_constant<bool, std::numeric_limits<ValueType>::has_quiet_NaN>());

        EXPECT_EQ(v0Sum, v1Sum);
    }
}

TEST(dfgNumeric, accumulate)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(numeric);
    std::vector<uint8> vec8(40000);
    DFG_MODULE_NS(alg)::generateAdjacent(vec8, uint16(0), uint16(1));
    std::vector<uint16> vec16(30000);
    DFG_MODULE_NS(alg)::generateAdjacent(vec16, uint16(0), uint16(1));
    std::vector<uint32> vec32(vec16.begin(), vec16.end());
    std::vector<uint64> vec64(vec16.begin(), vec16.end());
    std::vector<float> vecF(vec16.begin(), vec16.end());
    std::vector<double> vecD(vec16.begin(), vec16.end());

    accumulateImpl(vec8);
    accumulateImpl(vec16);
    accumulateImpl(vec32);
    accumulateImpl(vec64);
    accumulateImpl(vecF);
    accumulateImpl(vecD);

    const auto accD = accumulate(vecD);
    EXPECT_EQ(accD, accumulateTyped<double>(vec16));
    EXPECT_EQ(accD, accumulateTyped<double>(vec32));
    EXPECT_EQ(accD, accumulateTyped<double>(vec64));
    EXPECT_EQ(accD, accumulateTyped<double>(vecF));
}

TEST(dfgNumeric, rescale)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_ROOT_NS::DFG_SUB_NS_NAME(numeric);

    // Test empty list handling
    {
        std::vector<double> empty;
        rescale(empty, -1, 1);
        EXPECT_TRUE(empty.empty());
    }

    // Test single item list handling
    {
        std::vector<double> vec(1, 6.0);
        rescale(vec, -1, 1);
        EXPECT_EQ(vec.size(), 1);
        EXPECT_EQ(vec.front(), -1);
    }

    // Test range of positive values
    // and
    // Test non-empty range to single value range.
    {
        std::array<double, 4> arr = { 2, 10, 6, 5};
        std::array<double, 4> arrExpected = { -1, 1, -1 + 2*4.0/8.0, -1 + 2*3.0/8.0 };
        std::array<double, 4> arrExpected2 = { 5, 5, 5, 5 };
        rescale(arr, -1, 1);
        EXPECT_EQ(arrExpected.size(), arr.size());
        EXPECT_TRUE(std::equal(arr.begin(), arr.end(), arrExpected.begin()));

        rescale(arr, 5, 5);
        EXPECT_EQ(arrExpected2.size(), arr.size());
        EXPECT_TRUE(std::equal(arr.begin(), arr.end(), arrExpected2.begin()));
    }

    // Test range of negative values
    {
        std::array<double, 4> arr = { -5, -10, -10, -6 };
        std::array<double, 4> arrExpected = { 3, -2, -2, 2 };
        rescale(arr, -2, 3);
        EXPECT_EQ(arrExpected.size(), arr.size());
        EXPECT_TRUE(std::equal(arr.begin(), arr.end(), arrExpected.begin()));
    }

    // Test range of positive and negative values
    {
        const std::array<double, 3> arrBegin = { -1e3, 1e8, -5.4e9 };
        std::array<double, 3> arr = arrBegin;
        const std::array<double, 3> arrExpected = { (-1000.0 + 5.4e9) / (5.4e9+1e8) , 1, 0 };
        rescale(arr, 0, 1);
        EXPECT_EQ(arrExpected.size(), arr.size());
        EXPECT_TRUE(std::equal(arr.begin(), arr.end(), arrExpected.begin()));

        // Test that function works also for non-contiguous containers.
        std::list<double> listTest(arrBegin.begin(), arrBegin.end());
        rescale(listTest, 0, 1);
        EXPECT_TRUE(std::equal(arrExpected.begin(), arrExpected.end(), listTest.begin()));
    }

    // Test mapping to inverse range.
    {
        std::array<double, 6> arr = { 2, 7, 6, 12, 4, 7 };
        const std::array<double, 6> arrExpected = { 1, -1, -0.6, -3, 0.2, -1 };
        const std::array<double, 6> arrExpected2 = { 7, 3, 3.8, -1, 5.4, 3 };
        rescale(arr, 1, -3);
        EXPECT_EQ(arrExpected.size(), arr.size());
        for (size_t i = 0; i < arr.size(); ++i)
        {
            EXPECT_NEAR(arrExpected[i], arr[i], 1e-12);
        }
        rescale(arr, -1, 7);
        EXPECT_EQ(arrExpected2.size(), arr.size());
        for (size_t i = 0; i < arr.size(); ++i)
        {
            EXPECT_NEAR(arrExpected2[i], arr[i], 1e-12);
        }
    }

#if 0 // Disabled completely since there no value to compare to.
    {
        using namespace DFG_MODULE_NS(alg);
        typedef double ValType;
        std::vector<ValType> vals(1024);
        generateAdjacent(vals, 0, 1);
        rescale(vals, 0, 2046);
        std::vector<ValType> valsExpected(1024);
        generateAdjacent(valsExpected, 0, 2);
        EXPECT_EQ(valsExpected, vals);

        auto eng = DFG_MODULE_NS(rand)::createDefaultRandEngineRandomSeeded();
        auto distrEng = DFG_MODULE_NS(rand)::makeDistributionEngineUniform(&eng, 1, 10);
        DFG_MODULE_NS(time)::DFG_CLASS_NAME(TimerCpu) timer;
        for (int i = 0; i < 100000; ++i)
        {
            rescale(vals, 0, distrEng() * 1023);
        }
        const auto elapsed = timer.elapsedWallSeconds();
        std::cout << "Time rescale: " << 1000 * elapsed << " ms\n";
        const auto diff = vals[1] - vals[0];
        for (size_t i = 2; i < vals.size(); ++i)
        {
            EXPECT_NEAR(diff, vals[i] - vals[i-1], 1e-6);
        }
    }
#endif // ENABLE_RUNTIME_COMPARISONS

    // Test integer range (should fail to compile)
    {
        //std::array<int, 2> arr = { 1, 2 };
        //rescale(arr, 0, 1);
    }
}

TEST(dfgNumeric, percentileRange_and_percentile_ceilElem)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(numeric);

    // Test single element
    {
        const double arr[] = { 1 };
        const auto rvArr0_100 = percentileRangeInSortedII(arr, 0, 100);
        EXPECT_EQ(rvArr0_100.first, std::begin(arr) + 0);
        EXPECT_EQ(rvArr0_100.second, std::end(arr));

        const auto rvArr40_60 = percentileRangeInSortedII(arr, 40, 60);
        EXPECT_EQ(rvArr0_100, rvArr40_60);

        const auto percentileElem0 = percentileInSorted_enclosingElem(arr, 0);
        EXPECT_EQ(1, percentileElem0);
        const auto percentileElem50 = percentileInSorted_enclosingElem(arr, 50);
        EXPECT_EQ(1, percentileElem50);
        const auto percentileElem100 = percentileInSorted_enclosingElem(arr, 100);
        EXPECT_EQ(1, percentileElem100);
    }

    // Test two elements
    {
        const double arr[] = { 1, 2 };
        const auto rvArr0_50 = percentileRangeInSortedII(arr, 0, 50);
        EXPECT_EQ(rvArr0_50.first, std::begin(arr));
        EXPECT_EQ(rvArr0_50.second, std::end(arr));

        const auto rvArr0_49 = percentileRangeInSortedII(arr, 0, 49);
        EXPECT_EQ(rvArr0_49.first, std::begin(arr));
        EXPECT_EQ(rvArr0_49.second, std::begin(arr) + 1);

        const auto rvArr50_50 = percentileRangeInSortedII(arr, 50, 50);
        EXPECT_EQ(rvArr50_50.first, std::begin(arr));
        EXPECT_EQ(rvArr50_50.second, std::end(arr));

        const auto rvArr51_50 = percentileRangeInSortedII(arr, 51, 50);
        EXPECT_EQ(rvArr51_50.first, std::end(arr));
        EXPECT_EQ(rvArr51_50.second, std::end(arr));

        const auto rvArr51_100 = percentileRangeInSortedII(arr, 51, 100);
        EXPECT_EQ(rvArr51_100.first, std::begin(arr) + 1);
        EXPECT_EQ(rvArr51_100.second, std::end(arr));

        const auto percentileElem0 = percentileInSorted_enclosingElem(arr, 0);
        EXPECT_EQ(1, percentileElem0);
        const auto percentileElem50 = percentileInSorted_enclosingElem(arr, 50);
        EXPECT_EQ(2, percentileElem50);
        const auto percentileElem100 = percentileInSorted_enclosingElem(arr, 100);
        EXPECT_EQ(2, percentileElem100);
    }

    // Test four elements
    {
        const double arr[] = { 1, 2, 3, 4 };
        const auto rvArr = percentileRangeInSortedII(arr, 0, 100);
        EXPECT_EQ(rvArr.first, std::begin(arr));
        EXPECT_EQ(rvArr.second, std::end(arr));

        const auto rvArr30_60 = percentileRangeInSortedII(arr, 30, 60);
        EXPECT_EQ(rvArr30_60.first, std::begin(arr) + 1);
        EXPECT_EQ(rvArr30_60.second, std::begin(arr) + 3);

        const auto percentileElem0 = percentileInSorted_enclosingElem(arr, 0);
        EXPECT_EQ(1, percentileElem0);
        const auto percentileElem249 = percentileInSorted_enclosingElem(arr, 24.9);
        EXPECT_EQ(1, percentileElem249);
        const auto percentileElem25 = percentileInSorted_enclosingElem(arr, 25);
        EXPECT_EQ(2, percentileElem25);
        const auto percentileElem49 = percentileInSorted_enclosingElem(arr, 49);
        EXPECT_EQ(2, percentileElem49);
        const auto percentileElem50 = percentileInSorted_enclosingElem(arr, 50);
        EXPECT_EQ(3, percentileElem50);
        const auto percentileElem75 = percentileInSorted_enclosingElem(arr, 75);
        EXPECT_EQ(4, percentileElem75);
        const auto percentileElem100 = percentileInSorted_enclosingElem(arr, 110);
        EXPECT_EQ(4, percentileElem100);

        // Test percentileInSorted_enclosingElem with ArrayWrapper
        {
            using namespace DFG_MODULE_NS(cont);
            EXPECT_EQ(percentileElem50, (percentileInSorted_enclosingElem(DFG_CLASS_NAME(ArrayWrapper)::createArrayWrapper(arr), 50)));
        }
    }

    // Test 10 elements.
    {
        const double arr[] = { 1,2,3,4,5,6,7,8,9,10 };
        const std::array<double, 10> stdArr = { 1,2,3,4,5,6,7,8,9,10 };
        const auto rvArr = percentileRangeInSortedII(arr, 0, 100);
        const auto rvStdArr = percentileRangeInSortedII(stdArr, 0, 100);
        EXPECT_EQ(rvArr.first, std::begin(arr));
        EXPECT_EQ(rvArr.second, std::end(arr));
        EXPECT_EQ(rvStdArr.first, std::begin(stdArr));
        EXPECT_EQ(rvStdArr.second, std::end(stdArr));

        const auto rv20_70 = percentileRangeInSortedII(arr, 20, 70);
        EXPECT_EQ(rv20_70.first, std::begin(arr) + 1);
        EXPECT_EQ(rv20_70.second, std::begin(arr) + 8);

        const auto rv5_8 = percentileRangeInSortedII(arr, 5, 8);
        EXPECT_EQ(rv5_8.first, std::begin(arr));
        EXPECT_EQ(rv5_8.second, std::begin(arr) + 1);

        const auto rv15_22 = percentileRangeInSortedII(arr, 15, 22);
        EXPECT_EQ(rv15_22.first, std::begin(arr) + 1);
        EXPECT_EQ(rv15_22.second, std::begin(arr) + 3);
    }

    // Test big element count
    {
        std::vector<double> vec(13000);
        DFG_MODULE_NS(alg)::generateAdjacent(vec, 0, 1);
        const auto rv54_63 = percentileRangeInSortedII(vec, 54.35, 63.17);
        EXPECT_EQ(rv54_63.first, std::begin(vec) + 7065);
        EXPECT_EQ(rv54_63.second, std::begin(vec) + 8213);
    }

    // Test that 50, 50 gives median.
    {
        auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();
        const size_t nSize = 1001;
        std::vector<double> vals(nSize);
        for (size_t i = 0; i < nSize; ++i)
        {
            vals[i] = DFG_MODULE_NS(rand)::rand(randEng, -200.0, 50.0);
        }
        std::sort(vals.begin(), vals.end());
        const auto rvPairOdd = percentileRangeInSortedII(vals, 50, 50);
        EXPECT_EQ(1, rvPairOdd.second - rvPairOdd.first);
        EXPECT_EQ(medianInSorted(vals), *rvPairOdd.first);

        const auto percentileElem50 = percentileInSorted_enclosingElem(vals, 50);
        EXPECT_EQ(medianInSorted(vals), percentileElem50);

        vals.erase(vals.begin());
        const auto rvPairEven = percentileRangeInSortedII(vals, 50, 50);
        EXPECT_EQ(2, rvPairEven.second - rvPairEven.first);
        EXPECT_EQ(medianInSorted(vals), 0.5 * (*(rvPairEven.second - 1) + *rvPairEven.first));
    }
}

TEST(dfgNumeric, trimByPercentileRange)
{
    using namespace DFG_MODULE_NS(numeric);
    auto randEng = DFG_MODULE_NS(rand)::createDefaultRandEngineUnseeded();
    const size_t nSize = 1001;
    std::vector<double> vals(nSize);
    for (size_t i = 0; i < nSize; ++i)
    {
        vals[i] = DFG_MODULE_NS(rand)::rand(randEng, -200.0, 50.0);
    }
    auto valsSorted = vals;
    std::sort(valsSorted.begin(), valsSorted.end());
    const auto pr = percentileRangeInSortedII(valsSorted, 30, 65);
    const std::vector<double> expected(pr.first, pr.second);
    trimToPercentileRangeII(vals, 30, 65);
    trimToPercentileRangeIISorted(valsSorted, 30, 65);
    EXPECT_EQ(expected, vals);
    EXPECT_EQ(valsSorted, vals);
}

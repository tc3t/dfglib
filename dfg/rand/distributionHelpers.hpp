#pragma once

#include "../dfgBase.hpp"

#include <random>
#include <limits>
#include "../rand.hpp"
#include "../str/strTo.hpp"
#include "../str.hpp"
#include "../math.hpp"
#include "../func.hpp"
#include <tuple>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(rand) {

    // Wrapper for std::negative_binomial_distribution to avoid buggy implementations regarding handling of p == 1.
    template<class Int_T = int>
    class NegativeBinomialDistribution : public std::negative_binomial_distribution<Int_T>
    {
    public:
        typedef std::negative_binomial_distribution<Int_T> BaseClass;

        explicit NegativeBinomialDistribution()
        {}

        explicit NegativeBinomialDistribution(Int_T k, double p = 0.5)
            : BaseClass(k, p)
        {}

        template<class RandEng_T>
        typename BaseClass::result_type operator()(RandEng_T& g)
        {
            if (this->p() == 1)
                return 0;
            else
                return BaseClass::operator()(g);
        }

    };

    // This namespace defines functions that can be used to validate args used to construct std:: distributions.
    // Args need to be validated because passing invalid args to constructors result in many/all cases either UB
    // or crash due to ASSERT in constructor or operator().
    namespace distributionArgValidation
    {
        namespace DFG_DETAIL_NS
        {
            template <class T> bool isFiniteAndGreaterThanZero(const T& val)
            {
                return ::DFG_MODULE_NS(math)::isFinite(val) && val > 0;
            }

            template <class T, class Args_T>
            bool hasFiniteGreaterThanZeroSingleReal(const Args_T& args)
            {
                return isFiniteAndGreaterThanZero(std::get<0>(args));
            }

            template <class T, class Args_T>
            bool hasFiniteGreaterThanZeroTwoReals(const Args_T& args)
            {
                return isFiniteAndGreaterThanZero(std::get<0>(args)) && isFiniteAndGreaterThanZero(std::get<1>(args));
            }

            template <class T, class Args_T>
            bool hasFiniteFirstAndFiniteGreaterThanZeroSecond(const Args_T& args)
            {
                return ::DFG_MODULE_NS(math)::isFinite(std::get<0>(args)) && isFiniteAndGreaterThanZero<T>(std::get<1>(args));
            }
        }

        template <class T, class Args_T>
        bool canConstruct_uniform_int_distribution(const Args_T& args)
        {
            // 0: minimum
            // 1: maximum
            return static_cast<T>(std::get<0>(args)) <= static_cast<T>(std::get<1>(args));
        }

        template <class T, class Args_T>
        bool canConstruct_binomial_distribution(const Args_T& args)
        {
            // 0: count (integer)
            // 1: probability (real)
            return static_cast<T>(std::get<0>(args)) >= 0
                &&
                std::get<1>(args) >= 0
                &&
                std::get<1>(args) <= 1;
        }

        template <class Args_T>
        bool canConstruct_bernoulli_distribution(const Args_T& args)
        {
            // 0: probability (real)
            return std::get<0>(args) >= 0
                &&
                std::get<0>(args) <= 1;
        }

        template <class T, class Args_T>
        bool canConstruct_NegativeBinomialDistribution(const Args_T& args)
        {
            // 0: count (integer)
            // 1: probability (real)
            return static_cast<T>(std::get<0>(args)) > 0
                &&
                std::get<1>(args) > 0
                &&
                std::get<1>(args) <= 1;
        }

        template <class T, class Args_T>
        bool canConstruct_geometric_distribution(const Args_T& args)
        {
            // 0: probability (real)
            return std::get<0>(args) > 0
                &&
                std::get<0>(args) < 1;
        }

        template <class T, class Args_T>
        bool canConstruct_poisson_distribution(const Args_T& args) { return DFG_DETAIL_NS::hasFiniteGreaterThanZeroSingleReal<T>(args); }

        template <class T, class Args_T>
        bool canConstruct_uniform_real_distribution(const Args_T& args)
        {
            // 0: minimum
            // 1: maximum
            return static_cast<T>(std::get<0>(args)) < static_cast<T>(std::get<1>(args))
                &&
                static_cast<T>(std::get<1>(args)) - static_cast<T>(std::get<0>(args)) <= (std::numeric_limits<T>::max)();
        }

        template <class T, class Args_T>
        bool canConstruct_normal_distribution(const Args_T& args) { return DFG_DETAIL_NS::hasFiniteFirstAndFiniteGreaterThanZeroSecond<T>(args); }

        template <class T, class Args_T>
        bool canConstruct_cauchy_distribution(const Args_T& args) { return DFG_DETAIL_NS::hasFiniteFirstAndFiniteGreaterThanZeroSecond<T>(args); }

        template <class T, class Args_T>
        bool canConstruct_exponential_distribution(const Args_T& args) { return DFG_DETAIL_NS::hasFiniteGreaterThanZeroSingleReal<T>(args); }

        template <class T, class Args_T>
        bool canConstruct_chi_squared_distribution(const Args_T& args) { return DFG_DETAIL_NS::hasFiniteGreaterThanZeroSingleReal<T>(args); }

        template <class T, class Args_T>
        bool canConstruct_student_t_distribution(const Args_T& args) { return DFG_DETAIL_NS::hasFiniteGreaterThanZeroSingleReal<T>(args); }

        template <class T, class Args_T>
        bool canConstruct_gamma_distribution(const Args_T& args) { return DFG_DETAIL_NS::hasFiniteGreaterThanZeroTwoReals<T>(args); }

        template <class T, class Args_T>
        bool canConstruct_weibull_distribution(const Args_T& args) { return DFG_DETAIL_NS::hasFiniteGreaterThanZeroTwoReals<T>(args); }

        template <class T, class Args_T>
        bool canConstruct_extreme_value_distribution(const Args_T& args) { return DFG_DETAIL_NS::hasFiniteFirstAndFiniteGreaterThanZeroSecond<T>(args); }

        template <class T, class Args_T>
        bool canConstruct_lognormal_distribution(const Args_T& args) { return DFG_DETAIL_NS::hasFiniteFirstAndFiniteGreaterThanZeroSecond<T>(args); }

        template <class T, class Args_T>
        bool canConstruct_fisher_f_distribution(const Args_T& args) { return DFG_DETAIL_NS::hasFiniteGreaterThanZeroTwoReals<T>(args); }

        namespace DFG_DETAIL_NS
        {
            const size_t args_0_or_1      = 3; // n'th bit means if n-arguments is accepted (i.e. 3 = 11b = accepts zero or one args)
            const size_t args_0_or_1_or_2 = 7;
            template <size_t N> struct MaxArgCount {};
            template <> struct MaxArgCount<3> { static const int value = 1; };
            template <> struct MaxArgCount<7> { static const int value = 2; };
        }

        template <class Distr_T> struct DistributionDetails;

    #define DFG_TEMP_DEFINE_DISTRIBUTION_DETAILS(NAMESPACE, CLASS_NAME, ACCEPTED_ARG_COUNT, ...) \
        template <class T> struct DistributionDetails<NAMESPACE::CLASS_NAME<T>> \
        { \
            template <class Args_T> \
            static bool canConstruct(const Args_T& args) \
            { \
                return canConstruct_##CLASS_NAME<T>(args.second); \
            } \
            static bool isAcceptableParamCount(const ptrdiff_t val) { return val < 32 && ((DFG_DETAIL_NS::ACCEPTED_ARG_COUNT & (ptrdiff_t(1) << val)) != 0); } \
            static const ptrdiff_t MaxArgCount = DFG_DETAIL_NS::MaxArgCount<DFG_DETAIL_NS::ACCEPTED_ARG_COUNT>::value; \
            static std::pair<bool, std::tuple<__VA_ARGS__>> makeUninitializedParams() { return std::make_pair(false, std::tuple<__VA_ARGS__>()); } \
        }

        // ---------------------------------------------------------------

        // Defining specialization for integer distributions
        DFG_TEMP_DEFINE_DISTRIBUTION_DETAILS(std, uniform_int_distribution,       args_0_or_1_or_2,      T, T);
        DFG_TEMP_DEFINE_DISTRIBUTION_DETAILS(std, binomial_distribution,          args_0_or_1_or_2,      T, double);
        DFG_TEMP_DEFINE_DISTRIBUTION_DETAILS(::DFG_MODULE_NS(rand), NegativeBinomialDistribution,        args_0_or_1_or_2,      T, double);
        DFG_TEMP_DEFINE_DISTRIBUTION_DETAILS(std, geometric_distribution,              args_0_or_1, double);
        DFG_TEMP_DEFINE_DISTRIBUTION_DETAILS(std, poisson_distribution,                args_0_or_1, double);

        // bernoulli_distribution does not have template parameters so not using the macro helper.
        template <> struct DistributionDetails<std::bernoulli_distribution>
        {
            template <class Args_T> static bool canConstruct(const Args_T& args) { return distributionArgValidation::canConstruct_bernoulli_distribution(args.second); }
            static bool isAcceptableParamCount(const ptrdiff_t val) { return val == 0 || val == 1; }
            static const int MaxArgCount = 1;
            static std::pair<bool, std::tuple<double>> makeUninitializedParams() { return std::make_pair(false, std::tuple<double>()); } \
        };

        // ---------------------------------------------------------------

        // Defining specialization for real-valued distributions
        DFG_TEMP_DEFINE_DISTRIBUTION_DETAILS(std, exponential_distribution,        args_0_or_1, T);
        DFG_TEMP_DEFINE_DISTRIBUTION_DETAILS(std, chi_squared_distribution,        args_0_or_1, T);
        DFG_TEMP_DEFINE_DISTRIBUTION_DETAILS(std, student_t_distribution,          args_0_or_1, T);
        DFG_TEMP_DEFINE_DISTRIBUTION_DETAILS(std, uniform_real_distribution,  args_0_or_1_or_2, T, T);
        DFG_TEMP_DEFINE_DISTRIBUTION_DETAILS(std, normal_distribution,        args_0_or_1_or_2, T, T);
        DFG_TEMP_DEFINE_DISTRIBUTION_DETAILS(std, cauchy_distribution,        args_0_or_1_or_2, T, T);
        DFG_TEMP_DEFINE_DISTRIBUTION_DETAILS(std, gamma_distribution,         args_0_or_1_or_2, T, T);
        DFG_TEMP_DEFINE_DISTRIBUTION_DETAILS(std, weibull_distribution,       args_0_or_1_or_2, T, T);
        DFG_TEMP_DEFINE_DISTRIBUTION_DETAILS(std, extreme_value_distribution, args_0_or_1_or_2, T, T);
        DFG_TEMP_DEFINE_DISTRIBUTION_DETAILS(std, lognormal_distribution,     args_0_or_1_or_2, T, T);
        DFG_TEMP_DEFINE_DISTRIBUTION_DETAILS(std, fisher_f_distribution,      args_0_or_1_or_2, T, T);


    #undef DFG_TEMP_DEFINE_DISTRIBUTION_DETAILS

        namespace DFG_DETAIL_NS
        {
            template <class Str_T, class Int_T>
            void tryParseFloatAsInt(std::true_type, const Str_T& str, Int_T& rDest, bool& rbSuccess)
            {
                bool bSuccess = false;
                auto val = ::DFG_MODULE_NS(str)::strTo<double>(str, &bSuccess);
                Int_T intVal = 0;
                if (bSuccess && ::DFG_MODULE_NS(math)::isFloatConvertibleTo<Int_T>(val, &intVal))
                {
                    rDest = intVal;
                    rbSuccess = true;
                }
            }

            template <class Str_T, class Int_T>
            void tryParseFloatAsInt(std::false_type, const Str_T, Int_T&, bool&)
            {
            }

            template <class Str_T, class Dest_T>
            bool assignStrToDest(const Str_T& str, Dest_T& dest)
            {
                if (!::DFG_MODULE_NS(str)::isEmptyStr(str))
                {
                    bool bSuccess = false;
                    dest = ::DFG_MODULE_NS(str)::strTo<Dest_T>(str, &bSuccess);
                    if (!bSuccess)
                        tryParseFloatAsInt(typename std::is_integral<Dest_T>(), str, dest, bSuccess);
                    return bSuccess;
                }
                else
                    return false;
            }

            template <class Iterable_T, class ArgSet_T>
            bool assignValues(const Iterable_T& iterable, ArgSet_T& vals, std::integral_constant<int, 1>)
            {
                return (!isEmpty(iterable)) ? assignStrToDest(*iterable.begin(), std::get<0>(vals)) : true;
            }

            template <class Iterable_T, class ArgSet_T>
            bool assignValues(const Iterable_T& iterable, ArgSet_T& vals, std::integral_constant<int, 2>)
            {
                bool bGoodAssigns = true;
                if (!isEmpty(iterable))
                    bGoodAssigns = assignStrToDest(*std::begin(iterable), std::get<0>(vals));
                if (!bGoodAssigns)
                    return false;
                if (count(iterable) >= 2)
                    bGoodAssigns = assignStrToDest(*(std::begin(iterable) + 1), std::get<1>(vals));
                return bGoodAssigns;
            }

            template <class Iterable_T, class ArgSet_T>
            bool assignValues(const Iterable_T& iterable, ArgSet_T& vals)
            {
                return assignValues(iterable, vals, std::integral_constant<int, std::tuple_size<ArgSet_T>::value>());
            }
        } // namespace DFG_DETAIL_NS

    } // namespace distributionArgValidation

    // Returns true if given args can be used to construct distribution of type Distr_T, otherwise false.
    // Note: does not handle default values, i.e. requires full args set.
    template <class Distr_T, class Args_T> bool validateDistributionParams(const Args_T& args)
    {
        return distributionArgValidation::DistributionDetails<Distr_T>::canConstruct(args);
    }

    // Makes a distribution from given args. See makeDistributionConstructArgs() and makeUninitializedDistributionArgs() for ways to create args.
    // Requires: args.first == true
    template <class Distr_T, class T> Distr_T makeDistribution(const std::pair<bool, std::tuple<T>>& args)
    {
        DFG_ASSERT_UB(args.first);
        return Distr_T(std::get<0>(args.second));
    }

    // Two arg overload, see single arg version for documentation.
    template <class Distr_T, class T0, class T1> Distr_T makeDistribution(const std::pair<bool, std::tuple<T0, T1>>& args)
    {
        DFG_ASSERT_UB(args.first);
        return Distr_T(std::get<0>(args.second), std::get<1>(args.second));
    }

    // Creates constructor args to be used for creating a distribution of type Distr_T from given set of arguments.
    // To determine whether returned args are valid for to be passed to makeDistribution(), check first-member from returned object.
    // Note: currently iterable is required to be list of strings.
    template <class Distr_T, class Iterable_T>
    auto makeDistributionConstructArgs(const Iterable_T& iterable) -> decltype(::DFG_MODULE_NS(rand)::distributionArgValidation::DistributionDetails<Distr_T>::makeUninitializedParams())
    {
        typedef distributionArgValidation::DistributionDetails<Distr_T> ValidatorT;
        typedef decltype(ValidatorT::makeUninitializedParams()) ReturnType;
        if (!ValidatorT::isAcceptableParamCount(count(iterable)))
            return ReturnType();
        ReturnType rv;
        rv.first = distributionArgValidation::DFG_DETAIL_NS::assignValues(iterable, rv.second);
        if (rv.first)
            rv.first = validateDistributionParams<Distr_T>(rv);
        return rv;
    }

    // Creates uninitialized list of args that can be manually filled and validated before passed to makeDistribution()
    template <class Distr_T>
    auto makeUninitializedDistributionArgs() -> decltype(::DFG_MODULE_NS(rand)::distributionArgValidation::DistributionDetails<Distr_T>::makeUninitializedParams())
    {
        return distributionArgValidation::DistributionDetails<Distr_T>::makeUninitializedParams();
    }

    // Helper functor for generating random values from given distribution.
    // Example usage:
    //      auto rangEng = dfg::rand::createDefaultRandEngineRandomSeeded();
    //      DistributionFunctor<std::normal_distribution<double>> distr(&randEng);
    //      auto val = distr(0.0, 1.0); // Note that operator() interface is picky with types, e.g. distr(0, 1) won't compile.
    // What this class does compared to direct lambdas like [&](double a, double b) { std::normal_distribution<double>(a, b)(randEng); } is argument validation.
    template <class Distr_T>
    class DistributionFunctor
    {
    public:
        using RandEngT = decltype(createDefaultRandEngineRandomSeeded());
        using DistrT = Distr_T;
        using ArgStorage_T = typename std::tuple_element<1, decltype(::DFG_MODULE_NS(rand)::makeUninitializedDistributionArgs<DistrT>())>::type;
        using StdFunction_T = typename std::conditional<std::tuple_size<ArgStorage_T>::value == 2, std::function<double(double, double)>, std::function<double(double)>>::type;

        struct InitialValueSetter
        {
            // Integers gets set to 0.
            template <class T>
            void operator()(T& a) { a = std::numeric_limits<T>::quiet_NaN(); }
        };

    public:
        StdFunction_T toStdFunction() const { return StdFunction_T(*this); }

        DistributionFunctor(RandEngT* pRandEng)
            : m_pRandEng(pRandEng)
        {
            ::DFG_MODULE_NS(func)::forEachInTuple(m_argStorage, InitialValueSetter());
        }

        template <class ... Types>
        double operator()(Types ... args)
        {
            const auto argTuple = std::make_tuple(args...);
            if (!m_bGoodToGenerate || m_argStorage != argTuple)
                initializeDistribution(argTuple);
            return generateFromDistribution();
        }

        template <size_t N, class Args_T>
        bool copyArgsToInternalStorage(const Args_T&, std::false_type)
        {
            return true;
        }

        template <size_t N, class Args_T>
        bool copyArgsToInternalStorage(const Args_T& args, std::true_type)
        {
            using namespace ::DFG_MODULE_NS(math);
            using ElementT = typename std::tuple_element<N, ArgStorage_T>::type;
            const auto& newArg = std::get<N>(args);
            if (!isFloatConvertibleTo<ElementT>(newArg))
                return false;
            std::get<N>(m_argStorage) = static_cast<ElementT>(newArg);
            return copyArgsToInternalStorage<N + 1>(args, std::integral_constant<bool, (N + 1 < std::tuple_size<ArgStorage_T>::value)>());
        }

        template <class DoubleArgs_T>
        void initializeDistribution(const DoubleArgs_T& args)
        {
            if (copyArgsToInternalStorage<0>(args, std::true_type()))
            {
                using namespace ::DFG_MODULE_NS(rand);
                auto distrArgs = makeUninitializedDistributionArgs<DistrT>();
                distrArgs.second = m_argStorage;
                distrArgs.first = validateDistributionParams<DistrT>(distrArgs);
                if (distrArgs.first)
                {
                    m_distr = makeDistribution<DistrT>(distrArgs);
                    m_bGoodToGenerate = (m_pRandEng != nullptr);
                }
                else
                    m_bGoodToGenerate = false;
            }
            else
                m_bGoodToGenerate = false;
        }

        double generateFromDistribution()
        {
            return (m_bGoodToGenerate) ? m_distr(*m_pRandEng) : std::numeric_limits<double>::quiet_NaN();
        }

        ArgStorage_T m_argStorage;
        DistrT m_distr;
        bool m_bGoodToGenerate = false;
        RandEngT* m_pRandEng = nullptr;
    }; // class DistributionFunctor

    namespace DFG_DETAIL_NS
    {
        // Helper for iterating through available distribution types using default template types (int/double)
        // This is under DFG_DETAIL_NS as distribution types are not guaranteed to be stable.
        template <class Handler_T>
        void forEachDistributionType(Handler_T&& handler)
        {
            handler(static_cast<const std::uniform_int_distribution<int>*>(nullptr));
            handler(static_cast<const std::binomial_distribution<int>*>(nullptr));
            handler(static_cast<const std::bernoulli_distribution*>(nullptr));
            handler(static_cast<const NegativeBinomialDistribution<int>*>(nullptr));
            handler(static_cast<const std::geometric_distribution<int>*>(nullptr));
            handler(static_cast<const std::poisson_distribution<int>*>(nullptr));
            // Real valued
            handler(static_cast<const std::uniform_real_distribution<double>*>(nullptr));
            handler(static_cast<const std::normal_distribution<double>*>(nullptr));
            handler(static_cast<const std::cauchy_distribution<double>*>(nullptr));
            handler(static_cast<const std::exponential_distribution<double>*>(nullptr));
            handler(static_cast<const std::gamma_distribution<double>*>(nullptr));
            handler(static_cast<const std::weibull_distribution<double>*>(nullptr));
            handler(static_cast<const std::extreme_value_distribution<double>*>(nullptr));
            handler(static_cast<const std::lognormal_distribution<double>*>(nullptr));
            handler(static_cast<const std::chi_squared_distribution<double>*>(nullptr));
            handler(static_cast<const std::fisher_f_distribution<double>*>(nullptr));
            handler(static_cast<const std::student_t_distribution<double>*>(nullptr));
        }
    } // namespace DFG_DETAIL_NS

} } // module namespace

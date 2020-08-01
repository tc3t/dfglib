#pragma once

#include "../dfgBase.hpp"

#include <random>
#include <limits>
#include "../str/strTo.hpp"
#include "../str.hpp"
#include "../math.hpp"

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
            template <class Str_T, class Dest_T>
            void assignStrToDest(const Str_T str, Dest_T& dest)
            {
                if (!::DFG_MODULE_NS(str)::isEmptyStr(str))
                {
                    dest = ::DFG_MODULE_NS(str)::strTo<Dest_T>(str);
                }
            }

            template <class Iterable_T, class ArgSet_T>
            void assignValues(const Iterable_T& iterable, ArgSet_T& vals, std::integral_constant<int, 1>)
            {
                if (!isEmpty(iterable))
                    assignStrToDest(*iterable.begin(), std::get<0>(vals));
            }

            template <class Iterable_T, class ArgSet_T>
            void assignValues(const Iterable_T& iterable, ArgSet_T& vals, std::integral_constant<int, 2>)
            {
                if (!isEmpty(iterable))
                    assignStrToDest(*std::begin(iterable), std::get<0>(vals));
                if (count(iterable) >= 2)
                    assignStrToDest(*(std::begin(iterable) + 1), std::get<1>(vals));
            }

            template <class Iterable_T, class ArgSet_T>
            void assignValues(const Iterable_T& iterable, ArgSet_T& vals)
            {
                assignValues(iterable, vals, std::integral_constant<int, std::tuple_size<ArgSet_T>::value>());
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
        distributionArgValidation::DFG_DETAIL_NS::assignValues(iterable, rv.second);
        rv.first = validateDistributionParams<Distr_T>(rv);
        return rv;
    }

    // Creates uninitialized list of args that can be manually filled and validated before passed to makeDistribution()
    template <class Distr_T>
    auto makeUninitializedDistributionArgs() -> decltype(::DFG_MODULE_NS(rand)::distributionArgValidation::DistributionDetails<Distr_T>::makeUninitializedParams())
    {
        return distributionArgValidation::DistributionDetails<Distr_T>::makeUninitializedParams();
    }


} } // module namespace

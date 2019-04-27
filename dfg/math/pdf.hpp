#pragma once

#include "../dfgDefs.hpp"
#include "pow.hpp"
#include "constants.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(math) {

class DFG_CLASS_NAME(ProbabilityDensityFunction) {};

// Probability density function of Lorentzian distribution a.k.a. Cauchy distribution
// see http://en.wikipedia.org/wiki/Cauchy_distribution, http://mathworld.wolfram.com/CauchyDistribution.html
class DFG_CLASS_NAME(LorentzianDistributionPdf) : public DFG_CLASS_NAME(ProbabilityDensityFunction)
{
public:
	typedef double ValueType;
	DFG_CLASS_NAME(LorentzianDistributionPdf)(const ValueType& x0, const ValueType& fwhm) :
		m_x0(x0),
		m_fwhm(fwhm)
	{
	}
	ValueType operator()(const ValueType& x)
	{
		return (m_fwhm / (2.0*const_pi)) / ( pow2(x - m_x0) + pow2(m_fwhm/2.0) );
	}

    ValueType m_x0;
    ValueType m_fwhm;
};

class DFG_CLASS_NAME(GaussianDistributionPdf) : public DFG_CLASS_NAME(ProbabilityDensityFunction)
{
public:
	typedef double ValueType;
	DFG_CLASS_NAME(GaussianDistributionPdf)(const ValueType& mean, const ValueType& sigma) : m_x0(mean), m_sigma(sigma)
	{
	}
	ValueType operator()(const ValueType& x)
	{
		return exp(-0.5*pow2((x-m_x0)/m_sigma)) / (m_sigma * const_sqrt2Pi);
	}

    ValueType m_x0;
    ValueType m_sigma;
};

}} // module math

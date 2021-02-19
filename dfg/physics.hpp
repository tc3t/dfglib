#ifndef DFG_PHYSICS_DIDNITUL
#define DFG_PHYSICS_DIDNITUL

#include "dfgDefs.hpp"
#include "physics/constants.hpp"
#include "math/constants.hpp"
#include "math/pow.hpp"
#include "build/utils.hpp"
#include <boost/units/quantity.hpp>
#include <boost/units/systems/si/temperature.hpp>
#include <boost/units/systems/si/frequency.hpp>
#include <boost/units/systems/si/energy.hpp>
#include <boost/units/systems/si/temperature.hpp>
#include <boost/units/systems/temperature/celsius.hpp>
#include <boost/units/systems/temperature/fahrenheit.hpp>

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(physics) {

class CelsiusType
{
public:
	explicit CelsiusType(double val) :
	  m_val(val)
	{}

	double value() const {return m_val;}

	double m_val;
};

const auto kelvin		= boost::units::si::kelvin;
const auto celsius		= boost::units::celsius::degrees;
const auto fahrenheit	= boost::units::fahrenheit::degrees;

class InvalidType {};

inline double photonEnergy_RawSi(const double& fInHz)
{
	return const_hPlanck_rawSiValue * fInHz;
}

inline double photonEnergyVacuumWavelength_RawSi(const double& waveLengthInM)
{
	return photonEnergy_RawSi(const_speedOfLight_rawSiValue / waveLengthInM);
}

template <class FromQuantity_T, class ToUnit_T> inline InvalidType convertTo(const FromQuantity_T&, const ToUnit_T&)
{
	DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(FromQuantity_T,
												DFG_CURRENT_FUNCTION_NAME ": not implemented. Note that e.g. for temperatures "
												"either convertToAbsolute or convertToDifference should be used.");
	return InvalidType();
}

template <class FromQuantity_T, class ToUnit_T> inline InvalidType convertToAbsolute(const FromQuantity_T&, const ToUnit_T&)
{
	DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(FromQuantity_T, DFG_CURRENT_FUNCTION_NAME ": not implemented.");
	return InvalidType();
}

template <class FromQuantity_T, class ToUnit_T> inline InvalidType convertToDifference(const FromQuantity_T&, const ToUnit_T&)
{
	DFG_BUILD_GENERATE_FAILURE_IF_INSTANTIATED(FromQuantity_T, DFG_CURRENT_FUNCTION_NAME ": not implemented.");
	return InvalidType();
}

// Celsius -> Kelvin
auto inline convertToAbsolute(const decltype(1.0 * celsius)& from, const decltype(kelvin)&) -> decltype(from.value() * kelvin)
{
	return (from.value() + 273.15) * kelvin;
}

// Kelvin -> Celsius
auto inline convertToAbsolute(const decltype(1.0 * kelvin)& from, const decltype(celsius)&) -> decltype(from.value() * celsius)
{
	return (from.value() - 273.15) * celsius;
}

auto inline convertToDifference(const decltype(1.0 * celsius)& from, const decltype(kelvin)&) -> decltype(from.value() * kelvin)
{
	return from.value() * kelvin;
}

auto inline convertToDifference(const decltype(1.0 * kelvin)& from, const decltype(celsius)&) -> decltype(from.value() * celsius)
{
	return from.value() * celsius;
}

// Wikipedia http://en.wikipedia.org/wiki/Planck%27s_law
// TODO: test
inline double planckBlackBodySpectralEnergyDensityByFrequency_RawSi(const double TinK, const double fInHz)
{
	const double factor = 8.0 * DFG_SUB_NS_NAME(math)::const_pi * const_hPlanck_rawSiValue / DFG_SUB_NS_NAME(math)::pow3(const_speedOfLight_rawSiValue);
	const double beta = 1.0 / (const_k_B_rawSiValue * TinK);
	return factor * DFG_SUB_NS_NAME(math)::pow3(fInHz) / (exp(photonEnergy_RawSi(fInHz) * beta) - 1);
}

// Wikipedia http://en.wikipedia.org/wiki/Planck%27s_law
// TODO: test
inline double planckBlackBodySpectralEnergyDensityByWaveLength_RawSi(const double TinK, const double wavelengthInM)
{
	const double factor = 8.0 * DFG_SUB_NS_NAME(math)::const_pi * const_hPlanck_rawSiValue * const_speedOfLight_rawSiValue;
	const double beta = 1.0 / (const_k_B_rawSiValue * TinK);
	return factor * (1.0 / DFG_SUB_NS_NAME(math)::pow5(wavelengthInM)) * (1.0 / (exp(photonEnergyVacuumWavelength_RawSi(wavelengthInM) * beta) - 1));
}

}} // module physics

#endif // include guard

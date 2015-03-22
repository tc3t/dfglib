#pragma once

#include "../dfgDefs.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(physics) {

// For more constants, see http://physics.nist.gov/cuu/Constants/index.html.

// Constants as plain value.
const double const_m_u_rawSiValue = 1.660538921e-27; // Atomic mass unit in SI-units. Source: 2010 CODATA recommended values, http://physics.nist.gov/cgi-bin/cuu/Value?u
const double const_k_B_rawSiValue = 1.3806488e-23;	// Boltzmann constant in SI-units. Source: 2010 CODATA recommended values, http://physics.nist.gov/cgi-bin/cuu/Value?k
const double const_hPlanck_rawSiValue = 6.62606957e-34; // Planck constant in SI units. Source: 2010 CODATA recommended values, http://physics.nist.gov/cgi-bin/cuu/Value?h
const double const_hBarPlanck_rawSiValue = 1.054571726e-34; // Planck constant / (2*Pi). Source: 2010 CODATA recommended values, http://physics.nist.gov/cgi-bin/cuu/Value?hbar
const double const_speedOfLight_rawSiValue = 299792458;		// Speed of light. Source: 2010 CODATA recommended values, http://physics.nist.gov/cgi-bin/cuu/Value?c
const double const_stefanBoltzmannConstant_rawSiValue = 5.670373e-8; // Stefan-Boltzmann constant. Source: 2010 CODATA recommended values, http://physics.nist.gov/cgi-bin/cuu/Value?sigma)

const double const_electronMass_rawSiValue = 9.10938291e-31; // Electron mass in SI units. Source: 2010 CODATA recommended values, http://physics.nist.gov/cgi-bin/cuu/Value?me
const double const_electronMass_rawAtomicMassUnitsValue = 5.4857990946e-4; // Electron mass in atomic mass units. Source: 2010 CODATA recommended values, http://physics.nist.gov/cgi-bin/cuu/Value?meu

}} // module physics

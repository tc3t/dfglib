#ifndef DFG_COLOUR_LNRJZKHT
#define DFG_COLOUR_LNRJZKHT

#include "dfgBase.hpp"
#include "colour/defs.hpp"
#include "colour/specRendJw.hpp"
#include "alg.hpp"
#include "cont.hpp"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(colour) {

template <class T>
class DFG_CLASS_NAME(RgbT)
{
public:
	DFG_CLASS_NAME(RgbT)() : r(0), g(0), b(0) {}
	DFG_CLASS_NAME(RgbT)(T rp, T gp, T bp) :
		r(rp), g(gp), b(bp)
	{
	}

	template <class Val_T>
	void multiply(const Val_T mul)
	{
		r *= mul;
		g *= mul;
		b *= mul;
	}

	bool operator==(const DFG_CLASS_NAME(RgbT)& other) const
	{
		return r == other.r && g == other.g && b == other.b;
	}

	T r,g,b;
};

typedef DFG_CLASS_NAME(RgbT)<float> DFG_CLASS_NAME(RgbF);
typedef DFG_CLASS_NAME(RgbT)<double> DFG_CLASS_NAME(RgbD);


// Contains algorithms related to spectrum <-> colour conversions.
class DFG_CLASS_NAME(SpectrumColour)
{
public:
	// TODO: test
	static DFG_CLASS_NAME(RgbD) wavelengthInNmToRgbD(const double wlInNm, ColourSystem cs, const bool bGammaCorrect = true);

	// Note: This is experimental function and is subject to change. 
	static DFG_CLASS_NAME(RgbD) wavelengthInNmToRgbDSimpleGradient_Experimental(const double wlInNm);

	// Note: Sampling rate in wavelength axis is unspecified so if the spectrum has 
	//       peaks, it's recommended to use the overload in which the sampling interval 
	//       is user defined.
	template <class WlInNmToIntensityFunc_T>
	static DFG_CLASS_NAME(RgbD) spectrumToRgbDSimpleNm(WlInNmToIntensityFunc_T wlInNmToIntensityFunc,
										ColourSystem cs,
										const bool bGammaCorrect = true);

	template <class WlRange_T, class WlInNmToIntensityFunc_T>
	static DFG_CLASS_NAME(RgbD) spectrumToRgbDSimpleNm(const WlRange_T& range,
										WlInNmToIntensityFunc_T wlInNmToIntensityFunc,
										ColourSystem cs,
										const bool bGammaCorrect = true);
}; // class SpectrumColour

template <class WlIterable_T, class WlInNmToIntensityFunc_T>
DFG_CLASS_NAME(RgbD) DFG_CLASS_NAME(SpectrumColour)::spectrumToRgbDSimpleNm(const WlIterable_T& wlRangeInNm,
										WlInNmToIntensityFunc_T wlInNmToIntensityFunc,
										ColourSystem cs,
										const bool bGammaCorrect)
{
	const auto end = wlRangeInNm.end();
	double X = 0, Y = 0, Z = 0;
	for(auto iter = wlRangeInNm.begin(); iter != end; ++iter)
	{
		const auto wlInNm = *iter;
		if (wlInNm < srjw::cieColourMatchTableMinimumInNm())
			continue;
		if (wlInNm > srjw::cieColourMatchTableMaximumInNm())
			continue;
		double cieTableIndex = (wlInNm - srjw::cieColourMatchTableMinimumInNm()) / srjw::cieColourMatchTableStepInNm();
		if (cieTableIndex < 0 || cieTableIndex > count(srjw::cie_colour_match) - 1)
		{
			DFG_ASSERT_CORRECTNESS(false);
			continue;
		}

		const auto interpolatedCieColourMatchX = srjw::evaluateCieColourMatchAtFloatIndexNc(cieTableIndex, 0);
		const auto interpolatedCieColourMatchY = srjw::evaluateCieColourMatchAtFloatIndexNc(cieTableIndex, 1);
		const auto interpolatedCieColourMatchZ = srjw::evaluateCieColourMatchAtFloatIndexNc(cieTableIndex, 2);
			
		const auto intensity = wlInNmToIntensityFunc(wlInNm);
		X += intensity * interpolatedCieColourMatchX;
		Y += intensity * interpolatedCieColourMatchY;
		Z += intensity * interpolatedCieColourMatchZ;
	}
	const auto XYZ = (X + Y + Z);
	const auto x = X / XYZ;
	const auto y = Y / XYZ;
	const auto z = Z / XYZ;

	DFG_CLASS_NAME(RgbD) rgb;
	srjw::xyzToRgbWithConstrainAndNorm(cs, x, y, z, rgb.r, rgb.g, rgb.b);
	if (bGammaCorrect)
		gamma_correct_rgb(srjw::colourSystems[cs], rgb.r, rgb.g, rgb.b);
	return rgb;
}

template <class WlInNmToIntensityFunc_T>
DFG_CLASS_NAME(RgbD) DFG_CLASS_NAME(SpectrumColour)::spectrumToRgbDSimpleNm(WlInNmToIntensityFunc_T wlInNmToIntensityFunc,
											  ColourSystem cs,
											  const bool bGammaCorrect)
{
	double x,y,z;
	srjw::spectrum_to_xyz(wlInNmToIntensityFunc, x, y, z);
	DFG_CLASS_NAME(RgbD) rgb;
	srjw::xyzToRgbWithConstrainAndNorm(cs, x ,y, z, rgb.r, rgb.g, rgb.b);
	if (bGammaCorrect)
		gamma_correct_rgb(srjw::colourSystems[cs], rgb.r, rgb.g, rgb.b);
	return rgb;
}

inline DFG_CLASS_NAME(RgbD) DFG_CLASS_NAME(SpectrumColour)::wavelengthInNmToRgbD(const double wlInNm, ColourSystem cs, const bool bGammaCorrect)
{
	std::array<double, 1> wl = { wlInNm };
	const auto intensity = [](const double wlInNm) -> double
							{
								// An ad hoc scaling to make spectrum ends to fade to darkness.
								if (wlInNm < 400 || wlInNm > 700)
									return 0;
								else if (wlInNm < 450)
									return (wlInNm - 400) / (450 - 400);
								else if (wlInNm > 650)
									return 1 - (wlInNm - 650) / (700 - 650);
								else
									return 1;
							};
	auto rgb = spectrumToRgbDSimpleNm(wl, [](DFG_CLASS_NAME(Dummy)) {return 1; }, cs, bGammaCorrect);
	rgb.multiply(intensity(wlInNm)); 
	return rgb;
}

inline DFG_CLASS_NAME(RgbD) DFG_CLASS_NAME(SpectrumColour)::wavelengthInNmToRgbDSimpleGradient_Experimental(const double wlInNm)
{
	// Tables are invented vaguely based on http://en.wikipedia.org/wiki/Spectral_color
	// TODO: Define better values.
	//                                         violet, indigo,  blue, cyan, turquoise, green, yellow, red
	const std::array<double, 9> wavelengths = {   400,    440,   460,  490,       510,   550,    570, 610, 700};
	const std::array<double, 9> reds =        {    16,     75,     0,    0,        64,     0,    255, 255,   0};
	const std::array<double, 9> greens =      {     0,      0,     0,  255,       224,   255,    255,   0,   0};
	const std::array<double, 9> blues =       {    32,    130,   255,  255,       208,     0,      0,   0,   0};

	const auto index = DFG_MODULE_NS(alg)::floatIndexInSorted(wavelengths, wlInNm);
	if (index < 0 || index > (wavelengths.size() - 1))
		return DFG_CLASS_NAME(RgbD)(0, 0, 0);

	const auto r = DFG_MODULE_NS(cont)::getInterpolatedValue(reds, index) / 255.0;
	const auto g = DFG_MODULE_NS(cont)::getInterpolatedValue(greens, index) / 255.0;
	const auto b = DFG_MODULE_NS(cont)::getInterpolatedValue(blues, index) / 255.0;

	return DFG_CLASS_NAME(RgbD)(r, g, b);
}

}} // module colour

#endif


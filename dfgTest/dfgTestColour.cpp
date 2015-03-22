#include "stdafx.h"

#include <dfg/iterableForRange.hpp>
#include <dfg/colour.hpp>
#include <dfg/physics.hpp>
#include <array>
#include <type_traits>

#ifdef _MSC_VER // In GCC 4.8.0 'has_trivial_assign' is not a member of 'std'
    DFG_STATIC_ASSERT(std::has_trivial_assign<DFG_ROOT_NS::DFG_SUB_NS_NAME(colour)::DFG_CLASS_NAME(RgbF)>::value == true, "Expecting RGB-class to have trivial assign");
    DFG_STATIC_ASSERT(std::has_trivial_assign<DFG_ROOT_NS::DFG_SUB_NS_NAME(colour)::DFG_CLASS_NAME(RgbD)>::value == true, "Expecting RGB-class to have trivial assign");
#endif

TEST(DfgColour, SpectrumToRgb)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_ROOT_NS::DFG_SUB_NS_NAME(colour);

    std::vector<double> wlVals;
    std::vector<double> wlVals2;
    for(double i = srjw::cieColourMatchTableMinimumInNm();
                i <= srjw::cieColourMatchTableMaximumInNm();
                i = i + srjw::cieColourMatchTableStepInNm()
        )
    {
        wlVals.push_back(i);
        wlVals2.push_back(i + 0.5*srjw::cieColourMatchTableStepInNm());
    }

    const auto wlVals3 = DFG_CLASS_NAME(IterableForRangeSimple)<double>(srjw::cieColourMatchTableMinimumInNm(),
                                                    srjw::cieColourMatchTableMaximumInNm() + 0.5 * srjw::cieColourMatchTableStepInNm(),
                                                    srjw::cieColourMatchTableStepInNm());

    const auto spectrum = [](const double T, const double wl)
                        {
                            return DFG_ROOT_NS::DFG_SUB_NS_NAME(physics)::planckBlackBodySpectralEnergyDensityByWaveLength_RawSi(T, wl*1e-9);
                        };

    std::vector<DFG_CLASS_NAME(RgbD)> rgbs;
    const auto bGammaCorrection = false;
    for(double T = 1000; T <= 10000; T += 500)
    {
        const auto func = [&](const double wl) {return spectrum(T, wl);};
        const auto rgb = DFG_CLASS_NAME(SpectrumColour)::spectrumToRgbDSimpleNm(func, ColourSystemSMPTEsystemSrjw, bGammaCorrection);
        const auto rgb2 = DFG_CLASS_NAME(SpectrumColour)::spectrumToRgbDSimpleNm(wlVals, func, ColourSystemSMPTEsystemSrjw, bGammaCorrection);
        const auto rgb3 = DFG_CLASS_NAME(SpectrumColour)::spectrumToRgbDSimpleNm(wlVals2, func, ColourSystemSMPTEsystemSrjw, bGammaCorrection);
        const auto rgb4 = DFG_CLASS_NAME(SpectrumColour)::spectrumToRgbDSimpleNm(wlVals3, func, ColourSystemSMPTEsystemSrjw, bGammaCorrection);
        rgbs.push_back(rgb);
        EXPECT_EQ(rgb, rgb2);
        EXPECT_EQ(rgb, rgb4);

        // Expect array with values in between of cie table values to be approximately the same.
        EXPECT_NEAR(rgb.r, rgb3.r, 0.001);
        EXPECT_NEAR(rgb.g, rgb3.g, 0.001);
        EXPECT_NEAR(rgb.b, rgb3.b, 0.001);
    }

    // Expected value are from the original example program of specrend.c
    std::array<DFG_CLASS_NAME(RgbD), 19> expectedRgbs =
    {
        DFG_CLASS_NAME(RgbD)(1.000, 0.007, 0.000), // 1000 K
        DFG_CLASS_NAME(RgbD)(1.000, 0.126, 0.000), // 1500 K
        DFG_CLASS_NAME(RgbD)(1.000, 0.234, 0.010), // 2000 K
        DFG_CLASS_NAME(RgbD)(1.000, 0.349, 0.067), // 2500 K
        DFG_CLASS_NAME(RgbD)(1.000, 0.454, 0.151), // 3000 K
        DFG_CLASS_NAME(RgbD)(1.000, 0.549, 0.254), // 3500 K
        DFG_CLASS_NAME(RgbD)(1.000, 0.635, 0.370), // 4000 K
        DFG_CLASS_NAME(RgbD)(1.000, 0.710, 0.493), // 4500 K
        DFG_CLASS_NAME(RgbD)(1.000, 0.778, 0.620), // 5000 K
        DFG_CLASS_NAME(RgbD)(1.000, 0.837, 0.746), // 5500 K
        DFG_CLASS_NAME(RgbD)(1.000, 0.890, 0.869), // 6000 K
        DFG_CLASS_NAME(RgbD)(1.000, 0.937, 0.988), // 6500 K
        DFG_CLASS_NAME(RgbD)(0.907, 0.888, 1.000), // 7000 K
        DFG_CLASS_NAME(RgbD)(0.827, 0.839, 1.000), // 7500 K
        DFG_CLASS_NAME(RgbD)(0.762, 0.800, 1.000), // 8000 K
        DFG_CLASS_NAME(RgbD)(0.711, 0.766, 1.000), // 8500 K
        DFG_CLASS_NAME(RgbD)(0.668, 0.738, 1.000), // 9000 K
        DFG_CLASS_NAME(RgbD)(0.632, 0.714, 1.000), // 9500 K
        DFG_CLASS_NAME(RgbD)(0.602, 0.693, 1.000) // 10000 K
    };

    EXPECT_EQ(rgbs.size(), expectedRgbs.size());

    for(size_t i = 0; i<Min(rgbs.size(), expectedRgbs.size()); ++i)
    {
        // There are some differences e.g. due to different constant values.
        EXPECT_NEAR(rgbs[i].r, expectedRgbs[i].r, 0.001);
        EXPECT_NEAR(rgbs[i].g, expectedRgbs[i].g, 0.001);
        EXPECT_NEAR(rgbs[i].b, expectedRgbs[i].b, 0.001);
    }
}

TEST(DfgColour, wavelengthInNmToRgbD)
{
}


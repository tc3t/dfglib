#include "stdafx.h"

#if (DFGTEST_BUILD_MODULE_DEFAULT == 1)

#include <dfg/physics.hpp>

TEST(dfgPhysics, misc)
{

}

TEST(dfgPhysics, convert)
{
	using namespace DFG_ROOT_NS::DFG_SUB_NS_NAME(physics);

	const auto CtoK = convertToAbsolute(-273.15 * celsius, kelvin);
	EXPECT_EQ(0.0 * kelvin, CtoK);
	EXPECT_EQ(0.0, CtoK.value());

	const auto CtoK2 = convertToAbsolute(1.0 * celsius, kelvin);
	EXPECT_EQ(274.15 * kelvin, CtoK2);
	EXPECT_EQ(274.15, CtoK2.value());

	const auto KtoC = convertToAbsolute(0.0 * kelvin, celsius);
	EXPECT_EQ(-273.15 * celsius, KtoC);
	EXPECT_EQ(-273.15, KtoC.value());

	const auto KtoC2 = convertToAbsolute(274.15 * kelvin, celsius);
	EXPECT_EQ(1.0 * celsius, KtoC2);
	EXPECT_EQ(1.0, KtoC2.value());

	const auto CDiffToKDiff = convertToDifference(-20.0 * celsius, kelvin);
	EXPECT_EQ(-20 * kelvin, CDiffToKDiff);
	EXPECT_EQ(-20, CDiffToKDiff.value());

	const auto KDiffToCDiff = convertToDifference(-20.0 * kelvin, celsius);
	EXPECT_EQ(-20.0 * celsius, KDiffToCDiff);
	EXPECT_EQ(-20.0, KDiffToCDiff.value());
}

#endif

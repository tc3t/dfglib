#include <stdafx.h>
#include <dfg/bits.hpp>

TEST(dfgBits, BitTest)
{
	using namespace DFG_MODULE_NS(bits);

	enum TempEnum { Zero, One, Two, Three, Four };
	// 
	EXPECT_EQ(bitTest(15, Zero), true);
	EXPECT_EQ(bitTest(15, 1), true);
	EXPECT_EQ(bitTest(15, One), bitTest(7, 1));
	EXPECT_EQ(bitTest(15, Four), false);
	EXPECT_EQ(bitTest(~size_t(0), 31), true);
}

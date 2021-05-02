#include "stdafx.h"
#include <dfg/staticMap.hpp>

namespace
{
static const char* names[] = {"ten", "twelve", "fourty five"};
enum {Ten = 10, Twenty = 20, FourtyFive = 45};

DFG_BEGIN_STATIC_MAP(mapToName, names)
	DFG_ADD_MAPPING(mapToName, Ten, 0);
	DFG_ADD_MAPPING(mapToName, Twenty, 1);
	DFG_ADD_MAPPING(mapToName, FourtyFive, 2);
DFG_END_STATIC_MAP(mapToName)
}

TEST(DfgMisc, StaticMap)
{
	// Note: Pointer comparison in EXPECT_EQ is OK here.
	EXPECT_EQ(names[mapToName<Ten>::index], names[0]);
	EXPECT_EQ(names[mapToName<Twenty>::index], names[1]);
	EXPECT_EQ(names[mapToName<FourtyFive>::index], names[2]);
}

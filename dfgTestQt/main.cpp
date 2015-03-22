#include <dfg/dfgDefs.hpp>

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
#include <gtest/gtest.h>
#include <QApplication>
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

#include "stdafx.h"
#include "../externals/gtest/gtest.h"

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    // Code below can be used to run only specific test.
    //::testing::GTEST_FLAG(filter) = "dfgCont.VectorInsertPerformance";
    auto rv = RUN_ALL_TESTS();
    return rv;
}

#include "stdafx.h"
#include "../externals/gtest/gtest.h"

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    auto rv = RUN_ALL_TESTS();
    return rv;
}

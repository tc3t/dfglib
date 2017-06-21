#include "stdafx.h"
#include <dfg/hash.hpp>
#include <dfg/str.hpp>
#include <vector>

#ifdef _WIN32
TEST(dfgHash, HashCreator)
{
	using namespace DFG_ROOT_NS;
	using namespace DFG_MODULE_NS(hash);
	using namespace DFG_MODULE_NS(str);
	
	DFG_DEFINE_STRING_LITERAL_C(szTest1, "BOOST_AUTO_TEST_CASE( TestHashCreator )");
	DFG_DEFINE_STRING_LITERAL_C(szTest1_Md5_expected, "8c6af274c06f1e5685ad0e2808eda0f5");
	DFG_DEFINE_STRING_LITERAL_C(szTest1_Sha1_expected, "82ba8da53c54e6ec22a0fe003deea088bf2f91e6");

	auto szHashMd5 = DFG_CLASS_NAME(HashCreator)(HashTypeMd5).process(szTest1, DFG_COUNTOF_CSL(szTest1)).toStr<char>();
	DFG_MODULE_NS(str)::toLower(szHashMd5);
	auto szHashSha1 = DFG_CLASS_NAME(HashCreator)(HashTypeSha1).process(szTest1, DFG_COUNTOF_CSL(szTest1)).toStr<char>();
	DFG_MODULE_NS(str)::toLower(szHashSha1);

	EXPECT_EQ(strCmp(szHashMd5, szTest1_Md5_expected), 0);
	EXPECT_EQ(strCmp(szHashSha1, szTest1_Sha1_expected), 0);
}
#endif

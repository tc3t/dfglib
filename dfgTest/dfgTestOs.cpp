#include <stdafx.h>
#include <dfg/os.hpp>
#include <dfg/str.hpp>
#include <dfg/io.hpp>
#include <dfg/os/memoryMappedFile.hpp>

TEST(DfgOs, pathFindExtension)
{
	using namespace DFG_MODULE_NS(os);
	using namespace DFG_MODULE_NS(str);
	EXPECT_EQ(strCmp(".txt", pathFindExtension("c:/testFolder/test.txt")), 0);
	EXPECT_EQ(strCmp(".txt", pathFindExtension("c:\\testFolder\\test.txt")), 0);
	EXPECT_EQ(strCmp(L".", pathFindExtension(L"c:/testFolder/test.")), 0);
	EXPECT_EQ(strCmp(L".", pathFindExtension(L"c:\\testFolder\\test.")), 0);
	EXPECT_EQ(strCmp(L"", pathFindExtension(L"c:/testFolder/test")), 0);
	EXPECT_EQ(strCmp(L".extension", pathFindExtension(L"z:/testFolder/test/dfg/dfgvcbcv/bcvbb/.extension")), 0);
	EXPECT_EQ(strCmp(L".extension", pathFindExtension(L"z:\\testFolder\\test\\dfg\\dfgvcbcv\\bcvbb\\.extension")), 0);
	EXPECT_NE(strCmp(L".extension", pathFindExtension(L"z:\\testFolder\\test\\dfg\\dfgvcbcv\\bcvbb\\.extensionA")), 0);
}

TEST(DfgOs, MemoryMappedFile)
{
	using namespace DFG_MODULE_NS(os);
	using namespace DFG_MODULE_NS(io);
	DFG_CLASS_NAME(MemoryMappedFile) mmf;
	mmf.open("testfiles/matrix_3x3.txt");
	EXPECT_TRUE(mmf.is_open());
	EXPECT_TRUE(mmf.size() == 62);
	std::vector<char> bytes(mmf.begin(), mmf.end());
	const auto vec2 = fileToVector("testfiles/matrix_3x3.txt");
	EXPECT_EQ(bytes, vec2);
}

#include <stdafx.h>
#include <dfg/chartsAll.hpp>
#include <dfg/cont/Vector.hpp>
#include <dfg/alg.hpp>

namespace
{
    auto badOperationAxis = ::DFG_MODULE_NS(charts)::ChartEntryOperation::axisIndex_invalid;
    void badOperation(::DFG_MODULE_NS(charts)::ChartEntryOperation& op, ::DFG_MODULE_NS(charts)::ChartOperationPipeData& arg)
    {
        using namespace ::DFG_MODULE_NS(charts);
        op.privFilterBySingle(arg, badOperationAxis, [](::DFG_ROOT_NS::Dummy) { return true; });
    }

    // Operation that takes value vectors as input and outputs string vectors.
    void operation_stringVectorOutput(::DFG_MODULE_NS(charts)::ChartEntryOperation& op, ::DFG_MODULE_NS(charts)::ChartOperationPipeData& arg)
    {
        using namespace ::DFG_ROOT_NS;
        using namespace ::DFG_MODULE_NS(alg);
        using namespace ::DFG_MODULE_NS(charts);
        using namespace ::DFG_MODULE_NS(cont);
        // Expecting two input vectors and creating three string vector outputs: [toStr(xi)], [toStr(yi)] and [toStr(xi+yi)]
        auto pXvals = arg.valuesByIndex(0);
        auto pYvals = arg.valuesByIndex(1);
        if (!pXvals || !pYvals)
        {
            op.setError(ChartEntryOperation::error_missingInput);
            return;
        }
        if (pXvals->size() != pYvals->size())
        {
            op.setError(ChartEntryOperation::error_pipeDataVectorSizeMismatch);
            return;
        }
        const auto nCount = pXvals->size();
        char szBuffer[64] = "";
        EXPECT_EQ(2, arg.vectorCount());
        arg.createStringVectors(3, nCount);
        arg.fillStringVector(0, *pXvals, [&](const double d) { DFG_MODULE_NS(str)::toStr(d, szBuffer); return SzPtrAscii(szBuffer); });
        arg.fillStringVector(1, *pYvals, [&](const double d) { DFG_MODULE_NS(str)::toStr(d, szBuffer); return SzPtrAscii(szBuffer); });
        arg.fillStringVector(2, *pXvals, *pYvals, [&](const double x, const double y) { DFG_MODULE_NS(str)::toStr(x + y, szBuffer); return SzPtrAscii(szBuffer); });
        arg.setStringVectorsAsData();
        EXPECT_EQ(3, arg.vectorCount());
    }

    // Operation that takes 3 string vectors as input and outputs two value vectors
    void operation_stringVectorInput(::DFG_MODULE_NS(charts)::ChartEntryOperation& op, ::DFG_MODULE_NS(charts)::ChartOperationPipeData& arg)
    {
        using namespace DFG_ROOT_NS;
        using namespace ::DFG_MODULE_NS(charts);
        if (arg.vectorCount() != 3)
        {
            op.setError(ChartEntryOperation::error_unexpectedInputVectorCount);
            return;
        }
        auto pStrings0 = arg.stringsByIndex(0);
        auto pStrings1 = arg.stringsByIndex(1);
        auto pStrings2 = arg.stringsByIndex(2);
        if (!pStrings0 || !pStrings1 || !pStrings2)
        {
            op.setError(ChartEntryOperation::error_unexpectedInputVectorTypes);
            return;
        }

        auto pValues0 = arg.editableValuesByIndex(0);
        auto pValues1 = arg.editableValuesByIndex(1);

        if (!pValues0 || !pValues1)
        {
            op.setError(ChartEntryOperation::error_unableToCreateDataVectors);
            return;
        }

        std::transform(pStrings1->begin(), pStrings1->end(), std::back_inserter(*pValues0), [](const StringViewUtf8 sv) { return ::DFG_MODULE_NS(str)::strTo<double>(sv); });
        std::transform(pStrings2->begin(), pStrings2->end(), std::back_inserter(*pValues1), [](const StringViewUtf8 sv) { return ::DFG_MODULE_NS(str)::strTo<double>(sv); });

        arg.setDataRefs(ChartOperationPipeData::DataVectorRef(pValues0), ChartOperationPipeData::DataVectorRef(pValues1));
    }

    // Transforms [x], [y] -> [x], [x+y]
    void operation_yToXplusY(::DFG_MODULE_NS(charts)::ChartEntryOperation& op, ::DFG_MODULE_NS(charts)::ChartOperationPipeData& arg)
    {
        using namespace DFG_ROOT_NS;
        using namespace ::DFG_MODULE_NS(charts);
        if (arg.vectorCount() != 2)
        {
            op.setError(ChartEntryOperation::error_unexpectedInputVectorCount);
            return;
        }
        auto pXvals = arg.constValuesByIndex(0);
        auto pYvals = arg.valuesByIndex(1);
        if (!pXvals || !pYvals)
        {
            op.setError(ChartEntryOperation::error_missingInput);
            return;
        }
        if (pXvals->size() != pYvals->size())
        {
            op.setError(ChartEntryOperation::error_pipeDataVectorSizeMismatch);
            return;
        }
        // [y] -> [x + y]
        std::transform(pXvals->cbegin(), pXvals->cend(), pYvals->begin(), pYvals->begin(), [](const double d0, const double d1) { return d0 + d1; });
    }
} // unnamed namespace


TEST(dfgCharts, operations)
{
    using namespace ::DFG_MODULE_NS(alg);
    using namespace ::DFG_MODULE_NS(charts);

    {
        ChartEntryOperation op;
        ChartOperationPipeData pipeArg(nullptr, nullptr);
        EXPECT_FALSE(op);
        EXPECT_FALSE(op.hasErrors());
        EXPECT_EQ(ChartEntryOperation::axisIndex_x, ChartEntryOperation::axisStrToIndex(DFG_UTF8("x")));
        EXPECT_EQ(ChartEntryOperation::axisIndex_y, ChartEntryOperation::axisStrToIndex(DFG_UTF8("y")));
        EXPECT_EQ(ChartEntryOperation::axisIndex_invalid, ChartEntryOperation::axisStrToIndex(DFG_UTF8("a")));
        op(pipeArg);
        EXPECT_TRUE(op.hasError(ChartEntryOperation::error_noCallable));
    }

    // BadOperation: missing input
    {
        ChartEntryOperation op(&badOperation);
        badOperationAxis = ChartEntryOperation::axisIndex_x;
        ChartOperationPipeData pipeArg(nullptr, nullptr);
        op(pipeArg);
        EXPECT_TRUE(op.hasError(ChartEntryOperation::error_missingInput));
        badOperationAxis = ChartEntryOperation::axisIndex_invalid;
    }

    // BadOperation: missing input because badOperationAxis is invalid.
    {
        ChartEntryOperation op(&badOperation);
        ValueVectorD x, y;
        ChartOperationPipeData pipeArg(&x, &y);
        op(pipeArg);
        EXPECT_TRUE(op.hasError(ChartEntryOperation::error_missingInput));
    }

    // operation_stringVectorInput / operation_stringVectorInput
    {
        ChartEntryOperation op1(&operation_stringVectorOutput);
        ValueVectorD x(10), y(10), xySum(10);
        generateAdjacent(x, 0, 1);
        generateAdjacent(y, 100, 1);
        std::transform(x.begin(), x.end(), y.begin(), xySum.begin(), [](double x, double y) { return x + y; });
        const auto yOrig = y;
        ChartOperationPipeData pipeArg(&x, &y);
        op1(pipeArg);
        EXPECT_FALSE(op1.hasErrors());
        EXPECT_EQ(3, pipeArg.vectorCount());
        EXPECT_NE(nullptr, pipeArg.stringsByIndex(0));
        EXPECT_NE(nullptr, pipeArg.stringsByIndex(1));
        EXPECT_NE(nullptr, pipeArg.stringsByIndex(2));

        ChartEntryOperation op2(&operation_stringVectorInput);
        op2(pipeArg);
        EXPECT_FALSE(op2.hasErrors());
        EXPECT_EQ(2, pipeArg.vectorCount());
        ASSERT_NE(nullptr, pipeArg.valuesByIndex(0));
        ASSERT_NE(nullptr, pipeArg.valuesByIndex(1));
        EXPECT_EQ(yOrig, *pipeArg.valuesByIndex(0));
        EXPECT_EQ(xySum, *pipeArg.valuesByIndex(1));
    }

    // Testing read only input
    {
        const ValueVectorD x = { 1, 2, 3 };
        const ValueVectorD y = { 4, 5, 6 };
        ChartEntryOperation op(&operation_yToXplusY);
        ChartOperationPipeData pipeArg(&x, &y);
        op(pipeArg);
        EXPECT_FALSE(op.hasErrors());
        ASSERT_NE(nullptr, pipeArg.constValuesByIndex(0));
        ASSERT_NE(nullptr, pipeArg.constValuesByIndex(1));
        EXPECT_EQ(ValueVectorD({ 1, 2, 3 }), *pipeArg.constValuesByIndex(0));
        EXPECT_EQ(&x, pipeArg.constValuesByIndex(0)); // To test that operation didn't make copies of const input that didn't need to be modified.
        EXPECT_EQ(ValueVectorD({ 5, 7, 9 }), *pipeArg.constValuesByIndex(1));
        EXPECT_NE(&y, pipeArg.constValuesByIndex(1)); // To test that operation made copy of const input which needed editing.
    }
}

TEST(dfgCharts, operations_passWindow)
{
    using namespace ::DFG_ROOT_NS;
    using namespace ::DFG_MODULE_NS(alg);
    using namespace ::DFG_MODULE_NS(charts);
    using namespace ::DFG_MODULE_NS(cont);
    using namespace ::DFG_MODULE_NS(str);

    const auto testDateToDouble = [](const StringViewC& sv)
    {
        const int y = strTo<unsigned int>(sv.substr_startCount(0, 4));
        const int m = strTo<unsigned int>(sv.substr_startCount(5, 2));
        const int d = strTo<unsigned int>(sv.substr_startCount(8, 2));
        return static_cast<double>((y << 16) + (m << 8) + d);
    };

    const auto stringToDoubleConverter = [&](const StringViewSzUtf8 svUtf8)
    {
        auto sv = svUtf8.asUntypedView();
        if (sv.size() == 10 && sv[4] == '-')
            return testDateToDouble(sv);
        else
            return strTo<double>(sv);
    };

    ChartEntryOperationManager opManager;
    opManager.setStringToDoubleConverter(stringToDoubleConverter);
    opManager.add<operations::PassWindowOperation>();

    // Basic x-axis passwindow
    {
        Vector<ChartEntryOperation> operations;
        operations.push_back(opManager.createOperation(DFG_ASCII("passWindow(x, 20, 45)")));
        ValueVectorD valsX(100);
        ValueVectorD valsY(100);
        generateAdjacent(valsX, 0, 1);
        generateAdjacent(valsY, 500, 1);
        ChartOperationPipeData arg(&valsX, &valsY);
        operations.front()(arg);
        EXPECT_EQ(26, valsX.size());
        EXPECT_EQ(26, valsY.size());
        EXPECT_EQ(20, valsX.front());
        EXPECT_EQ(45, valsX.back());
        EXPECT_EQ(520, valsY.front());
        EXPECT_EQ(545, valsY.back());
    }

    // Basic y-axis passwindow
    {
        Vector<ChartEntryOperation> operations;
        operations.push_back(opManager.createOperation(DFG_ASCII("passWindow(y, 5, 10)")));
        ValueVectorD valsY = { 8, -1, 12, 5, 7 };
        ValueVectorD valsX(valsY.size());
        generateAdjacent(valsX, 0, 1);
        ChartOperationPipeData arg(&valsX, &valsY);
        operations.front()(arg);
        EXPECT_EQ(ValueVectorD({ 0, 3, 4 }), valsX);
        EXPECT_EQ(ValueVectorD({ 8, 5, 7 }), valsY);
    }

    // Custom string-to-double parser
    {
        Vector<ChartEntryOperation> operations;
        operations.push_back(opManager.createOperation(DFG_ASCII("passWindow(y, 2020-08-25, 2020-08-27)")));
        ValueVectorD valsY = { testDateToDouble("2020-08-24"), testDateToDouble("2020-08-25"), testDateToDouble("2020-08-26"), testDateToDouble("2020-08-28") };
        ValueVectorD valsX(valsY.size());
        generateAdjacent(valsX, 0, 1);
        ChartOperationPipeData arg(&valsX, &valsY);
        operations.front()(arg);
        EXPECT_EQ(ValueVectorD({ 1, 2 }), valsX);
        EXPECT_EQ(ValueVectorD({ testDateToDouble("2020-08-25"), testDateToDouble("2020-08-26") }), valsY);
    }

    // [Const double], [const double] input
    {
        auto op = opManager.createOperation(DFG_ASCII("passWindow(x, 1.5, 2.5)"));
        const ValueVectorD x = { 1, 2, 3 };
        const ValueVectorD y = { 4, 5, 6 };
        ChartOperationPipeData pipeArg(&x, &y);
        op(pipeArg);
        EXPECT_FALSE(op.hasErrors());
        ASSERT_NE(nullptr, pipeArg.constValuesByIndex(0));
        ASSERT_NE(nullptr, pipeArg.constValuesByIndex(1));
        EXPECT_EQ(ValueVectorD({ 2 }), *pipeArg.constValuesByIndex(0));
        EXPECT_EQ(ValueVectorD({ 5 }), *pipeArg.constValuesByIndex(1));
    }

    // [Const string], [const double] input
    {
        auto op = opManager.createOperation(DFG_ASCII("passWindow(y, 1.5, 2.5)"));
        using StringVector = ChartOperationPipeData::StringVector;
        const StringVector x = { StringUtf8(DFG_UTF8("a")), StringUtf8(DFG_UTF8("b")), StringUtf8(DFG_UTF8("c")) };
        const ValueVectorD y = { 1, 2, 3 };
        ChartOperationPipeData pipeArg(&x, &y);
        op(pipeArg);
        EXPECT_FALSE(op.hasErrors());
        ASSERT_NE(nullptr, pipeArg.constStringsByIndex(0));
        ASSERT_NE(nullptr, pipeArg.constValuesByIndex(1));
        EXPECT_EQ(StringVector({ StringUtf8(DFG_UTF8("b")) }), *pipeArg.constStringsByIndex(0));
        EXPECT_EQ(ValueVectorD({ 2 }), *pipeArg.constValuesByIndex(1));
    }

    // [string], [double] input
    {
        auto op = opManager.createOperation(DFG_ASCII("passWindow(y, 1.5, 2.5)"));
        using StringVector = ChartOperationPipeData::StringVector;
        StringVector x = { StringUtf8(DFG_UTF8("a")), StringUtf8(DFG_UTF8("b")), StringUtf8(DFG_UTF8("c")) };
        ValueVectorD y = { 1, 2, 3 };
        ChartOperationPipeData pipeArg(&x, &y);
        op(pipeArg);
        EXPECT_FALSE(op.hasErrors());
        ASSERT_NE(nullptr, pipeArg.constStringsByIndex(0));
        ASSERT_NE(nullptr, pipeArg.constValuesByIndex(1));
        EXPECT_EQ(StringVector({ StringUtf8(DFG_UTF8("b")) }), *pipeArg.constStringsByIndex(0));
        EXPECT_EQ(ValueVectorD({ 2 }), *pipeArg.constValuesByIndex(1));
        EXPECT_EQ(&x, pipeArg.constStringsByIndex(0));
        EXPECT_EQ(&y, pipeArg.constValuesByIndex(1));
    }

    // Missing input
    {
        auto op = opManager.createOperation(DFG_ASCII("passWindow(x, 20, 45)"));
        ValueVectorD valsY(100);
        ChartOperationPipeData arg(nullptr, &valsY);
        op(arg);
        EXPECT_TRUE(op.hasError(ChartEntryOperation::error_missingInput));
    }

    // Testing error handling
    {
        auto op = opManager.createOperation(DFG_ASCII("passWindow(z, 5, 10)"));
        EXPECT_FALSE(op); // op is expected to evalute to false since axis argument is invalid.
        EXPECT_TRUE(op.hasError(ChartEntryOperation::error_badCreationArgs));
    }
}

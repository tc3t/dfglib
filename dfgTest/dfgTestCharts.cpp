#include "stdafx.h"

#if (DFGTEST_BUILD_MODULE_DEFAULT == 1)

#include <dfg/chartsAll.hpp>
#include <dfg/cont/Vector.hpp>
#include <dfg/cont/SetVector.hpp>
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
        EXPECT_EQ(static_cast<int>(ChartEntryOperation::axisIndex_x), ChartEntryOperation::axisStrToIndex(DFG_UTF8("x")));
        EXPECT_EQ(static_cast<int>(ChartEntryOperation::axisIndex_y), ChartEntryOperation::axisStrToIndex(DFG_UTF8("y")));
        EXPECT_EQ(static_cast<int>(ChartEntryOperation::axisIndex_invalid), ChartEntryOperation::axisStrToIndex(DFG_UTF8("a")));
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
        ChartEntryOperationList operations;
        operations.push_back(opManager.createOperation(DFG_ASCII("passWindow(x, 20, 45)")));
        ValueVectorD valsX(100);
        ValueVectorD valsY(100);
        generateAdjacent(valsX, 0, 1);
        generateAdjacent(valsY, 500, 1);
        ChartOperationPipeData arg(&valsX, &valsY);
        operations.executeAll(arg);
        EXPECT_EQ(26, valsX.size());
        EXPECT_EQ(26, valsY.size());
        EXPECT_EQ(20, valsX.front());
        EXPECT_EQ(45, valsX.back());
        EXPECT_EQ(520, valsY.front());
        EXPECT_EQ(545, valsY.back());
    }

    // Basic y-axis passwindow
    {
        ChartEntryOperationList operations;
        operations.push_back(opManager.createOperation(DFG_ASCII("passWindow(y, 5, 10)")));
        ValueVectorD valsY = { 8, -1, 12, 5, 7 };
        ValueVectorD valsX(valsY.size());
        generateAdjacent(valsX, 0, 1);
        ChartOperationPipeData arg(&valsX, &valsY);
        operations.executeAll(arg);
        EXPECT_EQ(ValueVectorD({ 0, 3, 4 }), valsX);
        EXPECT_EQ(ValueVectorD({ 8, 5, 7 }), valsY);
    }

    // Custom string-to-double parser
    {
        ChartEntryOperationList operations;
        operations.push_back(opManager.createOperation(DFG_ASCII("passWindow(y, 2020-08-25, 2020-08-27)")));
        ValueVectorD valsY = { testDateToDouble("2020-08-24"), testDateToDouble("2020-08-25"), testDateToDouble("2020-08-26"), testDateToDouble("2020-08-28") };
        ValueVectorD valsX(valsY.size());
        generateAdjacent(valsX, 0, 1);
        ChartOperationPipeData arg(&valsX, &valsY);
        operations.executeAll(arg);
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

    // Testing that bad filter vector results to output where all vectors are empty.
    {
        // Defining a passWindow for x-axis and trying to use it to string-vector.
        auto op = opManager.createOperation(DFG_ASCII("passWindow(x, 1.5, 2.5)"));
        using StringVector = ChartOperationPipeData::StringVector;
        StringVector x = { StringUtf8(DFG_UTF8("a")), StringUtf8(DFG_UTF8("b")), StringUtf8(DFG_UTF8("c")) };
        ValueVectorD y = { 1, 2, 3 };
        ChartOperationPipeData pipeArg(&x, &y);
        op(pipeArg);
        EXPECT_TRUE(op.hasErrors());
        EXPECT_TRUE(op.hasError(ChartEntryOperation::error_unexpectedInputVectorTypes));
        EXPECT_EQ(2, pipeArg.vectorCount());
        ASSERT_NE(nullptr, pipeArg.constStringsByIndex(0));
        ASSERT_NE(nullptr, pipeArg.constValuesByIndex(1));
        EXPECT_EQ(0, pipeArg.constStringsByIndex(0)->size());
        EXPECT_EQ(0, pipeArg.constValuesByIndex(1)->size());
    }

    // Testing -inf, inf handling in arguments
    {
        auto op = opManager.createOperation(DFG_ASCII("passWindow(x, -inf, inf)"));
        ASSERT_EQ(3, op.argCount());
        EXPECT_EQ(-std::numeric_limits<double>::infinity(), op.argAsDouble(1));
        EXPECT_EQ(std::numeric_limits<double>::infinity(), op.argAsDouble(2));
    }
}

TEST(dfgCharts, operations_blockWindow)
{
    using namespace ::DFG_ROOT_NS;
    using namespace ::DFG_MODULE_NS(alg);
    using namespace ::DFG_MODULE_NS(charts);
    using namespace ::DFG_MODULE_NS(cont);
    using namespace ::DFG_MODULE_NS(str);

    ChartEntryOperationManager opManager;

    // Basic x-axis blockwindow
    {
        ChartEntryOperationList operations;
        operations.push_back(opManager.createOperation(DFG_ASCII("blockWindow(x, 2, 5)")));
        ValueVectorD valsX({1, 2, 3, 4, 5, 6});
        ValueVectorD valsY({9, 8, 7, 6, 5, 4});
        ChartOperationPipeData arg(&valsX, &valsY);
        operations.executeAll(arg);
        EXPECT_EQ(2, valsX.size());
        EXPECT_EQ(2, valsY.size());
        EXPECT_EQ(ValueVectorD({1, 6}), valsX);
        EXPECT_EQ(ValueVectorD({9, 4}), valsY);
    }

    // Basic y-axis blockwindow
    {
        ChartEntryOperationList operations;
        operations.push_back(opManager.createOperation(DFG_ASCII("blockWindow(y, 2, 5)")));
        ValueVectorD valsX({ 1, 2, 3, 4, 5, 6 });
        ValueVectorD valsY({ 9, 8, 7, 6, 5, 4 });
        ChartOperationPipeData arg(&valsX, &valsY);
        operations.executeAll(arg);
        EXPECT_EQ(4, valsX.size());
        EXPECT_EQ(4, valsY.size());
        EXPECT_EQ(ValueVectorD({ 1, 2, 3, 4 }), valsX);
        EXPECT_EQ(ValueVectorD({ 9, 8, 7, 6 }), valsY);
    }

    // Multiple blockwindows
    {
        ChartEntryOperationList operations;
        operations.push_back(opManager.createOperation(DFG_ASCII("blockWindow(x, 5, 6)")));
        operations.push_back(opManager.createOperation(DFG_ASCII("blockWindow(y, 8, 9)")));
        operations.push_back(opManager.createOperation(DFG_ASCII("blockWindow(y, 7, 7)")));
        ValueVectorD valsX({ 1, 2, 3, 4, 5, 6 });
        ValueVectorD valsY({ 9, 8, 7, 6, 5, 4 });
        ChartOperationPipeData arg(&valsX, &valsY);
        operations.executeAll(arg);
        EXPECT_EQ(1, valsX.size());
        EXPECT_EQ(1, valsY.size());
        EXPECT_EQ(ValueVectorD({ 4 }), valsX);
        EXPECT_EQ(ValueVectorD({ 6 }), valsY);
    }
}

TEST(dfgCharts, operations_smoothing_indexNb)
{
    using namespace ::DFG_ROOT_NS;
    using namespace ::DFG_MODULE_NS(charts);

    ChartEntryOperationManager opManager;

    // Basic test with default arguments
    {
        ChartEntryOperationList operations;
        operations.push_back(opManager.createOperation(DFG_ASCII("smoothing_indexNb()")));
        const ValueVectorD valsX{1, 2, 10};
        ValueVectorD valsY{50, 60, 100};
        ChartOperationPipeData arg(&valsX, &valsY);
        operations.executeAll(arg);
        EXPECT_EQ(ValueVectorD({55, 70, 80}), valsY);
        EXPECT_EQ(&valsX, arg.constValuesByIndex(0)); // Testing that const input hasn't been touched
        EXPECT_EQ(&valsY, arg.constValuesByIndex(1)); // Testing that y values were changed inplace.
    }

    // Basic test with explicitly set smoothing radius
    {
        ChartEntryOperationList operations;
        operations.push_back(opManager.createOperation(DFG_ASCII("smoothing_indexNb(2)")));
        const ValueVectorD valsX{ 1, 2, 10, 15 };
        const ValueVectorD valsY{ 50, 60, 100, 150 };
        ChartOperationPipeData arg(&valsX, &valsY);
        operations.executeAll(arg);
        EXPECT_EQ(&valsX, arg.constValuesByIndex(0)); // Testing that const input hasn't been touched
        EXPECT_NE(&valsY, arg.constValuesByIndex(1)); // Testing that new y container was created.
        ASSERT_NE(nullptr, arg.constValuesByIndex(1));
        EXPECT_EQ(ValueVectorD({ 70, 90, 90, (60.0 + 100.0 + 150.0) / 3.0 }), *arg.constValuesByIndex(1));
    }

    // Basic test with median smoothing
    {
        ChartEntryOperationList operations;
        operations.push_back(opManager.createOperation(DFG_ASCII("smoothing_indexNb(1, median)")));
        const ValueVectorD valsX{ 1, 2, 10 };
        ValueVectorD valsY{ 50, 60, 100 };
        ChartOperationPipeData arg(&valsX, &valsY);
        operations.executeAll(arg);
        EXPECT_EQ(ValueVectorD({ 55, 60, 80 }), valsY);
        EXPECT_EQ(&valsX, arg.constValuesByIndex(0)); // Testing that const input hasn't been touched
        EXPECT_EQ(&valsY, arg.constValuesByIndex(1)); // Testing that y values were changed inplace.
    }

    // Basic test with median smoothing with longer radius
    {
        ChartEntryOperationList operations;
        operations.push_back(opManager.createOperation(DFG_ASCII("smoothing_indexNb(2, median)")));
        const ValueVectorD valsX{ 1, 2, 10, 15 };
        ValueVectorD valsY{ 50, 60, 100, 150 };
        ChartOperationPipeData arg(&valsX, &valsY);
        operations.executeAll(arg);
        EXPECT_EQ(ValueVectorD({ 60, 80, 80, 100 }), valsY);
        EXPECT_EQ(&valsX, arg.constValuesByIndex(0)); // Testing that const input hasn't been touched
        EXPECT_EQ(&valsY, arg.constValuesByIndex(1)); // Testing that y values were changed inplace.
    }

    // Median smoothing with longer data
    {
        ChartEntryOperationList operations;
        operations.push_back(opManager.createOperation(DFG_ASCII("smoothing_indexNb(2, median)")));
        const ValueVectorD valsX{ 1 ,  2, 10,  15, 20, 30, 40, 50 };
              ValueVectorD valsY{ 50, 60, 20, 150, 10,  3, 15, 70 };
        ChartOperationPipeData arg(&valsX, &valsY);
        operations.executeAll(arg);
        EXPECT_EQ(ValueVectorD({ 50, 55, 50, 20, 15, 15, 12.5, 15 }), valsY);
        EXPECT_EQ(&valsX, arg.constValuesByIndex(0)); // Testing that const input hasn't been touched
        EXPECT_EQ(&valsY, arg.constValuesByIndex(1)); // Testing that y values were changed inplace.
    }

    // Handling of empty radius
    {
        ChartEntryOperationList operations;
        operations.push_back(opManager.createOperation(DFG_ASCII("smoothing_indexNb(0)")));
        const ValueVectorD valsX{ 1, 2, 10 };
        const ValueVectorD valsY{ 50, 60, 100 };
        ChartOperationPipeData arg(&valsX, &valsY);
        operations.executeAll(arg);
        // Testing that inputs hasn't been touched since with radius 0 nothing should get done.
        EXPECT_EQ(&valsX, arg.constValuesByIndex(0));
        EXPECT_EQ(&valsY, arg.constValuesByIndex(1));
    }

    // Testing various invalid arguments
    {
        EXPECT_FALSE(opManager.createOperation(DFG_ASCII("smoothing_indexNb(abc)"))); // Non-number radius
        EXPECT_FALSE(opManager.createOperation(DFG_ASCII("smoothing_indexNb(1, invalidType)"))); // Invalid smoothing type
        EXPECT_FALSE(opManager.createOperation(DFG_ASCII("smoothing_indexNb(1, 2, 3)"))); // Too many arguments
        EXPECT_FALSE(opManager.createOperation(DFG_ASCII("smoothing_indexNb(-1)"))); // Negative radius
        EXPECT_FALSE(opManager.createOperation(DFG_ASCII("smoothing_indexNb(1.25)"))); // Non-integer radius
    }
}


TEST(dfgCharts, operations_formula)
{
    using namespace ::DFG_ROOT_NS;
    using namespace ::DFG_MODULE_NS(charts);

    ChartEntryOperationManager opManager;

    const ValueVectorD defaultValsX{ 1 ,  2, 10,  15 };
    const ValueVectorD defaultValsY{ 50, 60, 20, 150 };
    const auto formulaXedit0 = DFG_ASCII("formula(x, x + 2)");
    const auto formulaYedit0 = DFG_ASCII("formula(y, x + y + abs(-2))");
    const ValueVectorD xValsAfterFormulaXedits0{ 3, 4, 12, 17 };
    const ValueVectorD yValsAfterFormulaYEdits0{ 53, 64, 32, 167 };

    // Basic tests with modifiable inputs
    {
        // Editing x
        {
            auto xVals = defaultValsX;
            auto yVals = defaultValsY;
            ChartOperationPipeData arg(&xVals, &yVals);
            opManager.createOperation(formulaXedit0)(arg);
            EXPECT_EQ(&xVals, arg.constValuesByIndex(0));
            EXPECT_EQ(&yVals, arg.constValuesByIndex(1));
            EXPECT_EQ(xValsAfterFormulaXedits0, xVals);
            EXPECT_EQ(defaultValsY, yVals);
        }

        // Editing y
        {
            auto xVals = defaultValsX;
            auto yVals = defaultValsY;
            ChartOperationPipeData arg(&xVals, &yVals);
            opManager.createOperation(formulaYedit0)(arg);
            EXPECT_EQ(&xVals, arg.constValuesByIndex(0));
            EXPECT_EQ(&yVals, arg.constValuesByIndex(1));
            EXPECT_EQ(defaultValsX, xVals);
            EXPECT_EQ(yValsAfterFormulaYEdits0, yVals);
        }
    }

    // Basic test with const inputs
    {
        // Editing x
        {
            ChartOperationPipeData arg(&defaultValsX, &defaultValsY);
            opManager.createOperation(formulaXedit0)(arg);
            ASSERT_TRUE(arg.constValuesByIndex(0) != nullptr && arg.constValuesByIndex(1) != nullptr);
            EXPECT_NE(&defaultValsX, arg.constValuesByIndex(0));
            EXPECT_EQ(&defaultValsY, arg.constValuesByIndex(1));
            EXPECT_EQ(xValsAfterFormulaXedits0, *arg.constValuesByIndex(0));
            EXPECT_EQ(defaultValsY, *arg.constValuesByIndex(1));
        }

        // Editing y
        {
            ChartOperationPipeData arg(&defaultValsX, &defaultValsY);
            opManager.createOperation(formulaYedit0)(arg);
            ASSERT_TRUE(arg.constValuesByIndex(0) != nullptr && arg.constValuesByIndex(1) != nullptr);
            EXPECT_EQ(&defaultValsX, arg.constValuesByIndex(0));
            EXPECT_NE(&defaultValsY, arg.constValuesByIndex(1));
            EXPECT_EQ(defaultValsX, *arg.constValuesByIndex(0));
            EXPECT_EQ(yValsAfterFormulaYEdits0, *arg.constValuesByIndex(1));
        }
    }

    // modifiable x input, but not modified
    {
        auto xVals = defaultValsX;
        ChartOperationPipeData arg(&xVals, &defaultValsY);
        opManager.createOperation(formulaYedit0)(arg);
        ASSERT_TRUE(arg.constValuesByIndex(0) != nullptr && arg.constValuesByIndex(1) != nullptr);
        EXPECT_EQ(&xVals, arg.constValuesByIndex(0));
        EXPECT_NE(&defaultValsY, arg.constValuesByIndex(1));
        EXPECT_EQ(defaultValsX, *arg.constValuesByIndex(0));
        EXPECT_EQ(yValsAfterFormulaYEdits0, *arg.constValuesByIndex(1));
    }

    // modifiable y input, but not modified
    {
        auto yVals = defaultValsY;
        ChartOperationPipeData arg(&defaultValsX, &yVals);
        opManager.createOperation(formulaXedit0)(arg);
        ASSERT_TRUE(arg.constValuesByIndex(0) != nullptr && arg.constValuesByIndex(1) != nullptr);
        EXPECT_NE(&defaultValsX, arg.constValuesByIndex(0));
        EXPECT_EQ(&yVals, arg.constValuesByIndex(1));
        EXPECT_EQ(xValsAfterFormulaXedits0, *arg.constValuesByIndex(0));
        EXPECT_EQ(defaultValsY, *arg.constValuesByIndex(1));
    }

    // Only x-input
    {
        ValueVectorD xVals({ 1, 2, 3 });
        auto op = opManager.createOperation(DFG_UTF8("formula(x, 1+x)"));
        ChartOperationPipeData arg(&xVals, nullptr);
        op(arg);
        EXPECT_EQ(&xVals, arg.constValuesByIndex(0));
        EXPECT_EQ(ValueVectorD({2, 3, 4}), xVals);
    }

    // Only y-input
    {
        ValueVectorD yVals({ 1, 2, 3 });
        auto op = opManager.createOperation(DFG_UTF8("formula(y, 1+y)"));
        ChartOperationPipeData arg(nullptr, &yVals);
        op(arg);
        EXPECT_EQ(&yVals, arg.constValuesByIndex(1));
        EXPECT_EQ(ValueVectorD({ 2, 3, 4 }), yVals);
    }

    // Error handling: syntax error, use of undeclared variable, use of undeclared function
    const char* errorCases[] = {"formula(x, 1+-*/2)", "formula(x, 1+z)", "formula(x, 2 + unknownFunction(1))"};
    for (const auto& pszFormula : errorCases)
    {
        ValueVectorD xVals({1, 2, 3});
        const ValueVectorD yVals({1, 2, 3});
        auto op = opManager.createOperation(SzPtrUtf8(pszFormula));
        ChartOperationPipeData arg(&xVals, &yVals);
        op(arg);
        EXPECT_EQ(&xVals, arg.constValuesByIndex(0));
        EXPECT_TRUE(std::all_of(xVals.begin(), xVals.begin(), [](double d) { return ::DFG_MODULE_NS(math)::isNan(d); }));
        EXPECT_EQ(&yVals, arg.constValuesByIndex(1));
    }


    // Error handling: missing input
    {
        auto op = opManager.createOperation(SzPtrUtf8("formula(x, 1+x)"));
        ChartOperationPipeData arg;
        op(arg);
        EXPECT_TRUE(op.hasErrors());
        EXPECT_TRUE(op.hasError(ChartEntryOperation::error_missingInput));
    }
}

TEST(dfgCharts, ChartEntryOperationManager)
{
    using namespace ::DFG_ROOT_NS;
    using namespace ::DFG_MODULE_NS(alg);
    using namespace ::DFG_MODULE_NS(charts);
    using namespace ::DFG_MODULE_NS(cont);

    ChartEntryOperationManager opManager;

    const auto pszTestOperation = DFG_UTF8("testOperation");
    opManager.add(pszTestOperation, [](const ChartEntryOperationManager::CreationArgList&) { return ChartEntryOperation(); });

    EXPECT_TRUE(opManager.hasOperation(operations::PassWindowOperation::id()));
    EXPECT_TRUE(opManager.hasOperation(pszTestOperation));
    EXPECT_FALSE(opManager.hasOperation(DFG_UTF8("non existent operation")));

    SetVector<StringUtf8> operationSet;
    opManager.forEachOperationId([&](const StringViewUtf8& sv)
    {
        operationSet.insert(sv.toString());
    });
    ASSERT_EQ(opManager.operationCount(), operationSet.size());
    EXPECT_TRUE(std::equal(operationSet.cbegin(), operationSet.cend(), opManager.m_knownOperations.beginKey()));
}

namespace
{
    class ControlItemMoc : public ::DFG_MODULE_NS(charts)::AbstractChartControlItem
    {
    private:
        std::pair<bool, String> fieldValueStrImpl(FieldIdStrViewInputParam fieldId) const override
        {
            DFG_UNUSED(fieldId);
            return std::pair<bool, String>(false, String());
        }

        void forEachPropertyIdImpl(std::function<void(StringView svId)>) const override
        {
        }
    };

    class ControlItemMoc2 : public ControlItemMoc
    {
    private:
        bool hasFieldImpl(FieldIdStrViewInputParam fieldId) const override
        {
            return fieldId == ::DFG_MODULE_NS(charts)::fieldsIds::ChartObjectFieldIdStr_binCount;
        }
    };
} // unnamed namespace

TEST(dfgCharts, AbstractChartControlItem)
{
    using namespace ::DFG_MODULE_NS(charts);

    {
        ControlItemMoc a;
        EXPECT_EQ(ControlItemMoc::LogLevel::info, a.logLevel()); // Currently default log level is expected to be info.
        bool bGoodLogLevel;
        EXPECT_EQ(ControlItemMoc::LogLevel::info, a.logLevel(DFG_UTF8(""), &bGoodLogLevel));
        EXPECT_FALSE(bGoodLogLevel);
        EXPECT_EQ(ControlItemMoc::LogLevel::info, a.logLevel(DFG_UTF8("invalid"), &bGoodLogLevel));
        EXPECT_FALSE(bGoodLogLevel);

        EXPECT_EQ(ControlItemMoc::LogLevel::debug, a.logLevel(DFG_UTF8("debug"), &bGoodLogLevel));
        EXPECT_TRUE(bGoodLogLevel);
        EXPECT_EQ(ControlItemMoc::LogLevel::info, a.logLevel(DFG_UTF8("info"), &bGoodLogLevel));
        EXPECT_TRUE(bGoodLogLevel);
        EXPECT_EQ(ControlItemMoc::LogLevel::warning, a.logLevel(DFG_UTF8("warning"), &bGoodLogLevel));
        EXPECT_TRUE(bGoodLogLevel);
        EXPECT_EQ(ControlItemMoc::LogLevel::error, a.logLevel(DFG_UTF8("error"), &bGoodLogLevel));
        EXPECT_TRUE(bGoodLogLevel);
        EXPECT_EQ(ControlItemMoc::LogLevel::none, a.logLevel(DFG_UTF8("none"), &bGoodLogLevel));
        EXPECT_TRUE(bGoodLogLevel);

        EXPECT_EQ(ControlItemMoc::LogLevel::debug,   a.logLevel(ControlItemMoc::LogLevel::debug));
        EXPECT_EQ(ControlItemMoc::LogLevel::info,    a.logLevel(ControlItemMoc::LogLevel::info));
        EXPECT_EQ(ControlItemMoc::LogLevel::warning, a.logLevel(ControlItemMoc::LogLevel::warning));
        EXPECT_EQ(ControlItemMoc::LogLevel::error,   a.logLevel(ControlItemMoc::LogLevel::error));
        EXPECT_EQ(ControlItemMoc::LogLevel::none,    a.logLevel(ControlItemMoc::LogLevel::none));
    }

    // hasField()
    {
        ControlItemMoc a;
        ControlItemMoc2 a2;
        EXPECT_FALSE(a.hasField(ChartObjectFieldIdStr_name));
        EXPECT_FALSE(a.hasField(ChartObjectFieldIdStr_binCount));
        EXPECT_FALSE(a2.hasField(ChartObjectFieldIdStr_name));
        EXPECT_TRUE(a2.hasField(ChartObjectFieldIdStr_binCount));
    }

}

TEST(dfgCharts, ChartOperationPipeData)
{
    using namespace DFG_ROOT_NS;
    using namespace ::DFG_MODULE_NS(charts);
    using namespace ::DFG_MODULE_NS(cont);

    // Testing constructors and assignment. In particular testing that if data has references to it's internal structures,
    // the references in the copied one must not be invalidated when the copied-from object is destroyed.
    {
        const ValueVectorD constValues = { 1 };
        const ChartOperationPipeData::StringVector constStrings = { StringUtf8(DFG_UTF8("a")) };
        ChartOperationPipeData dataCopied;
        ChartOperationPipeData dataMoved;
        {
            ChartOperationPipeData dataTemp;

            dataTemp.m_valueVectors.push_back(ValueVectorD()); // Unreferenced internal value vector
            dataTemp.m_valueVectors.push_back(ValueVectorD(1, 2));
            dataTemp.m_stringVectors.push_back(ChartOperationPipeData::StringVector()); // Unreferenced internal string vector
            dataTemp.m_stringVectors.push_back(ChartOperationPipeData::StringVector(1, StringUtf8(DFG_UTF8("b"))));

            dataTemp.m_vectorRefs.push_back(&constValues);
            dataTemp.m_vectorRefs.push_back(&dataTemp.m_stringVectors[1]);
            dataTemp.m_vectorRefs.push_back(&dataTemp.m_valueVectors[1]);
            dataTemp.m_vectorRefs.push_back(&constStrings);

            dataCopied = dataTemp;
            dataMoved = std::move(dataTemp);
        }
        ASSERT_EQ(4, dataCopied.vectorCount());
        ASSERT_EQ(4, dataMoved.vectorCount());

        EXPECT_TRUE(dataCopied.m_valueVectors == dataMoved.m_valueVectors);
        EXPECT_TRUE(dataCopied.m_stringVectors == dataMoved.m_stringVectors);

        // Verifying that both copied and moved still refer to external values
        EXPECT_EQ(&constValues, dataCopied.constValuesByIndex(0));
        EXPECT_EQ(&constValues, dataMoved.constValuesByIndex(0));

        // Verifying that internal string references refer to internal items.
        EXPECT_EQ(dataCopied.constStringsByIndex(1), dataCopied.editableStringsByIndex(1));
        EXPECT_EQ(dataMoved.constStringsByIndex(1), dataMoved.editableStringsByIndex(1));

        // Verifying that internal value references refer to internal items.
        EXPECT_EQ(dataCopied.constValuesByIndex(2), dataCopied.editableValuesByIndex(1));
        EXPECT_EQ(dataMoved.constValuesByIndex(2), dataMoved.editableValuesByIndex(1));

        // Verifying that both copied and moved still refer to external strings
        EXPECT_EQ(&constStrings, dataCopied.constStringsByIndex(3));
        EXPECT_EQ(&constStrings, dataMoved.constStringsByIndex(3));
    }
}

#endif

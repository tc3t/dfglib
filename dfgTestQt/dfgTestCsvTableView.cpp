#include <dfg/qt/CsvItemModel.hpp>
#include <dfg/io.hpp>
#include <dfg/math/sign.hpp>
#include <dfg/io/OmcByteStream.hpp>
#include <dfg/qt/containerUtils.hpp>
#include <dfg/qt/CsvTableView.hpp>
#include <dfg/qt/ConsoleDisplay.hpp>
#include <dfg/qt/CsvTableViewActions.hpp>
#include <dfg/qt/CsvTableViewChartDataSource.hpp>
#include <dfg/qt/CsvFileDataSource.hpp>
#include <dfg/qt/TableEditor.hpp>
#include <dfg/qt/SQLiteFileDataSource.hpp>
#include <dfg/qt/NumberGeneratorDataSource.hpp>
#include <dfg/qt/connectHelper.hpp>
#include <dfg/math.hpp>
#include <dfg/qt/sqlTools.hpp>
#include <dfg/qt/PatternMatcher.hpp>
#include <dfg/iter/FunctionValueIterator.hpp>
#include <dfg/os/TemporaryFileStream.hpp>
#include "../dfgTest/dfgTest.hpp"
#include "dfgTestQt_gtestPrinters.hpp"

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
    #include <gtest/gtest.h>
    #include <QAction>
    #include <QCoreApplication>
    #include <QDateTime>
    #include <QFile>
    #include <QFileInfo>
    #include <QDialog>
    #include <QSpinBox>
    #include <QSqlError>
    #include <QSqlQuery>
    #include <QSqlRecord>
    #include <QGridLayout>
    #include <QSortFilterProxyModel>
    #include <QThread>
    #include <QUndoStack>
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS

namespace
{
    const double baseDateTestDoubleValue = static_cast<double>(QDateTime::fromString(QStringLiteral("2020-04-25T00:00:00Z"), Qt::ISODate).toMSecsSinceEpoch()) / 1000.0;

    void testDateToDouble(const char* pszDateString, ::DFG_MODULE_NS(qt)::ChartDataType dataType, const double offsetFromBase, const double baseValue = baseDateTestDoubleValue)
    {
        using namespace  ::DFG_ROOT_NS;
        using namespace  ::DFG_MODULE_NS(qt);
        using ColumnDataTypeMap = ::DFG_MODULE_NS(qt)::GraphDataSource::ColumnDataTypeMap;
        ColumnDataTypeMap columnDataTypes;
        EXPECT_EQ(baseValue + offsetFromBase, GraphDataSource::cellStringToDouble(SzPtrUtf8(pszDateString), 1, &columnDataTypes));
        EXPECT_TRUE(columnDataTypes.size() == 1 && columnDataTypes.frontKey() == 1 && columnDataTypes.frontValue() == dataType);
    }

    void testDateToDouble_invalidDateFormat(const char* pszDateString)
    {
        using namespace  ::DFG_ROOT_NS;
        using namespace  ::DFG_MODULE_NS(qt);
        using ColumnDataTypeMap = ::DFG_MODULE_NS(qt)::GraphDataSource::ColumnDataTypeMap;
        ColumnDataTypeMap columnDataTypes;
        EXPECT_TRUE(::DFG_MODULE_NS(math)::isNan(GraphDataSource::cellStringToDouble(SzPtrUtf8(pszDateString), 1, &columnDataTypes)));
    }
} // unnamed namespace

TEST(dfgQt, CsvTableView_undoAfterRemoveRows)
{
    ::DFG_MODULE_NS(qt)::CsvItemModel model;
    ::DFG_MODULE_NS(qt)::CsvTableView view(nullptr, nullptr);
    view.setModel(&model);

    view.insertRowHere();
    view.insertRowHere();
    view.insertColumn();
    view.insertColumn();

    model.setDataNoUndo(0, 1, DFG_UTF8("a"));
    model.setDataNoUndo(1, 1, DFG_UTF8("b"));

    view.setCurrentIndex(model.index(1, 1));
    view.insertRowAfterCurrent();
    view.insertRowAfterCurrent();

    model.setDataNoUndo(2, 1, DFG_UTF8("c"));
    model.setDataNoUndo(3, 1, DFG_UTF8("d"));

    EXPECT_EQ(4, model.getRowCount());
    view.selectColumn(1);
    view.deleteSelectedRow(); // Deletes all rows given the selectColumn() above.

    EXPECT_EQ(0, model.getRowCount());
    view.m_spUndoStack->item().undo();
    EXPECT_EQ(4, model.getRowCount());
    EXPECT_EQ(QString(), model.data(model.index(0, 0)).toString());
    EXPECT_EQ(QString(), model.data(model.index(1, 0)).toString());
    EXPECT_EQ(QString(), model.data(model.index(2, 0)).toString());
    EXPECT_EQ(QString(), model.data(model.index(3, 0)).toString());

    EXPECT_EQ(QString("a"), model.data(model.index(0, 1)).toString());
    EXPECT_EQ(QString("b"), model.data(model.index(1, 1)).toString());
    EXPECT_EQ(QString("c"), model.data(model.index(2, 1)).toString());
    EXPECT_EQ(QString("d"), model.data(model.index(3, 1)).toString());
}

TEST(dfgQt, CsvTableView_undoAfterResize)
{
    ::DFG_MODULE_NS(qt)::CsvItemModel csvModel;
    ::DFG_MODULE_NS(qt)::CsvTableView view(nullptr, nullptr);
    QSortFilterProxyModel viewModel;
    viewModel.setSourceModel(&csvModel);
    viewModel.setDynamicSortFilter(true);
    view.setModel(&viewModel);

    view.resizeTableNoUi(2, 2);
    csvModel.setColumnName(0, "Col1");
    csvModel.setColumnName(1, "Col2");
    csvModel.setDataNoUndo(0, 0, DFG_UTF8("a"));
    csvModel.setDataNoUndo(0, 1, DFG_UTF8("b"));
    csvModel.setDataNoUndo(1, 0, DFG_UTF8("c"));
    csvModel.setDataNoUndo(1, 1, DFG_UTF8("d"));

    view.resizeTableNoUi(2, 1);
    DFGTEST_EXPECT_LEFT(2, csvModel.rowCount());
    DFGTEST_EXPECT_LEFT(1, csvModel.columnCount());
    view.undo();
    DFGTEST_EXPECT_LEFT(2, csvModel.rowCount());
    DFGTEST_EXPECT_LEFT(2, csvModel.columnCount());
    DFGTEST_EXPECT_LEFT("Col1", csvModel.getHeaderName(0));
    DFGTEST_EXPECT_LEFT("Col2", csvModel.getHeaderName(1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("a", csvModel.rawStringViewAt(0, 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("b", csvModel.rawStringViewAt(0, 1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("c", csvModel.rawStringViewAt(1, 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("d", csvModel.rawStringViewAt(1, 1));

    view.resizeTableNoUi(1, 3);
    DFGTEST_EXPECT_LEFT(1, csvModel.rowCount());
    DFGTEST_EXPECT_LEFT(3, csvModel.columnCount());
    view.undo();
    DFGTEST_EXPECT_LEFT(2, csvModel.rowCount());
    DFGTEST_EXPECT_LEFT(2, csvModel.columnCount());
    DFGTEST_EXPECT_LEFT("Col1", csvModel.getHeaderName(0));
    DFGTEST_EXPECT_LEFT("Col2", csvModel.getHeaderName(1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("a", csvModel.rawStringViewAt(0, 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("b", csvModel.rawStringViewAt(0, 1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("c", csvModel.rawStringViewAt(1, 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("d", csvModel.rawStringViewAt(1, 1));
}

namespace
{
    void populateModelWithRowColumnIndexStrings(::DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)& csvModel, int nRowCount = -1, int nColCount = -1)
    {
        if (csvModel.rowCount() == 0 && nRowCount >= 0)
            csvModel.insertRows(0, nRowCount);
        if (csvModel.columnCount() == 0 && nColCount >= 0)
            csvModel.insertColumns(0, nColCount);

        nRowCount = csvModel.rowCount();
        nColCount = csvModel.columnCount();
        for (int c = 0; c < nColCount; ++c)
        {
            for (int r = 0; r < nRowCount; ++r)
            {
                csvModel.setDataNoUndo(r, c, QString::number(r) + QString::number(c));
            }
        }
    }
}

TEST(dfgQt, CsvTableView_copyToClipboard)
{
    // Note: this tests only that the created strings, which would be copied clipboard, are well formed, clipboard is not changed.

    ::DFG_MODULE_NS(qt)::CsvItemModel csvModel;
    ::DFG_MODULE_NS(qt)::CsvTableView view(nullptr, nullptr);
    QSortFilterProxyModel viewModel;
    viewModel.setSourceModel(&csvModel);
    viewModel.setDynamicSortFilter(true);

    view.setModel(&viewModel);

    populateModelWithRowColumnIndexStrings(csvModel, 4, 4);
    csvModel.insertRow(4);
    csvModel.setDataNoUndo(4, 2, "42"); // For testing leading null cells.

    // Testing contiguous selections starting from first element.
    {
        view.selectionModel()->select(viewModel.index(0, 0), QItemSelectionModel::SelectCurrent);
        EXPECT_EQ(QString("00\n"), view.makeClipboardStringForCopy());
        view.selectionModel()->select(viewModel.index(0, 1), QItemSelectionModel::Select);
        EXPECT_EQ(QString("00\t01\n"), view.makeClipboardStringForCopy());
        view.selectionModel()->select(viewModel.index(0, 2), QItemSelectionModel::Select);
        EXPECT_EQ(QString("00\t01\t02\n"), view.makeClipboardStringForCopy());
        view.selectionModel()->select(viewModel.index(0, 3), QItemSelectionModel::Select);
        EXPECT_EQ(QString("00\t01\t02\t03\n"), view.makeClipboardStringForCopy());
    }

    // Testing miscellaneous selections on first row
    {
        view.clearSelection();
        view.selectionModel()->select(viewModel.index(0, 1), QItemSelectionModel::SelectCurrent);
        EXPECT_EQ(QString("01\n"), view.makeClipboardStringForCopy());
        view.selectionModel()->select(viewModel.index(0, 3), QItemSelectionModel::Select);
        EXPECT_EQ(QString("01\t03\n"), view.makeClipboardStringForCopy());

        view.clearSelection();
        view.selectionModel()->select(viewModel.index(0, 3), QItemSelectionModel::SelectCurrent);
        EXPECT_EQ(QString("03\n"), view.makeClipboardStringForCopy());
    }

    // Testing miscellaneous selections of single items
    {
        view.clearSelection();
        view.selectionModel()->select(viewModel.index(1, 0), QItemSelectionModel::SelectCurrent);
        view.selectionModel()->select(viewModel.index(3, 2), QItemSelectionModel::Select);
        EXPECT_EQ(QString("10\n32\n"), view.makeClipboardStringForCopy());

        view.clearSelection();
        view.selectionModel()->select(viewModel.index(0, 3), QItemSelectionModel::SelectCurrent);
        view.selectionModel()->select(viewModel.index(1, 0), QItemSelectionModel::Select);
        view.selectionModel()->select(viewModel.index(2, 2), QItemSelectionModel::Select);
        EXPECT_EQ(QString("03\n10\n22\n"), view.makeClipboardStringForCopy());
    }

    // Testing null cell handling
    {
        view.clearSelection();
        view.selectionModel()->select(viewModel.index(4, 0), QItemSelectionModel::SelectCurrent);
        view.selectionModel()->select(viewModel.index(4, 1), QItemSelectionModel::Select);
        view.selectionModel()->select(viewModel.index(4, 2), QItemSelectionModel::Select);
        view.selectionModel()->select(viewModel.index(4, 3), QItemSelectionModel::Select);
        EXPECT_EQ(QString("\t\t42\t\n"), view.makeClipboardStringForCopy());

    }

    // Testing column selections
    {
        view.selectColumn(0);
        EXPECT_EQ(QString("00\n10\n20\n30\n\n"), view.makeClipboardStringForCopy());

        view.selectColumn(2);
        EXPECT_EQ(QString("02\n12\n22\n32\n42\n"), view.makeClipboardStringForCopy());

        {
            view.clearSelection();
            QItemSelection selection;
            selection.push_back(QItemSelectionRange(viewModel.index(0, 0), viewModel.index(4, 0)));
            selection.push_back(QItemSelectionRange(viewModel.index(0, 2), viewModel.index(4, 2)));
            view.selectionModel()->select(selection, QItemSelectionModel::Select);
            EXPECT_EQ(QString("00\t02\n10\t12\n20\t22\n30\t32\n\t42\n"), view.makeClipboardStringForCopy());
        }

        {
            view.clearSelection();
            QItemSelection selection;
            selection.push_back(QItemSelectionRange(viewModel.index(0, 2), viewModel.index(4, 2)));
            selection.push_back(QItemSelectionRange(viewModel.index(0, 3), viewModel.index(4, 3)));
            view.selectionModel()->select(selection, QItemSelectionModel::Select);
            EXPECT_EQ(QString("02\t03\n12\t13\n22\t23\n32\t33\n42\t\n"), view.makeClipboardStringForCopy());
        }
    }

    // Testing whole table selection
    {
        view.selectAll();
        EXPECT_EQ(QString("00\t01\t02\t03\n"
                          "10\t11\t12\t13\n"
                          "20\t21\t22\t23\n"
                          "30\t31\t32\t33\n"
                          "\t\t42\t\n"),
                view.makeClipboardStringForCopy());
    }

    // Testing whole table selection after filter has been activated.
    {
        viewModel.setFilterKeyColumn(-1);
        viewModel.setFilterFixedString("12");
        EXPECT_EQ(csvModel.getRowCount(), view.getRowCount_dataModel());
        EXPECT_EQ(1, view.getRowCount_viewModel());
        view.selectAll();
        EXPECT_EQ(QString("10\t11\t12\t13\n"), view.makeClipboardStringForCopy());
        viewModel.setFilterFixedString(QString());
    }

    // Testing selection in sorted table
    {
        viewModel.setDynamicSortFilter(true);
        viewModel.sort(2, Qt::DescendingOrder);
        view.clearSelection();
        view.selectionModel()->select(viewModel.index(0, 2), QItemSelectionModel::SelectCurrent);
        view.selectRow(0);
        EXPECT_EQ(QString("\t\t42\t\n"), view.makeClipboardStringForCopy());
    }

    // Testing selection in filtered and sorted table.
    {
        viewModel.setFilterKeyColumn(-1);
        viewModel.setFilterFixedString("33");
        EXPECT_EQ(csvModel.getRowCount(), view.getRowCount_dataModel());
        EXPECT_EQ(1, view.getRowCount_viewModel());
        view.clearSelection();
        QItemSelection selection;
        selection.push_back(QItemSelectionRange(viewModel.index(0, 0), viewModel.index(0, 0)));
        selection.push_back(QItemSelectionRange(viewModel.index(0, 3), viewModel.index(0, 3)));
        view.selectionModel()->select(selection, QItemSelectionModel::Select);
        EXPECT_EQ(QString("30\t33\n"), view.makeClipboardStringForCopy());
    }
}

TEST(dfgQt, CsvTableView_paste)
{
    using namespace ::DFG_MODULE_NS(qt);
    CsvItemModel csvModel;
    CsvTableView view(nullptr, nullptr);
    QSortFilterProxyModel viewModel;
    viewModel.setSourceModel(&csvModel);
    //viewModel.setDynamicSortFilter(true);
    view.setModel(&viewModel);

    const int nRowCount = 100;
    const int nColCount = 4;
    populateModelWithRowColumnIndexStrings(csvModel, nRowCount, nColCount);

    view.selectColumn(0);
    const auto clipboardStringCol0 = view.makeClipboardStringForCopy();
    view.copy();

    // Connecting to dataChanged-signal
    size_t nSignalCount = 0;
    DFG_QT_VERIFY_CONNECT(QObject::connect(&csvModel, &CsvItemModel::dataChanged, [&]() { ++nSignalCount; }));

    // Selecting 0, 1 and paste
    view.makeSingleCellSelection(0, 1);
    view.paste();

    // Making sure that got only one change notification
    EXPECT_EQ(1u, nSignalCount);

    // Making sure that columns 0 and 1 are identical.
    view.selectColumn(1);
    EXPECT_EQ(clipboardStringCol0, view.makeClipboardStringForCopy());

    // Trying to paste again and making sure that don't receive dataChanged-signal
    view.makeSingleCellSelection(0, 1);
    nSignalCount = 0;
    view.paste();
    EXPECT_EQ(0u, nSignalCount);

    // Setting a few cells to column 1, pasting again and making sure that get expected number of change notifications.
    // Note: this is partly implementation test rather than interface test (i.e. point is not to make interface guarantees that there will be
    //       exactly certain number of dataChanged-signals, but guarantee that paste won't generate ridiculous number of signals.
    csvModel.setDataNoUndo(10, 1, "a");
    csvModel.setDataNoUndo(11, 1, "a");
    csvModel.setDataNoUndo(12, 1, "a");
    csvModel.setDataNoUndo(50, 1, "b");
    csvModel.setDataNoUndo(75, 1, "c");
    view.makeSingleCellSelection(0, 1);
    nSignalCount = 0;
    view.paste();
    EXPECT_EQ(3u, nSignalCount);
    view.selectColumn(1);
    EXPECT_EQ(clipboardStringCol0, view.makeClipboardStringForCopy());

    // Testing signal count when whole table is getting pasted
    view.selectAll();
    view.copy();
    view.clearSelected();
    view.makeSingleCellSelection(0, 0);
    nSignalCount = 0;
    view.paste();
    EXPECT_EQ(static_cast<size_t>(nColCount), nSignalCount); // Having only 1 signal would be better, but as commented above, at least making sure that there's no signal per cell.

    // Testing signal count when pasting row
    view.selectAll();
    view.copy();
    view.clearSelected();
    view.selectRow(0);
    view.makeSingleCellSelection(1, 0);
    nSignalCount = 0;
    view.paste();
    EXPECT_EQ(static_cast<size_t>(nColCount), nSignalCount); // Having only 1 signal would be better.
}

TEST(dfgQt, CsvTableView_clear)
{
    using namespace ::DFG_MODULE_NS(qt);
    CsvItemModel csvModel;
    CsvTableView view(nullptr, nullptr);
    QSortFilterProxyModel viewModel;
    viewModel.setSourceModel(&csvModel);
    view.setModel(&viewModel);

    const int nRowCount = 10;
    const int nColCount = 4;
    populateModelWithRowColumnIndexStrings(csvModel, nRowCount, nColCount);

    size_t nSignalCount = 0;
    DFG_QT_VERIFY_CONNECT(QObject::connect(&csvModel, &CsvItemModel::dataChanged, [&]() { ++nSignalCount; }));

    view.selectAll();
    const auto sAfterPopulating = view.makeClipboardStringForCopy();
    view.selectAll();
    nSignalCount = 0;
    view.clearSelected();
    const auto sAfterClearing = view.makeClipboardStringForCopy();
    EXPECT_EQ(static_cast<size_t>(nColCount), nSignalCount); // Expecting signaling from clear to behave similarly to pasting.
    nSignalCount = 0;
    view.undo();
    EXPECT_EQ(sAfterPopulating, view.makeClipboardStringForCopy());
    EXPECT_EQ(static_cast<size_t>(nColCount), nSignalCount);
    nSignalCount = 0;
    view.redo();
    EXPECT_EQ(sAfterClearing, view.makeClipboardStringForCopy());
    EXPECT_EQ(static_cast<size_t>(nColCount), nSignalCount);
}

TEST(dfgQt, CsvTableView_replace)
{
    using namespace ::DFG_MODULE_NS(qt);
    CsvItemModel csvModel;
    CsvTableView view(nullptr, nullptr);
    QSortFilterProxyModel viewModel;
    viewModel.setSourceModel(&csvModel);
    view.setModel(&viewModel);

    csvModel.insertRows(0, 2);
    csvModel.insertColumns(0, 2);
    csvModel.setDataNoUndo(0, 0, DFG_UTF8("a"));
    csvModel.setDataNoUndo(0, 1, DFG_UTF8("b"));
    csvModel.setDataNoUndo(1, 0, DFG_UTF8("1a2"));
    csvModel.setDataNoUndo(1, 1, DFG_UTF8("d"));
    csvModel.setModifiedStatus(false);

    // This should not edit anything as nothing is selected
    EXPECT_EQ(0u, view.replace(QVariantMap({{"find", "a"}, {"replace", "c"}})));
    EXPECT_FALSE(csvModel.isModified());

    view.selectAll();
    EXPECT_EQ(0u, view.replace(QVariantMap()));
    EXPECT_FALSE(csvModel.isModified());
    EXPECT_EQ(0u, view.replace(QVariantMap({{"find", "a"}})));
    EXPECT_FALSE(csvModel.isModified());
    EXPECT_EQ(0u, view.replace(QVariantMap({{"find", "a3"}, {"replace", "bug"}})));
    EXPECT_FALSE(csvModel.isModified());

    EXPECT_EQ(2u, view.replace(QVariantMap({{"find", "a"}, {"replace", "c"}})));
    EXPECT_TRUE(csvModel.isModified());

    EXPECT_STREQ("c", csvModel.rawStringPtrAt(0, 0).c_str());
    EXPECT_STREQ("b", csvModel.rawStringPtrAt(0, 1).c_str());
    EXPECT_STREQ("1c2", csvModel.rawStringPtrAt(1, 0).c_str());
    EXPECT_STREQ("d", csvModel.rawStringPtrAt(1, 1).c_str());
}

TEST(dfgQt, CsvTableView_evaluateSelectionAsFormula)
{
    using namespace ::DFG_MODULE_NS(qt);
    CsvItemModel csvModel;
    CsvTableView view(nullptr, nullptr);
    QSortFilterProxyModel viewModel;
    viewModel.setSourceModel(&csvModel);
    view.setModel(&viewModel);

    csvModel.insertRows(0, 3);
    csvModel.insertColumns(0, 2);

    csvModel.setDataNoUndo(0, 0, DFG_UTF8("=-1.5+2"));
    csvModel.setDataNoUndo(0, 1, DFG_UTF8("abc"));
    csvModel.setDataNoUndo(1, 0, DFG_UTF8(""));
    csvModel.setDataNoUndo(1, 1, DFG_UTF8("hypot(3, 4) + 10^2"));
    csvModel.setDataNoUndo(2, 0, DFG_UTF8("1.5+2"));
    csvModel.setDataNoUndo(2, 1, DFG_UTF8("-5+(2,5*2)"));

    // Selecting first, testing evaluation and undo/redo
    {
        view.makeSingleCellSelection(0, 0);
        view.evaluateSelectionAsFormula();
        EXPECT_EQ("0.5", csvModel.rawStringViewAt(0, 0).asUntypedView());
        view.undo();
        EXPECT_EQ("=-1.5+2", csvModel.rawStringViewAt(0, 0).asUntypedView());
        view.redo();
        EXPECT_EQ("0.5", csvModel.rawStringViewAt(0, 0).asUntypedView());
        view.undo();
        EXPECT_EQ("=-1.5+2", csvModel.rawStringViewAt(0, 0).asUntypedView());
    }

    // Selecting all, testing evaluation and undo/redo
    {
        view.selectAll();
        view.evaluateSelectionAsFormula();
        EXPECT_EQ("0.5", csvModel.rawStringViewAt(0, 0).asUntypedView());
        EXPECT_EQ("abc", csvModel.rawStringViewAt(0, 1).asUntypedView());
        EXPECT_EQ("", csvModel.rawStringViewAt(1, 0).asUntypedView());
        EXPECT_EQ("105", csvModel.rawStringViewAt(1, 1).asUntypedView());
        EXPECT_EQ("3.5", csvModel.rawStringViewAt(2, 0).asUntypedView());
        EXPECT_EQ("-5+(2,5*2)", csvModel.rawStringViewAt(2, 1).asUntypedView());

        view.undo();
        EXPECT_EQ("=-1.5+2", csvModel.rawStringViewAt(0, 0).asUntypedView());
        EXPECT_EQ("abc", csvModel.rawStringViewAt(0, 1).asUntypedView());
        EXPECT_EQ("", csvModel.rawStringViewAt(1, 0).asUntypedView());
        EXPECT_EQ("hypot(3, 4) + 10^2", csvModel.rawStringViewAt(1, 1).asUntypedView());
        EXPECT_EQ("1.5+2", csvModel.rawStringViewAt(2, 0).asUntypedView());
        EXPECT_EQ("-5+(2,5*2)", csvModel.rawStringViewAt(2, 1).asUntypedView());

        view.redo();
        EXPECT_EQ("0.5", csvModel.rawStringViewAt(0, 0).asUntypedView());
        EXPECT_EQ("abc", csvModel.rawStringViewAt(0, 1).asUntypedView());
        EXPECT_EQ("", csvModel.rawStringViewAt(1, 0).asUntypedView());
        EXPECT_EQ("105", csvModel.rawStringViewAt(1, 1).asUntypedView());
        EXPECT_EQ("3.5", csvModel.rawStringViewAt(2, 0).asUntypedView());
        EXPECT_EQ("-5+(2,5*2)", csvModel.rawStringViewAt(2, 1).asUntypedView());
    }
}

TEST(dfgQt, CsvTableView_stringToDouble)
{
    using namespace ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS;
    using DataType = ::DFG_MODULE_NS(qt)::ChartDataType;

    // yyyy-MM
    testDateToDouble("2020-05", DataType::dateOnlyYearMonth, 6 * (60 * 60 * 24));
    testDateToDouble(" 2020-05", DataType::dateOnlyYearMonth, 6 * (60 * 60 * 24)); // Tests trimming whitespaces in front.
    // yyyy-MM-dd
    testDateToDouble("2020-04-25", DataType::dateOnly, 0);
    //yyyy-MM-dd Wd
    testDateToDouble("2020-04-25 la", DataType::dateOnly, 0);
    // yyyy-MM-dd[T ]hh:mm:ss
    testDateToDouble("2020-04-25T12:34:56", DataType::dateAndTime, 60 * (12 * 60 + 34) + 56);
    testDateToDouble("2020-04-25 12:34:56", DataType::dateAndTime, 60 * (12 * 60 + 34) + 56);
    // yyyy-MM-dd[T ]hh:mm:ss.123
    testDateToDouble("2020-04-25T12:34:56.123", DataType::dateAndTimeMillisecond, 60 * (12 * 60 + 34) + 56 + 0.123);
    testDateToDouble("2020-04-25 12:34:56.123", DataType::dateAndTimeMillisecond, 60 * (12 * 60 + 34) + 56 + 0.123);
    // yyyy-MM-dd[T ]hh:mm:ss[Z|+HH:00]
    testDateToDouble("2020-04-25T00:00:00Z", DataType::dateAndTimeTz, 0);
    testDateToDouble("2020-04-25T00:00:00+03", DataType::dateAndTimeTz, -3 * 60 * 60);
    testDateToDouble("2020-04-25T00:00:00+03:00", DataType::dateAndTimeTz, -3 * 60 * 60);
    testDateToDouble("2020-04-25T00:00:00-03", DataType::dateAndTimeTz, 3 * 60 * 60);
    testDateToDouble("2020-04-25T00:00:00-03:00", DataType::dateAndTimeTz, 3 * 60 * 60);
    // yyyy-MM-dd[T ]hh:mm:ss.zzz[Z|+HH:00]
    testDateToDouble("2020-04-25T00:00:00.125Z", DataType::dateAndTimeMillisecondTz, 0.125);
    testDateToDouble("2020-04-25T00:00:00.125+03", DataType::dateAndTimeMillisecondTz, -3 * 60 * 60 + 0.125);
    testDateToDouble("2020-04-25T00:00:00.125+03:00", DataType::dateAndTimeMillisecondTz, -3 * 60 * 60 + 0.125);
    testDateToDouble("2020-04-25T00:00:00.125-03", DataType::dateAndTimeMillisecondTz, 3 * 60 * 60 + 0.125);
    testDateToDouble("2020-04-25T00:00:00.125-03:00", DataType::dateAndTimeMillisecondTz, 3 * 60 * 60 + 0.125);

    //dd.MM.yyyy et al.
    {
        testDateToDouble("25.04.2020", DataType::dateOnly, 0);
        testDateToDouble("la 25.04.2020", DataType::dateOnly, 0);
        testDateToDouble("25.4.2020", DataType::dateOnly, 0);
        testDateToDouble("1.5.2020", DataType::dateOnly, 6 * 24 * 60 * 60);
        testDateToDouble("pe 1.5.2020", DataType::dateOnly, 6 * 24 * 60 * 60);

        const double baseDate20221218 = static_cast<double>(QDateTime::fromString(QStringLiteral("2022-12-18T00:00:00Z"), Qt::ISODate).toMSecsSinceEpoch()) / 1000.0;
        testDateToDouble("18.12.2022 12:01", DataType::dateAndTime, 12 * 60 * 60 + 1 * 60, baseDate20221218);
        testDateToDouble("18.12.2022 1:01", DataType::dateAndTime, 1 * 60 * 60 + 1 * 60, baseDate20221218);
        testDateToDouble("   18.12.2022 1:01", DataType::dateAndTime, 1 * 60 * 60 + 1 * 60, baseDate20221218); // Tests trimming whitespaces in front.
        testDateToDouble("18.12.2022 1:01:02", DataType::dateAndTime, 1 * 60 * 60 + 1 * 60 + 2, baseDate20221218);
        testDateToDouble("18.12.2022 1:01:02.125", DataType::dateAndTimeMillisecond, 1 * 60 * 60 + 1 * 60 + 2 + 0.125, baseDate20221218);
        testDateToDouble("18.12.2022 12:01:02.125", DataType::dateAndTimeMillisecond, 12 * 60 * 60 + 1 * 60 + 2 + 0.125, baseDate20221218);
        testDateToDouble("su 18.12.2022 12:01:02.125", DataType::dateAndTimeMillisecond, 12 * 60 * 60 + 1 * 60 + 2 + 0.125, baseDate20221218);
    }

    // Times only
    {
        // hh:mm
        testDateToDouble("12:34", DataType::dayTime, 60 * (12 * 60 + 34), 0);

        // [h]h:mm:ss[.zzz]
        testDateToDouble("12:34:56", DataType::dayTime, 60 * (12 * 60 + 34) + 56, 0);
        testDateToDouble("12:34:56.123", DataType::dayTimeMillisecond, 60 * (12 * 60 + 34) + 56 + 0.123, 0);
        testDateToDouble("2:34:56", DataType::dayTime, 60 * (2 * 60 + 34) + 56, 0);
        testDateToDouble("2:34:56.125", DataType::dayTimeMillisecond, 60 * (2 * 60 + 34) + 56 + 0.125, 0);
    }

    // Testing that invalid format produce NaN's
    {
        testDateToDouble_invalidDateFormat("2020-4-25");
        testDateToDouble_invalidDateFormat("2020-04-25la");
        testDateToDouble_invalidDateFormat("2020-04-25 abc");
        testDateToDouble_invalidDateFormat("2020-04-25a");
        testDateToDouble_invalidDateFormat("2020-04-31"); // Invalid day value
        testDateToDouble_invalidDateFormat("20-04-25");
        testDateToDouble_invalidDateFormat("25-04-2020");

        testDateToDouble_invalidDateFormat("2020-04-25a12:34:56");
        testDateToDouble_invalidDateFormat("25-04-2020T12:34:56");
        testDateToDouble_invalidDateFormat("2020-4-25T32:34:56");
        testDateToDouble_invalidDateFormat("2020-13-25T12:34:56"); // Invalid month value
        testDateToDouble_invalidDateFormat("2020-04-25T32:34:56"); // Invalid time value
        testDateToDouble_invalidDateFormat("2020-04-25T12:34:56.");
        testDateToDouble_invalidDateFormat("2020-04-25T12:34:56.1");
        testDateToDouble_invalidDateFormat("2020-04-25T12:34:56.12");
        testDateToDouble_invalidDateFormat("2020-04-25T12:34:56.abc");

        testDateToDouble_invalidDateFormat("abc 31.4.2020");
        testDateToDouble_invalidDateFormat("31.4.2020"); // Invalid day
        testDateToDouble_invalidDateFormat("31..2020");
        testDateToDouble_invalidDateFormat("1.13.2020"); // Invalid month
        testDateToDouble_invalidDateFormat("1.-1.2020"); // Invalid month
        testDateToDouble_invalidDateFormat("1.0.2020"); // Invalid month

        testDateToDouble_invalidDateFormat("18.12.2022 123:01"); // Invalid hour
        testDateToDouble_invalidDateFormat("18.12.2022 12:01:1"); // Invalid second
        testDateToDouble_invalidDateFormat("18.12.2022 12:01:01.1234"); // Invalid millisecond
        testDateToDouble_invalidDateFormat("18.12.2022 ab:12"); // Invalid hour
        testDateToDouble_invalidDateFormat("18.12.2022 12:ab"); // Invalid minute
        testDateToDouble_invalidDateFormat("18.12.2022 12.23"); // Unsupported hour-minute separator

        testDateToDouble_invalidDateFormat("32:34:56");
        testDateToDouble_invalidDateFormat("32:34::56");
        testDateToDouble_invalidDateFormat("12::34:56");
        testDateToDouble_invalidDateFormat("12:34:56.");
        testDateToDouble_invalidDateFormat("12:34:56.1");
        testDateToDouble_invalidDateFormat("12:34:56.12");
    }

    // double values
    {
        using namespace ::DFG_MODULE_NS(qt);
        using namespace ::DFG_MODULE_NS(str);
        EXPECT_EQ(strTo<double>("-0.0"), GraphDataSource::cellStringToDouble(DFG_UTF8("-0.0")));
        EXPECT_EQ(0.25, GraphDataSource::cellStringToDouble(DFG_UTF8("    0.25  ")));
        EXPECT_EQ(-0.25, GraphDataSource::cellStringToDouble(DFG_UTF8("  -25E-2  ")));
        EXPECT_EQ(strTo<double>("1.23456789"), GraphDataSource::cellStringToDouble(DFG_UTF8("1.23456789")));
        EXPECT_EQ(strTo<double>("1e300"), GraphDataSource::cellStringToDouble(DFG_UTF8("1e300")));
        EXPECT_EQ(strTo<double>("1e-9"), GraphDataSource::cellStringToDouble(DFG_UTF8("1e-9")));
        EXPECT_EQ(strTo<double>("-1e-9"), GraphDataSource::cellStringToDouble(DFG_UTF8("-1e-9")));
        EXPECT_EQ(-std::numeric_limits<double>::infinity(), GraphDataSource::cellStringToDouble(DFG_UTF8("-inf")));
        EXPECT_EQ(std::numeric_limits<double>::infinity(), GraphDataSource::cellStringToDouble(DFG_UTF8("inf")));
    }
}

TEST(dfgQt, CsvTableView_saveAsShown)
{
    using namespace ::DFG_MODULE_NS(qt);
    CsvItemModel csvModel;
    CsvTableView view(nullptr, nullptr);
    QSortFilterProxyModel viewModel;
    viewModel.setSourceModel(&csvModel);
    viewModel.setDynamicSortFilter(true);
    view.setModel(&viewModel);

    ASSERT_TRUE(csvModel.openString(
    "a,4\n"
    "bc,3\n"
    "c,2\n"
    "d,1\n"
    ));

    viewModel.setFilterWildcard("*c");
    viewModel.sort(1);
    ::DFG_MODULE_NS(os)::TemporaryFileStream tempFile;
    tempFile.setAutoRemove(false);
    CsvItemModel::SaveOptions saveOptions(&csvModel);
    saveOptions.setProperty("CsvTableView_saveAsShown", "1");
    const auto sTempPath = QString::fromStdWString(tempFile.path());
    ASSERT_TRUE(view.saveToFileImpl(sTempPath, saveOptions));
    tempFile.close(); // Following open seemed to fail if not closed.

    view.createNewTable();
    view.openFile(sTempPath);

    EXPECT_EQ(2, viewModel.rowCount());
    EXPECT_EQ(2, viewModel.columnCount());
    EXPECT_EQ("c", viewModel.data(viewModel.index(0, 0)));
    EXPECT_EQ("2", viewModel.data(viewModel.index(0, 1)));
    EXPECT_EQ("bc", viewModel.data(viewModel.index(1, 0)));
    EXPECT_EQ("3", viewModel.data(viewModel.index(1, 1)));
    EXPECT_TRUE(QFile::remove(sTempPath));
}

TEST(dfgQt, CsvTableView_dateTimeToString)
{
    using namespace ::DFG_MODULE_NS(qt);
    CsvItemModel csvModel;
    CsvTableView view(nullptr, nullptr);
    view.setModel(&csvModel);

    // Testing default names
    EXPECT_EQ("sa 2021-09-18", view.dateTimeToString(QDate(2021, 9, 18), "WD yyyy-MM-dd"));
    EXPECT_EQ("sa 2021-09-18 12:01:02.345", view.dateTimeToString(QDateTime(QDate(2021, 9, 18), QTime(12, 1, 2, 345)), "WD yyyy-MM-dd hh:mm:ss.zzz"));

    // Testing view-specific names
    view.setProperty("CsvTableView_weekDayNames", "a,b,c,d,e,Saturday,g");
    EXPECT_EQ("Saturday 2021-09-18", view.dateTimeToString(QDate(2021, 9, 18), "WD yyyy-MM-dd"));
    EXPECT_EQ("Saturday 2021-09-18 12:01:02.345", view.dateTimeToString(QDateTime(QDate(2021, 9, 18), QTime(12, 1, 2, 345)), "WD yyyy-MM-dd hh:mm:ss.zzz"));

    // Testing .conf-specific names.
    EXPECT_TRUE(view.openFile("testfiles/example_with_conf0.csv"));
    EXPECT_EQ("sat 2021-09-18", view.dateTimeToString(QDate(2021, 9, 18), "WD yyyy-MM-dd"));
    EXPECT_EQ("sat 2021-09-18 12:01:02.345", view.dateTimeToString(QDateTime(QDate(2021, 9, 18), QTime(12, 1, 2, 345)), "WD yyyy-MM-dd hh:mm:ss.zzz"));

    // Testing that a new table doesn't use settings from previously opened table
    view.createNewTable();
    EXPECT_EQ("Saturday 2021-09-18", view.dateTimeToString(QDate(2021, 9, 18), "WD yyyy-MM-dd"));
    EXPECT_EQ("Saturday 2021-09-18 12:01:02.345", view.dateTimeToString(QDateTime(QDate(2021, 9, 18), QTime(12, 1, 2, 345)), "WD yyyy-MM-dd hh:mm:ss.zzz"));
}

TEST(dfgQt, CsvTableView_insertDateTimes)
{
    using namespace ::DFG_MODULE_NS(qt);
    CsvItemModel csvModel;
    CsvTableView view(nullptr, nullptr);
    view.setModel(&csvModel);

    csvModel.insertRows(0, 3);
    csvModel.insertColumns(0, 1);

    QTime insertedTime;
    QDate insertedDate;
    QDateTime insertedDateTime;
    const auto doInserts = [&]()
    {
        view.selectCell(0, 0);
        insertedTime = view.insertTime();
        view.selectCell(1, 0);
        insertedDate = view.insertDate();
        view.selectCell(2, 0);
        insertedDateTime = view.insertDateTime();
    };

    const auto testInserts = [&](const QString sTimeFormat, const QString sDateFormat, const QString sDateTimeFormat)
    {
        const auto sTime = view.dateTimeToString(insertedTime, sTimeFormat);
        const auto sDate = view.dateTimeToString(insertedDate, sDateFormat);
        const auto sDateTime = view.dateTimeToString(insertedDateTime, sDateTimeFormat);
        EXPECT_FALSE(sTime.isEmpty());
        EXPECT_FALSE(sDate.isEmpty());
        EXPECT_FALSE(sDateTime.isEmpty());
        EXPECT_EQ(sTime, viewToQString(csvModel.rawStringViewAt(0, 0)));
        EXPECT_EQ(sDate, viewToQString(csvModel.rawStringViewAt(1, 0)));
        EXPECT_EQ(sDateTime, viewToQString(csvModel.rawStringViewAt(2, 0)));
    };

    // Testing default formats
    doInserts();
    testInserts("hh:mm:ss.zzz", "yyyy-MM-dd", "yyyy-MM-dd hh:mm:ss.zzz");

    // Testing view-specific names
    view.setProperty("CsvTableView_timeFormat", "hh:mm:ss");
    view.setProperty("CsvTableView_dateFormat", "dd.MM.yyyy");
    view.setProperty("CsvTableView_dateTimeFormat", "yyyy hh");
    doInserts();
    testInserts("hh:mm:ss", "dd.MM.yyyy", "yyyy hh");

    // Testing .conf-specific names.
    EXPECT_TRUE(view.openFile("testfiles/example_with_conf0.csv"));
    csvModel.insertRows(0, 3);
    doInserts();
    testInserts("hh:mm", "WD dd", "WD dd.MM hh:mm");

    // Testing that a new table doesn't use settings from previously opened table
    csvModel.setModifiedStatus(false); // To prevent messagebox from opening.
    view.createNewTable();
    csvModel.insertRows(0, 3);
    csvModel.insertColumns(0, 1);
    doInserts();
    testInserts("hh:mm:ss", "dd.MM.yyyy", "yyyy hh");
}

TEST(dfgQt, CsvTableView_insertDateTimesUndoRedo)
{
    using namespace ::DFG_MODULE_NS(qt);
    CsvItemModel csvModel;
    CsvTableView view(nullptr, nullptr);
    QSortFilterProxyModel viewModel;
    viewModel.setSourceModel(&csvModel);
    viewModel.setDynamicSortFilter(true);
    view.setModel(&viewModel);

    ASSERT_TRUE(csvModel.openString(
    ",\n"
    "a,4\n"
    "bc,3\n"
    "c,2\n"
    "d,1\n"
    ));

    viewModel.setFilterWildcard("*c");
    viewModel.sort(1);

    const auto viewModelContent = [&](const int r, const int c)
    {
        const auto index = viewModel.index(r, c);
        const auto s = viewModel.data(index);
        return s.toString();
    };

    // Now view table should be
    // c, 2
    // bc, 3
    DFGTEST_EXPECT_LEFT("c", viewModelContent(0, 0));
    DFGTEST_EXPECT_LEFT("bc", viewModelContent(1, 0));
    DFGTEST_EXPECT_LEFT("2", viewModelContent(0, 1));
    DFGTEST_EXPECT_LEFT("3", viewModelContent(1, 1));

    view.selectColumn(1);
    view.insertGeneric("test", "test insert");

    view.resetSorting();
    viewModel.setFilterWildcard(QString());

    DFGTEST_EXPECT_LEFT("a",  viewModelContent(0, 0));
    DFGTEST_EXPECT_LEFT("bc", viewModelContent(1, 0));
    DFGTEST_EXPECT_LEFT("c",  viewModelContent(2, 0));
    DFGTEST_EXPECT_LEFT("d",  viewModelContent(3, 0));

    DFGTEST_EXPECT_LEFT("4",    viewModelContent(0, 1));
    DFGTEST_EXPECT_LEFT("test", viewModelContent(1, 1));
    DFGTEST_EXPECT_LEFT("test", viewModelContent(2, 1));
    DFGTEST_EXPECT_LEFT("1",    viewModelContent(3, 1));

    view.undo();

    DFGTEST_EXPECT_LEFT("a",  viewModelContent(0, 0));
    DFGTEST_EXPECT_LEFT("bc", viewModelContent(1, 0));
    DFGTEST_EXPECT_LEFT("c",  viewModelContent(2, 0));
    DFGTEST_EXPECT_LEFT("d",  viewModelContent(3, 0));

    DFGTEST_EXPECT_LEFT("4",    viewModelContent(0, 1));
    DFGTEST_EXPECT_LEFT("3", viewModelContent(1, 1));
    DFGTEST_EXPECT_LEFT("2", viewModelContent(2, 1));
    DFGTEST_EXPECT_LEFT("1",    viewModelContent(3, 1));

    view.redo();

    DFGTEST_EXPECT_LEFT("a",  viewModelContent(0, 0));
    DFGTEST_EXPECT_LEFT("bc", viewModelContent(1, 0));
    DFGTEST_EXPECT_LEFT("c",  viewModelContent(2, 0));
    DFGTEST_EXPECT_LEFT("d",  viewModelContent(3, 0));

    DFGTEST_EXPECT_LEFT("4",    viewModelContent(0, 1));
    DFGTEST_EXPECT_LEFT("test", viewModelContent(1, 1));
    DFGTEST_EXPECT_LEFT("test", viewModelContent(2, 1));
    DFGTEST_EXPECT_LEFT("1",    viewModelContent(3, 1));

    view.selectCell(2, 1);
    view.insertGeneric("test2", "test insert 2");
    DFGTEST_EXPECT_LEFT("test2", viewModelContent(2, 1));
    view.undo();
    DFGTEST_EXPECT_LEFT("test", viewModelContent(2, 1));
    view.redo();
    DFGTEST_EXPECT_LEFT("test2", viewModelContent(2, 1));
}

TEST(dfgQt, CsvTableView_resizeTableNoUi)
{
    using namespace DFG_MODULE_NS(qt);
    CsvItemModel csvModel;
    CsvTableView view(nullptr, nullptr);
    view.setModel(&csvModel);

    view.resizeTableNoUi(-1, -1);
    EXPECT_TRUE(view.resizeTableNoUi(1, -1));
    EXPECT_EQ(1, csvModel.rowCount());
    EXPECT_EQ(0, csvModel.columnCount());
    EXPECT_TRUE(view.resizeTableNoUi(-1, 1));
    EXPECT_EQ(1, csvModel.rowCount());
    EXPECT_EQ(1, csvModel.columnCount());
    csvModel.setDataNoUndo(0, 0, DFG_UTF8("abc"));
    EXPECT_TRUE(view.resizeTableNoUi(10, 500));
    EXPECT_EQ(10, csvModel.rowCount());
    EXPECT_EQ(500, csvModel.columnCount());
    EXPECT_TRUE(view.resizeTableNoUi(200, 30));
    EXPECT_EQ(200, csvModel.rowCount());
    EXPECT_EQ(30, csvModel.columnCount());
    EXPECT_TRUE(view.resizeTableNoUi(1, 1));
    EXPECT_EQ("abc", csvModel.rawStringViewAt(0, 0).asUntypedView());
    EXPECT_TRUE(view.resizeTableNoUi(0, 0));
    EXPECT_EQ(0, csvModel.rowCount());
    EXPECT_EQ(0, csvModel.columnCount());
}

TEST(dfgQt, CsvTableView_transpose)
{
    using namespace ::DFG_MODULE_NS(qt);
    CsvItemModel csvModel;
    CsvTableView view(nullptr, nullptr);
    view.setModel(&csvModel);

    // Empty table (making sure it doesn't crash etc)
    DFGTEST_EXPECT_TRUE(view.transpose());

    // Single item table
    view.resizeTableNoUi(1, 1);
    csvModel.setDataNoUndo(0, 0, DFG_UTF8("abc"));
    DFGTEST_EXPECT_TRUE(view.transpose());
    DFGTEST_EXPECT_LEFT("abc", csvModel.rawStringViewAt(0, 0).asUntypedView());

    const auto populateTable = [&](const int nRows, const int nCols)
    {
        view.resizeTableNoUi(nRows, nCols);
        for (int r = 0; r < nRows; ++r)
        {
            for (int c = 0; c < nCols; ++c)
            {
                csvModel.setDataNoUndo(r, c, QString("%1,%2").arg(r).arg(c));
            }
        }
    };

    const auto verifyTable = [&](const int r, const int c)
    {
        DFGTEST_EXPECT_LEFT(r, csvModel.rowCount());
        DFGTEST_EXPECT_LEFT(c, csvModel.columnCount());
        for (int r = 0; r < csvModel.rowCount(); ++r)
        {
            for (int c = 0; c < csvModel.columnCount(); ++c)
            {
                DFGTEST_EXPECT_LEFT(QString("%1,%2").arg(c).arg(r), viewToQString(csvModel.rawStringViewAt(r, c)));
            }
        }
    };

    // Square
    populateTable(3, 3);
    DFGTEST_EXPECT_TRUE(view.transpose());
    verifyTable(3, 3);

    // Wide
    populateTable(5, 20);
    DFGTEST_EXPECT_TRUE(view.transpose());
    verifyTable(20, 5);

    // Tall
    populateTable(17, 8);
    DFGTEST_EXPECT_TRUE(view.transpose());
    verifyTable(8, 17);
}

TEST(dfgQt, CsvTableView_contextMenuSize)
{
    // Context menu is already big, but making sure that it is at least small enough to fit to single context menu column on some typical resolution.
    using namespace ::DFG_MODULE_NS(qt);
    CsvItemModel csvModel;
    CsvTableView view(nullptr, nullptr);
    view.setModel(&csvModel);

    view.setReadOnlyMode(false);
    const auto actions = view.actions();
    size_t nCount = 0;
    for (auto pAct : actions)
    {
        if (pAct && pAct->isVisible() && !pAct->isSeparator())
            ++nCount;
    }
    DFGTEST_EXPECT_LE(nCount, 38); // A semi-arbitrary value.
}

TEST(dfgQt, CsvTableView_columnIndexMappingWithHiddenColumns)
{
    using namespace ::DFG_MODULE_NS(qt);
    CsvItemModel csvModel;
    CsvTableView view(nullptr, nullptr);
    CsvTableViewSortFilterProxyModel viewModel(&view);
    viewModel.setSourceModel(&csvModel);
    view.setModel(&viewModel);

    const int nTotalColumnCount = 5;
    DFGTEST_ASSERT_TRUE(csvModel.setSize(3, nTotalColumnCount));

    // Setting column names
    for (int i = 0; i < nTotalColumnCount; ++i)
        csvModel.setColumnName(i, QString(QChar('a' + i)));

    // Hiding 2. and 3. columns
    view.setColumnVisibility(1, false);
    view.setColumnVisibility(2, false);

    DFGTEST_ASSERT_EQ(3, view.model()->columnCount());

    // Testing data-to-view-index conversions
    DFGTEST_EXPECT_EQ(0,  view.columnIndexDataToView(ColumnIndex_data(0)).value());
    DFGTEST_EXPECT_EQ(-1, view.columnIndexDataToView(ColumnIndex_data(1)).value());
    DFGTEST_EXPECT_EQ(-1, view.columnIndexDataToView(ColumnIndex_data(2)).value());
    DFGTEST_EXPECT_EQ(1,  view.columnIndexDataToView(ColumnIndex_data(3)).value());
    DFGTEST_EXPECT_EQ(2,  view.columnIndexDataToView(ColumnIndex_data(4)).value());
    DFGTEST_EXPECT_EQ(-1, view.columnIndexDataToView(ColumnIndex_data(5)).value());

    // Testing view-to-data-index conversions
    DFGTEST_EXPECT_EQ(0,  view.columnIndexViewToData(ColumnIndex_view(0)).value());
    DFGTEST_EXPECT_EQ(3,  view.columnIndexViewToData(ColumnIndex_view(1)).value());
    DFGTEST_EXPECT_EQ(4,  view.columnIndexViewToData(ColumnIndex_view(2)).value());
    DFGTEST_EXPECT_EQ(-1, view.columnIndexViewToData(ColumnIndex_view(3)).value());
    DFGTEST_EXPECT_EQ(-1, view.columnIndexViewToData(ColumnIndex_view(4)).value());

    // Testing that column names are correct
    {
        for (int i = 0; i < nTotalColumnCount; ++i)
            EXPECT_EQ(QString(QChar('a' + i)), view.getColumnName(ColumnIndex_data(i)));

        EXPECT_EQ("a", view.getColumnName(ColumnIndex_view(0)));
        EXPECT_EQ("d", view.getColumnName(ColumnIndex_view(1)));
        EXPECT_EQ("e", view.getColumnName(ColumnIndex_view(2)));
    }
}

TEST(dfgQt, CsvTableView_generateContentByFormula_cellValue)
{
    using namespace ::DFG_MODULE_NS(qt);
    CsvItemModel csvModel;
    CsvTableView view(nullptr, nullptr);
    view.setModel(&csvModel);

    csvModel.openString(
        ",\n"
        "1.25,2021-11-22 12:01:02\n"
        "\"2,5\",2021-11-22 12:01:02.125+00:00\n"
        "3,2021-11-22 12:01:02.125+01:00\n"
        "4,00:00:00\n"
        "5,13:01:02.125\n"
        "6,24:00:00\n"
    );

    DFGTEST_EXPECT_EQ_LITERAL_UTF8("1.25", csvModel.rawStringViewAt(0, 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("2,5", csvModel.rawStringViewAt(1, 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("3", csvModel.rawStringViewAt(2, 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("4", csvModel.rawStringViewAt(3, 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("5", csvModel.rawStringViewAt(4, 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("6", csvModel.rawStringViewAt(5, 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("2021-11-22 12:01:02", csvModel.rawStringViewAt(0, 1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("2021-11-22 12:01:02.125+00:00", csvModel.rawStringViewAt(1, 1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("2021-11-22 12:01:02.125+01:00", csvModel.rawStringViewAt(2, 1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("00:00:00", csvModel.rawStringViewAt(3, 1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("13:01:02.125", csvModel.rawStringViewAt(4, 1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("24:00:00", csvModel.rawStringViewAt(5, 1));

    // Transforming values with formula generator, text at (r, c) -> cellValue(r, c)
    // Dates are expected to be converted into numeric values.
    {
        CsvItemModel generateParamModel;
        generateParamModel.openString(
                    "\t\n"
                    "Target\tWhole table\n"
                    "Generator\tFormula\n"
                    "Formula\tcellValue(trow, tcol)\n"
                    "Format type\tg\n"
                    "Format precision\t14");
        view.generateContentImpl(generateParamModel);
    }

    DFGTEST_EXPECT_EQ_LITERAL_UTF8("1.25", csvModel.rawStringViewAt(0, 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("2.5", csvModel.rawStringViewAt(1, 0)); // This tests for (hacky) 'comma as dot'-conversion.
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("3", csvModel.rawStringViewAt(2, 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("4", csvModel.rawStringViewAt(3, 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("5", csvModel.rawStringViewAt(4, 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("6", csvModel.rawStringViewAt(5, 0));

    // Dates without timezone specifier are interpreted as UTC
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("1637582462", csvModel.rawStringViewAt(0, 1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("1637582462.125", csvModel.rawStringViewAt(1, 1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("1637578862.125", csvModel.rawStringViewAt(2, 1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("0", csvModel.rawStringViewAt(3, 1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("46862.125", csvModel.rawStringViewAt(4, 1));
    // 24:00:00 is not a valid QTime so 24:00:00 cellValue() returns nan.
    //DFGTEST_EXPECT_EQ_LITERAL_UTF8("86400", csvModel.rawStringViewAt(5, 1));
}

TEST(dfgQt, CsvTableView_generateContentByFormula_cellValue_dateHandling)
{
    using namespace ::DFG_MODULE_NS(qt);
    CsvItemModel csvModel;
    CsvTableView view(nullptr, nullptr);
    view.setModel(&csvModel);

    // 2021-11-25T10:01:02Z = 1637834462 s
    const QString sExpectedLocal = [] { auto dt = QDateTime::fromMSecsSinceEpoch(1637834462125); dt.setTimeSpec(Qt::LocalTime); return dt;}().toString("yyyy-MM-dd hh:mm:ss.zzz");

    DFGTEST_ASSERT_FALSE(sExpectedLocal.isEmpty());

    // First column has expected strings, second has items that are converted.
    csvModel.openString(
        QString(",\n"
        "2021-11-22 mo 12:01:02.125,2021-11-22 12:01:02.125\n"       // Tests round-trip with exact decimal
        "2021-11-23 tu 12:01:02.333,2021-11-23 12:01:02.333\n"       // Tests round-trip with inexact decimal
        "2021-11-24 we 10:01:02.125,2021-11-24 12:01:02.125+02:00\n" // Tests round-trip with timezone info
        "2021-11-25 Thu 10:01:02.125,1637834462.125\n"               // Tests value to utc date handling with seconds.
        "2021-11-25 10:01:02.125,1637834462125\n"                    // Tests value to utc date handling with milliseconds.
        "%1,1637834462.125\n"                                        // Tests value to local date handling with seconds.
        "%1,1637834462125\n").arg(sExpectedLocal)                    // Tests value to local date handling with milliseconds.
    );

    CsvItemModel generateParamModel;
    generateParamModel.openString(
                "\t\n"
                "Target\tSelection\n"
                "Generator\tFormula\n"
                "Formula\tcellValue(trow, tcol)\n"
                "Format type\tg\n"
                "Format precision\t14");

    // Converting dates on second column into epoch time values
    for (int r = 0; r < 3; ++r)
    {
        view.selectCell(r, 1);
        view.generateContentImpl(generateParamModel);
    }

    generateParamModel.setDataNoUndo(3, 1, DFG_UTF8("date_sec_utc"));
    generateParamModel.setDataNoUndo(4, 1, DFG_UTF8("yyyy-MM-dd WD hh:mm:ss.zzz"));
    for (int r = 0; r < 4; ++r)
    {
        if (r == 3) // For the last one in the first four, using different weekday format
            view.setProperty("CsvTableView_weekDayNames", "Mon,Tue,Wed,Thu,Fri,Sat,Sun");
        view.selectCell(r, 1);
        view.generateContentImpl(generateParamModel);
    }

    generateParamModel.setDataNoUndo(3, 1, DFG_UTF8("date_msec_utc"));
    generateParamModel.setDataNoUndo(4, 1, DFG_UTF8("yyyy-MM-dd hh:mm:ss.zzz"));
    for (int r = 4; r < 7; ++r)
    {
        if (r == 4)
            generateParamModel.setDataNoUndo(3, 1, DFG_UTF8("date_msec_utc"));
        else if (r == 5)
            generateParamModel.setDataNoUndo(3, 1, DFG_UTF8("date_sec_local"));
        else
            generateParamModel.setDataNoUndo(3, 1, DFG_UTF8("date_msec_local"));
        view.selectCell(r, 1);
        view.generateContentImpl(generateParamModel);
    }

    for (int r = 0; r < csvModel.rowCount(); ++r)
    {
        DFGTEST_EXPECT_LEFT(csvModel.rawStringViewAt(r, 0).asUntypedView(), csvModel.rawStringViewAt(r, 1).asUntypedView());
    }
}

TEST(dfgQt, CsvTableView_generateContent_numberFormat)
{
    using namespace ::DFG_ROOT_NS;
    using namespace ::DFG_MODULE_NS(math);
    using namespace ::DFG_MODULE_NS(qt);
    using namespace ::DFG_MODULE_NS(str);
    CsvItemModel csvModel;
    CsvTableView view(nullptr, nullptr);
    view.setModel(&csvModel);

    view.resizeTableNoUi(1, 1);

    CsvItemModel generateModel;
    generateModel.openString(
                "\t\n"
                "Target\tSelection\n"
                "Generator\tFormula\n"
                "Formula\t1.25\n"
                "Format type\tg\n"
                "Format precision\t14");

    const auto createGenerateParam = [&](const double value, const char* pszType, const char* pszPrecision) -> CsvItemModel&
    {
        generateModel.setDataNoUndo(2, 1, SzPtrUtf8(toStrC(value).c_str()));
        generateModel.setDataNoUndo(3, 1, SzPtrUtf8(pszType));
        generateModel.setDataNoUndo(4, 1, SzPtrUtf8(pszPrecision));
        return generateModel;
    };

    const auto isAnyOf = [](const StringViewC sv, const std::array<const char*, 2>& arr) { return std::find(arr.begin(), arr.end(), sv) != arr.end(); };

    view.selectCell(0, 0);
    view.generateContentImpl(createGenerateParam(1.25, "g", "14"));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("1.25", csvModel.rawStringViewAt(0, 0));

    view.generateContentImpl(createGenerateParam(1.25, "g", "1"));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("1", csvModel.rawStringViewAt(0, 0));

    // Testing empty precision: it is to be interpreted as default roundtrippable representation
    {
        view.generateContentImpl(createGenerateParam(1.25, "g", ""));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("1.25", csvModel.rawStringViewAt(0, 0));

        view.generateContentImpl(createGenerateParam(123456789.125, "g", ""));
        DFGTEST_EXPECT_TRUE(isAnyOf(csvModel.rawStringViewAt(0, 0).asUntypedView(), { "123456789.125", "1.23456789125e+08" }));
    }

#if DFG_TOSTR_USING_TO_CHARS_WITH_FLOAT_PREC_ARG == 1
    // a (hex)
    {
        view.generateContentImpl(createGenerateParam(1.265625, "a", ""));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("0x1.44p+0", csvModel.rawStringViewAt(0, 0));

        view.generateContentImpl(createGenerateParam(1.265625, "a", "1"));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("0x1.4p+0", csvModel.rawStringViewAt(0, 0));

        view.generateContentImpl(createGenerateParam(-1.265625, "a", ""));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("-0x1.44p+0", csvModel.rawStringViewAt(0, 0));

        // Testing that negative zero works correctly.
        view.generateContentImpl(createGenerateParam(signCopied(0.0, -1.0), "a", ""));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("-0x0p+0", csvModel.rawStringViewAt(0, 0));
    }

    // e (scientific)
    {
        view.generateContentImpl(createGenerateParam(1.25, "e", ""));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("1.25e+00", csvModel.rawStringViewAt(0, 0));

        view.generateContentImpl(createGenerateParam(1.28, "e", "1"));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("1.3e+00", csvModel.rawStringViewAt(0, 0));
    }

    // f (fixed)
    {
        view.generateContentImpl(createGenerateParam(1.25, "f", ""));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("1.25", csvModel.rawStringViewAt(0, 0));

        view.generateContentImpl(createGenerateParam(1.28, "f", "1"));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("1.3", csvModel.rawStringViewAt(0, 0));

        view.generateContentImpl(createGenerateParam(1.25, "f", "6"));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("1.250000", csvModel.rawStringViewAt(0, 0));
    }
#endif
}

TEST(dfgQt, CsvTableView_columnVisibilityConfPropertyHandling)
{
    using namespace ::DFG_MODULE_NS(qt);
    CsvItemModel csvModel;
    CsvTableView view(nullptr, nullptr);
    view.setModel(&csvModel);

    EXPECT_TRUE(view.openFile("testfiles/example_with_conf0.csv"));
    DFGTEST_EXPECT_TRUE(view.isColumnVisible(ColumnIndex_data(0)));
    DFGTEST_EXPECT_FALSE(view.isColumnVisible(ColumnIndex_data(1)));
    DFGTEST_EXPECT_TRUE(view.isColumnVisible(ColumnIndex_data(2)));
}

TEST(dfgQt, CsvTableView_sorting)
{
    using namespace ::DFG_ROOT_NS;
    using namespace ::DFG_MODULE_NS(io);
    using namespace ::DFG_MODULE_NS(qt);
    CsvItemModel csvModel;
    CsvTableView view(nullptr, nullptr);
    CsvTableViewSortFilterProxyModel viewModel(&view);
    viewModel.setSourceModel(&csvModel);
    view.setModel(&viewModel);

    csvModel.openString(
        ",\n"
        "2,7\n"
        "-1,50\n"
        "-10,8\n"
    );

    CsvFormatDefinition saveFormat(',', '"', EndOfLineTypeN, TextEncoding::encodingUTF8);
    saveFormat.bomWriting(false);
    saveFormat.setProperty(CsvTableView::s_szCsvSaveOption_saveAsShown, "1");

    const auto saveToString = [&]() -> std::string
    {
        const char szFilePath[] = "testfiles/generated/dfgQt_CsvTableView_sorting.csv";
        view.saveToFileImpl(szFilePath, saveFormat);
        const auto bytes = ::DFG_MODULE_NS(io)::fileToMemory_readOnly(szFilePath);
        const auto byteSpan = bytes.asSpan<char>();
        return std::string(byteSpan.begin(), byteSpan.end());
    };

    view.setSortingEnabled(true);

    // Text sorting by first column
    view.sortByColumn(0, Qt::AscendingOrder);
    DFGTEST_EXPECT_LEFT(",\n-1,50\n-10,8\n2,7\n", saveToString());

    // Text sorting by second column
    view.sortByColumn(1, Qt::AscendingOrder);
    DFGTEST_EXPECT_LEFT(",\n-1,50\n2,7\n-10,8\n", saveToString());

    // Number sorting by first column
    csvModel.setColumnType(0, CsvItemModel::ColTypeNumber);
    view.sortByColumn(0, Qt::AscendingOrder);
    DFGTEST_EXPECT_LEFT(",\n-10,8\n-1,50\n2,7\n", saveToString());

    // Number sorting by second column
    csvModel.setColumnType(1, CsvItemModel::ColTypeNumber);
    view.sortByColumn(1, Qt::AscendingOrder);
    DFGTEST_EXPECT_LEFT(",\n2,7\n-10,8\n-1,50\n", saveToString());
}

TEST(dfgQt, CsvTableView_populateCsvConfig)
{
    using namespace ::DFG_MODULE_NS(qt);
    using namespace ::DFG_MODULE_NS(io);
    CsvItemModel csvModel;
    CsvTableView view(nullptr, nullptr);
    view.setModel(&csvModel);

    csvModel.setProperty("CsvItemModel_defaultFormatSeparator", ":");
    csvModel.setProperty("CsvItemModel_defaultFormatEnclosingChar", "'");
    csvModel.setProperty("CsvItemModel_defaultFormatEndOfLine", "\r\n");

    view.resizeTableNoUi(2, 3);
    csvModel.setColumnType(1, CsvItemModel::ColTypeNumber);

    const auto config = view.populateCsvConfig();
    const auto saveOptions = csvModel.getSaveOptions();

    DFGTEST_EXPECT_TRUE(saveOptions.bomWriting());
    DFGTEST_EXPECT_LEFT(':', saveOptions.separatorChar());
    DFGTEST_EXPECT_LEFT('\'', saveOptions.enclosingChar());
    DFGTEST_EXPECT_LEFT(EndOfLineTypeRN, saveOptions.eolType());

    DFGTEST_EXPECT_EQ_LITERAL_UTF8("1", config.value(DFG_UTF8("bom_writing")));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8(":", config.value(DFG_UTF8("separator_char")));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("'", config.value(DFG_UTF8("enclosing_char")));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("\\r\\n", config.value(DFG_UTF8("end_of_line_type")));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("UTF8", config.value(DFG_UTF8("encoding")));

    // Testing that non-printable chars are returned in \xYY format.
    {
        csvModel.setProperty("CsvItemModel_defaultFormatSeparator", "\\x1f");
        const auto config2 = view.populateCsvConfig();
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("\\x1f", config2.value(DFG_UTF8("separator_char")));
    }

    // columnsByIndex
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("number", config.value(DFG_UTF8("columnsByIndex/2/datatype")));
    DFGTEST_EXPECT_TRUE(config.contains(DFG_UTF8("columnsByIndex/1/width_pixels")));
    DFGTEST_EXPECT_TRUE(config.contains(DFG_UTF8("columnsByIndex/2/width_pixels")));
    DFGTEST_EXPECT_TRUE(config.contains(DFG_UTF8("columnsByIndex/3/width_pixels")));

    // These properties are filled by TableEditor so not available here.
    //DFGTEST_EXPECT_TRUE(config.contains(DFG_UTF8("properties/windowHeight")));
    //DFGTEST_EXPECT_TRUE(config.contains(DFG_UTF8("properties/windowPosX")));
    //DFGTEST_EXPECT_TRUE(config.contains(DFG_UTF8("properties/windowPosY")));
    //DFGTEST_EXPECT_TRUE(config.contains(DFG_UTF8("properties/windowWidth")));
}

TEST(dfgQt, CsvTableView_changeRadix)
{
    using namespace DFG_ROOT_NS;
    using namespace ::DFG_MODULE_NS(qt);
    using namespace ::DFG_MODULE_NS(str);

    using JsonId = CsvTableViewActionChangeRadixParams::ParamId;
    const auto idToStr = [](const JsonId id) { return CsvTableViewActionChangeRadixParams::paramStringId(id); };

    // Basic tests
    {
        CsvItemModel csvModel;
        CsvTableView view(nullptr, nullptr);
        view.setModel(&csvModel);

        const int64 rowValues[] = {1, 8, 16, -36, int64_min, int64_max};

        // csv-file with identical value on each row but with different radix on every column
        // 2, 8, 10, 16, 36
        // with value 1, 8, 16, -36, int64_min, int64_max
        DFGTEST_ASSERT_TRUE(csvModel.openString(
            " , , , , \n"                 // Header
            "1,1,1,1,1\n"                 // 1
            "1000,10,8,8,8\n"             // 8
            "10000, 20, 16, 10, g\n"      // 16
            "-100100, -44, -36, -24, -10\n" // -36
            "-1000000000000000000000000000000000000000000000000000000000000000, -1000000000000000000000, -9223372036854775808, -8000000000000000, -1y2p0ij32e8e8\n" // int64_min
            "111111111111111111111111111111111111111111111111111111111111111, 777777777777777777777, 9223372036854775807, 7fffffffffffffff, 1y2p0ij32e8e7" // int64_max
        ));

        const auto changeRadixOnColumn = [&](const int nCol, const int nSourceRadix, const int nResultRadix)
        {
            view.selectColumn(nCol);
            CsvTableViewActionChangeRadixParams params(
            {
                { idToStr(JsonId::fromRadix), nSourceRadix}, {idToStr(JsonId::toRadix), nResultRadix},
                { idToStr(JsonId::ignorePrefix), ""}, {idToStr(JsonId::ignoreSuffix), ""},
                { idToStr(JsonId::resultPrefix), ""}, {idToStr(JsonId::resultSuffix), ""}
            });
            view.changeRadix(params);
        };

        const auto testEquality = [&]()
        {
            const auto nCol = csvModel.getColumnCount();
            for (int r = 0; r < csvModel.getRowCount(); ++r)
            {
                for (int c = 1; c < nCol; ++c)
                    DFGTEST_EXPECT_LEFT(csvModel.rawStringViewAt(r, c - 1), csvModel.rawStringViewAt(r, c));
            }
        };

        // Changing all columns to radix 10
        changeRadixOnColumn(0,  2, 10);
        changeRadixOnColumn(1,  8, 10);
        changeRadixOnColumn(2, 10, 10);
        changeRadixOnColumn(3, 16, 10);
        changeRadixOnColumn(4, 36, 10);
        testEquality();

        // Changing all columns to radix 2
        changeRadixOnColumn(0, 10, 2);
        changeRadixOnColumn(1, 10, 2);
        changeRadixOnColumn(2, 10, 2);
        changeRadixOnColumn(3, 10, 2);
        changeRadixOnColumn(4, 10, 2);
        testEquality();

        // Changing all columns to radix 36
        changeRadixOnColumn(0, 2, 36);
        changeRadixOnColumn(1, 2, 36);
        changeRadixOnColumn(2, 2, 36);
        changeRadixOnColumn(3, 2, 36);
        changeRadixOnColumn(4, 2, 36);
        testEquality();

        // Changing all columns to radix 10
        changeRadixOnColumn(0, 36, 10);
        changeRadixOnColumn(1, 36, 10);
        changeRadixOnColumn(2, 36, 10);
        changeRadixOnColumn(3, 36, 10);
        changeRadixOnColumn(4, 36, 10);
        testEquality();

        // Testing that trying to change to invalid radix does nothing.
        changeRadixOnColumn(0, 36, -1);
        changeRadixOnColumn(1, 36, int32_max);
        changeRadixOnColumn(2, 36, -2);
        changeRadixOnColumn(3, 36, 0);
        changeRadixOnColumn(4, 36, int32_min);

        for (int r = 0; r < int(DFG_COUNTOF(rowValues)); ++r)
        {
            for (int c = 0; c < csvModel.getColumnCount(); ++c)
                DFGTEST_EXPECT_LEFT(rowValues[r], strTo<int64>(csvModel.rawStringViewAt(r, c)));
        }
    }

    // Testing radix 62
    {
        CsvItemModel csvModel;
        CsvTableView view(nullptr, nullptr);
        view.setModel(&csvModel);
        view.resizeTableNoUi(1, 1);
        auto params = CsvTableViewActionChangeRadixParams({ { idToStr(JsonId::fromRadix), 10}, { idToStr(JsonId::toRadix), 62 } });
        view.resizeTableNoUi(4, 1);
        csvModel.setDataNoUndo(0, 0, DFG_UTF8("61"));
        csvModel.setDataNoUndo(1, 0, DFG_UTF8("62"));
        csvModel.setDataNoUndo(2, 0, SzPtrUtf8(toStrC(int64_min).c_str()));
        csvModel.setDataNoUndo(3, 0, SzPtrUtf8(toStrC(int64_max).c_str()));
        view.selectColumn(0);
        view.changeRadix(params);
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("Z", csvModel.rawStringViewAt(0, 0));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("10", csvModel.rawStringViewAt(1, 0));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("-aZl8N0y58M8", csvModel.rawStringViewAt(2, 0)); // -aZl8N0y58M8 = -1* (10, 61, 21, 8, 49, 0, 34, 5, 8, 48, 8)
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("aZl8N0y58M7", csvModel.rawStringViewAt(3, 0));
    }

    // Prefix/suffix handling
    {
        CsvItemModel csvModel;
        CsvTableView view(nullptr, nullptr);
        view.setModel(&csvModel);
        view.resizeTableNoUi(3, 1);
        csvModel.setDataNoUndo(0, 0, DFG_UTF8("0x10_abc"));
        csvModel.setDataNoUndo(1, 0, DFG_UTF8("20_abc"));
        csvModel.setDataNoUndo(2, 0, DFG_UTF8("0x-30"));
        CsvTableViewActionChangeRadixParams params(
        {
            { idToStr(JsonId::fromRadix), 16}, {idToStr(JsonId::toRadix), 10},
            { idToStr(JsonId::ignorePrefix), "0x"}, {idToStr(JsonId::ignoreSuffix), "_abc"},
            { idToStr(JsonId::resultPrefix), "a"}, {idToStr(JsonId::resultSuffix), "b"}
        });
        view.selectColumn(0);
        view.changeRadix(params);

        DFGTEST_EXPECT_EQ_LITERAL_UTF8("a16b", csvModel.rawStringViewAt(0, 0));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("a32b", csvModel.rawStringViewAt(1, 0));
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("a-48b", csvModel.rawStringViewAt(2, 0));
    }

    // Custom digit tests
    {
        CsvItemModel csvModel;
        CsvTableView view(nullptr, nullptr);
        view.setModel(&csvModel);
        view.resizeTableNoUi(1, 1);

        csvModel.setDataNoUndo(0, 0, DFG_UTF8("9876543210"));
        CsvTableViewActionChangeRadixParams params(
        {
            { idToStr(JsonId::fromRadix), 10},
            { idToStr(JsonId::resultDigits), "-+bilgfd/_" }
        });
        view.selectColumn(0);
        view.changeRadix(params);
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("_/dfglib+-", csvModel.rawStringViewAt(0, 0));

        // Testing that faulty params having inconsistent arguments does nothing
#define DFG_TEMP_ES "\xE2\x82\xAC" // \xE2\x82\xAC is eurosign in UTF-8
        params = CsvTableViewActionChangeRadixParams({ { idToStr(JsonId::fromRadix), 10}, { idToStr(JsonId::resultDigits), QString::fromUtf8(DFG_TEMP_ES "I") } }); 
        params.toRadix = 10;
        csvModel.setDataNoUndo(0, 0, DFG_UTF8("5"));
        view.selectColumn(0);
        view.changeRadix(params);
        // changeRadix() should do nothing since toRadix (=10) is not the same as radix defined by resultDigits (=2)
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("5", csvModel.rawStringViewAt(0, 0));

        // Testing that multibyte codepoint as digits work correctly.
        params = CsvTableViewActionChangeRadixParams({ { idToStr(JsonId::fromRadix), 10}, { idToStr(JsonId::resultDigits), QString::fromUtf8(DFG_TEMP_ES "I") } });
        view.selectColumn(0);
        view.changeRadix(params);
        DFGTEST_EXPECT_EQ_LITERAL_UTF8("I" DFG_TEMP_ES "I", csvModel.rawStringViewAt(0, 0)); // 5 = 101b
#undef DFG_TEMP_ES

        // Testing radix 64 with given digits
        {
            const char digits64[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ{}";
            params = CsvTableViewActionChangeRadixParams({ { idToStr(JsonId::fromRadix), 10}, { idToStr(JsonId::resultDigits), digits64 } });
            view.resizeTableNoUi(4, 1);
            csvModel.setDataNoUndo(0, 0, DFG_UTF8("63"));
            csvModel.setDataNoUndo(1, 0, DFG_UTF8("64"));
            csvModel.setDataNoUndo(2, 0, SzPtrUtf8(toStrC(int64_min).c_str()));
            csvModel.setDataNoUndo(3, 0, SzPtrUtf8(toStrC(int64_max).c_str()));
            view.selectColumn(0);
            view.changeRadix(params);
            DFGTEST_EXPECT_EQ_LITERAL_UTF8("}", csvModel.rawStringViewAt(0, 0));
            DFGTEST_EXPECT_EQ_LITERAL_UTF8("10", csvModel.rawStringViewAt(1, 0));
            DFGTEST_EXPECT_EQ_LITERAL_UTF8("-80000000000", csvModel.rawStringViewAt(2, 0));
            DFGTEST_EXPECT_EQ_LITERAL_UTF8("7}}}}}}}}}}", csvModel.rawStringViewAt(3, 0));
        }
    }

    DFGTEST_EXPECT_TRUE(CsvTableViewActionChangeRadixParams({ { idToStr(JsonId::fromRadix), 10}, { idToStr(JsonId::toRadix), 62 } }).isValid());
    DFGTEST_EXPECT_FALSE(CsvTableViewActionChangeRadixParams({ { idToStr(JsonId::fromRadix), 10}, { idToStr(JsonId::toRadix), 63 } }).isValid());
}

TEST(dfgQt, CsvTableView_cellEditability)
{
    using namespace ::DFG_MODULE_NS(qt);
    CsvTableView view{ CsvTableView::TagCreateWithModels() };
    view.resizeTableNoUi(2, 3);
    view.clearUndoStack();
    view.setColumnReadOnly(ColumnIndex_data(1), true);
    DFGTEST_ASSERT_TRUE(view.csvModel() != nullptr);
    view.csvModel()->setCellReadOnlyStatus(1, 2, true);

#define DFGTEST_TEST_CELL_STATUS(VIEW_ROW, VIEW_COL, EXPECTED) DFGTEST_EXPECT_LEFT(CsvTableView::CellEditability::EXPECTED, view.getCellEditability(RowIndex_data(VIEW_ROW), view.columnIndexViewToData(ColumnIndex_view(VIEW_COL))));
    DFGTEST_TEST_CELL_STATUS(0, 0, editable);
    DFGTEST_TEST_CELL_STATUS(0, 1, blocked_columnReadOnly);
    DFGTEST_TEST_CELL_STATUS(0, 2, editable);
    DFGTEST_TEST_CELL_STATUS(1, 0, editable);
    DFGTEST_TEST_CELL_STATUS(1, 1, blocked_columnReadOnly);
    DFGTEST_TEST_CELL_STATUS(1, 2, blocked_cellReadOnly);

    // Hiding first column and making sure that read-only status follows correctly.
    view.setColumnVisibility(0, false);
    DFGTEST_TEST_CELL_STATUS(0, 0, blocked_columnReadOnly);
    DFGTEST_TEST_CELL_STATUS(0, 1, editable);
    DFGTEST_TEST_CELL_STATUS(1, 0, blocked_columnReadOnly);
    DFGTEST_TEST_CELL_STATUS(1, 1, blocked_cellReadOnly);

    view.unhideAllColumns();
    DFGTEST_TEST_CELL_STATUS(0, 0, editable);
    DFGTEST_TEST_CELL_STATUS(0, 1, blocked_columnReadOnly);
    DFGTEST_TEST_CELL_STATUS(0, 2, editable);
    DFGTEST_TEST_CELL_STATUS(1, 0, editable);
    DFGTEST_TEST_CELL_STATUS(1, 1, blocked_columnReadOnly);
    DFGTEST_TEST_CELL_STATUS(1, 2, blocked_cellReadOnly);

#undef DFGTEST_TEST_CELL_STATUS
}

namespace
{
    class CsvTableWidgetForFilterSelectionTest : public ::DFG_MODULE_NS(qt)::CsvTableWidget
    {
    public:
        using BaseClass = ::DFG_MODULE_NS(qt)::CsvTableWidget;
        CsvTableWidgetForFilterSelectionTest()
        {
            const auto filterSlot = [&](QString s)
            {
                m_sReceivedFilter = s;
                s.replace("} {", "}\n{");
                getViewModel().setFilterFromNewLineSeparatedJsonList(s.toUtf8());

            };
            DFG_QT_VERIFY_CONNECT(connect(this, &CsvTableView::sigFilterJsonRequested, &getViewModel(), filterSlot));
        }

        QString m_sReceivedFilter;
    }; // class CsvTableWidgetForFilterSelectionTest
} // unnamed namespace

TEST(dfgQt, CsvTableView_filterFromSelection)
{
    #define DFGTEST_TEMP_TEST_CELL(STR, ROW, COL) DFGTEST_EXPECT_EQ(STR, viewWidget.model()->data(viewWidget.model()->index(ROW, COL)).toString());
    #define DFGTEST_TEMP_TEST_ROW(ROW, S0, S1, S2) DFGTEST_TEMP_TEST_CELL(S0, ROW, 0); DFGTEST_TEMP_TEST_CELL(S1, ROW, 1); DFGTEST_TEMP_TEST_CELL(S2, ROW, 2)


    CsvTableWidgetForFilterSelectionTest viewWidget;

    const char szInputString[] =
        ",,\n"
        "1,2,3\n"
        "4,\"\"\"|()\",6\n" // Second cell from this row is selected in first test
        "7,8,9\n"
        "a,b,c\n" // 'b' is selected from this row in first test
        "d,e,f\n" // 'f' is selected from this row in first test
        "g,h,aFb\n"
        "f,i,j\n"
        "1,h,afb\n"
        "1,h,aFb\n"
        "1,h,aFb2\n"
        "1,h1,aFb";

    viewWidget.getCsvModel().openString(szInputString);

    const auto pModel = viewWidget.model();
    DFGTEST_ASSERT_TRUE(pModel != nullptr);
    const auto nRowCountTotal = pModel->rowCount();
    const auto nColCountTotal = pModel->columnCount();
    DFGTEST_EXPECT_LEFT(11, nRowCountTotal);
    DFGTEST_EXPECT_LEFT(3, nColCountTotal);
    // Selecting cells that have content '"|()', 'b' and 'f'

    // Testing default filter (or-columns, case-insensitive, substring match)
    {
        const QModelIndexList selection = { viewWidget.model()->index(1, 1), viewWidget.model()->index(3, 1), viewWidget.model()->index(4, 2) };
        viewWidget.setSelectedIndexes(selection, nullptr);
        DFGTEST_EXPECT_EQ(3, viewWidget.getSelectedItemCount());

        viewWidget.onFilterFromSelectionRequested();

        DFGTEST_EXPECT_EQ(8, viewWidget.model()->rowCount());

        DFGTEST_TEMP_TEST_ROW(0, "4", "\"|()", "6");
        DFGTEST_TEMP_TEST_ROW(1, "a", "b", "c");
        DFGTEST_TEMP_TEST_ROW(2, "d", "e", "f");
        DFGTEST_TEMP_TEST_ROW(3, "g", "h", "aFb");
        DFGTEST_TEMP_TEST_ROW(4, "1", "h", "afb");
        DFGTEST_TEMP_TEST_ROW(5, "1", "h", "aFb");
        DFGTEST_TEMP_TEST_ROW(6, "1", "h", "aFb2");
        DFGTEST_TEMP_TEST_ROW(7, "1", "h1", "aFb");

        // Testing filter format. This mostly tests against unintended changed in filter so if test fails just due to intentional format changes
        // (or unintentional ones that don't change filter behaviour), simply adjust test string.
        const auto szExpectedFilter = R"STR({"and_group":"col_2","apply_columns":"2","text":"\\\"\\|\\(\\)|b","type":"reg_exp"} {"and_group":"col_3","apply_columns":"3","text":"f","type":"reg_exp"} )STR";
        DFGTEST_EXPECT_LEFT(szExpectedFilter, viewWidget.m_sReceivedFilter);
    }

    // Testing strict filter (and-columns, case-sensitive, whole string match)
    {
        // Clearing filter
        Q_EMIT viewWidget.sigFilterJsonRequested(QString());
        DFGTEST_EXPECT_LEFT(nRowCountTotal, viewWidget.getRowCount_viewModel());

        // Selecting "h" and "aFb" from row 5
        viewWidget.clearSelection(); // For some reason without this call setSelectedIndexes() would select 5 cells.
        const QModelIndexList selection = { viewWidget.model()->index(5, 1), viewWidget.model()->index(5, 2) };
        viewWidget.setSelectedIndexes(selection, nullptr);
        DFGTEST_EXPECT_EQ(2, viewWidget.getSelectedItemCount());

        // Creating filter from selection with strict match flags
        using namespace ::DFG_MODULE_NS(qt);
        viewWidget.setFilterFromSelection(
            CsvTableViewSelectionFilterFlags::Enum::columnMatchByAnd |
            CsvTableViewSelectionFilterFlags::Enum::caseSensitive |
            CsvTableViewSelectionFilterFlags::Enum::wholeStringMatch
        );

        DFGTEST_EXPECT_EQ(2, viewWidget.model()->rowCount());

        DFGTEST_TEMP_TEST_ROW(0, "g", "h", "aFb");
        DFGTEST_TEMP_TEST_ROW(1, "1", "h", "aFb");

        // Testing filter format. This mostly tests against unintended changed in filter so if test fails just due to intentional format changes
        // (or unintentional ones that don't change filter behaviour), simply adjust test string.
        const auto szExpectedFilter = R"STR({"apply_columns":"2","case_sensitive":true,"text":"^h$","type":"reg_exp"} {"apply_columns":"3","case_sensitive":true,"text":"^aFb$","type":"reg_exp"} )STR";
        DFGTEST_EXPECT_LEFT(szExpectedFilter, viewWidget.m_sReceivedFilter);
    }
#undef DFGTEST_TEMP_TEST_CELL
#undef DFGTEST_TEMP_TEST_ROW
}

TEST(dfgQt, CsvTableView_sortSettingsFromConfFile)
{
    using namespace ::DFG_MODULE_NS(qt);
    CsvTableWidget table;
    DFGTEST_EXPECT_TRUE(table.openFile("testfiles/CsvTableView_sortSettingsFromConfFile.csv"));

    DFGTEST_EXPECT_LEFT(1, table.getViewModel().sortColumn());
    DFGTEST_EXPECT_TRUE(table.isSortingEnabled());
    DFGTEST_EXPECT_LEFT(Qt::SortOrder::AscendingOrder, table.getViewModel().sortOrder());
    DFGTEST_EXPECT_LEFT(Qt::CaseInsensitive, table.getViewModel().sortCaseSensitivity());

#define DFG_TEMP_EXPECT(STR, R, C) \
    DFGTEST_EXPECT_LEFT(STR, table.getViewModel().data(table.getViewModel().index(R, C)).toString())

    DFG_TEMP_EXPECT("4" , 0, 0);
    DFG_TEMP_EXPECT("A" , 0, 1);

    DFG_TEMP_EXPECT("1" , 1, 0);
    DFG_TEMP_EXPECT("aa", 1, 1);

    DFG_TEMP_EXPECT("5" , 2, 0);
    DFG_TEMP_EXPECT("B" , 2, 1);

    DFG_TEMP_EXPECT("2" , 3, 0);
    DFG_TEMP_EXPECT("bb", 3, 1);

    DFG_TEMP_EXPECT("6" , 4, 0);
    DFG_TEMP_EXPECT("C" , 4, 1);

    DFG_TEMP_EXPECT("3" , 5, 0);
    DFG_TEMP_EXPECT("cc", 5, 1);
#undef DFG_TEMP_EXPECT
}

TEST(dfgQt, CsvTableView_trimCells)
{
    ::DFG_MODULE_NS(qt)::CsvTableWidget view;
    auto& rModel = view.getCsvModel();
    view.resizeTableNoUi(3, 2);
    rModel.setItem(0, 0, " a  ");
    rModel.setItem(0, 1, "   b");
    rModel.setItem(1, 0, "c \t c");
    rModel.setItem(1, 1, "d   ");
    rModel.setItem(2, 0, "\te");
    rModel.setItem(2, 1, " \t f\t\t     \t");
    DFGTEST_EXPECT_LEFT(3, view.getCsvModel().getRowCount());
    DFGTEST_EXPECT_LEFT(2, view.getCsvModel().getColumnCount());

    view.setSelectedIndexes({ {0, 0}, {0, 1} });
    view.onTrimCellsUiAction();

    DFGTEST_EXPECT_EQ_LITERAL_UTF8("a", rModel.rawStringViewAt(0, 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("b", rModel.rawStringViewAt(0, 1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("c \t c", rModel.rawStringViewAt(1, 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("d   ", rModel.rawStringViewAt(1, 1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("\te", rModel.rawStringViewAt(2, 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8(" \t f\t\t     \t", rModel.rawStringViewAt(2, 1));

    view.selectAll();
    view.onTrimCellsUiAction();

    DFGTEST_EXPECT_EQ_LITERAL_UTF8("a", rModel.rawStringViewAt(0, 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("b", rModel.rawStringViewAt(0, 1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("c \t c", rModel.rawStringViewAt(1, 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("d", rModel.rawStringViewAt(1, 1));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("e", rModel.rawStringViewAt(2, 0));
    DFGTEST_EXPECT_EQ_LITERAL_UTF8("f", rModel.rawStringViewAt(2, 1));
}

TEST(dfgQt, TableView_makeSingleCellSelection)
{
    using namespace ::DFG_MODULE_NS(qt);
    CsvItemModel csvModel;
    CsvTableView view(nullptr, nullptr);
    view.setModel(&csvModel);
    csvModel.insertRows(0, 4);
    csvModel.insertColumns(0, 4);
    populateModelWithRowColumnIndexStrings(csvModel);
    view.makeSingleCellSelection(3, 3);
    auto pSelectionModel = view.selectionModel();
    ASSERT_TRUE(pSelectionModel != nullptr);
    const auto makeSingleCellIndexList = [&](const int r, const int c) { QModelIndexList list; list.push_back(csvModel.index(r,c)); return list; };
    EXPECT_EQ(makeSingleCellIndexList(3, 3), pSelectionModel->selectedIndexes());
    view.makeSingleCellSelection(1, 1);
    EXPECT_EQ(makeSingleCellIndexList(1, 1), pSelectionModel->selectedIndexes());
    view.selectColumn(2);
    view.makeSingleCellSelection(0, 0);
    EXPECT_EQ(makeSingleCellIndexList(0, 0), pSelectionModel->selectedIndexes());
}

TEST(dfgQt, TableView_setSelectedIndexes)
{
    ::DFG_MODULE_NS(qt)::CsvTableWidget view;
    view.resizeTableNoUi(2, 2);
    view.setSelectedIndexes({ view.model()->index(1, 0), view.model()->index(1, 1) }, nullptr);
    DFGTEST_EXPECT_LEFT(2, view.getSelectedItemCount());
    view.setSelectedIndexes({ view.model()->index(1, 0) }, nullptr);
    DFGTEST_EXPECT_LEFT(1, view.getSelectedItemCount());

    {
        view.setSelectedIndexes({ { 0, 1 }, { 1, 0} });
        const auto selected = view.getSelectedItemIndexes_dataModel();
        DFGTEST_EXPECT_LEFT(2, selected.size());
        DFGTEST_EXPECT_LEFT(0, selected.value(0).row());
        DFGTEST_EXPECT_LEFT(1, selected.value(0).column());
        DFGTEST_EXPECT_LEFT(1, selected.value(1).row());
        DFGTEST_EXPECT_LEFT(0, selected.value(1).column());
    }
}

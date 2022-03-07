#include <dfg/qt/CsvItemModel.hpp>
#include <dfg/io.hpp>
#include <dfg/math/sign.hpp>
#include <dfg/io/OmcByteStream.hpp>
#include <dfg/qt/containerUtils.hpp>
#include <dfg/qt/CsvTableView.hpp>
#include <dfg/qt/ConsoleDisplay.hpp>
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
    static const double baseDateTestDoubleValue = static_cast<double>(QDateTime::fromString("2020-04-25T00:00:00Z", Qt::ISODate).toMSecsSinceEpoch()) / 1000.0;

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
    }

    // Times only
    {
        // hh:mm:ss
        testDateToDouble("12:34:56", DataType::dayTime, 60 * (12 * 60 + 34) + 56, 0);
        testDateToDouble("12:34:56.123", DataType::dayTimeMillisecond, 60 * (12 * 60 + 34) + 56 + 0.123, 0);
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
    EXPECT_LE(nCount, 37); // A semi-arbitrary value.
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
        "2, 7\n"
        "-1, 50\n"
        "-10, 8\n"
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

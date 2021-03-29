//#include <stdafx.h>
#include <dfg/qt/CsvItemModel.hpp>
#include <dfg/io.hpp>
#include <dfg/io/OmcByteStream.hpp>
#include <dfg/qt/containerUtils.hpp>
#include <dfg/qt/CsvTableView.hpp>
#include <dfg/qt/ConsoleDisplay.hpp>
#include <dfg/qt/CsvTableViewChartDataSource.hpp>
#include <dfg/qt/CsvFileDataSource.hpp>
#include <dfg/qt/SQLiteFileDataSource.hpp>
#include <dfg/qt/NumericGeneratorDataSource.hpp>
#include <dfg/qt/connectHelper.hpp>
#include <dfg/math.hpp>
#include <dfg/qt/sqlTools.hpp>
#include <dfg/qt/PatternMatcher.hpp>

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
#include <gtest/gtest.h>
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
#include <dfg/qt/qxt/gui/qxtspanslider.h>
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS

TEST(dfgQt, CsvItemModel)
{

    for (size_t i = 0;; ++i)
    {
        const QString sInputPath = QString("testfiles/example%1.csv").arg(i);
        const QString sOutputPath = QString("testfiles/generated/example%1.testOutput.csv").arg(i);
        if (!QFileInfo(sInputPath).exists())
        {
            const bool bAtLeastOneFileFound = (i != 0);
            EXPECT_TRUE(bAtLeastOneFileFound);
            break;
        }

        DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel) model;

        DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::LoadOptions loadOptions;
        if (i == 3) // Example 3 needs non-native eol-settings.
            loadOptions.separatorChar('\t');
        if (i == 4)
            loadOptions.textEncoding(DFG_MODULE_NS(io)::encodingLatin1);

        const auto bOpenSuccess = model.openFile(sInputPath, loadOptions);
        EXPECT_EQ(true, bOpenSuccess);

        DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::SaveOptions saveOptions(&model);
        if (i == 3)
            saveOptions = loadOptions;
        if (i == 4)
            saveOptions.textEncoding(loadOptions.textEncoding());

        saveOptions.eolType((i == 1 || i == 4) ? DFG_MODULE_NS(io)::EndOfLineTypeN : DFG_MODULE_NS(io)::EndOfLineTypeRN);

        // Note: EOL-setting does not affect in-cell EOL-items, i.e. saving of eol-chars inside cell content
        // is not affected by eol-setting.
        const auto bSaveSuccess = model.saveToFile(sOutputPath, saveOptions);
        EXPECT_EQ(true, bSaveSuccess);

        auto inputBytesWithAddedEol = DFG_MODULE_NS(io)::fileToVector(sInputPath.toLatin1().data());
        const auto sEol = ::DFG_MODULE_NS(io)::eolStrFromEndOfLineType(saveOptions.eolType());
        inputBytesWithAddedEol.insert(inputBytesWithAddedEol.end(), sEol.cbegin(), sEol.cend());
        const auto outputBytes = DFG_MODULE_NS(io)::fileToVector(sOutputPath.toLatin1().data());
        EXPECT_EQ(inputBytesWithAddedEol, outputBytes);

        // Test in-memory saving
        {
            DFG_MODULE_NS(io)::DFG_CLASS_NAME(OmcByteStream)<std::vector<char>> strm;
            model.save(strm, saveOptions);
            const auto& outputBytesMc = strm.container();
            EXPECT_EQ(inputBytesWithAddedEol, outputBytesMc);
        }
    }

    // Testing insertRows/Columns
    {
        using namespace DFG_MODULE_NS(qt);
        CsvItemModel model;
        // Making sure that bad insert counts are handled correctly.
        EXPECT_FALSE(model.insertRows(0, 0));
        EXPECT_FALSE(model.insertColumns(0, 0));
        EXPECT_FALSE(model.insertRows(0, -5));
        EXPECT_FALSE(model.insertColumns(0, -5));
        EXPECT_EQ(0, model.rowCount());
        EXPECT_EQ(0, model.columnCount());
    }
}

TEST(dfgQt, CsvItemModel_removeRows)
{
    DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel) model;

    model.insertColumns(0, 1);
    model.insertRows(0, 10);
    EXPECT_EQ(10, model.rowCount());
    EXPECT_EQ(1, model.columnCount());
    for (int r = 0, nRowCount = model.rowCount(); r < nRowCount; ++r)
    {
        char c[2] = { static_cast<char>('a' + r), '\0' };
        model.setDataNoUndo(r, 0, DFG_ROOT_NS::SzPtrAscii(c));
    }
    model.removeRows(0, 2);
    EXPECT_EQ(8, model.rowCount());
    EXPECT_EQ(QString("c"), model.data(model.index(0, 0)).toString());

    model.removeRows(0, 1);
    EXPECT_EQ(7, model.rowCount());
    EXPECT_EQ(QString("d"), model.data(model.index(0, 0)).toString());

    const int removeRows0[] = { -1 };
    model.removeRows(removeRows0);
    EXPECT_EQ(7, model.rowCount()); // Passing bogus row index shouldn't do anything.

    const int removeRows1[] = { 1, 5, 6 };
    model.removeRows(removeRows1);
    EXPECT_EQ(4, model.rowCount());
    EXPECT_EQ(QString("d"), model.data(model.index(0, 0)).toString());
    EXPECT_EQ(QString("f"), model.data(model.index(1, 0)).toString());

    const int removeRows2[] = { 1, 2 };
    model.removeRows(removeRows2);
    EXPECT_EQ(2, model.rowCount());
    EXPECT_EQ(QString("d"), model.data(model.index(0, 0)).toString());
    EXPECT_EQ(QString("h"), model.data(model.index(1, 0)).toString());

    // Removing remaining rows and testing that bogus entries in the input list gets handled correctly also in this case.
    const int removeRows3[] = { -5, -2, -1, 0, 1, 2, 3, 50 };
    model.removeRows(removeRows3);
    EXPECT_EQ(0, model.rowCount());
}

TEST(dfgQt, CsvItemModel_filteredRead)
{
    using namespace DFG_MODULE_NS(qt);
    const char szFilePath5x5[] = "testfiles/test_5x5_sep1F_content_row_comma_column.csv";
    const char szFilePathAb[] = "testfiles/example_forFilterRead.csv";

    CsvItemModel model;

    // Row and column filter.
    {
        auto options = model.getLoadOptionsForFile(szFilePath5x5);
        options.setProperty(CsvOptionProperty_includeRows, "0;2;4");
        options.setProperty(CsvOptionProperty_includeColumns, "2;3;5");
        model.openFile(szFilePath5x5, options);
        ASSERT_EQ(2, model.rowCount());
        ASSERT_EQ(3, model.columnCount());
        EXPECT_EQ(QString("Col1"), model.getHeaderName(0));
        EXPECT_EQ(QString("Col2"), model.getHeaderName(1));
        EXPECT_EQ(QString("Col4"), model.getHeaderName(2));
        EXPECT_EQ(QString("2,1"), QString::fromUtf8(model.RawStringPtrAt(0, 0).c_str()));
        EXPECT_EQ(QString("2,2"), QString::fromUtf8(model.RawStringPtrAt(0, 1).c_str()));
        EXPECT_EQ(QString("2,4"), QString::fromUtf8(model.RawStringPtrAt(0, 2).c_str()));
        EXPECT_EQ(QString("4,1"), QString::fromUtf8(model.RawStringPtrAt(1, 0).c_str()));
        EXPECT_EQ(QString("4,2"), QString::fromUtf8(model.RawStringPtrAt(1, 1).c_str()));
        EXPECT_EQ(QString("4,4"), QString::fromUtf8(model.RawStringPtrAt(1, 2).c_str()));
    }

    // Content filter, basic test
    {
        auto options = model.getLoadOptionsForFile(szFilePath5x5);
        const char szFilters[] = R"({ "text": "1,4" })";
        options.setProperty(CsvOptionProperty_readFilters, szFilters);
        model.openFile(szFilePath5x5, options);
        ASSERT_EQ(1, model.rowCount());
        ASSERT_EQ(5, model.columnCount());
        EXPECT_EQ(QString("Col4"), model.getHeaderName(4));
        EXPECT_EQ(QString("1,4"), QString::fromUtf8(model.RawStringPtrAt(0, 4).c_str()));
        EXPECT_EQ(5, model.m_table.cellCountNonEmpty());
    }

    // Content filter, both OR and AND filters
    {
        auto options = model.getLoadOptionsForFile(szFilePath5x5);
        const char szFilters[] = R"({ "text": "3,*", "and_group": "a" }
                                    { "text": "*,0", "and_group": "a" }
                                    {"text": "1,4", "and_group": "b"} )";
        options.setProperty(CsvOptionProperty_readFilters, szFilters);
        model.openFile(szFilePath5x5, options);
        ASSERT_EQ(2, model.rowCount());
        ASSERT_EQ(5, model.columnCount());
        EXPECT_EQ(QString("Col0"), model.getHeaderName(0));
        EXPECT_EQ(QString("Col4"), model.getHeaderName(4));
        EXPECT_EQ(QString("1,4"), QString::fromUtf8(model.RawStringPtrAt(0, 4).c_str()));
        EXPECT_EQ(QString("3,0"), QString::fromUtf8(model.RawStringPtrAt(1, 0).c_str()));
        EXPECT_EQ(10, model.m_table.cellCountNonEmpty());
    }

    // Row, column and content filters
    {
        auto options = model.getLoadOptionsForFile(szFilePath5x5);
        const char szFilters[] = R"({ "text": "3,*", "and_group": "a" }
                                    { "text": "*,1", "and_group": "a" }
                                    { "text": "4,1", "and_group": "b"} )";
        options.setProperty(CsvOptionProperty_includeRows, "0;3;5");
        options.setProperty(CsvOptionProperty_includeColumns, "2");
        options.setProperty(CsvOptionProperty_readFilters, szFilters);
        model.openFile(szFilePath5x5, options);
        ASSERT_EQ(1, model.rowCount());
        ASSERT_EQ(1, model.columnCount());
        EXPECT_EQ(QString("Col1"), model.getHeaderName(0));
        EXPECT_EQ(QString("3,1"), QString::fromUtf8(model.RawStringPtrAt(0, 0).c_str()));
        EXPECT_EQ(1, model.m_table.cellCountNonEmpty());
    }

    // Case sensitivity in content filter
    {
        {
            auto options = model.getLoadOptionsForFile(szFilePathAb);
            const char szFilters[] = R"({ "text": "A", "case_sensitive": true } )";
            options.setProperty(CsvOptionProperty_readFilters, szFilters);
            model.openFile(szFilePathAb, options);
            EXPECT_EQ(0, model.m_table.cellCountNonEmpty());
        }

        {
            auto options = model.getLoadOptionsForFile(szFilePathAb);
            const char szFilters[] = R"({ "text": "A", "case_sensitive": false } )";
            options.setProperty(CsvOptionProperty_readFilters, szFilters);
            model.openFile(szFilePathAb, options);
            EXPECT_EQ(6, model.m_table.cellCountNonEmpty());
        }
    }

    // Column apply-filters in content filter
    {
        auto options = model.getLoadOptionsForFile(szFilePathAb);
        const char szFilters[] = R"({ "text": "a", "apply_columns": "2" } )";
        options.setProperty(CsvOptionProperty_readFilters, szFilters);
        model.openFile(szFilePathAb, options);
        ASSERT_EQ(1, model.rowCount());
        ASSERT_EQ(2, model.columnCount());
        EXPECT_EQ(QString("Col0"), model.getHeaderName(0));
        EXPECT_EQ(QString("Col1"), model.getHeaderName(1));
        EXPECT_EQ(QString("b"), QString::fromUtf8(model.RawStringPtrAt(0, 0).c_str()));
        EXPECT_EQ(QString("ab"), QString::fromUtf8(model.RawStringPtrAt(0, 1).c_str()));
        EXPECT_EQ(2, model.m_table.cellCountNonEmpty());
    }

    // Row apply-filters in content filter
    {
        auto options = model.getLoadOptionsForFile(szFilePathAb);
        const char szFilters[] = R"({ "text": "c", "apply_rows": "1;3" } )";
        options.setProperty(CsvOptionProperty_readFilters, szFilters);
        model.openFile(szFilePathAb, options);
        ASSERT_EQ(2, model.rowCount());
        ASSERT_EQ(2, model.columnCount());
        EXPECT_EQ(QString("Col0"), model.getHeaderName(0));
        EXPECT_EQ(QString("Col1"), model.getHeaderName(1));
        EXPECT_EQ(QString("b"), QString::fromUtf8(model.RawStringPtrAt(0, 0).c_str()));
        EXPECT_EQ(QString("ab"), QString::fromUtf8(model.RawStringPtrAt(0, 1).c_str()));
        EXPECT_EQ(QString("ab"), QString::fromUtf8(model.RawStringPtrAt(1, 0).c_str()));
        EXPECT_EQ(QString("c"), QString::fromUtf8(model.RawStringPtrAt(1, 1).c_str()));
        EXPECT_EQ(4, model.m_table.cellCountNonEmpty());
    }
}

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
    EXPECT_EQ(0, nSignalCount);

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
    EXPECT_EQ(3, nSignalCount);
    view.selectColumn(1);
    EXPECT_EQ(clipboardStringCol0, view.makeClipboardStringForCopy());

    // Testing signal count when whole table is getting pasted
    view.selectAll();
    view.copy();
    view.clearSelected();
    view.makeSingleCellSelection(0, 0);
    nSignalCount = 0;
    view.paste();
    EXPECT_EQ(nColCount, nSignalCount); // Having only 1 signal would be better, but as commented above, at least making sure that there's no signal per cell.

    // Testing signal count when pasting row
    view.selectAll();
    view.copy();
    view.clearSelected();
    view.selectRow(0);
    view.makeSingleCellSelection(1, 0);
    nSignalCount = 0;
    view.paste();
    EXPECT_EQ(nColCount, nSignalCount); // Having only 1 signal would be better.
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
    EXPECT_EQ(nColCount, nSignalCount); // Expecting signaling from clear to behave similarly to pasting.
    nSignalCount = 0;
    view.undo();
    EXPECT_EQ(sAfterPopulating, view.makeClipboardStringForCopy());
    EXPECT_EQ(nColCount, nSignalCount);
    nSignalCount = 0;
    view.redo();
    EXPECT_EQ(sAfterClearing, view.makeClipboardStringForCopy());
    EXPECT_EQ(nColCount, nSignalCount);
}

namespace
{
    struct CsvItemModelReadFormatTestCase
    {
        const char* m_pszInput;
        DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::LoadOptions m_loadOptions;
    };
}

// Test that saving uses the same settings as loading
TEST(dfgQt, CsvItemModel_readFormatUsageOnWrite)
{
    typedef DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel) ModelT;
    typedef DFG_MODULE_NS(io)::DFG_CLASS_NAME(OmcByteStream)<std::string> StrmT;
    typedef DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)::LoadOptions LoadOptionsT;
    typedef DFG_ROOT_NS::DFG_CLASS_NAME(CsvFormatDefinition) CsvFormatDef;
#define DFG_TEMP_BOM DFG_UTF_BOM_STR_UTF8
    CsvItemModelReadFormatTestCase testCases[] =
    {
        // Auto-detected separators with default enclosing char.
        { DFG_TEMP_BOM "a,b,\",c\"\n",                          LoadOptionsT() },
        { DFG_TEMP_BOM "a;b;\";c\"\n",                          LoadOptionsT() },
        { DFG_TEMP_BOM "a\tb\t\"\tc\"\n",                       LoadOptionsT() },
        { DFG_TEMP_BOM "a\x1f" "b\x1f" "\"\x1f" "c\"\n",        LoadOptionsT() },
        // Custom formats
        { DFG_TEMP_BOM "a2b2\"2c\"\n",                          CsvFormatDef('2', '"', DFG_MODULE_NS(io)::EndOfLineTypeN, DFG_MODULE_NS(io)::encodingUTF8)  },      // Separator '2', enc '"', eol n
        { DFG_TEMP_BOM "a|b|^|c^\n",                            CsvFormatDef('|', '^', DFG_MODULE_NS(io)::EndOfLineTypeN, DFG_MODULE_NS(io)::encodingUTF8) },       // Separator '|', enc '^', eol n
        { DFG_TEMP_BOM "a|b|^|c^\r\n",                          CsvFormatDef('|', '^', DFG_MODULE_NS(io)::EndOfLineTypeRN, DFG_MODULE_NS(io)::encodingUTF8) },      // Separator '|', enc '^', eol rn
        { "\xE4" "|\xF6" "|^|c^\n",                             CsvFormatDef('|', '^', DFG_MODULE_NS(io)::EndOfLineTypeN, DFG_MODULE_NS(io)::encodingLatin1) },     // Separator '|', enc '^', eol n, encoding Latin1
        // Note: EOL-handling is not tested thoroughly as parsing does not detect EOL (e.g. reading with \n will read \r\n fine, but does not automatically set read EOL as \r\n).
    };
    
    for (auto iter = std::begin(testCases); iter != std::end(testCases); ++iter)
    {
        ModelT model;
        model.openFromMemory(iter->m_pszInput, DFG_MODULE_NS(str)::strLen(iter->m_pszInput), iter->m_loadOptions);
        EXPECT_EQ(3, model.columnCount());
        StrmT ostrm;
        model.save(ostrm);
        
        EXPECT_EQ(iter->m_pszInput, ostrm.container());
    }
#undef DFG_TEMP_BOM
}

// Tests for SQLite handling (open, save)
TEST(dfgQt, CsvItemModel_SQLite)
{
    // Testing that csv -> SQLite -> csv roundtrip is non-modifying given that sqlite -> csv is done with correct save options.
    using namespace DFG_MODULE_NS(qt);
    const char szFilePath[] = "testfiles/example3.csv";
    const QString sSqlitePath = QString("testfiles/generated/csvItemModel_sqliteTest.sqlite3");
    const QString sCsvFromSqlitePath = QString("testfiles/generated/csvItemModel_sqliteTest.csv");
    const auto initialBytes = ::DFG_MODULE_NS(io)::fileToByteContainer<std::string>(szFilePath) + "\r\n";
    QStringList columnNamesInCsv;
    auto originalSaveOptions = CsvItemModel().getSaveOptions();

    // Removing existing temp files
    {
        QFile::remove(sSqlitePath);
        QFile::remove(sCsvFromSqlitePath);
    }

    // Reading from csv and exporting as SQLite
    {
        CsvItemModel model;
        EXPECT_TRUE(model.openFile(szFilePath));
        columnNamesInCsv = model.getColumnNames();
        originalSaveOptions = model.getSaveOptions();
        EXPECT_TRUE(model.exportAsSQLiteFile(sSqlitePath));
    }

    // Reading from SQLite and writing as csv
    {
        CsvItemModel model;
        const auto tableNames = ::DFG_MODULE_NS(sql)::SQLiteDatabase::getSQLiteFileTableNames(sSqlitePath);
        ASSERT_EQ(1, tableNames.size());
        const auto columnNames = ::DFG_MODULE_NS(sql)::SQLiteDatabase::getSQLiteFileTableColumnNames(sSqlitePath, tableNames.front());
        EXPECT_EQ(columnNamesInCsv, columnNames);
        EXPECT_TRUE(model.openFromSqlite(sSqlitePath, QString("SELECT * FROM '%1';").arg(tableNames.front())));
        EXPECT_TRUE(model.saveToFile(sCsvFromSqlitePath, originalSaveOptions));
    }

    const auto finalBytes = ::DFG_MODULE_NS(io)::fileToByteContainer<std::string>(qStringToFileApi8Bit(sCsvFromSqlitePath));
    EXPECT_EQ(initialBytes, finalBytes);

    EXPECT_TRUE(QFile::remove(sSqlitePath));
    EXPECT_TRUE(QFile::remove(sCsvFromSqlitePath));
}

TEST(dfgQt, SpanSlider)
{
    // Simply test that this compiles and links.
    auto pSpanSlider = std::unique_ptr<QxtSpanSlider>(new QxtSpanSlider(Qt::Horizontal));
#if 0 // Enable to test in a GUI.
    auto pLayOut = new QGridLayout();
    auto pEditMinVal = new QSpinBox;
    auto pEditMaxVal = new QSpinBox;

    QObject::connect(pSpanSlider.get(), SIGNAL(lowerValueChanged(int)), pEditMinVal, SLOT(setValue(int)));
    QObject::connect(pSpanSlider.get(), SIGNAL(upperValueChanged(int)), pEditMaxVal, SLOT(setValue(int)));

    pLayOut->addWidget(pSpanSlider.release(), 0, 0, 1, 2); // Takes ownership
    pLayOut->addWidget(pEditMinVal, 1, 0, 1, 1); // Takes ownership
    pLayOut->addWidget(pEditMaxVal, 1, 1, 1, 1); // Takes ownership
    QDialog w;
    w.setLayout(pLayOut); // Takes ownership
    w.exec();
#endif
}

namespace
{
    class QObjectStorageTestWidget : public QWidget
    {
    public:
        QObjectStorageTestWidget()
        {
            m_threads.push_back(new QThread(this));
            m_threads.push_back(new QThread);
        }
    public:
        std::vector<DFG_MODULE_NS(qt)::QObjectStorage<QThread>> m_threads;
    };
}

TEST(dfgQt, QObjectStorage)
{
    // Testing moving of QObject storages
    {
        DFG_MODULE_NS(qt)::QObjectStorage<QDialog> storage0(new QDialog);
        auto pDialog = storage0.get();
        EXPECT_EQ(pDialog, storage0.get());
        DFG_MODULE_NS(qt)::QObjectStorage<QDialog> storage1 = std::move(storage0);
        EXPECT_FALSE(storage0);
        EXPECT_EQ(pDialog, storage1.get());

        // Testing move assignment
        DFG_MODULE_NS(qt)::QObjectStorage<QDialog> storage2;
        storage2 = std::move(storage1);
        EXPECT_FALSE(storage1);
        EXPECT_EQ(pDialog, storage2.get());
    }

    // Testing actual class that has std::vector<QObjectStorage<T>> as member.
    {
        QObjectStorageTestWidget widgetTest;
    }
}

TEST(dfgQt, ConsoleDisplay)
{
    ::DFG_MODULE_NS(qt)::ConsoleDisplay display;
    const size_t nLengthLimit = 40;
    display.lengthInCharactersLimit(nLengthLimit);
    // Note: display adds prefix to entries so not this many entries are really required to go beyond 40 chars.
    display.addEntry("0123456789");
    display.addEntry("0123456789");
    display.addEntry("0123456789");
    display.addEntry("0123456789");
    display.addEntry("0123456789");
    EXPECT_TRUE(display.lengthInCharacters() < nLengthLimit);
}

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

TEST(dfgQt, qstringUtils)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(qt);
    EXPECT_EQ(QString("abc"), viewToQString(DFG_UTF8("abc")));
    EXPECT_EQ(QString("abc"), untypedViewToQStringAsUtf8("abc"));
    EXPECT_EQ(DFG_UTF8("abc"), qStringToStringUtf8(QString("abc")));
}

TEST(dfgQt, StringMatchDefinition)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(qt);
#define DFGTEST_TEMP_VERIFY(JSON_STR, EXPECTED_STR, EXPECTED_CASE, EXPECTED_SYNTAX) \
    { \
        const auto smd = StringMatchDefinition::fromJson(DFG_UTF8(JSON_STR)); \
        EXPECT_EQ(EXPECTED_STR, smd.matchString()); \
        EXPECT_EQ(EXPECTED_CASE, smd.caseSensitivity()); \
        EXPECT_EQ(EXPECTED_SYNTAX, smd.patternSyntax()); \
    }

    DFGTEST_TEMP_VERIFY("non_json", "*", Qt::CaseInsensitive, PatternMatcher::Wildcard);
    DFGTEST_TEMP_VERIFY("{}", "", Qt::CaseInsensitive, PatternMatcher::Wildcard);
    DFGTEST_TEMP_VERIFY(R"( { "text": "abc" } )", "abc", Qt::CaseInsensitive, PatternMatcher::Wildcard);
    DFGTEST_TEMP_VERIFY(R"( { "case_sensitive": true } )", "", Qt::CaseSensitive, PatternMatcher::Wildcard);
    DFGTEST_TEMP_VERIFY(R"( { "type": "wildcard" } )", "", Qt::CaseInsensitive, PatternMatcher::Wildcard);
    DFGTEST_TEMP_VERIFY(R"( { "type": "fixed" } )", "", Qt::CaseInsensitive, PatternMatcher::FixedString);
    DFGTEST_TEMP_VERIFY(R"( { "type": "reg_exp" } )", "", Qt::CaseInsensitive, PatternMatcher::RegExp);
    DFGTEST_TEMP_VERIFY(R"( { "text": "a|B", "case_sensitive":true, "type": "reg_exp" } )", "a|B", Qt::CaseSensitive, PatternMatcher::RegExp);

#undef DFGTEST_TEMP_VERIFY

    // Tests with Wildcard
    {
        EXPECT_FALSE(StringMatchDefinition("A", Qt::CaseSensitive, PatternMatcher::Wildcard).isMatchWith(DFG_UTF8("abc")));
        EXPECT_FALSE(StringMatchDefinition("A", Qt::CaseSensitive, PatternMatcher::Wildcard).isMatchWith(QString("abc")));
        EXPECT_TRUE(StringMatchDefinition("A", Qt::CaseInsensitive, PatternMatcher::Wildcard).isMatchWith(DFG_UTF8("abc")));
        EXPECT_TRUE(StringMatchDefinition("A", Qt::CaseInsensitive, PatternMatcher::Wildcard).isMatchWith(QString("abc")));

        // Testing that characters that are special in regular expression but not in wildcard are handled correctly.
        EXPECT_TRUE(StringMatchDefinition("+{()}", Qt::CaseInsensitive, PatternMatcher::Wildcard).isMatchWith(QString("a+{()}b")));
        EXPECT_FALSE(StringMatchDefinition("+{()}", Qt::CaseInsensitive, PatternMatcher::Wildcard).isMatchWith(QString("a+{(}b")));

        EXPECT_TRUE(StringMatchDefinition("a*b[cde]f?g", Qt::CaseInsensitive, PatternMatcher::Wildcard).isMatchWith(QString("a1234qwbef+ghi")));
        EXPECT_FALSE(StringMatchDefinition("a*b[cde]f?g", Qt::CaseInsensitive, PatternMatcher::Wildcard).isMatchWith(QString("a1234qwbff+ghi")));
    }

    // Tests with FixedString
    {
        EXPECT_FALSE(StringMatchDefinition("A", Qt::CaseSensitive, PatternMatcher::FixedString).isMatchWith(DFG_UTF8("abc")));
        EXPECT_FALSE(StringMatchDefinition("A", Qt::CaseSensitive, PatternMatcher::FixedString).isMatchWith(QString("abc")));
        EXPECT_TRUE(StringMatchDefinition("A", Qt::CaseInsensitive, PatternMatcher::FixedString).isMatchWith(DFG_UTF8("abc")));
        EXPECT_TRUE(StringMatchDefinition("A", Qt::CaseInsensitive, PatternMatcher::FixedString).isMatchWith(QString("abc")));

        // Testing that characters that are special in wildcard and regular expression are handled correctly.
        EXPECT_TRUE(StringMatchDefinition("*", Qt::CaseInsensitive, PatternMatcher::FixedString).isMatchWith(QString("a?*b")));
        EXPECT_FALSE(StringMatchDefinition("*", Qt::CaseInsensitive, PatternMatcher::FixedString).isMatchWith(QString("a?b")));
    }
}

namespace
{
template <class Source_T>
static void testFileDataSource(const QString& sExtension,
                               std::function<std::unique_ptr<Source_T> (QString, QString)> sourceCreator,
                               std::function<bool (QString, ::DFG_MODULE_NS(qt)::CsvItemModel&)> fileCreator,
                               std::function<bool (QString, ::DFG_MODULE_NS(qt)::CsvItemModel&)> editer)
{
    using namespace ::DFG_ROOT_NS;
    using namespace ::DFG_MODULE_NS(qt);

    const QString sTestFilePath("testfiles/generated/tempFileDataSourceTest." + sExtension);
    QFile::remove(sTestFilePath);

    {
        CsvItemModel model;
        model.openString("Col0,Col1,123.456\nab,b,18.9.2020\nb,a\xE2\x82\xAC" "b,2020-09-18\nab,c,2020-09-18 12:00:00\n");
        ASSERT_TRUE(fileCreator(sTestFilePath, model));
    }

    const size_t nColCount = 3;
    std::array<std::vector<double>, nColCount> expectedRows = { std::vector<double>({0, 1, 2, 3}), {0, 1, 2, 3}, {0, 1, 2, 3} };
    std::array<std::vector<std::string>, nColCount> expectedStrings = { std::vector<std::string>(
                                                                {"Col0", "ab", "b", "ab"}),
                                                                {"Col1", "b", "a\xE2\x82\xAC" "b", "c"}, // \xE2\x82\xAC is eurosign in UTF-8
                                                                {"123.456", "18.9.2020", "2020-09-18", "2020-09-18 12:00:00"} };

    // csv source includes header on row 0, SQLiteSource doesn't.
    const bool bIncludeHeaderInComparisons = (sExtension == "csv");
    
    if (!bIncludeHeaderInComparisons)
    {
        ::DFG_MODULE_NS(alg)::forEachFwd(expectedRows,    [](std::vector<double>& v)      { v.erase(v.begin()); });
        ::DFG_MODULE_NS(alg)::forEachFwd(expectedStrings, [](std::vector<std::string>& v) { v.erase(v.begin()); });
    }
    const std::array<ChartDataType, nColCount> expectedColumnDataTypes = {ChartDataType::unknown, ChartDataType::unknown, ChartDataType::dateAndTime };
    std::array<std::vector<double>, nColCount> expectedValues;
    std::transform(expectedStrings[2].begin(),
                   expectedStrings[2].end(),
                   std::back_inserter(expectedValues[2]), [](const std::string& s) { return GraphDataSource::cellStringToDouble(SzPtrUtf8(s.c_str())); } );

    for (DataSourceIndex c = 0; c < nColCount + 1; ++c)
    {
        std::vector<double> rows;
        std::vector<std::string> strings;
        std::vector<double> values;
        auto spSource = sourceCreator(sTestFilePath, "csvSource");
        auto& source = *spSource;
        source.forEachElement_byColumn(c, DataQueryDetails(DataQueryDetails::DataMaskAll), [&](const SourceDataSpan& sourceSpan)
        {
            rows.insert(rows.end(), sourceSpan.rows().begin(), sourceSpan.rows().end());
            const auto nOldSize = static_cast<uint16>(strings.size());
            strings.resize(nOldSize + sourceSpan.stringViews().size());
            std::transform(sourceSpan.stringViews().begin(), sourceSpan.stringViews().end(), strings.begin() + nOldSize, [](const StringViewUtf8& sv) { return sv.toString().rawStorage(); });
            values.insert(values.end(), sourceSpan.doubles().begin(), sourceSpan.doubles().end());
        });
        if (c < expectedRows.size())
        {
            ASSERT_EQ(expectedRows.size(), expectedStrings.size());
            EXPECT_EQ(expectedRows[c], rows);
            EXPECT_EQ(expectedStrings[c], strings);
            if (!expectedValues[c].empty())
            {
                EXPECT_EQ(expectedValues[c].size(), values.size());
                if (expectedValues[c].size() == values.size())
                    EXPECT_TRUE(std::equal(values.begin(), values.end(), expectedValues[c].begin()));
            }
        }
        else
        {
            EXPECT_TRUE(rows.empty());
            EXPECT_TRUE(strings.empty());
        }
        if (isValidIndex(expectedColumnDataTypes, c))
            EXPECT_EQ(expectedColumnDataTypes[c], source.columnDataType(c));
    }

    // Testing that data mask is taken into account
    const DataQueryDetails::DataMask masks[] = { DataQueryDetails::DataMaskRows, DataQueryDetails::DataMaskNumerics, DataQueryDetails::DataMaskStrings, DataQueryDetails::DataMaskRowsAndNumerics, DataQueryDetails::DataMaskRowsAndStrings };
    for (const auto& mask : masks)
    {
        const DataSourceIndex nCol = 2;
        std::vector<double> rows;
        std::vector<std::string> strings;
        std::vector<double> values;
        auto spSource = sourceCreator(sTestFilePath, "csvSource");
        auto& source = *spSource;
        source.forEachElement_byColumn(nCol, DataQueryDetails(mask), [&](const SourceDataSpan& sourceSpan)
        {
            rows.insert(rows.end(), sourceSpan.rows().begin(), sourceSpan.rows().end());
            const auto nOldSize = static_cast<uint16>(strings.size());
            strings.resize(nOldSize + sourceSpan.stringViews().size());
            std::transform(sourceSpan.stringViews().begin(), sourceSpan.stringViews().end(), strings.begin() + nOldSize, [](const StringViewUtf8& sv) { return sv.toString().rawStorage(); });
            values.insert(values.end(), sourceSpan.doubles().begin(), sourceSpan.doubles().end());
        });
        if (mask & DataQueryDetails::DataMaskRows)
            EXPECT_EQ(expectedRows[nCol], rows);
        else
            EXPECT_TRUE(rows.empty());

        if (mask & DataQueryDetails::DataMaskNumerics)
            EXPECT_EQ(expectedValues[nCol], values);
        else
            EXPECT_TRUE(values.empty());

        if (mask & DataQueryDetails::DataMaskStrings)
            EXPECT_EQ(expectedStrings[nCol], strings);
        else
            EXPECT_TRUE(strings.empty());
    }

    // Testing that change signaling works
    {
        auto spSource = sourceCreator(sTestFilePath, "csvSource");
        auto& source = *spSource;
        bool bChangeNotificationReceived = false;
        DFG_QT_VERIFY_CONNECT(QObject::connect(&source, &CsvFileDataSource::sigChanged, [&]() { bChangeNotificationReceived = true;}));
        {
            CsvItemModel model;
            model.openString("a,b\n1,2");
            EXPECT_TRUE(editer(sTestFilePath, model));
        }

        for (int i = 0; i < 10 && !bChangeNotificationReceived; ++i)
        {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
        // Checking that change notification was received and that new columns are effective.
        EXPECT_TRUE(bChangeNotificationReceived);
        ASSERT_EQ(2, source.columnNames().size());
        EXPECT_EQ("a", source.columnNames()[0u]);
        EXPECT_EQ("b", source.columnNames()[1u]);
        std::vector<std::string> elems;
        source.forEachElement_byColumn(0, DataQueryDetails(DataQueryDetails::DataMaskAll), [&](const SourceDataSpan& sourceSpan)
        {
            std::transform(sourceSpan.stringViews().begin(), sourceSpan.stringViews().end(), std::back_inserter(elems), [](const StringViewUtf8& sv) { return sv.toString().rawStorage(); });
        });
        source.forEachElement_byColumn(1, DataQueryDetails(DataQueryDetails::DataMaskAll), [&](const SourceDataSpan& sourceSpan)
        {
            std::transform(sourceSpan.stringViews().begin(), sourceSpan.stringViews().end(), std::back_inserter(elems), [](const StringViewUtf8& sv) { return sv.toString().rawStorage(); });
        });
        if (bIncludeHeaderInComparisons)
            EXPECT_EQ(std::vector<std::string>({"a", "1", "b", "2"}), elems);
        else
            EXPECT_EQ(std::vector<std::string>({ "1", "2" }), elems);
    }

    QFile::remove(sTestFilePath);
}
}

TEST(dfgQt, CsvFileDataSource)
{
    using namespace ::DFG_MODULE_NS(qt);
    auto sourceCreator = [](const QString& sPath, const QString& sId) { return std::unique_ptr<CsvFileDataSource>(new CsvFileDataSource(sPath, sId)); };
    auto fileCreator = [](QString sPath, CsvItemModel& model) { return model.saveToFile(sPath); };
    testFileDataSource<CsvFileDataSource>("csv", sourceCreator, fileCreator, fileCreator);
}

TEST(dfgQt, SQLiteFileDataSource)
{
    using namespace ::DFG_MODULE_NS(qt);
    auto sourceCreator = [](const QString& sPath, const QString& sId) { return std::unique_ptr<SQLiteFileDataSource>(new SQLiteFileDataSource("SELECT * FROM table_from_csv", sPath, sId)); };
    auto fileCreator = [](QString sPath, CsvItemModel& model) { QFile::remove(sPath); return model.exportAsSQLiteFile(sPath); };
    testFileDataSource<SQLiteFileDataSource>("sqlite3", sourceCreator, fileCreator, fileCreator);
}

TEST(dfgQt, NumericGeneratorDataSource)
{
    using namespace ::DFG_MODULE_NS(qt);
    using namespace ::DFG_MODULE_NS(cont);

    // Basic test
    {
        NumericGeneratorDataSource ds("test", 5);
        ValueVector<double> vals;
        ds.forEachElement_byColumn(0, DataQueryDetails(DataQueryDetails::DataMaskAll), [&](const SourceDataSpan& dataSpan)
        {
            EXPECT_TRUE(dataSpan.rows().empty()); // Rows are not expected to be present.
            auto doubleSpan = dataSpan.doubles();
            vals.insert(vals.end(), doubleSpan.begin(), doubleSpan.end());
        });
        EXPECT_EQ(ValueVector<double>({0, 1, 2, 3, 4}), vals);
    }

    const auto storeValuesFromSource = [](NumericGeneratorDataSource& ds)
    {
        ValueVector<double> vals;
        ds.forEachElement_byColumn(0, DataQueryDetails(DataQueryDetails::DataMaskAll), [&](const SourceDataSpan& dataSpan)
        {
            auto doubleSpan = dataSpan.doubles();
            vals.insert(vals.end(), doubleSpan.begin(), doubleSpan.end());
        });
        return vals;
    };

    // Testing data more than block size.
    {
        const DataSourceIndex nRowCount = 5;
        NumericGeneratorDataSource ds("test", nRowCount, 3);
        const auto vals = storeValuesFromSource(ds);
        EXPECT_EQ(ValueVector<double>({0, 1, 2, 3, 4}), vals);
    }

    // createByCountFirstStep
    {
        {
            auto spDs = NumericGeneratorDataSource::createByCountFirstStep("test", 0, 1, 2);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_TRUE(storeValuesFromSource(*spDs).empty());
        }
        {
            auto spDs = NumericGeneratorDataSource::createByCountFirstStep("test", 1, 3, 5);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_EQ(ValueVector<double>({3}), storeValuesFromSource(*spDs));
        }
        {
            auto spDs = NumericGeneratorDataSource::createByCountFirstStep("test", 3, 10, -20);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_EQ(ValueVector<double>({10, -10, -30}), storeValuesFromSource(*spDs));
        }
        {
            auto spDs = NumericGeneratorDataSource::createByCountFirstStep("test", 3, 10, 0);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_EQ(ValueVector<double>({10, 10, 10}), storeValuesFromSource(*spDs));
        }
    }

    // createByCountFirstLast
    {
        {
            auto spDs = NumericGeneratorDataSource::createByCountFirstLast("test", 0, 1, 2);
            ASSERT_TRUE(spDs == nullptr);
        }
        {
            auto spDs = NumericGeneratorDataSource::createByCountFirstLast("test", 1, 3, 3);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_EQ(ValueVector<double>({3}), storeValuesFromSource(*spDs));
        }
        {
            auto spDs = NumericGeneratorDataSource::createByCountFirstLast("test", 1, 3, 5);
            ASSERT_TRUE(spDs == nullptr);
        }
        {
            auto spDs = NumericGeneratorDataSource::createByCountFirstLast("test", 2, 3, 5);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_EQ(ValueVector<double>({3, 5}), storeValuesFromSource(*spDs));
        }
        {
            auto spDs = NumericGeneratorDataSource::createByCountFirstLast("test", 5, 6, 8);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_EQ(ValueVector<double>({6, 6.5, 7, 7.5, 8}), storeValuesFromSource(*spDs));
        }
        {
            auto spDs = NumericGeneratorDataSource::createByCountFirstLast("test", 5, 8, 6);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_EQ(ValueVector<double>({8, 7.5, 7, 6.5, 6}), storeValuesFromSource(*spDs));
        }
        {
            auto spDs = NumericGeneratorDataSource::createByCountFirstLast("test", 1, 3, std::numeric_limits<double>::quiet_NaN());
            ASSERT_TRUE(spDs == nullptr);
        }
        {
            auto spDs = NumericGeneratorDataSource::createByCountFirstLast("test", 1, std::numeric_limits<double>::quiet_NaN(), 3);
            ASSERT_TRUE(spDs == nullptr);
        }
    }

    // createByfirstLastStep
    {
        {
            auto spDs = NumericGeneratorDataSource::createByfirstLastStep("test", 0, 1, 0);
            ASSERT_TRUE(spDs == nullptr);
        }

        {
            auto spDs = NumericGeneratorDataSource::createByfirstLastStep("test", 0, 1, 1);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_EQ(ValueVector<double>({0, 1}), storeValuesFromSource(*spDs));
        }
        {
            auto spDs = NumericGeneratorDataSource::createByfirstLastStep("test", 1, 3, 4);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_EQ(ValueVector<double>({1}), storeValuesFromSource(*spDs));
        }
        {
            auto spDs = NumericGeneratorDataSource::createByfirstLastStep("test", 1, 3, 0.75);
            ASSERT_TRUE(spDs != nullptr);
            EXPECT_EQ(ValueVector<double>({1, 1.75, 2.5}), storeValuesFromSource(*spDs));
        }
        {
            auto spDs = NumericGeneratorDataSource::createByfirstLastStep("test", 1, 3, -1);
            ASSERT_TRUE(spDs == nullptr);
        }
        {
            auto spDs = NumericGeneratorDataSource::createByfirstLastStep("test", 3, 1, 1);
            ASSERT_TRUE(spDs == nullptr);
        }
        {
            auto spDs = NumericGeneratorDataSource::createByfirstLastStep("test", 1, 3, std::numeric_limits<double>::quiet_NaN());
            ASSERT_TRUE(spDs == nullptr);
        }
        {
            auto spDs = NumericGeneratorDataSource::createByfirstLastStep("test", 1, std::numeric_limits<double>::quiet_NaN(), 3);
            ASSERT_TRUE(spDs == nullptr);
        }
        {
            auto spDs = NumericGeneratorDataSource::createByfirstLastStep("test", 1, 2, std::numeric_limits<double>::quiet_NaN());
            ASSERT_TRUE(spDs == nullptr);
        }
    }

    // Testing query mask handling: handler is not expected to get called if numbers are not requested.
    {
        NumericGeneratorDataSource ds("test", 10);
        bool bCallbackCalled = false;
        ds.forEachElement_byColumn(0, DataQueryDetails(DataQueryDetails::DataMaskRowsAndStrings), [&](const SourceDataSpan&)
        {
            bCallbackCalled = true;
        });
        EXPECT_FALSE(bCallbackCalled);
    }
}

TEST(dfgQt, SQLiteDatabase)
{
    using namespace ::DFG_MODULE_NS(sql);

    EXPECT_TRUE(SQLiteDatabase::defaultConnectOptionsForWriting().isEmpty()); // Implementation detail, feel free to change is implementation changes.

    const QString sTestFilePath = "testfiles/generated/sqliteDatabase_test_file_63585.sqlite3";
    EXPECT_TRUE(!QFile::exists(sTestFilePath) || QFile::remove(sTestFilePath));

    // Testing that non-existing file flag won't be created by constructor when using read-only connect option.
    {
        SQLiteDatabase db(sTestFilePath);
        EXPECT_FALSE(db.isOpen());
    }

    // Testing custom connect options
    {
        SQLiteDatabase db(sTestFilePath, "testing");
        EXPECT_EQ("testing", db.connectOptions());
    }

    QMap<QString, QStringList> tableNameToColumnNames;
    tableNameToColumnNames["first_test_table"] = QStringList() << "Column0" << "Column1";
    tableNameToColumnNames["second_test_table"] = QStringList() << "id";
    const int nFirstTestTableRowCount = 10;

    const auto createTestContent = [](int r, int c) { return QString("%1,%2").arg(r).arg(c); };

    // Creating tables to test database
    {
        SQLiteDatabase db(sTestFilePath, SQLiteDatabase::defaultConnectOptionsForWriting());
        EXPECT_TRUE(db.isOpen());
        EXPECT_EQ(SQLiteDatabase::defaultConnectOptionsForWriting(), db.connectOptions());

        EXPECT_TRUE(db.transaction());
        auto statement = db.createQuery();
        const QString sCreateStatement = QString("CREATE TABLE %1 ('%2' TEXT, '%3' TEXT)").arg(tableNameToColumnNames.firstKey(), tableNameToColumnNames.first()[0], tableNameToColumnNames.first()[1]);
        EXPECT_FALSE(SQLiteDatabase::isSelectQuery(sCreateStatement));
        EXPECT_TRUE(statement.prepare(sCreateStatement));
        EXPECT_TRUE(statement.exec());
        EXPECT_TRUE(statement.exec(QString("CREATE TABLE %1 (id TEXT)").arg(tableNameToColumnNames.keys()[1])));

        const QString sInsertStatement = QString("INSERT INTO %1 VALUES(?, ?)").arg(tableNameToColumnNames.firstKey());
        EXPECT_TRUE(statement.prepare(sInsertStatement));
        EXPECT_FALSE(SQLiteDatabase::isSelectQuery(sInsertStatement));
        for (int r = 0; r < nFirstTestTableRowCount; ++r)
        {
            statement.addBindValue(createTestContent(r, 0));
            statement.addBindValue(createTestContent(r, 1));
            EXPECT_TRUE(statement.exec());
        }
        EXPECT_TRUE(db.commit());
        EXPECT_TRUE(db.isOpen());
        db.close();
        EXPECT_FALSE(db.isOpen());
    }

    // Reading from new test database
    {
        SQLiteDatabase db(sTestFilePath);
        EXPECT_TRUE(db.isOpen());
        EXPECT_EQ("QSQLITE_OPEN_READONLY", db.connectOptions()); // Implementation detail, feel free to change is implementation changes.

        EXPECT_EQ(tableNameToColumnNames.keys(), db.tableNames());

        auto query = db.createQuery();
        const QString sQuery = QString("SELECT * FROM %1;").arg(tableNameToColumnNames.firstKey());
        EXPECT_TRUE(db.isSelectQuery(sQuery));
        EXPECT_TRUE(query.prepare(sQuery));
        EXPECT_TRUE(query.exec());

        auto rec = query.record();
        const auto nColCount = rec.count();
        EXPECT_EQ(2, nColCount);

        EXPECT_EQ(tableNameToColumnNames.first()[0], rec.fieldName(0));
        EXPECT_EQ(tableNameToColumnNames.first()[1], rec.fieldName(1));

        // Reading records and filling table.
        size_t nRowCount = 0;
        for (int r = 0; query.next(); ++r, ++nRowCount)
        {
            for (int c = 0; c < nColCount; ++c)
                EXPECT_EQ(createTestContent(r, c), query.value(c).toString());
        }
        EXPECT_EQ(nFirstTestTableRowCount, nRowCount);
    }

    // Testing static getters
    {
        // isSQLiteFileExtension()
        EXPECT_TRUE(SQLiteDatabase::isSQLiteFileExtension("db"));
        EXPECT_TRUE(SQLiteDatabase::isSQLiteFileExtension("sqlite"));
        EXPECT_TRUE(SQLiteDatabase::isSQLiteFileExtension("sqlite3"));
        EXPECT_FALSE(SQLiteDatabase::isSQLiteFileExtension("sqlitee"));

        // isSQLiteFile()
        {
            const QString sCustomExtensionPath = sTestFilePath + "custom_ext";
            EXPECT_TRUE(QFile::copy(sTestFilePath, sCustomExtensionPath));
            EXPECT_TRUE(QFileInfo::exists(sCustomExtensionPath));
            EXPECT_TRUE(SQLiteDatabase::isSQLiteFile(sTestFilePath));
            EXPECT_TRUE(SQLiteDatabase::isSQLiteFile(sCustomExtensionPath));
            EXPECT_FALSE(SQLiteDatabase::isSQLiteFile(sCustomExtensionPath, true));
            EXPECT_FALSE(SQLiteDatabase::isSQLiteFile("testfiles/test_5x5_sep1F_content_row_comma_column.csv"));
            EXPECT_FALSE(SQLiteDatabase::isSQLiteFile("testfiles/test_5x5_sep1F_content_row_comma_column.csv", true));
            EXPECT_TRUE(QFile::remove(sCustomExtensionPath));
        }

        // getSQLiteFileTableNames()
        EXPECT_EQ(tableNameToColumnNames.keys(), SQLiteDatabase::getSQLiteFileTableNames(sTestFilePath));
        EXPECT_TRUE(SQLiteDatabase::getSQLiteFileTableNames("testfiles/test_5x5_sep1F_content_row_comma_column.csv").isEmpty());

        // getSQLiteFileTableColumnNames
        EXPECT_EQ(tableNameToColumnNames.values()[0], SQLiteDatabase::getSQLiteFileTableColumnNames(sTestFilePath, tableNameToColumnNames.keys()[0]));
        EXPECT_EQ(tableNameToColumnNames.values()[1], SQLiteDatabase::getSQLiteFileTableColumnNames(sTestFilePath, tableNameToColumnNames.keys()[1]));
    }

    EXPECT_TRUE(QFile::remove(sTestFilePath));
}

TEST(dfgQt, qStringToStringUtf8)
{
    using namespace DFG_ROOT_NS;
    using namespace DFG_MODULE_NS(qt);
    QString s;
    for (uint16 i = 32; i < 260; ++i)
        s.push_back(QChar(i));
    s.push_back(QChar(0x20AC)); // Euro-sign
    const char32_t u32 = 123456; // Arbitrary codepoint that requires surrogates.
    s.append(QString::fromUcs4(&u32, 1));
    const auto s8 = qStringToStringUtf8(s);
    const QString sRoundTrip = QString::fromUtf8(s8.beginRaw(), saturateCast<int>(s8.sizeInRawUnits()));
    EXPECT_EQ(s, sRoundTrip);
}

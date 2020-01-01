//#include <stdafx.h>
#include <dfg/qt/CsvItemModel.hpp>
#include <dfg/io.hpp>
#include <dfg/io/OmcByteStream.hpp>
#include <dfg/qt/containerUtils.hpp>
#include <dfg/qt/CsvTableView.hpp>

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
#include <gtest/gtest.h>
#include <QFileInfo>
#include <QDialog>
#include <QSpinBox>
#include <QGridLayout>
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

        const auto inputBytes = DFG_MODULE_NS(io)::fileToVector(sInputPath.toLatin1().data());
        const auto outputBytes = DFG_MODULE_NS(io)::fileToVector(sOutputPath.toLatin1().data());
        EXPECT_EQ(inputBytes, outputBytes);

        // Test in-memory saving
        {
            DFG_MODULE_NS(io)::DFG_CLASS_NAME(OmcByteStream)<std::vector<char>> strm;
            model.save(strm, saveOptions);
            const auto& outputBytesMc = strm.container();
            EXPECT_EQ(inputBytes, outputBytesMc);
        }
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
        char c[2] = { 'a' + r, '\0' };
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

TEST(dfgQt, CsvTableView_undoAfterRemoveRows)
{
    ::DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel) model;
    ::DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableView) view(nullptr);
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
    }

    // Testing actual class that has std::vector<QObjectStorage<T>> as member.
    {
        QObjectStorageTestWidget widgetTest;
    }
}

//#include <stdafx.h>
#include <dfg/qt/CsvItemModel.hpp>
#include <dfg/io.hpp>
#include <dfg/io/OmcByteStream.hpp>

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
#include <gtest/gtest.h>
#include <QFileInfo>
#include <QDialog>
#include <QSpinBox>
#include <QGridLayout>
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

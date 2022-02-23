#include "ContentGeneratorDialog.hpp"
#include "../../CsvTableView.hpp"
#include "../../CsvItemModel.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QComboBox>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt)
{

// Defines completer items for integer distribution parameters
const char* ContentGeneratorDialog::integerDistributionCompleters[] =
{
    "uniform, 0, 100",
    "binomial, 10, 0.5",
    "bernoulli, 0.5",
    "negative_binomial, 10, 0.5",
    "geometric, 0.5",
    "poisson, 10",
    nullptr // End-of-list marker
};

// Defines completer items for real distribution parameters
const char* ContentGeneratorDialog::realDistributionCompleters[] =
{
    "uniform, 0, 1",
    "normal, 0, 1",
    "cauchy, 0, 1",
    "exponential, 1",
    "gamma, 1, 1",
    "weibull, 1, 1",
    "extreme_value, 0, 1",
    "lognormal, 0, 1",
    "chi_squared, 1",
    "fisher_f, 1, 1",
    "student_t, 1",
    nullptr // End-of-list marker
};

// In syntax |x;y;z... items x,y,z define the indexes in table below that are parameters for given item.
const char ContentGeneratorDialog::szGenerators[] = "Random integers|9,Random doubles|10;6;7,Fill|8,Formula|2;6;7";

// Note: this is a POD-table (for notes about initialization of PODs, see
//    -http://stackoverflow.com/questions/2960307/pod-global-object-initialization
//    -http://stackoverflow.com/questions/15212261/default-initialization-of-pod-types-in-c
const ContentGeneratorDialog::PropertyDefinition ContentGeneratorDialog::arrPropDefs[] =
{
    // Key name               Value type           Value items (if key type is list)                      Default value
    { "Target"              , ValueTypeKeyList  , "Selection,Whole table"                               , "Selection"      , nullptr }, // 0
    { "Generator"           , ValueTypeKeyList  , szGenerators                                          , "Random integers", nullptr }, // 1
    { "Formula"             , ValueTypeCsvList  , ""                                                    , "trow + tcol"    , nullptr }, // 2
    { "Unused"              , ValueTypeInteger  , ""                                                    , ""               , nullptr }, // 3
    { "Unused"              , ValueTypeDouble   , ""                                                    , ""               , nullptr }, // 4
    { "Unused"              , ValueTypeDouble   , ""                                                    , ""               , nullptr }, // 5
    { "Format type"         , ValueTypeString   , ""                                                    , "g"              , nullptr }, // 6
    { "Format precision"    , ValueTypeUInteger , ""                                                    , ""               , nullptr }, // 7
    { "Fill string"         , ValueTypeString   , ""                                                    , ""               , nullptr }, // 8
    { "Parameters"          , ValueTypeCsvList  , ""                                                    , "uniform, 0, 100", integerDistributionCompleters }, // 9
    { "Parameters"          , ValueTypeCsvList  , ""                                                    , "uniform, 0, 1"  , realDistributionCompleters }  // 10
};

ContentGeneratorDialog::ContentGeneratorDialog(QWidget * pParent) :
    BaseClass(pParent),
    m_pLayout(nullptr),
    m_pGeneratorControlsLayout(nullptr),
    m_nLatestComboBoxItemIndex(-1)
{
    m_spSettingsTable.reset(new CsvTableView(nullptr, this));
    m_spSettingsModel.reset(new CsvItemModel);
    m_spDynamicHelpWidget.reset(new QLabel(this));
    m_spDynamicHelpWidget->setTextInteractionFlags(static_cast<Qt::TextInteractionFlags>(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard)); // Using static_cast for Qt 5.9 compatibility.
    m_spSettingsTable->setModel(m_spSettingsModel.get());
    m_spSettingsTable->setItemDelegate(new ComboBoxDelegate(this));


    m_spSettingsModel->insertColumns(0, 2);

    m_spSettingsModel->setColumnName(0, tr("Parameter"));
    m_spSettingsModel->setColumnName(1, tr("Value"));

    // Set streching for the last column
    {
        auto pHeader = m_spSettingsTable->horizontalHeader();
        if (pHeader)
            pHeader->setStretchLastSection(true);
    }
    //
    {
        auto pHeader = m_spSettingsTable->verticalHeader();
        if (pHeader)
            pHeader->setDefaultSectionSize(30);
    }
    m_pLayout = new QVBoxLayout(this);

    m_pLayout->addWidget(m_spSettingsTable.get());

    for (size_t i = 0; i <= LastNonParamPropertyId; ++i)
    {
        const auto nRow = m_spSettingsModel->rowCount();
        m_spSettingsModel->insertRows(nRow, 1);
        m_spSettingsModel->setData(m_spSettingsModel->index(nRow, 0), tr(arrPropDefs[i].m_pszName));
        m_spSettingsModel->setData(m_spSettingsModel->index(nRow, 1), arrPropDefs[i].m_pszDefault);
    }

    m_pLayout->addWidget(new QLabel(tr("Note: undo is not available for content generation (clears undo buffer)"), this));
    auto pStaticHelpLabel = new QLabel(tr("Available formats:\n"
        "    Numbers: see documentation of printf\n"
        "    Date times:\n"
        "        date_sec_local, date_msec_local: value is interpreted as epoch time in seconds or milliseconds and converted to local time\n"
        "        date_sec_utc, date_msec_utc: like date_sec_local, but converted to UTC time\n"
        "        Format precision accepts formats defined for QDateTime, for example yyyy-MM-dd HH:mm:ss.zzz. As an extension to QDateTime formats, WD can be used to insert weekday"), this);
    pStaticHelpLabel->setTextInteractionFlags(static_cast<Qt::TextInteractionFlags>(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard)); // Using static_cast for Qt 5.9 compatibility.
    m_pLayout->addWidget(pStaticHelpLabel);
    m_pLayout->addWidget(m_spDynamicHelpWidget.get());

    // Adding spacer item to avoid labels expanding, which doesn't look good.
    m_pLayout->addStretch();

    auto& rButtonBox = *(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel));

    DFG_QT_VERIFY_CONNECT(connect(&rButtonBox, SIGNAL(accepted()), this, SLOT(accept())));
    DFG_QT_VERIFY_CONNECT(connect(&rButtonBox, SIGNAL(rejected()), this, SLOT(reject())));

    m_pLayout->addWidget(&rButtonBox);

    createPropertyParams(PropertyIdGenerator, 0);

    DFG_QT_VERIFY_CONNECT(connect(m_spSettingsModel.get(), &QAbstractItemModel::dataChanged, this, &ContentGeneratorDialog::onDataChanged));

    updateDynamicHelp();
    removeContextHelpButtonFromDialog(this);
}

auto ContentGeneratorDialog::rowToPropertyId(const int i) const -> PropertyId
{
    if (i == PropertyIdGenerator)
        return PropertyIdGenerator;
    else
        return PropertyIdInvalid;
}

int ContentGeneratorDialog::propertyIdToRow(const PropertyId propId) const
{
    if (propId == PropertyIdGenerator)
        return 1;
    else
        return -1;
}

void ContentGeneratorDialog::setGenerateFailed(const bool bFailed, const QString& sFailReason)
{
    if (m_pLayout)
    {
        if (bFailed)
        {
            if (!m_spGenerateFailedNoteWidget)
            {
                m_spGenerateFailedNoteWidget.reset(new QLabel(this));
                m_pLayout->insertWidget(m_pLayout->count() - 1, m_spGenerateFailedNoteWidget.get());
            }
            const QString sMsg = (sFailReason.isEmpty()) ?
                tr("Note: Generating content failed; this may be caused by bad parameters") :
                tr("Note: Generating content failed with reason:<br>%1").arg(sFailReason);
            m_spGenerateFailedNoteWidget->setText(QString("<font color=\"#ff0000\">%1</font>").arg(sMsg));
        }
        if (m_spGenerateFailedNoteWidget)
            m_spGenerateFailedNoteWidget->setVisible(bFailed);
    }
}

void ContentGeneratorDialog::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>&/* roles*/)
{
    const auto isCellInIndexRect = [](int r, int c, const QModelIndex& tl, const QModelIndex& br)
    {
        return (r >= tl.row() && r <= br.row() && c >= tl.column() && c <= br.column());
    };
    if (isCellInIndexRect(1, 1, topLeft, bottomRight))
    {
        createPropertyParams(rowToPropertyId(1), m_nLatestComboBoxItemIndex);
        updateDynamicHelp();

        // Resizing table so that all parameter rows show
        if (m_spSettingsTable && m_spSettingsModel)
        {
            auto pVerticalHeader = m_spSettingsTable->verticalHeader();
            const auto nSectionSize = (pVerticalHeader) ? pVerticalHeader->sectionSize(0) : 30;
            const auto nShowCount = m_spSettingsModel->rowCount() + 1; // + 1 for header
            const auto nSize = nSectionSize * nShowCount + 2; // +2 to avoid scroll bar appearing, size isn't exactly row heigth * row count.
            m_spSettingsTable->setMinimumHeight(nSize);
            m_spSettingsTable->setMaximumHeight(nSize);
        }
    }
}

auto ContentGeneratorDialog::generatorType() const -> GeneratorType
{
    if (m_spSettingsModel)
        return generatorType(*m_spSettingsModel);
    else
        return GeneratorTypeUnknown;
}

auto ContentGeneratorDialog::generatorParameters(const int itemIndex) -> std::vector<std::reference_wrapper<const PropertyDefinition>>
{
    std::vector<std::reference_wrapper<const PropertyDefinition>> rv;
    QString sKeyList = arrPropDefs[PropertyIdGenerator].m_keyList;
    const auto keys = sKeyList.split(',');
    if (!DFG_ROOT_NS::isValidIndex(keys, itemIndex))
        return rv;
    QString sName = keys[itemIndex];
    const auto n = sName.indexOf('|');
    if (n < 0)
        return rv;
    sName.remove(0, n + 1);
    const auto paramIndexes = sName.split(';');
    for (const auto& s : paramIndexes)
    {
        bool bOk = false;
        const auto index = s.toInt(&bOk);
        if (bOk && DFG_ROOT_NS::isValidIndex(arrPropDefs, index))
            rv.push_back(arrPropDefs[index]);
        else
        {
            DFG_ASSERT(false);
        }
    }
    return rv;
}

void ContentGeneratorDialog::setCompleter(const int nTargetRow, const char** pCompleterItems)
{
    if (pCompleterItems)
    {
        auto& spCompleter = m_completers[pCompleterItems];
        if (!spCompleter) // If completer object does not exist, creating one.
        {
            spCompleter.reset(new QCompleter(this));
            QStringList completerItems;
            for (auto p = pCompleterItems; *p != nullptr; ++p) // Expecting array to end with nullptr-entry.
            {
                completerItems.push_back(*p);
            }
            spCompleter->setModel(new QStringListModel(completerItems, spCompleter.data())); // Model is owned by completer object.
        }
        if (!m_spCompleterDelegateForDistributionParams)
            m_spCompleterDelegateForDistributionParams.reset(new CsvTableViewCompleterDelegate(this, spCompleter.get()));
        m_spCompleterDelegateForDistributionParams->m_spCompleter = spCompleter.get();
        // Settings delegate for the row in order to enable completer. Note that delegate should not be set for first column, but done here since there seems to be no easy way to set it only
        // for the second cell in row.
        m_spSettingsTable->setItemDelegateForRow(nTargetRow, m_spCompleterDelegateForDistributionParams.data()); // Does not transfer ownership, delegate is parent owned by 'this'.
    }
    else
    {
        m_spSettingsTable->setItemDelegateForRow(nTargetRow, nullptr);
    }
}

void ContentGeneratorDialog::updateDynamicHelp()
{
    const auto genType = generatorType();
    if (genType == GeneratorTypeRandomIntegers)
    {
        m_spDynamicHelpWidget->setText(tr("Available integer distributions (hint: there's a completer in parameter input, trigger with ctrl+space):<br>"
            "<li><b>uniform, min, max</b>: Uniformly distributed values in range [min, max]. Requires min &lt;= max.</li>"
            "<li><b>binomial, count, probability</b>: Binomial distribution. Requires count &gt;= 0, probability within [0, 1]</li>"
            "<li><b>bernoulli, probability</b>: Bernoulli distribution. Requires probability within [0, 1]</li>"
            "<li><b>negative_binomial, count, probability</b>: Negative binomial distribution. Requires count &gt; 0, probability within ]0, 1]</li>"
            "<li><b>geometric, probability</b>: Geometric distribution. Requires probability within ]0, 1[</li>"
            "<li><b>poisson, mean</b>: Poisson distribution. Requires mean &gt; 0</li>"));
    }
    else if (genType == GeneratorTypeRandomDoubles)
    {
        m_spDynamicHelpWidget->setText(tr("Available real distributions (hint: there's a completer in input line, trigger with ctrl+space):<br>"
            "<li><b>uniform, min, max</b>: Uniformly distributed values in range [min, max[. Requires min &lt; max.</li>"
            "<li><b>normal, mean, stddev</b>: Normal distribution. Requires stddev &gt; 0</li>"
            "<li><b>cauchy, a, b</b>: Cauchy (Lorentz) distribution. Requires b &gt; 0</li>"
            "<li><b>exponential, lambda</b>: Exponential distribution. Requires lambda &gt; 0</li>"
            "<li><b>gamma, alpha, beta</b>: Gamma distribution. Requires alpha &gt; 0, beta &gt; 0</li>"
            "<li><b>weibull, a, b</b>: Geometric distribution. Requires a &gt; 0, b &gt; 0</li>"
            "<li><b>extreme_value, a, b</b>: Extreme value distribution. Requires b &gt; 0</li>"
            "<li><b>lognormal, m, s</b>: Lognormal distribution. Requires s &gt; 0</li>"
            "<li><b>chi_squared, n</b>: Chi squared distribution. Requires n &gt; 0</li>"
            "<li><b>fisher_f, a, b</b>: Fisher f distribution. Requires a &gt; 0, b &gt; 0</li>"
            "<li><b>student_t, n</b>: Student's t distribution. Requires n &gt; 0</li>"));
    }
    else if (genType == GeneratorTypeFormula)
    {
        ::DFG_MODULE_NS(math)::FormulaParser parser;
        if (!parser.defineRandomFunctions())
            QMessageBox::information(this, tr("Internal error"), tr("Internal error occurred causing documentation of random functions to be unavailable"));
        QString sFuncNames;
        decltype(sFuncNames.length()) nLastBrPos = 0;
        parser.forEachDefinedFunctionNameWhile([&](const ::DFG_ROOT_NS::StringViewC& sv)
            {
                sFuncNames += QString("%1, ").arg(QString::fromLatin1(sv.data(), sv.sizeAsInt()));
                if (sFuncNames.length() - nLastBrPos > 80)
                {
                    sFuncNames += "<br>&nbsp;&nbsp;&nbsp;&nbsp;";
                    nLastBrPos = sFuncNames.length();
                }
                return true;
            });
        sFuncNames.resize(sFuncNames.length() - 2); // Removing trailing ", ".
        m_spDynamicHelpWidget->setText(tr("Generates content using given formula.<br>"
            "<b>Available variables and content-related functions</b>:"
            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<b>cellValue(row, column)</b>: Value of cell at (row, column) (1-based indexes). Dates return epoch times (dates interpreted as UTC if timezone not specified), times return seconds since previous midnight. Content is generated from top to bottom.</li>"
            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<b>trow</b>: Row of cell to which content is being generated, 1-based index.</li>"
            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<b>tcol</b>: Column of cell to which content is being generated, 1-based index.</li>"
            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<b>rowcount</b>: Number of rows in table </li>"
            "<li>&nbsp;&nbsp;&nbsp;&nbsp;<b>colcount</b>: Number of columns in table</li>"
            "<b>Available functions:</b> %1<br>"
            "<b>Note</b>: trow and tcol are table indexes: even when generating to sorted or filtered table, these are rows/columns of the underlying table, not those of shown.<br>"
            "For example if a table of 5 rows is sorted so that row 5 is shown as first, trow value for that cell is 5, not 1. Currently there is no variable for accessing view rows/columns.<br>"
            "<b>Example</b>: rowcount - trow + 1 (this generates descending row indexes, 1-based index)<br>"
            "<b>Example</b>: cellValue(trow, tcol - 1) + cellValue(trow, tcol + 1) (value in each cell will be the sum of left and right neighbour cells)<br>"
            "<b>Example</b>: cellValue(trow - 1, tcol) * 2 (value is each cell will be twice the value in cell above)"

        ).arg(sFuncNames));
    }
    else
    {
        m_spDynamicHelpWidget->setText(QString()); // There are no instructions for this generator type so clear help.
    }
}

void ContentGeneratorDialog::createPropertyParams(const PropertyId prop, const int itemIndex)
{
    if (!m_spSettingsModel)
    {
        DFG_ASSERT(false);
        return;
    }
    if (prop == PropertyIdGenerator)
    {
        const auto& params = generatorParameters(itemIndex);
        const auto nParamCount = static_cast<int>(params.size());
        auto nBaseRow = propertyIdToRow(PropertyIdGenerator);
        if (nBaseRow < 0)
        {
            DFG_ASSERT(false);
            return;
        }
        ++nBaseRow;
        if (m_spSettingsModel->rowCount() < nBaseRow + nParamCount) // Need to add rows?
            m_spSettingsModel->insertRows(nBaseRow, nBaseRow + nParamCount - m_spSettingsModel->rowCount());
        if (m_spSettingsModel->rowCount() > nBaseRow + nParamCount) // Need to remove rows?
            m_spSettingsModel->removeRows(nBaseRow, m_spSettingsModel->rowCount() - (nBaseRow + nParamCount));
        for (int i = 0; i < nParamCount; ++i)
        {
            const auto nTargetRow = nBaseRow + i;
            m_spSettingsModel->setData(m_spSettingsModel->index(nTargetRow, 0), params[i].get().m_pszName);
            m_spSettingsModel->setData(m_spSettingsModel->index(nTargetRow, 1), params[i].get().m_pszDefault);

            setCompleter(nTargetRow, params[i].get().m_pCompleterItems);
        }
    }
    else
    {
        DFG_ASSERT_IMPLEMENTED(false);
    }
}

void ContentGeneratorDialog::setLatestComboBoxItemIndex(int index)
{
    m_nLatestComboBoxItemIndex = index;
}

auto ContentGeneratorDialog::generatorType(const CsvItemModel& csvModel) -> GeneratorType
{
    // TODO: use more reliable detection (string comparison does not work with tr())
    DFG_STATIC_ASSERT(GeneratorType_last == 4, "This implementation handles only 4 generator types");
    const auto& sGenerator = csvModel.data(csvModel.index(1, 1)).toString();
    if (sGenerator == "Random integers")
        return GeneratorTypeRandomIntegers;
    else if (sGenerator == "Random doubles")
        return GeneratorTypeRandomDoubles;
    else if (sGenerator == "Fill")
        return GeneratorTypeFill;
    else if (sGenerator == "Formula")
        return GeneratorTypeFormula;
    else
    {
        DFG_ASSERT_IMPLEMENTED(false);
        return GeneratorTypeUnknown;
    }
}

auto ContentGeneratorDialog::targetType(const CsvItemModel& csvModel) -> TargetType
{
    // TODO: use more reliable detection (string comparison does not work with tr())
    DFG_STATIC_ASSERT(TargetType_last == 2, "This implementation handles only two target types");
    const auto& sTarget = csvModel.data(csvModel.index(0, 1)).toString();
    if (sTarget == "Selection")
        return TargetTypeSelection;
    else if (sTarget == "Whole table")
        return TargetTypeWholeTable;
    else
    {
        DFG_ASSERT_IMPLEMENTED(false);
        return TargetTypeUnknown;
    }
}

ComboBoxDelegate::ComboBoxDelegate(ContentGeneratorDialog* parent) :
    m_pParentDialog(parent)
{
}

QWidget* ComboBoxDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (!index.isValid())
        return nullptr;
    const auto nRow = index.row();
    const auto nCol = index.column();
    if (nCol != 1) // Only second column is editable.
        return nullptr;

    else if (nRow < 2)
    {
        auto pComboBox = new QComboBox(parent);
        DFG_QT_VERIFY_CONNECT(connect(pComboBox, SIGNAL(currentIndexChanged(int)), pComboBox, SLOT(close()))); // TODO: check this, Qt's star delegate example connects differently here.
        return pComboBox;
    }
    else
        return BaseClass::createEditor(parent, option, index);
}

void ComboBoxDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto pModel = index.model();
    if (!pModel)
        return;
    auto pComboBoxDelegate = qobject_cast<QComboBox*>(editor);
    if (pComboBoxDelegate == nullptr)
    {
        BaseClass::setEditorData(editor, index);
        return;
    }
    //const auto keyVal = pModel->data(pModel->index(index.row(), index.column() - 1));
    const auto value = index.data(Qt::EditRole).toString();

    const auto& values = ComboBoxDelegate::valueListFromProperty(rowToPropertyId(index.row()));

    pComboBoxDelegate->addItems(values);
    pComboBoxDelegate->setCurrentText(value);
}

void ComboBoxDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (!model)
        return;
    auto pComboBoxDelegate = qobject_cast<QComboBox*>(editor);
    if (pComboBoxDelegate)
    {
        const auto& value = pComboBoxDelegate->currentText();
        const auto selectionIndex = pComboBoxDelegate->currentIndex();
        if (m_pParentDialog)
            m_pParentDialog->setLatestComboBoxItemIndex(selectionIndex);
        model->setData(index, value, Qt::EditRole);
    }
    else
        BaseClass::setModelData(editor, model, index);

}

void ComboBoxDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    DFG_UNUSED(index);
    if (editor)
        editor->setGeometry(option.rect);
}

auto ComboBoxDelegate::rowToPropertyId(const int r) -> PropertyId
{
    if (r == 0)
        return ContentGeneratorDialog::PropertyIdTarget;
    else if (r == 1)
        return ContentGeneratorDialog::PropertyIdGenerator;
    else
        return ContentGeneratorDialog::PropertyIdInvalid;
}

QStringList ComboBoxDelegate::valueListFromProperty(const PropertyId id)
{
    if (!isValidIndex(ContentGeneratorDialog::arrPropDefs, id))
        return QStringList();
    QStringList items = QString(ContentGeneratorDialog::arrPropDefs[id].m_keyList).split(',');
    for (auto iter = items.begin(); iter != items.end(); ++iter)
    {
        const auto n = iter->indexOf('|');
        if (n < 0)
            continue;
        iter->remove(n, iter->length() - n);
    }
    return items;
}

} } // Module dfg::qt

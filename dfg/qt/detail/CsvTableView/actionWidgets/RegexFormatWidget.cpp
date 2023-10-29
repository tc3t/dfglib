#include "RegexFormatWidget.hpp"
#include "../../../widgetHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QGridLayout>
    #include <QHBoxLayout>
    #include <QLabel>
    #include <QLineEdit>
    #include <QPalette>
    #include <QPlainTextEdit>
    #include <QRadioButton>
    #include <QScrollBar>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt) { namespace CsvTableViewActionWidgets {

RegexFormatWidget::RegexFormatWidget(QWidget* pParent)
    : BaseClass(pParent)
{
    auto pLayout = new QGridLayout(this);

    const auto regexConstructor = [](const QString& s)
    {
        return std::regex(s.toStdString());
    };

    using LineEditVH = LineEditWithValidityHighlighting;
    auto pRegexLineEdit = new LineEditVH([&](const QString& s) { return LineEditVH::validityChecker_throwingConstructor(s, regexConstructor); }, this);
    pRegexLineEdit->setObjectName("regexInput");
    pRegexLineEdit->setText("(.*)");

    auto pFormatLineEdit = new QLineEdit(this);
    pFormatLineEdit->setObjectName("formatInput");
    pFormatLineEdit->setText("Example match = '{1}'");

    auto pInputPreview = new QPlainTextEdit(this);
    pInputPreview->setObjectName("inputPreviewWidget");
    
    auto pOutputPreview = new QPlainTextEdit(this);
    pOutputPreview->setObjectName("outputPreviewWidget");
    pOutputPreview->setReadOnly(true);

    int nRow = 0;
    auto pDescriptionText = new QLabel(tr("Regex format provides advanced string transformations by regular expression matching "
        "and using up to 10 capturing groups as arguments to fmt-style format string.\n\n"
        "Example:\n"
        "    regex = '^(a)(b)(.*)'\n"
        "    format = '{0} transformed into {3}{2:b>3}{1}'\n"
        "    With input 'ab123', result is 'ab123 transformed into 123bbba'\n"), this); // Deletion by parent
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    pDescriptionText->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
#else
    pDescriptionText->setTextInteractionFlags(static_cast<Qt::TextInteractionFlags>(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard));
#endif
    pLayout->addWidget(pDescriptionText, nRow, 0, 1, 2);
    ++nRow;
    pLayout->addWidget(new QLabel(tr("Regular expression"), this), nRow, 0);
    pLayout->addWidget(pRegexLineEdit, nRow, 1);
    ++nRow;
    pLayout->addWidget(new QLabel(tr("Format string"), this), nRow, 0);
    pLayout->addWidget(pFormatLineEdit, nRow, 1);
    ++nRow;

    // Controls for non-match behaviour
    {
        auto spNonMatchLayout = std::make_unique<QHBoxLayout>();
        spNonMatchLayout->addWidget(new QLabel(tr("Action on non-match:")));
        auto pRadioButtonClear = new QRadioButton(tr("Clear"), this);
        pRadioButtonClear->setObjectName("radioButtonNonMatchClear");
        auto pRadioButtonKeep = new QRadioButton(tr("Keep"), this);
        connect(pRadioButtonClear, &QRadioButton::toggled, this, &RegexFormatWidget::updatePreviewOutput);
        pRadioButtonClear->setChecked(true);
        spNonMatchLayout->addWidget(pRadioButtonClear);
        spNonMatchLayout->addWidget(pRadioButtonKeep);
        spNonMatchLayout->addStretch(10);
        pLayout->addLayout(spNonMatchLayout.release(), nRow, 0, 1, 2); // Sublayout becomes child of pLayout
        ++nRow;
    }

    // Creating sublayout for input/output previews
    {
        auto spPreviewLayout = std::make_unique<QGridLayout>();
        spPreviewLayout->addWidget(new QLabel(tr("Preview input"), this), 0, 0);
        spPreviewLayout->addWidget(new QLabel(tr("Preview output"), this), 0, 2);
        spPreviewLayout->addWidget(pInputPreview, 1, 0);
        spPreviewLayout->addWidget(pOutputPreview, 1, 2);
        spPreviewLayout->setColumnMinimumWidth(1, 20); // Adds space between input and output widgets
        pLayout->addLayout(spPreviewLayout.release(), nRow, 0, 1, 2); // Sublayout becomes child of pLayout
        ++nRow;
    }

    pLayout->addWidget(addOkCancelButtonBoxToDialog(this), nRow, 1);
    ++nRow;

    removeContextHelpButtonFromDialog(this);
    this->setWindowTitle(tr("Regex format"));

    // Setting scroll bars so that sliding input preview vertical slider automatically moves output scroll position
    {
        auto pInputVertScroll = pInputPreview->verticalScrollBar();
        auto pOutputVertScroll = pOutputPreview->verticalScrollBar();
        if (pInputVertScroll && pOutputVertScroll)
            connect(pInputVertScroll, &QAbstractSlider::valueChanged, pOutputVertScroll, &QAbstractSlider::setSliderPosition);
    }

    // Connecting textChanged-signals to update UI as needed
    {
        // Changing regex triggers output preview update.
        connect(pRegexLineEdit, &QLineEdit::textChanged, this, &RegexFormatWidget::updatePreviewOutput);
        // Changing format string triggers output preview update
        connect(pFormatLineEdit, &QLineEdit::textChanged, this, &RegexFormatWidget::updatePreviewOutput);
        // Changing preview input triggers output preview update
        connect(pInputPreview, &QPlainTextEdit::textChanged, this, &RegexFormatWidget::updatePreviewOutput);
    }
}

RegexFormatWidget::~RegexFormatWidget() = default;

void RegexFormatWidget::setPreviewInput(const QStringList& inputPreview)
{
    auto pInputPreviewWidget = this->findChild<QPlainTextEdit*>("inputPreviewWidget");
    DFG_ASSERT_CORRECTNESS(pInputPreviewWidget != nullptr);
    if (pInputPreviewWidget)
        pInputPreviewWidget->setPlainText(inputPreview.join('\n'));
}

void RegexFormatWidget::updatePreviewOutput()
{
    auto pInputPreview = this->findChild<QPlainTextEdit*>("inputPreviewWidget");
    auto pOutputPreview = this->findChild<QPlainTextEdit*>("outputPreviewWidget");
    if (pInputPreview == nullptr || pOutputPreview == nullptr)
    {
        DFG_ASSERT_CORRECTNESS(false);
        return;
    }

    try
    {
        const auto params = this->getParams();
        std::regex re(params.m_sRegex.rawStorage());
        const auto sFormat = params.m_sFormat.rawStorage();
        auto examples = pInputPreview->toPlainText().split('\n');
        for (auto& e : examples)
        {
            std::string sOutput;
            const auto bApply = CsvTableViewActionRegexFormat::applyToSingle(
                sOutput,
                e.toStdString(),
                re,
                sFormat,
                params.m_nonMatchBehaviour,
                nullptr
            );
            if (bApply)
                e = QString::fromStdString(sOutput);
        }
        pOutputPreview->setPlainText(examples.join('\n'));
        // Re-applying slider position from input widget since setPlainText resets it.
        {
            auto pInputVertScrollBar = pInputPreview->verticalScrollBar();
            auto pOutputVertScrollBar = pOutputPreview->verticalScrollBar();
            if (pInputVertScrollBar && pOutputVertScrollBar)
                pOutputVertScrollBar->setSliderPosition(pInputVertScrollBar->sliderPosition());
        }
    }
    catch (...)
    {
        pOutputPreview->setPlainText(QString());
    }
}

QString RegexFormatWidget::getRegex() const
{
    return getChildWidgetString<QLineEdit*>(this, "regexInput");
}

QString RegexFormatWidget::getFormatString() const
{
    return getChildWidgetString<QLineEdit*>(this, "formatInput");
}

auto RegexFormatWidget::getParams() const -> CsvTableViewActionRegexFormatParams
{
    auto pClearButton = this->findChild<const QRadioButton*>("radioButtonNonMatchClear");
    DFG_ASSERT_CORRECTNESS(pClearButton != nullptr);
    const auto nonMatchBehaviour = (pClearButton && pClearButton->isChecked()) ? CsvTableViewActionRegexFormatParams::NonMatchBehaviour::clear : CsvTableViewActionRegexFormatParams::NonMatchBehaviour::keep;
    return CsvTableViewActionRegexFormatParams(qStringToStringUtf8(this->getRegex()), qStringToStringUtf8(this->getFormatString()), nonMatchBehaviour);
}

}}} // namespace dfg::qt::CsvTableViewActionWidgets

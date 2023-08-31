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
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt) { namespace CsvTableViewActionWidgets {

RegexFormatWidget::RegexFormatWidget(QWidget* pParent)
    : BaseClass(pParent)
{
    auto pLayout = new QGridLayout(this);

    auto pRegexLineEdit = new QLineEdit(this);
    pRegexLineEdit->setObjectName("regexInput");

    auto pFormatLineEdit = new QLineEdit(this);
    pFormatLineEdit->setObjectName("formatInput");

    auto pInputPreview = new QPlainTextEdit(this);
    pInputPreview->setObjectName("inputPreviewWidget");
    
    auto pOutputPreview = new QPlainTextEdit(this);
    pOutputPreview->setObjectName("outputPreviewWidget");
    pOutputPreview->setReadOnly(true);

    int nRow = 0;
    pLayout->addWidget(new QLabel(tr("Format with regular expression allows matching content by regular expression and use up to 10 subexpressions (capturing groups) "
        "as arguments to fmt-style format string.\n"
        "Example: regex = '^(a)(b)(.*)', format = '{0} transformed into {3}{2}{1}'. With input 'ab123', result is 'ab123 transformed to 123ba'"), this), nRow, 0, 1, 2);
    ++nRow;
    pLayout->addWidget(new QLabel(tr("Regular expression"), this), nRow, 0);
    pLayout->addWidget(pRegexLineEdit, nRow, 1);
    ++nRow;
    pLayout->addWidget(new QLabel(tr("Format string"), this), nRow, 0);
    pLayout->addWidget(pFormatLineEdit, nRow, 1);
    ++nRow;

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

    pLayout->addWidget(addOkCancelButtonBoxToDialog(this), nRow, 1);
    ++nRow;

    removeContextHelpButtonFromDialog(this);
    this->setWindowTitle(tr("Format with regular expression"));

    // Setting scroll bars so that sliding input preview vertical slider automatically moves output scroll position
    {
        auto pInputVertScroll = pInputPreview->verticalScrollBar();
        auto pOutputVertScroll = pOutputPreview->verticalScrollBar();
        if (pInputVertScroll && pOutputVertScroll)
            connect(pInputVertScroll, &QAbstractSlider::valueChanged, pOutputVertScroll, &QAbstractSlider::setSliderPosition);
    }

    // Connecting textChanged-signals to update UI as needed
    {
        const auto defaultLineEditBackgroundColor = pRegexLineEdit->palette().color(QPalette::Base);
        // Changing regex triggers syntax check for it and updates output preview.
        connect(pRegexLineEdit, &QLineEdit::textChanged, this, [=](const QString& sText)
            {
                bool bIsValid = true;
                try
                {
                    std::regex(sText.toStdString());
                }
                catch (...)
                {
                    bIsValid = false;
                }
                QPalette palette = pRegexLineEdit->palette();
                // If regex is invalid, setting reddish background
                palette.setColor(QPalette::Base, (bIsValid) ? defaultLineEditBackgroundColor : QColor(255, 0, 0, 32));
                pRegexLineEdit->setPalette(palette);
                updatePreviewOutput();
            });

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

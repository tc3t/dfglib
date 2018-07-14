#include "TableEditor.hpp"
#include "CsvTableView.hpp"
#include "CsvItemModel.hpp"

#include "connectHelper.hpp"

#include "qtIncludeHelpers.hpp"
DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QGridLayout>
#include <QLineEdit>
#include <QStatusBar>
#include <QApplication>
#include <QDesktopWidget>
#include <QHeaderView>
DFG_END_INCLUDE_QT_HEADERS

#define DFG_TABLEEDITOR_LOG_WARNING(x) // Placeholder for logging warning

DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::DFG_CLASS_NAME(TableEditor)()
{
    // Model
    m_spTableModel.reset(new DFG_CLASS_NAME(CsvItemModel));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableModel.get(), &DFG_CLASS_NAME(CsvItemModel)::sigSourcePathChanged, this, &DFG_CLASS_NAME(TableEditor)::onSourcePathChanged));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableModel.get(), &DFG_CLASS_NAME(CsvItemModel)::sigOnNewSourceOpened, this, &DFG_CLASS_NAME(TableEditor)::onNewSourceOpened));

    // View
    m_spTableView.reset(new DFG_CLASS_NAME(CsvTableView)(this));
    m_spTableView->setModel(m_spTableModel.get());

    // Source path line edit
    m_spLineEditSourcePath.reset(new QLineEdit());
    m_spLineEditSourcePath->setReadOnly(true);
    m_spLineEditSourcePath->setPlaceholderText(tr("<no file path available>"));

    // Status bar
    {
        m_spStatusBar.reset(new DFG_CLASS_NAME(TableEditorStatusBar));
    }

    // Layout
    {
        std::unique_ptr<QGridLayout> spLayout(new QGridLayout);
        spLayout->addWidget(m_spLineEditSourcePath.get(), 0, 0);
        spLayout->addWidget(m_spTableView.get(), 1, 0);
        spLayout->addWidget(m_spStatusBar.get(), 2, 0);
        delete layout();
        setLayout(spLayout.release());
    }

    // Set default window size.
    {
        auto desktop = QApplication::desktop();
        if (desktop)
        {
            const auto screenRect = desktop->screenGeometry(this); // Returns screen geometry of the screen that contains 'this' widget.
            resize(screenRect.size() * 0.75); // Fill 0.75 of the whole screen in both directions (arbitrary default value)
        }
    }

    resizeColumnsToView();
}

DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::~DFG_CLASS_NAME(TableEditor)()
{
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::tryOpenFileFromPath(QString path)
{
    if (!m_spTableModel || (m_spTableModel && m_spTableModel->isModified()))
        return false;

    return m_spTableModel->openFile(path);
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onSourcePathChanged()
{
    if (!m_spTableModel || !m_spLineEditSourcePath)
        return;
    m_spLineEditSourcePath->setText(m_spTableModel->getFilePath());
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onNewSourceOpened()
{
    if (!m_spTableModel)
        return;
    auto& model = *m_spTableModel;
    if (m_spLineEditSourcePath)
        m_spLineEditSourcePath->setText(model.getFilePath());
    if (m_spStatusBar)
        m_spStatusBar->showMessage(tr("Reading lasted %1 s").arg(model.latestReadTimeInSeconds(), 0, 'g', 4));
    resizeColumnsToView();
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::resizeColumnsToView(ColumnResizeStyle style)
{
    auto viewport = (m_spTableView) ? m_spTableView->viewport() : nullptr;
    if (!viewport || !m_spTableModel || m_spTableModel->getColumnCount() == 0)
        return;
    if (style == ColumnResizeStyle_evenlyDistributed)
    {
        const auto width = m_spTableView->viewport()->width();
        const int sectionSize = width / m_spTableModel->getColumnCount();
        m_spTableView->horizontalHeader()->setDefaultSectionSize(sectionSize);
    }
    else
        DFG_TABLEEDITOR_LOG_WARNING(QString("Requested resizeColumnsToView with unknown style '%1'").arg(static_cast<int>(style)));
}

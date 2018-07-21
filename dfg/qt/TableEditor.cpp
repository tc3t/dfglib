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
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QFileInfo>
#include <QHeaderView>
DFG_END_INCLUDE_QT_HEADERS

#define DFG_TABLEEDITOR_LOG_WARNING(x) // Placeholder for logging warning

DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::DFG_CLASS_NAME(TableEditor)()
{
    // Model
    m_spTableModel.reset(new DFG_CLASS_NAME(CsvItemModel));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableModel.get(), &DFG_CLASS_NAME(CsvItemModel)::sigSourcePathChanged, this, &ThisClass::onSourcePathChanged));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableModel.get(), &DFG_CLASS_NAME(CsvItemModel)::sigOnNewSourceOpened, this, &ThisClass::onNewSourceOpened));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableModel.get(), &DFG_CLASS_NAME(CsvItemModel)::sigModifiedStatusChanged, this, &ThisClass::onModifiedStatusChanged));

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

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::updateWindowTitle()
{
    const auto filePath = (m_spTableModel) ? m_spTableModel->getFilePath() : QString();
    // Note: [*] is a placeholder for modified-indicator (see QWidget::windowModified)
    QString prename = (!filePath.isEmpty()) ? QFileInfo(filePath).fileName() + "[*] - " : QString();
    QString title(prename + QCoreApplication::applicationName());
    setWindowModified(m_spTableModel && m_spTableModel->isModified());
    setWindowTitle(title);
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onSourcePathChanged()
{
    if (!m_spTableModel || !m_spLineEditSourcePath)
        return;
    m_spLineEditSourcePath->setText(m_spTableModel->getFilePath());

    updateWindowTitle();
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
    updateWindowTitle();
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onModifiedStatusChanged(const bool b)
{
    setWindowModified(b);
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

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::setAllowApplicationSettingsUsage(const bool b)
{
    if (m_spTableView)
        m_spTableView->setAllowApplicationSettingsUsage(b);
}

namespace
{
    static bool handleExitConfirmationAndReturnTrueIfCanExit(DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableView)* view)
    {
        auto model = (view) ? view->csvModel() : nullptr;
        if (!model)
            return true;

        // Finish edits before checking modified as otherwise ongoing edit whose editor
        // has not been closed would not be detected.
        // In practice: user opens file, edits a cell and presses X while cell content editor is active,
        // without line below:
        // -> this function would be called before model's setData()-handler
        // -> Since no setData()-calls have been made, this function gets 'model->isModified() == false'
        //    and editor closes without confirmation.
        view->finishEdits();

        if (model->isModified())
        {
            const auto existingPath = model->getFilePath();
            const QString uiPath = (!existingPath.isEmpty()) ?
                                QCoreApplication::tr("(path: %1)").arg(existingPath)
                                : QCoreApplication::tr("(no path exists, it will be asked if saving is chosen)");
            const auto rv = QMessageBox::question(nullptr,
                                                      QCoreApplication::tr("Save to file?"),
                                                      QCoreApplication::tr("Content has been edited, save to file?\n%1").arg(uiPath),
                                                      QMessageBox::Yes,
                                                      QMessageBox::No,
                                                      QMessageBox::Cancel);
            if (rv == QMessageBox::Cancel)
                return false;

            if (rv == QMessageBox::Yes)
                view->save();
        }

        return true;
    }
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::closeEvent(QCloseEvent* event)
{
    if (!event)
    {
        BaseClass::closeEvent(event);
        return;
    }

    if (handleExitConfirmationAndReturnTrueIfCanExit(m_spTableView.get()))
        event->accept();
    else
        event->ignore();
}

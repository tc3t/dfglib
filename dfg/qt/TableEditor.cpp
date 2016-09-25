#include "TableEditor.hpp"
#include "CsvTableView.hpp"
#include "CsvItemModel.hpp"

#include "connectHelper.hpp"

#include "qtIncludeHelpers.hpp"
DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QGridLayout>
#include <QLineEdit>
DFG_END_INCLUDE_QT_HEADERS

DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::DFG_CLASS_NAME(TableEditor)()
{
    // Model
    m_spTableModel.reset(new DFG_CLASS_NAME(CsvItemModel));
    DFG_QT_VERIFY_CONNECT(connect(m_spTableModel.get(), &DFG_CLASS_NAME(CsvItemModel)::sigSourcePathChanged, this, &DFG_CLASS_NAME(TableEditor)::onNewSourceOpened));

    // View
    m_spTableView.reset(new DFG_CLASS_NAME(CsvTableView)(this));
    m_spTableView->setModel(m_spTableModel.get());

    // Source path line edit
    m_spLineEditSourcePath.reset(new QLineEdit());
    m_spLineEditSourcePath->setReadOnly(true);
    m_spLineEditSourcePath->setPlaceholderText(tr("<no file path available>"));

    // Layout
    {
        std::unique_ptr<QGridLayout> spLayout(new QGridLayout);
        spLayout->addWidget(m_spLineEditSourcePath.get(), 0, 0);
        spLayout->addWidget(m_spTableView.get(), 1, 0);
        delete layout();
        setLayout(spLayout.release());
    }

}

DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::~DFG_CLASS_NAME(TableEditor)()
{
}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(TableEditor)::onNewSourceOpened()
{
    if (m_spLineEditSourcePath && m_spTableModel)
        m_spLineEditSourcePath->setText(m_spTableModel->getFilePath());
}

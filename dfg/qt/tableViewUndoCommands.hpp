#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QTableView>
#include <QUndoCommand>
DFG_END_INCLUDE_QT_HEADERS


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt) {
	DFG_SUB_NS(undoCommands)
	{
		enum InsertRowType { InsertRowTypeAfter, InsertRowTypeBefore };

		class DFG_CLASS_NAME(TableViewUndoCommandInsertRow) : public QUndoCommand
		{
		public:
			typedef DFG_CLASS_NAME(TableViewUndoCommandInsertRow) ThisClass;
			// If nRow is given, InsertRowType is ignored.
			ThisClass(QTableView* pTableView, InsertRowType type, int nRow = -1)
				:
				m_pView(pTableView),
				m_pModel((pTableView) ? pTableView->model() : nullptr),
				m_nWhere(nRow)
			{
				if (!m_pView || !m_pModel)
					return;
				if (m_nWhere >= 0)
					m_nWhere = Min(m_nWhere, m_pModel->rowCount());
				else
				{
					auto index = m_pView->currentIndex();
					if (type == InsertRowTypeBefore)
					{
						if (index.row() < 0)
							m_nWhere = m_pModel->rowCount();
						else
							m_nWhere = index.row();
					}
					else
						m_nWhere = (index.row() >= 0) ? index.row() + 1 : m_pModel->rowCount();
				}

				QString sDesc;
				sDesc = QString("Insert row to %1").arg(m_nWhere);
				setText(sDesc);
			}

			void undo()
			{
				m_pModel->removeRow(m_nWhere);
			}
			void redo()
			{
				m_pModel->insertRow(m_nWhere);
			}
		private:
			QTableView* m_pView;
			QAbstractItemModel* m_pModel;
			int m_nWhere;	// Stores the index of the new row.
		};
	}
} }

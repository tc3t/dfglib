#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QTableView>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{
	class DFG_CLASS_NAME(TableView) : public QTableView
	{
	public:
		typedef DFG_CLASS_NAME(TableView) ThisClass;
		typedef QTableView BaseClass;

		ThisClass(QWidget* pParent) : BaseClass(pParent)
		{

		}

		static void invertSelection(QTableView* pTable)
		{
			const auto pModel = (pTable) ? pTable->model() : nullptr;
			if (pModel == nullptr)
				return;
			const auto rowCount = pModel->rowCount();
			const auto colCount = pModel->columnCount();
			auto pSelectionModel = pTable->selectionModel();
			for (int r = 0; r<rowCount; ++r)
			{
				for (int c = 0; c<colCount; ++c)
				{
					QModelIndex index = pModel->index(r, c);
					pSelectionModel->select(index, QItemSelectionModel::Toggle);
				}
			}
		}

		void invertSelection()
		{
			invertSelection(this);
		}
	};
}}

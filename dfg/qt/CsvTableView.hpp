#pragma once

#include "../dfgDefs.hpp"
#include "TableView.hpp"
#include <memory>

class QUndoStack;
class QAbstractProxyModel;

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

	class DFG_CLASS_NAME(CsvItemModel);

	// View for showing CsvItemModel.
	// TODO: test
	// TODO: Fix implementation to work with proxy models.
	class DFG_CLASS_NAME(CsvTableView) : public DFG_CLASS_NAME(TableView)
	{
		Q_OBJECT

	public:
		typedef DFG_CLASS_NAME(TableView) BaseClass;
		typedef DFG_CLASS_NAME(CsvTableView) ThisClass;
		typedef DFG_CLASS_NAME(CsvItemModel) CsvModel;
		ThisClass(QWidget* pParent);

		void setModel(QAbstractItemModel* pModel) override;
		CsvModel* csvModel();

		// Returns the smallest row index in the current view selection,
		// row count if no selection exists.
		int getFirstSelectedViewRow() const;

		QModelIndex getFirstSelectedItem(QAbstractProxyModel* pProxy) const;

		// Returns list of row indexes of column @p nCol.
		// If proxy model is given, the returned indexes will correspond
		// to the indexes of the underlying model, otherwise they will be
		// {0, 1,..., rowCount()-1}
		std::vector<int> getRowsOfCol(const int nCol, QAbstractProxyModel* pProxy) const;

		// Returned list is free of duplicates. If @p pProxy != nullptr,
		// the selected indexes will be mapped by the proxy.
		std::vector<int> getRowsOfSelectedItems(QAbstractProxyModel* pProxy, const bool bSort = true) const;

		// Returned list of selected indexes. If @p pProxy != nullptr,
		// the selected indexes will be mapped by the proxy.
		QModelIndexList getSelectedItemIndexes(QAbstractProxyModel* pProxy) const;

		std::vector<int> getDataModelRowsOfSelectedItems(QAbstractProxyModel& rProxy, const bool bSort = true) const
		{
			return getRowsOfSelectedItems(&rProxy, bSort);
		}

		void invertSelection();

		bool isRowMode() const;

		QAbstractProxyModel* getProxyModelPtr() { return m_spProxyModel.get(); }

	private:
		template <class T, class Param0_T>
		bool executeAction(Param0_T&& p0);

		template <class T, class Param0_T, class Param1_T>
		bool executeAction(Param0_T&& p0, Param1_T&& p1);

		template <class T, class Param0_T, class Param1_T, class Param2_T>
		bool executeAction(Param0_T&& p0, Param1_T&& p1, Param2_T&& p2);

	public slots:
		bool openFromFile();
		bool saveToFile();
		bool clearSelected();
		bool insertRowHere();
		bool insertRowAfterCurrent();
		bool insertColumn();
		bool insertColumnAfterCurrent();
		bool paste();
		bool copy();
		bool cut();

		bool deleteCurrentColumn();
		bool deleteCurrentColumn(const int nCol);
		bool deleteSelectedRow();

		bool moveFirstRowToHeader();
		bool moveHeaderToFirstRow();
		/*
		void pasteColumn();
		void pasteColumn(const int nCol);
		void copyColumn();
		void copyColumn(const int nCol);
		void moveColumnRight();
		void moveColumnLeft();
		void moveRowDown();
		void moveRowUp();
		void renameColumn();
		void renameColumn(const int nCol);
		
		void insertColumnAfterCurrent();
		void insertColumnAfterCurrent(const int nCol);
		void insertColumn(const int nCol);
		void insertAfterCurrent();
		void insert();
		void deleteSelected();
		*/

	protected:
		void contextMenuEvent(QContextMenuEvent* pEvent) override;

	public:
		std::unique_ptr<QUndoStack> m_spUndoStack;
		std::unique_ptr<QAbstractProxyModel> m_spProxyModel;

	};

} } // module namespace

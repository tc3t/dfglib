#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QTableView>
#include <QUndoCommand>
DFG_END_INCLUDE_QT_HEADERS


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt) {

    class DFG_CLASS_NAME(UndoCommand) : public QUndoCommand
    {
    public:
        // Direct redo without need to construct the object first, which may take massive amount of resources compared to direct action.
        // For example deleting content from a N item row:
        //      -doing it with T(params).redo() stores old content (N strings) for undo although it will never use it.
        template <class Impl_T, class Param0_T>
        static void directRedo(Param0_T&& p0)
        {
            Impl_T::template privDirectRedo<Impl_T>(std::forward<Param0_T>(p0));
        }

       template <class Impl_T, class Param0_T, class Param1_T>
       static void directRedo(Param0_T&& p0, Param1_T&& p1)
       {
            Impl_T::template privDirectRedo<Impl_T>(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1));
       }

       template <class Impl_T, class Param0_T, class Param1_T, class Param2_T>
       static void directRedo(Param0_T&& p0, Param1_T&& p1, Param2_T&& p2)
       {
            Impl_T::template privDirectRedo<Impl_T>(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1), std::forward<Param2_T>(p2));
       }

        template <class Impl_T, class Param0_T>
        static void privDirectRedo(Param0_T&& p0)
        {
            Impl_T(std::forward<Param0_T>(p0)).redo();
        }

       template <class Impl_T, class Param0_T, class Param1_T>
       static void privDirectRedo(Param0_T&& p0, Param1_T&& p1)
       {
           Impl_T(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1)).redo();
       }

       template <class Impl_T, class Param0_T, class Param1_T, class Param2_T>
       static void privDirectRedo(Param0_T&& p0, Param1_T&& p1, Param2_T&& p2)
       {
           Impl_T(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1), std::forward<Param2_T>(p2)).redo();
       }
    };

    DFG_SUB_NS(undoCommands)
    {
        enum InsertRowType { InsertRowTypeAfter, InsertRowTypeBefore };

        class DFG_CLASS_NAME(TableViewUndoCommandInsertRow) : public DFG_CLASS_NAME(UndoCommand)
        {
        public:
            typedef DFG_CLASS_NAME(TableViewUndoCommandInsertRow) ThisClass;
            // If nRow is given, InsertRowType is ignored.
            DFG_CLASS_NAME(TableViewUndoCommandInsertRow)(QTableView* pTableView, InsertRowType type, int nRow = -1)
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

        class DFG_CLASS_NAME(CsvTableViewActionResizeTable) : public DFG_CLASS_NAME(UndoCommand)
        {
        public:
            DFG_CLASS_NAME(CsvTableViewActionResizeTable)(QTableView* pView, const int nNewRowCount, const int nNewColCount)
                : m_pView(pView),
                m_nOldRowCount(-1),
                m_nOldColCount(-1),
                m_nNewRowCount(-1),
                m_nNewColCount(-1)
            {
                auto pModel = (m_pView) ? m_pView->model() : nullptr;
                if (!pModel)
                    return;

                m_nOldRowCount = pModel->rowCount();
                m_nOldColCount = pModel->columnCount();
                m_nNewRowCount = nNewRowCount;
                m_nNewColCount = nNewColCount;

                QString sDesc = m_pView->tr("Resize to (%1, %2)").arg(m_nNewRowCount).arg(m_nNewColCount);
                setText(sDesc);
            }

            void impl(const int nTargetRowCount, const int nTargetColCount)
            {
                auto pModel = (m_pView) ? m_pView->model() : nullptr;
                if (!pModel)
                    return;

                const int nCurrentRowCount = pModel->rowCount();
                const int nCurrentColCount = pModel->columnCount();

                // Change row count
                if (nCurrentRowCount >= 0 && nTargetRowCount >= 0)
                {
                    const auto nPositiveCount = (nCurrentRowCount < nTargetRowCount) ? nTargetRowCount - nCurrentRowCount : nCurrentRowCount - nTargetRowCount;
                    if (nCurrentRowCount < nTargetRowCount)
                        pModel->insertRows(nCurrentRowCount, nPositiveCount);
                    else
                        pModel->removeRows(nCurrentRowCount - nPositiveCount, nPositiveCount);
                }

                // Change column count
                if (nCurrentColCount >= 0 && nTargetColCount >= 0)
                {
                    const auto nPositiveCount = (nCurrentColCount < nTargetColCount) ? nTargetColCount - nCurrentColCount : nCurrentColCount - nTargetColCount;
                    if (nCurrentColCount < nTargetColCount)
                        pModel->insertColumns(nCurrentColCount, nPositiveCount);
                    else
                        pModel->removeColumns(nCurrentColCount - nPositiveCount, nPositiveCount);
                }
            }

            void undo()
            {
                impl(m_nOldRowCount, m_nOldColCount);
            }

            void redo()
            {
                impl(m_nNewRowCount, m_nNewColCount);
            }

        private:
            QTableView* m_pView;
            int m_nOldRowCount;
            int m_nOldColCount;
            int m_nNewRowCount;
            int m_nNewColCount;

        };
    }
} }

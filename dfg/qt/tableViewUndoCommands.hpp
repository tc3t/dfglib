#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"
#include "../numericTypeTools.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QTableView>
#include <QUndoCommand>
DFG_END_INCLUDE_QT_HEADERS


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt) {

    class UndoCommand : public QUndoCommand
    {
    public:
        // If UndoCommand has static privDirectRedo() function, calling it directly to avoid constructing the undo object,
        // which may take massive amount of resources compared to direct action.
        // For example deleting content from a N item row:
        //      -doing it with T(params).redo() stores old content (N strings), which are never used if undo is disabled.
        template <class Impl_T, class Param0_T>
        static void directRedo(Param0_T&& p0)
        {
            Impl_T::template privDirectRedo<Impl_T>(std::forward<Param0_T>(p0));
        }

        // privDirectRedo() optimization overload for 2 args.
       template <class Impl_T, class Param0_T, class Param1_T>
       static void directRedo(Param0_T&& p0, Param1_T&& p1)
       {
            Impl_T::template privDirectRedo<Impl_T>(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1));
       }

       // privDirectRedo() optimization overload for 3 args.
       template <class Impl_T, class Param0_T, class Param1_T, class Param2_T>
       static void directRedo(Param0_T&& p0, Param1_T&& p1, Param2_T&& p2)
       {
            Impl_T::template privDirectRedo<Impl_T>(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1), std::forward<Param2_T>(p2));
       }

       // Default directRedo() implementation through new UndoCommand object.
        template <class Impl_T, class Param0_T>
        static void privDirectRedo(Param0_T&& p0)
        {
            Impl_T(std::forward<Param0_T>(p0)).redo();
        }

        // Default directRedo() implementation overload for 2 args.
       template <class Impl_T, class Param0_T, class Param1_T>
       static void privDirectRedo(Param0_T&& p0, Param1_T&& p1)
       {
           Impl_T(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1)).redo();
       }

       // Default directRedo() implementation overload for 3 args.
       template <class Impl_T, class Param0_T, class Param1_T, class Param2_T>
       static void privDirectRedo(Param0_T&& p0, Param1_T&& p1, Param2_T&& p2)
       {
           Impl_T(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1), std::forward<Param2_T>(p2)).redo();
       }
    }; // class UndoCommand

    DFG_SUB_NS(undoCommands)
    {
        enum InsertRowType { InsertRowTypeAfter, InsertRowTypeBefore };

        class TableViewUndoCommandInsertRow : public UndoCommand
        {
        public:
            typedef TableViewUndoCommandInsertRow ThisClass;
            // If nRow is given, InsertRowType is ignored.
            TableViewUndoCommandInsertRow(QTableView* pTableView, InsertRowType type, int nRow = -1)
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
        }; // class TableViewUndoCommandInsertRow
    }
} }

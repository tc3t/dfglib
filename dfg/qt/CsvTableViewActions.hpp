#pragma once

#include "../dfgDefs.hpp"
#include "CsvTableView.hpp"
#include "CsvItemModel.hpp"
#include "qtIncludeHelpers.hpp"
#include "../OpaquePtr.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QAbstractProxyModel>
#include <QApplication>
#include <QClipboard>
#include <QPointer>
DFG_END_INCLUDE_QT_HEADERS

#include "../io/DelimitedTextReader.hpp"
#include "../alg.hpp"
#include "qtBasic.hpp"
#include "tableViewUndoCommands.hpp"
#include "../math/FormulaParser.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{
    namespace DFG_DETAIL_NS
    {
        using CellMemory = CsvItemModel::RawDataTable;

        inline void removeNewlineCharsFromEnd(QString& s)
        {
            if (s.endsWith("\r\n"))
                s.chop(2);
            else if (s.endsWith('\n') || s.endsWith('\r'))
                s.chop(1);
        }

        // Base class for undo commands that edit selection cell-by-cell.
        // With this base class actual command doesn't need to implement undo/redo functionality.
        class SelectionForEachUndoCommand : public UndoCommand
        {
        public:
            class VisitorParams
            {
            public:
                using CountT = CsvItemModel::LinearIndex;

                QModelIndex index() const   { return m_modelIndex; }
                CsvItemModel& dataModel()   { return m_rDataModel; }
                CsvTableView& view()        { return m_rView;      }
                StringViewUtf8 stringView() { return m_svData; }
                CountT editCount() const    { return m_nEditCount; }

                // Sets string at m_modelIndex to svNewData and increments m_nEditCount if edited.
                // @return True if cell was edited, false otherwise.
                bool setCellString(const StringViewUtf8& svNewData);

                VisitorParams& setIndex(const QModelIndex& index);

                QModelIndex m_modelIndex;
                CsvItemModel& m_rDataModel;
                CsvTableView& m_rView;
                StringViewUtf8 m_svData;
                CountT m_nEditCount = 0;
            };

            // Abstract visitor class that implements the actual cell visit behaviour
            class Visitor
            {
            public:
                virtual ~Visitor() = default;

                // Called for each visited cell
                virtual void handleCell(VisitorParams& params) = 0;

                // Called when all cells have been visited.
                virtual void onForEachLoopDone(VisitorParams&) {}

                // Returns the maximum number of failure messages to store when cell handling fails.
                // This may logically be dependent on the actual visitor, but using static value 10
                // until need arise to make it customisable.
                size_t maxFailureMessageCount() const { return 10; }
            };

            // Constructor.
            //      -sCommandTitleTemplate is used as a format template that gets one argument: number of selected cells
            SelectionForEachUndoCommand(QString sCommandTitleTemplate, CsvTableView* pView);

            void undo();
            void redo();

            // Optimization for undoless action which doesn't need to store old cells to undo buffer.
            template <class Impl_T, class ... Args_T>
            static void privDirectRedo(CsvTableView* pView, Args_T&& ... args)
            {
                auto spGenerator = Impl_T::createVisitorStatic(args...);
                if (!spGenerator)
                    return;
                privDirectRedoImpl(pView, static_cast<const QModelIndexList*>(nullptr), [&](VisitorParams& params)
                    {
                        pView->forEachCsvModelIndexInSelection([&](const QModelIndex& index, bool& /*rbContinue*/)
                            {
                                spGenerator->handleCell(params.setIndex(index));
                            });
                        spGenerator->onForEachLoopDone(params.setIndex(QModelIndex()));
                    });
            }

            static void privDirectRedoImpl(CsvTableView* pView, const QModelIndexList* pSelectList, std::function<void(VisitorParams&)> looper);
            static void privDirectRedoImpl(CsvTableView* pView, const CsvTableView::ItemSelection* pSelectList, std::function<void(VisitorParams&)> looper);

        private:
            virtual std::unique_ptr<Visitor> createVisitor() = 0;

            template <class Selection_T, class Mapper_T>
            static void privDirectRedoImpl(CsvTableView* pView, Selection_T* pSelection, std::function<void(VisitorParams&)>& looper, Mapper_T&& mapper);

        private:
            QPointer<CsvTableView> m_spView;
            CsvTableView::ItemSelection m_initialSelection; // Stores CsvModel indexes to which operation is to be done.
            CellMemory m_cellMemoryUndo;        // Stores cell content before the operation.
            uint32 m_nRedoCounter = 0;          // Counts number of redos, used to avoid selection handling on first redo (i.e. when action is applied for the first time).
        }; // SelectionForEachUndoCommand

    } // namespace DFG_DETAIL_NS

    typedef DFG_SUB_NS_NAME(undoCommands)::TableViewUndoCommandInsertRow CsvTableViewActionInsertRow;

    class CsvTableViewActionInsertColumn : public UndoCommand
    {
    public:
        CsvTableViewActionInsertColumn(CsvItemModel* pModel, const int nCol)
            : m_pModel(pModel),
              m_nWhere(nCol)
        {
            QString sDesc;
            sDesc = QString("Insert column to %1").arg(m_nWhere);
            setText(sDesc);
        }

        void undo()
        {
            m_pModel->removeColumn(m_nWhere);
        }
        void redo()
        {
            m_pModel->insertColumn(m_nWhere);
        }
    private:
        CsvItemModel* m_pModel;
        int m_nWhere;
    };

    namespace DFG_DETAIL_NS
    {
        static void storeCells(CellMemory& cellMemory, CsvTableView& rView)
        {
            auto pModel = rView.csvModel();
            if (!pModel)
                return;
            rView.forEachCsvModelIndexInSelection([&](const QModelIndex& index, bool& bContinue)
            {
                DFG_UNUSED(bContinue);
                SzPtrUtf8R p = pModel->rawStringPtrAt(index);
                cellMemory.setElement(index.row(), index.column(), (p) ? p : SzPtrUtf8(""));
            });
        }

        static void restoreCells(const CellMemory& cellMemory, CsvItemModel& rModel, const SzPtrUtf8R pFill = SzPtrUtf8R(nullptr), std::function<bool()> isCancelledFunc = std::function<bool()>())
        {
            rModel.setDataByBatch_noUndo(cellMemory, pFill, std::move(isCancelledFunc));
        }

    } // namespace DFG_DETAIL_NS

    class CsvTableViewActionDelete : public UndoCommand
    {
    public:
        typedef CsvTableViewActionDelete ThisClass;
        typedef UndoCommand BaseClass;

        // Note: Sorting can change indexes after undo point -> must use model indexes and make sure that
        // they won't be modified outside undo.
        CsvTableViewActionDelete(CsvTableView& rTableView, QAbstractProxyModel* pProxyModel, const bool bRowMode) :
            m_pView(&rTableView),
            m_pCsvModel(rTableView.csvModel()),
            m_pProxyModel(pProxyModel),
            m_bRowMode(bRowMode)
        {
            if (!m_pView || !m_pCsvModel)
                return;
            if (m_bRowMode)
            {
                m_nFirstItemRow = m_pView->getFirstSelectedViewRow();
                m_vecIndexes = m_pView->getRowsOfSelectedItems(m_pProxyModel);
                if (m_vecIndexes.empty())
                    return;

                m_vecData.resize(m_vecIndexes.size());
                std::sort(m_vecIndexes.begin(), m_vecIndexes.end());

                QString sDesc;
                if (m_vecIndexes.size() >= 6)
                {
                    sDesc = QString("Delete rows (%1 rows in total)").arg(m_vecIndexes.size());
                }
                else
                {
                    sDesc = "Delete rows {";
                    for (size_t i = 0; i < m_vecIndexes.size(); ++i)
                    {
                        sDesc.append(" ");
                        sDesc.append(QString("%1").arg(CsvItemModel::internalRowIndexToVisible(m_vecIndexes[i])));
                    }
                    sDesc.append(" }");
                }
                setText(sDesc);

                int nIndex = static_cast<int>(m_vecIndexes.size() - 1);
                for (auto iter = m_vecIndexes.rbegin(); iter != m_vecIndexes.rend(); ++iter, --nIndex)
                {
                    m_vecData[nIndex] = m_pCsvModel->rowToString(*iter);
                }
            }
            else // Cell mode
            {
                if (m_pView)
                {
                    DFG_DETAIL_NS::storeCells(m_cellMemory, *m_pView);
                    setText("Delete some cell contents");
                }
            }
        }

        void undo()
        {
            if (!m_pView || !m_pCsvModel)
                return;
            if (m_bRowMode)
            {
                if (m_vecData.size() != m_vecIndexes.size())
                {
                    DFG_ASSERT(false);
                    return;
                }

                for (size_t i = 0; i < m_vecIndexes.size(); ++i)
                {
                    const int nRow = m_vecIndexes[i];
                    m_pCsvModel->insertRows(nRow, 1);
                    m_pCsvModel->setRow(nRow, m_vecData[i]);
                }
            }
            else // cell mode
            {
                if (m_pCsvModel)
                    DFG_DETAIL_NS::restoreCells(m_cellMemory, *m_pCsvModel);
            }

        }
        void redo()
        {
            if (!m_pView || !m_pCsvModel)
                return;
            if (m_bRowMode)
            {
                m_pCsvModel->removeRows(m_vecIndexes);
                const auto nNewSelectedRow = Min(m_nFirstItemRow, m_pCsvModel->getRowCount() - 1);
                if (m_pView->selectionBehavior() == QAbstractItemView::SelectRows)
                    m_pView->selectRow(nNewSelectedRow);
                else
                {
                    QAbstractItemModel* pViewModel = (m_pProxyModel) ? static_cast<QAbstractItemModel*>(m_pProxyModel) : m_pCsvModel;
                    m_pView->selectionModel()->setCurrentIndex(pViewModel->index(nNewSelectedRow, m_pView->currentIndex().column()), QItemSelectionModel::Select);
                }
                
            }
            else
            {
                if (!m_pCsvModel)
                    return;
                m_pCsvModel->setDataByBatch_noUndo(m_cellMemory, DFG_UTF8(""));
            }
        }

        // Optimization for undoless deletion.
        template <class Impl_T>
        static void privDirectRedo(CsvTableView& rTableView, QAbstractProxyModel* pProxyModel, const bool bRowMode)
        {
            DFG_STATIC_ASSERT((std::is_same<Impl_T, ThisClass>::value), "");

            auto pCsvModel = rTableView.csvModel();
            if (bRowMode || !pCsvModel)
            {
                BaseClass::privDirectRedo<ThisClass>(rTableView, pProxyModel, bRowMode);
                return;
            }

            // Creating a temporary cellMemory just to get the indexes in order to avoid the dataChanged-flood 
            // that would result if changing one by one. Compared to the previous one-by-one implementation, this
            // is a regression at least from memory usage perspective, but for now considering signal flood as 
            // worse option - there obviously would be a more optimal overall solution between the two.
            DFG_DETAIL_NS::CellMemory cellMemory;
            rTableView.forEachCsvModelIndexInSelection([&](const QModelIndex& index, bool& /*bContinue*/)
            {
                cellMemory.setElement(index.row(), index.column(), DFG_UTF8(""));
            });
            pCsvModel->setDataByBatch_noUndo(cellMemory, DFG_UTF8(""));
        }

    private:
        CsvTableView* m_pView;
        CsvItemModel* m_pCsvModel;
        QAbstractProxyModel* m_pProxyModel;
        std::vector<int> m_vecIndexes;
        std::vector<QString> m_vecData;
        DFG_DETAIL_NS::CellMemory m_cellMemory; // Used in cell mode to store cell texts.
        int m_nFirstItemRow = -1;
        bool m_bRowMode;
    }; // class CsvTableViewActionDelete

    // Represents a fill operation: setting constant content to N cells which is considered as one undo/redo-step
    class CsvTableViewActionFill : public UndoCommand
    {
    public:
        CsvTableViewActionFill(CsvTableView* pView, const QString& sFill, const QString& sOperationUiName);

        void undo();
        void redo();

    private:
        QPointer<CsvTableView> m_spView;
        StringUtf8 m_sFill;
        QString m_sOperationUiName;
        size_t m_nCellCount = 0;
        DFG_DETAIL_NS::CellMemory m_cellMemoryUndo;
        DFG_DETAIL_NS::CellMemory m_cellMemoryRedo; // Note: effectively stores just indexes.
    }; // class CsvTableViewActionFill

    class CsvTableViewActionPaste : public UndoCommand
    {
    public:
        CsvTableViewActionPaste(CsvTableView* pView)
            : m_pView(pView),
              m_nWhere(0)
        {
            m_bRowMode = (m_pView) ? m_pView->isRowMode() : false;

            if (!m_pView)
                return;

            auto pModel = m_pView->csvModel();

            if (m_bRowMode)
            {
                // If sorting is not enabled, put paste items on current position, otherwise to the end of the table.
                m_nWhere = m_pView->getFirstSelectedViewRow();
                if (m_pView->isSortingEnabled())
                    m_nWhere = pModel->rowCount();
                else
                {
                    auto pProxy = m_pView->getProxyModelPtr();
                    DFG_UNUSED(pProxy); // Silences warning in release build.
                    DFG_ASSERT(!pProxy || m_nWhere == m_pView->getDataModelRowsOfSelectedItems().front());
                }
                QString sText = QApplication::clipboard()->text();

                // Read the clipboard string to vector of entries.
                QTextStream strm(&sText, QIODevice::ReadOnly);
                do
                {
                    QString sLine = strm.readLine();
                    if (sLine.isEmpty())
                        continue;
                    m_vecLines.push_back(sLine);
                } while (!strm.atEnd());

                QString sDesc;
                sDesc = QString("Paste %1 rows to row %2").arg(m_vecLines.size()).arg(m_nWhere);
                setText(sDesc);
            }
            else // case: cell mode.
            {
                const QModelIndexList& indexList = m_pView->getSelectedItemIndexes_dataModel();
                if (!indexList.empty())
                    m_where = indexList[0];
                else
                    m_where = pModel->index(0, 0);

                QString sText = QApplication::clipboard()->text();

                auto viewModelIndex = m_pView->mapToViewModel(m_where);
                auto pViewModel = viewModelIndex.model();
                if (!pViewModel)
                {
                    setText(QString("Invalid paste to (%1, %2)").arg(m_where.row()).arg(m_where.column()));
                    return;
                }

                QTextStream strm(&sText, QIODevice::ReadOnly);
                DFG_MODULE_NS(io)::DelimitedTextReader::read<wchar_t>(strm, '\t', '"', '\n', [&](const size_t nRow, const size_t nCol, const wchar_t* const p, const size_t nSize)
                {
                    const auto indexTarget = m_pView->mapToDataModel(pViewModel->index(viewModelIndex.row() + static_cast<int>(nRow),
                                                                                                       viewModelIndex.column() + static_cast<int>(nCol)));
                    if (!indexTarget.isValid())
                        return;
                    const auto nTargetRow = indexTarget.row();
                    const auto nTargetCol = indexTarget.column();
                    m_cellMemoryUndo.setElement(nTargetRow, nTargetCol, pModel->rawStringPtrAt(nTargetRow, nTargetCol));
                    const int intSize = (nSize < NumericTraits<int>::maxValue) ? static_cast<int>(nSize) : NumericTraits<int>::maxValue;
                    m_cellMemoryRedo.setElement(nTargetRow, nTargetCol, SzPtrUtf8R(QString::fromWCharArray(p, intSize).toUtf8().data())); // TODO: Should not need to create a temporary QString for wchar_t -> UTF8 transform.
                });

                QString sDesc;
                sDesc = QString("Paste cells to (%1, %2)").arg(m_where.row()).arg(m_where.column());
                setText(sDesc);
            }
        }

        void undo()
        {
            if (!m_pView)
                return;
            auto pModel = m_pView->csvModel();
            if (m_bRowMode)
                pModel->removeRows(m_nWhere, static_cast<int>(m_vecLines.size()));
            else if (pModel) // case: cell mode
                DFG_DETAIL_NS::restoreCells(m_cellMemoryUndo, *pModel);
        }
        void redo()
        {
            if (!m_pView)
                return;
            auto pModel = m_pView->csvModel();
            if (!pModel)
                return;
            if (m_bRowMode)
            {
                std::vector<int> vecPasteModelIndexes;
                m_pView->clearSelection();
                int nPasteRow = m_nWhere;
                for (auto iter = m_vecLines.begin(); iter != m_vecLines.end(); ++iter, ++nPasteRow)
                {
                    pModel->insertRow(nPasteRow);
                    pModel->setRow(nPasteRow, *iter);
                }

                QItemSelection selection;
                auto pProxy = m_pView->getProxyModelPtr();
                if (!pProxy)
                    return;
                for (int i = 0, nCount = static_cast<int>(m_vecLines.size()); i<nCount; ++i)
                {
                    selection.select(pProxy->mapFromSource(pModel->index(m_nWhere + i, 0)),
                        pProxy->mapFromSource(pModel->index(m_nWhere + i, 0))
                        );
                }

                m_pView->selectionModel()->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Rows);
            }
            else
            {
                DFG_DETAIL_NS::restoreCells(m_cellMemoryRedo, *pModel);
            }
        }
    private:
        CsvTableView* m_pView;
        int m_nWhere;	// Stores the index of the new row.
        QModelIndex m_where; /// Cell mode: Stores the top-left dataModel index where the paste should take place.
        DFG_DETAIL_NS::CellMemory m_cellMemoryUndo;
        DFG_DETAIL_NS::CellMemory m_cellMemoryRedo;
        std::vector<QString> m_vecLines;
        bool m_bRowMode;
    }; // class CsvTableViewActionPaste

    class CsvTableViewActionDeleteColumn : public UndoCommand
    {
    public:
        CsvTableViewActionDeleteColumn(CsvTableView* pView, const int nCol)
            : m_pView(pView),
                m_nWhere(nCol)
        {
            auto pModel = (m_pView) ? m_pView->csvModel() : nullptr;
            if (!pModel)
                return;
            m_sColumnName = pModel->headerData(nCol, Qt::Horizontal).toString();
            QString sDesc;
            sDesc = QString("Delete column %1, \"%2\"").arg(m_nWhere).arg(m_sColumnName);
            setText(sDesc);
            pModel->columnToStrings(nCol, m_vecStrings);
        }

        void undo()
        {
            auto pModel = (m_pView) ? m_pView->csvModel() : nullptr;
            if (!pModel)
                return;
            pModel->insertColumn(m_nWhere);
            pModel->setColumnCells(m_nWhere, m_vecStrings);
            pModel->setColumnName(m_nWhere, m_sColumnName);

        }
        void redo()
        {
            auto pModel = (m_pView) ? m_pView->model() : nullptr;
            if (!pModel)
                return;
            pModel->removeColumn(m_nWhere);
        }
    private:
        CsvTableView* m_pView;
        int m_nWhere;
        std::vector<QString> m_vecStrings;
        QString m_sColumnName;
    };

    class CsvTableViewHeaderActionPasteColumn : public UndoCommand
    {
    public:
        CsvTableViewHeaderActionPasteColumn(CsvTableView* pView, const int nCol)
            : m_pView(pView), m_nWhere(nCol)
        {

            auto pModel = (m_pView) ? m_pView->csvModel() : nullptr;
            if (!pModel)
                return;

            QString str = QApplication::clipboard()->text();

            // TODO: use CSV-reading (current implementation fails on embedded new lines)
            QTextStream strm(&str, QIODevice::ReadOnly);
            QString sLine;
            while (!strm.atEnd())
            {
                sLine = strm.readLine();
                m_vecStringsNew.push_back(sLine);
            }

            QString sDesc;
            sDesc = QString("Paste column to %1, \"%2\"").arg(m_nWhere).arg(pModel->getHeaderName(nCol));
            setText(sDesc);
            pModel->columnToStrings(nCol, m_vecStringsOld);
        }

        void undo()
        {
            auto pModel = (m_pView) ? m_pView->csvModel() : nullptr;
            if (!pModel)
                return;

            if (m_vecStringsNew.size() > m_vecStringsOld.size())
            {
                const auto nDiff = static_cast<int>(m_vecStringsNew.size() - m_vecStringsOld.size());
                pModel->removeRows(pModel->getRowCount() - nDiff, nDiff);
            }
            pModel->setColumnCells(m_nWhere, m_vecStringsOld);
        }
        void redo()
        {
            auto pModel = (m_pView) ? m_pView->csvModel() : nullptr;
            if (!pModel)
                return;
            const auto nVecStringsNewSize = static_cast<int>(m_vecStringsNew.size());
            if (pModel->getRowCount() < nVecStringsNewSize)
                pModel->insertRows(pModel->getRowCount(), nVecStringsNewSize - pModel->getRowCount());
            pModel->setColumnCells(m_nWhere, m_vecStringsNew);

            // Select the whole column
            m_pView->clearSelection();
            auto pViewModel = m_pView->model();
            if (!pViewModel)
                return;
            QItemSelection selection(pViewModel->index(0, m_nWhere), pViewModel->index(pViewModel->rowCount() - 1, m_nWhere));
            m_pView->selectionModel()->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Columns);
        }
    private:
        CsvTableView* m_pView;
        int m_nWhere;
        std::vector<QString> m_vecStringsOld;
        std::vector<QString> m_vecStringsNew;
    };

    class CsvTableViewHeaderActionRenameColumn : public UndoCommand
    {
    public:
        CsvTableViewHeaderActionRenameColumn(CsvTableView* pView, const int nCol, const QString& sNew)
            : m_pView(pView), m_nWhere(nCol), m_sColumnNameNew(sNew)
        {
            auto pModel = (m_pView) ? m_pView->csvModel() : nullptr;
            if (!pModel)
                return;
 
            m_sColumnNameOld = pModel->getHeaderName(nCol);
            QString sDesc;
            sDesc = QString("Rename column %1, \"%2\" to \"%3\"").arg(m_nWhere).arg(m_sColumnNameOld, m_sColumnNameNew);
            setText(sDesc);
        }

        void undo()
        {
            auto pModel = (m_pView) ? m_pView->csvModel() : nullptr;
            if (!pModel)
                return;
            pModel->setColumnName(m_nWhere, m_sColumnNameOld);

        }
        void redo()
        {
            auto pModel = (m_pView) ? m_pView->csvModel() : nullptr;
            if (!pModel)
                return;
            pModel->setColumnName(m_nWhere, m_sColumnNameNew);
        }
    private:
        CsvTableView* m_pView;
        int m_nWhere;
        QString m_sColumnNameOld;
        QString m_sColumnNameNew;

    };

    class CsvTableViewActionMoveFirstRowToHeader : public UndoCommand
    {
    public:
        CsvTableViewActionMoveFirstRowToHeader(CsvTableView* pView, bool bInverted = false)
            : m_pView(pView),
            m_bInverted(bInverted)
        {
            auto pModel = (m_pView) ? m_pView->model() : nullptr;
            if (!pModel || (!m_bInverted && pModel->rowCount() < 1))
                return;

            const auto nColCount = pModel->columnCount();
            for (int i = 0; i < nColCount; ++i)
            {
                m_vecHdrStrings.push_back(pModel->headerData(i, Qt::Horizontal).toString());
                m_vecFirstRowStrings.push_back(pModel->data(pModel->index(0, i)).toString());
            }
            QString sDesc = (m_bInverted) ? m_pView->tr("Move header to first row") : m_pView->tr("Move first row to header");
            setText(sDesc);
        }

        void undoImpl()
        {
            const auto pModel = (m_pView) ? m_pView->csvModel() : nullptr;
            if (!pModel)
                return;

            const auto& rFirstRowStrings = (m_bInverted) ? m_vecHdrStrings : m_vecFirstRowStrings;

            if (rFirstRowStrings.empty())
                return;

            pModel->insertRow(0);

            DFG_MODULE_NS(alg)::forEachFwdWithIndexT<int>(rFirstRowStrings, [&](const QString& s, const int i)
            {
                pModel->setDataNoUndo(pModel->index(0, i), s);
            });
            DFG_MODULE_NS(alg)::forEachFwdWithIndexT<int>(m_vecHdrStrings, [&](const QString& s, const int i)
            {
                if (m_bInverted)
                    pModel->setColumnName(i, QString());
                else
                    pModel->setColumnName(i, s);
            });
        }

        void redoImpl()
        {
            const auto pModel = (m_pView) ? m_pView->csvModel() : nullptr;
            if (!pModel)
                return;
            //CsvTableViewActionDelete(*m_pView, m_pView->getProxyModelPtr(), true, ).redo();

            const auto& headerNameStrings = (m_bInverted) ? m_vecHdrStrings : m_vecFirstRowStrings;
            if (headerNameStrings.empty())
                return;

            pModel->removeRow(0);
            
            DFG_MODULE_NS(alg)::forEachFwdWithIndexT<int>(headerNameStrings, [&](const QString& s, const int i)
            {
                pModel->setColumnName(i, s);
            });
        }

        void undo()
        {
            if (m_bInverted)
            {
                redoImpl();
                return;
            }
            else
                undoImpl();
        }

        void redo()
        {
            if (m_bInverted)
            {
                undoImpl();
                return;
            }
            else
                redoImpl();
        }
    private:
        CsvTableView* m_pView;
        bool m_bInverted; // If m_bInverted == true, behaviour is "Header to first row".
        std::vector<QString> m_vecHdrStrings;
        std::vector<QString> m_vecFirstRowStrings;
    }; // class CsvTableViewActionMoveFirstRowToHeader

    class CsvTableViewActionEvaluateSelectionAsFormula : public DFG_DETAIL_NS::SelectionForEachUndoCommand
    {
    public:
        using BaseClass = DFG_DETAIL_NS::SelectionForEachUndoCommand;

        CsvTableViewActionEvaluateSelectionAsFormula(CsvTableView* pView);

        class FormulaVisitor : public Visitor
        {
        public:
            void handleCell(VisitorParams& params) override;
            void onForEachLoopDone(VisitorParams& params) override;

            DFG_OPAQUE_PTR_DECLARE();
        };

        static std::unique_ptr<FormulaVisitor> createVisitorStatic();
        std::unique_ptr<Visitor> createVisitor() override;
    }; // class CsvTableViewActionEvaluateSelectionAsFormula

    class CsvTableViewActionResizeTable : public UndoCommand
    {
    public:
        CsvTableViewActionResizeTable(CsvTableView* pView, int nNewRowCount, int nNewColCount);

        void impl(const int nTargetRowCount, const int nTargetColCount);

        void undo();

        void redo();

    private:
        QPointer<CsvTableView> m_spView;
        int m_nOldRowCount = -1;
        int m_nOldColCount = -1;
        int m_nNewRowCount = -1;
        int m_nNewColCount = -1;
        DFG_DETAIL_NS::CellMemory m_cellMemory; // When shrinking table, stores content from the removed area so that undo can restored them.

        DFG_OPAQUE_PTR_DECLARE();
    }; // class CsvTableViewActionResizeTable

    class CsvTableViewActionChangeRadixParams
    {
    public:
        enum class ParamId
        {
            fromRadix,
            toRadix,
            ignorePrefix,
            ignoreSuffix,
            resultPrefix,
            resultSuffix,
            resultDigits
        };

        CsvTableViewActionChangeRadixParams(const QVariantMap& params = QVariantMap());
        bool isValid() const;
        static QString paramStringId(ParamId);
        bool hasResultAdjustments() const;

        int fromRadix = 10;
        int toRadix = 0;
        StringUtf8 ignorePrefix;
        StringUtf8 ignoreSuffix;
        StringUtf8 resultPrefix;
        StringUtf8 resultSuffix;
        QString resultDigits;
    };

    class CsvTableViewActionChangeRadix : public DFG_DETAIL_NS::SelectionForEachUndoCommand
    {
    public:
        using BaseClass = DFG_DETAIL_NS::SelectionForEachUndoCommand;
        using Params = CsvTableViewActionChangeRadixParams;

        CsvTableViewActionChangeRadix(CsvTableView* pView, Params params);

        class RadixChangeVisitor : public Visitor
        {
        public:
            RadixChangeVisitor(Params params);
            void handleCell(VisitorParams& params) override;
            void onForEachLoopDone(VisitorParams& params) override;

            DFG_OPAQUE_PTR_DECLARE();
        };

        static std::unique_ptr<RadixChangeVisitor> createVisitorStatic(Params params);

        std::unique_ptr<Visitor> createVisitor() override;

    private:
        DFG_OPAQUE_PTR_DECLARE();
    }; // class CsvTableViewActionChangeRadix


    class CsvTableViewActionTrimCells : public DFG_DETAIL_NS::SelectionForEachUndoCommand
    {
    public:
        using BaseClass = DFG_DETAIL_NS::SelectionForEachUndoCommand;

        class TrimCellVisitor : public Visitor
        {
        public:
            void handleCell(VisitorParams& params) override;
            void onForEachLoopDone(VisitorParams& params) override;
        }; // class TrimCellVisitor

        CsvTableViewActionTrimCells(CsvTableView* pView);

        static std::unique_ptr<TrimCellVisitor> createVisitorStatic();

        std::unique_ptr<Visitor> createVisitor() override;
    }; // CsvTableViewActionTrimCells
}}

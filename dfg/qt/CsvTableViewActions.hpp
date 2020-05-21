#pragma once

#include "../dfgDefs.hpp"
#include "CsvTableView.hpp"
#include "CsvItemModel.hpp"
#include "qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QAbstractProxyModel>
#include <QApplication>
#include <QClipboard>
DFG_END_INCLUDE_QT_HEADERS

#include <unordered_map>

#include "../io/DelimitedTextReader.hpp"
#include "../alg.hpp"
#include "qtBasic.hpp"
#include "tableViewUndoCommands.hpp"
#include "../cont/table.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{
    namespace DFG_DETAIL_NS
    {
        inline void removeNewlineCharsFromEnd(QString& s)
        {
            if (s.endsWith("\r\n"))
                s.chop(2);
            else if (s.endsWith('\n') || s.endsWith('\r'))
                s.chop(1);
        }

    };


    typedef DFG_SUB_NS_NAME(undoCommands)::DFG_CLASS_NAME(TableViewUndoCommandInsertRow) DFG_CLASS_NAME(CsvTableViewActionInsertRow);
    typedef DFG_SUB_NS_NAME(undoCommands)::DFG_CLASS_NAME(CsvTableViewActionResizeTable) DFG_CLASS_NAME(CsvTableViewActionResizeTable);

    class DFG_CLASS_NAME(CsvTableViewActionInsertColumn) : public DFG_CLASS_NAME(UndoCommand)
    {
    public:
        DFG_CLASS_NAME(CsvTableViewActionInsertColumn)(DFG_CLASS_NAME(CsvItemModel)* pModel, const int nCol)
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
        DFG_CLASS_NAME(CsvItemModel)* m_pModel;
        int m_nWhere;
    };

    namespace DFG_DETAIL_NS
    {
        typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(TableSz)<char, int, DFG_MODULE_NS(io)::encodingUTF8> CellMemory;

        static void storeCells(CellMemory& cellMemory, DFG_CLASS_NAME(CsvTableView)& rView)
        {
            auto pModel = rView.csvModel();
            if (!pModel)
                return;
            rView.forEachCsvModelIndexInSelection([&](const QModelIndex& index, bool& bContinue)
            {
                DFG_UNUSED(bContinue);
                SzPtrUtf8R p = pModel->RawStringPtrAt(index.row(), index.column());
                cellMemory.setElement(index.row(), index.column(), (p) ? p : SzPtrUtf8(""));
            });
        }

        static void restoreCells(const CellMemory& cellMemory, DFG_CLASS_NAME(CsvItemModel)& rModel)
        {
            rModel.setDataByBatch_noUndo(cellMemory);
        }

    } // namespace DFG_DETAIL_NS

    class DFG_CLASS_NAME(CsvTableViewActionDelete) : public DFG_CLASS_NAME(UndoCommand)
    {
    public:
        typedef DFG_CLASS_NAME(CsvTableViewActionDelete) ThisClass;
        typedef DFG_CLASS_NAME(UndoCommand) BaseClass;

        // Note: Sorting can change indexes after undo point -> must use model indexes and make sure that
        // they won't be modified outside undo.
        DFG_CLASS_NAME(CsvTableViewActionDelete)(DFG_CLASS_NAME(CsvTableView)& rTableView, QAbstractProxyModel* pProxyModel, const bool bRowMode) :
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
                        sDesc.append(QString("%1").arg(DFG_CLASS_NAME(CsvItemModel)::internalRowIndexToVisible(m_vecIndexes[i])));
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
                if (m_pView)
                    m_pView->onSelectionContentChanged();
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
                if (m_pView)
                    m_pView->onSelectionContentChanged();
            }
        }

        // Optimization for undoless deletion.
        template <class Impl_T>
        static void privDirectRedo(DFG_CLASS_NAME(CsvTableView)& rTableView, QAbstractProxyModel* pProxyModel, const bool bRowMode)
        {
            DFG_STATIC_ASSERT((std::is_same<Impl_T, ThisClass>::value), "");

            auto pCsvModel = rTableView.csvModel();
            if (bRowMode || !pCsvModel)
            {
                BaseClass::privDirectRedo<ThisClass>(rTableView, pProxyModel, bRowMode);
                return;
            }

            rTableView.forEachCsvModelIndexInSelection([=](const QModelIndex& index, bool& /*bContinue*/)
            {
                pCsvModel->setDataNoUndo(index.row(), index.column(), DFG_UTF8(""));
            });
            rTableView.onSelectionContentChanged();
        }

    private:
        DFG_CLASS_NAME(CsvTableView)* m_pView;
        DFG_CLASS_NAME(CsvItemModel)* m_pCsvModel;
        QAbstractProxyModel* m_pProxyModel;
        std::vector<int> m_vecIndexes;
        std::vector<QString> m_vecData;
        DFG_DETAIL_NS::CellMemory m_cellMemory; // Used in cell mode to store cell texts.
        int m_nFirstItemRow;
        bool m_bRowMode;
    };

    class DFG_CLASS_NAME(CsvTableViewActionPaste) : public DFG_CLASS_NAME(UndoCommand)
    {
    public:
        DFG_CLASS_NAME(CsvTableViewActionPaste)(DFG_CLASS_NAME(CsvTableView)* pView)
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
                DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::read<wchar_t>(strm, '\t', '"', '\n', [&](const size_t nRow, const size_t nCol, const wchar_t* const p, const size_t nSize)
                {
                    const auto indexTarget = m_pView->mapToDataModel(pViewModel->index(viewModelIndex.row() + static_cast<int>(nRow),
                                                                                                       viewModelIndex.column() + static_cast<int>(nCol)));
                    if (!indexTarget.isValid())
                        return;
                    const auto nTargetRow = indexTarget.row();
                    const auto nTargetCol = indexTarget.column();
                    m_cellMemoryUndo.setElement(nTargetRow, nTargetCol, pModel->RawStringPtrAt(nTargetRow, nTargetCol));
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
        DFG_CLASS_NAME(CsvTableView)* m_pView;
        int m_nWhere;	// Stores the index of the new row.
        QModelIndex m_where; /// Cell mode: Stores the top-left dataModel index where the paste should take place.
        DFG_DETAIL_NS::CellMemory m_cellMemoryUndo;
        DFG_DETAIL_NS::CellMemory m_cellMemoryRedo;
        std::vector<QString> m_vecLines;
        bool m_bRowMode;
    };

    class DFG_CLASS_NAME(CsvTableViewActionDeleteColumn) : public DFG_CLASS_NAME(UndoCommand)
    {
    public:
        DFG_CLASS_NAME(CsvTableViewActionDeleteColumn)(DFG_CLASS_NAME(CsvTableView)* pView, const int nCol)
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
        DFG_CLASS_NAME(CsvTableView)* m_pView;
        int m_nWhere;
        std::vector<QString> m_vecStrings;
        QString m_sColumnName;
    };

    class DFG_CLASS_NAME(CsvTableViewHeaderActionPasteColumn) : public DFG_CLASS_NAME(UndoCommand)
    {
    public:
        DFG_CLASS_NAME(CsvTableViewHeaderActionPasteColumn)(DFG_CLASS_NAME(CsvTableView)* pView, const int nCol)
            : m_pView(pView), m_nWhere(nCol)
        {

            auto pModel = (m_pView) ? m_pView->csvModel() : nullptr;
            if (!pModel)
                return;

            QString str = QApplication::clipboard()->text();

            // TODO: use CSV-reading (current implementation fails on embedded new lines)
            QTextStream strm(&str, QIODevice::ReadOnly);
            QString sLine;
            for (int nRow = 0; !strm.atEnd(); ++nRow)
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
        DFG_CLASS_NAME(CsvTableView)* m_pView;
        int m_nWhere;
        std::vector<QString> m_vecStringsOld;
        std::vector<QString> m_vecStringsNew;
    };

    class DFG_CLASS_NAME(CsvTableViewHeaderActionRenameColumn) : public DFG_CLASS_NAME(UndoCommand)
    {
    public:
        DFG_CLASS_NAME(CsvTableViewHeaderActionRenameColumn)(DFG_CLASS_NAME(CsvTableView)* pView, const int nCol, const QString& sNew)
            : m_pView(pView), m_nWhere(nCol), m_sColumnNameNew(sNew)
        {
            auto pModel = (m_pView) ? m_pView->csvModel() : nullptr;
            if (!pModel)
                return;
 
            m_sColumnNameOld = pModel->getHeaderName(nCol);
            QString sDesc;
            sDesc = QString("Rename column %1, \"%2\" to \"%3\"").arg(m_nWhere).arg(m_sColumnNameOld).arg(m_sColumnNameNew);
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
        DFG_CLASS_NAME(CsvTableView)* m_pView;
        int m_nWhere;
        QString m_sColumnNameOld;
        QString m_sColumnNameNew;

    };

    class DFG_CLASS_NAME(CsvTableViewActionMoveFirstRowToHeader) : public DFG_CLASS_NAME(UndoCommand)
    {
    public:
        DFG_CLASS_NAME(CsvTableViewActionMoveFirstRowToHeader)(DFG_CLASS_NAME(CsvTableView)* pView, bool bInverted = false)
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
            //DFG_CLASS_NAME(CsvTableViewActionDelete)(*m_pView, m_pView->getProxyModelPtr(), true, ).redo();

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
        DFG_CLASS_NAME(CsvTableView)* m_pView;
        bool m_bInverted; // If m_bInverted == true, behaviour is "Header to first row".
        std::vector<QString> m_vecHdrStrings;
        std::vector<QString> m_vecFirstRowStrings;
    };
}}

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QObject>
#include <QDateTime>
#include <QPointer>
#include <QVariant>
DFG_END_INCLUDE_QT_HEADERS

#include "graphTools.hpp"
#include "connectHelper.hpp"
#include "CsvTableView.hpp"
#include "CsvItemModel.hpp"
#include "../cont/MapVector.hpp"
#include "../cont/ViewableSharedPtr.hpp"
#include "../alg.hpp"
#include <memory>
#include <limits>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt) {

namespace DFG_DETAIL_NS
{
    double dateToDouble(QDateTime&& dt)
    {
        if (dt.isValid())
        {
            dt.setTimeSpec(Qt::UTC);
            return static_cast<double>(dt.toMSecsSinceEpoch()) / 1000.0;
        }
        else
            return std::numeric_limits<double>::quiet_NaN();
    }

    double timeToDouble(const QTime& t)
    {
        return (t.isValid()) ? static_cast<double>(t.msecsSinceStartOfDay()) / 1000.0 : std::numeric_limits<double>::quiet_NaN();
    }

    double stringToDouble(const QString& s)
    {
        bool b;
        const auto v = s.toDouble(&b);
        return (b) ? v : std::numeric_limits<double>::quiet_NaN();
    }

    // Return value in case of invalid input as GIGO, in most cases returns NaN. Also in case of invalid input typeMap's value at nCol is unspecified.
    double cellStringToDouble(const QString& s, const int nCol, GraphDataSource::ColumnDataTypeMap& typeMap)
    {
        const auto setColumnDataType = [&](const ChartDataType t)
            {
                typeMap[nCol] = t;
            };
        // TODO: add parsing for timezone specifier (e.g. 2020-04-25 12:00:00.123+0300)
        // TODO: add parsing for fractional part longer than 3 digits.
        if (s.size() >= 8 && s[4] == '-' && s[7] == '-') // Something starting with ????-??-?? (ISO 8601, https://en.wikipedia.org/wiki/ISO_8601)
        {
            if (s.size() >= 19 && s[13] == ':' && s[16] == ':' && ::DFG_MODULE_NS(alg)::contains("T ", s[10].toLatin1())) // Case ????-??-??[T ]hh:mm:ss[.zzz]
            {
                if (s.size() >= 23 && s[19] == '.')
                {
                    setColumnDataType(ChartDataType::dateAndTimeMillisecond);
                    return dateToDouble(QDateTime::fromString(s, QString("yyyy-MM-dd%1hh:mm:ss.zzz").arg(s[10])));
                }
                else
                {
                    setColumnDataType(ChartDataType::dateAndTime);
                    return dateToDouble(QDateTime::fromString(s, QString("yyyy-MM-dd%1hh:mm:ss").arg(s[10])));
                }
            }
            else if (s.size() == 13 && s[10] == ' ') // Case: "yyyy-MM-dd Wd". where Wd is two char weekday indicator.
            {
                setColumnDataType(ChartDataType::dateOnly);
                return dateToDouble(QDateTime::fromString(s, QString("yyyy-MM-dd'%1'").arg(s.mid(10, 3))));
            }
            else if (s.size() == 10) // Case: "yyyy-MM-dd"
            {
                setColumnDataType(ChartDataType::dateOnly);
                return dateToDouble(QDateTime::fromString(s, "yyyy-MM-dd"));
            }
            else
            {
                return std::numeric_limits<double>::quiet_NaN();
            }
        }
        // [d]d.[m]m.yyyy
        QRegExp regExp(R"((?:^|^\w\w )(\d{1,2})(?:\.)(\d{1,2})(?:\.)(\d\d\d\d)$)");
        if (regExp.exactMatch(s) && regExp.captureCount() == 3)
        {
            setColumnDataType(ChartDataType::dateOnly);
            const auto items = regExp.capturedTexts();
            // 0 has entire match, so actual captures start from index 1.
            return dateToDouble(QDateTime(QDate(regExp.cap(3).toInt(), regExp.cap(2).toInt(), regExp.cap(1).toInt())));
        }

        if (s.size() >= 8 && s[2] == ':' && s[5] == ':')
        {
            if (s.size() >= 10 && s[8] == '.')
            {
                setColumnDataType(ChartDataType::dayTimeMillisecond);
                return timeToDouble(QTime::fromString(s, "hh:mm:ss.zzz"));
            }
            else
            {
                setColumnDataType(ChartDataType::dayTime);
                return timeToDouble(QTime::fromString(s, "hh:mm:ss"));
            }
        }
        if (std::count(s.begin(), s.end(), '-') >= 2 || std::count(s.begin(), s.end(), '.') >= 2 || std::count(s.begin(), s.end(), ':') >= 2)
            return std::numeric_limits<double>::quiet_NaN();
        if (s.indexOf(',') < 0)
            return stringToDouble(s);
        else
        {
            auto s2 = s;
            s2.replace(',', '.'); // Hack: to make comma-localized values such as "1,2" be interpreted as 1.2
            return stringToDouble(s2);
        }
    }
} // namespace DFG_DETAIL_NS


// Selection analyzer that gathers data from selection for graphs.
class SelectionAnalyzerForGraphing : public CsvTableViewSelectionAnalyzer
{
public:
    typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(MapVectorSoA)<double, double> RowToValueMap;
    typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(MapVectorSoA)<int, RowToValueMap> ColumnToValuesMap;
    class Table : public ColumnToValuesMap
    {
    public:
        GraphDataSource::ColumnDataTypeMap m_columnTypes;
        GraphDataSource::ColumnNameMap m_columnNames;
    };

    inline void analyzeImpl(const QItemSelection selection) override;

    ::DFG_MODULE_NS(cont)::ViewableSharedPtr<Table> m_spTable;
};

void SelectionAnalyzerForGraphing::analyzeImpl(const QItemSelection selection)
{
    auto pView = qobject_cast<CsvTableView*>(this->m_spView.data());
    if (!pView || !m_spTable)
        return;

    m_spTable.edit([&](Table& rTable, const Table* pOld)
    {
        // TODO: this may be storing way too much data: e.g. if all items are selected, graphing may not use any or perhaps only two columns.
        //       -> filter according to actual needs. Note that in order to be able to do so, would need to know something about graph definitions here.
        // TODO: which indexes to use e.g. in case of filtered table: source row indexes or view rows?
        //      -e.g. if table of 1000 rows is filtered so that only rows 100, 600 and 700 are shown, should single column graph use
        //       values 1, 2, 3 or 100, 600, 700 as x-coordinate value? This might be something to define in graph definition -> another reason why
        //       graph definitions should be somehow known at this point.

        DFG_UNUSED(pOld);

        ::DFG_MODULE_NS(cont)::SetVector<int> presentColumnIndexes;
        presentColumnIndexes.setSorting(true); // Sorting is needed for std::set_difference

        // Clearing all values from every column so that those would actually contain current values, not remnants from previous update.
        // Note that removing unused columns is done after going through the selection.
        ::DFG_MODULE_NS(alg)::forEachFwd(rTable.valueRange(), [](RowToValueMap& columnValues) { columnValues.clear(); });

        // Sorting is needed for std::set_difference
        rTable.setSorting(true);

        CompletionStatus completionStatus = CompletionStatus_started;
        rTable.m_columnTypes.clear();
        rTable.m_columnNames.clear();

        // Getting column names
        {
            auto pModel = pView->csvModel();
            if (pModel)
            {
                const auto nCount = pModel->columnCount();
                for (int i = 0; i < nCount; ++i)
                    rTable.m_columnNames[i] = pModel->getHeaderName(i);
            }
        }

        CsvTableView::forEachIndexInSelection(*pView, selection, CsvTableView::ModelIndexTypeView, [&](const QModelIndex& mi, bool& rbContinue)
        {
            if (m_abNewSelectionPending)
            {
                completionStatus = ::DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)::CompletionStatus_terminatedByNewSelection;
                rbContinue = false;
                return;
            }
            else if (!m_abIsEnabled.load(std::memory_order_relaxed))
            {
                completionStatus = ::DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)::CompletionStatus_terminatedByDisabling;
                rbContinue = false;
                return;
            }

            if (!mi.isValid())
                return;
            auto sVal = mi.data().toString();
            presentColumnIndexes.insert(mi.column());
            // Note that indexes are view indexes, not source model indexes (e.g. in case of filtered table, row indexes in filtered table)
            const auto nCol = mi.column();
            rTable[mi.column()][CsvItemModel::internalRowIndexToVisible(mi.row())] = DFG_DETAIL_NS::cellStringToDouble(sVal, nCol, rTable.m_columnTypes);
        });

        // Removing unused columns from rTable.
        std::vector<int> colsToRemove;
        const auto colsInTable = rTable.keyRange();
        std::set_difference(colsInTable.cbegin(), colsInTable.cend(), presentColumnIndexes.cbegin(), presentColumnIndexes.cend(), std::back_inserter(colsToRemove));
        for (auto iter = colsToRemove.cbegin(), iterEnd = colsToRemove.cend(); iter != iterEnd; ++iter)
        {
            rTable.erase(*iter);
        }
    });

    Q_EMIT sigAnalyzeCompleted();
}

// TODO: consider moving to dfg/qt instead of being here in dfgQtTableEditor main.cpp
class CsvTableViewChartDataSource : public GraphDataSource
{
public:
    using BaseClass = GraphDataSource;

    CsvTableViewChartDataSource(CsvTableView* view);

    QObject* underlyingSource() override;

    void forEachElement_fromTableSelection(std::function<void(DataSourceIndex, DataSourceIndex, QVariant)> handler) override;

    DataSourceIndex columnCount() const override;

    DataSourceIndex columnIndexByName(const StringViewUtf8 sv) const override;

    SingleColumnDoubleValuesOptional singleColumnDoubleValues_byOffsetFromFirst(const DataSourceIndex offsetFromFirst) override;

    SingleColumnDoubleValuesOptional singleColumnDoubleValues_byColumnIndex(const DataSourceIndex nColIndex) override;

    void enable(const bool b) override;

    ColumnDataTypeMap columnDataTypes() const override;

    ColumnNameMap columnNames() const override;

    std::shared_ptr<const SelectionAnalyzerForGraphing::Table> privGetTableView() const;

    QPointer<CsvTableView> m_spView;
    std::shared_ptr<::DFG_MODULE_NS(cont)::ViewableSharedPtrViewer<SelectionAnalyzerForGraphing::Table>> m_spDataViewer;
    std::shared_ptr<SelectionAnalyzerForGraphing> m_spSelectionAnalyzer;
};

CsvTableViewChartDataSource::CsvTableViewChartDataSource(CsvTableView* view)
    : m_spView(view)
{
    if (!m_spView)
        return;
    m_uniqueId = QString("viewSelection_%1").arg(QString::number(QDateTime::currentMSecsSinceEpoch() % 1000000));
    m_spSelectionAnalyzer = std::make_shared<SelectionAnalyzerForGraphing>();
    m_spSelectionAnalyzer->m_spTable.reset(std::make_shared<SelectionAnalyzerForGraphing::Table>());
    m_spDataViewer = m_spSelectionAnalyzer->m_spTable.createViewer();
    DFG_QT_VERIFY_CONNECT(connect(m_spSelectionAnalyzer.get(), &CsvTableViewSelectionAnalyzer::sigAnalyzeCompleted, this, &GraphDataSource::sigChanged));
    m_spView->addSelectionAnalyzer(m_spSelectionAnalyzer);
}

QObject* CsvTableViewChartDataSource::underlyingSource()
{
    return m_spView;
}

void CsvTableViewChartDataSource::forEachElement_fromTableSelection(std::function<void(DataSourceIndex, DataSourceIndex, QVariant)> handler)
{
    if (!handler || !m_spView)
        return;

    auto spTableView = (m_spDataViewer) ? m_spDataViewer->view() : nullptr;
    if (!spTableView) // This may happen e.g. if the table is being updated in analyzeImpl() (in another thread). 
        return;
    const auto& rTable = *spTableView;

    for (auto iterCol = rTable.begin(); iterCol != rTable.end(); ++iterCol)
    {
        for (auto iterRow = iterCol->second.begin(); iterRow != iterCol->second.end(); ++iterRow)
        {
            handler(static_cast<DataSourceIndex>(iterRow->first), static_cast<DataSourceIndex>(iterCol->first), iterRow->second);
        }
    }
}

auto CsvTableViewChartDataSource::columnCount() const -> DataSourceIndex
{
    auto spTableView = privGetTableView();
    return (spTableView) ? spTableView->size() : 0;
}

auto CsvTableViewChartDataSource::columnIndexByName(const StringViewUtf8 sv) const -> DataSourceIndex
{
    auto pModel = (m_spView) ? m_spView->csvModel() : nullptr;
    if (!pModel)
        return invalidIndex();
    auto nIndex = pModel->findColumnIndexByName(QString::fromUtf8(sv.beginRaw(), static_cast<int>(sv.size())), -1);
    if (nIndex < 0)
        return invalidIndex();
    // Getting here means that column was found from the CsvItemModel table; must still check if the column is present in selection.
    auto spTableView = privGetTableView();
    auto pTable = (spTableView) ? &*spTableView : nullptr;
    if (!pTable)
        return invalidIndex();
    auto iter = pTable->find(nIndex);
    return (iter != pTable->end()) ? static_cast<DataSourceIndex>(iter->first) : invalidIndex();
}

auto CsvTableViewChartDataSource::singleColumnDoubleValues_byOffsetFromFirst(const DataSourceIndex offsetFromFirst) -> SingleColumnDoubleValuesOptional
{
    auto spTableView = privGetTableView();
    if (!spTableView) // This may happen e.g. if the table is being updated in analyzeImpl() (in another thread). 
        return SingleColumnDoubleValuesOptional();
    const auto& rTable = *spTableView;
    if (rTable.empty())
        return SingleColumnDoubleValuesOptional();
    const DataSourceIndex nFirstCol = rTable.frontKey();
    const auto nTargetCol = nFirstCol + offsetFromFirst;
    return singleColumnDoubleValues_byColumnIndex(nTargetCol);
}

auto CsvTableViewChartDataSource::singleColumnDoubleValues_byColumnIndex(const DataSourceIndex nColIndex) -> SingleColumnDoubleValuesOptional
{
    auto spTableView = privGetTableView();
    if (!spTableView) // This may happen e.g. if the table is being updated in analyzeImpl() (in another thread). 
        return SingleColumnDoubleValuesOptional();
    const auto& rTable = *spTableView;
    if (rTable.empty())
        return SingleColumnDoubleValuesOptional();
    auto iter = rTable.find(nColIndex);
    if (iter == rTable.end())
        return SingleColumnDoubleValuesOptional();
    const auto& values = iter->second.valueRange();
    auto rv = std::make_shared<DoubleValueVector>();
    rv->resize(values.size());
    std::copy(values.cbegin(), values.cend(), rv->begin());
    return rv;
}

void CsvTableViewChartDataSource::enable(const bool b)
{
    if (!m_spView)
        return;
    if (b)
    {
        m_spView->addSelectionAnalyzer(m_spSelectionAnalyzer);
        m_spSelectionAnalyzer->addSelectionToQueue(m_spView->getSelection());
    }
    else
        m_spView->removeSelectionAnalyzer(m_spSelectionAnalyzer.get());
}

auto CsvTableViewChartDataSource::columnDataTypes() const -> ColumnDataTypeMap
{
    auto spViewer = privGetTableView();
    return (spViewer) ? spViewer->m_columnTypes : ColumnDataTypeMap();
}

auto CsvTableViewChartDataSource::columnNames() const -> ColumnNameMap
{
    auto spViewer = privGetTableView();
    return (spViewer) ? spViewer->m_columnNames : ColumnNameMap();
}

auto CsvTableViewChartDataSource::privGetTableView() const -> std::shared_ptr<const SelectionAnalyzerForGraphing::Table>
{
    return (m_spDataViewer) ? m_spDataViewer->view() : nullptr;
}

} } // Module namespace

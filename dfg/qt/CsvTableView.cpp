#include "qtIncludeHelpers.hpp"
#include "qtBasic.hpp" // This must be included before DelimitedTextReader.hpp gets included, otherwise on some compilers there are compile errors about missing QTextStream overloads.
#include "CsvTableView.hpp"
#include "CsvItemModel.hpp"
#include "CsvTableViewActions.hpp"
#include "QtApplication.hpp"
#include "widgetHelpers.hpp"
#include "../os.hpp"
#include "../os/TemporaryFileStream.hpp"
#include "PropertyHelper.hpp"
#include "connectHelper.hpp"
#include "CsvTableViewCompleterDelegate.hpp"
#include "../time/timerCpu.hpp"
#include "../cont/valueArray.hpp"
#include "TableEditor.hpp"
#include "../rand.hpp"
#include "../rand/distributionHelpers.hpp"
#include "../cont/SetVector.hpp"
#include "JsonListWidget.hpp"
#include "sqlTools.hpp"
#include "../math/FormulaParser.hpp"
#include "../func/memFuncMedian.hpp"
#include "InputDialog.hpp"
#include "stringConversions.hpp"
#include "../cont/Flags.hpp"
#include "../math/sign.hpp"
#include "../str.hpp"
#include "detail/CsvTableView/SelectionDetailCollector.hpp"
#include "detail/CsvTableView/ContentGeneratorDialog.hpp"
#include <chrono>
#include <bitset>
#include <cctype> // For std::isdigit
#include <thread>

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QMenu>
#include <QFileDialog>
#include <QUndoStack>
#include <QFormLayout>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QCompleter>
#include <QDate>
#include <QDateTime>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLabel>
#include <QMessageBox>
#include <QMetaMethod>
#include <QMimeData>
#include <QMutex>
#include <QMutexLocker>
#include <QProcess>
#include <QProgressBar>
#include <QProgressDialog>
#include <QPushButton>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QThread>
#include <QTime>
#include <QTimer>
#include <QToolButton>
#include <QToolTip>
#include <QUndoView>
#include <QUrl>
#include <QReadWriteLock>
#include <QStringListModel>
DFG_END_INCLUDE_QT_HEADERS

#include <map>
#include "../alg.hpp"
#include "../cont/SortedSequence.hpp"
#include "../math.hpp"
#include "../str/stringLiteralCharToValue.hpp"
#include "../io/DelimitedTextWriter.hpp"

#if (DFG_BUILD_OPT_USE_BOOST == 1)
    DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
        #include <boost/accumulators/framework/accumulator_set.hpp>
        #include <boost/accumulators/statistics/variance.hpp>
        #include <boost/accumulators/statistics/stats.hpp>
    DFG_END_INCLUDE_WITH_DISABLED_WARNINGS
#endif // DFG_BUILD_OPT_USE_BOOST

#define DFG_CSVTABLEVIEW_PROPERTY_PREFIX "CsvTableView_"

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt) { namespace DFG_DETAIL_NS
{
    template <class T>
    QString floatToQString(const T val, const FloatToStringParam toStrParam)
    {
        return QString::fromLatin1(::DFG_MODULE_NS(str)::floatingPointToStr<DFG_ROOT_NS::StringAscii>(val, toStrParam.m_nPrecision).c_str().c_str());
    }

    const char* gBasicSelectionDetailCollectorStrIds[] = 
    {
        // Note: order must match with BasicSelectionDetailCollector::Detail enum
        "cell_count_included",
        "cell_count_excluded",
        "sum",
        "average",
        "median",
        "min",
        "max",
        #if defined(BOOST_VERSION)
            "variance",
            "stddev_population",
            "stddev_sample",
        #endif // BOOST_VERSION
        "is_sorted_num"
    };

    const char* gBasicSelectionDetailCollectorUiNames_long[] =
    {
        // Note: order must match with BasicSelectionDetailCollector::Detail enum
        QT_TR_NOOP("Count (Included)"),
        QT_TR_NOOP("Count (Excluded)"),
        QT_TR_NOOP("Sum"),
        QT_TR_NOOP("Avg"),
        QT_TR_NOOP("Median"),
        QT_TR_NOOP("Min"),
        QT_TR_NOOP("Max"),
        #if defined(BOOST_VERSION)
            QT_TR_NOOP("Variance"),
            QT_TR_NOOP("Standard deviation (population)"),
            QT_TR_NOOP("Standard deviation (sample)"),
        #endif // BOOST_VERSION
        QT_TR_NOOP("Is sorted (numerically)")
    };

    const char* gBasicSelectionDetailCollectorUiNames_short[] =
    {
        // Note: order must match with BasicSelectionDetailCollector::Detail enum
        QT_TR_NOOP("Included"),
        QT_TR_NOOP("Excluded"),
        QT_TR_NOOP("Sum"),
        QT_TR_NOOP("Avg"),
        QT_TR_NOOP("Median"),
        QT_TR_NOOP("Min"),
        QT_TR_NOOP("Max"),
        #if defined(BOOST_VERSION)
            QT_TR_NOOP("Variance"),
            QT_TR_NOOP("StdDev (pop)"),
            QT_TR_NOOP("StdDev (smp)"),
        #endif // BOOST_VERSION
        QT_TR_NOOP("Is sorted (num)")
    };

    class BasicSelectionDetailCollector
    {
    public:
        enum class BuiltInDetail
        {
            // Note: order must match with gBasicSelectionDetailCollectorUiNames
            // Note: these are expected to be bit shifts
            cellCountIncluded,
            cellCountExcluded,
            sum,
            average,
            median,
            minimum,
            maximum,
#if defined(BOOST_VERSION)
            variance,
            stddev_population,
            stddev_sample,
#endif // BOOST_VERSION
            isSortedNum,
            detailCount
        }; // enum Detail

        using FlagContainer = std::bitset<32>;

        BasicSelectionDetailCollector()
        {
            m_enableFlags = defaultEnableFlags();
        }

        static FlagContainer defaultEnableFlags()
        {
            FlagContainer flags;
            flags.set();
            flags[static_cast<int>(BuiltInDetail::median)] = false;
#if defined(BOOST_VERSION)
            flags[static_cast<int>(BuiltInDetail::variance)] = false;
            flags[static_cast<int>(BuiltInDetail::stddev_population)] = false;
            flags[static_cast<int>(BuiltInDetail::stddev_sample)] = false;
#endif // BOOST_VERSION
            flags[static_cast<int>(BuiltInDetail::isSortedNum)] = false;
            return flags;
        }

        void handleCell(const QAbstractItemModel& rModel, const QModelIndex& index)
        {
            QString str = rModel.data(index).toString();
            str.replace(',', '.'); // Hack: to make comma-localized values such as "1,2" be interpreted as 1.2
            bool bOk;
            const double val = str.toDouble(&bOk);
            if (bOk)
            {
                m_avgMf(val);
                m_minMaxMf(val);
                if (isEnabled(BuiltInDetail::median))
                    m_medianMf(val);
#if defined(BOOST_VERSION)
                if (isEnabled(BuiltInDetail::variance) || isEnabled(BuiltInDetail::stddev_population) || isEnabled(BuiltInDetail::stddev_sample))
                    m_varianceMf(val);
#endif // BOOST_VERSION
                if (isEnabled(BuiltInDetail::isSortedNum) && m_nSortedUntil + 1 >= m_avgMf.callCount())
                {
                    if (::DFG_MODULE_NS(math)::isNan(val) ||  // Ignoring NaNs
                        m_avgMf.callCount() == 1 ||
                        val == m_previousNumber ||
                        m_sortDirection == 0 ||
                        (m_sortDirection == 1 && val >= m_previousNumber) ||
                        (m_sortDirection == -1 && val <= m_previousNumber))
                    {
                        ++m_nSortedUntil;
                        if (m_sortDirection == 0)
                        {
                            if (val > m_previousNumber)
                                m_sortDirection = 1;
                            else if (val < m_previousNumber)
                                m_sortDirection = -1;
                        }
                    }
                }
                m_previousNumber = val;

                for (auto pCollector : m_activeNonBuiltInCollectors)
                {
                    if (!pCollector)
                        continue;
                    pCollector->update(val);
                }
            }
            else
                ++m_nExcluded;
        }

        template <size_t N>
        static QString privUiName(const BuiltInDetail id, const char* (&arr)[N])
        {
            const auto i = static_cast<int>(id);
            return (isValidIndex(arr, i)) ? QObject::tr(arr[i]) : QString();
        }

        static QString uiName_long(const BuiltInDetail id)
        {
            return privUiName(id, gBasicSelectionDetailCollectorUiNames_long);
        }

        static QString uiName_short(const BuiltInDetail id)
        {
            return privUiName(id, gBasicSelectionDetailCollectorUiNames_short);
        }

        QString privMakeIsSortedValueString(const QItemSelection& selection) const
        {
            if (selection.size() != 1 || m_avgMf.callCount() == 0)
                return QObject::tr("N/A");
            const char* pszSortDir = (m_sortDirection >= 0) ? QT_TR_NOOP("asc") : QT_TR_NOOP("desc");
            if (m_nSortedUntil == m_avgMf.callCount())
                return QObject::tr("yes (%1)").arg(pszSortDir);
            else
                return QObject::tr("no (%1 for %2 first)").arg(pszSortDir).arg(m_nSortedUntil);
        }

        QString uiValueStr(const BuiltInDetail id, const QItemSelection& selection, const FloatToStringParam toStrParam) const
        {
            switch (id)
            {
                case BuiltInDetail::cellCountIncluded: return QString::number(m_avgMf.callCount());
                case BuiltInDetail::cellCountExcluded: return QString::number(m_nExcluded);
                case BuiltInDetail::sum              : return floatToQString(m_avgMf.sum(), toStrParam);
                case BuiltInDetail::average          : return floatToQString(m_avgMf.average(), toStrParam);
                case BuiltInDetail::median           : return floatToQString(m_medianMf.median(), toStrParam);
                case BuiltInDetail::minimum          : return floatToQString(m_minMaxMf.minValue(), toStrParam);
                case BuiltInDetail::maximum          : return floatToQString(m_minMaxMf.maxValue(), toStrParam);
#if defined(BOOST_VERSION)
                case BuiltInDetail::variance         : return floatToQString(boost::accumulators::variance(m_varianceMf), toStrParam);
                case BuiltInDetail::stddev_population: return floatToQString(std::sqrt(boost::accumulators::variance(m_varianceMf)), toStrParam);
                case BuiltInDetail::stddev_sample    : return floatToQString(std::sqrt(double(m_avgMf.callCount()) / double(m_avgMf.callCount() - 1) * boost::accumulators::variance(m_varianceMf)), toStrParam);
#endif
                case BuiltInDetail::isSortedNum      : return privMakeIsSortedValueString(selection);
                default: DFG_ASSERT_IMPLEMENTED(false); return QString();
            }
        }

        static auto selectionDetailNameToId(const ::DFG_ROOT_NS::StringViewUtf8& svId) -> BuiltInDetail;
        static auto builtInDetailToStrId(BuiltInDetail detail) -> StringViewUtf8;

        QString resultString(const QItemSelection& selection, const FloatToStringParam toStrParam) const
        {
            DFG_STATIC_ASSERT(DFG_COUNTOF(gBasicSelectionDetailCollectorStrIds)        == static_cast<size_t>(BuiltInDetail::detailCount), "Detail enum/string array count mismatch");
            DFG_STATIC_ASSERT(DFG_COUNTOF(gBasicSelectionDetailCollectorUiNames_long)  == static_cast<size_t>(BuiltInDetail::detailCount), "Detail enum/string array count mismatch");
            DFG_STATIC_ASSERT(DFG_COUNTOF(gBasicSelectionDetailCollectorUiNames_short) == static_cast<size_t>(BuiltInDetail::detailCount), "Detail enum/string array count mismatch");
            QString s;

            const auto addToResultString = [&](const QString& uiName, const QString& sValue)
            {
                if (!s.isEmpty())
                    s += ", ";
                s += QString("%1: %2").arg(uiName, sValue);
            };

            forEachBuiltInDetailIdWhile([&](const BuiltInDetail& id)
            {
                if (!isEnabled(id))
                    return true;
                auto pCollector = (m_spCollectors) ? m_spCollectors->find(builtInDetailToStrId(id)) : nullptr;
                addToResultString(uiName_short(id), uiValueStr(id, selection, (pCollector) ? pCollector->toStrParam(toStrParam) : toStrParam));
                return true;
            });
            for (auto pCollector : m_activeNonBuiltInCollectors)
            {
                if (pCollector)
                    addToResultString(pCollector->getUiName_short(), floatToQString(pCollector->value(), pCollector->toStrParam(toStrParam)));
            }
            return s;
        }

        template <class Func_T>
        static void forEachBuiltInDetailIdWhile(Func_T&& func)
        {
            for (int i = 0; i < static_cast<int>(BuiltInDetail::detailCount); ++i)
            {
                if (!func(static_cast<BuiltInDetail>(i)))
                    break;
            }
        }

        bool isEnabled(const BuiltInDetail i) const
        {
            return isEnabled(m_enableFlags, i);
        }

        static bool isEnabled(const FlagContainer& cont, const BuiltInDetail i)
        {
            return cont[static_cast<int>(i)];
        }

        void setEnabled(const BuiltInDetail i, const bool b)
        {
            m_enableFlags[static_cast<int>(i)] = b;
        }

        void setEnableFlags(const FlagContainer& flags)
        {
            m_enableFlags = flags;
        }

        void loadDetailHandlers(CsvTableViewBasicSelectionAnalyzerPanel::CollectorContainerPtr spCollectors);

        ::DFG_MODULE_NS(func)::MemFuncMinMax<double> m_minMaxMf;
        ::DFG_MODULE_NS(func)::MemFuncAvg<double> m_avgMf;
        ::DFG_MODULE_NS(func)::MemFuncMedian<double> m_medianMf;
#if defined(BOOST_VERSION)
        boost::accumulators::accumulator_set<double, boost::accumulators::stats<boost::accumulators::tag::variance(boost::accumulators::lazy)>> m_varianceMf;
#endif
        FlagContainer m_enableFlags;
        size_t m_nExcluded = 0;
        double m_previousNumber = std::numeric_limits<double>::quiet_NaN();
        uint32 m_nSortedUntil = 0; // Stores the number of sorted cells from first.
        int m_sortDirection = 0; // 1 = ascending, -1 = descending, 0 = either.
        CsvTableViewBasicSelectionAnalyzerPanel::CollectorContainerPtr m_spCollectors;
        std::vector<SelectionDetailCollector*> m_activeNonBuiltInCollectors;
    }; // BasicSelectionDetailCollector

    auto BasicSelectionDetailCollector::builtInDetailToStrId(const BuiltInDetail detail) -> StringViewUtf8
    {
        const auto i = static_cast<int>(detail);
        return (isValidIndex(gBasicSelectionDetailCollectorStrIds, i)) ? StringViewUtf8(SzPtrUtf8(gBasicSelectionDetailCollectorStrIds[i])) : StringViewUtf8();
    }

    auto BasicSelectionDetailCollector::selectionDetailNameToId(const StringViewUtf8& svId) -> BuiltInDetail
    {
        for (size_t i = 0; i < static_cast<size_t>(BuiltInDetail::detailCount); ++i)
        {
            const auto detailId = static_cast<BuiltInDetail>(i);
            if (svId == builtInDetailToStrId(detailId))
                return detailId;
        }
        return BuiltInDetail::detailCount;
    }

    template <class WeekDayListGen_T>
    QString& handleWeekDayFormat(QString& s, const int weekDay, WeekDayListGen_T&& listGen)
    {
        const auto nIndexOfWd = s.indexOf(QLatin1String("WD"));
        if (nIndexOfWd < 0)
            return s;
        const auto weekDayNames = listGen();
        if (::DFG_ROOT_NS::isValidIndex(weekDayNames, weekDay - 1))
            s.replace(nIndexOfWd, 2, weekDayNames[weekDay - 1]);
        return s;
    }

    class ConfFileProperty
    {
    public:
        ConfFileProperty(QString sKey, QString sDefaultValue)
            : m_sKey(std::move(sKey))
            , m_sDefaultValue(std::move(sDefaultValue))
        {}

        const QString& key() const;
        const QString& defaultValue() const;

        QString m_sKey;
        QString m_sDefaultValue;
    }; // class ConfFileProperty

    const QString& ConfFileProperty::key() const
    {
        return m_sKey;
    }

    const QString& ConfFileProperty::defaultValue() const
    {
        return m_sDefaultValue;
    }

}}} // dfg::qt::DFG_DETAIL_NS


/////////////////////////////////
// Start of dfg::qt namespace
DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt) {

namespace
{
    enum CsvTableViewPropertyId
    {
        CsvTableViewPropertyId_diffProgPath,
        CsvTableViewPropertyId_initialScrollPosition,
        CsvTableViewPropertyId_minimumVisibleColumnWidth,
        CsvTableViewPropertyId_timeFormat,
        CsvTableViewPropertyId_dateFormat,
        CsvTableViewPropertyId_dateTimeFormat,
        CsvTableViewPropertyId_editMode,
        CsvTableViewPropertyId_weekDayNames
    };

    DFG_QT_DEFINE_OBJECT_PROPERTY_CLASS(CsvTableView)

    template <CsvTableViewPropertyId ID>
    auto getCsvTableViewProperty(const CsvTableView* view) -> typename CsvTableViewPropertyDefinition<ID>::PropertyType
    {
        return DFG_MODULE_NS(qt)::getProperty<DFG_QT_OBJECT_PROPERTY_CLASS_NAME(CsvTableView)<ID>>(view);
    }

    template <CsvTableViewPropertyId ID>
    QString getCsvModelOrViewProperty(const CsvTableView* pView)
    {
        auto pCsvModel = pView->csvModel();
        if (pCsvModel)
        {
            const ::DFG_ROOT_NS::StringViewC svId = DFG_QT_OBJECT_PROPERTY_CLASS_NAME(CsvTableView)<ID>::getStrId();
            const auto svPropertyName = svId.substr_start(DFG_COUNTOF_SZ(DFG_CSVTABLEVIEW_PROPERTY_PREFIX));
            const auto modelOptions = pCsvModel->getOpenTimeLoadOptions();
            const auto s = modelOptions.getProperty(svPropertyName, std::string(1, '\0'));
            if (s.size() != 1 || s[0] != '\0')
                return QString::fromUtf8(s.c_str());
        }
        return getCsvTableViewProperty<ID>(pView);
    }

    template <CsvTableViewPropertyId ID>
    void setCsvTableViewProperty(CsvTableView* view, const typename CsvTableViewPropertyDefinition<ID>::PropertyType& val)
    {
        DFG_MODULE_NS(qt)::setProperty<DFG_QT_OBJECT_PROPERTY_CLASS_NAME(CsvTableView)<ID>>(view, QVariant(val));
    }

    // Properties
    DFG_QT_DEFINE_OBJECT_PROPERTY("diffProgPath", CsvTableView, CsvTableViewPropertyId_diffProgPath, QString, PropertyType);
    DFG_QT_DEFINE_OBJECT_PROPERTY(DFG_CSVTABLEVIEW_PROPERTY_PREFIX "initialScrollPosition", CsvTableView, CsvTableViewPropertyId_initialScrollPosition, QString, PropertyType);
    DFG_QT_DEFINE_OBJECT_PROPERTY(DFG_CSVTABLEVIEW_PROPERTY_PREFIX "minimumVisibleColumnWidth", CsvTableView, CsvTableViewPropertyId_minimumVisibleColumnWidth, int, []() { return 5; });
    DFG_QT_DEFINE_OBJECT_PROPERTY(DFG_CSVTABLEVIEW_PROPERTY_PREFIX "timeFormat", CsvTableView, CsvTableViewPropertyId_timeFormat, QString, []() { return QString("hh:mm:ss.zzz"); });
    DFG_QT_DEFINE_OBJECT_PROPERTY(DFG_CSVTABLEVIEW_PROPERTY_PREFIX "dateFormat", CsvTableView, CsvTableViewPropertyId_dateFormat, QString, []() { return QString("yyyy-MM-dd"); });
    DFG_QT_DEFINE_OBJECT_PROPERTY(DFG_CSVTABLEVIEW_PROPERTY_PREFIX "dateTimeFormat", CsvTableView, CsvTableViewPropertyId_dateTimeFormat, QString, []() { return QString("yyyy-MM-dd hh:mm:ss.zzz"); });
    DFG_QT_DEFINE_OBJECT_PROPERTY(DFG_CSVTABLEVIEW_PROPERTY_PREFIX "editMode", CsvTableView, CsvTableViewPropertyId_editMode, QString, []() { return QString(); });
    DFG_QT_DEFINE_OBJECT_PROPERTY_QSTRING(DFG_CSVTABLEVIEW_PROPERTY_PREFIX "weekDayNames", CsvTableView, CsvTableViewPropertyId_weekDayNames, QString, []() { return QString("mo,tu,we,th,fr,sa,su"); });

    const int gnDefaultRowHeight = 21; // Default row height seems to be 30, which looks somewhat wasteful so make it smaller.

    class UndoViewWidget : public QDialog
    {
    public:
        typedef QDialog BaseClass;
        UndoViewWidget(CsvTableView* pParent)
            : BaseClass(pParent)
            , m_spTableView(pParent)
        {
            setAttribute(Qt::WA_DeleteOnClose, true);
            removeContextHelpButtonFromDialog(this);
            if (!pParent || !pParent->m_spUndoStack)
                return;
            auto pLayout = new QHBoxLayout(this);
            m_spUndoView.reset(new QUndoView(&pParent->m_spUndoStack.get()->item(), this));
            const auto children = m_spUndoView->children();
            m_spUndoView->installEventFilter(this);
            for (auto pChild : children) if (pChild)
                pChild->installEventFilter(this);
            pLayout->addWidget(m_spUndoView.get());
        }

        // Hack: Because QUndoView handles undo stack directly, it will bypass CsvTableView read/edit lock when used. To prevent this,
        //       using event filter to check if lock is available before allowing events that possibly trigger undo stack.
        //       While hacky, this should address at least most cases. Window will also be left in disabled state after operation ends,
        //       but can be re-enabled simply be clicking.
        bool eventFilter(QObject* pObj, QEvent* pEvent) override
        {
            const auto eventType = (pEvent) ? pEvent->type() : QEvent::None;
            if ((eventType == QEvent::KeyPress || eventType == QEvent::MouseButtonPress) && (m_spTableView && m_spUndoView))
            {
                auto lockReleaser = m_spTableView->tryLockForEdit();
                const auto bEnable = lockReleaser.isLocked();
                m_spUndoView->setEnabled(bEnable);
                if (!bEnable)
                    return true; // Returning true means that child doesn't get the event. 
            }
            return BaseClass::eventFilter(pObj, pEvent);
        }

        ~UndoViewWidget()
        {
        }

        QPointer<CsvTableView> m_spTableView;
        QObjectStorage<QUndoView> m_spUndoView;
    }; // Class UndoViewWidget

    static void doModalOperation(QWidget* pParent, const QString& sProgressDialogLabel, const ProgressWidget::IsCancellable isCancellable, const QString& sThreadName, std::function<void (ProgressWidget*)> func)
    {
        QEventLoop eventLoop;

        auto pProgressDialog = new ProgressWidget(sProgressDialogLabel, isCancellable, pParent);
        auto pWorkerThread = new QThread();
        pWorkerThread->setObjectName(sThreadName); // Sets thread name visible to debugger.
        DFG_MAYBE_UNUSED const auto callerThreadId = std::this_thread::get_id();
        DFG_QT_VERIFY_CONNECT(QObject::connect(pWorkerThread, &QThread::started, [&]() // Note: while typically there would be a context-object in connect()-arguments,
                                                                                       //       not using it here since e.g. using pWorkerThread there would cause functor to get executed in caller-thread
                                                                                       //       instead of worker thread. (ticket #141)
                {
                    DFG_ASSERT_CORRECTNESS(callerThreadId != std::this_thread::get_id());
                    func(pProgressDialog);
                    pWorkerThread->quit();
                }));
        // Connect thread finish to trigger event loop quit and closing of progress bar.
        DFG_QT_VERIFY_CONNECT(QObject::connect(pWorkerThread, &QThread::finished, &eventLoop, &QEventLoop::quit));
        DFG_QT_VERIFY_CONNECT(QObject::connect(pWorkerThread, &QThread::finished, pWorkerThread, &QObject::deleteLater));
        DFG_QT_VERIFY_CONNECT(QObject::connect(pWorkerThread, &QObject::destroyed, pProgressDialog, &QObject::deleteLater));

        pWorkerThread->start();

        // Wait a while before showing the progress dialog; don't want to pop it up for tiny files.
        QTimer::singleShot(750, pProgressDialog, SLOT(show()));

        // Keep event loop running while operating.
        eventLoop.exec();
    }

    QString getSQLiteDatabaseQueryFromUser(const QString& sFilePath, QWidget* pParent)
    {
        ::DFG_MODULE_NS(sql)::SQLiteFileOpenDialog dlg(sFilePath, pParent);
        return (dlg.exec() == QDialog::Accepted) ? dlg.query() : QString();
    }

    struct CsvTableViewFlag
    {
        enum
        {
            readOnly = 0
        };
    };

    DFG_DEFINE_SCOPED_ENUM_FLAGS_WITH_OPERATORS(ActionFlags, ::DFG_ROOT_NS::uint32,
        unknown               = 0x0,
        readOnly              = 0x1,  // Makes no changes of any kind to table content nor to view.
        contentEdit           = 0x2,  // May edit table content
        contentStructureEdit  = 0x4,  // May edit table structure (changing row/column count etc.).
        viewEdit              = 0x8,  // May edit view (selection, proxy/filter).
        confEdit              = 0x10, // May edit fields related to .conf-file (e.g. column type), but not actual table content
        anyContentEdit        = contentEdit | contentStructureEdit,
        defaultContentEdit    = contentEdit | viewEdit,
        defaultStructureEdit  = contentEdit | contentStructureEdit | viewEdit
    ) // ActionsFlags

    const char gszPropertyActionFlags[] = "dfglib_actionFlags";

    void setActionFlags(QAction* pAction, const ActionFlags flags)
    {
        if (!pAction)
            return;
        pAction->setProperty(gszPropertyActionFlags, flags.toNumber());
    }

    ActionFlags getActionType(QAction* pAction)
    {
        return (pAction) ? ActionFlags::fromNumber(pAction->property(gszPropertyActionFlags).toUInt()) : ActionFlags::unknown;
    }

    QMenu* createActionMenu(::DFG_MODULE_NS(qt)::CsvTableView* pParent, const QString& sMenuTitle, const ActionFlags actionFlags, QMenu* pMenu = nullptr)
    {
        if (!pParent)
            return nullptr;
        auto pMenuAction = new QAction(sMenuTitle, pParent);
        setActionFlags(pMenuAction, actionFlags);
        if (!pMenu)
            pMenu = new QMenu();
        // Scheduling destruction of menu with the parent action.
        DFG_QT_VERIFY_CONNECT(QObject::connect(pMenuAction, &QObject::destroyed, pMenu, &QObject::deleteLater));
        pMenuAction->setMenu(pMenu); // Does not transfer ownership.
        pParent->addAction(pMenuAction);
        return pMenu;
    }

    struct ActionCheckability
    {
        ActionCheckability(bool bCheckable, bool bChecked = false)
            : m_checkable(bCheckable)
            , m_on(bChecked)
        {}
        bool m_checkable;
        bool m_on;
    }; // ActionCheckability

    template <class Slot_T>
    QAction& addViewAction(::DFG_MODULE_NS(qt)::CsvTableView& rView,
        QWidget& rAddActionTargetAndParent,
        const QString& sTitle,
        const QString& sShortCut,
        const ActionFlags actionFlags,
        const ActionCheckability checkability,
        Slot_T&& slot)
    {
        auto pAction = new QAction(sTitle, &rAddActionTargetAndParent);
        if (!sShortCut.isEmpty())
            pAction->setShortcut(sShortCut);
        if (actionFlags != ActionFlags::unknown)
            setActionFlags(pAction, actionFlags);
        if (checkability.m_checkable)
        {
            pAction->setCheckable(checkability.m_checkable);
            pAction->setChecked(checkability.m_on);
            DFG_QT_VERIFY_CONNECT(QObject::connect(pAction, &QAction::toggled, &rView, slot));
        }
        else
            DFG_QT_VERIFY_CONNECT(QObject::connect(pAction, &QAction::triggered, &rView, slot));
        rAddActionTargetAndParent.addAction(pAction); // Ownership of pAction is not transferred to rAddActionTargetAndParent.
        return *pAction;
    }

    const char noShortCut[] = "";

} // unnamed namespace

DFG_STATIC_ASSERT((std::is_same<CsvTableView::Index, CsvItemModel::Index>::value), "CsvTableView::Index and CsvItemModel::Index differ");

DFG_OPAQUE_PTR_DEFINE(CsvTableView)
{
public:
    std::vector<CsvTableView::PropertyFetcher> m_propertyFetchers;
    std::bitset<1> m_flags;
    QPointer<QAction> m_spActReadOnly;
    QPointer<QAction> m_spActSortCaseSensitivity;
    QAbstractItemView::EditTriggers m_editTriggers;
    QPalette m_readWriteModePalette;
    QVariantMap m_previousChangeRadixArgs;
    QString m_sInitialScrollPosition;
    QObjectStorage<QCheckBox> m_spFilterCheckBoxCaseSensitive;
    QObjectStorage<QCheckBox> m_spFilterCheckBoxWholeStringMatch;
    QObjectStorage<QCheckBox> m_spFilterCheckBoxColumnMatchByAnd;
};

const char CsvTableView::s_szCsvSaveOption_saveAsShown[] = "CsvTableView_saveAsShown";

CsvTableView::CsvTableView(std::shared_ptr<QReadWriteLock> spReadWriteLock, QWidget* pParent, const ViewType viewType)
    : BaseClass(pParent)
    , m_matchDef(QString(), Qt::CaseInsensitive, PatternMatcher::Wildcard)
    , m_nFindColumnIndex(0)
    , m_bUndoEnabled(true)
{
    m_spEditLock = std::move(spReadWriteLock);
    if (!m_spEditLock)
        m_spEditLock = std::make_shared<QReadWriteLock>(QReadWriteLock::Recursive);
    this->setItemDelegate(new CsvTableViewDelegate(this));

    this->setHorizontalHeader(new TableHeaderView(this));

    auto pVertHdr = verticalHeader();
    if (pVertHdr)
    {
        const auto nMinSize = pVertHdr->minimumSectionSize();
        pVertHdr->setMinimumSectionSize(Min(nMinSize, gnDefaultRowHeight));
        pVertHdr->setDefaultSectionSize(gnDefaultRowHeight); // TODO: make customisable
    }

    // TODO: make customisable.
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    switch (viewType)
    {
        case ViewType::allFeatures: addAllActions(); break;
        case ViewType::fixedDimensionEdit:
            addFindAndSelectionActions();
                addSeparatorAction();
            addContentEditActions();
                addSeparatorAction();
            addHeaderActions();
                addSeparatorAction();
        break;
    }
}

CsvTableView::CsvTableView(TagCreateWithModels, QWidget* pParent, ViewType viewType)
    : CsvTableView(nullptr, pParent, viewType)
{
    auto pCsvModel = new CsvItemModel;
    pCsvModel->setParent(this);
    auto pProxyModel = new CsvTableViewSortFilterProxyModel(this);
    pProxyModel->setSourceModel(pCsvModel);
    DFG_ASSERT_CORRECTNESS(pCsvModel->parent() == this);
    DFG_ASSERT_CORRECTNESS(pProxyModel->parent() == this);
    this->setModel(pProxyModel);
}

void CsvTableView::addSeparatorAction()
{
    addSeparatorActionTo(this);
}

void CsvTableView::addSeparatorActionTo(QWidget* pTarget)
{
    if (!pTarget)
        return;
    auto pAction = new QAction(this);
    pAction->setSeparator(true);
    pTarget->addAction(pAction);
}

void CsvTableView::addAllActions()
{
    addOpenSaveActions();
        addSeparatorAction();
    addDimensionEditActions();
        addSeparatorAction();
    addFindAndSelectionActions();
        addSeparatorAction();
    addContentEditActions();
        addSeparatorAction();
    addSortActions();
        addSeparatorAction();
    addHeaderActions();
        addSeparatorAction();
    addMiscellaneousActions();
}

#define DFG_TEMP_ADD_VIEW_ACTION(OBJ, NAME, SHORTCUT, ACTIONFLAGS, HANDLER) addViewAction(*this, OBJ, NAME, SHORTCUT, ACTIONFLAGS, false, &ThisClass::HANDLER)
#define DFG_TEMP_ADD_VIEW_ACTION_CHECKABLE(OBJ, NAME, SHORTCUT, ACTIONFLAGS, HANDLER) addViewAction(*this, OBJ, NAME, SHORTCUT, ACTIONFLAGS, true, &ThisClass::HANDLER)

void CsvTableView::addOpenSaveActions()
{
    // New table
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("New table"),                tr("Ctrl+N"),       ActionFlags::unknown, createNewTable);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("New table from clipboard"), tr("Ctrl+Shift+N"), ActionFlags::unknown, createNewTableFromClipboard);

    // Open table
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Open file..."), tr("Ctrl+O"), ActionFlags::unknown, openFromFile);
    // Advanced open -menu
    {
        auto pMenu = createActionMenu(this, tr("Open advanced"), ActionFlags::unknown);
        if (pMenu)
        {
            DFG_TEMP_ADD_VIEW_ACTION(*pMenu, tr("Open file with options..."), noShortCut, ActionFlags::unknown, openFromFileWithOptions);
            DFG_TEMP_ADD_VIEW_ACTION(*pMenu, tr("Reload from file"),          noShortCut, ActionFlags::unknown, reloadFromFile);
        }
    }

    // Merge
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Merge files to current..."), noShortCut, ActionFlags::defaultStructureEdit, mergeFilesToCurrent);

    // Save
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Save"),                         tr("Ctrl+S"),   ActionFlags::unknown, save);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Save to file..."),              noShortCut,     ActionFlags::unknown, saveToFile);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Save to file with options..."), noShortCut,     ActionFlags::unknown, saveToFileWithOptions);

    // Config menu
    {
        auto pMenu = createActionMenu(this, tr("Config"), ActionFlags::unknown);
        if (pMenu)
        {
            DFG_TEMP_ADD_VIEW_ACTION(*pMenu, tr("Open related config file..."),      noShortCut, ActionFlags::unknown, openConfigFile); // To improve: this entry could be disabled if there is no file open or it does not have associated config.
            DFG_TEMP_ADD_VIEW_ACTION(*pMenu, tr("Save config file..."),              noShortCut, ActionFlags::unknown, saveConfigFile);
            DFG_TEMP_ADD_VIEW_ACTION(*pMenu, tr("Save config file with options..."), noShortCut, ActionFlags::unknown, saveConfigFileWithOptions);
            // Adding app config -action (will be hidden if getAllowApplicationSettingsUsage() return false and is made visible later if value changes)
            {
                addSeparatorActionTo(pMenu);
                auto& rAction = DFG_TEMP_ADD_VIEW_ACTION(*pMenu, tr("Open app config file"), noShortCut, ActionFlags::unknown, openAppConfigFile);
                DFG_QT_VERIFY_CONNECT(connect(this, &ThisClass::sigOnAllowApplicationSettingsUsageChanged, &rAction, &QAction::setVisible));
                rAction.setVisible(getAllowApplicationSettingsUsage());
            }
        }
    } // Config menu
}

void CsvTableView::addDimensionEditActions()
{
    // Header actions
    {
        auto pMenu = createActionMenu(this, tr("Header actions"), ActionFlags::unknown);
        if (pMenu)
        {
            // Header/first line move actions
            DFG_TEMP_ADD_VIEW_ACTION(*pMenu, tr("Move first row to header"), noShortCut, ActionFlags::defaultContentEdit, moveFirstRowToHeader);
            DFG_TEMP_ADD_VIEW_ACTION(*pMenu, tr("Move header to first row"), noShortCut, ActionFlags::defaultContentEdit, moveHeaderToFirstRow);

            DFG_TEMP_ADD_VIEW_ACTION(*pMenu, tr("Select column visibility..."), noShortCut, ActionFlags::viewEdit, showSelectColumnVisibilityDialog);
        }
    }

    // "Insert row/column"-items
    {
        auto pMenu = createActionMenu(this, tr("Insert row/column"), ActionFlags::defaultStructureEdit);
        if (pMenu)
        {
            DFG_TEMP_ADD_VIEW_ACTION(*pMenu, tr("Insert row here"),             tr("Ins"),                ActionFlags::defaultStructureEdit, insertRowHere);
            DFG_TEMP_ADD_VIEW_ACTION(*pMenu, tr("Insert row after current"),    tr("Shift+Ins"),          ActionFlags::defaultStructureEdit, insertRowAfterCurrent);
            DFG_TEMP_ADD_VIEW_ACTION(*pMenu, tr("Insert column"),               tr("Ctrl+Alt+Ins"),       ActionFlags::defaultStructureEdit, insertColumn);
            DFG_TEMP_ADD_VIEW_ACTION(*pMenu, tr("Insert column after current"), tr("Ctrl+Alt+Shift+Ins"), ActionFlags::defaultStructureEdit, insertColumnAfterCurrent);
        }
    } // End of "Insert row/column"-items

    // "Delete row/column"-items
    {
        auto pMenu = createActionMenu(this, tr("Delete row/column"), ActionFlags::defaultStructureEdit);
        if (pMenu)
        {
            DFG_TEMP_ADD_VIEW_ACTION(*pMenu, tr("Delete selected row(s)"), tr("Shift+Del"), ActionFlags::defaultStructureEdit, deleteSelectedRow);
            addViewAction(*this, *pMenu,     tr("Delete current column"),  tr("Ctrl+Del"),  ActionFlags::defaultStructureEdit, false, static_cast<decltype(&ThisClass::deleteSelectedRow)>(&ThisClass::deleteCurrentColumn)); // QOverload didn't work on some Qt versions so using deleteSelectedRow() to get bool (void) overload
        }
    } // End of "Insert row/column"-items

    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Resize table"), tr("Ctrl+R"), ActionFlags::defaultStructureEdit, resizeTable);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Transpose"),    noShortCut,   ActionFlags::defaultStructureEdit, transpose);
}

void CsvTableView::addFindAndSelectionActions()
{
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Invert selection"), tr("Ctrl+I"), ActionFlags::viewEdit, invertSelection);

    // Find and filter actions
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Go to line"),    tr("Ctrl+G"),   ActionFlags::viewEdit,           onGoToCellTriggered);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Find"),          tr("Ctrl+F"),   ActionFlags::viewEdit,           onFindRequested);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Find next"),     tr("F3"),       ActionFlags::viewEdit,           onFindNext);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Find previous"), tr("Shift+F3"), ActionFlags::viewEdit,           onFindPrevious);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Replace"),       tr("Ctrl+H"),   ActionFlags::defaultContentEdit, onReplace);

    // Filter-actions
    {
        auto pMenu = createActionMenu(this, tr("Filter"), ActionFlags::viewEdit);
        if (pMenu)
        {
            DFG_TEMP_ADD_VIEW_ACTION(*pMenu, tr("Set filter"), tr("Alt+F"), ActionFlags::viewEdit, onFilterRequested);
            addSectionEntryToMenu(pMenu, "Filter from selection");

            const auto createFilterOptionItem = [&](QObjectStorage<QCheckBox>& spCheckBox, const QString& s, const bool bChecked)
            {
                spCheckBox.reset(new QCheckBox(s, pMenu));
                auto pAct = new QWidgetAction(pMenu);
                spCheckBox->setCheckable(true);
                spCheckBox->setChecked(bChecked);
                pAct->setDefaultWidget(spCheckBox.get());
                return pAct;
            };

            pMenu->addAction(createFilterOptionItem(DFG_OPAQUE_REF().m_spFilterCheckBoxCaseSensitive, tr("Case-sensitive"), false));
            pMenu->addAction(createFilterOptionItem(DFG_OPAQUE_REF().m_spFilterCheckBoxWholeStringMatch, tr("Whole string match"), false));
            pMenu->addAction(createFilterOptionItem(DFG_OPAQUE_REF().m_spFilterCheckBoxColumnMatchByAnd, tr("'and'-matching across columns"), false));

            DFG_TEMP_ADD_VIEW_ACTION(*pMenu, tr("Create filter from selection"), noShortCut, ActionFlags::viewEdit, onFilterFromSelectionRequested);
            
        }
    } // End of Filter actions
    
}

void CsvTableView::addContentEditActions()
{
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Cut"),                          tr("Ctrl+X"), ActionFlags::defaultContentEdit, cut);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Copy"),                         tr("Ctrl+C"), ActionFlags::readOnly,           copy);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Paste"),                        tr("Ctrl+V"), ActionFlags::defaultContentEdit, paste);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Clear selected cell(s)"),       tr("Del"),    ActionFlags::defaultContentEdit, clearSelected);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Generate content..."),          tr("Alt+G"),  ActionFlags::defaultContentEdit, generateContent);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Evaluate selected as formula"), tr("Alt+C"),  ActionFlags::defaultContentEdit, evaluateSelectionAsFormula);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Change radix..."),              noShortCut,   ActionFlags::defaultContentEdit, onChangeRadixUiAction);

    // Insert-menu
    {
        auto pMenu = createActionMenu(this, tr("Insert"), ActionFlags::defaultContentEdit);
        if (pMenu)
        {
            DFG_TEMP_ADD_VIEW_ACTION(*pMenu, tr("Date"),          tr("Alt+Q"),       ActionFlags::defaultContentEdit, insertDate);
            DFG_TEMP_ADD_VIEW_ACTION(*pMenu, tr("Time"),          tr("Alt+W"),       ActionFlags::defaultContentEdit, insertTime);
            DFG_TEMP_ADD_VIEW_ACTION(*pMenu, tr("Date and time"), tr("Shift+Alt+Q"), ActionFlags::defaultContentEdit, insertDateTime);

        }
    } // End of "Insert row/column"-items

    /* Not Implemented
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Move row up"), tr("Alt+Up"), ActionFlags::defaultContentEdit, moveRowUp);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Move row down"), tr("Alt+Up"), ActionFlags::defaultContentEdit, moveRowDown);
    */

    privAddUndoRedoActions();
}

void CsvTableView::addSortActions()
{
    auto pMenu = createActionMenu(this, tr("Sorting"), ActionFlags::viewEdit);
    if (!pMenu)
        return;

    DFG_TEMP_ADD_VIEW_ACTION_CHECKABLE(*pMenu, tr("Sortable columns"),       noShortCut, ActionFlags::viewEdit, setSortingEnabled);
    DFG_OPAQUE_REF().m_spActSortCaseSensitivity = &DFG_TEMP_ADD_VIEW_ACTION_CHECKABLE(*pMenu, tr("Case sensitive sorting"), noShortCut, ActionFlags::viewEdit, setCaseSensitiveSorting);
    DFG_TEMP_ADD_VIEW_ACTION(*pMenu,           tr("Reset sorting"),          noShortCut, ActionFlags::viewEdit, resetSorting);

}

void CsvTableView::addHeaderActions()
{
    auto spMenu = createResizeColumnsMenu();
    if (!spMenu)
        return;

    createActionMenu(this, tr("Resize header"), ActionFlags::readOnly, spMenu.release());
}

void CsvTableView::addMiscellaneousActions()
{
    DFG_TEMP_ADD_VIEW_ACTION(*this,           tr("Diff with unmodified"), tr("Alt+D"), ActionFlags::readOnly, diffWithUnmodified);
    DFG_TEMP_ADD_VIEW_ACTION_CHECKABLE(*this, tr("Row mode"),             noShortCut,  ActionFlags::viewEdit, setRowMode).setToolTip(tr("Selections by row instead of by cell (experimental)"));
    auto& rActReadOnly = DFG_TEMP_ADD_VIEW_ACTION_CHECKABLE(*this, tr("Read-only"), noShortCut, ActionFlags::readOnly, setReadOnlyMode);
    rActReadOnly.setChecked(DFG_OPAQUE_REF().m_flags.test(CsvTableViewFlag::readOnly));
    DFG_OPAQUE_REF().m_spActReadOnly = &rActReadOnly;
}

CsvTableView::~CsvTableView()
{
    // Removing temporary files.
    for (auto iter = m_tempFilePathsToRemoveOnExit.cbegin(), iterEnd = m_tempFilePathsToRemoveOnExit.cend(); iter != iterEnd; ++iter)
    {
        QFile::remove(*iter);
    }
    stopAnalyzerThreads();
}

void CsvTableView::stopAnalyzerThreads()
{
    // Sending interrupt signals to analyzers.
    forEachSelectionAnalyzer([](SelectionAnalyzer& analyzer) { analyzer.disable(); });

    // Sending event loop stop request to analyzer threads.
    forEachSelectionAnalyzerThread([](QThread& thread) { thread.quit(); });
    // Waiting for analyzer threads to stop.
    forEachSelectionAnalyzerThread([](QThread& thread) { thread.wait(); });
}

template <class Func_T>
void CsvTableView::forEachSelectionAnalyzer(Func_T func)
{
    for (auto iter = m_selectionAnalyzers.cbegin(), iterEnd = m_selectionAnalyzers.cend(); iter != iterEnd; ++iter)
    {
        func(**iter);
    }
}

template <class Func_T>
void CsvTableView::forEachSelectionAnalyzerThread(Func_T func)
{
    for (auto iter = m_analyzerThreads.begin(), iterEnd = m_analyzerThreads.end(); iter != iterEnd; ++iter)
    {
        if (*iter)
            func(**iter);
    }
}

auto CsvTableView::createResizeColumnsMenu() -> std::unique_ptr<QMenu>
{
    std::unique_ptr<QMenu> spMenu(new QMenu);

    // Column
    DFG_TEMP_ADD_VIEW_ACTION(*spMenu, tr("Col: Resize to view evenly"),        noShortCut, ActionFlags::readOnly, onColumnResizeAction_toViewEvenly);
    DFG_TEMP_ADD_VIEW_ACTION(*spMenu, tr("Col: Resize to view content aware"), noShortCut, ActionFlags::readOnly, onColumnResizeAction_toViewContentAware);
    DFG_TEMP_ADD_VIEW_ACTION(*spMenu, tr("Col: Resize all to content"),        noShortCut, ActionFlags::readOnly, onColumnResizeAction_content);
    DFG_TEMP_ADD_VIEW_ACTION(*spMenu, tr("Col: Set fixed size..."),            noShortCut, ActionFlags::readOnly, onColumnResizeAction_fixedSize);

    // Note: not using addSection() (i.e. a separator with text) as they are not shown in Windows in default style;
    // can be made visible e.g. by using "fusion" style (for more information see setStyle(), QStyleFactory::create)
    spMenu->addSeparator();

    // Row
    DFG_TEMP_ADD_VIEW_ACTION(*spMenu, tr("Row: Resize all to content"), noShortCut, ActionFlags::readOnly, onRowResizeAction_content);
    DFG_TEMP_ADD_VIEW_ACTION(*spMenu, tr("Row: Set fixed size..."),     noShortCut, ActionFlags::readOnly, onRowResizeAction_fixedSize);

    return spMenu;
}

#undef DFG_TEMP_ADD_VIEW_ACTION
#undef DFG_TEMP_ADD_VIEW_ACTION_CHECKABLE

void CsvTableView::createUndoStack()
{
    m_spUndoStack.reset(new DFG_MODULE_NS(cont)::TorRef<QUndoStack>);
}

void CsvTableView::clearUndoStack()
{
    if (m_spUndoStack)
        m_spUndoStack->item().clear();
}

void CsvTableView::showUndoWindow()
{
    if (!m_spUndoStack)
        return;
    auto pUndoWidget = new UndoViewWidget(this);
    pUndoWidget->show();
}

namespace
{
    const char gszMenuText_enableUndo[] = "Enable undo";
    const char gszMenuText_clearUndoBuffer[] = "&Clear undo buffer";
    const char gszMenuText_showUndoWindow[] = "Show undo buffer";

    void setUndoRedoActionText(QAction* pAction, const QString& sCommandName, const QString& sActionName)
    {
        if (!pAction)
            return;
        if (!sActionName.isEmpty())
            pAction->setText(QString("%1 '%2'").arg(sCommandName, sActionName));
        else
            pAction->setText(sCommandName);
    }

    void setUndoActionText(QAction* pAction, const QString& s) { setUndoRedoActionText(pAction, QAction::tr("&Undo"), s); }
    void setRedoActionText(QAction* pAction, const QString& s) { setUndoRedoActionText(pAction, QAction::tr("&Redo"), s); }
} // unnamed namespace

void CsvTableView::privAddUndoRedoActions(QAction* pAddBefore)
{
    if (!m_spUndoStack)
        createUndoStack();
    if (m_spUndoStack)
    {
        QUndoStack* const undoStackPtr = &m_spUndoStack->item();
        // Adding undo-action
        {
            // auto pActionUndo = m_spUndoStack->item().createUndoAction(this, tr("&Undo"));
            auto pActionUndo = new QAction(tr("&Undo"), this);
            QPointer<QAction> spActionUndo = pActionUndo;
            DFG_QT_VERIFY_CONNECT(connect(pActionUndo, &QAction::triggered, this, &ThisClass::undo));
            DFG_QT_VERIFY_CONNECT(connect(undoStackPtr, &QUndoStack::canUndoChanged, pActionUndo, &QAction::setEnabled));
            DFG_QT_VERIFY_CONNECT(connect(undoStackPtr, &QUndoStack::undoTextChanged, [=](const QString& s) { setUndoActionText(spActionUndo.data(), s); }));
            pActionUndo->setEnabled(undoStackPtr->canUndo());
            pActionUndo->setShortcuts(QKeySequence::Undo);
            DFG_QT_VERIFY_CONNECT(connect(this, &CsvTableView::sigReadOnlyModeChanged, pActionUndo, &QAction::setDisabled));
            insertAction(pAddBefore, pActionUndo);
        }

        // Add redo-action
        {
            //auto pActionRedo = m_spUndoStack->item().createRedoAction(this, tr("&Redo"));
            auto pActionRedo = new QAction(tr("&Redo"), this);
            QPointer<QAction> spActionRedo = pActionRedo;
            DFG_QT_VERIFY_CONNECT(connect(pActionRedo, &QAction::triggered, this, &ThisClass::redo));
            DFG_QT_VERIFY_CONNECT(connect(undoStackPtr, &QUndoStack::canRedoChanged, pActionRedo, &QAction::setEnabled));
            DFG_QT_VERIFY_CONNECT(connect(undoStackPtr, &QUndoStack::redoTextChanged, [=](const QString& s) { setRedoActionText(spActionRedo.data(), s); }));
            pActionRedo->setEnabled(undoStackPtr->canRedo());
            pActionRedo->setShortcuts(QKeySequence::Redo);
            DFG_QT_VERIFY_CONNECT(connect(this, &CsvTableView::sigReadOnlyModeChanged, pActionRedo, &QAction::setDisabled));
            insertAction(pAddBefore, pActionRedo);
        }

        // Undo menu
        {
            auto pAction = new QAction(tr("Undo details"), this);
            setActionFlags(pAction, ActionFlags::defaultContentEdit);

            auto pMenu = new QMenu();
            // Schedule destruction of menu with the parent action.
            DFG_QT_VERIFY_CONNECT(connect(pAction, &QObject::destroyed, pMenu, [=]() { delete pMenu; }));

            // Add 'enable undo' -action
            {
                auto pActionUndoEnableDisable = new QAction(tr(gszMenuText_enableUndo), this);
                setActionFlags(pActionUndoEnableDisable, ActionFlags::viewEdit);
                pActionUndoEnableDisable->setCheckable(true);
                pActionUndoEnableDisable->setChecked(m_bUndoEnabled);
                DFG_QT_VERIFY_CONNECT(connect(pActionUndoEnableDisable, &QAction::toggled, this, &ThisClass::setUndoEnabled));
                pMenu->addAction(pActionUndoEnableDisable);
            }

            // Add Clear undo buffer -action
            {
                auto pActionClearUndoBuffer = new QAction(tr(gszMenuText_clearUndoBuffer), this);
                setActionFlags(pActionClearUndoBuffer, ActionFlags::viewEdit);
                DFG_QT_VERIFY_CONNECT(connect(pActionClearUndoBuffer, &QAction::triggered, this, &ThisClass::clearUndoStack));
                pMenu->addAction(pActionClearUndoBuffer);
            }

            // Add show undo window -action
            {
                auto pActionUndoWindow = new QAction(tr(gszMenuText_showUndoWindow), this);
                setActionFlags(pActionUndoWindow, ActionFlags::defaultContentEdit); // Just showing the window is not yet editing, but action is considered as such since it can also be used for editing.
                DFG_QT_VERIFY_CONNECT(connect(pActionUndoWindow, &QAction::triggered, this, &ThisClass::showUndoWindow));
                pMenu->addAction(pActionUndoWindow);
            }

            pAction->setMenu(pMenu); // Does not transfer ownership.
            addAction(pAction);
        }
    }
}

void CsvTableView::setExternalUndoStack(QUndoStack* pUndoStack)
{
    if (!m_spUndoStack)
        createUndoStack();

    m_spUndoStack->setRef(pUndoStack);

    // Find and remove undo&redo actions from action list since they use the old undostack...
    auto acts = actions();
    std::vector<QAction*> removeList;
    QAction* pAddNewBefore = nullptr;
    for (auto iter = acts.begin(); iter != acts.end(); ++iter)
    {
        auto pAction = *iter;
        if (!pAction)
            continue;
        if (pAction->shortcut() == QKeySequence::Undo)
            removeList.push_back(pAction);
        else if (pAction->shortcut() == QKeySequence::Redo)
            removeList.push_back(pAction);
        else if (pAction->text() == tr(gszMenuText_clearUndoBuffer))
        {
            removeList.push_back(pAction);
            if (iter + 1 != acts.end())
                pAddNewBefore = *(iter + 1);
        }
    }
    for (auto iter = removeList.begin(); iter != removeList.end(); ++iter)
        removeAction(*iter);

    // ... and add the new undo&redo actions that refer to the new undostack.
    privAddUndoRedoActions(pAddNewBefore);
}

void CsvTableView::contextMenuEvent(QContextMenuEvent* pEvent)
{
    DFG_UNUSED(pEvent);

    QMenu menu;
    menu.addActions(actions());
    menu.exec(QCursor::pos());

    //BaseClass::contextMenuEvent(pEvent);
    /*
    QMenu menu;

    auto actionDelete_current_column = new QAction(this);
    actionDelete_current_column->setObjectName(QStringLiteral("actionDelete_current_column"));
    auto actionRename_column = new QAction(this);
    actionRename_column->setObjectName(QStringLiteral("actionRename_column"));
    auto actionMove_column_left = new QAction(this);
    actionMove_column_left->setObjectName(QStringLiteral("actionMove_column_left"));
    auto actionMove_column_right = new QAction(this);
    actionMove_column_right->setObjectName(QStringLiteral("actionMove_column_right"));
    auto actionCopy_column = new QAction(this);
    actionCopy_column->setObjectName(QStringLiteral("actionCopy_column"));
    auto actionPaste_column = new QAction(this);
    actionPaste_column->setObjectName(QStringLiteral("actionPaste_column"));
    auto actionDelete_selected_row_s = new QAction(this);
    actionDelete_selected_row_s->setObjectName(QStringLiteral("actionDelete_selected_row_s"));

    actionRename_column->setText(QApplication::translate("MainWindow", "Rename column", 0, QApplication::UnicodeUTF8));
    actionRename_column->setShortcut(QApplication::translate("MainWindow", "Ctrl+R", 0, QApplication::UnicodeUTF8));
    actionMove_column_left->setText(QApplication::translate("MainWindow", "Move column left", 0, QApplication::UnicodeUTF8));
    actionMove_column_left->setShortcut(QApplication::translate("MainWindow", "Alt+Left", 0, QApplication::UnicodeUTF8));
    actionMove_column_right->setText(QApplication::translate("MainWindow", "Move column right", 0, QApplication::UnicodeUTF8));
    actionMove_column_right->setShortcut(QApplication::translate("MainWindow", "Alt+Right", 0, QApplication::UnicodeUTF8));
    actionCopy_column->setText(QApplication::translate("MainWindow", "Copy column", 0, QApplication::UnicodeUTF8));
    actionCopy_column->setShortcut(QApplication::translate("MainWindow", "Ctrl+D", 0, QApplication::UnicodeUTF8));
    actionPaste_column->setText(QApplication::translate("MainWindow", "Paste column", 0, QApplication::UnicodeUTF8));
    actionPaste_column->setShortcut(QApplication::translate("MainWindow", "Ctrl+Shift+V", 0, QApplication::UnicodeUTF8));

    menu.addAction();

    menu.exec(QCursor::pos());
    */
}

void CsvTableView::dragEnterEvent(QDragEnterEvent* pEvent)
{
    if (pEvent && !getAcceptableFilePathFromMimeData(pEvent->mimeData()).isEmpty())
    {
        pEvent->acceptProposedAction();
        pEvent->setAccepted(true);
    }
}

void CsvTableView::dragMoveEvent(QDragMoveEvent* pEvent)
{
    DFG_UNUSED(pEvent);
    // Not calling base class implementation as that would pass mimedata to canDropMimeData() of CsvItemModel
    // which by default returns false effectively disabling drag&drop.
    //BaseClass::dragMoveEvent(pEvent);
}

void CsvTableView::dropEvent(QDropEvent* pEvent)
{
    auto pMimeData = (pEvent) ? pEvent->mimeData() : nullptr;
    const auto sPath = getAcceptableFilePathFromMimeData(pMimeData);
    if (sPath.isEmpty())
        return;

    if (this->getProceedConfirmationFromUserIfInModifiedState(tr("open file\n%1").arg(sPath)))
        this->openFile(sPath);
}

void CsvTableView::mousePressEvent(QMouseEvent* event)
{
    // When clicking item, disabling autoscroll temporarily to avoid unwanted timer-based ensureVisible-functionality from kicking in (#136).
    // For details, see d->delayedAutoScroll in QAbstractItemView
    // Note that can't set autoScroll completely off as that would disable e.g. autoscroll when selecting items with mouse drag.
    const auto bAutoScroll = hasAutoScroll();
    if (bAutoScroll)
        setAutoScroll(false);
    BaseClass::mousePressEvent(event);
    if (bAutoScroll)
        setAutoScroll(true);
}

void CsvTableView::setModel(QAbstractItemModel* pModel)
{
    const QAbstractItemModel* pPreviousViewModel = model();
    const CsvModel* pPreviousCsvModel = csvModel();
    if (pPreviousCsvModel)
    {
        DFG_VERIFY(disconnect(pPreviousCsvModel, &CsvModel::sigOnNewSourceOpened, this, &ThisClass::onNewSourceOpened));
    }
    if (pPreviousViewModel)
        DFG_VERIFY(disconnect(pPreviousViewModel, &CsvModel::dataChanged, this, &ThisClass::onViewModelDataChanged));
    BaseClass::setModel(pModel);
    auto pCsvModel = csvModel();
    if (m_spUndoStack && pCsvModel)
        pCsvModel->setUndoStack(&m_spUndoStack->item());
    if (pCsvModel)
    {
        // From Qt documentation: "Note: Qt::UniqueConnections do not work for lambdas, non-member functions and functors; they only apply to connecting to member functions"
        DFG_QT_VERIFY_CONNECT(connect(pCsvModel, &CsvModel::sigOnNewSourceOpened, this, &ThisClass::onNewSourceOpened, Qt::UniqueConnection));
    }
    if (pModel)
        DFG_QT_VERIFY_CONNECT(connect(pModel, &CsvModel::dataChanged, this, &ThisClass::onViewModelDataChanged, Qt::UniqueConnection));
    DFG_QT_VERIFY_CONNECT(connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &ThisClass::onSelectionModelChanged));

    // If model is of type QSortFilterProxyModel, updating checked-status of sort case sensitivity.
    auto pNewSortModel = qobject_cast<QSortFilterProxyModel*>(pModel);
    if (pNewSortModel)
    {
        if (DFG_OPAQUE_REF().m_spActSortCaseSensitivity)
        {
            const auto sortCaseSensitivity = pNewSortModel->sortCaseSensitivity();
            DFG_OPAQUE_REF().m_spActSortCaseSensitivity->setChecked(sortCaseSensitivity == Qt::CaseSensitivity::CaseSensitive);
        }
    }
}

namespace
{
    template <class CsvModel_T, class ProxyModel_T, class Model_T>
    CsvModel_T* csvModelImpl(Model_T* pModel)
    {
        auto pCsvModel = qobject_cast<CsvModel_T*>(pModel);
        if (pCsvModel)
            return pCsvModel;
        auto pProxyModel = qobject_cast<ProxyModel_T*>(pModel);
        return (pProxyModel) ? qobject_cast<CsvModel_T*>(pProxyModel->sourceModel()) : nullptr;
    }
} // unnamed namespace

auto CsvTableView::csvModel() -> CsvModel*
{
    return csvModelImpl<CsvModel, QAbstractProxyModel>(model());
}

auto CsvTableView::csvModel() const -> const CsvModel*
{
   return csvModelImpl<const CsvModel, const QAbstractProxyModel>(model());
}

int CsvTableView::getFirstSelectedViewRow() const
{
    auto selection = getSelection();
    auto pModel = model();
    const auto nRowCount = (pModel) ? pModel->rowCount() : 0;
    ::DFG_MODULE_NS(func)::MemFuncMin<int> minRow(nRowCount);
    for (auto iter = selection.cbegin(); iter != selection.cend(); ++iter)
    {
        minRow(iter->top());
    }

    return minRow.value();
}

std::vector<int> CsvTableView::getRowsOfCol(const int nCol, const QAbstractProxyModel* pProxy) const
{
    auto pModel = model();
    std::vector<int> vec(static_cast<size_t>((pModel) ? pModel->rowCount() : 0));
    if (!pProxy)
    {
        DFG_MODULE_NS(alg)::generateAdjacent(vec, 0, 1);
    }
    else
    {
        for (int i = 0; i<int(vec.size()); ++i)
            vec[static_cast<size_t>(i)] = pProxy->mapToSource(pProxy->index(i, nCol)).row();
    }
    return vec;
}

QModelIndexList CsvTableView::selectedIndexes() const
{
    DFG_ASSERT_WITH_MSG(false, "Avoid using selectedIndexes() as it's behaviour is unclear when using proxies: selected indexes of proxy or underlying model?");
    return BaseClass::selectedIndexes();
}

int CsvTableView::getRowCount_dataModel() const
{
    auto pModel = this->csvModel();
    return (pModel) ? pModel->getRowCount() : 0;
}

int CsvTableView::getRowCount_viewModel() const
{
    auto pModel = this->model();
    return (pModel) ? pModel->rowCount() : 0;
}

QModelIndexList CsvTableView::getSelectedItemIndexes_dataModel() const
{
    auto pSelectionModel = selectionModel();
    if (!pSelectionModel)
        return QModelIndexList();
    auto selected = pSelectionModel->selectedIndexes();
    if (selected.isEmpty())
        return QModelIndexList();
    auto pProxy = getProxyModelPtr();
    // Map indexes to underlying model. For unknown reason the indexes returned by selection model
    // seem to be sometimes from proxy and sometimes from underlying.
    if (pProxy && selected.front().model() == pProxy)
        std::transform(selected.begin(), selected.end(), selected.begin(), [=](const QModelIndex& index) { return pProxy->mapToSource(index); });
    return selected;
}

QModelIndexList CsvTableView::getSelectedItemIndexes_viewModel() const
{
    auto indexes = getSelectedItemIndexes_dataModel();
    auto pProxy = getProxyModelPtr();
    if (pProxy)
        std::transform(indexes.begin(), indexes.end(), indexes.begin(), [=](const QModelIndex& index) { return pProxy->mapFromSource(index); });
    return indexes;
}

std::vector<int> CsvTableView::getRowsOfSelectedItems(const QAbstractProxyModel* pProxy) const
{
    QModelIndexList listSelected = (!pProxy) ? getSelectedItemIndexes_viewModel() : getSelectedItemIndexes_dataModel();

    ::DFG_MODULE_NS(cont)::SetVector<int> rows;
    for (const auto& listIndex : qAsConst(listSelected))
    {
        if (listIndex.isValid())
            rows.insert(listIndex.row());
    }
    return std::move(rows.m_storage);
}

void CsvTableView::invertSelection()
{
    BaseClass::invertSelection();
}

bool CsvTableView::isRowMode() const
{
    return (selectionBehavior() == QAbstractItemView::SelectRows);
}

void CsvTableView::setRowMode(const bool b)
{
    setSelectionBehavior((b) ? QAbstractItemView::SelectRows : QAbstractItemView::SelectItems);
}

void CsvTableView::setUndoEnabled(const bool bEnable)
{
    auto pCsvModel = csvModel();
    clearUndoStack();
    m_bUndoEnabled = bEnable;
    if (!pCsvModel)
        return;
    if (bEnable)
    {
        if (m_spUndoStack)
            pCsvModel->setUndoStack(&m_spUndoStack->item());
    }
    else
    {
        pCsvModel->setUndoStack(nullptr);
    }
}

bool CsvTableView::isReadOnlyMode() const
{
    return DFG_OPAQUE_PTR() && DFG_OPAQUE_PTR()->m_flags.test(CsvTableViewFlag::readOnly);
}

namespace DFG_DETAIL_NS
{
    void handleActionForReadOnly(QAction* pAction, const bool bReadOnly)
    {
        if (!pAction)
            return;
        const auto type = getActionType(pAction);
        if (type & ActionFlags::anyContentEdit)
        {
            pAction->setVisible(!bReadOnly); // Note: setVisible(false) implies that action is not accesssible at all, e.g. through shortcut (i.e. action is effectively disabled)
        }

        // If action has menu, going through menu actions as well.
        auto pMenu = pAction->menu();
        if (pMenu)
        {
            for (auto pMenuAction : pMenu->actions())
                handleActionForReadOnly(pMenuAction, bReadOnly);
        }
    }
} // namespace DFG_DETAIL_NS

void CsvTableView::setReadOnlyMode(const bool bReadOnly)
{
    auto& rOpaq = DFG_OPAQUE_REF();
    if (rOpaq.m_flags.test(CsvTableViewFlag::readOnly) == bReadOnly)
        return; // Requested flag is effective already.

    const auto handleActionList = [=](const QList<QAction*>& actions)
    {
        for (const auto pAction : actions)
        {
            ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::handleActionForReadOnly(pAction, bReadOnly);
        }
    };

    if (bReadOnly)
    {
        auto lockReleaser = this->tryLockForEdit(); // Maybe not needed, but just to make sure that there are no ongoing operations that might not play nicely with setting read-only mode.
        if (!lockReleaser.isLocked())
        {
            showStatusInfoTip(tr("Unable to enable read-only mode: there seems to be pending operations"));
            if (rOpaq.m_spActReadOnly)
                rOpaq.m_spActReadOnly->setChecked(false);
            return;
        }
        // Storing old edit triggers and disabling all triggers.
        rOpaq.m_editTriggers = this->editTriggers();
        this->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // Setting slightly gray background in read-only mode
        {
            auto newPalette = this->palette();
            rOpaq.m_readWriteModePalette = newPalette;
            newPalette.setColor(QPalette::Base, this->getReadOnlyBackgroundColour());
            //newPalette.setColor(QPalette::Window, QColor(0, 0, 255)); // This seems to affect only the space below row headers.
            this->setPalette(newPalette);
        }

        // Hiding edit actions from context menu
        handleActionList(this->actions());
    }
    else // Case: setting read-write mode
    {
        // Restoring edit triggers.
        this->setEditTriggers(rOpaq.m_editTriggers);
        // Restoring old palette
        this->setPalette(rOpaq.m_readWriteModePalette);
        // Showing and enabling edit actions.
        handleActionList(this->actions());
    }

    // Enabling/disabling undo buffer windows so that they can't be used to trigger undo
    {
        const auto undoWidgets = this->findChildren<UndoViewWidget*>();
        for (auto pUndoWidget : undoWidgets) if (pUndoWidget)
            pUndoWidget->setDisabled(bReadOnly);
    }

    rOpaq.m_flags.set(CsvTableViewFlag::readOnly, bReadOnly);
    if (rOpaq.m_spActReadOnly)
        rOpaq.m_spActReadOnly->setChecked(bReadOnly);
    Q_EMIT sigReadOnlyModeChanged(bReadOnly);
}

template <class Str_T>
void CsvTableView::setReadOnlyModeFromProperty(const Str_T& s)
{
    if (s == "readOnly")
        this->setReadOnlyMode(true);
    else if (s == "readWrite")
        this->setReadOnlyMode(false);
}

void CsvTableView::insertGeneric(const QString& s, const QString& sOperationUiName)
{
    executeAction<CsvTableViewActionFill>(this, s, sOperationUiName);
}

auto CsvTableView::insertDate() -> QDate
{
    const auto date = QDate::currentDate();
    insertGeneric(dateTimeToString(date, getCsvModelOrViewProperty<CsvTableViewPropertyId_dateFormat>(this)), tr("Insert date"));
    return date;
}

auto CsvTableView::insertTime() -> QTime
{
    const auto t = QTime::currentTime();
    insertGeneric(dateTimeToString(t, getCsvModelOrViewProperty<CsvTableViewPropertyId_timeFormat>(this)), tr("Insert time"));
    return t;
}

auto CsvTableView::insertDateTime() -> QDateTime
{
    const auto dt = QDateTime::currentDateTime();
    insertGeneric(dateTimeToString(dt, getCsvModelOrViewProperty<CsvTableViewPropertyId_dateTimeFormat>(this)), tr("Insert DateTime"));
    return dt;
}

bool CsvTableView::saveToFileImpl(const CsvFormatDefinition& formatDef)
{
    auto sPath = QFileDialog::getSaveFileName(this,
        tr("Save file"),
        QString()/*dir*/,
        tr("CSV file (*.csv);;SQLite3 file (*.sqlite3);;All files (*.*)"),
        nullptr/*selected filter*/,
        QFileDialog::Options() /*options*/);

    if (sPath.isEmpty())
        return false;
    return saveToFileImpl(sPath, formatDef);
}

bool CsvTableView::saveToFileImpl(const QString& path, const CsvFormatDefinition& formatDef)
{
    QFileInfo fileInfo(path);
    if (fileInfo.exists() && !fileInfo.isWritable())
    {
        QMessageBox::warning(nullptr, tr("Save failed"), tr("Target path has existing file that can't be written to (read-only file?)\n\n%1").arg(path));
        return false;
    }

    auto lockReleaser = tryLockForRead();
    if (!lockReleaser.isLocked())
    {
        QMessageBox::information(
            this,
            tr("Saving failed"),
            tr("Couldn't save document: a write operation was in progress.")
        );
        return false;
    }

    const bool bSaveAsSqlite = (fileInfo.suffix() == QLatin1String("sqlite3"));
    const bool bSaveAsShown = (formatDef.getProperty(s_szCsvSaveOption_saveAsShown, "") == "1");

    auto pModel = csvModel();
    if (!pModel)
        return false;

    // Checking if saving would (silently) change some format aspects in existing file. Currently the following items are checked:
    //      -Encoding (e.g. existing file is UTF16 but would be saved as UTF8)
    //      -EOL (e.g. existing file has \r\n but would be saved with \n)
    // If changes are detected, user is asked whether to continue saving.
    if (!bSaveAsSqlite)
    {
        QString sFormatChangeNotification;

        const auto sExistingPath = pModel->getFilePath();
        if (!sExistingPath.isEmpty() && sExistingPath == path && QFileInfo::exists(sExistingPath))
        {
            const auto existingCsvFormat = CsvItemModel::peekCsvFormatFromFile(sExistingPath);

            const auto existingEncoding = existingCsvFormat.textEncoding();
            if (existingEncoding != DFG_MODULE_NS(io)::TextEncoding::encodingUnknown && existingEncoding != formatDef.textEncoding())
            {
                sFormatChangeNotification = QString("<li>%1</li>").arg(tr("Encoding from '%1' to '%2'").
                    arg(untypedViewToQStringAsUtf8(::DFG_MODULE_NS(io)::encodingToStrId(existingEncoding)),
                        untypedViewToQStringAsUtf8(formatDef.textEncodingAsString())
                    ).toHtmlEscaped());
            }
            const auto existingEol = existingCsvFormat.eolType();
            if (existingEol != formatDef.eolType())
            {
                sFormatChangeNotification += QString("<li>%1</li>").arg(tr("end-of-line from '%1' to '%2'").
                    arg(untypedViewToQStringAsUtf8(::DFG_MODULE_NS(io)::eolLiteralStrFromEndOfLineType(existingEol)),
                        untypedViewToQStringAsUtf8(formatDef.eolTypeAsString())
                    ).toHtmlEscaped());
            }
        }
        if (!sFormatChangeNotification.isEmpty())
        {
            if (QMessageBox::Yes != QMessageBox::question(
                this,
                tr("Confirm format change"),
                tr("Saving to file<br>"
                    "%1<br>"
                    "would change the following file format properties compared to existing file:"
                    "<ul>%2</ul>"
                    "Continue?<br><br>"
                    "Note: only encoding and end-of-line changes are detected.").arg(path.toHtmlEscaped(), sFormatChangeNotification)))
                return false;
        }
    }

    CsvItemModel saveAsShownModel; // Temporary model used if saving as shown.

    if (bSaveAsShown)
    {
        // Creating new CsvItemModel from shown and saving that.
        // This wastes resources as some table content needs to be duplicated, but allows use of existing saving machinery.
        auto pViewModel = this->model();
        if (!pViewModel)
            return false;
        const auto nRowCount = pViewModel->rowCount();
        const auto nColCount = pViewModel->columnCount();
        saveAsShownModel.insertRows(0, nRowCount);
        saveAsShownModel.insertColumns(0, nColCount);
        using Index = std::remove_const<decltype(nRowCount)>::type;
        for (Index c = 0; c < nColCount; ++c)
        {
            for (Index r = 0; r < nRowCount; ++r)
            {
                const auto viewIndex = pViewModel->index(r, c);
                const auto sourceIndex = mapToDataModel(viewIndex);
                const auto pStr = pModel->rawStringPtrAt(sourceIndex);
                saveAsShownModel.setDataNoUndo(r, c, pStr);
            }
        }
        pModel = &saveAsShownModel;
    }

    if (bSaveAsSqlite && QMessageBox::question(this, tr("SQLite export"), tr("SQlite export is rudimentary, continue anyway?")) != QMessageBox::Yes)
        return false;
        

    bool bSuccess = false;
    doModalOperation(tr("Saving to file\n%1").arg(path), ProgressWidget::IsCancellable::no, "CsvTableViewFileWriter", [&](ProgressWidget*)
        {
            // TODO: allow user to cancel saving (e.g. if it takes too long)
            if (bSaveAsSqlite)
                bSuccess = pModel->exportAsSQLiteFile(path, formatDef);
            else
                bSuccess = pModel->saveToFile(path, formatDef);
        });

    if (bSuccess && bSaveAsSqlite)
        QMessageBox::information(this, tr("SQLite export"), tr("Successfully exported table to SQLite file\n%1").arg(path));
    else
    {
        QString sAdditionalDetails = (!pModel->m_messagesFromLatestSave.isEmpty())
            ? QString("\n\nThe following message(s) were generated:\n%1").arg(pModel->m_messagesFromLatestSave.join('\n'))
            : QString();
        if (bSuccess && !sAdditionalDetails.isEmpty())
            QMessageBox::information(nullptr, tr("Save messages"), tr("Saving to path\n%1%2").arg(path, sAdditionalDetails));
        else if (!bSuccess)
            QMessageBox::warning(nullptr, tr("Save failed"), tr("Failed to save to path\n%1%2").arg(path, sAdditionalDetails));
    }

    return bSuccess;
}


bool CsvTableView::save()
{
    auto model = csvModel();
    const auto& path = (model) ? model->getFilePath() : QString();
    if (!path.isEmpty())
        return saveToFileImpl(path, model->getSaveOptions());
    else
        return saveToFile();
}

bool CsvTableView::saveToFile()
{
    auto pModel = csvModel();
    return (pModel) ? saveToFileImpl(pModel->getSaveOptions()) : false;
}

namespace DFG_DETAIL_NS {

const char szContentFilterHelpText[] =
    QT_TR_NOOP("List of single line JSON-strings defining list of filters\n"
               "Example: { \"text\":\"abc\", \"apply_columns\":\"2\" }\n"
               "Hint: filters can also be defined in .conf-file with key 'properties/readFilters'\n\n"
               "The following fields are available:\n"
               "    text: \"<filter text>\"\n"
               "        Defines actual filter text whose meaning depends on filter type\n"
               "        Default: empty\n"
               "    type: {wildcard, fixed, reg_exp}\n"
               "        Defines filter type. 'fixed' is a search string as such without any special character interpretations and 'reg_exp' is based on QRegularExpression.\n"
               "        Default: wildcard\n"
               "    case_sensitive: {true, false}\n"
               "        Defines whether this filter is case sensitive or not.\n"
               "        Default: false (=case insensitive)\n"
               "    apply_columns: List of columns in IntervalSet syntax\n"
               "        Defines columns where the filter is applied, first column is 1.\n"
               "        Default: Empty (=match any column)\n"
               "    apply_rows: List of rows in IntervalSet syntax\n"
               "        Defines rows on which filter is applied, header is row 0.\n"
               "        Default: All rows expect header\n"
               "    and_group: arbitrary string identifier\n"
               "        Defines filter group for filters that are AND'ed; different and-groups are OR'ed.\n"
               "        Default: Filter belongs to default and-group.\n"
              );
} // namespace DFG_DETAIL_NS

class CsvFormatDefinitionDialog : public QDialog
{
public:
    typedef QDialog BaseClass;
    enum DialogType { DialogTypeSave, DialogTypeLoad };

    typedef CsvItemModel::LoadOptions LoadOptions;
    typedef CsvItemModel::SaveOptions SaveOptions;

    CsvFormatDefinitionDialog(CsvTableView* pParent, const DialogType dialogType, const CsvTableView::CsvModel* pModel, const QString& sFilePath = QString())
        : BaseClass(pParent)
        , m_dialogType(dialogType)
        , m_loadOptions(pModel)
        , m_saveOptions(pModel)
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(io);
        removeContextHelpButtonFromDialog(this);
        auto spLayout = std::unique_ptr<QFormLayout>(new QFormLayout);
        m_spSeparatorEdit.reset(new QComboBox(this));
        m_spEnclosingEdit.reset(new QComboBox(this));
        m_spEolEdit.reset(new QComboBox(this));
        m_spEncodingEdit.reset(new QComboBox(this));
        if (isSaveDialog())
        {
            m_spEnclosingOptions.reset(new QComboBox(this));
            m_spSaveHeader.reset(new QCheckBox(this));
            m_spWriteBOM.reset(new QCheckBox(this));
            m_spSaveAsShown.reset(new QCheckBox(this));
            m_spSaveAsShown->setToolTip(tr("If selected, currently shown table is saved instead of the underlying table.\nThis can be used e.g. to save filtered and sorted snapshot of the whole table"));
        }
        else
        {
            m_spCompleterColumns.reset(new QLineEdit(this));
            m_spIncludeRows.reset(new QLineEdit(this));
            m_spIncludeColumns.reset(new QLineEdit(this));
            m_spThreadCountMax.reset(new QLineEdit(this));
            m_spThreadReadBlockMin.reset(new QLineEdit(this));
            m_spContentFilterWidget.reset(new JsonListWidget(this));
        }

        const auto defaultFormatSettings = (isSaveDialog()) ? m_saveOptions : CsvTableView::CsvModel::peekCsvFormatFromFile(sFilePath);
        const auto confFileOptions = CsvItemModel::getLoadOptionsForFile(sFilePath, pModel);

        // Separator
        {
            m_spSeparatorEdit->addItems(QStringList() << "\\x1f" << "," << "\\t" << ";");
            if (isSaveDialog())
            {
                // When populating save dialog, default-select separator that is defined in model options.
                addCurrentOptionToCombobox(*m_spSeparatorEdit, defaultFormatSettings.separatorChar());
            }
            else // Case load dialog: adding peeked separator
                addCurrentOptionToCombobox(*m_spSeparatorEdit, defaultFormatSettings.separatorChar());
            m_spSeparatorEdit->setEditable(true);
        }

        // Enclosing char
        m_spEnclosingEdit->addItems(QStringList() << "\"" << "");
        if (isSaveDialog())
        {
            // When populating save dialog, default-select encloser that is defined in model options.
            addCurrentOptionToCombobox(*m_spEnclosingEdit, defaultFormatSettings.enclosingChar());
        }
        m_spEnclosingEdit->setEditable(true);

        // EOL
        m_spEolEdit->addItems(QStringList() << "\\n" << "\\r" << "\\r\\n");
        {
            addCurrentOptionToCombobox(*m_spEolEdit, untypedViewToQStringAsUtf8(defaultFormatSettings.eolTypeAsString()));
        }
        m_spEolEdit->setEditable(false);

        if (isSaveDialog())
        {
            m_spEnclosingOptions->addItem(tr("Only when needed"), static_cast<int>(EbEncloseIfNeeded));
            m_spEnclosingOptions->addItem(tr("Every non-empty cell"), static_cast<int>(EbEncloseIfNonEmpty));

            m_spEncodingEdit->addItems(QStringList() << encodingToStrId(encodingUTF8) << encodingToStrId(encodingLatin1));
            addCurrentOptionToCombobox(*m_spEncodingEdit, encodingToStrId(defaultFormatSettings.textEncoding()));
            m_spEncodingEdit->setEditable(false);

            m_spSaveHeader->setChecked(true);

            m_spWriteBOM->setChecked(true);
        }
        else // Case: load dialog
        {
            m_spEncodingEdit->addItems(QStringList() << tr("auto")
                                                     << encodingToStrId(encodingUTF8)
                                                     << encodingToStrId(encodingWindows1252)
                                                     << encodingToStrId(encodingLatin1));
        }

        spLayout->addRow(tr("Separator char"), m_spSeparatorEdit.get());
        spLayout->addRow(tr("Enclosing char"), m_spEnclosingEdit.get());
        if (isSaveDialog())
        {
            spLayout->addRow(tr("Enclosing behaviour"), m_spEnclosingOptions.get());
        }
        spLayout->addRow(tr("End-of-line"), m_spEolEdit.get());
        spLayout->addRow(tr("Encoding"), m_spEncodingEdit.get());
        if (isSaveDialog())
        {
            spLayout->addRow(tr("Save header"), m_spSaveHeader.get());
            spLayout->addRow(tr("Write BOM"), m_spWriteBOM.get());
            spLayout->addRow(tr("Save as shown"), m_spSaveAsShown.get());
        }
        else // Case: load dialog
        {
            // Completer columns
            spLayout->addRow(tr("Completer columns"), m_spCompleterColumns.get());
            CsvItemModel::setCompleterHandlingFromInputSize(m_loadOptions, DFG_MODULE_NS(os)::fileSize(qStringToFileApi8Bit(sFilePath)), nullptr);
            m_spCompleterColumns->setText(m_loadOptions.getProperty(CsvOptionProperty_completerColumns, "*").c_str());
            m_spCompleterColumns->setToolTip(tr("Comma-separated list of column indexes (1-based index) where completion is available, use * to enable on all."));

            // Include rows
            spLayout->addRow(tr("Include rows"), m_spIncludeRows.get());
            DFG_REQUIRE(m_spIncludeRows != nullptr);
            m_spIncludeRows->setPlaceholderText(tr("all"));
            m_spIncludeRows->setToolTip(tr("Defines rows to include from file, all when empty. If filtered table should have header from the original table, include row 0 in the filter\
                                            <br>Syntax example: to include header and rows 4, 7 and 10-20: <i>0; 4; 7; 10:20</i>"));

            // Include columns
            spLayout->addRow(tr("Include columns"), m_spIncludeColumns.get());
            DFG_REQUIRE(m_spIncludeColumns != nullptr);
            m_spIncludeColumns->setPlaceholderText(tr("all"));
            m_spIncludeColumns->setToolTip(tr("Defines columns to include from file, all when empty.\
                                              <br>Syntax example: to include first, second and fourth column: <i>1:2; 4</i>"));

            // Threading options
            {
                spLayout->addRow(tr("Maximum read thread count"), m_spThreadCountMax.get());
                spLayout->addRow(tr("Minimum read thread block size (in MB)"), m_spThreadReadBlockMin.get());
                DFG_REQUIRE(m_spThreadCountMax != nullptr);
                DFG_REQUIRE(m_spThreadReadBlockMin != nullptr);
                m_spThreadCountMax->setPlaceholderText(tr("auto"));
                const auto confThreadCount = confFileOptions.getPropertyT<LoadOptions::PropertyId::readOpt_threadCount>(0);
                if (confThreadCount != 0)
                    m_spThreadCountMax->setText(QString::number(confThreadCount));
                
                m_spThreadReadBlockMin->setText(QString::number(confFileOptions.getPropertyT<LoadOptions::PropertyId::readOpt_threadBlockSizeMinimum>(10000000) / 1000000.0));
            }

            // Content filters
            spLayout->addRow(tr("Content filters"), m_spContentFilterWidget.get());
            DFG_REQUIRE(m_spContentFilterWidget != nullptr);
            const auto loadOptions = CsvItemModel::getLoadOptionsForFile(sFilePath, pModel);
            const auto readFilters = loadOptions.getProperty(CsvOptionProperty_readFilters, "");
            m_spContentFilterWidget->setPlaceholderText(tr("List of filters, see tooltip for syntax guide"));
            if (!readFilters.empty())
                m_spContentFilterWidget->setPlainText(QString::fromUtf8(readFilters.c_str()));

            m_spContentFilterWidget->setToolTip(::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::szContentFilterHelpText);
            m_spContentFilterWidget->setMaximumHeight(100);
        }

        spLayout->addRow(new QLabel(tr("Notes"), this),
            new QLabel(tr("%1Threaded reading requires empty enclosing char and\n%2UTF8 / Latin1 / Windows-1252 encoding\n"
                          "%1Best read performance is achieved with empty enclosing char,\n%2UTF8 encoding and no completer columns"
                         ).arg(QString("%1 ").arg(QChar(0x2022)), "   "), this)); // 0x2022 is code point of bullet. Not using <li> since it introduced unwanted margins.


        spLayout->addRow(QString(), addOkCancelButtonBoxToDialog(this));

        setLayout(spLayout.release());
    }

    static void addCurrentOptionToCombobox(QComboBox& cb, QString sSelection)
    {
        if (sSelection == "\\x9")
            sSelection = "\\t";
        else if (sSelection == "\n")
            sSelection = "\\n";
        else if (sSelection == "\r")
            sSelection = "\\r";
        else if (sSelection == "\r\n")
            sSelection = "\\r\\n";

        const auto nIndex = cb.findText(sSelection);
        if (nIndex != -1)
            cb.setCurrentIndex(nIndex);
        else
        {
            // Selection wasn't present in predefined list -> add and select it.
            cb.addItem(sSelection);
            cb.setCurrentIndex(cb.count() - 1);
        }
    }

    static void addCurrentOptionToCombobox(QComboBox& cb, const int nSelection)
    {
        const bool bPrintable = QChar::isPrint(DFG_ROOT_NS::saturateCast<unsigned int>(nSelection));
        QString sSelection;
        if (bPrintable)
            sSelection = QString(QChar(nSelection));
        else if (nSelection >= 0)
            sSelection = QString("\\x%1").arg(nSelection, 0, 16);
        // Note: if nSelection is < 0 (e.g. metaCharNone), sSelection will be empty -> adds empty line to combobox.
        addCurrentOptionToCombobox(cb, std::move(sSelection));
    }

    bool isSaveDialog() const
    {
        return m_dialogType == DialogTypeSave;
    }

    bool isLoadDialog() const
    {
        return !isSaveDialog();
    }

    static bool isAcceptableSeparatorOrEnclosingChar(const DFG_ROOT_NS::int32 val, const DFG_MODULE_NS(io)::TextEncoding encoding)
    {
        DFG_UNUSED(encoding);
        // csv parsing doesn't support at least separators that are wider than one base character: parser reads non-ascii code point to a UTF8-array and
        // separator detection compares last base char, not last code point, with given separator code point. So for now only support ASCII.
        return (val >= 0 && val < 128);
    }

    void showInvalidOptionMessage(const QString& sTitle, const QString& sText)
    {
        QMessageBox::information(this, sTitle, sText);
    }

    void accept() override
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(io);
        if (isSaveDialog() && (!m_spSeparatorEdit || !m_spEnclosingEdit || !m_spEnclosingOptions || !m_spEolEdit || !m_spSaveHeader || !m_spWriteBOM || !m_spEncodingEdit || !m_spSaveAsShown))
        {
            QMessageBox::information(this, tr("CSV saving"), tr("Internal error occurred; saving failed."));
            return;
        }
        if (isLoadDialog() && (!m_spSeparatorEdit || !m_spEnclosingEdit || !m_spEolEdit || !m_spCompleterColumns || !m_spEncodingEdit))
        {
            QMessageBox::information(this, tr("CSV loading"), tr("Internal error occurred; loading failed."));
            return;
        }
        auto sSep = m_spSeparatorEdit->currentText().trimmed();
        auto sEnc = m_spEnclosingEdit->currentText().trimmed();
        auto sEol = m_spEolEdit->currentText().trimmed();

        DFG_MODULE_NS(io)::EndOfLineType eolType = DFG_MODULE_NS(io)::EndOfLineTypeNative;

        const auto sep = DFG_MODULE_NS(str)::stringLiteralCharToValue<int32>(sSep.toStdWString());
        const auto enc = DFG_MODULE_NS(str)::stringLiteralCharToValue<int32>(sEnc.toStdWString());

        eolType = DFG_MODULE_NS(io)::endOfLineTypeFromStr(sEol.toStdString());

        // TODO: check for identical values (e.g. require that sep != enc)
        if (!sep.first || (!sEnc.isEmpty() && !enc.first) || eolType == DFG_MODULE_NS(io)::EndOfLineTypeNative)
        {
            // TODO: more informative message for the user.
            QMessageBox::information(this, tr("CSV saving"), tr("Chosen settings can't be used. Please revise the selections."));
            return;
        }

        const auto encoding = strIdToEncoding(m_spEncodingEdit->currentText().toLatin1().data());

        {
            if (!isAcceptableSeparatorOrEnclosingChar(sep.second, encoding))
            {
                QMessageBox::information(this, tr("Unsupported separator"), tr("Unsupported separator character value %1.\nOnly ASCII characters are supported.").arg(sep.second));
                return;
            }
            if (!isAcceptableSeparatorOrEnclosingChar(enc.second, encoding))
            {
                QMessageBox::information(this, tr("Unsupported enclosing char"), tr("Unsupported enclosing character value %1.\nOnly ASCII characters are supported.").arg(enc.second));
                return;
            }
        }

        if (isLoadDialog())
        {
            m_loadOptions.m_cEnc = (!sEnc.isEmpty()) ? enc.second : ::DFG_MODULE_NS(io)::DFG_CLASS_NAME(DelimitedTextReader)::s_nMetaCharNone;
            m_loadOptions.m_cSep = sep.second;
            m_loadOptions.m_eolType = eolType;
            m_loadOptions.setProperty(CsvOptionProperty_completerColumns, m_spCompleterColumns->text().toStdString());
            m_loadOptions.setProperty(CsvOptionProperty_includeRows, m_spIncludeRows->text().toStdString());
            m_loadOptions.setProperty(CsvOptionProperty_includeColumns, m_spIncludeColumns->text().toStdString());
            const auto syntaxCheckResult = m_spContentFilterWidget->checkSyntax();
            if (!syntaxCheckResult.first)
            {
                QMessageBox::information(this, tr("Invalid content filters"), tr("Invalid content filters detected:\n\n%1").arg(JsonListWidget::formatErrorMessage(syntaxCheckResult)));
                return;
            }
            m_loadOptions.setProperty(CsvOptionProperty_readFilters, m_spContentFilterWidget->entriesAsNewLineSeparatedString().toUtf8().data());
            m_loadOptions.textEncoding(encoding);

            // Setting threading options
            {
                DFG_REQUIRE(m_spThreadCountMax != nullptr);
                DFG_REQUIRE(m_spThreadReadBlockMin != nullptr);
                bool bGoodConvert = true, bGoodConvert2 = false;
                const auto sThreadCount = m_spThreadCountMax->text();
                const auto nThreadCount = (!sThreadCount.isEmpty()) ? sThreadCount.toUInt(&bGoodConvert) : 0;
                const auto blockSizeMinMb = round(m_spThreadReadBlockMin->text().toDouble(&bGoodConvert2) * 1000000);
                if (!bGoodConvert)
                {
                    showInvalidOptionMessage(tr("Invalid thread count"), tr("Thread count must be a non-negative integer"));
                    return;
                }
                uint64 nBlockSizeInMb = 0;
                if (!bGoodConvert2 || !::DFG_MODULE_NS(math)::isFloatConvertibleTo(blockSizeMinMb, &nBlockSizeInMb))
                {
                    showInvalidOptionMessage(tr("Invalid thread read block size"), tr("Minimum thread read block size must be a non-negative integer in range of uint64"));
                    return;
                }
                m_loadOptions.setPropertyT<LoadOptions::PropertyId::readOpt_threadCount>(nThreadCount);
                m_loadOptions.setPropertyT<LoadOptions::PropertyId::readOpt_threadBlockSizeMinimum>(nBlockSizeInMb);
            }
        }
        else // case: save dialog
        {
            m_saveOptions.m_cEnc = enc.second;
            m_saveOptions.m_cSep = sep.second;
            m_saveOptions.m_eolType = eolType;
            m_saveOptions.enclosementBehaviour((sEnc.isEmpty()) ? EbNoEnclose : static_cast<EnclosementBehaviour>(m_spEnclosingOptions->currentData().toInt()));
            m_saveOptions.headerWriting(m_spSaveHeader->isChecked());
            m_saveOptions.bomWriting(m_spWriteBOM->isChecked());
            if (m_spSaveAsShown->isChecked())
                m_saveOptions.setProperty(CsvTableView::s_szCsvSaveOption_saveAsShown, "1");
            m_saveOptions.textEncoding(encoding);
        }

        BaseClass::accept();
    }

    LoadOptions getLoadOptions() const
    {
        return m_loadOptions;
    }

    SaveOptions getSaveOptions() const
    {
        return m_saveOptions;
    }


    DialogType m_dialogType;
    LoadOptions m_loadOptions;
    SaveOptions m_saveOptions;
    QObjectStorage<QComboBox> m_spSeparatorEdit;
    QObjectStorage<QComboBox> m_spEnclosingEdit;
    QObjectStorage<QComboBox> m_spEnclosingOptions;
    QObjectStorage<QComboBox> m_spEolEdit;
    QObjectStorage<QComboBox> m_spEncodingEdit;
    QObjectStorage<QCheckBox> m_spSaveHeader;
    QObjectStorage<QCheckBox> m_spWriteBOM;
    QObjectStorage<QCheckBox> m_spSaveAsShown;
    // Load-only properties
    QObjectStorage<QLineEdit> m_spCompleterColumns;
    QObjectStorage<QLineEdit> m_spIncludeRows;
    QObjectStorage<QLineEdit> m_spIncludeColumns;
    QObjectStorage<QLineEdit> m_spThreadCountMax;
    QObjectStorage<QLineEdit> m_spThreadReadBlockMin;
    QObjectStorage<JsonListWidget> m_spContentFilterWidget;
}; // Class CsvFormatDefinitionDialog

bool CsvTableView::saveToFileWithOptions()
{
    CsvFormatDefinitionDialog dlg(this, CsvFormatDefinitionDialog::DialogTypeSave, csvModel());
    if (dlg.exec() != QDialog::Accepted)
        return false;
    return saveToFileImpl(dlg.getSaveOptions());
}

auto CsvTableView::populateCsvConfig() const -> CsvConfig
{
    auto pModel = csvModel();
    return (pModel) ? populateCsvConfig(*pModel) : CsvConfig();
}

auto CsvTableView::populateCsvConfig(const CsvItemModel& rDataModel) const -> CsvConfig
{
    CsvConfig config;

    rDataModel.populateConfig(config);

    // Adding column widths and visibility
    {
        char szBuffer[64];
        const auto formatToBuffer = [&](const char* pszFormat, const int nCol) { ::DFG_MODULE_NS(str)::DFG_DETAIL_NS::sprintf_s(szBuffer, sizeof(szBuffer), pszFormat, nCol); };
        for (int c = 0, nCount = rDataModel.columnCount(); c < nCount; ++c)
        {
            const auto typedColumn = ColumnIndex_data(c);
            const auto viewIndex = columnIndexDataToView(typedColumn);
            const auto nVisibleColumnIndex = CsvModel::internalColumnIndexToVisible(c);
            if (isColumnVisible(typedColumn))
            {
                formatToBuffer("columnsByIndex/%d/width_pixels", nVisibleColumnIndex);
                config.setKeyValue(StringUtf8(SzPtrUtf8(szBuffer)), StringUtf8::fromRawString(DFG_MODULE_NS(str)::toStrC(columnWidth(viewIndex.value()))));
            }
            else // case: column is hidden, storing only visibility flag.
            {
                formatToBuffer("columnsByIndex/%d/visible", nVisibleColumnIndex);
                config.setKeyValue(StringUtf8(SzPtrUtf8(szBuffer)), StringUtf8(DFG_UTF8("0")));
            }
        }
    }

    // Adding read-only mode if enabled
    if (this->isReadOnlyMode())
        config.setKeyValue(qStringToStringUtf8(QString("properties/%1").arg(CsvOptionProperty_editMode)), StringUtf8::fromRawString("readOnly"));

    // Adding additional properties that there might be, in practice this may mean e.g. properties/chartControls
    auto pOpaq = DFG_OPAQUE_PTR();
    if (pOpaq)
    {
        for (const auto& fetcher : pOpaq->m_propertyFetchers)
        {
            if (!fetcher)
                continue;
            const auto& keyValue = fetcher();
            config.setKeyValue(qStringToStringUtf8(keyValue.first), qStringToStringUtf8(keyValue.second));
        }
    }

    // Adding items that are in existing .conf-file, but which were not yet added.
    CsvConfig existingConfig;
    existingConfig.loadFromFile(qStringToFileApi8Bit(CsvFormatDefinition::csvFilePathToConfigFilePath(rDataModel.getFilePath())));
    existingConfig.forEachKeyValue([&](const CsvConfig::StringViewT svKey, const CsvConfig::StringViewT svValue)
    {
        if (!config.contains(svKey) && svKey != DFG_UTF8("properties"))
            config.setKeyValue(svKey.toString(), svValue.toString());
    });

    return config;
}

bool CsvTableView::saveConfigFileTo(const CsvConfig& config, const QString& sPath)
{
    const auto bSuccess = config.saveToFile(qStringToFileApi8Bit(sPath));
    if (!bSuccess)
        QMessageBox::information(this, tr("Saving failed"), tr("Saving config file to path '%1' failed").arg(sPath));

    return bSuccess;
}

auto CsvTableView::askConfigFilePath(CsvItemModel& rModel) -> QString
{
    const auto sModelPath = rModel.getFilePath();
    const auto sPathSuggestion = (sModelPath.isEmpty()) ? QString() : CsvFormatDefinition::csvFilePathToConfigFilePath(sModelPath);

    auto sPath = QFileDialog::getSaveFileName(this,
        tr("Save config file"),
        sPathSuggestion,
        tr("CSV Config file (*.csv.conf);;All files (*.*)"),
        nullptr/*selected filter*/,
        QFileDialog::Options() /*options*/);
    return sPath;
}

bool CsvTableView::saveConfigFile()
{
    auto pModel = csvModel();
    if (!pModel)
        return false;

    const auto sPath = askConfigFilePath(*pModel);

    if (sPath.isEmpty())
        return false;

    const auto config = populateCsvConfig(*pModel);
    
    return saveConfigFileTo(config, sPath);
}

bool CsvTableView::saveConfigFileWithOptions()
{
    auto pModel = csvModel();
    if (!pModel)
        return false;

    auto config = populateCsvConfig(*pModel);

    CsvTableViewDlg viewDlg(nullptr, this, ViewType::fixedDimensionEdit);

    CsvItemModel configModel;

    CsvConfig existingConfig;
    const auto sExistingConfPath = CsvFormatDefinition::csvFilePathToConfigFilePath(pModel->getFilePath());
    const bool bHasExistingConfPath = QFileInfo::exists(sExistingConfPath);
    existingConfig.loadFromFile(qStringToFileApi8Bit(sExistingConfPath));

    configModel.insertRows(0, saturateCast<int>(config.entryCount()));
    configModel.insertColumns(0, (bHasExistingConfPath) ? 3 : 2);
    configModel.setColumnName(0, tr("Key"));
    configModel.setColumnName(1, tr("Value"));
    if (bHasExistingConfPath)
    {
        configModel.setColumnName(2, tr("Existing value"));
        configModel.setColumnProperty(2, CsvItemModelColumnProperty::readOnly, true);
    }

    int nRow = 0;
    config.forEachKeyValue([&](const StringViewUtf8& svKey, const StringViewUtf8& svValue)
    {
        configModel.setItem(nRow, 0, svKey);
        configModel.setItem(nRow, 1, svValue);
        if (bHasExistingConfPath)
        {
            const auto bOldConfigHasKey = existingConfig.contains(svKey);
            const auto existingValue = existingConfig.value(svKey);
            configModel.setItem(nRow, 2, existingValue);
            if (bOldConfigHasKey && existingValue != svValue) // If existing value is different from proposed value, highlighting old value in red.
                configModel.setHighlighter(CsvItemModel::HighlightDefinition(QString("diffHighlighter_%1").arg(nRow), nRow, 2, StringMatchDefinition::makeMatchEverythingMatcher(), QBrush(QColor(255, 0, 0, 64), Qt::SolidPattern)));
        }
        ++nRow;
    });

    viewDlg.setModel(&configModel);

    // Adding additional controls to CsvTableViewDlg
    {
        const auto pParentWidget = &viewDlg.dialog();
        auto pControlWidget = new QWidget(pParentWidget); // Deletion by parentship
        auto pLayout = new QHBoxLayout(pControlWidget);
        pLayout->addWidget(new QLabel(tr("To omit item from saved config, clear related text from Key-column"), pControlWidget));

        // Adding insert-button
        {
            auto pInsertButton = new QToolButton(pParentWidget); // Deletion through parentship
            auto& rButton = *pInsertButton;
            rButton.setPopupMode(QToolButton::InstantPopup);
            rButton.setText(rButton.tr("Insert "));

            auto pMenu = new QMenu(&rButton); // Deletion through parentship

            // Adding items to menu.
            forEachUserInsertableConfFileProperty([&](const DFG_DETAIL_NS::ConfFileProperty& propArg)
                {
                    const auto prop = propArg;
                    auto pAction = new QAction(prop.key(), pInsertButton);
                    if (!pAction)
                        return;
                    const auto clickHandler = [&viewDlg, prop, pMenu, pAction]()
                    {
                        auto pModel = viewDlg.csvView().csvModel();
                        if (!pModel)
                            return;
                        const auto nNewRow = pModel->rowCount();
                        pModel->insertRow(nNewRow);
                        pModel->setData(pModel->index(nNewRow, 0), prop.key());
                        pModel->setData(pModel->index(nNewRow, 1), prop.defaultValue());
                        QTimer::singleShot(0, pMenu, [=]() { pMenu->removeAction(pAction); }); // Removes inserted action from menu
                    };
                    DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, pParentWidget, clickHandler));
                    pMenu->addAction(pAction);
                });
            rButton.setMenu(pMenu); // Does not transfer ownership
            if (!pMenu->actions().empty()) // Adding button to layout only if it has items.
                pLayout->addWidget(&rButton);
        }

        viewDlg.addVerticalLayoutWidget(0, pControlWidget);
    }

    // Launching dialog itself
    viewDlg.resize(600, 500);
    if (viewDlg.exec() != QDialog::Accepted)
        return false;

    config.clear();
    for (int r = 0, nCount = configModel.rowCount(); r < nCount; ++r)
    {
        const auto svKey = configModel.rawStringViewAt(r, 0);
        if (!svKey.empty())
            config.setKeyValue(configModel.rawStringViewAt(r, 0).toString(), configModel.rawStringViewAt(r, 1).toString());
    }

    const auto sPath = askConfigFilePath(*pModel);
    if (sPath.isEmpty())
        return false;

    return saveConfigFileTo(config, sPath);
}

bool CsvTableView::openAppConfigFile()
{
    const auto sPath = QtApplication::getApplicationSettingsPath();
    if (sPath.isEmpty())
        return false;
    QFileInfo fi(sPath);
    if (!fi.exists())
    {
        if (QMessageBox::question(this,
                                  tr("App config file is missing"),
                                  tr("App config file does not exist, create?\n\nFile path would be '%1'?").arg(sPath))
                == QMessageBox::Yes)
        {
            if (QtApplication::createApplicationSettingsFile())
                return openAppConfigFile();
            else
            {
                QMessageBox::warning(this, tr("Opening app config file"), tr("Unable to create app config file"));
                return false;
            }
        }
        return false;
    }
    if (fi.isExecutable() || !fi.isReadable())
    {
        QString sLabel;
        if (!fi.exists())
            sLabel = tr("File does not exist");
        else if (!fi.isReadable())
            sLabel = tr("File exists but is not readable");
        else if (fi.isExecutable())
            sLabel = tr("File exists but is executable");
        sLabel += tr(", path '%1'").arg(sPath);
        QMessageBox::information(this, tr("Unable to open app config file"), sLabel);
        return false;
    }
    return QDesktopServices::openUrl(QUrl(QString("file:///%1").arg(sPath)));
}

bool CsvTableView::openConfigFile()
{
    auto pModel = csvModel();
    if (!pModel)
        return false;
    const auto sCsvPath = pModel->getFilePath();
    if (sCsvPath.isEmpty())
    {
        QMessageBox::information(this, tr("No config file"), tr("Current table has no path so can't search for config file. This can happen if table is new and unsaved or if table was opened from file with filters."));
        return false;
    }

    const auto sConfigPath = CsvFormatDefinition::csvFilePathToConfigFilePath(sCsvPath);
    QFileInfo fi(sConfigPath);
    if (!fi.exists())
    {
        const auto yesNo = QMessageBox::question(this, tr("No config file"), tr("File\n'%1'\nhas no config file.\nCreate one?").arg(sCsvPath));
        if (yesNo != QMessageBox::Yes || !saveConfigFile())
            return false;
    }

    std::unique_ptr<TableEditor> spConfigWidget(new TableEditor());
    spConfigWidget->setAllowApplicationSettingsUsage(getAllowApplicationSettingsUsage());
    auto bOpened = spConfigWidget->tryOpenFileFromPath(sConfigPath);
    if (!bOpened)
    {
        QMessageBox::information(this, tr("Unable to open config file"), tr("Failed to open config file from path '%1'.").arg(sConfigPath));
        return false;
    }

    auto pContainerWidget = new QDialog(this);
    pContainerWidget->setAttribute(Qt::WA_DeleteOnClose, true);
    removeContextHelpButtonFromDialog(pContainerWidget);
    delete pContainerWidget->layout();
    auto pLayout = new QHBoxLayout(pContainerWidget);
    pLayout->addWidget(spConfigWidget.get());
    spConfigWidget->setParent(pContainerWidget);

    pContainerWidget->resize(this->size());
    pContainerWidget->show();
    spConfigWidget->resizeColumnsToView();
    spConfigWidget.release(); // Deleted through childhood of container widget.

    return true;
}

bool CsvTableView::openFile(const QString& sPath)
{
    return openFile(sPath, CsvItemModel::getLoadOptionsForFile(sPath, csvModel()));
}

bool CsvTableView::openFile(const QString& sPath, const DFG_ROOT_NS::CsvFormatDefinition& formatDef)
{
    if (sPath.isEmpty())
        return false;
    auto pModel = csvModel();
    if (!pModel)
        return false;

    const auto fileSizeDouble = static_cast<double>(QFileInfo(sPath).size());

    const bool bOpenAsSqlite = ::DFG_MODULE_NS(sql)::SQLiteDatabase::isSQLiteFile(sPath);
    QString sQuery;
    if (bOpenAsSqlite)
    {
        const bool bHasQueryInConf = formatDef.hasProperty(CsvOptionProperty_sqlQuery);
        if (bHasQueryInConf)
            sQuery = untypedViewToQStringAsUtf8(formatDef.getProperty(CsvOptionProperty_sqlQuery, ""));
        if (!bHasQueryInConf || !::DFG_MODULE_NS(sql)::SQLiteDatabase::isSelectQuery(sQuery))
        {
            sQuery = getSQLiteDatabaseQueryFromUser(sPath, this);
            if (sQuery.isEmpty())
                return false;
        }
    }

    // Reset models to prevent event loop from updating stuff while model is being read in another thread
    auto pViewModel = model();
    auto pProxyModel = getProxyModelPtr();
    setModel(nullptr);
    if (pProxyModel && pProxyModel->sourceModel() == pModel)
        pProxyModel->setSourceModel(nullptr);

    const QString sAdditionalInfo = (bOpenAsSqlite) ? tr("\nQuery: %1").arg(sQuery) : QString();

    clearUndoStack(); // Clearing undo stack before modal operation since doing it during read caused "ASSERT failure in QCoreApplication::sendEvent: "Cannot send events to objects owned by a different thread."

    bool bSuccess = false;
    doModalOperation(tr("Reading file of size %1\n%2%3").arg(formattedDataSize(QFileInfo(sPath).size()), sPath, sAdditionalInfo), ProgressWidget::IsCancellable::yes, "CsvTableViewFileLoader", [&](ProgressWidget* pProgressWidget)
        {
            CsvItemModel::LoadOptions loadOptions(formatDef);
            bool bHasProgress = false;
            if (!bOpenAsSqlite && !loadOptions.isFilteredRead() && pProgressWidget) // Currently progress indicator works only for non-filtered csv-files.
            {
                pProgressWidget->setRange(0, 100);
                bHasProgress = true;
            }
            if (pProgressWidget)
            {
                using TimePointT = std::chrono::steady_clock::time_point;
                const auto timePointToAtomicType = [](const TimePointT& tp) { return tp.time_since_epoch().count(); };
                std::atomic<long long> aLastSetValue{timePointToAtomicType(std::chrono::steady_clock::now())};
                std::atomic<uint32> anLastThreadCount{1};
                const QString sOriginalLabel = pProgressWidget->labelText();
                loadOptions.setProgressController(CsvModel::LoadOptions::ProgressController([pProgressWidget, fileSizeDouble, bHasProgress, &sOriginalLabel, &timePointToAtomicType, &aLastSetValue, &anLastThreadCount](const CsvModel::LoadOptions::ProgressControllerParamT param)
                {
                    const auto nProcessedBytes = param.counter();
                    // Calling setValue for progressWidget; note that using invokeMethod() since progressWidget lives in another thread.
                    // Also limiting call rate to maximum of once per 50 ms to prevent calls getting queued if callback gets called more often than what setValues can be invoked.
                    const auto steadyNow = std::chrono::steady_clock::now();
                    if (steadyNow - TimePointT(TimePointT::duration(aLastSetValue.load(std::memory_order_relaxed))) > std::chrono::milliseconds(50))
                    {
                        const bool bCancelled = pProgressWidget->isCancelled();
                        if (bHasProgress && !bCancelled)
                        {
                            const auto nProgressPercentage = ::DFG_ROOT_NS::round<int>(100.0 * static_cast<double>(nProcessedBytes) / fileSizeDouble);
                            DFG_VERIFY(QMetaObject::invokeMethod(pProgressWidget, "setValue", Qt::QueuedConnection, QGenericReturnArgument(), Q_ARG(int, nProgressPercentage)));
                            // If thread count used in the operation changed, updating label text.
                            const auto nThreadCount = param.threadCount();
                            if (anLastThreadCount.load(std::memory_order_relaxed) != nThreadCount)
                            {
                                QString sLabel = tr("%1\nUsing %2 threads").arg(sOriginalLabel).arg(nThreadCount);
                                DFG_VERIFY(QMetaObject::invokeMethod(pProgressWidget, "setLabelText", Qt::QueuedConnection, QGenericReturnArgument(), Q_ARG(QString, sLabel)));
                            }
                        }
                        aLastSetValue.store(timePointToAtomicType(steadyNow), std::memory_order_relaxed);
                        return CsvModel::LoadOptions::ProgressCallbackReturnT(!bCancelled);
                    }
                    return CsvModel::LoadOptions::ProgressCallbackReturnT(true);
                }));
            }
            if (bOpenAsSqlite)
                bSuccess = pModel->openFromSqlite(sPath, sQuery, loadOptions);
            else
                bSuccess = pModel->openFile(sPath, loadOptions);
        });

    if (pProxyModel)
        pProxyModel->setSourceModel(pModel);
    setModel(pViewModel);

    // Resize columns to view evenly.
    {
        // Note about scroll bar handling (hack): it seems that scroll bar size is not properly taken into account at this point
        // so that there might actually be a horizontal scroll bar even after resizing to view evenly. As a workaround,
        // forcing scroll bars temporarily visible seems to be enough to avoid scroll bars from showing up on newly opened file.
        const auto hScrollPolicy = this->horizontalScrollBarPolicy();
        const auto vScrollPolicy = this->verticalScrollBarPolicy();
        // Set scroll bars on
        this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        // Do resizing.
        this->onColumnResizeAction_toViewEvenly();
        // Restore original scroll bars policies.
        this->setHorizontalScrollBarPolicy(hScrollPolicy);
        this->setVerticalScrollBarPolicy(vScrollPolicy);
    }

    // Apply column width hints from config file if present
    {
        typedef CsvConfig::StringViewT SvT;
        CsvConfig config;
        config.loadFromFile(qStringToFileApi8Bit(CsvFormatDefinition::csvFilePathToConfigFilePath(sPath)));
        if (config.entryCount() > 0 && config.valueStrOrNull(DFG_UTF8("columnsByIndex")) != nullptr)
        {
            config.forEachStartingWith(DFG_UTF8("columnsByIndex/"), [&](const SvT& relUri, const SvT& value) {
                auto pColSep = std::find(relUri.beginRaw(), relUri.endRaw(), '/');
                if (pColSep == relUri.endRaw() || pColSep == relUri.beginRaw())
                    return;
                StringViewC svIndex(relUri.beginRaw(), pColSep - relUri.beginRaw());
                const auto nCol = CsvModel::visibleColumnIndexToInternal(DFG_MODULE_NS(str)::strTo<int>(svIndex));
                if (pModel->isValidColumn(nCol))
                {
                    const auto svId = StringViewC(pColSep + 1, relUri.endRaw() - (pColSep + 1));
                    if (svId == "width_pixels")
                    {
                        const auto confWidth = ::DFG_MODULE_NS(str)::strTo<int>(value);
                        if (confWidth >= 0)
                            setColumnWidth(nCol, confWidth);
                    }
                    else if (svId == "visible" && ::DFG_MODULE_NS(str)::strTo<int>(value) == 0)
                        setColumnVisibility(nCol, false);
                }
            });
        }
    }

    // Setting initial scroll position
    {
        const auto loadOptions = pModel->getOpenTimeLoadOptions();
        auto& sInitialScrollPos = DFG_OPAQUE_REF().m_sInitialScrollPosition;
        sInitialScrollPos = untypedViewToQStringAsUtf8(loadOptions.getProperty(CsvOptionProperty_initialScrollPosition, "___"));
        if (sInitialScrollPos == "___")
            sInitialScrollPos = getCsvTableViewProperty<CsvTableViewPropertyId_initialScrollPosition>(this);
        scrollToDefaultPosition();
    }

    if (bSuccess)
        onNewSourceOpened();
    else
    {
        const QString sInfoPart = (!pModel->m_messagesFromLatestOpen.isEmpty()) ? tr("\nThe following message(s) were generated:\n%1").arg(pModel->m_messagesFromLatestOpen.join('\n')) : tr("\nThere are no details about the problem available.");
        QMessageBox::information(this, tr("Open failed"), tr("Opening file\n%1\nencountered problems.%2").arg(sPath, sInfoPart));
    }

    return bSuccess;
}

bool CsvTableView::getProceedConfirmationFromUserIfInModifiedState(const QString& sTranslatedActionDescription)
{
    auto pModel = csvModel();
    if (pModel && pModel->isModified())
    {
        const auto rv = QMessageBox::question(this,
                                              tr("Confirm closing edited file"),
                                              tr("Content has been edited, discard changes and %1?").arg(sTranslatedActionDescription),
                                              QMessageBox::Yes | QMessageBox::No,
                                              QMessageBox::No);
        return (rv == QMessageBox::Yes);
    }
    return true;
}

void CsvTableView::createNewTable()
{
    auto pCsvModel = csvModel();
    if (!pCsvModel || !getProceedConfirmationFromUserIfInModifiedState(tr("open a new table")))
        return;
    setReadOnlyMode(false); // Resetting read-only mode since user likely wants to edit empty table.
    pCsvModel->openNewTable();
}

bool CsvTableView::createNewTableFromClipboard()
{
    auto pCsvModel = csvModel();
    if (!pCsvModel || !getProceedConfirmationFromUserIfInModifiedState(tr("open a new table")))
        return false;
    pCsvModel->openNewTable();
    pCsvModel->removeColumns(0, pCsvModel->getColumnCount());
    pCsvModel->removeRows(0, pCsvModel->getRowCount());

    auto pClipboard = QApplication::clipboard();
    const QByteArray sClipboardText = (pClipboard) ? pClipboard->text().toUtf8() : QByteArray();
    auto loadOptions = CsvItemModel::LoadOptions();
    loadOptions.textEncoding(DFG_MODULE_NS(io)::encodingUTF8);
    bool bSuccess = false;

    doModalOperation(tr("Reading from clipboard, input size is %1").arg(sClipboardText.size()), ProgressWidget::IsCancellable::yes, "CsvTableViewClipboardLoader", [&](ProgressWidget*)
    {
        bSuccess = pCsvModel->openFromMemory(sClipboardText.data(), static_cast<size_t>(sClipboardText.size()), loadOptions);
    });

    if (bSuccess)
        onNewSourceOpened();

    return bSuccess;
}

bool CsvTableView::openFromFile()
{
    if (!getProceedConfirmationFromUserIfInModifiedState(tr("open a new file")))
        return false;
    return openFile(getFilePathFromFileDialog());
}

bool CsvTableView::openFromFileWithOptions()
{
    if (!getProceedConfirmationFromUserIfInModifiedState(tr("open a new file")))
        return false;
    const auto sPath = getFilePathFromFileDialog();
    if (sPath.isEmpty())
        return false;
    CsvFormatDefinitionDialog::LoadOptions loadOptions(csvModel());
    if (!::DFG_MODULE_NS(sql)::SQLiteDatabase::isSQLiteFile(sPath))
    {
        CsvFormatDefinitionDialog dlg(this, CsvFormatDefinitionDialog::DialogTypeLoad, csvModel(), sPath);
        if (dlg.exec() != QDialog::Accepted)
            return false;
        loadOptions = dlg.getLoadOptions();
        // Disable completer size limit as there is no control for the size limit so user would otherwise be unable to
        // easily enable completer for big files.
        loadOptions.setProperty(CsvOptionProperty_completerEnabledSizeLimit, DFG_MODULE_NS(str)::toStrC(uint64(NumericTraits<uint64>::maxValue)));
    }
    return openFile(sPath, loadOptions);
}

auto CsvTableView::reloadFromFile() -> bool
{
    auto pCsvModel = csvModel();
    const auto sPath = (pCsvModel) ? pCsvModel->getFilePath() : QString();
    if (sPath.isEmpty())
    {
        showStatusInfoTip(tr("Unable to reload; no path available")); // Note: filtered open clears path and can't be reloaded until original path is stored somewhere.
        return false;
    }

    if (!getProceedConfirmationFromUserIfInModifiedState(tr("reload table from file")))
        return false;
    
    if (!sPath.isEmpty() && openFile(sPath, pCsvModel->getOpenTimeLoadOptions())) // Note: using old open time load options, which is needed e.g. to keep filter options in case of "open with options", means that if one changes .conf-file, it won't be taken into account on reload.
        return true;
    else
    {
        showStatusInfoTip(tr("Reloading failed from path '%1'").arg(sPath));
        return false;
    }
}

bool CsvTableView::mergeFilesToCurrent()
{
    auto sPaths = QFileDialog::getOpenFileNames(this,
        tr("Select files to merge"),
        QString()/*dir*/,
        getFilterTextForOpenFileDialog(),
        nullptr/*selected filter*/,
        QFileDialog::Options() /*options*/);
    if (sPaths.isEmpty())
        return false;

    auto pModel = csvModel();
    if (pModel)
    {
        const auto bSuccess = pModel->importFiles(sPaths);
        if (!bSuccess)
            QMessageBox::information(nullptr, "", tr("Failed to merge files"));
        return bSuccess;
    }
    else
        return false;
}


// TODO: better description for what was blocked, perhaps T to provide a static function that returns short description.
// TODO: not all actions go through executeAction() so this protection does not cover all cases -> take those into account as well
//      -notably cell edits (done in CsvItemModel::setData) and CsvItemModel::batchEditNoUndo() (used e.g. by "Generate content")
#define DFG_TEMP_HANDLE_EXECUTE_ACTION_LOCKING \
    auto lockReleaser = tryLockForEdit(); \
    if (!lockReleaser.isLocked()) \
        { privShowExecutionBlockedNotification(typeid(T).name()); return false; }


template <class T, class Param0_T>
bool CsvTableView::executeAction(Param0_T&& p0)
{
    DFG_TEMP_HANDLE_EXECUTE_ACTION_LOCKING;
    
    if (m_spUndoStack && m_bUndoEnabled)
        pushToUndoStack<T>(std::forward<Param0_T>(p0));
    else
        DFG_CLASS_NAME(UndoCommand)::directRedo<T>(std::forward<Param0_T>(p0));

    return true;
}

template <class T, class Param0_T, class Param1_T>
bool CsvTableView::executeAction(Param0_T&& p0, Param1_T&& p1)
{
    DFG_TEMP_HANDLE_EXECUTE_ACTION_LOCKING;

    if (m_spUndoStack && m_bUndoEnabled)
        pushToUndoStack<T>(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1));
    else
        DFG_CLASS_NAME(UndoCommand)::directRedo<T>(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1));

    return true;
}

template <class T, class Param0_T, class Param1_T, class Param2_T>
bool CsvTableView::executeAction(Param0_T&& p0, Param1_T&& p1, Param2_T&& p2)
{
    DFG_TEMP_HANDLE_EXECUTE_ACTION_LOCKING;

    if (m_spUndoStack && m_bUndoEnabled)
        pushToUndoStack<T>(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1), std::forward<Param2_T>(p2));
    else
        DFG_CLASS_NAME(UndoCommand)::directRedo<T>(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1), std::forward<Param2_T>(p2));

    return true;
}

#undef DFG_TEMP_HANDLE_EXECUTE_ACTION_LOCKING

template <class T, class Param0_T>
void CsvTableView::pushToUndoStack(Param0_T&& p0)
{
    if (!m_spUndoStack)
        createUndoStack();
    QUndoCommand* command = new T(std::forward<Param0_T>(p0));
    (*m_spUndoStack)->push(command); // Stack takes ownership of command.
}

template <class T, class Param0_T, class Param1_T>
void CsvTableView::pushToUndoStack(Param0_T&& p0, Param1_T&& p1)
{
    if (!m_spUndoStack)
        createUndoStack();
    QUndoCommand* command = new T(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1));
    (*m_spUndoStack)->push(command); // Stack takes ownership of command.
}

template <class T, class Param0_T, class Param1_T, class Param2_T>
void CsvTableView::pushToUndoStack(Param0_T&& p0, Param1_T&& p1, Param2_T&& p2)
{
    if (!m_spUndoStack)
        createUndoStack();
    QUndoCommand* command = new T(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1), std::forward<Param2_T>(p2));
    (*m_spUndoStack)->push(command); // Stack takes ownership of command.
}

bool CsvTableView::clearSelected()
{
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionDelete)>(*this, getProxyModelPtr(), false /*false = not row mode*/);
}

bool CsvTableView::insertRowImpl(const int insertType)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
    auto pProxy = qobject_cast<QSortFilterProxyModel*>(getProxyModelPtr());
    if (pProxy && !pProxy->filterRegularExpression().pattern().isEmpty() && !pProxy->filterRegularExpression().match(QString()).hasMatch())
        showStatusInfoTip(tr("Inserted row was hidden due to filter"));
#endif
    return executeAction<CsvTableViewActionInsertRow>(this, static_cast<DFG_SUB_NS_NAME(undoCommands)::InsertRowType>(insertType));
}

bool CsvTableView::insertRowHere()
{
    return insertRowImpl(DFG_SUB_NS_NAME(undoCommands)::InsertRowTypeBefore);
}

bool CsvTableView::insertRowAfterCurrent()
{
    return insertRowImpl(DFG_SUB_NS_NAME(undoCommands)::InsertRowTypeAfter);
}

bool CsvTableView::insertColumn()
{
    const auto nCol = currentIndex().column();
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionInsertColumn)>(csvModel(), nCol);
}

bool CsvTableView::insertColumnAfterCurrent()
{
    const auto nCol = currentIndex().column();
    if (nCol >= 0)
        return executeAction<DFG_CLASS_NAME(CsvTableViewActionInsertColumn)>(csvModel(), nCol + 1);
    else
        return false;
}

bool CsvTableView::cut()
{
    copy();
    clearSelected();
    return true;
}

void CsvTableView::undo()
{
    if (!m_spUndoStack)
        return;
    auto lockReleaser = tryLockForEdit();
    if (!lockReleaser.isLocked())
    {
        privShowExecutionBlockedNotification("undo");
        return;
    }
    m_spUndoStack->item().undo();
}

void CsvTableView::redo()
{
    if (!m_spUndoStack)
        return;
    auto lockReleaser = tryLockForEdit();
    if (!lockReleaser.isLocked())
    {
        privShowExecutionBlockedNotification("redo");
        return;
    }
    m_spUndoStack->item().redo();
}

size_t CsvTableView::replace(const QVariantMap& params)
{
    if (params["find"].isNull() || params["replace"].isNull())
        return 0;
    auto pCsvModel = csvModel();
    if (!pCsvModel)
        return 0;
    const auto sFindText = qStringToStringUtf8(params["find"].toString());
    const auto sReplaceText = qStringToStringUtf8(params["replace"].toString());

    auto lockReleaser = tryLockForEdit();
    if (!lockReleaser.isLocked())
    {
        privShowExecutionBlockedNotification("Replace");
        return 0;
    }
    size_t nEditCount = 0;
    const auto selection = storeSelection();

    doModalOperation(tr("Executing Find & Replace..."), ProgressWidget::IsCancellable::yes, "find_replace", [&](ProgressWidget* pProgressDialog)
    {
        const auto bOldModifiedStatus = pCsvModel->isModified();
        pCsvModel->batchEditNoUndo([&](CsvItemModel::DataTable& table)
        {
            StringUtf8 sTemp;
            forEachCsvModelIndexInSelection([&](const QModelIndex& index, bool& rbContinue)
            {
                if (!pCsvModel->isCellEditable(index))
                    return;
                auto tpsz = table(index.row(), index.column());
                if (!tpsz || std::strstr(tpsz.c_str(), sFindText.c_str().c_str()) == nullptr)
                    return;
                if (pProgressDialog && pProgressDialog->isCancelled())
                {
                    rbContinue = false;
                    return;
                }
                sTemp = tpsz;
                ::DFG_MODULE_NS(str)::replaceSubStrsInplace(sTemp.rawStorage(), sFindText.rawStorage(), sReplaceText.rawStorage());
                ++nEditCount;
                table.setElement(index.row(), index.column(), sTemp);
            });
        });
        pCsvModel->setModifiedStatus(bOldModifiedStatus || nEditCount > 0);
    }); // Modal operation end

    restoreSelection(selection);

    QString sToolTipMsg;
    if (nEditCount > 0)
    {
        clearUndoStack(); // If something was edited outside undostack, undoing previous actions might not work correctly anymore so clearing the undostack.
        sToolTipMsg = tr("Replace edited %1 cell(s)").arg(nEditCount);
    }
    else
        sToolTipMsg = tr("No cells were edited by replace");

    // Triggering tooltip directly from here didn't show so using a timer.
    QTimer::singleShot(10, this, [=]() { QToolTip::showText(QCursor::pos(), sToolTipMsg); } );
    return nEditCount;
}

QString CsvTableView::makeClipboardStringForCopy(QChar cDelim)
{
    auto pViewModel = model();
    if (!pViewModel)
        return QString();

    const auto sm = selectionModel();
    auto selection = (sm) ? sm->selection() : QItemSelection();

    /*
     Selection consists of QItemSelectionRange's each of which is a rectangular selection.
     Examples:
          -when selecting all, there will be one QItemSelectionRange where topLeft() and bottomRight() will point to first and last cell in table.
          -When selecting N disconnected cells individually, there will be N QItemSelectionRange's in QItemSelection.
          -When selecting two columns, there will be two QItemSelectionRange's, both spanning all rows but only one column.

     When creating a string for clipboard, content must be gathered beginning from top row and each row may contain entries from more than one QItemSelectionRange.
    */

    const auto computeTopRow = [&]()
        {
            ::DFG_MODULE_NS(func)::MemFuncMin<int> minFunc;
            std::for_each(selection.cbegin(), selection.cend(), [&](const QItemSelectionRange& sr) { minFunc(sr.top()); });
            return minFunc.value();
        };

    QString sOutput;
    auto nTopRow = computeTopRow();
    std::vector<decltype(selection.cbegin())> rangesWithSelectionsOnRow;
    CsvModel::IndexSet presentColumnsOnRow;
    while (!selection.isEmpty())
    {
        rangesWithSelectionsOnRow.clear();
        presentColumnsOnRow.clear();
        // Selecting only those ranges that have something in this row.
        for (auto iter = selection.cbegin(), iterEnd = selection.cend(); iter != iterEnd; ++iter)
        {
            if (iter->top() <= nTopRow)
            {
                rangesWithSelectionsOnRow.push_back(iter);
                for (int c = iter->left(); c <= iter->right(); ++c)
                    presentColumnsOnRow.insert(c); // Not quite optimal if there are lots of columns, but shall do for now.
            }
        }
        DFG_ASSERT_CORRECTNESS(!rangesWithSelectionsOnRow.empty());
        bool bFirstSelectedOnRow = true;
        for (auto iterCol = presentColumnsOnRow.cbegin(), iterColEnd = presentColumnsOnRow.cend(); iterCol != iterColEnd; ++iterCol)
        {
            const auto c = *iterCol;
            for (auto iter = rangesWithSelectionsOnRow.cbegin(), iterEnd = rangesWithSelectionsOnRow.cend(); iter != iterEnd; ++iter)
            {
                if ((*iter)->contains(nTopRow, c, QModelIndex()))
                {
                    if (!bFirstSelectedOnRow)
                        sOutput += cDelim;
                    else
                        bFirstSelectedOnRow = false;
                    CsvModel::dataCellToString(pViewModel->data(pViewModel->index(nTopRow, c)).toString(), sOutput, cDelim);
                }
            }
        }
        sOutput.push_back('\n');
        ++nTopRow;
        // Removing selections which have been completely processed (i.e. those whose bottom() < nTopRow)
        selection.erase(::DFG_MODULE_NS(alg)::removeIf(selection.begin(), selection.end(), [=](const QItemSelectionRange& sr) { return nTopRow > sr.bottom(); }), selection.end());
        nTopRow = Max(nTopRow, computeTopRow());
    }

    return sOutput;
}

bool CsvTableView::copy()
{
    QString str = makeClipboardStringForCopy();

    QApplication::clipboard()->setText(str);
    return true;
}

bool CsvTableView::paste()
{
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionPaste)>(this);
}

bool CsvTableView::deleteCurrentColumn()
{
    const auto curIndex = currentIndex();
    const int nRow = curIndex.row();
    const int nCol = curIndex.column();
    const auto rv = deleteCurrentColumn(nCol);
    selectionModel()->setCurrentIndex(model()->index(nRow, nCol), QItemSelectionModel::NoUpdate);
    return rv;
}

bool CsvTableView::deleteCurrentColumn(const int nCol)
{
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionDeleteColumn)>(this, nCol);
}

bool CsvTableView::deleteSelectedRow()
{
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionDelete)>(*this, getProxyModelPtr(), true /*row mode*/);
}

bool CsvTableView::resizeTable()
{
    auto pModel = model();
    if (!pModel)
        return false;
    // TODO: ask new dimensions with a single dialog rather than with two separate.
    bool bOk = false;
    const int nRows = QInputDialog::getInt(this, tr("Table resizing"), tr("New row count"), pModel->rowCount(), 0, NumericTraits<int>::maxValue, 1, &bOk);
    if (!bOk)
        return false;
    const int nCols = QInputDialog::getInt(this, tr("Table resizing"), tr("New column count"), pModel->columnCount(), 0, NumericTraits<int>::maxValue, 1, &bOk);
    if (!bOk || nRows < 0 || nCols < 0)
        return false;
    return resizeTableNoUi(nRows, nCols);
}

bool CsvTableView::resizeTableNoUi(const int r, const int c)
{
    return executeAction<CsvTableViewActionResizeTable>(this, r, c);
}

bool CsvTableView::transpose()
{
    auto pCsvModel = csvModel();
    if (!pCsvModel)
        return false;

    const auto nNewColCount = pCsvModel->rowCount();
    if (nNewColCount > 10000)
    {
        if (QMessageBox::question(this,
                                  tr("Confirm transpose"),
                                  tr("Transposed table would have %1 columns. Data structures are not optimized for having big number columns \
                                     and having too many columns may take a long time or cause a crash. Proceed nevertheless?").arg(nNewColCount))
                != QMessageBox::Yes)
            return false;
    }

    const auto waitCursor = makeScopedCaller([]() { QApplication::setOverrideCursor(QCursor(Qt::WaitCursor)); },
                                             []() { QApplication::restoreOverrideCursor(); });

    // Note: not using doModalOperation() as it caused "ASSERT failure in QCoreApplication::sendEvent: "Cannot send events to objects owned by a different thread"
    //       from the transpose implementation.
    bool bSuccess = pCsvModel->transpose();

    if (!bSuccess)
        showStatusInfoTip(tr("Transpose failed"));
    return bSuccess;
}

void CsvTableView::onChangeRadixUiAction()
{
    if (!isSelectionNonEmpty())
    {
        this->showStatusInfoTip(tr("Selection is empty"));
        return;
    }

    using JsonId = CsvTableViewActionChangeRadix::Params::ParamId;
    const auto idToStr = [](const JsonId id) { return CsvTableViewActionChangeRadix::Params::paramStringId(id); };

    if (DFG_OPAQUE_REF().m_previousChangeRadixArgs.isEmpty())
    {
        DFG_OPAQUE_REF().m_previousChangeRadixArgs = {
            { idToStr(JsonId::fromRadix), 10}, {idToStr(JsonId::toRadix), 0},
            { idToStr(JsonId::ignorePrefix), ""}, {idToStr(JsonId::ignoreSuffix), ""},
            { idToStr(JsonId::resultPrefix), ""}, {idToStr(JsonId::resultSuffix), ""}, { idToStr(JsonId::resultDigits), "" }
        };
    }

    bool bOk = false;
    const auto items = InputDialog::getJsonAsVariantMap(
        this,
        tr("Change radix"),
        tr("Change radix for numbers in selected cells."
           "<ul>"
           "<li>Valid %3 values are 2-36</li>"
           "<li>Valid %2 values are 2-62</li>"
           "<li>Values in range of int64 can be converted</li>"
           "<li>If prefix/suffix is given, lack of such prefix/suffix is not considered as error</li>"
           "<li>%1 can be used to define custom digits (e.g. \"ab\" would define radix 2 with digits {a,b} instead of typical {0,1}): if given, %2 can be omitted as length defines radix</li>"
           "</ul>")
            .arg(idToStr(JsonId::resultDigits), idToStr(JsonId::toRadix), idToStr(JsonId::fromRadix)),
        DFG_OPAQUE_REF().m_previousChangeRadixArgs,
        idToStr(JsonId::toRadix), // Sets selected text to be value of toRadix
        &bOk
    );
    if (!bOk)
        return;

    DFG_OPAQUE_REF().m_previousChangeRadixArgs = items;

    CsvTableViewActionChangeRadix::Params params(items);
    if (!params.isValid())
    {
        this->showStatusInfoTip(tr("Action ignored: not all radix values are valid: source radix = %1, destination radix = %2")
            .arg(params.fromRadix)
            .arg(params.toRadix));
        return;
    }
    changeRadix(params);
}

void CsvTableView::changeRadix(const CsvTableViewActionChangeRadixParams& params)
{
    executeAction<CsvTableViewActionChangeRadix>(this, params);
}

void CsvTableView::evaluateSelectionAsFormula()
{
    if (isSelectionNonEmpty())
        executeAction<CsvTableViewActionEvaluateSelectionAsFormula>(this);
    else
        QToolTip::showText(QCursor::pos(), tr("Selection is empty"));
}

bool CsvTableView::generateContent()
{
    auto pModel = model();
    if (!pModel)
        return false;

    // Note: in search below, findChild<ContentGeneratorDialog*>(); (without name specifier) would returns UndoViewWidget if had been created (in Qt 5.13.2)
    //       According to https://stackoverflow.com/questions/35869441/why-is-qobject-findchildren-returning-children-with-a-common-base-class
    //       this is expected to be caused by lack of Q_OBJECT in these helper dialogs.
    auto pGeneratorDialog = findChild<ContentGeneratorDialog*>("ContentGeneratorDialog");
    if (!pGeneratorDialog)
    {
        pGeneratorDialog = new ContentGeneratorDialog(this);
        pGeneratorDialog->setObjectName("ContentGeneratorDialog");
    }

    pGeneratorDialog->resize(350, 450);
    while (true) // Show dialog until values are accepted or cancel is selected.
    {
        const auto rv = pGeneratorDialog->exec();
        if (rv == QDialog::Accepted && pGeneratorDialog->m_spSettingsModel)
        {
            // Trying to lock view for write.
            auto lockReleaser = tryLockForEdit();
            if (!lockReleaser.isLocked())
            {
                pGeneratorDialog->setGenerateFailed(true, privCreateActionBlockedDueToLockedContentMessage(tr("content generation")));
                continue;
            }

            const auto waitCursor = makeScopedCaller([]() { QApplication::setOverrideCursor(QCursor(Qt::WaitCursor)); },
                                                     []() { QApplication::restoreOverrideCursor(); });
            if (generateContentImpl(*pGeneratorDialog->m_spSettingsModel))
            {
                pGeneratorDialog->setGenerateFailed(false);
                this->clearUndoStack();
                return true;
            }
            pGeneratorDialog->setGenerateFailed(true);
        }
        else
            return false;
    }
}

auto CsvTableView::storeSelection() const -> SelectionRangeList
{
    auto selection = this->getSelection();
    SelectionRangeList list; // Stored as topRow, topCol, bottomRow, bottomCol
    for (auto iter = selection.cbegin(); iter != selection.cend(); ++iter)
    {
        const auto& topLeft = iter->topLeft();
        const auto& bottomRight = iter->bottomRight();
        list.push_back(std::make_tuple(topLeft.row(), topLeft.column(), bottomRight.row(), bottomRight.column()));
    }
    return list;
}

void CsvTableView::restoreSelection(const SelectionRangeList& selection) const
{
    // Reconstructing old selection from stored indexes and setting it.
    QItemSelection newSelection;
    auto pViewModel = this->model();
    if (!pViewModel)
        return;
    for (auto iter = selection.cbegin(); iter != selection.cend(); ++iter)
    {
        newSelection.push_back(QItemSelectionRange(pViewModel->index(std::get<0>(*iter), std::get<1>(*iter)),
            pViewModel->index(std::get<2>(*iter), std::get<3>(*iter))));
    }
    // Restoring selection.
    auto pSelectionModel = this->selectionModel();
    if (pSelectionModel)
        pSelectionModel->select(newSelection, QItemSelectionModel::ClearAndSelect);
}

// Calls 'generator' for each cell in target. 
// Generator received 4 arguments: table, model row (0-based), model column (0-based), cell counter (0 for first item).
template <class Generator_T>
static void generateForEachInTarget(const ::DFG_MODULE_NS(qt)::ContentGeneratorDialog::TargetType targetType, const CsvTableView& view, ::DFG_MODULE_NS(qt)::CsvItemModel& rModel, Generator_T generator)
{
    DFG_STATIC_ASSERT(::DFG_MODULE_NS(qt)::ContentGeneratorDialog::TargetType_last == 2, "This implementation handles only two target types");

    const auto preEditSelectionRanges = view.storeSelection();

    if (targetType == ::DFG_MODULE_NS(qt)::ContentGeneratorDialog::TargetTypeWholeTable)
    {
        const auto nRows = rModel.rowCount();
        const auto nCols = rModel.columnCount();
        if (nRows < 1 || nCols < 1) // Nothing to add?
            return;
        size_t nCounter = 0;
        rModel.batchEditNoUndo([&](CsvItemModel::DataTable& table)
        {
            // Note: top-to-bottom visit pattern is guaranteed in documentation of cellValue() parser function.
            for (int c = 0; c < nCols; ++c)
            {
                if (rModel.isReadOnlyColumn(c))
                    continue;
                for (int r = 0; r < nRows; ++r, ++nCounter)
                {
                    if (rModel.isCellEditable(r, c))
                        generator(table, r, c, nCounter);
                }
            }
        });
    }
    else if (targetType == ::DFG_MODULE_NS(qt)::ContentGeneratorDialog::TargetTypeSelection)
    {
        size_t nCounter = 0;
        rModel.batchEditNoUndo([&](CsvItemModel::DataTable& table)
        {
            view.forEachCsvModelIndexInSelection([&](const QModelIndex& index, bool& bContinue)
            {
                DFG_UNUSED(bContinue);
                if (rModel.isCellEditable(index.row(), index.column()))
                    generator(table, index.row(), index.column(), nCounter++);
            });
        });
    }
    else
    {
        DFG_ASSERT_IMPLEMENTED(false);
    }

    // Restoring old selection
    view.restoreSelection(preEditSelectionRanges);
}

namespace
{
    // TODO: test
    bool isValidFloatPrefix(const QString& sPrefix)
    {
        return (sPrefix.isEmpty() ||
            sPrefix == "l" ||
            sPrefix == "L");
    }

    // TODO: test
    bool isValidIntegerPrefix(const QString& sPrefix)
    {
        return (sPrefix.isEmpty() ||
            sPrefix == "I" ||
            sPrefix == "l" ||
            sPrefix == "L" ||
            sPrefix == "ll" ||
            sPrefix == "LL" ||
            sPrefix == "I32" ||
            sPrefix == "I64");
    }

    const char gszIntegerTypes[] = "diouxX";
#if DFG_TOSTR_USING_TO_CHARS_WITH_FLOAT_PREC_ARG == 1
    const char gszFloatTypes[] = "aefg";
#else
    const char gszFloatTypes[] = "g"; // Note: actual formatting may differ from sprintf 'g' and std::to_chars() with std::chars_format::general
#endif

    // TODO: test
    bool isValidNumberFormatType(const QString& s)
    {
        if (s.isEmpty())
            return false;
        const auto cLast = s[s.size() - 1].toLatin1();

        const bool bIsFloatType = (std::strchr(gszFloatTypes, cLast) != nullptr);
        const bool bIsIntergerType = (!bIsFloatType && std::strchr(gszIntegerTypes, cLast) != nullptr);
        if (!bIsFloatType && !bIsIntergerType)
            return false;
        const auto sPrefix = s.mid(0, s.size() - 1);
        return ((bIsFloatType && isValidFloatPrefix(sPrefix)) ||
                (bIsIntergerType && isValidIntegerPrefix(sPrefix)));
    }

    std::string handlePrecisionParameters(const ::DFG_MODULE_NS(qt)::CsvItemModel& settingsModel)
    {
        const auto sFormatType = settingsModel.data(settingsModel.index(ContentGeneratorDialog::LastNonParamPropertyId + 2, 1)).toString().trimmed();
        auto sPrecision = settingsModel.data(settingsModel.index(ContentGeneratorDialog::LastNonParamPropertyId + 3, 1)).toString();
        if (sFormatType == "date_sec_local" || sFormatType == "date_msec_local" || sFormatType == "date_sec_utc" || sFormatType == "date_msec_utc")
            return std::string(QString("%1:%2").arg(sFormatType, sPrecision).toLatin1());
        bool bPrecisionIsUint;
        const auto tr = [&](const char* psz) { return settingsModel.tr(psz); };
        if (!sPrecision.isEmpty() && (sPrecision.toUInt(&bPrecisionIsUint) > 1000 || !bPrecisionIsUint)) // Not sure does this need to be limited; just cut it somewhere.
        {
            QMessageBox::information(nullptr, tr("Invalid parameter"), tr("Precision-parameter has invalid value; no content generation is done"));
            return std::string();
        }
        if (!isValidNumberFormatType(sFormatType))
        {
            QMessageBox::information(nullptr, tr("Invalid parameter"), tr("Format type parameter '%1' is not supported, only the following printf-types are available: %2").arg(sFormatType, gszFloatTypes));
            return std::string();
        }
        if (!sPrecision.isEmpty())
            sPrecision.prepend('.');
        return std::string(("%" + sPrecision + sFormatType).toLatin1());
    }

    typedef decltype(QString().split(' ')) RandQStringArgs;

    enum class GeneratorFormatType
    {
        number,
        dateFromSecondsLocal,
        dateFromMilliSecondsLocal,
        dateFromSecondsUtc,
        dateFromMilliSecondsUtc
    };

    auto sprintfTypeCharToCharsFormat(const char c) -> ::DFG_MODULE_NS(str)::CharsFormat
    {
        using namespace ::DFG_MODULE_NS(str);
    #if DFG_TOSTR_USING_TO_CHARS_WITH_FLOAT_PREC_ARG == 1
        switch (c)
        {
            case 'a': return CharsFormat::hex;
            case 'e': return CharsFormat::scientific;
            case 'f': return CharsFormat::fixed;
            case 'g': return CharsFormat::general;
            default:  return CharsFormat::default_fmt;
        }
    #else
        DFG_UNUSED(c);
        return CharsFormat::default_fmt;
    #endif
    }

    int formatStringToPrecision(const ::DFG_ROOT_NS::StringViewC sv)
    {
        using namespace ::DFG_ROOT_NS;
        using namespace ::DFG_MODULE_NS(str);
        auto p = std::find(sv.begin(), sv.end(), '.');
        if (p == sv.end())
            return -1;
        ++p;
        const auto pStart = p;
        for (; p != sv.end() && std::isdigit(*p) != 0; ++p) {}
        const auto nPrec = strTo<int>(StringViewC(pStart, p));
        return nPrec;
    }

    template <class T, size_t N>
    auto numberValToString(T val, char(&buffer)[N], const char *pFormat, std::true_type) -> ::DFG_ROOT_NS::SzPtrUtf8R
    {
        using namespace ::DFG_ROOT_NS;
        using namespace ::DFG_MODULE_NS(str);
        using namespace ::DFG_MODULE_NS(math);
        DFG_STATIC_ASSERT(std::is_floating_point<T>::value, "This function requires floating point type");
        if (!pFormat)
        {
            DFG_ASSERT(false); // pFormat is always assumed to be non-null in this implementation.
            return DFG_UTF8("");
        }
        const StringViewC svFormat(pFormat);
        // pFormat is expected to always be of format %[.nPrecision]type
        const auto nPrecision = formatStringToPrecision(svFormat);
        #if DFG_TOSTR_USING_TO_CHARS_WITH_FLOAT_PREC_ARG == 1
            const auto charsFormat = sprintfTypeCharToCharsFormat(svFormat.back());
            auto p = buffer;
            if (charsFormat == CharsFormat::hex)
            {
                // Unlike 'a' in printf specification, CharsFormat::hex does not prepend 0x so adding it manually.
                DFG_STATIC_ASSERT(N >= 4, "Expecting buffer to be at least 4 bytes.");
                if (signBit(val) == 1)
                {
                    *p++ = '-';
                    val = signCopied(val, 1.0);
                }
                *p++ = '0';
                *p++ = 'x';
            }
            floatingPointToStr(val, p, N - (p - buffer), nPrecision, charsFormat);
        #else // case: DFG_TOSTR_USING_TO_CHARS_WITH_FLOAT_PREC_ARG != 1
            floatingPointToStr(val, buffer, nPrecision);
        #endif

        return SzPtrUtf8R(buffer);
    }

    template <class T, size_t N>
    auto numberValToString(const T val, char(&buffer)[N], const char* pFormat, std::false_type) -> ::DFG_ROOT_NS::SzPtrUtf8R
    {
        using namespace ::DFG_ROOT_NS;
        using namespace ::DFG_MODULE_NS(str);
        DFG_STATIC_ASSERT(!std::is_floating_point<T>::value, "This function is not for floating point types");
        DFG_UNUSED(pFormat);
        toStr(val, buffer);
        return SzPtrUtf8R(buffer);
    }

    qint64 valToDateInt64(const double val)
    {
        qint64 intVal;
        double valIntPart;
        std::modf(val, &valIntPart);
        return (::DFG_MODULE_NS(math)::isFloatConvertibleTo(valIntPart, &intVal)) ? intVal : -1;
    }

    template <class T, size_t N>
    void setTableElement(const ::DFG_MODULE_NS(qt)::CsvTableView& rView, ::DFG_MODULE_NS(qt)::CsvItemModel::DataTable& table, const int r, const int c, const T val, char(&buffer)[N], const char *pFormat, const GeneratorFormatType type)
    {
        using namespace ::DFG_ROOT_NS;
        using namespace ::DFG_MODULE_NS(str);
        if (type == GeneratorFormatType::number)
        {
            table.setElement(r, c, numberValToString(val, buffer, pFormat, std::is_floating_point<T>()));
            buffer[0] = '\0';
        }
        else // Case: other than numeric type.
        {
            const auto factor = (type == GeneratorFormatType::dateFromSecondsLocal || type == GeneratorFormatType::dateFromSecondsUtc) ? 1000 : 1;
            const auto i64 = valToDateInt64(factor * val);
            if (i64 < 0 || !pFormat)
            {
                table.setElement(r, c, DFG_UTF8(""));
                return;
            }
            QString sResult;
            Qt::TimeSpec timeSpec = Qt::TimeZone; // Using Qt::TimeZone as "not set"-value
            if (type == GeneratorFormatType::dateFromSecondsLocal || type == GeneratorFormatType::dateFromMilliSecondsLocal)
                timeSpec = Qt::LocalTime;
            else if (type == GeneratorFormatType::dateFromSecondsUtc || type == GeneratorFormatType::dateFromMilliSecondsUtc)
                timeSpec = Qt::UTC;

            if (timeSpec != Qt::TimeZone)
                sResult = rView.dateTimeToString(QDateTime::fromMSecsSinceEpoch(i64, timeSpec), pFormat);
            else
            {
                DFG_ASSERT_IMPLEMENTED(false);
            }
            table.setElement(r, c, qStringToStringUtf8(sResult));
        }
    }

    GeneratorFormatType adjustFormatAndGetType(const char*& pFormat)
    {
        using namespace ::DFG_MODULE_NS(str);
        if (!pFormat)
            return GeneratorFormatType::number;
        else if (beginsWith(pFormat, "date_sec_local:"))
        {
            pFormat += DFG_COUNTOF_SZ("date_sec_local:");
            return GeneratorFormatType::dateFromSecondsLocal;
        }
        else if (beginsWith(pFormat, "date_msec_local:"))
        {
            pFormat += DFG_COUNTOF_SZ("date_msec_local:");
            return GeneratorFormatType::dateFromMilliSecondsLocal;
        }
        else if (beginsWith(pFormat, "date_sec_utc:"))
        {
            pFormat += DFG_COUNTOF_SZ("date_sec_utc:");
            return GeneratorFormatType::dateFromSecondsUtc;
        }
        else if (beginsWith(pFormat, "date_msec_utc:"))
        {
            pFormat += DFG_COUNTOF_SZ("date_msec_utc:");
            return GeneratorFormatType::dateFromMilliSecondsUtc;
        }
        else
            return GeneratorFormatType::number;
    }

    template <class Distr_T, size_t ArgCount>
    bool generateRandom(const ContentGeneratorDialog::TargetType target, CsvTableView& view, const RandQStringArgs& qstrArgs, const char* pFormat = nullptr)
    {
        auto pModel = view.csvModel();
        if (!pModel)
            return false;
        auto& rModel = *pModel;

        std::vector<std::string> strArgs(qstrArgs.size());
        std::transform(qstrArgs.cbegin(), qstrArgs.cend(), strArgs.begin(), [](const QString& s) { return s.toStdString(); });
        if (strArgs.size() != ArgCount)
            return false;

        auto args = ::DFG_MODULE_NS(rand)::makeDistributionConstructArgs<Distr_T>(strArgs);
        if (!args.first) // Are arguments valid?
            return false;

        auto distr = ::DFG_MODULE_NS(rand)::makeDistribution<Distr_T>(args);

        auto randEng = ::DFG_MODULE_NS(rand)::createDefaultRandEngineRandomSeeded();
        char szBuffer[32];
        const auto formatType = adjustFormatAndGetType(pFormat);
        const auto generator = [&](CsvItemModel::DataTable& table, int r, int c, size_t)
            {
                // If result type is bool, using int, otherwise the same type. This is because toStr() wouldn't compile with bool result type.
                typedef typename std::conditional<std::is_same<decltype(distr(randEng)), bool>::value, int, decltype(distr(randEng))>::type ResultType;
                const ResultType val = distr(randEng);
                setTableElement(view, table, r, c, val, szBuffer, pFormat, formatType);
            };
        generateForEachInTarget(target, view, rModel, generator);
        return true;
    }

    bool generateRandomInt(const ContentGeneratorDialog::TargetType target, CsvTableView& view, RandQStringArgs& params)
    {
        if (params.isEmpty())
            return false;
        const QString sDistribution = params.takeFirst();
        if (sDistribution == QLatin1String("uniform"))
            return generateRandom<std::uniform_int_distribution<int>, 2>(target, view, params); // Params: min, max.
        else if (sDistribution == QLatin1String("binomial"))
            return generateRandom<std::binomial_distribution<int>, 2>(target, view, params); // Params: count, probability.
        else if (sDistribution == QLatin1String("bernoulli"))
            return generateRandom<std::bernoulli_distribution, 1>(target, view, params); // Params: probability
        else if (sDistribution == QLatin1String("negative_binomial"))
            return generateRandom<::DFG_MODULE_NS(rand)::NegativeBinomialDistribution<int>, 2>(target, view, params); // Params: count, probability.
        else if (sDistribution == QLatin1String("geometric"))
            return generateRandom<std::geometric_distribution<int>, 1>(target, view, params); // Params: probability
        else if (sDistribution == QLatin1String("poisson"))
            return generateRandom<std::poisson_distribution<int>, 1>(target, view, params); // Params: mean
        return false;
    }

    bool generateRandomReal(const ContentGeneratorDialog::TargetType target, DFG_CLASS_NAME(CsvTableView)& view, RandQStringArgs& params, const char* pszFormat)
    {
        if (params.isEmpty())
            return false;
        const QString sDistribution = params.takeFirst();
        if (sDistribution == QLatin1String("uniform"))
            return generateRandom<std::uniform_real_distribution<double>, 2>(target, view, params, pszFormat);
        else if (sDistribution == QLatin1String("normal"))
            return generateRandom<std::normal_distribution<double>, 2>(target, view, params, pszFormat);
        else if (sDistribution == QLatin1String("cauchy"))
            return generateRandom<std::cauchy_distribution<double>, 2>(target, view, params, pszFormat);
        else if (sDistribution == QLatin1String("exponential"))
            return generateRandom<std::exponential_distribution<double>, 1>(target, view, params, pszFormat);
        else if (sDistribution == QLatin1String("gamma"))
            return generateRandom<std::gamma_distribution<double>, 2>(target, view, params, pszFormat);
        else if (sDistribution == QLatin1String("weibull"))
            return generateRandom<std::weibull_distribution<double>, 2>(target, view, params, pszFormat);
        else if (sDistribution == QLatin1String("extreme_value"))
            return generateRandom<std::extreme_value_distribution<double>, 2>(target, view, params, pszFormat);
        else if (sDistribution == QLatin1String("lognormal"))
            return generateRandom<std::lognormal_distribution<double>, 2>(target, view, params, pszFormat);
        else if (sDistribution == QLatin1String("chi_squared"))
            return generateRandom<std::chi_squared_distribution<double>, 1>(target, view, params, pszFormat);
        else if (sDistribution == QLatin1String("fisher_f"))
            return generateRandom<std::fisher_f_distribution<double>, 2>(target, view, params, pszFormat);
        else if (sDistribution == QLatin1String("student_t"))
            return generateRandom<std::student_t_distribution<double>, 1>(target, view, params, pszFormat);
        return false;
    }

    static double cellValueAsDouble(::DFG_MODULE_NS(qt)::CsvItemModel::DataTable* pTable, const double rowDouble, const double colDouble)
    {
        using namespace ::DFG_MODULE_NS(math);
        using namespace ::DFG_MODULE_NS(qt);
        if (!pTable)
            return std::numeric_limits<double>::quiet_NaN();
        int r, c;
        if (!isFloatConvertibleTo<int>(rowDouble, &r) || !isFloatConvertibleTo(colDouble, &c))
            return std::numeric_limits<double>::quiet_NaN();
        r = ::DFG_MODULE_NS(qt)::CsvItemModel::visibleRowIndexToInternal(r);
        c = ::DFG_MODULE_NS(qt)::CsvItemModel::visibleRowIndexToInternal(c);
        const auto tpsz = (*pTable)(r, c);
        const auto rv = (tpsz) ? tableCellStringToDouble(tpsz) : std::numeric_limits<double>::quiet_NaN();
        return rv;
    }
} // unnamed namespace

bool CsvTableView::generateContentImpl(const CsvModel& settingsModel)
{
    const auto genType = ContentGeneratorDialog::generatorType(settingsModel);
    const auto target = ContentGeneratorDialog::targetType(settingsModel);
    auto pModel = csvModel();
    if (!pModel)
        return false;
    auto& rModel = *pModel;

    DFG_STATIC_ASSERT(ContentGeneratorDialog::GeneratorType_last == 4, "This implementation handles only 4 generator types");
    if (genType == ContentGeneratorDialog::GeneratorTypeRandomIntegers)
    {
        if (settingsModel.rowCount() < ContentGeneratorDialog::LastNonParamPropertyId + 2) // Not enough parameters in model?
        {
            DFG_ASSERT(false);
            return false;
        }
        auto params = settingsModel.data(settingsModel.index(ContentGeneratorDialog::LastNonParamPropertyId + 1, 1)).toString().split(',');
        return generateRandomInt(target, *this, params);
    }
    else if (genType == ContentGeneratorDialog::GeneratorTypeRandomDoubles)
    {
        if (settingsModel.rowCount() < ContentGeneratorDialog::LastNonParamPropertyId + 4) // Not enough parameters in model?
        {
            DFG_ASSERT(false);
            return false;
        }
        auto params = settingsModel.data(settingsModel.index(ContentGeneratorDialog::LastNonParamPropertyId + 1, 1)).toString().split(',');
        const std::string sFormat = handlePrecisionParameters(settingsModel);
        if (sFormat.empty())
            return false;
        const auto pszFormat = sFormat.c_str();
        return generateRandomReal(target, *this, params, pszFormat);
    }
    else if (genType == ContentGeneratorDialog::GeneratorTypeFill)
    {
        if (settingsModel.rowCount() < ContentGeneratorDialog::LastNonParamPropertyId + 2) // Not enough parameters in model?
        {
            DFG_ASSERT(false);
            return false;
        }
        const auto sFill = settingsModel.data(settingsModel.index(ContentGeneratorDialog::LastNonParamPropertyId + 1, 1)).toString().toUtf8();
        const auto pszFillU8 = SzPtrUtf8(sFill.data());
        const auto generator = [&](CsvItemModel::DataTable& table, int r, int c, size_t)
        {
            table.setElement(r, c, pszFillU8);
        };
        generateForEachInTarget(target, *this, rModel, generator);
        return true;
    }
    else if (genType == ContentGeneratorDialog::GeneratorTypeFormula)
    {
        if (settingsModel.rowCount() < ContentGeneratorDialog::LastNonParamPropertyId + 4) // Not enough parameters in model?
        {
            DFG_ASSERT(false);
            return false;
        }
        const std::string sFormat = handlePrecisionParameters(settingsModel);
        if (sFormat.empty())
            return false;

        auto pszFormat = sFormat.c_str();
        const auto sFormula = settingsModel.data(settingsModel.index(ContentGeneratorDialog::LastNonParamPropertyId + 1, 1)).toString().toUtf8();
        ::DFG_MODULE_NS(math)::FormulaParser parser;
        DFG_VERIFY(parser.defineRandomFunctions());
        if (!parser.setFormula(sFormula.data()))
        {
            DFG_ASSERT_WITH_MSG(false, "Failed to set formula to parser");
            return false;
        }
        char buffer[32] = "";
        double tr = std::numeric_limits<double>::quiet_NaN();
        double tc = std::numeric_limits<double>::quiet_NaN();
        const double rowCount = rModel.rowCount();
        const double colCount = rModel.columnCount();
        CsvItemModel::DataTable* pTable = nullptr;
        if (!parser.defineFunctor("cellValue", [&](double r, double c) { return cellValueAsDouble(pTable, r, c); }, false)
            || !parser.defineVariable("trow", &tr)
            || !parser.defineVariable("tcol", &tc)
            || !parser.defineConstant("rowcount", rowCount)
            || !parser.defineConstant("colcount", colCount))
        {
            DFG_ASSERT_WITH_MSG(false, "Failed to set functions/variables/constants to parser");
            return false;
        }
        const auto formatType = adjustFormatAndGetType(pszFormat);
        const auto generator = [&](CsvItemModel::DataTable& table, const int r, const int c, size_t)
        {
            pTable = &table;
            tr = CsvItemModel::internalRowIndexToVisible(r);
            tc = CsvItemModel::internalColumnIndexToVisible(c);
            const auto val = parser.evaluateFormulaAsDouble();
            setTableElement(*this, table, r, c, val, buffer, pszFormat, formatType);
        };
        generateForEachInTarget(target, *this, rModel, generator);
        return true;
    }
    else
    {
        DFG_ASSERT_IMPLEMENTED(false);
    }
    return false;
}

namespace
{
    namespace ColumnPropertyId
    {
        const SzPtrUtf8R visible = DFG_UTF8("visible");
    } // namespace ColumnPropertyId

    bool checkHeaderFirstRowActionPreconditions(::DFG_MODULE_NS(qt)::CsvTableView& rView, std::function<bool(const ::DFG_MODULE_NS(qt)::CsvTableView::CsvModel&)> funcAdditionalPreconditions)
    {
        using namespace ::DFG_MODULE_NS(qt);
        auto pModel = rView.csvModel();
        if (!pModel || pModel->columnCount() <= 0 || (funcAdditionalPreconditions && !funcAdditionalPreconditions(*pModel)))
            return false;
        bool bHiddenColumnFound = false;
        bool bReadOnlyColumnFound = false;
        const auto lockReleaser = rView.tryLockForRead();
        QString sBlockedMessage;
        if (!pModel->forEachColInfoWhile(lockReleaser, [&](const CsvTableView::CsvModel::ColInfo& rColInfo)
            {
                bHiddenColumnFound = !rView.getColumnPropertyByDataModelIndex(rColInfo.index(), ColumnPropertyId::visible, true).toBool();
                bReadOnlyColumnFound = rColInfo.getProperty(CsvItemModelColumnProperty::readOnly).toBool();
                return !bHiddenColumnFound && !bReadOnlyColumnFound;
            }))
        {
            sBlockedMessage = rView.tr("Action can't be executed at the moment: there seems to be ongoing operations");
        }
        else
        {
            if (bHiddenColumnFound)
                sBlockedMessage = rView.tr("Action can't be executed while there are hidden columns");
            else if (bReadOnlyColumnFound)
                sBlockedMessage = rView.tr("Action can't be executed while there are read-only columns");
        }

        if (sBlockedMessage.isEmpty())
            return true;
        else
        {
            rView.showStatusInfoTip(sBlockedMessage);
            return false;
        }
    }
}

bool CsvTableView::moveFirstRowToHeader()
{
    if (checkHeaderFirstRowActionPreconditions(*this, [](const CsvModel& csvModel) { return csvModel.rowCount() >= 1; }))
        return executeAction<CsvTableViewActionMoveFirstRowToHeader>(this);
    else
        return false;
}

bool CsvTableView::moveHeaderToFirstRow()
{
    if (checkHeaderFirstRowActionPreconditions(*this, [](const CsvModel&) { return true; }))
        return executeAction<CsvTableViewActionMoveFirstRowToHeader>(this, true);
    else
        return false;
}

QAbstractProxyModel* CsvTableView::getProxyModelPtr()
{
    return qobject_cast<QAbstractProxyModel*>(model());
}

const QAbstractProxyModel* CsvTableView::getProxyModelPtr() const
{
    return qobject_cast<const QAbstractProxyModel*>(model());
}

bool CsvTableView::diffWithUnmodified()
{
    const char szTempFileNameTemplate[] = "dfgqtCTV"; // static part for temporary filenames.
    auto dataModelPtr = csvModel();
    if (!dataModelPtr)
        return false;
    const QString sFilePath = dataModelPtr->getFilePath();

    if (!QFileInfo(sFilePath).isReadable())
        return false;

    auto sDifferPath = getCsvTableViewProperty<CsvTableViewPropertyId_diffProgPath>(this);
    bool bDifferPathWasAsked = false;

    if (sDifferPath.isEmpty())
    {
        const auto rv = QMessageBox::question(this,
                                              tr("Unable to locate diff viewer"),
                                              tr("Diff viewer path was not found; locate it manually?")
                                              );
        if (rv != QMessageBox::Yes)
            return false;
        const auto manuallyLocatedDiffer = getFilePathFromFileDialog(tr("Locate diff viewer"));
        if (manuallyLocatedDiffer.isEmpty())
            return false;
        // TODO: store this to in-memory property set so it doesn't need to be queried again for this run.
        sDifferPath = manuallyLocatedDiffer;
        bDifferPathWasAsked = true;
    }

    typedef DFG_MODULE_NS(io)::DFG_CLASS_NAME(OfStreamWithEncoding) StreamT;
    DFG_MODULE_NS(os)::DFG_CLASS_NAME(TemporaryFileStreamT)<StreamT> strmTemp(nullptr, // nullptr = use default temp path
                                                                              szTempFileNameTemplate,
                                                                              nullptr, // nullptr = no suffix
                                                                              ".csv" // extension
                                                                              );
    strmTemp.setAutoRemove(false); // To not remove the file while it's being used by diff viewer.
    strmTemp.stream().m_streamBuffer.m_encodingBuffer.setEncoding(DFG_MODULE_NS(io)::encodingUnknown);

    // TODO: saving can take time -> don't freeze GUI while it is being done.
    if (!dataModelPtr->save(strmTemp.stream()))
    { // Saving file unsuccessfull, can't diff.
        QMessageBox::information(this,
                                 tr("Unable to diff"),
                                 tr("Saving current temporary document for diffing failed -> unable to diff"));
        return false;
    }

    const QString sEditedFileTempPath = QString::fromUtf8(strmTemp.pathU8().c_str());

    strmTemp.close();

    const bool bStarted = QProcess::startDetached(sDifferPath,
                                                  QStringList() << sFilePath << sEditedFileTempPath);
    if (!bStarted)
    {
        QMessageBox::information(this, tr("Unable to diff"), tr("Couldn't start diff application from path '%1'").arg(sDifferPath));
        strmTemp.setAutoRemove(true);
        return false;
    }
    else
    {
        m_tempFilePathsToRemoveOnExit.push_back(QString::fromUtf8(toCharPtr_raw(strmTemp.pathU8())));

        if (bDifferPathWasAsked)
        {
            // Ask whether to store the path to settings
            const bool bIsAppSettingsEnabled = getAllowApplicationSettingsUsage();
            QMessageBox mb(QMessageBox::Question,
                tr("Merge viewer path handling"),
                tr("Use the following diff viewer on subsequent operations?\n'%1'").arg(sDifferPath)
            );
            QPushButton* pStore = nullptr;
            if (bIsAppSettingsEnabled)
                pStore = mb.addButton(tr("Yes (will be stored to app settings)"), QMessageBox::YesRole);
            else
                pStore = mb.addButton(tr("Yes (in effect for this run)"), QMessageBox::YesRole);
            mb.addButton(tr("No"), QMessageBox::NoRole);
            mb.exec();
            const auto pClickedButton = mb.clickedButton();
            if (pClickedButton == pStore)
                setCsvTableViewProperty<CsvTableViewPropertyId_diffProgPath>(this, sDifferPath);
        }
        
        return true;
    }
}

bool CsvTableView::getAllowApplicationSettingsUsage() const
{
    return property(gPropertyIdAllowAppSettingsUsage).toBool();
}

void CsvTableView::setAllowApplicationSettingsUsage(const bool b)
{
    setProperty(gPropertyIdAllowAppSettingsUsage, b);
    setReadOnlyModeFromProperty(getCsvTableViewProperty<CsvTableViewPropertyId_editMode>(this));
    Q_EMIT sigOnAllowApplicationSettingsUsageChanged(b);
}

void CsvTableView::finishEdits()
{
    const auto viewState = state();
    if (viewState == EditingState)
    {
        // For now approximate that focus widget is our editor.
        auto focusWidget = QApplication::focusWidget();
        if (focusWidget)
            focusWidget->clearFocus();
    }
}

int CsvTableView::getFindColumnIndex() const
{
    return m_nFindColumnIndex;
}

void CsvTableView::onFilterRequested()
{
    const QMetaMethod findActivatedSignal = QMetaMethod::fromSignal(&ThisClass::sigFilterActivated);
    if (isSignalConnected(findActivatedSignal))
        Q_EMIT sigFilterActivated();
    else
        showStatusInfoTip(tr("Sorry, standalone filter is not implemented."));
}

void CsvTableView::onFilterFromSelectionRequested()
{
    const auto getOption = [](const QObjectStorage<QCheckBox>& spCheckBox)
    {
        return (spCheckBox) ? spCheckBox->isChecked() : false;
    };

    const auto bCaseSensitive = getOption(DFG_OPAQUE_REF().m_spFilterCheckBoxCaseSensitive);
    const auto bWholeStringMatch = getOption(DFG_OPAQUE_REF().m_spFilterCheckBoxWholeStringMatch);
    const auto bColumnMatchByAnd = getOption(DFG_OPAQUE_REF().m_spFilterCheckBoxColumnMatchByAnd);

    CsvTableViewSelectionFilterFlags flags;
    flags.setFlag(CsvTableViewSelectionFilterFlags::Enum::caseSensitive, bCaseSensitive);
    flags.setFlag(CsvTableViewSelectionFilterFlags::Enum::wholeStringMatch, bWholeStringMatch);
    flags.setFlag(CsvTableViewSelectionFilterFlags::Enum::columnMatchByAnd, bColumnMatchByAnd);
    setFilterFromSelection(flags);
}

void CsvTableView::setFilterFromSelection(const CsvTableViewSelectionFilterFlags flags)
{
    const QMetaMethod signalMetaMethod = QMetaMethod::fromSignal(&ThisClass::sigFilterJsonRequested);
    if (!isSignalConnected(signalMetaMethod))
    {
        showStatusInfoTip(tr("Sorry, standalone filter is not implemented."));
        return;
    }
    const auto pCsvModel = csvModel();
    if (!pCsvModel)
        return;

    // Fetching all selected items
    ::DFG_MODULE_NS(cont)::MapVectorSoA<Index, ::DFG_MODULE_NS(cont)::SetVector<QString>> columnItems;
    size_t nCounter = 0;
    const size_t nMaxCellCount = 500; // Some arbitrary sanity limit so that won't try create filter out of thoundands/millions of cells.
    forEachCsvModelIndexInSelection([&](const QModelIndex& index, bool& rbContinue)
    {
        rbContinue = (nCounter < nMaxCellCount);
        const auto s = pCsvModel->data(index).toString();
        if (!s.isEmpty())
            columnItems[index.column()].insert(s);
        ++nCounter;
    });
    // Checking if there were too many selected items and bailing out if yes.
    if (nCounter > nMaxCellCount)
    {
        this->showStatusInfoTip(tr("Too many cells selected for filter: maximum limit is %1.").arg(nMaxCellCount));
        return;
    }

    // Constructing filter: every cell text is OR'ed: every column gets a dedicated json-entity and within the column
    //                      texts are OR'ed within regexp handling.
    QString sFilter;
    for (const auto& colIndexAndStrings : columnItems)
    {
        QString sTextPattern;
        for (const auto& s : colIndexAndStrings.second)
        {
            if (!sTextPattern.isEmpty())
                sTextPattern += "|";
            const char* pszFormat = (flags.testFlag(CsvTableViewSelectionFilterFlags::Enum::wholeStringMatch)) ? "^%1$" : "%1";
            sTextPattern += QString(pszFormat).arg(QRegularExpression::escape(s));
        }

        const auto nColumn = colIndexAndStrings.first;
        const auto nUserColumn = CsvItemModel::internalColumnIndexToVisible(nColumn);

        QVariantMap jsonFields;
        jsonFields["text"] = sTextPattern;
        jsonFields["type"] = "reg_exp";
        jsonFields["apply_columns"] = QString::number(nUserColumn);
        if (flags.testFlag(CsvTableViewSelectionFilterFlags::Enum::caseSensitive))
            jsonFields["case_sensitive"] = true;
        if (!flags.testFlag(CsvTableViewSelectionFilterFlags::Enum::columnMatchByAnd) && columnItems.size() > 1)
        {
            const QString sAndGroup = QString("col_%1").arg(nUserColumn);
            jsonFields["and_group"] = sAndGroup;
        }

        sFilter += QString("%1 ").arg(QString::fromUtf8(QJsonDocument::fromVariant(jsonFields).toJson(QJsonDocument::Compact)));
    }

    Q_EMIT sigFilterJsonRequested(sFilter);
}

void CsvTableView::onGoToCellTriggered()
{
    auto pModel = model();
    if (!pModel || pModel->rowCount() <= 0)
        return;
    const auto nMinRowVal = CsvModel::internalRowIndexToVisible(0);
    const auto nMaxRowVal = CsvModel::internalRowIndexToVisible(pModel->rowCount() - 1);
    const auto nMinColumnVal = CsvModel::internalColumnIndexToVisible(0) - 1;
    const auto nMaxColumnVal = CsvModel::internalColumnIndexToVisible(pModel->columnCount() - 1);

    QDialog dlg(this);
    auto pLayout = new QFormLayout(&dlg); // Deletion by parentship

    auto pSpinBoxRow = new QSpinBox(&dlg); // Deletion by parentship
    pSpinBoxRow->setRange(nMinRowVal, nMaxRowVal);
    pSpinBoxRow->setValue(nMinRowVal);
    pLayout->addRow(tr("Line number (%1 - %2) (n'th line from top)").arg(nMinRowVal).arg(nMaxRowVal), pSpinBoxRow);

    auto pSpinBoxColumn = new QSpinBox(&dlg); // Deletion by parentship
    pSpinBoxColumn->setRange(nMinColumnVal, nMaxColumnVal);
    pSpinBoxColumn->setValue(nMinColumnVal);
    pLayout->addRow(tr("Column number (%1 - %2)").arg(nMinColumnVal + 1).arg(nMaxColumnVal), pSpinBoxColumn);
    pSpinBoxColumn->setSpecialValueText(tr("Not selected"));

    pLayout->addRow(new QLabel("Note: In case of filtered or sorted table, indexes refer to visible table indexes", &dlg)); // Deletion by parentship

    pSpinBoxRow->selectAll(); // Sets focus to row edit for expected UX: Ctrl + G -> type row number -> enter.

    pLayout->addRow(addOkCancelButtonBoxToDialog(&dlg));

    removeContextHelpButtonFromDialog(&dlg);
    const auto rv = dlg.exec();
    if (rv == QDialog::Accepted)
    {
        const auto nRow = CsvModel::visibleRowIndexToInternal(pSpinBoxRow->value());
        const auto nCol = CsvModel::visibleColumnIndexToInternal(pSpinBoxColumn->value());
        if (nCol < 0)
            selectRow(nRow);
        else
            selectCell(nRow, nCol);
    }
}

void CsvTableView::onFindRequested()
{
    const QMetaMethod findActivatedSignal = QMetaMethod::fromSignal(&ThisClass::sigFindActivated);
    if (isSignalConnected(findActivatedSignal))
    {
        Q_EMIT sigFindActivated();
    }
    else
    {
        // TODO: use find panel here.
        bool bOk;
        auto s = QInputDialog::getText(this, tr("Find"), tr("Set text to search for"), QLineEdit::Normal, QString(), &bOk);
        if (bOk)
        {
            m_matchDef.setMatchString(std::move(s));
            setFindText(m_matchDef, m_nFindColumnIndex);
        }
    }
}

void CsvTableView::onFind(const bool forward)
{
    if (!m_matchDef.hasMatchString())
        return;

    auto pBaseModel = csvModel();
    if (!pBaseModel)
        return;

    // This to prevent setting invalid find column from resetting view (e.g. scroll to top).
    if (getFindColumnIndex() >= pBaseModel->getColumnCount())
    {
        QToolTip::showText(QCursor::pos(), tr("Find column index is invalid"));
        return;
    }

    const auto findSeed = [&]() -> QModelIndex
        {
            if (m_latestFoundIndex.isValid())
                return m_latestFoundIndex;
            else if (getFindColumnIndex() >= 0)
            {
                const auto current = currentIndex();
                if (current.isValid())
                    return pBaseModel->index(current.row(), getFindColumnIndex());
                else
                    return pBaseModel->index(0, getFindColumnIndex()); // Might need to improve this e.g. to return cell from top left corner of visible rect.
            }
            else
                return currentIndex();
        }();

    // TODO: this doesn't work correctly with proxy filtering: finds cells also from filter-hidden cells.
    const auto found = pBaseModel->findNextHighlighterMatch(findSeed, (forward) ? CsvModel::FindDirectionForward : CsvModel::FindDirectionBackward);

    if (found.isValid())
    {
        m_latestFoundIndex = found;
        scrollTo(mapToViewModel(m_latestFoundIndex));
        setCurrentIndex(mapToViewModel(m_latestFoundIndex));
    }
    else
        forgetLatestFindPosition();
}

void CsvTableView::onFindNext()
{
    onFind(true);
}

void CsvTableView::onFindPrevious()
{
    onFind(false);
}

class ReplaceDialog : public QDialog
{
public:
    using BaseClass = QDialog;

    ReplaceDialog(CsvTableView* pParent)
        : BaseClass(pParent)
        , m_spTableView(pParent)
    {
        auto spLayout = std::unique_ptr<QFormLayout>(new QFormLayout);
        m_spFindEdit.reset(new QLineEdit(this));
        m_spReplaceEdit.reset(new QLineEdit(this));
        spLayout->addRow(tr("Note:"), new QLabel(tr("Applies to selection only, is case sensitive and no undo available"), this));
        spLayout->addRow(tr("Find"), m_spFindEdit.get());
        spLayout->addRow(tr("Replace"), m_spReplaceEdit.get());

        auto& rButtonBox = *(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel));
        DFG_QT_VERIFY_CONNECT(connect(&rButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept));
        DFG_QT_VERIFY_CONNECT(connect(&rButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject));
        spLayout->addWidget(&rButtonBox);

        delete this->layout();
        this->setLayout(spLayout.release());
        removeContextHelpButtonFromDialog(this);
        setWindowTitle(tr("Find & Replace"));
    }

    void accept() override
    {
        if (!m_spTableView)
            return BaseClass::reject();
        QVariantMap params;
        params["find"] = m_spFindEdit->text();
        params["replace"] = m_spReplaceEdit->text();
        m_spTableView->replace(params);

        BaseClass::accept();
    }

    QObjectStorage<QLineEdit> m_spFindEdit;
    QObjectStorage<QLineEdit> m_spReplaceEdit;
    QPointer<CsvTableView> m_spTableView;
};

bool CsvTableView::isSelectionNonEmpty() const
{
    bool bNonEmptySelection = false;
    forEachCsvModelIndexInSelection([&](const QModelIndex&, bool& rbContinue)
    {
        rbContinue = false;
        bNonEmptySelection = true;
    });
    return bNonEmptySelection;
}

void CsvTableView::onReplace()
{
    if (!isSelectionNonEmpty())
    {
        QToolTip::showText(QCursor::pos(), tr("Replace requires selection"));
        return;
    }
    ReplaceDialog dlg(this);
    dlg.exec();
}

void CsvTableView::setFindText(const StringMatchDef matchDef, const int nCol)
{
    auto lockReleaser = this->tryLockForEdit();
    if (!lockReleaser.isLocked())
    {
        // Couldn't acquire lock. Scheduling a new try in 200 ms.
        QPointer<CsvTableView> thisPtr = this;
        QTimer::singleShot(200, this, [=]() { if (thisPtr) thisPtr->setFindText(matchDef, nCol); });
        return;
    }

    m_matchDef = matchDef;
    m_nFindColumnIndex = nCol;
    auto pBaseModel = csvModel();
    if (!pBaseModel)
        return;

    CsvModel::HighlightDefinition hld("te0", getFindColumnIndex(), m_matchDef);
    pBaseModel->setHighlighter(std::move(hld));

    forgetLatestFindPosition();
}

template <class Func_T>
void CsvTableView::forEachCompleterEnabledColumnIndex(Func_T func)
{
    auto pModel = csvModel();
    if (pModel)
    {
        const auto nColCount = pModel->getColumnCount();
        for (int i = 0; i < nColCount; ++i)
        {
            auto pColInfo = pModel->getColInfo(i);
            if (pColInfo->hasCompleter())
                func(i, pColInfo);
        }
    }
}

void CsvTableView::onNewSourceOpened()
{
    // Setting completer delegates to columns that have completers enabled.
    forEachCompleterEnabledColumnIndex([&](const int nCol, CsvModel::ColInfo* pColInfo)
    {
        typedef CsvTableViewCompleterDelegate DelegateClass;
        if (pColInfo)
        {
            auto existingColumnDelegate = qobject_cast<DelegateClass*>(this->itemDelegateForColumn(nCol));
            // Note: delegates live in 'this', but actual completers live in model. These custom delegates will be
            // used on all columns for which completion has been enabled on any of the models opened, but actual completer
            // objects will be available only from current model; in other columns the weak reference to completer object
            // will be null and the delegate fallbacks to behaviour without completer.
            if (!existingColumnDelegate)
            {
                auto* pDelegate = new CsvTableViewCompleterDelegate(this, pColInfo->m_spCompleter.get());
                setItemDelegateForColumn(nCol, pDelegate); // Does not transfer ownership, delegate is parent owned by 'this'.
            }
            else // If there already is an delegate, update the completer.
                existingColumnDelegate->m_spCompleter = pColInfo->m_spCompleter.get();
        }
    });

    // Setting view-related properties from load options.
    auto pCsvModel = this->csvModel();
    if (pCsvModel)
    {
        const auto loadOptions = pCsvModel->getOpenTimeLoadOptions();

        // Setting edit mode if present in load options
        setReadOnlyModeFromProperty(loadOptions.getProperty(CsvOptionProperty_editMode, ""));
    }
}

namespace
{
class WidgetPair : public QWidget
{
public:
    typedef QWidget BaseClass;

protected:
    WidgetPair(QWidget* pParent);

public:
    ~WidgetPair();

    static std::unique_ptr<WidgetPair> createHorizontalPair(QWidget* pParent,
                                                            QWidget* pFirst,
                                                            QWidget* pSecond,
                                                            const bool reparentWidgets = true);

    static std::unique_ptr<WidgetPair>createHorizontalLabelLineEditPair(QWidget* pParent,
                                                                         QString sLabel,
                                                                         const QString& sLineEditPlaceholder = QString());

    QHBoxLayout* m_pLayout = nullptr;
    QWidget* m_pFirst = nullptr;
    QWidget* m_pSecond = nullptr;
}; // class WidgetPair

WidgetPair::WidgetPair(QWidget* pParent) :
    BaseClass(pParent)
{
}

WidgetPair::~WidgetPair()
{

}

std::unique_ptr<WidgetPair> WidgetPair::createHorizontalPair(QWidget* pParent,
                                                        QWidget* pFirst,
                                                        QWidget* pSecond,
                                                        const bool reparentWidgets)
{
    std::unique_ptr<WidgetPair> spWp(new WidgetPair(pParent));
    spWp->m_pLayout = new QHBoxLayout(spWp.get());
    if (reparentWidgets)
    {
        if (pFirst)
            pFirst->setParent(spWp.get());
        if (pSecond)
            pSecond->setParent(spWp.get());

    }
    spWp->m_pFirst = pFirst;
    spWp->m_pSecond = pSecond;

    spWp->m_pLayout->addWidget(spWp->m_pFirst);
    spWp->m_pLayout->addWidget(spWp->m_pSecond);
    return spWp;
}

std::unique_ptr<WidgetPair> WidgetPair::createHorizontalLabelLineEditPair(QWidget* pParent,
                                                                     QString sLabel,
                                                                     const QString& sLineEditPlaceholder)
{
    auto pLabel = new QLabel(sLabel);
    auto pLineEdit = new QLineEdit(sLineEditPlaceholder);
    return createHorizontalPair(pParent, pLabel, pLineEdit);
}

} // unnamed namespace

DFG_OPAQUE_PTR_DEFINE(CsvTableViewBasicSelectionAnalyzerPanel)
{
    CsvTableViewBasicSelectionAnalyzerPanel::CollectorContainerPtr m_spCollectors;

    static void addDetailMenuEntry(CsvTableViewBasicSelectionAnalyzerPanel& rPanel, SelectionDetailCollector& rCollector, QMenu& rMenu)
    {
        auto pCheckBox = rCollector.createCheckBox(&rMenu);
        if (pCheckBox)
        {
            pCheckBox->setText(rCollector.getUiName_long());
            pCheckBox->setChecked(rCollector.isEnabled());
            auto pCollector = &rCollector;
            DFG_QT_VERIFY_CONNECT(connect(pCheckBox, &QCheckBox::toggled, pCheckBox, [=](const bool b)
            {
                // Since CheckBox is owned by pCollector, this handler should never get called if pCollector has been deleted.
                pCollector->enable(b, false);
            }));

            // Creating tooltip text
            rCollector.updateCheckBoxToolTip();

            const auto addAction = [&](const QString& s, std::function<void()> slotHandler)
            {
                auto pAct = new QAction(s, &rMenu); // Deletion through parentship
                DFG_QT_VERIFY_CONNECT(connect(pAct, &QAction::triggered, pCheckBox, slotHandler));
                pCheckBox->addAction(pAct);
            };

            // Adding context menu actions
            addAction(tr("Reset result precision to default"), [=]() { pCollector->deleteProperty(SelectionDetailCollector::s_propertyName_resultPrecision); pCollector->updateCheckBoxToolTip(); });
            addAction(tr("Set result precision..."), [=]()
                {
                    bool bOk;
                    const auto nNew = QInputDialog::getInt(pCollector->getCheckBoxPtr(),
                                         tr("New precision"),
                                         tr("New precision for collector '%1'").arg(pCollector->getUiName_long()),
                                         pCollector->getProperty(SelectionDetailCollector::s_propertyName_resultPrecision, -1).toInt(),
                                         -1, 999, 1, &bOk);
                    if (!bOk)
                        return;
                    pCollector->setProperty(SelectionDetailCollector::s_propertyName_resultPrecision, nNew);
                    pCollector->updateCheckBoxToolTip();
                });

            if (!rCollector.isBuiltIn())
            {
                const auto sId = rCollector.id().toString();
                QPointer<CsvTableViewBasicSelectionAnalyzerPanel> spPanel = &rPanel;

                // Delete action
                addAction(tr("Delete"), [=]() { if (spPanel) spPanel->deleteDetail(sId); });
                // Export to clipboard
                addAction(tr("Export definition to clipboard"), [=]()
                    { 
                        auto pClipboard = QApplication::clipboard();
                        if (pClipboard)
                            pClipboard->setText(pCollector->exportDefinitionToJson());
                    });
            }
            pCheckBox->setContextMenuPolicy(Qt::ActionsContextMenu);
        }
        rMenu.addAction(rCollector.getCheckBoxAction());
    }

    static void removeDetailMenuEntry(SelectionDetailCollector& rCollector, QMenu& rMenu)
    {
        rMenu.removeAction(rCollector.getCheckBoxAction());
    }

    std::atomic_int m_nDefaultNumericPrecision { -1 };
}; // CsvTableViewBasicSelectionAnalyzerPanel opaque class 

CsvTableViewBasicSelectionAnalyzerPanel::CsvTableViewBasicSelectionAnalyzerPanel(QWidget *pParent) :
    BaseClass(pParent)
{
    auto layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    int column = 0;

    // Title-Label
    layout->addWidget(new QLabel(tr("Selection"), this), 0, column++);

    // Value display
    m_spValueDisplay.reset(new QLineEdit(this));
    m_spValueDisplay->setReadOnly(true);
    layout->addWidget(m_spValueDisplay.get(), 0, column++);

    // Detail selector
    {
        m_spDetailSelector.reset(new QToolButton(this));
        m_spDetailSelector->setPopupMode(QToolButton::InstantPopup);
        m_spDetailSelector->setText(tr("Details"));
        auto pMenu = new QMenu(this); // Deletion through parentship
        using DetailCollector = ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::BasicSelectionDetailCollector;
        const auto defaultFlags = DetailCollector::defaultEnableFlags();
        DFG_OPAQUE_REF().m_spCollectors = std::make_shared<SelectionDetailCollectorContainer>();

        // Generic items such as enable/disable all
        {
            {
                auto pAct = pMenu->addAction(tr("Enable all"));
                DFG_QT_VERIFY_CONNECT(connect(pAct, &QAction::triggered, this, &CsvTableViewBasicSelectionAnalyzerPanel::onEnableAllDetails));
            }
            {
                auto pAct = pMenu->addAction(tr("Disable all"));
                DFG_QT_VERIFY_CONNECT(connect(pAct, &QAction::triggered, this, &CsvTableViewBasicSelectionAnalyzerPanel::onDisableAllDetails));
            }
            {
                auto pAct = pMenu->addAction(tr("Set default numeric precision..."));
                DFG_QT_VERIFY_CONNECT(connect(pAct, &QAction::triggered, this, &CsvTableViewBasicSelectionAnalyzerPanel::onQueryDefaultNumericPrecision));
            }
        }

        addSectionEntryToMenu(pMenu, "Built-in");

        DetailCollector::forEachBuiltInDetailIdWhile([&](const DetailCollector::BuiltInDetail id)
        {
            // https://stackoverflow.com/questions/2050462/prevent-a-qmenu-from-closing-when-one-of-its-qaction-is-triggered

            DFG_OPAQUE_REF().m_spCollectors->push_back(std::make_shared<SelectionDetailCollector>(::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::BasicSelectionDetailCollector::builtInDetailToStrId(id).toString()));

            auto pCollector = DFG_OPAQUE_REF().m_spCollectors->back().get();
            const auto bEnabled = DetailCollector::isEnabled(defaultFlags, id);
            pCollector->enable(bEnabled);
            pCollector->setProperty("ui_name_short", DetailCollector::uiName_short(id));
            pCollector->setProperty("ui_name_long", DetailCollector::uiName_long(id));

            DFG_OPAQUE_REF().addDetailMenuEntry(*this, *pCollector, *pMenu);
            return true;
        }); // For each built-in detail ID

        addSectionEntryToMenu(pMenu, "Custom");

        // Adding "Add..." action
        {
            auto pActCustom = pMenu->addAction(tr("Add..."));
            DFG_QT_VERIFY_CONNECT(connect(pActCustom, &QAction::triggered, this, &CsvTableViewBasicSelectionAnalyzerPanel::onAddCustomCollector));
        }

        m_spDetailSelector->setMenu(pMenu); // Does not transfer ownership
        layout->addWidget(m_spDetailSelector.get(), 0, column++);
    }

    // Progress bar
    m_spProgressBar.reset(new QProgressBar(this));
    m_spProgressBar->setMaximumWidth(75);
    layout->addWidget(m_spProgressBar.get(), 0, column++);

    // Stop button
    m_spStopButton.reset(new QPushButton(tr("Stop"), this));
    m_spStopButton->setEnabled(false); // To be enabled when evaluation is ongoing.
    m_spStopButton->setCheckable(true);
    layout->addWidget(m_spStopButton.get(), 0, column++);

    // 'Maximum time'-control
    auto maxTimeControl = WidgetPair::createHorizontalLabelLineEditPair(this, tr("Limit (s)"));
    maxTimeControl->setMaximumWidth(100);
    if (maxTimeControl->m_pFirst)
        maxTimeControl->m_pFirst->setMaximumWidth(50);
    if (maxTimeControl->m_pSecond)
    {
        m_spTimeLimitDisplay = qobject_cast<QLineEdit*>(maxTimeControl->m_pSecond);
        if (m_spTimeLimitDisplay)
        {
            m_spTimeLimitDisplay->setText("1");
            m_spTimeLimitDisplay->setMaximumWidth(50);
        }
    }
    layout->addWidget(maxTimeControl.release(), 0, column++);

    DFG_QT_VERIFY_CONNECT(connect(this, &ThisClass::sigEvaluationStartingHandleRequest, this, &ThisClass::onEvaluationStarting_myThread));
    DFG_QT_VERIFY_CONNECT(connect(this, &ThisClass::sigEvaluationEndedHandleRequest, this, &ThisClass::onEvaluationEnded_myThread));
    DFG_QT_VERIFY_CONNECT(connect(this, &ThisClass::sigSetValueDisplayString, this, &ThisClass::setValueDisplayString_myThread));
}

CsvTableViewBasicSelectionAnalyzerPanel::~CsvTableViewBasicSelectionAnalyzerPanel() = default;

void CsvTableViewBasicSelectionAnalyzerPanel::setValueDisplayString(const QString& s)
{
    Q_EMIT sigSetValueDisplayString(s);
}

void CsvTableViewBasicSelectionAnalyzerPanel::setValueDisplayString_myThread(const QString& s)
{
    if (m_spValueDisplay)
        m_spValueDisplay->setText(s);
}

void CsvTableViewBasicSelectionAnalyzerPanel::onEvaluationStarting(const bool bEnabled)
{
    Q_EMIT sigEvaluationStartingHandleRequest(bEnabled);
}

void CsvTableViewBasicSelectionAnalyzerPanel::onEvaluationStarting_myThread(const bool bEnabled)
{
    if (bEnabled)
    {
        setValueDisplayString(tr("Working..."));
        if (m_spProgressBar)
        {
            m_spProgressBar->setVisible(true);

            // Sets text to be shown inside the progress bar, the actual text-align value seems to have no effect.
            m_spProgressBar->setStyleSheet("QProgressBar::chunk { text-align: left; }");
            m_spProgressBar->setRange(0, 0); // Activates generic 'something is happening' indicator.
            m_spProgressBar->setValue(-1);
        }
        if (m_spStopButton)
        {
            m_spStopButton->setEnabled(true);
            m_spStopButton->setChecked(false);
        }
    }
    else
    {
        setValueDisplayString(tr("Disabled"));
        if (m_spProgressBar)
            m_spProgressBar->setVisible(false);
    }
}

void CsvTableViewBasicSelectionAnalyzerPanel::onEvaluationEnded(const double timeInSeconds, const DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)::CompletionStatus completionStatus)
{
    Q_EMIT sigEvaluationEndedHandleRequest(timeInSeconds, static_cast<int>(completionStatus));
}

void CsvTableViewBasicSelectionAnalyzerPanel::onEvaluationEnded_myThread(const double timeInSeconds, const int completionStatus)
{
    if (m_spProgressBar)
    {
        m_spProgressBar->setRange(0, 1); // Deactivates generic 'something is happening' indicator.
        m_spProgressBar->setValue(m_spProgressBar->maximum());
        m_spProgressBar->setFormat(QString("%1 s").arg(timeInSeconds));
        if (completionStatus != static_cast<int>(DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)::CompletionStatus_completed))
        {
            // When work gets interrupted, set color to red.
            m_spProgressBar->setStyleSheet("QProgressBar::chunk { background-color: #FF0000; text-align: left; }");
        }
    }
    if (m_spStopButton)
        m_spStopButton->setEnabled(false);
}

void CsvTableViewBasicSelectionAnalyzerPanel::onAddCustomCollector()
{
    const auto sLabel = tr("New selection detail. Description of fields:<br>"
        "Common fields:"
        "<ul>"
            "<li><b>%3:</b> Short name version to show in UI.</li>"
            "<li><b>%4:</b> Long name version to show in UI. if omitted, short name will be used</li>"
            "<li><b>%5:</b> A more detailed description of the new detail. Can be omitted</li>"
            "<li><b>%6:</b> Numerical precision of result in range [-1, 999]. Can be omitted, -1 means roundtrippable precision</li>"
        "</ul>"
        "Formula accumulator:"
        "<ul>"
            "<li><b>%1:</b> Formula used to calculate the value, predefined variables 'acc' and 'value' are available : 'acc' is current accumulant value and 'value' is current cell value</li>"
            "<li><b>%2:</b> Initial value of the accumulant, for example when defining a sum accumulant, initial value is typically 0.</li>"
        "</ul>"
        "Percentile (using MemFuncPercentile_enclosingElem):"
        "<ul>"
            "<li><b>%7:</b> percentage (range [0, 100]).</li>"
        "</ul>")
        .arg(SelectionDetailCollector_formula::s_propertyName_formula,
             SelectionDetailCollector_formula::s_propertyName_initialValue,
             SelectionDetailCollector::s_propertyName_uiNameShort,
             SelectionDetailCollector::s_propertyName_uiNameLong,
             SelectionDetailCollector::s_propertyName_description,
             SelectionDetailCollector::s_propertyName_resultPrecision,
             SelectionDetailCollector_percentile::s_propertyName_percentage);

    const QString sExampleSquareSum = tr(R"({ "%4": "SquareSum", "%1": "acc + value^2", "%2": "0", "%3": "Calculates sum of squares" })")
        .arg(SelectionDetailCollector_formula::s_propertyName_formula,
             SelectionDetailCollector_formula::s_propertyName_initialValue,
             SelectionDetailCollector::s_propertyName_description,
             SelectionDetailCollector::s_propertyName_uiNameShort);
    const QString sExamplePercentile = tr(R"({ "%1": "10", "%2": "10th percentile" })")
        .arg(SelectionDetailCollector_percentile::s_propertyName_percentage, SelectionDetailCollector::s_propertyName_description);

    const QString sExamples = tr("# Examples:\n#    %1\n#    %2\n\n").arg(sExampleSquareSum, sExamplePercentile);
        
    QString sJson = tr("%7# Below is a template for formula accumulator\n{\n  \"%1\": \"\",\n  \"%2\": \"\",\n  \"%3\": \"\",\n  \"%4\": \"\",\n  \"%5\": \"\",\n  \"%6\": \"\"\n}")
                    .arg(SelectionDetailCollector_formula::s_propertyName_formula,
                         SelectionDetailCollector_formula::s_propertyName_initialValue,
                         SelectionDetailCollector::s_propertyName_uiNameShort,
                         SelectionDetailCollector::s_propertyName_uiNameLong,
                         SelectionDetailCollector::s_propertyName_description,
                         SelectionDetailCollector::s_propertyName_resultPrecision,
                         sExamples
                        );
    QJsonDocument jsonDoc;
    while (true) // Asking definition until getting valid json or cancel.
    {
        sJson = InputDialog::getMultiLineText(
            this,
            tr("New selection detail"),
            sLabel,
            sJson
        );

        if (sJson.isEmpty())
            return;

        // Removing leading commented lines
        auto lines = sJson.split('\n');
        while (!lines.empty() && lines.front().startsWith("#"))
            lines.pop_front();

        sJson = lines.join('\n');

        QJsonParseError parseError;
        jsonDoc = QJsonDocument::fromJson(sJson.toUtf8(), &parseError);
        if (jsonDoc.isNull()) // Parsing failed?
        {
            QMessageBox::information(this, tr("Invalid json format"), tr("Parsing json failed with error: '%1'").arg(parseError.errorString()));
            continue;
        }
        break;
    }

    auto inputs = jsonDoc.toVariant().toMap();
    // If there's no type-property, deducing it from other fields and adding it to data map
    if (!inputs.contains(SelectionDetailCollector::s_propertyName_type))
    {
        if (inputs.contains(SelectionDetailCollector_formula::s_propertyName_formula))
            inputs[SelectionDetailCollector::s_propertyName_type] = QString("accumulator");
        else if (inputs.contains(SelectionDetailCollector_percentile::s_propertyName_percentage))
        {
            inputs[SelectionDetailCollector::s_propertyName_type] = QString("percentile");
            // If there's no short name, using default name "percentile_<percentage>.
            if (!inputs.contains(SelectionDetailCollector::s_propertyName_uiNameShort))
                inputs[SelectionDetailCollector::s_propertyName_uiNameShort] = QString("percentile_%1").arg(inputs[SelectionDetailCollector_percentile::s_propertyName_percentage].toString());
        }
    }

    if (!addDetail(inputs))
    {
        QMessageBox::warning(this, tr("Adding failed"), tr("Adding new detail failed"));
    }
}

void CsvTableViewBasicSelectionAnalyzerPanel::setEnableStatusForAll(const bool b)
{
    auto spCollectors = collectors();
    if (!spCollectors)
        return;
    spCollectors->setEnableStatusForAll(b);
}

void CsvTableViewBasicSelectionAnalyzerPanel::onEnableAllDetails()
{
    setEnableStatusForAll(true);
}

void CsvTableViewBasicSelectionAnalyzerPanel::onDisableAllDetails()
{
    setEnableStatusForAll(false);
}

void CsvTableViewBasicSelectionAnalyzerPanel::onQueryDefaultNumericPrecision()
{
    const auto nOld = defaultNumericPrecision();
    bool bOk = false;
    const auto nNew = QInputDialog::getInt(this, tr("Default numeric precision"), tr("New numeric precision\n(default roundtrippable precision can be set with -1)"), nOld, -1, 999, 1, &bOk);
    if (!bOk)
        return;
    setDefaultNumericPrecision(nNew);
}

double CsvTableViewBasicSelectionAnalyzerPanel::getMaxTimeInSeconds() const
{
    bool bOk = false;
    double val = 0;
    if (m_spTimeLimitDisplay)
        val = m_spTimeLimitDisplay->text().toDouble(&bOk);
    return (bOk) ? val : -1.0;
}

bool CsvTableViewBasicSelectionAnalyzerPanel::isStopRequested() const
{
    return (m_spStopButton && m_spStopButton->isChecked());
}

void CsvTableViewBasicSelectionAnalyzerPanel::clearAllDetails()
{
    setEnableStatusForAll(false);
}

bool CsvTableViewBasicSelectionAnalyzerPanel::addDetail(const QVariantMap& items)
{
    // Expected fields
    //      id             : identifier of the detail, must be unique.
    //      [type]         : Type of collector, either empty for built-in ones, "accumulator" or "percentile".
    //      [initial_value]: Initial value for collector if it needs one
    //                          -Needed by: accumulator
    //      [formula]      : Formula used to compute values if collector needs one
    //                          -Needed by: accumulator. Formula has variables "acc" (=accumulator value) and "value" (=value of new cell):
    //      [ui_name_short]: Short UI name for the detail
    //      [ui_name_long] : Long UI name for the detail
    //      [description]  : Longer description of the detail, shown as tooltip.
    //      [result_precision]: Numerical precision of string representation of collector's result
    // Example : { "id": "square_sum", "type": "accumulator", "initial_value": "0", "formula": "acc + value^2", "ui_name_short": "Sum^2", "ui_name_long": "Sum of squares", "description": "This detail shows the sum of squared values" }

    if (!DFG_OPAQUE_REF().m_spCollectors)
        return false;

    auto& collectors = *DFG_OPAQUE_REF().m_spCollectors;

    auto idQString = items.value("id").toString();
    if (idQString.isEmpty())
    {
        // If there's no id, generating random 4 character alphabetical id
        idQString = []()
        {
            using namespace ::DFG_MODULE_NS(rand);
            QString s(4, '\0');
            auto randEng = createDefaultRandEngineRandomSeeded();
            auto distrEng = makeDistributionEngineUniform(&randEng, int('a'), int('z'));
            std::generate(s.begin(), s.end(), [&]() { return QChar(distrEng()); } );
            return s;
        }();
    }
    const auto id = qStringToStringUtf8(idQString);
    const bool bEnabled = items.value("enabled", true).toBool();
    using Detail = ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::BasicSelectionDetailCollector::BuiltInDetail;
    const auto sType = items.value("type").toString();
    const auto nPrecision = items.value(SelectionDetailCollector::s_propertyName_resultPrecision, -2).toInt();
    if (sType.isEmpty())
    {
        const auto detail = ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::BasicSelectionDetailCollector::selectionDetailNameToId(id);
        if (detail == Detail::detailCount)
            return false;

        auto pExisting = collectors.find(id);
        if (!pExisting)
            return false; // Built-in details should already be created.
        pExisting->enable(bEnabled);
        if (nPrecision != -2)
            pExisting->setProperty(SelectionDetailCollector::s_propertyName_resultPrecision, nPrecision);

        pExisting->updateCheckBoxToolTip();
    }
    else if (sType == QLatin1String("accumulator") || sType == QLatin1String("percentile"))
    {
        auto pExisting = collectors.find(id);
        if (pExisting)
        {
            // If exists, for now just updating enable-flag.
            pExisting->enable(bEnabled);
            return true;
        }

        const auto sDescription = items.value(SelectionDetailCollector::s_propertyName_description).toString();
        const auto sUiNameShort = items.value(SelectionDetailCollector::s_propertyName_uiNameShort).toString();
        const auto sUiNameLong = items.value(SelectionDetailCollector::s_propertyName_uiNameLong).toString();

        std::shared_ptr<SelectionDetailCollector> spNewCollector;

        if (sType == QLatin1String("accumulator"))
        {
            const auto sInitialValueQString = items.value(SelectionDetailCollector_formula::s_propertyName_initialValue).toString();
            const auto sInitialValue = qStringToStringUtf8(sInitialValueQString);
            bool bOk = false;
            const auto initialValue = ::DFG_MODULE_NS(str)::strTo<double>(sInitialValue.rawStorage(), &bOk);
            if (!bOk)
                return false;

            const auto sFormulaQString = items.value(SelectionDetailCollector_formula::s_propertyName_formula).toString();
            const auto sFormula = qStringToStringUtf8(sFormulaQString);
            if (sFormula.empty())
                return false;

            spNewCollector = std::make_shared<SelectionDetailCollector_formula>(id, sFormula, initialValue);
            spNewCollector->setProperty(SelectionDetailCollector_formula::s_propertyName_formula, sFormulaQString);
            spNewCollector->setProperty(SelectionDetailCollector_formula::s_propertyName_initialValue, sInitialValueQString);
        }
        else if (sType == QLatin1String("percentile"))
        {
            bool bOkToDouble;
            const auto sPercentage = items.value(SelectionDetailCollector_percentile::s_propertyName_percentage).toString();
            const auto percentage = sPercentage.toDouble(&bOkToDouble);
            if (bOkToDouble)
            {
                spNewCollector = std::make_shared<SelectionDetailCollector_percentile>(id, percentage);
                spNewCollector->setProperty(SelectionDetailCollector_percentile::s_propertyName_percentage, sPercentage);
            }
        }

        if (!spNewCollector)
            return false;

        spNewCollector->setProperty(SelectionDetailCollector::s_propertyName_description, sDescription);
        spNewCollector->setProperty(SelectionDetailCollector::s_propertyName_type, sType);
        spNewCollector->setProperty(SelectionDetailCollector::s_propertyName_uiNameShort, sUiNameShort);
        spNewCollector->setProperty(SelectionDetailCollector::s_propertyName_uiNameLong, sUiNameLong);

        if (nPrecision != -2)
            spNewCollector->setProperty(SelectionDetailCollector::s_propertyName_resultPrecision, nPrecision);

        auto pMenu = (m_spDetailSelector) ? m_spDetailSelector->menu() : nullptr;
        if (pMenu)
            DFG_OPAQUE_REF().addDetailMenuEntry(*this, *spNewCollector, *pMenu);

        auto spNewCollectorSet = std::make_shared<SelectionDetailCollectorContainer>(collectors);
        spNewCollectorSet->push_back(std::move(spNewCollector));
        std::atomic_store(&DFG_OPAQUE_REF().m_spCollectors, std::move(spNewCollectorSet));
    }
    else
        return false;

    return true;
}

bool CsvTableViewBasicSelectionAnalyzerPanel::deleteDetail(const StringViewUtf8& id)
{
    if (!DFG_OPAQUE_REF().m_spCollectors)
        return false;

    auto& collectors = *DFG_OPAQUE_REF().m_spCollectors;

    auto pExisting = collectors.find(id);

    if (!pExisting || pExisting->isBuiltIn())
        return false; // Didn't find requested collector or it is a built-in

    if (QMessageBox::question(this, tr("Confirm deletion"), tr("Delete selection detail '%1'?").arg(pExisting->getUiName_short())) != QMessageBox::Yes)
        return false;

    auto spNewCollectorSet = std::make_shared<SelectionDetailCollectorContainer>(collectors);
    if (spNewCollectorSet->erase(id))
    {
        // Removing related action from 'detail button' context menu.
        auto pMenu = (m_spDetailSelector) ? m_spDetailSelector->menu() : nullptr;
        if (pMenu)
            DFG_OPAQUE_REF().removeDetailMenuEntry(*pExisting, *pMenu);

        std::atomic_store(&DFG_OPAQUE_REF().m_spCollectors, std::move(spNewCollectorSet));
        return true;
    }
    else
        return false;
}

void CsvTableViewBasicSelectionAnalyzerPanel::setDefaultDetails()
{
    if (!DFG_OPAQUE_REF().m_spCollectors)
        return;
    const auto defaultFlags = ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::BasicSelectionDetailCollector::defaultEnableFlags();
    for (auto& spItem : *DFG_OPAQUE_REF().m_spCollectors)
    {
        if (!spItem)
            continue;
        const auto builtInDetailId = ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::BasicSelectionDetailCollector::selectionDetailNameToId(spItem->id());
        if (builtInDetailId == ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::BasicSelectionDetailCollector::BuiltInDetail::detailCount)
            continue;
        spItem->enable(defaultFlags.test(static_cast<size_t>(builtInDetailId)));
    }
}

void CsvTableViewBasicSelectionAnalyzerPanel::setDefaultNumericPrecision(const int nDefaultPrecision)
{
    DFG_OPAQUE_REF().m_nDefaultNumericPrecision = limited(nDefaultPrecision, -1, 999);
}

int CsvTableViewBasicSelectionAnalyzerPanel::defaultNumericPrecision() const
{
    auto pOpaq = DFG_OPAQUE_PTR();
    return (pOpaq) ? pOpaq->m_nDefaultNumericPrecision.load() : -1;
}

auto CsvTableViewBasicSelectionAnalyzerPanel::collectors() const -> CollectorContainerPtr
{
    auto pOpaq = DFG_OPAQUE_PTR();
    if (!pOpaq)
        return nullptr;
    return std::atomic_load(&pOpaq->m_spCollectors);
}

auto CsvTableViewBasicSelectionAnalyzerPanel::detailConfigsToString() const -> QString
{
    auto spCollectors = collectors();
    if (!spCollectors)
        return QString();
    QString s;
    for (const auto& spCollector : *spCollectors)
    {
        if (!spCollector || !spCollector->isEnabled())
            continue;
        if (!s.isEmpty())
            s += "\n";
        s += spCollector->exportDefinitionToJson(true /*= include id*/, true /* single line */);
    }
    return s;
}

void CsvTableView::onSelectionModelChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    onSelectionModelOrContentChanged(selected, deselected, QItemSelection());
}

void CsvTableView::onViewModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    DFG_UNUSED(roles);
    // For now not checking if change actually happened within selection, to be improved later if there are practical cases where content gets changed outside the selection.
    onSelectionModelOrContentChanged(QItemSelection(), QItemSelection(), QItemSelection(topLeft, bottomRight));
}

void CsvTableView::onSelectionModelOrContentChanged(const QItemSelection& selected, const QItemSelection& deselected, const QItemSelection& editedViewModelItems)
{
    Q_EMIT sigSelectionChanged(selected, deselected, editedViewModelItems);

    if (m_selectionAnalyzers.empty())
        return;

    const auto sm = selectionModel();
    const auto selection = (sm) ? sm->selection() : QItemSelection();

    // If analyzer threads has not been created yet, create them now.
    // TODO: using threads is likely error prone: user could probably do modifications during processing that would make analyzers malfunctions.
    if (m_analyzerThreads.empty())
    {
        // TODO: make max thread count customisable.
        // If ideal thread count is T, there will be at max [T - 1] threads for analyzers. In total 1 thread for UI + analyzers threads.
        // Concrete examples with 4 logical processors and N analyzers:
        //      -N = 1 or 2 or 3: 1 thread for UI, N thread(s) for analyzers
        //      -N >= 4, 1 thread for UI, 3 thread for analyzers
        const auto nIdealThreadCount = Max(1, QThread::idealThreadCount() - 1);
        const auto nThreadCount = Min(nIdealThreadCount, static_cast<int>(m_selectionAnalyzers.size()));
        for (int i = 0; i < nThreadCount; ++i)
        {
            auto pThread = new QThread(this); // Ownership through parentship.
            pThread->setObjectName("selectionAnalyzer" + QString::number(i));
            m_analyzerThreads.push_back(pThread);
        }

        // Distributing analyzers to threads evenly. If there are more analyzers than threads, first threads may get one analyzer more than latter ones.
        int nThread = 0;
        forEachSelectionAnalyzer([&](SelectionAnalyzer& analyzer) { analyzer.moveToThread(m_analyzerThreads[nThread++ % m_analyzerThreads.size()].get()); });

        // Starting analyzer threads.
        forEachSelectionAnalyzerThread([](QThread& thread) { thread.start(); });
    }

    // Add selection to every analyzer's queue.
    forEachSelectionAnalyzer([&](SelectionAnalyzer& analyzer) { analyzer.addSelectionToQueue(selection); });
}

void CsvTableView::addSelectionAnalyzer(std::shared_ptr<SelectionAnalyzer> spAnalyzer)
{
    if (!spAnalyzer)
        return;
    spAnalyzer->m_spView = this;
    if (m_selectionAnalyzers.end() == std::find(m_selectionAnalyzers.begin(), m_selectionAnalyzers.end(), spAnalyzer)) // If not already present?
        m_selectionAnalyzers.push_back(std::move(spAnalyzer));
}

void CsvTableView::removeSelectionAnalyzer(const SelectionAnalyzer* pAnalyzer)
{
    auto iter = std::find_if(m_selectionAnalyzers.begin(), m_selectionAnalyzers.end(), [=](const std::shared_ptr<SelectionAnalyzer>& sp)
    {
        return (sp.get() == pAnalyzer);
    });
    if (iter != m_selectionAnalyzers.end())
        m_selectionAnalyzers.erase(iter);
}

QModelIndex CsvTableView::mapToViewModel(const QModelIndex& index) const
{
    const auto pIndexModel = index.model();
    if (pIndexModel == model())
        return index;
    else if (pIndexModel == csvModel() && getProxyModelPtr())
        return getProxyModelPtr()->mapFromSource(index);
    else
        return QModelIndex();
}

QModelIndex CsvTableView::mapToDataModel(const QModelIndex& index) const
{
    const auto pIndexModel = index.model();
    if (pIndexModel == csvModel())
        return index;
    else if (pIndexModel == model() && getProxyModelPtr())
        return getProxyModelPtr()->mapToSource(index);
    else
        return QModelIndex();
}

void CsvTableView::forgetLatestFindPosition()
{
    m_latestFoundIndex = QModelIndex();
}

void CsvTableView::onColumnResizeAction_toViewEvenly()
{
    auto pViewPort = viewport();
    auto pHorizontalHeader = horizontalHeader();
    const auto nColCount = pHorizontalHeader ? pHorizontalHeader->count() : 0;
    if (!pViewPort || nColCount < 1)
        return;
    const auto w = pViewPort->width();
    const int sectionSize = Max(w / nColCount, getCsvTableViewProperty<CsvTableViewPropertyId_minimumVisibleColumnWidth>(this));
    pHorizontalHeader->setDefaultSectionSize(sectionSize);
}

void CsvTableView::onColumnResizeAction_toViewContentAware()
{
    auto pHeader = horizontalHeader();
    auto pViewPort = viewport();
    const auto numHeader = (pHeader) ? pHeader->count() : 0;
    if (!pHeader || !pViewPort || numHeader < 1)
        return;

    const auto minSectionSize = pHeader->minimumSectionSize();

    DFG_MODULE_NS(cont)::DFG_CLASS_NAME(ValueVector)<int> sizes;
    sizes.reserve(static_cast<size_t>(numHeader));
    for (int i = 0; i < numHeader; ++i)
    {
        // Note: using only content hints, to take column header width into account, use pHeader->sectionSizeHint(i);
        sizes.push_back(Max(minSectionSize, sizeHintForColumn(i)));
        //sizes.push_back(pHeader->sectionSizeHint(i));
    }
    // Using int64 in accumulate to avoid possibility for integer overflow.
    const int64 nHintTotalWidth = DFG_MODULE_NS(numeric)::accumulate(sizes, int64(0));
    const auto nAvailableTotalWidth = pViewPort->width();

    if (nHintTotalWidth < nAvailableTotalWidth)
    {
        // Case: content requirements are less than available window width.
        // Action: increase width for every column.
        auto nFreePixels = static_cast<int>(nAvailableTotalWidth - nHintTotalWidth);
        const auto nIncrement = nFreePixels / numHeader;
        for (int i = 0; i < numHeader; ++i)
        {
            pHeader->resizeSection(i, sizes[i] + nIncrement);
            nFreePixels -= nIncrement;
        }
        if (nFreePixels > 0) // If division wasn't even, add remainder to last column
            pHeader->resizeSection(numHeader - 1, pHeader->sectionSize(numHeader - 1) + nFreePixels);
        return;
    }
    else // case: content width >= available space.
    {
        /* Rough description of logics: resize all 'less than average'-width columns to content, for others
           distribute remaining space evenly.

         1. Calculate truncation limit as average width available for remaining columns.
         2. Go through columns and if there is one whose needed width is less than truncation limit,
            resize it, mark it handled and goto 1. Note that this operation may free width for remaining columns
            so on next round the truncation limit may be greater.
         3. If all remaining columns have size greater than truncation limit, distribute available width evenly for those.
         */

        int numUnhandled = numHeader;
        int nAvailableWidth = nAvailableTotalWidth;
        while (numUnhandled > 0)
        {
            // 1.
            const int truncationLimit = nAvailableWidth / numUnhandled;
            bool bHandledOne = false;
            // 2.
            for (int i = 0; i < numHeader; ++i)
            {
                if (sizes[i] < 0)
                    continue; // Column already handled.
                if (sizes[i] <= truncationLimit)
                {
                    pHeader->resizeSection(i, sizes[i]);
                    numUnhandled--;
                    nAvailableWidth -= sizes[i];
                    sizes[i] = -1; // Using -1 as "already handled"-indicator.
                    bHandledOne = true;
                    break;
                }
            }
            if (bHandledOne)
                continue;
            // 3.
            // All headers were wider than truncation limit -> distribute space evenly.
            for (int i = 0; i < numHeader; ++i)
            {
                if (sizes[i] > truncationLimit)
                {
                    numUnhandled--;
                    nAvailableWidth -= truncationLimit;
                    if (nAvailableWidth >= truncationLimit)
                        pHeader->resizeSection(i, truncationLimit);
                    else // Final column, add the remainder width there.
                    {
                        pHeader->resizeSection(i, truncationLimit + nAvailableWidth);
                        break;
                    }
                }
            }
            DFG_ASSERT_CORRECTNESS(numUnhandled == 0);
            numUnhandled = 0; // Set to zero as if there's a bug it's better to have malfunctioning size behaviour than
                              // infinite loop here.
        }
    } // 'Needs truncation'-case
}

    namespace
    {
        void onHeaderResizeAction_content(QHeaderView* pHeader)
        {
            if (!pHeader)
                return;
            const auto waitCursor = makeScopedCaller([]() { QApplication::setOverrideCursor(QCursor(Qt::WaitCursor)); },
                                                     []() { QApplication::restoreOverrideCursor(); });
            pHeader->resizeSections(QHeaderView::ResizeToContents);
        }

        void onHeaderResizeAction_fixedSize(QWidget* pParent, QHeaderView* pHeader)
        {
            if (!pHeader)
                return;
            bool bOk = false;
            const auto nCurrentSetting = pHeader->defaultSectionSize();
            const auto nMinimumSize = pHeader->minimumSectionSize();
            const auto nNewSize = QInputDialog::getInt(pParent,
                                                       QApplication::tr("Header size"),
                                                       QApplication::tr("New header size (minimum is %1)").arg(nMinimumSize),
                                                       nCurrentSetting,
                                                       nMinimumSize,
                                                       1048575, // Maximum section size at least in Qt 5.2
                                                       1,
                                                       &bOk);
            if (bOk && nNewSize > 0)
            {
                pHeader->setDefaultSectionSize(nNewSize);
            }
        }
    } // unnamed namespace

void CsvTableView::onColumnResizeAction_content()
{
    onHeaderResizeAction_content(horizontalHeader());
}

void CsvTableView::onRowResizeAction_content()
{
    onHeaderResizeAction_content(verticalHeader());
}

void CsvTableView::onColumnResizeAction_fixedSize()
{
    onHeaderResizeAction_fixedSize(this, horizontalHeader());
}

void CsvTableView::onRowResizeAction_fixedSize()
{
    onHeaderResizeAction_fixedSize(this, verticalHeader());
}

QModelIndex CsvTableView::mapToSource(const QAbstractItemModel* pModel, const QAbstractProxyModel* pProxy, const int r, const int c)
{
    if (pProxy)
        return pProxy->mapToSource(pProxy->index(r, c));
    else if (pModel)
        return pModel->index(r, c);
    else
        return QModelIndex();
}

QItemSelection CsvTableView::getSelection() const
{
    auto pSelectionModel = selectionModel();
    return (pSelectionModel) ? pSelectionModel->selection() : QItemSelection();
}

auto CsvTableView::privCreateActionBlockedDueToLockedContentMessage(const QString& actionname) -> QString
{
    if (isReadOnlyMode())
        return tr("Executing action '%1' was blocked: table is in read-only mode").arg(actionname);
    else
        return tr("Executing action '%1' was blocked: table is being accessed by some other operation. Please try again later.").arg(actionname);
}

void CsvTableView::showStatusInfoTip(const QString& sMsg)
{
    showInfoTip(sMsg, this);
}

void CsvTableView::privShowExecutionBlockedNotification(const QString& actionname)
{
    // Showing a note to user that execution was blocked. Not using QToolTip as it is a bit brittle in this use case:
    // it may disappear too quickly and disappear triggers are difficult to control in general.

    // This was the original tooltip version which worked ok with most notifications, but e.g. with
    // blocked edits in CsvTableViewDelegate was not shown at all.
    //QToolTip::showText(QCursor::pos(), privCreateActionBlockedDueToLockedContentMessage(actionname));

    showStatusInfoTip(privCreateActionBlockedDueToLockedContentMessage(actionname));
}

auto CsvTableView::tryLockForEdit() -> LockReleaser
{
    return (m_spEditLock && m_spEditLock->tryLockForWrite()) ? LockReleaser(m_spEditLock.get()) : LockReleaser();
}

auto CsvTableView::tryLockForRead() const -> LockReleaser
{
    return (m_spEditLock && m_spEditLock->tryLockForRead()) ? LockReleaser(m_spEditLock.get()) : LockReleaser();
}

auto CsvTableView::horizontalTableHeader() -> TableHeaderView*
{
    return qobject_cast<TableHeaderView*>(horizontalHeader());
}

void CsvTableView::setColumnNames()
{
    auto pCsvModel = this->csvModel();
    if (!pCsvModel)
        return;

    QDialog dlg(this);
    auto spLayout = std::make_unique<QVBoxLayout>(this);

    // Creating model and filling existing column names to it (as rows).
    CsvItemModel columnNameModel;
    columnNameModel.insertColumn(0);
    columnNameModel.setColumnName(0, tr("Column name"));

    const auto nColCount = pCsvModel->columnCount();
    columnNameModel.insertRows(0, nColCount);
    for (int c = 0; c < nColCount; ++c)
    {
        columnNameModel.setData(columnNameModel.index(c, 0), pCsvModel->getHeaderName(c));
        if (pCsvModel->getColumnProperty(c, CsvItemModelColumnProperty::readOnly).toBool())
            columnNameModel.setCellReadOnlyStatus(c, 0, true);
    }

    // Creating a dialog that has CsvTableView and ok & cancel buttons.
    CsvTableView columnNameView(nullptr, this, ViewType::fixedDimensionEdit);
    columnNameView.setModel(&columnNameModel);
    spLayout->addWidget(&columnNameView);

    spLayout->addWidget(addOkCancelButtonBoxToDialog(&dlg));

    dlg.setLayout(spLayout.release());
    removeContextHelpButtonFromDialog(&dlg);

    dlg.setMinimumHeight(350);
    dlg.show();
    columnNameView.onColumnResizeAction_toViewEvenly();

    // Setting focus to the item on which the context menu was opened from.
    {
        auto pHeader = horizontalTableHeader();
        if (pHeader)
            columnNameView.selectRow(pHeader->m_nLatestContextMenuEventColumn_dataModel);
    }

    if (dlg.exec() != QDialog::Accepted)
        return;

    // Setting column names.
    {
        for (int c = 0, nCount = columnNameModel.rowCount(); c < nCount; ++c)
        {
            const auto sOldName = pCsvModel->getHeaderName(c);
            const auto sNewName = columnNameModel.data(columnNameModel.index(c, 0)).toString();
            if (sOldName != sNewName)
                pCsvModel->setColumnName(c, sNewName);
        }
    }
}

void CsvTableView::addConfigSavePropertyFetcher(PropertyFetcher fetcher)
{
    DFG_OPAQUE_REF().m_propertyFetchers.push_back(std::move(fetcher));
}

auto CsvTableView::weekDayNames() const -> QStringList
{
    const auto s = getCsvModelOrViewProperty<CsvTableViewPropertyId_weekDayNames>(this);
    return s.split(',');
}

auto CsvTableView::dateTimeToString(const QDateTime& dateTime, const QString& sFormat) const -> QString
{
    QString rv = dateTime.toString(sFormat);
    ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::handleWeekDayFormat(rv, dateTime.date().dayOfWeek(), [&]() { return this-> weekDayNames(); });
    return rv;
}

auto CsvTableView::dateTimeToString(const QDate& date, const QString& sFormat) const -> QString
{
    QString rv = date.toString(sFormat);
    ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::handleWeekDayFormat(rv, date.dayOfWeek(), [&]() { return this-> weekDayNames(); });
    return rv;
}

auto CsvTableView::dateTimeToString(const QTime& qtime, const QString& sFormat) const -> QString
{
    return qtime.toString(sFormat);
}

namespace
{
    template <class View_T>
    static auto getColInfo(View_T& rView, const int nCol) -> decltype(rView.csvModel()->getColInfo(nCol))
    {
        auto pCsvModel = rView.csvModel();
        return (pCsvModel) ? pCsvModel->getColInfo(nCol) : nullptr;
    }

    template <class View_T>
    static auto getColInfo(View_T& rView, const ColumnIndex_data col) -> decltype(getColInfo(rView, 0))
    {
        return getColInfo(rView, col.value());
    }

    template <class View_T>
    static auto getColInfo(View_T& rView, const ColumnIndex_view col) -> decltype(getColInfo(rView, 0))
    {
        return getColInfo(rView, rView.columnIndexViewToData(col));
    }

    uintptr_t viewPropertyContextId(const CsvTableView& view)
    {
        return reinterpret_cast<uintptr_t>(&view);
    }

    static QVariant getColumnProperty(const CsvTableView& rView, const CsvItemModel::ColInfo& rColInfo, const StringViewUtf8& svKey, const QVariant defaultValue)
    {
        return rColInfo.getProperty(viewPropertyContextId(rView), svKey, defaultValue);
    }
} // unnamed namespace

template <class Func_T>
QVariant CsvTableView::getColumnPropertyByDataModelIndexImpl(int nDataModelCol, QVariant defaultValue, Func_T func) const
{
    // This is brittle: if getting read lock fails, this function may effectively return bogus column property which in turn
    // can mean e.g. that UI shows faulty property status for a column. getColInfo() should probably always guarantee somekind of access
    // to 'latest known' column info.
    auto lockReleaser = tryLockForRead();
    if (!lockReleaser.isLocked())
        return defaultValue;
    auto pColInfo = getColInfo(*this, nDataModelCol);
    return (pColInfo) ? func(*pColInfo) : defaultValue;
}

QVariant CsvTableView::getColumnPropertyByDataModelIndex(const int nCol, const StringViewUtf8& svKey, const QVariant defaultValue) const
{
    return getColumnPropertyByDataModelIndexImpl(nCol, defaultValue, [&](const CsvItemModel::ColInfo& rColInfo) { return getColumnProperty(*this, rColInfo, svKey, defaultValue); });
}

QVariant CsvTableView::getColumnPropertyByDataModelIndex(const int nCol, const CsvItemModelColumnProperty propertyId, const QVariant defaultValue) const
{
    return getColumnPropertyByDataModelIndexImpl(nCol, defaultValue, [&](const CsvItemModel::ColInfo& rColInfo) { return rColInfo.getProperty(propertyId, defaultValue); });
}

void CsvTableView::setCaseSensitiveSorting(const bool bCaseSensitive)
{
    auto pProxy = qobject_cast<QSortFilterProxyModel*>(getProxyModelPtr());
    if (pProxy)
    {
        pProxy->setSortCaseSensitivity((bCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive);
        if (DFG_OPAQUE_REF().m_spActSortCaseSensitivity)
        {
            const auto bActionChecked = DFG_OPAQUE_REF().m_spActSortCaseSensitivity->isChecked();
            if (bCaseSensitive != bActionChecked)
                DFG_OPAQUE_REF().m_spActSortCaseSensitivity->setChecked(bCaseSensitive);
        }
    }
    else
        QToolTip::showText(QCursor::pos(), tr("Unable to toggle sort case sensitivity: no suitable proxy model found"));
}

void CsvTableView::resetSorting()
{
    auto pProxy = this->getProxyModelPtr();
    if (pProxy)
        pProxy->sort(-1);
    else
        QToolTip::showText(QCursor::pos(), tr("Unable to reset sorting: no proxy model found"));
}

bool CsvTableView::isColumnVisible(const ColumnIndex_data nCol) const
{
    return getColumnPropertyByDataModelIndex(nCol.value(), ColumnPropertyId::visible, true).toBool();
}

void CsvTableView::invalidateSortFilterProxyModel()
{
    auto pProxyModel = qobject_cast<QSortFilterProxyModel*>(getProxyModelPtr());
    if (pProxyModel)
        pProxyModel->invalidate();
}

void CsvTableView::setColumnVisibility(const int nCol, const bool bVisible, const ProxyModelInvalidation proxyInvalidation)
{
    auto lockReleaser = this->tryLockForEdit();
    if (!lockReleaser.isLocked())
    {
        showStatusInfoTip(tr("Unable to change column visibility: there seems to be ongoing operations"));
        return;
    }

    auto pCsvModel = csvModel();
    if (!pCsvModel)
        return;

    auto pColInfo = pCsvModel->getColInfo(nCol);
    if (!pColInfo)
        return;

    if (pColInfo->setProperty(viewPropertyContextId(*this), ColumnPropertyId::visible, bVisible) && proxyInvalidation == ProxyModelInvalidation::ifNeeded)
    {
        lockReleaser.unlock(); // Must be unlocked before invalidating since UI update may malfunction if holding edit-lock.
        invalidateSortFilterProxyModel();
    }
}

auto CsvTableView::getCellEditability(const RowIndex_data nRow, const ColumnIndex_data nCol) const -> CellEditability
{
    if (this->isReadOnlyMode())
        return CellEditability::blocked_tableReadOnly;
    else if (getColumnPropertyByDataModelIndex(nCol.value(), CsvItemModelColumnProperty::readOnly, false).toBool())
        return CellEditability::blocked_columnReadOnly;
    else if (csvModel() != nullptr && !csvModel()->isCellEditable(nRow, nCol.value()))
        return CellEditability::blocked_cellReadOnly;
    else
        return CellEditability::editable;
}

void CsvTableView::setColumnReadOnly(const ColumnIndex_data nCol, const bool bReadOnly)
{
    if (this->m_spUndoStack && this->m_spUndoStack->item().count() > 0)
    {
        if (QMessageBox::question(
                this,
                tr("Changing column read-only"),
                tr("Changing column read-only status clears undo-buffer (currently has %1 item(s)), proceed?").arg(this->m_spUndoStack->item().count())
            ) != QMessageBox::Yes)
            return;
    }

    auto lockReleaser = this->tryLockForEdit();
    if (!lockReleaser.isLocked())
    {
        showStatusInfoTip(tr("Unable to change column read-only status: there seem to be ongoing operations"));
        return;
    }

    clearUndoStack();

    auto pColInfo = getColInfo(*this, nCol);
    if (!pColInfo)
    {
        showStatusInfoTip(tr("Internal error: didn't find column info object"));
        return;
    }

    // Note: using real column property instead of context property as it was simpler to implement (don't need to implement checking everywhere in actions etc.)
    //       and it also makes sense to have the property in model itself rather on view-level.
    pColInfo->setProperty(CsvItemModelColumnProperty::readOnly, bReadOnly);
}

void CsvTableView::unhideAllColumns()
{
    auto lockReleaser = this->tryLockForEdit();
    if (!lockReleaser.isLocked())
    {
        showStatusInfoTip(tr("Unable to unhide columns: there seems to be ongoing operations"));
        return;
    }
    auto pCsvModel = csvModel();
    if (!pCsvModel)
        return;

    pCsvModel->forEachColInfoWhile(lockReleaser, [&](CsvItemModel::ColInfo& colInfo)
    {
        if (!colInfo.getProperty(viewPropertyContextId(*this), ColumnPropertyId::visible, true).toBool())
            colInfo.setProperty(viewPropertyContextId(*this), ColumnPropertyId::visible, true);
        return true;
    });

    lockReleaser.unlock(); // Must be unlocked before invalidating since UI update may malfunction if holding edit-lock.
    invalidateSortFilterProxyModel();
}

void CsvTableView::showSelectColumnVisibilityDialog()
{
    auto pCsvModel = this->csvModel();
    if (!pCsvModel)
        return;
    QStringList columns;
    std::vector<bool> visibilityFlags;
    {
        auto lockReleaser = this->tryLockForRead();
        if (!pCsvModel->forEachColInfoWhile(lockReleaser, [&](const CsvItemModel::ColInfo& colInfo)
            {
                columns.push_back(tr("Column %1 ('%2')").arg(CsvItemModel::internalColumnIndexToVisible(colInfo.index())).arg(colInfo.name()));
                visibilityFlags.push_back(getColumnProperty(*this, colInfo, ColumnPropertyId::visible, true).toBool());
                return true;
            }))
        {
            showStatusInfoTip(tr("Unable to change column visibility at the moment: there seems to be ongoing operations"));
            return;
        }
    }
    bool bOk = false;
    while (true)
    {
        const auto choices = InputDialog::getItems(this,
                            tr("Select column visibility"),
                            tr("Choose columns to hide"),
                            columns,
                            [&](const int i, const QString&) { return !visibilityFlags[i]; },
                            &bOk);
        if (!bOk)
            return;

        if (choices.size() == visibilityFlags.size())
        {
            showStatusInfoTip(tr("Hiding all columns is not supported."));
            continue;
        }

        std::fill(visibilityFlags.begin(), visibilityFlags.end(), true);
        for (const auto c : choices)
            visibilityFlags[c] = false;
        break;
    }

    auto lockReleaser = this->tryLockForEdit();
    if (!lockReleaser.isLocked())
    {
        showStatusInfoTip(tr("Unable to unhide selected columns: there seems to be ongoing operations"));
        return;
    }
    // Simply just running through all columns, a more fine-grained solution would be to determine what has changed.
    for (int c = 0; c < pCsvModel->columnCount(); ++c)
    {
        this->setColumnVisibility(c, visibilityFlags[c], ProxyModelInvalidation::no);
    }
    lockReleaser.unlock(); // Must be unlocked before invalidating since UI update may malfunction if holding edit-lock.
    invalidateSortFilterProxyModel();
}

auto CsvTableView::columnIndexDataToView(const ColumnIndex_data dataIndex) const -> ColumnIndex_view
{
    auto pProxy = getProxyModelPtr();
    if (!pProxy)
        return ColumnIndex_view(dataIndex.value());

    auto pCsvModel = csvModel();
    if (!pCsvModel)
        return ColumnIndex_view();

    const auto mapped = pProxy->mapFromSource(pCsvModel->index(0, dataIndex.value()));
    return ColumnIndex_view(mapped.column());
}

auto CsvTableView::columnIndexViewToData(const ColumnIndex_view viewIndex) const -> ColumnIndex_data
{
    auto pProxy = getProxyModelPtr();
    if (!pProxy)
        return ColumnIndex_data(viewIndex.value());

    const auto mapped = pProxy->mapToSource(pProxy->index(1, viewIndex.value()));
    return (mapped.column() >= 0) ? ColumnIndex_data(mapped.column()) : ColumnIndex_data();
}

QString CsvTableView::getColumnName(const ColumnIndex_data dataIndex) const
{
    auto pCsvModel = csvModel();
    return (pCsvModel) ? pCsvModel->getHeaderName(dataIndex.value()) : QString();
}

QString CsvTableView::getColumnName(const ColumnIndex_view viewIndex) const
{
    return getColumnName(columnIndexViewToData(viewIndex));
}

void CsvTableView::doModalOperation(const QString& sProgressDialogLabel, const ProgressWidget::IsCancellable isCancellable, const QString& sThreadName, std::function<void(ProgressWidget*)> func)
{
    ::DFG_MODULE_NS(qt)::doModalOperation(this, sProgressDialogLabel, isCancellable, sThreadName, func);
}

QColor CsvTableView::getReadOnlyBackgroundColour() const
{
    return QColor(248, 248, 248);
}

QString CsvTableView::getAcceptedFileTypeFilter() const
{
    return "*.csv *.tsv *.csv.conf *.sqlite3 *.sqlite *.db";
}

QString CsvTableView::getFilterTextForOpenFileDialog() const
{
    return tr("CSV or SQLite files (%1);; CSV files (*.csv *.tsv *.csv.conf);; SQLite files (*.sqlite3 *.sqlite *.db);; All files(*.*)")
        .arg(getAcceptedFileTypeFilter());
}

QString CsvTableView::getAcceptableFilePathFromMimeData(const QMimeData* pMimeData) const
{
    if (!pMimeData || !pMimeData->hasUrls())
        return QString();
    const auto urls = pMimeData->urls();
    if (urls.size() == 1 && urls.front().isLocalFile())
    {
        const auto url = urls.front();
        QFileInfo fi(url.toLocalFile());
        const auto isAcceptableFileSuffix = [&](const QString& sSuffix)
        {
            return getAcceptedFileTypeFilter().indexOf(QString("*.%1 ").arg(sSuffix), 0, Qt::CaseInsensitive) != -1;
        };
        return (isAcceptableFileSuffix(fi.suffix()) || isAcceptableFileSuffix(fi.completeSuffix())) ? fi.absoluteFilePath() : QString();
    }
    else
        return QString();
}

QString CsvTableView::getFilePathFromFileDialog()
{
    return getFilePathFromFileDialog(tr("Open file"));
}

QString CsvTableView::getFilePathFromFileDialog(const QString& sCaption)
{
    return QFileDialog::getOpenFileName(this,
        sCaption,
        QString(), // dir
        getFilterTextForOpenFileDialog(),
        nullptr, // selected filter
        QFileDialog::Options() // options
    );
}

void CsvTableView::scrollToDefaultPosition()
{
    const auto& sInitialScrollPosition = DFG_OPAQUE_REF().m_sInitialScrollPosition;
    if (sInitialScrollPosition == "bottom")
        scrollToBottom();
    else if (sInitialScrollPosition == "top" || sInitialScrollPosition.isEmpty())
        scrollToTop();
    else
        this->showStatusInfoTip(tr("Unrecognized scroll position '%1'. Supported ones are 'top' and 'bottom'; also empty is interpreted as 'use default'").arg(sInitialScrollPosition));
}

void CsvTableView::forEachUserInsertableConfFileProperty(std::function<void(const DFG_DETAIL_NS::ConfFileProperty&)> propHandler) const
{
    if (!propHandler)
        return;

    const auto config = this->populateCsvConfig();

    // Calling handler for items below when they are not already present.
#define DFG_TEMP_CALL_IF_NEEDED(KEY, DEFAULT_VALUE) if (!config.contains(DFG_UTF8(KEY))) propHandler(DFG_DETAIL_NS::ConfFileProperty(QStringLiteral(KEY), DEFAULT_VALUE))
    DFG_TEMP_CALL_IF_NEEDED("properties/dateFormat",            getCsvTableViewProperty<CsvTableViewPropertyId_dateFormat>(this));
    DFG_TEMP_CALL_IF_NEEDED("properties/dateTimeFormat",        getCsvTableViewProperty<CsvTableViewPropertyId_dateTimeFormat>(this));
    DFG_TEMP_CALL_IF_NEEDED("properties/editMode",              getCsvTableViewProperty<CsvTableViewPropertyId_editMode>(this));
    DFG_TEMP_CALL_IF_NEEDED("properties/initialScrollPosition", getCsvTableViewProperty<CsvTableViewPropertyId_initialScrollPosition>(this));
    DFG_TEMP_CALL_IF_NEEDED("properties/timeFormat",            getCsvTableViewProperty<CsvTableViewPropertyId_timeFormat>(this));
    DFG_TEMP_CALL_IF_NEEDED("properties/weekDayNames",          getCsvTableViewProperty<CsvTableViewPropertyId_weekDayNames>(this));
    //DFG_TEMP_CALL_IF_NEEDED("properties/", getCsvTableViewProperty<>(this));

#undef DFG_TEMP_CALL_IF_NEEDED
}


//////////////////////////////////////////////////////////////////////////
//
// class CsvTableWidget
//
//////////////////////////////////////////////////////////////////////////

DFG_OPAQUE_PTR_DEFINE(CsvTableWidget)
{
public:
    CsvItemModel m_csvModel;
    std::unique_ptr<CsvTableViewSortFilterProxyModel> m_spProxyModel;
};

CsvTableWidget::CsvTableWidget(QWidget* pParent)
    : BaseClass(nullptr, pParent)
{
    DFG_OPAQUE_REF().m_spProxyModel.reset(new CsvTableViewSortFilterProxyModel(this));
    DFG_OPAQUE_REF().m_spProxyModel->setSourceModel(&DFG_OPAQUE_REF().m_csvModel);
    this->setModel(DFG_OPAQUE_REF().m_spProxyModel.get());
}

CsvTableWidget::~CsvTableWidget() = default;

CsvItemModel& CsvTableWidget::getCsvModel() { return DFG_OPAQUE_REF().m_csvModel; }
const CsvItemModel& CsvTableWidget::getCsvModel() const { DFG_ASSERT_UB(DFG_OPAQUE_PTR()); return DFG_OPAQUE_PTR()->m_csvModel; }

CsvTableViewSortFilterProxyModel& CsvTableWidget::getViewModel() { DFG_ASSERT_UB(DFG_OPAQUE_REF().m_spProxyModel); return *DFG_OPAQUE_REF().m_spProxyModel; }
const CsvTableViewSortFilterProxyModel& CsvTableWidget::getViewModel() const { DFG_ASSERT_UB(DFG_OPAQUE_PTR() && DFG_OPAQUE_PTR()->m_spProxyModel); return *DFG_OPAQUE_PTR()->m_spProxyModel; }


//////////////////////////////////////////////////////////////////////////
//
// class CsvTableViewDlg
//
//////////////////////////////////////////////////////////////////////////

DFG_OPAQUE_PTR_DEFINE(CsvTableViewDlg)
{
public:
    QObjectStorage<QDialog> m_spDialog;
    QObjectStorage<CsvTableView> m_spTableView;
};

CsvTableViewDlg::CsvTableViewDlg(std::shared_ptr<QReadWriteLock> spReadWriteLock, QWidget* pParent, CsvTableView::ViewType viewType)
{
    DFG_OPAQUE_REF().m_spDialog.reset(new QDialog(pParent));
    DFG_OPAQUE_REF().m_spTableView.reset(new CsvTableView(std::move(spReadWriteLock), DFG_OPAQUE_REF().m_spDialog.get(), viewType));
    auto& dlg = *DFG_OPAQUE_REF().m_spDialog;
    auto spLayout = std::make_unique<QVBoxLayout>(nullptr);

    spLayout->addWidget(DFG_OPAQUE_REF().m_spTableView.get());
    auto& rButtonBox = *(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel));
    DFG_QT_VERIFY_CONNECT(QObject::connect(&rButtonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept));
    DFG_QT_VERIFY_CONNECT(QObject::connect(&rButtonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject));
    spLayout->addWidget(&rButtonBox);

    dlg.setLayout(spLayout.release());
    removeContextHelpButtonFromDialog(&dlg);
}

void CsvTableViewDlg::setModel(QAbstractItemModel* pModel)
{
    DFG_OPAQUE_REF().m_spTableView->setModel(pModel);
}

int CsvTableViewDlg::exec()
{
    DFG_OPAQUE_REF().m_spDialog->show(); // To get dimensions for column resize.
    csvView().onColumnResizeAction_toViewContentAware();
    return DFG_OPAQUE_REF().m_spDialog->exec();
}

void CsvTableViewDlg::resize(const int w, const int h)
{
    dialog().resize(w, h);
}

auto CsvTableViewDlg::dialog() -> QWidget&
{
    DFG_REQUIRE(DFG_OPAQUE_REF().m_spDialog != nullptr);
    return *DFG_OPAQUE_REF().m_spDialog;
}

auto CsvTableViewDlg::csvView() -> CsvTableView&
{
    DFG_REQUIRE(DFG_OPAQUE_REF().m_spTableView != nullptr);
    return *DFG_OPAQUE_REF().m_spTableView;
}

void CsvTableViewDlg::addVerticalLayoutWidget(int nPos, QWidget* pWidget)
{
    auto pLayout = qobject_cast<QVBoxLayout*>(dialog().layout());
    DFG_ASSERT_CORRECTNESS(pLayout != nullptr);
    if (!pLayout)
        return;
    pLayout->insertWidget(nPos, pWidget);
}

//////////////////////////////////////////////////////////////////////////
//
// class CsvTableViewSelectionAnalyzer
//
//////////////////////////////////////////////////////////////////////////

CsvTableViewSelectionAnalyzer::CsvTableViewSelectionAnalyzer()
    : m_abIsEnabled(true)
    , m_bPendingCheckQueue(false)
    , m_abNewSelectionPending(false)
{
    m_spMutexAnalyzeQueue.reset(new QMutex);
}

CsvTableViewSelectionAnalyzer::~CsvTableViewSelectionAnalyzer()
{

}

void CsvTableViewSelectionAnalyzer::addSelectionToQueueImpl(QItemSelection selection)
{
    QMutexLocker locker(m_spMutexAnalyzeQueue.get());
    // If there's nothing in analyze queue, add selection...
    if (m_analyzeQueue.empty())
        m_analyzeQueue.push_back(selection);
    else // ...and when there already is one, override existing with newer one.
        m_analyzeQueue.front() = selection;
    m_abNewSelectionPending = true;
    if (!m_bPendingCheckQueue) // Schedule a new analyze queue checking only if one is not already pending.
    {
        m_bPendingCheckQueue = true;
        QTimer::singleShot(0, this, SLOT(onCheckAnalyzeQueue()));
    }
}

void CsvTableViewSelectionAnalyzer::onCheckAnalyzeQueue()
{
    QMutexLocker locker(m_spMutexAnalyzeQueue.get());
    if (!m_analyzeQueue.empty())
    {
        
        // Try to lock readwrite lock, if fails, it means that someone might be editing the model so analyzers should not read the model at the same time.
        auto pView = qobject_cast<CsvTableView*>(m_spView.data());
        auto lockReleaser = (pView) ? pView->tryLockForRead() : LockReleaser();
        
        if (pView && !lockReleaser.isLocked())
        {
            // Ending up here means that couldn't acquire read lock, e.g. because table is being edited. Scheduling a new try in 100 ms.
            QTimer::singleShot(100, this, SLOT(onCheckAnalyzeQueue()));
            return;
        }

        auto selection = m_analyzeQueue.back();
        m_abNewSelectionPending = false;
        m_analyzeQueue.pop_back();
        m_bPendingCheckQueue = false;
        locker.unlock(); // We're done with retrieving new selection -> release mutex.
        analyzeImpl(selection);
    }
}

//////////////////////////////////////////////////////////////////////////
//
// class CsvTableViewBasicSelectionAnalyzer
//
//////////////////////////////////////////////////////////////////////////


CsvTableViewBasicSelectionAnalyzer::CsvTableViewBasicSelectionAnalyzer(PanelT* uiPanel)
   : m_spUiPanel(uiPanel)
{
}

CsvTableViewBasicSelectionAnalyzer::~CsvTableViewBasicSelectionAnalyzer() = default;

void CsvTableViewBasicSelectionAnalyzer::analyzeImpl(const QItemSelection selection)
{
    auto uiPanel = m_spUiPanel.data();
    if (!uiPanel)
        return;

    auto pCtvView = qobject_cast<const CsvTableView*>(m_spView.data());
    auto pModel = (pCtvView) ? pCtvView->csvModel() : nullptr;
    if (!pModel)
    {
        uiPanel->setValueDisplayString(QString());
        return;
    }

    const auto maxTime = uiPanel->getMaxTimeInSeconds();
    const auto enabled = (!DFG_MODULE_NS(math)::isNan(maxTime) &&  maxTime > 0);
    CompletionStatus completionStatus = CompletionStatus_started;
    DFG_MODULE_NS(time)::TimerCpu operationTimer;
    uiPanel->onEvaluationStarting(enabled);
    if (enabled)
    {
        ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::BasicSelectionDetailCollector collector;
        collector.loadDetailHandlers(uiPanel->collectors());

        const auto nNumericPrecision = uiPanel->defaultNumericPrecision();

        // For each selection
        for(auto iter = selection.cbegin(); iter != selection.cend(); ++iter)
        {
            // For each cell in selection
            pCtvView->forEachCsvModelIndexInSelectionRange(*iter, CsvTableView::ForEachOrder::inOrderFirstRows, [&](const QModelIndex& index, bool& rbContinue)
            {
                const auto bHasMaxTimePassed = operationTimer.elapsedWallSeconds() >= maxTime;
                if (bHasMaxTimePassed || uiPanel->isStopRequested() || m_abNewSelectionPending || !m_abIsEnabled.load(std::memory_order_relaxed))
                {
                    if (bHasMaxTimePassed)
                        completionStatus = ::DFG_MODULE_NS(qt)::CsvTableViewSelectionAnalyzer::CompletionStatus_terminatedByTimeLimit;
                    else if (m_abNewSelectionPending)
                        completionStatus = ::DFG_MODULE_NS(qt)::CsvTableViewSelectionAnalyzer::CompletionStatus_terminatedByNewSelection;
                    else if (!m_abIsEnabled)
                        completionStatus = ::DFG_MODULE_NS(qt)::CsvTableViewSelectionAnalyzer::CompletionStatus_terminatedByDisabling;
                    else
                        completionStatus = ::DFG_MODULE_NS(qt)::CsvTableViewSelectionAnalyzer::CompletionStatus_terminatedByUserRequest;
                    rbContinue = false;
                    return;
                }
                collector.handleCell(*pModel, index);
            }); // For each cell in selection
        } // For each selection
        const auto elapsedTime = operationTimer.elapsedWallSeconds();
        if (completionStatus == CompletionStatus_started)
            completionStatus = CompletionStatus_completed;
        uiPanel->onEvaluationEnded(elapsedTime, completionStatus);

        QString sMessage;
        if (completionStatus == CompletionStatus_completed)
            sMessage = collector.resultString(selection, nNumericPrecision);
        else if (completionStatus == CompletionStatus_terminatedByTimeLimit)
            sMessage = uiPanel->tr("Interrupted (time limit exceeded)");
        else if (completionStatus == CompletionStatus_terminatedByUserRequest)
            sMessage = uiPanel->tr("Stopped");
        else if (completionStatus == CompletionStatus_terminatedByNewSelection)
            sMessage = uiPanel->tr("Interrupted (new selection chosen)");
        else if (completionStatus == CompletionStatus_terminatedByDisabling)
            sMessage = uiPanel->tr("Interrupted (disabled)");
        else
            sMessage = uiPanel->tr("Interrupted (unknown reason)");

        uiPanel->setValueDisplayString(sMessage);
    }
    Q_EMIT sigAnalyzeCompleted();
}

#undef DFG_CSVTABLEVIEW_PROPERTY_PREFIX

//////////////////////////////////////////////////////////////////////////
//
// class TableHeaderView
//
//////////////////////////////////////////////////////////////////////////

TableHeaderView::TableHeaderView(CsvTableView* pParent) :
    BaseClass(Qt::Horizontal, pParent)
{
    setSectionsClickable(true); // Without this clicking header didn't select column
}

CsvTableView* TableHeaderView::tableView()
{
    return qobject_cast<CsvTableView*>(parent());
}

const CsvTableView* TableHeaderView::tableView() const
{
    return qobject_cast<const CsvTableView*>(parent());
}

int TableHeaderView::columnIndex_dataModel(const QPoint& pos) const
{
    const auto viewColumn = columnIndex_viewModel(pos);
    if (viewColumn == -1)
        return -1;

    auto pView = tableView();
    auto pCsvModel = (pView) ? pView->csvModel() : nullptr;
    if (!pCsvModel)
        return -1;

    auto& rView = *pView;
    // Not an optimal way to determine column index when there are lots of columns
    for (int nVisibleColumn = -1, nModelColumn = 0; nModelColumn < pCsvModel->columnCount(); ++nModelColumn)
    {
        const auto bVisible = rView.getColumnPropertyByDataModelIndex(nModelColumn, ColumnPropertyId::visible, true).toBool();
        if (bVisible)
            ++nVisibleColumn;
        if (nVisibleColumn == viewColumn)
            return nModelColumn;
    }
    return -1;
}

int TableHeaderView::columnIndex_viewModel(const QPoint& pos) const
{
    return logicalIndexAt(pos);
}

void TableHeaderView::contextMenuEvent(QContextMenuEvent* pEvent)
{
    if (!pEvent)
        return;
    m_nLatestContextMenuEventColumn_dataModel = columnIndex_dataModel(pEvent->pos());

    auto pView = tableView();
    if (!pView)
        return;

    auto& rView = *pView;

    QMenu menu;

    if (!rView.isReadOnlyMode())
        addViewAction(rView, menu, tr("Set column names"), noShortCut, ActionFlags::contentEdit, false, &CsvTableView::setColumnNames);

    const auto pCsvModel = rView.csvModel();
    auto lockReleaser = (pCsvModel) ? rView.tryLockForRead() : LockReleaser();
    if (pCsvModel && lockReleaser.isLocked())
    {
        menu.addSeparator();

        // "Select column visibility"
        addViewAction(rView, menu, tr("Select column visibility..."), noShortCut, ActionFlags::viewEdit, false, &CsvTableView::showSelectColumnVisibilityDialog);

        const auto nMaxUnhideEntryCount = 10;
        auto nHiddenCount = 0;
        pCsvModel->forEachColInfoWhile(lockReleaser, [&](const CsvItemModel::ColInfo& colInfo)
        {
            if (getColumnProperty(rView, colInfo, ColumnPropertyId::visible, true).toBool())
                return true;
            ++nHiddenCount;
            if (nHiddenCount > nMaxUnhideEntryCount)
                return true;// Only allowing at most 10 "unhide column" items to menu.
            if (nHiddenCount == 0)
                menu.addSeparator();
            const auto nColIndex = colInfo.index();
            const auto sMenuTitle = tr("Unhide column %1 ('%2')").arg(CsvItemModel::internalColumnIndexToVisible(nColIndex)).arg(colInfo.name().left(32));
            addViewAction(rView, menu, sMenuTitle, noShortCut, ActionFlags::viewEdit, false, [=]() { pView->setColumnVisibility(nColIndex, true); });
            return true;
        });

        // Unhide all
        if (nHiddenCount > 1)
        {
            auto pAction = menu.addAction(tr("Unhide all (%1 columns are hidden)").arg(nHiddenCount));
            DFG_QT_VERIFY_CONNECT(QObject::connect(pAction, &QAction::triggered, this, [=]() { pView->unhideAllColumns(); }));
        }

        const auto nShownCount = pCsvModel->columnCount() - nHiddenCount;

        // Column specific entries (not necessarily present e.g. if click happened outside columns)
        if (m_nLatestContextMenuEventColumn_dataModel != -1)
        {
            addSectionEntryToMenu(&menu, tr("Column  %1 ('%2')")
                .arg(CsvItemModel::internalColumnIndexToVisible(m_nLatestContextMenuEventColumn_dataModel))
                .arg(pCsvModel->getHeaderName(m_nLatestContextMenuEventColumn_dataModel).left(32)));

            const auto funcVisibleHandler = [=](const bool bVisible) { pView->setColumnVisibility(m_nLatestContextMenuEventColumn_dataModel, bVisible); };
            const auto bIsVisible = rView.getColumnPropertyByDataModelIndex(m_nLatestContextMenuEventColumn_dataModel, ColumnPropertyId::visible, true).toBool();
            auto& rActVisible = addViewAction(rView, menu, tr("Visible"), noShortCut, ActionFlags::viewEdit, { true, bIsVisible }, funcVisibleHandler);
            if (bIsVisible && nShownCount == 1)
            {
                rActVisible.setDisabled(true); // Not letting all columns to be hidden to prevent column header from getting removed.
                rActVisible.setToolTip(tr("Hiding disabled: hiding all columns is not supported"));
            }
            // Read-only item
            {
                const auto funcReadOnlyHandler = [=](const bool bReadOnly) { pView->setColumnReadOnly(ColumnIndex_data(m_nLatestContextMenuEventColumn_dataModel), bReadOnly); };
                const bool bReadOnly = rView.getColumnPropertyByDataModelIndex(m_nLatestContextMenuEventColumn_dataModel, CsvItemModelColumnProperty::readOnly, false).toBool();
                addViewAction(rView, menu, tr("Read-only"), noShortCut, ActionFlags::readOnly, { true, bReadOnly }, funcReadOnlyHandler);
            }
            auto pTypeMenu = menu.addMenu(tr("Data type"));
            if (pTypeMenu)
            {
                pTypeMenu->setToolTip(tr("Type can be used to change sorting behaviour. Does not affect underlying storage"));
                const auto funcSetType = [=](const CsvItemModel::ColType colType)
                {
                    auto pModel = pView->csvModel();
                    if (pModel)
                        pModel->setColumnType(m_nLatestContextMenuEventColumn_dataModel, colType);
                };
                const auto currentType = pCsvModel->getColType(m_nLatestContextMenuEventColumn_dataModel);
                const bool bIsTextType = (currentType == CsvItemModel::ColTypeText);
                const bool bIsNumberType = (currentType == CsvItemModel::ColTypeNumber);
                addViewAction(rView, *pTypeMenu, tr("Text")  , noShortCut, ActionFlags::confEdit, { true, bIsTextType }  , [=]() { funcSetType(CsvItemModel::ColTypeText);   });
                addViewAction(rView, *pTypeMenu, tr("Number"), noShortCut, ActionFlags::confEdit, { true, bIsNumberType }, [=]() { funcSetType(CsvItemModel::ColTypeNumber); });
            }
        }
    }

    lockReleaser.unlock();
    if (!menu.actions().isEmpty())
        menu.exec(QCursor::pos());
}

namespace DFG_DETAIL_NS
{
    void BasicSelectionDetailCollector::loadDetailHandlers(CsvTableViewBasicSelectionAnalyzerPanel::CollectorContainerPtr spCollectors)
    {
        m_spCollectors = spCollectors;
        if (!m_spCollectors)
            return;
        m_enableFlags.reset();
        for (const auto& spItem : *m_spCollectors)
        {
            if (!spItem)
                continue;
            const auto builtInDetail = selectionDetailNameToId(spItem->id());
            const bool bEnabled = spItem->isEnabled();
            if (builtInDetail != BuiltInDetail::detailCount) // Built-in detail?
                m_enableFlags.set(static_cast<size_t>(builtInDetail), bEnabled);
            else if (bEnabled)
                m_activeNonBuiltInCollectors.push_back(spItem.get());
            spItem->reset();
        }
    }
} // namespace DFG_DETAIL_NS inside dfg::qt

/////////////////////////////////////////////////////////////////////////////////////
// 
// ProgressWidget
//
/////////////////////////////////////////////////////////////////////////////////////

ProgressWidget::ProgressWidget(const QString sLabelText, const IsCancellable isCancellable, QWidget* pParent)
    : BaseClass(sLabelText, QString(), 0, 0, pParent)
{
    removeContextHelpButtonFromDialog(this);
    removeCloseButtonFromDialog(this);
    setWindowModality(Qt::WindowModal);
    if (isCancellable == IsCancellable::yes)
    {
        auto spCancelButton = std::unique_ptr<QPushButton>(new QPushButton(tr("Cancel"), this));
        DFG_QT_VERIFY_CONNECT(connect(spCancelButton.get(), &QPushButton::clicked, this, [&]() { m_abCancelled = true; }));
        setCancelButton(spCancelButton.release()); // "The progress dialog takes ownership"
    }
    else
        setCancelButton(nullptr); // Making sure that cancel button is not shown.
}

bool ProgressWidget::isCancelled() const
{
    return m_abCancelled.load(std::memory_order_relaxed);
}

/////////////////////////////////////////////////////////////////////////////////////
// 
// CsvTableViewSortFilterProxyModel
//
/////////////////////////////////////////////////////////////////////////////////////

DFG_OPAQUE_PTR_DEFINE(CsvTableViewSortFilterProxyModel)
{
    MultiMatchDefinition<CsvItemModelStringMatcher> m_matchers;
};

CsvTableViewSortFilterProxyModel::CsvTableViewSortFilterProxyModel(QWidget* pNonNullCsvTableViewParent)
    : BaseClass(pNonNullCsvTableViewParent)
{
    DFG_ASSERT_CORRECTNESS(pNonNullCsvTableViewParent != nullptr);
    this->setSortCaseSensitivity(Qt::CaseInsensitive);
}

CsvTableViewSortFilterProxyModel::~CsvTableViewSortFilterProxyModel() = default;

bool CsvTableViewSortFilterProxyModel::lessThan(const QModelIndex& sourceLeft, const QModelIndex& sourceRight) const
{
    auto pView = getTableView();
    auto pCsvModel = (pView) ? pView->csvModel() : nullptr;
    if (pCsvModel)
    {
        const auto dataIndexLeft = pView->mapToDataModel(sourceLeft);
        const auto colType = pCsvModel->getColType(dataIndexLeft.column());
        switch (colType)
        {
            case CsvItemModel::ColTypeNumber: return tableCellStringToDouble(pCsvModel->rawStringPtrAt(dataIndexLeft)) < tableCellStringToDouble(pCsvModel->rawStringPtrAt(pView->mapToDataModel(sourceRight)));
        }
    }
    return BaseClass::lessThan(sourceLeft, sourceRight);
}

bool CsvTableViewSortFilterProxyModel::filterAcceptsColumn(const int sourceColumn, const QModelIndex& sourceParent) const
{
    auto pView = getTableView();
    if (pView)
        return pView->isColumnVisible(ColumnIndex_data(sourceColumn));
    else
        return BaseClass::filterAcceptsColumn(sourceColumn, sourceParent);
}

bool CsvTableViewSortFilterProxyModel::filterAcceptsRow(const int sourceRow, const QModelIndex& sourceParent) const
{
    auto pOpaq = DFG_OPAQUE_PTR();

    const auto pSourceModel = (pOpaq && !pOpaq->m_matchers.empty()) ? qobject_cast<const CsvItemModel*>(this->sourceModel()) : nullptr;
    if (pSourceModel)
    {
        const auto nColCount = pSourceModel->columnCount();
        return pOpaq->m_matchers.isMatchByCallback([=](const CsvItemModelStringMatcher& matcher)
            {
                for (int c = 0; c < nColCount; ++c) if (matcher.isApplyColumn(c))
                {
                    if (matcher.isMatchWith(sourceRow, c, pSourceModel->rawStringViewAt(sourceRow, c)))
                        return true;
                }
                return false;
            });
    }
    else
        return BaseClass::filterAcceptsRow(sourceRow, sourceParent);
}

void CsvTableViewSortFilterProxyModel::setFilterFromNewLineSeparatedJsonList(const QByteArray& sJson)
{
    DFG_OPAQUE_REF().m_matchers = MultiMatchDefinition<CsvItemModelStringMatcher>::fromJson(SzPtrUtf8(sJson.data()));
    // In CsvItemModelStringMatcher row 1 means first non-header row, while here corresponding row is row 0 -> shifting apply rows.
    // Column doesn't need to be adjusted since there's no header and thus no 0 index difference.
    DFG_OPAQUE_REF().m_matchers.forEachWhile([](CsvItemModelStringMatcher& matcher)
        {
            matcher.m_rows.shift_raw(-CsvItemModel::internalRowToVisibleShift());
            return true;
        });
    this->invalidateFilter();
}


} } // namespace dfg::qt
/////////////////////////////////

#include "detail/CsvTableViewActions.cpp"
#include "detail/CsvTableView/SelectionDetailCollector.cpp"
#include "detail/CsvTableView/ContentGeneratorDialog.cpp"

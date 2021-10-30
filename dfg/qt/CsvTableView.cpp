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
#include <chrono>
#include <bitset>

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
    QString floatToQString(const T val)
    {
        return QString::fromLatin1(DFG_MODULE_NS(str)::floatingPointToStr<DFG_ROOT_NS::DFG_CLASS_NAME(StringAscii)>(val).c_str().c_str());
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

        QString uiValueStr(const BuiltInDetail id, const QItemSelection& selection) const
        {
            switch (id)
            {
                case BuiltInDetail::cellCountIncluded: return QString::number(m_avgMf.callCount());
                case BuiltInDetail::cellCountExcluded: return QString::number(m_nExcluded);
                case BuiltInDetail::sum              : return floatToQString(m_avgMf.sum());
                case BuiltInDetail::average          : return floatToQString(m_avgMf.average());
                case BuiltInDetail::median           : return floatToQString(m_medianMf.median());
                case BuiltInDetail::minimum          : return floatToQString(m_minMaxMf.minValue());
                case BuiltInDetail::maximum          : return floatToQString(m_minMaxMf.maxValue());
#if defined(BOOST_VERSION)
                case BuiltInDetail::variance         : return floatToQString(boost::accumulators::variance(m_varianceMf));
                case BuiltInDetail::stddev_population: return floatToQString(std::sqrt(boost::accumulators::variance(m_varianceMf)));
                case BuiltInDetail::stddev_sample    : return floatToQString(std::sqrt(double(m_avgMf.callCount()) / double(m_avgMf.callCount() - 1) * boost::accumulators::variance(m_varianceMf)));
#endif
                case BuiltInDetail::isSortedNum      : return privMakeIsSortedValueString(selection);
                default: DFG_ASSERT_IMPLEMENTED(false); return QString();
            }
        }

        static auto selectionDetailNameToId(const ::DFG_ROOT_NS::StringViewUtf8& svId) -> BuiltInDetail;
        static auto builtInDetailToStrId(BuiltInDetail detail) -> StringViewUtf8;

        QString resultString(const QItemSelection& selection) const
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
                addToResultString(uiName_short(id), uiValueStr(id, selection));
                return true;
            });
            for (auto pCollector : m_activeNonBuiltInCollectors)
            {
                if (pCollector)
                    addToResultString(pCollector->getUiName_short(), floatToQString(pCollector->value()));
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

}}} // dfg::qt::DFG_DETAIL_NS


using namespace DFG_MODULE_NS(qt);

namespace
{
    static const char gszDefaultOpenFileFilter[] = QT_TR_NOOP("CSV or SQLite files (*.csv *.tsv *.csv.conf *.sqlite3 *.sqlite *.db);; CSV files (*.csv *.tsv *.csv.conf);; SQLite files (*.sqlite3 *.sqlite *.db);; All files(*.*)");

    class ProgressWidget : public QProgressDialog
    {
    public:
        typedef QProgressDialog BaseClass;
        enum class IsCancellable { yes, no };

        ProgressWidget(const QString sLabelText, const IsCancellable isCancellable, QWidget* pParent)
            : BaseClass(sLabelText, QString(), 0, 0, pParent)
        {
            removeContextHelpButtonFromDialog(this);
            removeCloseButtonFromDialog(this);
            setWindowModality(Qt::WindowModal);
            if (isCancellable == IsCancellable::yes)
            {
                auto spCancelButton = std::unique_ptr<QPushButton>(new QPushButton(tr("Cancel"), this));
                connect(spCancelButton.get(), &QPushButton::clicked, [&]() { m_abCancelled = true; });
                setCancelButton(spCancelButton.release()); // "The progress dialog takes ownership"
            }
            else
                setCancelButton(nullptr); // Making sure that cancel button is not shown.
        }

        // Thread-safe
        bool isCancelled() const
        {
            return m_abCancelled.load(std::memory_order_relaxed);
        }

        std::atomic_bool m_abCancelled{ false };
    }; // Class ProgressWidget

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

    template <class T>
    QString floatToQString(const T val)
    {
        return ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::floatToQString(val);
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
        QObject::connect(pWorkerThread, &QThread::started, [&]()
                {
                    func(pProgressDialog);
                    pWorkerThread->quit();
                });
        // Connect thread finish to trigger event loop quit and closing of progress bar.
        QObject::connect(pWorkerThread, &QThread::finished, &eventLoop, &QEventLoop::quit);
        QObject::connect(pWorkerThread, &QThread::finished, pWorkerThread, &QObject::deleteLater);
        QObject::connect(pWorkerThread, &QObject::destroyed, pProgressDialog, &QObject::deleteLater);

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

    QString getOpenFileName(QWidget* pParent)
    {
        return QFileDialog::getOpenFileName(pParent,
                                            QApplication::tr("Open file"),
                                            QString()/*dir*/,
                                            QApplication::tr(gszDefaultOpenFileFilter),
                                            nullptr/*selected filter*/,
                                            QFileDialog::Options() /*options*/);
    }

    struct CsvTableViewFlag
    {
        enum
        {
            readOnly = 0
        };
    };

    struct ActionFlags
    {
        using uint32 = DFG_ROOT_NS::uint32;

        ActionFlags(uint32 flags)
            : m_nFlags(flags)
        {}

        enum
        {
            unknown               = 0x0,
            readOnly              = 0x1, // Makes no changes of any kind to table content nor to view.
            contentEdit           = 0x2, // May edit table content
            contentStructureEdit  = 0x4, // May edit table structure (changing row/column count etc.).
            viewEdit              = 0x8, // May edit view (selection, proxy/filter).
            anyContentEdit        = contentEdit | contentStructureEdit,
            defaultContentEdit    = contentEdit | viewEdit,
            defaultStructureEdit  = contentEdit | contentStructureEdit | viewEdit
        };

        operator uint32() const { return m_nFlags; }

        uint32 m_nFlags;
    };

    const char gszPropertyActionFlags[] = "dfglib_actionFlags";

    void setActionFlags(QAction* pAction, const ActionFlags flags)
    {
        if (!pAction)
            return;
        pAction->setProperty(gszPropertyActionFlags, DFG_ROOT_NS::uint32(flags));
    }

    ActionFlags getActionType(QAction* pAction)
    {
        return (pAction) ? ActionFlags(pAction->property(gszPropertyActionFlags).toUInt()) : ActionFlags::unknown;
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

DFG_OPAQUE_PTR_DEFINE(DFG_MODULE_NS(qt)::CsvTableView)
{
public:
    std::vector<::DFG_MODULE_NS(qt)::CsvTableView::PropertyFetcher> m_propertyFetchers;
    std::bitset<1> m_flags;
    QPointer<QAction> m_spActReadOnly;
    QAbstractItemView::EditTriggers m_editTriggers;
    QPalette m_readWriteModePalette;
};

::DFG_MODULE_NS(qt)::CsvTableView::CsvTableView(std::shared_ptr<QReadWriteLock> spReadWriteLock, QWidget* pParent, const ViewType viewType)
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
        pVertHdr->setDefaultSectionSize(gnDefaultRowHeight); // TODO: make customisable

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

void ::DFG_MODULE_NS(qt)::CsvTableView::addSeparatorAction()
{
    addSeparatorActionTo(this);
}

void ::DFG_MODULE_NS(qt)::CsvTableView::addSeparatorActionTo(QWidget* pTarget)
{
    if (!pTarget)
        return;
    auto pAction = new QAction(this);
    pAction->setSeparator(true);
    pTarget->addAction(pAction);
}

void ::DFG_MODULE_NS(qt)::CsvTableView::addAllActions()
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

void ::DFG_MODULE_NS(qt)::CsvTableView::addOpenSaveActions()
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
            if (QFileInfo::exists(QtApplication::getApplicationSettingsPath()))
            {
                addSeparatorActionTo(pMenu);
                auto& rAction = DFG_TEMP_ADD_VIEW_ACTION(*pMenu, tr("Open app config file"), noShortCut, ActionFlags::unknown, openAppConfigFile);
                DFG_QT_VERIFY_CONNECT(connect(this, &ThisClass::sigOnAllowApplicationSettingsUsageChanged, &rAction, &QAction::setVisible));
                rAction.setVisible(getAllowApplicationSettingsUsage());
            }
        }
    } // Config menu
}

void ::DFG_MODULE_NS(qt)::CsvTableView::addDimensionEditActions()
{
    // Header/first line move actions
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Move first row to header"), noShortCut, ActionFlags::defaultContentEdit, moveFirstRowToHeader);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Move header to first row"), noShortCut, ActionFlags::defaultContentEdit, moveHeaderToFirstRow);

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
            addViewAction(*this, *pMenu,     tr("Delete current column"),  tr("Ctrl+Del"),  ActionFlags::defaultStructureEdit, false, QOverload<void>::of(&ThisClass::deleteCurrentColumn));
        }
    } // End of "Insert row/column"-items

    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Resize table"), tr("Ctrl+R"), ActionFlags::defaultStructureEdit, resizeTable);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Transpose"),    noShortCut,   ActionFlags::defaultStructureEdit, transpose);
}

void ::DFG_MODULE_NS(qt)::CsvTableView::addFindAndSelectionActions()
{
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Invert selection"), tr("Ctrl+I"), ActionFlags::viewEdit, invertSelection);

    // Find and filter actions
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Go to line"),    tr("Ctrl+G"),   ActionFlags::viewEdit,           onGoToCellTriggered);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Find"),          tr("Ctrl+F"),   ActionFlags::viewEdit,           onFindRequested);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Find next"),     tr("F3"),       ActionFlags::viewEdit,           onFindNext);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Find previous"), tr("Shift+F3"), ActionFlags::viewEdit,           onFindPrevious);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Replace"),       tr("Ctrl+H"),   ActionFlags::defaultContentEdit, onReplace);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Filter"),        tr("Alt+F"),    ActionFlags::viewEdit,           onFilterRequested);
}

void::DFG_MODULE_NS(qt)::CsvTableView::addContentEditActions()
{
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Cut"),                          tr("Ctrl+X"), ActionFlags::defaultContentEdit, cut);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Copy"),                         tr("Ctrl+C"), ActionFlags::readOnly,           copy);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Paste"),                        tr("Ctrl+V"), ActionFlags::defaultContentEdit, paste);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Clear selected cell(s)"),       tr("Del"),    ActionFlags::defaultContentEdit, clearSelected);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Generate content..."),          tr("Alt+G"),  ActionFlags::defaultContentEdit, generateContent);
    DFG_TEMP_ADD_VIEW_ACTION(*this, tr("Evaluate selected as formula"), tr("Alt+C"),  ActionFlags::defaultContentEdit, evaluateSelectionAsFormula);

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

void ::DFG_MODULE_NS(qt)::CsvTableView::addSortActions()
{
    auto pMenu = createActionMenu(this, tr("Sorting"), ActionFlags::viewEdit);
    if (!pMenu)
        return;

    DFG_TEMP_ADD_VIEW_ACTION_CHECKABLE(*pMenu, tr("Sortable columns"),       noShortCut, ActionFlags::viewEdit, setSortingEnabled);
    DFG_TEMP_ADD_VIEW_ACTION_CHECKABLE(*pMenu, tr("Case sensitive sorting"), noShortCut, ActionFlags::viewEdit, setCaseSensitiveSorting);
    DFG_TEMP_ADD_VIEW_ACTION(*pMenu,           tr("Reset sorting"),          noShortCut, ActionFlags::viewEdit, resetSorting);

}

void ::DFG_MODULE_NS(qt)::CsvTableView::addHeaderActions()
{
    auto spMenu = createResizeColumnsMenu();
    if (!spMenu)
        return;

    createActionMenu(this, tr("Resize header"), ActionFlags::readOnly, spMenu.release());
}

void ::DFG_MODULE_NS(qt)::CsvTableView::addMiscellaneousActions()
{
    DFG_TEMP_ADD_VIEW_ACTION(*this,           tr("Diff with unmodified"), tr("Alt+D"), ActionFlags::readOnly, diffWithUnmodified);
    DFG_TEMP_ADD_VIEW_ACTION_CHECKABLE(*this, tr("Row mode"),             noShortCut,  ActionFlags::viewEdit, setRowMode).setToolTip(tr("Selections by row instead of by cell (experimental)"));
    auto& rActReadOnly = DFG_TEMP_ADD_VIEW_ACTION_CHECKABLE(*this, tr("Read-only"), noShortCut, ActionFlags::readOnly, setReadOnlyMode);
    rActReadOnly.setChecked(DFG_OPAQUE_REF().m_flags.test(CsvTableViewFlag::readOnly));
    DFG_OPAQUE_REF().m_spActReadOnly = &rActReadOnly;
}

DFG_CLASS_NAME(CsvTableView)::~DFG_CLASS_NAME(CsvTableView)()
{
    // Removing temporary files.
    for (auto iter = m_tempFilePathsToRemoveOnExit.cbegin(), iterEnd = m_tempFilePathsToRemoveOnExit.cend(); iter != iterEnd; ++iter)
    {
        QFile::remove(*iter);
    }
    stopAnalyzerThreads();
}

void DFG_CLASS_NAME(CsvTableView)::stopAnalyzerThreads()
{
    // Sending interrupt signals to analyzers.
    forEachSelectionAnalyzer([](SelectionAnalyzer& analyzer) { analyzer.disable(); });

    // Sending event loop stop request to analyzer threads.
    forEachSelectionAnalyzerThread([](QThread& thread) { thread.quit(); });
    // Waiting for analyzer threads to stop.
    forEachSelectionAnalyzerThread([](QThread& thread) { thread.wait(); });
}

template <class Func_T>
void DFG_CLASS_NAME(CsvTableView)::forEachSelectionAnalyzer(Func_T func)
{
    for (auto iter = m_selectionAnalyzers.cbegin(), iterEnd = m_selectionAnalyzers.cend(); iter != iterEnd; ++iter)
    {
        func(**iter);
    }
}

template <class Func_T>
void DFG_CLASS_NAME(CsvTableView)::forEachSelectionAnalyzerThread(Func_T func)
{
    for (auto iter = m_analyzerThreads.begin(), iterEnd = m_analyzerThreads.end(); iter != iterEnd; ++iter)
    {
        if (*iter)
            func(**iter);
    }
}

auto ::DFG_MODULE_NS(qt)::CsvTableView::createResizeColumnsMenu() -> std::unique_ptr<QMenu>
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

void DFG_CLASS_NAME(CsvTableView)::createUndoStack()
{
    m_spUndoStack.reset(new DFG_MODULE_NS(cont)::DFG_CLASS_NAME(TorRef)<QUndoStack>);
}

void DFG_CLASS_NAME(CsvTableView)::clearUndoStack()
{
    if (m_spUndoStack)
        m_spUndoStack->item().clear();
}

void DFG_CLASS_NAME(CsvTableView)::showUndoWindow()
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
}

void ::DFG_MODULE_NS(qt)::CsvTableView::privAddUndoRedoActions(QAction* pAddBefore)
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

void DFG_CLASS_NAME(CsvTableView)::setExternalUndoStack(QUndoStack* pUndoStack)
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

void DFG_CLASS_NAME(CsvTableView)::contextMenuEvent(QContextMenuEvent* pEvent)
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

void DFG_CLASS_NAME(CsvTableView)::setModel(QAbstractItemModel* pModel)
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
}

auto DFG_CLASS_NAME(CsvTableView)::csvModel() -> CsvModel*
{
    return csvModelImpl<CsvModel, QAbstractProxyModel>(model());
}

auto DFG_CLASS_NAME(CsvTableView)::csvModel() const -> const CsvModel*
{
   return csvModelImpl<const CsvModel, const QAbstractProxyModel>(model());
}

int DFG_CLASS_NAME(CsvTableView)::getFirstSelectedViewRow() const
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

std::vector<int> DFG_CLASS_NAME(CsvTableView)::getRowsOfCol(const int nCol, const QAbstractProxyModel* pProxy) const
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

QModelIndexList DFG_CLASS_NAME(CsvTableView)::selectedIndexes() const
{
    DFG_ASSERT_WITH_MSG(false, "Avoid using selectedIndexes() as it's behaviour is unclear when using proxies: selected indexes of proxy or underlying model?");
    return BaseClass::selectedIndexes();
}

int DFG_CLASS_NAME(CsvTableView)::getRowCount_dataModel() const
{
    auto pModel = this->csvModel();
    return (pModel) ? pModel->getRowCount() : 0;
}

int DFG_CLASS_NAME(CsvTableView)::getRowCount_viewModel() const
{
    auto pModel = this->model();
    return (pModel) ? pModel->rowCount() : 0;
}

QModelIndexList DFG_CLASS_NAME(CsvTableView)::getSelectedItemIndexes_dataModel() const
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

QModelIndexList DFG_CLASS_NAME(CsvTableView)::getSelectedItemIndexes_viewModel() const
{
    auto indexes = getSelectedItemIndexes_dataModel();
    auto pProxy = getProxyModelPtr();
    if (pProxy)
        std::transform(indexes.begin(), indexes.end(), indexes.begin(), [=](const QModelIndex& index) { return pProxy->mapFromSource(index); });
    return indexes;
}

std::vector<int> DFG_CLASS_NAME(CsvTableView)::getRowsOfSelectedItems(const QAbstractProxyModel* pProxy) const
{
    QModelIndexList listSelected = (!pProxy) ? getSelectedItemIndexes_viewModel() : getSelectedItemIndexes_dataModel();

    ::DFG_MODULE_NS(cont)::SetVector<int> rows;
    for (const auto& listIndex : listSelected)
    {
        if (listIndex.isValid())
            rows.insert(listIndex.row());
    }
    return std::move(rows.m_storage);
}

void DFG_CLASS_NAME(CsvTableView)::invertSelection()
{
    BaseClass::invertSelection();
}

bool DFG_CLASS_NAME(CsvTableView)::isRowMode() const
{
    return (selectionBehavior() == QAbstractItemView::SelectRows);
}

void DFG_CLASS_NAME(CsvTableView)::setRowMode(const bool b)
{
    setSelectionBehavior((b) ? QAbstractItemView::SelectRows : QAbstractItemView::SelectItems);
}

void DFG_CLASS_NAME(CsvTableView)::setUndoEnabled(const bool bEnable)
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

bool ::DFG_MODULE_NS(qt)::CsvTableView::isReadOnlyMode() const
{
    return DFG_OPAQUE_PTR() && DFG_OPAQUE_PTR()->m_flags.test(CsvTableViewFlag::readOnly);
}

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt) { namespace DFG_DETAIL_NS
{
    void handleActionForReadOnly(QAction* pAction, const bool bReadOnly)
    {
        if (!pAction)
            return;
        const auto type = getActionType(pAction);
        if ((type & ActionFlags::anyContentEdit) != 0)
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
}}}

void ::DFG_MODULE_NS(qt)::CsvTableView::setReadOnlyMode(const bool bReadOnly)
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
            newPalette.setColor(QPalette::Base, QColor(248, 248, 248));
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
void ::DFG_MODULE_NS(qt)::CsvTableView::setReadOnlyModeFromProperty(const Str_T& s)
{
    if (s == "readOnly")
        this->setReadOnlyMode(true);
    else if (s == "readWrite")
        this->setReadOnlyMode(false);
}

void ::DFG_MODULE_NS(qt)::CsvTableView::insertGeneric(const QString& s)
{
    auto pModel = model();
    if (!pModel)
        return;
    auto lockReleaser = this->tryLockForEdit();
    if (!lockReleaser.isLocked() || isReadOnlyMode()) // In read-only mode, should not even end up to this function, but checking isReadOnlyMode() for robustness.
    {
        privShowExecutionBlockedNotification(tr("Insert date/time"));
        return;
    }
    forEachIndexInSelection(*this, ModelIndexTypeView, [&](const QModelIndex& index, bool& bContinue)
    {
        DFG_UNUSED(bContinue);
        pModel->setData(index, s);
    });
}

auto ::DFG_MODULE_NS(qt)::CsvTableView::insertDate() -> QDate
{
    const auto date = QDate::currentDate();
    insertGeneric(dateTimeToString(date, getCsvModelOrViewProperty<CsvTableViewPropertyId_dateFormat>(this)));
    return date;
}

auto ::DFG_MODULE_NS(qt)::CsvTableView::insertTime() -> QTime
{
    const auto t = QTime::currentTime();
    insertGeneric(dateTimeToString(t, getCsvModelOrViewProperty<CsvTableViewPropertyId_timeFormat>(this)));
    return t;
}

auto ::DFG_MODULE_NS(qt)::CsvTableView::insertDateTime() -> QDateTime
{
    const auto dt = QDateTime::currentDateTime();
    insertGeneric(dateTimeToString(dt, getCsvModelOrViewProperty<CsvTableViewPropertyId_dateTimeFormat>(this)));
    return dt;
}

bool DFG_CLASS_NAME(CsvTableView)::saveToFileImpl(const DFG_ROOT_NS::DFG_CLASS_NAME(CsvFormatDefinition)& formatDef)
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

bool ::DFG_MODULE_NS(qt)::CsvTableView::saveToFileImpl(const QString& path, const CsvFormatDefinition& formatDef)
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
    const bool bSaveAsShown = (formatDef.getProperty("CsvTableView_saveAsShown", "") == "1");

    auto pModel = csvModel();
    if (!pModel)
        return false;

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
                const auto pStr = pModel->RawStringPtrAt(sourceIndex.row(), sourceIndex.column());
                saveAsShownModel.setDataNoUndo(r, c, pStr);
            }
        }
        pModel = &saveAsShownModel;
    }

    if (bSaveAsSqlite && QMessageBox::question(this, tr("SQLite export"), tr("SQlite export is rudimentary, continue anyway?")) != QMessageBox::Yes)
        return false;
        

    bool bSuccess = false;
    doModalOperation(this, tr("Saving to file\n%1").arg(path), ProgressWidget::IsCancellable::no, "CsvTableViewFileWriter", [&](ProgressWidget*)
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


bool DFG_CLASS_NAME(CsvTableView)::save()
{
    auto model = csvModel();
    const auto& path = (model) ? model->getFilePath() : QString();
    if (!path.isEmpty())
        return saveToFileImpl(path, model->getSaveOptions());
    else
        return saveToFile();
}

bool DFG_CLASS_NAME(CsvTableView)::saveToFile()
{
    auto pModel = csvModel();
    return (pModel) ? saveToFileImpl(pModel->getSaveOptions()) : false;
}

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt) { namespace DFG_DETAIL_NS {

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
} } }


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
        ,  m_saveOptions(pModel)
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
            m_spContentFilterWidget.reset(new JsonListWidget(this));
        }

        const auto defaultFormatSettings = (isSaveDialog()) ? m_saveOptions : peekCsvFormatFromFile(qStringToFileApi8Bit(sFilePath));

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
            addCurrentOptionToCombobox(*m_spEolEdit, DFG_MODULE_NS(io)::eolStrFromEndOfLineType(defaultFormatSettings.eolType()).c_str());
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

            // Content filters
            spLayout->addRow(tr("Content filters"), m_spContentFilterWidget.get());
            DFG_REQUIRE(m_spContentFilterWidget != nullptr);
            const auto loadOptions = CsvItemModel::getLoadOptionsForFile(sFilePath);
            const auto readFilters = loadOptions.getProperty(CsvOptionProperty_readFilters, "");
            m_spContentFilterWidget->setPlaceholderText(tr("List of filters, see tooltip for syntax guide"));
            if (!readFilters.empty())
                m_spContentFilterWidget->setPlainText(QString::fromUtf8(readFilters.c_str()));
            
            m_spContentFilterWidget->setToolTip(::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::szContentFilterHelpText);
            m_spContentFilterWidget->setMaximumHeight(100);
        }
        //spLayout->addRow(new QLabel(tr("Note: "), this));

        auto& rButtonBox = *(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel));

        DFG_QT_VERIFY_CONNECT(connect(&rButtonBox, SIGNAL(accepted()), this, SLOT(accept())));
        DFG_QT_VERIFY_CONNECT(connect(&rButtonBox, SIGNAL(rejected()), this, SLOT(reject())));

        spLayout->addRow(QString(), &rButtonBox);
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
                m_saveOptions.setProperty("CsvTableView_saveAsShown", "1");
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
    QObjectStorage<JsonListWidget> m_spContentFilterWidget;
}; // Class CsvFormatDefinitionDialog

bool CsvTableView::saveToFileWithOptions()
{
    CsvFormatDefinitionDialog dlg(this, CsvFormatDefinitionDialog::DialogTypeSave, csvModel());
    if (dlg.exec() != QDialog::Accepted)
        return false;
    return saveToFileImpl(dlg.getSaveOptions());
}

auto ::DFG_MODULE_NS(qt)::CsvTableView::populateCsvConfig(const CsvItemModel& rDataModel) -> CsvConfig
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
    for (const auto& fetcher : DFG_OPAQUE_REF().m_propertyFetchers)
    {
        if (!fetcher)
            continue;
        const auto& keyValue = fetcher();
        config.setKeyValue(qStringToStringUtf8(keyValue.first), qStringToStringUtf8(keyValue.second));
    }

    return config;
}

bool ::DFG_MODULE_NS(qt)::CsvTableView::saveConfigFileTo(const CsvConfig& config, const QString& sPath)
{
    const auto bSuccess = config.saveToFile(qStringToFileApi8Bit(sPath));
    if (!bSuccess)
        QMessageBox::information(this, tr("Saving failed"), tr("Saving config file to path '%1' failed").arg(sPath));

    return bSuccess;
}

auto ::DFG_MODULE_NS(qt)::CsvTableView::askConfigFilePath(CsvItemModel& rModel) -> QString
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

bool ::DFG_MODULE_NS(qt)::CsvTableView::saveConfigFile()
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

bool ::DFG_MODULE_NS(qt)::CsvTableView::saveConfigFileWithOptions()
{
    auto pModel = csvModel();
    if (!pModel)
        return false;

    auto config = populateCsvConfig(*pModel);

    CsvTableViewDlg viewDlg(nullptr, this, ViewType::fixedDimensionEdit);
    viewDlg.addVerticalLayoutWidget(0, new QLabel(tr("To omit item from saved config, clear related text from Key-column"), &viewDlg.dialog()));
    CsvItemModel configModel;

    configModel.insertRows(0, saturateCast<int>(config.entryCount()));
    configModel.insertColumns(0, 2);
    configModel.setColumnName(0, tr("Key"));
    configModel.setColumnName(1, tr("Value"));

    int nRow = 0;
    config.forEachKeyValue([&](const StringViewUtf8& svKey, const StringViewUtf8& svValue)
    {
        configModel.setItem(nRow, 0, svKey);
        configModel.setItem(nRow, 1, svValue);
        ++nRow;
    });

    viewDlg.setModel(&configModel);

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

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableView)::openAppConfigFile()
{
    const auto sPath = DFG_CLASS_NAME(QtApplication)::getApplicationSettingsPath();
    if (sPath.isEmpty())
        return false;
    QFileInfo fi(sPath);
    if (fi.isExecutable() || !fi.isReadable())
        return false;
    return QDesktopServices::openUrl(QUrl(QString("file:///%1").arg(sPath)));
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableView)::openConfigFile()
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
    return openFile(sPath, CsvItemModel::getLoadOptionsForFile(sPath));
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
    doModalOperation(this, tr("Reading file of size %1\n%2%3").arg(formattedDataSize(QFileInfo(sPath).size()), sPath, sAdditionalInfo), ProgressWidget::IsCancellable::yes, "CsvTableViewFileLoader", [&](ProgressWidget* pProgressWidget)
        {
            CsvItemModel::LoadOptions loadOptions(formatDef);
            bool bHasProgress = false;
            if (!bOpenAsSqlite && !loadOptions.isFilteredRead() && pProgressWidget) // Currently progress indicator works only for non-filtered csv-files.
            {
                pProgressWidget->setRange(0, 100);
                bHasProgress = true;
            }
            auto lastSetValue = std::chrono::steady_clock::now();
            if (pProgressWidget)
            {
                loadOptions.setProgressController(CsvModel::LoadOptions::ProgressController([&](const uint64 nProcessedBytes)
                {
                    // Calling setValue for progressWidget; note that using invokeMethod() since progressWidget lives in another thread.
                    // Also limiting call rate to maximum of once per 50 ms to prevent calls getting queued if callback gets called more often than what setValues can be invoked.
                    const auto steadyNow = std::chrono::steady_clock::now();
                    if (steadyNow - lastSetValue > std::chrono::milliseconds(50))
                    {
                        const bool bCancelled = pProgressWidget->isCancelled();
                        if (bHasProgress && !bCancelled)
                            DFG_VERIFY(QMetaObject::invokeMethod(pProgressWidget, "setValue", Qt::QueuedConnection, QGenericReturnArgument(), Q_ARG(int, ::DFG_ROOT_NS::round<int>(100.0 * static_cast<double>(nProcessedBytes) / fileSizeDouble))));
                        lastSetValue = steadyNow;
                        return !bCancelled;
                    }
                    return true;
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

    const auto scrollPos = getCsvTableViewProperty<CsvTableViewPropertyId_initialScrollPosition>(this);
    if (scrollPos == "bottom")
        scrollToBottom();

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

    if (bSuccess)
        onNewSourceOpened();
    else
    {
        const QString sInfoPart = (!pModel->m_messagesFromLatestOpen.isEmpty()) ? tr("\nThe following message(s) were generated:\n %1").arg(pModel->m_messagesFromLatestOpen.join('\n')) : QString();
        QMessageBox::information(this, tr("Open failed"), tr("Opening file\n%1\nfailed.%2.").arg(sPath, sInfoPart));
    }

    return bSuccess;
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableView)::getProceedConfirmationFromUserIfInModifiedState(const QString& sTranslatedActionDescription)
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

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableView)::createNewTable()
{
    auto pCsvModel = csvModel();
    if (!pCsvModel || !getProceedConfirmationFromUserIfInModifiedState(tr("open a new table")))
        return;
    setReadOnlyMode(false); // Resetting read-only mode since user likely wants to edit empty table.
    pCsvModel->openNewTable();
}

bool DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableView)::createNewTableFromClipboard()
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

    doModalOperation(this, tr("Reading from clipboard, input size is %1").arg(sClipboardText.size()), ProgressWidget::IsCancellable::yes, "CsvTableViewClipboardLoader", [&](ProgressWidget*)
    {
        bSuccess = pCsvModel->openFromMemory(sClipboardText.data(), static_cast<size_t>(sClipboardText.size()), loadOptions);
    });

    if (bSuccess)
        onNewSourceOpened();

    return bSuccess;
}

bool DFG_CLASS_NAME(CsvTableView)::openFromFile()
{
    if (!getProceedConfirmationFromUserIfInModifiedState(tr("open a new file")))
        return false;
    return openFile(getOpenFileName(this));
}

bool DFG_CLASS_NAME(CsvTableView)::openFromFileWithOptions()
{
    if (!getProceedConfirmationFromUserIfInModifiedState(tr("open a new file")))
        return false;
    const auto sPath = getOpenFileName(this);
    if (sPath.isEmpty())
        return false;
    CsvFormatDefinitionDialog::LoadOptions loadOptions;
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

auto ::DFG_MODULE_NS(qt)::CsvTableView::reloadFromFile() -> bool
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

bool DFG_CLASS_NAME(CsvTableView)::mergeFilesToCurrent()
{
    auto sPaths = QFileDialog::getOpenFileNames(this,
        tr("Select files to merge"),
        QString()/*dir*/,
        tr(gszDefaultOpenFileFilter),
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
bool DFG_CLASS_NAME(CsvTableView)::executeAction(Param0_T&& p0)
{
    DFG_TEMP_HANDLE_EXECUTE_ACTION_LOCKING;
    
    if (m_spUndoStack && m_bUndoEnabled)
        pushToUndoStack<T>(std::forward<Param0_T>(p0));
    else
        DFG_CLASS_NAME(UndoCommand)::directRedo<T>(std::forward<Param0_T>(p0));

    return true;
}

template <class T, class Param0_T, class Param1_T>
bool DFG_CLASS_NAME(CsvTableView)::executeAction(Param0_T&& p0, Param1_T&& p1)
{
    DFG_TEMP_HANDLE_EXECUTE_ACTION_LOCKING;

    if (m_spUndoStack && m_bUndoEnabled)
        pushToUndoStack<T>(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1));
    else
        DFG_CLASS_NAME(UndoCommand)::directRedo<T>(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1));

    return true;
}

template <class T, class Param0_T, class Param1_T, class Param2_T>
bool DFG_CLASS_NAME(CsvTableView)::executeAction(Param0_T&& p0, Param1_T&& p1, Param2_T&& p2)
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
void DFG_CLASS_NAME(CsvTableView)::pushToUndoStack(Param0_T&& p0)
{
    if (!m_spUndoStack)
        createUndoStack();
    QUndoCommand* command = new T(std::forward<Param0_T>(p0));
    (*m_spUndoStack)->push(command); // Stack takes ownership of command.
}

template <class T, class Param0_T, class Param1_T>
void DFG_CLASS_NAME(CsvTableView)::pushToUndoStack(Param0_T&& p0, Param1_T&& p1)
{
    if (!m_spUndoStack)
        createUndoStack();
    QUndoCommand* command = new T(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1));
    (*m_spUndoStack)->push(command); // Stack takes ownership of command.
}

template <class T, class Param0_T, class Param1_T, class Param2_T>
void DFG_CLASS_NAME(CsvTableView)::pushToUndoStack(Param0_T&& p0, Param1_T&& p1, Param2_T&& p2)
{
    if (!m_spUndoStack)
        createUndoStack();
    QUndoCommand* command = new T(std::forward<Param0_T>(p0), std::forward<Param1_T>(p1), std::forward<Param2_T>(p2));
    (*m_spUndoStack)->push(command); // Stack takes ownership of command.
}

bool DFG_CLASS_NAME(CsvTableView)::clearSelected()
{
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionDelete)>(*this, getProxyModelPtr(), false /*false = not row mode*/);
}

bool DFG_CLASS_NAME(CsvTableView)::insertRowHere()
{
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionInsertRow)>(this, DFG_SUB_NS_NAME(undoCommands)::InsertRowTypeBefore);
}

bool DFG_CLASS_NAME(CsvTableView)::insertRowAfterCurrent()
{
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionInsertRow)>(this, DFG_SUB_NS_NAME(undoCommands)::InsertRowTypeAfter);
}

bool DFG_CLASS_NAME(CsvTableView)::insertColumn()
{
    const auto nCol = currentIndex().column();
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionInsertColumn)>(csvModel(), nCol);
}

bool DFG_CLASS_NAME(CsvTableView)::insertColumnAfterCurrent()
{
    const auto nCol = currentIndex().column();
    if (nCol >= 0)
        return executeAction<DFG_CLASS_NAME(CsvTableViewActionInsertColumn)>(csvModel(), nCol + 1);
    else
        return false;
}

bool DFG_CLASS_NAME(CsvTableView)::cut()
{
    copy();
    clearSelected();
    return true;
}

void ::DFG_MODULE_NS(qt)::CsvTableView::undo()
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

void ::DFG_MODULE_NS(qt)::CsvTableView::redo()
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

    doModalOperation(this, tr("Executing Find & Replace..."), ProgressWidget::IsCancellable::yes, "find_replace", [&](ProgressWidget* pProgressDialog)
    {
        const auto bOldModifiedStatus = pCsvModel->isModified();
        pCsvModel->batchEditNoUndo([&](CsvItemModel::DataTable& table)
        {
            StringUtf8 sTemp;
            forEachCsvModelIndexInSelection([&](const QModelIndex& index, bool& rbContinue)
            {
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

QString DFG_MODULE_NS(qt)::CsvTableView::makeClipboardStringForCopy(QChar cDelim)
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

bool DFG_CLASS_NAME(CsvTableView)::copy()
{
    QString str = makeClipboardStringForCopy();

    QApplication::clipboard()->setText(str);
    return true;
}

bool DFG_CLASS_NAME(CsvTableView)::paste()
{
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionPaste)>(this);
}

bool DFG_CLASS_NAME(CsvTableView)::deleteCurrentColumn()
{
    const auto curIndex = currentIndex();
    const int nRow = curIndex.row();
    const int nCol = curIndex.column();
    const auto rv = deleteCurrentColumn(nCol);
    selectionModel()->setCurrentIndex(model()->index(nRow, nCol), QItemSelectionModel::NoUpdate);
    return rv;
}

bool DFG_CLASS_NAME(CsvTableView)::deleteCurrentColumn(const int nCol)
{
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionDeleteColumn)>(this, nCol);
}

bool DFG_CLASS_NAME(CsvTableView)::deleteSelectedRow()
{
    return executeAction<DFG_CLASS_NAME(CsvTableViewActionDelete)>(*this, getProxyModelPtr(), true /*row mode*/);
}

bool ::DFG_MODULE_NS(qt)::CsvTableView::resizeTable()
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

bool ::DFG_MODULE_NS(qt)::CsvTableView::resizeTableNoUi(const int r, const int c)
{
    return executeAction<CsvTableViewActionResizeTable>(this, r, c);
}

bool ::DFG_MODULE_NS(qt)::CsvTableView::transpose()
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

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QDialog>
    #include <QComboBox>
    #include <QStyledItemDelegate>
DFG_END_INCLUDE_QT_HEADERS

namespace
{
    // Note: ID's should match values in arrPropDefs-array.
    enum PropertyId
    {
        PropertyIdInvalid = -1,
        PropertyIdTarget,
        PropertyIdGenerator,
        LastNonParamPropertyId = PropertyIdGenerator
    };

    enum GeneratorType
    {
        GeneratorTypeUnknown,
        GeneratorTypeRandomIntegers,
        GeneratorTypeRandomDoubles,
        GeneratorTypeFill,
        GeneratorTypeFormula,
        GeneratorType_last = GeneratorTypeFormula
    };

    enum TargetType
    {
        TargetTypeUnknown,
        TargetTypeSelection,
        TargetTypeWholeTable,
        TargetType_last = TargetTypeWholeTable
    };

    enum ValueType
    {
        ValueTypeKeyList,
        ValueTypeInteger,
        ValueTypeUInteger = ValueTypeInteger,
        ValueTypeDouble,
        ValueTypeString,
        ValueTypeCsvList,
    };

    struct PropertyDefinition
    {
        const char* m_pszName;
        int m_nType;
        const char* m_keyList;
        const char* m_pszDefault;
        const char** m_pCompleterItems; // Array of strings defining the list of completer items, must end with nullptr item.
    };
    
    // Defines completer items for integer distribution parameters
    const char* integerDistributionCompleters[] =
    {
        "uniform, 0, 100",
        "binomial, 10, 0.5",
        "bernoulli, 0.5",
        "negative_binomial, 10, 0.5",
        "geometric, 0.5",
        "poisson, 10",
        nullptr // End-of-list marker
    };

    // Defines completer items for real distribution parameters
    const char* realDistributionCompleters[] =
    {
        "uniform, 0, 1",
        "normal, 0, 1",
        "cauchy, 0, 1",
        "exponential, 1",
        "gamma, 1, 1",
        "weibull, 1, 1",
        "extreme_value, 0, 1",
        "lognormal, 0, 1",
        "chi_squared, 1",
        "fisher_f, 1, 1",
        "student_t, 1",
        nullptr // End-of-list marker
    };

    // In syntax |x;y;z... items x,y,z define the indexes in table below that are parameters for given item.
    const char szGenerators[] = "Random integers|9,Random doubles|10;6;7,Fill|8,Formula|2;6;7";

    // Note: this is a POD-table (for notes about initialization of PODs, see
    //    -http://stackoverflow.com/questions/2960307/pod-global-object-initialization
    //    -http://stackoverflow.com/questions/15212261/default-initialization-of-pod-types-in-c
    const PropertyDefinition arrPropDefs[] =
    {
        // Key name               Value type           Value items (if key type is list)                      Default value
        { "Target"              , ValueTypeKeyList  , "Selection,Whole table"                               , "Selection"      , nullptr }, // 0
        { "Generator"           , ValueTypeKeyList  , szGenerators                                          , "Random integers", nullptr }, // 1
        { "Formula"             , ValueTypeCsvList  , ""                                                    , "trow + tcol"    , nullptr }, // 2
        { "Unused"              , ValueTypeInteger  , ""                                                    , ""               , nullptr }, // 3
        { "Unused"              , ValueTypeDouble   , ""                                                    , ""               , nullptr }, // 4
        { "Unused"              , ValueTypeDouble   , ""                                                    , ""               , nullptr }, // 5
        { "Format type"         , ValueTypeString   , ""                                                    , "g"              , nullptr }, // 6
        { "Format precision"    , ValueTypeUInteger , ""                                                    , "6"              , nullptr }, // 7. Note: empty value must be supported as well.
        { "Fill string"         , ValueTypeString   , ""                                                    , ""               , nullptr }, // 8
        { "Parameters"          , ValueTypeCsvList  , ""                                                    , "uniform, 0, 100", integerDistributionCompleters }, // 9
        { "Parameters"          , ValueTypeCsvList  , ""                                                    , "uniform, 0, 1"  , realDistributionCompleters }  // 10
    };

    PropertyId rowToPropertyId(const int r)
    {
        if (r == 0)
            return PropertyIdTarget;
        else if (r == 1)
            return PropertyIdGenerator;
        else
            return PropertyIdInvalid;
    }

    QStringList valueListFromProperty(const PropertyId id)
    {
        if (!::DFG_ROOT_NS::isValidIndex(arrPropDefs, id))
            return QStringList();
        QStringList items = QString(arrPropDefs[id].m_keyList).split(',');
        for (auto iter = items.begin(); iter != items.end(); ++iter)
        {
            const auto n = iter->indexOf('|');
            if (n < 0)
                continue;
            iter->remove(n, iter->length() - n);
        }
        return items;
    }

    class ContentGeneratorDialog;

    class ComboBoxDelegate : public QStyledItemDelegate
    {
        typedef QStyledItemDelegate BaseClass;

    public:
        ComboBoxDelegate(ContentGeneratorDialog* parent);

        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

        void setEditorData(QWidget *editor, const QModelIndex &index) const override;
        void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

        void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

        ContentGeneratorDialog* m_pParentDialog;
    };

    GeneratorType generatorType(const DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)& csvModel)
    {
        // TODO: use more reliable detection (string comparison does not work with tr())
        DFG_STATIC_ASSERT(GeneratorType_last == 4, "This implementation handles only 4 generator types");
        const auto& sGenerator = csvModel.data(csvModel.index(1, 1)).toString();
        if (sGenerator == "Random integers")
            return GeneratorTypeRandomIntegers;
        else if (sGenerator == "Random doubles")
            return GeneratorTypeRandomDoubles;
        else if (sGenerator == "Fill")
            return GeneratorTypeFill;
        else if (sGenerator == "Formula")
            return GeneratorTypeFormula;
        else
        {
            DFG_ASSERT_IMPLEMENTED(false);
            return GeneratorTypeUnknown;
        }
    }

    class ContentGeneratorDialog : public QDialog
    {
    public:
        typedef QDialog BaseClass;
        ContentGeneratorDialog(QWidget* pParent) :
            BaseClass(pParent),
            m_pLayout(nullptr),
            m_pGeneratorControlsLayout(nullptr),
            m_nLatestComboBoxItemIndex(-1)
        {
            m_spSettingsTable.reset(new CsvTableView(nullptr, this));
            m_spSettingsModel.reset(new CsvItemModel);
            m_spDynamicHelpWidget.reset(new QLabel(this));
            m_spDynamicHelpWidget->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
            m_spSettingsTable->setModel(m_spSettingsModel.get());
            m_spSettingsTable->setItemDelegate(new ComboBoxDelegate(this));


            m_spSettingsModel->insertColumns(0, 2);

            m_spSettingsModel->setColumnName(0, tr("Parameter"));
            m_spSettingsModel->setColumnName(1, tr("Value"));

            // Set streching for the last column
            {
                auto pHeader = m_spSettingsTable->horizontalHeader();
                if (pHeader)
                    pHeader->setStretchLastSection(true);
            }
            //
            {
                auto pHeader = m_spSettingsTable->verticalHeader();
                if (pHeader)
                    pHeader->setDefaultSectionSize(30);
            }
            m_pLayout = new QVBoxLayout(this);

            m_pLayout->addWidget(m_spSettingsTable.get());

            for (size_t i = 0; i <= LastNonParamPropertyId; ++i)
            {
                const auto nRow = m_spSettingsModel->rowCount();
                m_spSettingsModel->insertRows(nRow, 1);
                m_spSettingsModel->setData(m_spSettingsModel->index(nRow, 0), tr(arrPropDefs[i].m_pszName));
                m_spSettingsModel->setData(m_spSettingsModel->index(nRow, 1), arrPropDefs[i].m_pszDefault);
            }

            m_pLayout->addWidget(new QLabel(tr("Note: undo is not available for content generation (clears undo buffer)"), this));
            auto pStaticHelpLabel = new QLabel(tr("Available formats:\n"
                "    Numbers: see documentation of printf\n"
                "    Date times:\n"
                "        date_sec_local, date_msec_local: value is interpreted as epoch time in seconds or milliseconds and converted to local time\n"
                "        date_sec_utc, date_msec_utc: like date_sec_local, but converted to UTC time\n"
                "        Format precision accepts formats defined for QDateTime, for example yyyy-MM-dd HH:mm:ss.zzz"), this);
            pStaticHelpLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
            m_pLayout->addWidget(pStaticHelpLabel);
            m_pLayout->addWidget(m_spDynamicHelpWidget.get());

            // Adding spacer item to avoid labels expanding, which doesn't look good.
            m_pLayout->addStretch();

            auto& rButtonBox = *(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel));

            DFG_QT_VERIFY_CONNECT(connect(&rButtonBox, SIGNAL(accepted()), this, SLOT(accept())));
            DFG_QT_VERIFY_CONNECT(connect(&rButtonBox, SIGNAL(rejected()), this, SLOT(reject())));

            m_pLayout->addWidget(&rButtonBox);

            createPropertyParams(PropertyIdGenerator, 0);

            DFG_QT_VERIFY_CONNECT(connect(m_spSettingsModel.get(), &QAbstractItemModel::dataChanged, this, &ContentGeneratorDialog::onDataChanged));

            updateDynamicHelp();
            removeContextHelpButtonFromDialog(this);
        }

        void setGenerateFailed(bool bFailed, const QString& sFailReason = QString());

        PropertyId rowToPropertyId(const int i) const
        {
            if (i == PropertyIdGenerator)
                return PropertyIdGenerator;
            else
                return PropertyIdInvalid;
        }

        int propertyIdToRow(const PropertyId propId) const
        {
            if (propId == PropertyIdGenerator)
                return 1;
            else
                return -1;
        }

        std::vector<std::reference_wrapper<const PropertyDefinition>> generatorParameters(const int itemIndex)
        {
            std::vector<std::reference_wrapper<const PropertyDefinition>> rv;
            QString sKeyList = arrPropDefs[PropertyIdGenerator].m_keyList;
            const auto keys = sKeyList.split(',');
            if (!DFG_ROOT_NS::isValidIndex(keys, itemIndex))
                return rv;
            QString sName = keys[itemIndex];
            const auto n = sName.indexOf('|');
            if (n < 0)
                return rv;
            sName.remove(0, n + 1);
            const auto paramIndexes = sName.split(';');
            for (const auto& s : paramIndexes)
            {
                bool bOk = false;
                const auto index = s.toInt(&bOk);
                if (bOk && DFG_ROOT_NS::isValidIndex(arrPropDefs, index))
                    rv.push_back(arrPropDefs[index]);
                else
                {
                    DFG_ASSERT(false);
                }
            }
            return rv;
        }

        void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>&/* roles*/)
        {
            const auto isCellInIndexRect = [](int r, int c, const QModelIndex& tl, const QModelIndex&  br)
                                            {
                                                return (r >= tl.row() && r <= br.row() && c >= tl.column() && c <= br.column());
                                            };
            if (isCellInIndexRect(1, 1, topLeft, bottomRight))
            {
                createPropertyParams(rowToPropertyId(1), m_nLatestComboBoxItemIndex);
                updateDynamicHelp();

                // Resizing table so that all parameter rows show
                if (m_spSettingsTable && m_spSettingsModel)
                {
                    auto pVerticalHeader = m_spSettingsTable->verticalHeader();
                    const auto nSectionSize = (pVerticalHeader) ? pVerticalHeader->sectionSize(0) : 30;
                    const auto nShowCount = m_spSettingsModel->rowCount() + 1; // + 1 for header
                    const auto nSize = nSectionSize * nShowCount + 2; // +2 to avoid scroll bar appearing, size isn't exactly row heigth * row count.
                    m_spSettingsTable->setMinimumHeight(nSize);
                    m_spSettingsTable->setMaximumHeight(nSize);
                }
            }
        }

        GeneratorType generatorType() const
        {
            if (m_spSettingsModel)
                return ::generatorType(*m_spSettingsModel);
            else
                return GeneratorTypeUnknown;
        }

        void setCompleter(const int nTargetRow, const char** pCompleterItems)
        {
            if (pCompleterItems)
            {
                auto& spCompleter = m_completers[pCompleterItems];
                if (!spCompleter) // If completer object does not exist, creating one.
                {
                    spCompleter.reset(new QCompleter(this));
                    QStringList completerItems;
                    for (auto p = pCompleterItems; *p != nullptr; ++p) // Expecting array to end with nullptr-entry.
                    {
                        completerItems.push_back(*p);
                    }
                    spCompleter->setModel(new QStringListModel(completerItems, spCompleter.data())); // Model is owned by completer object.
                }
                if (!m_spCompleterDelegateForDistributionParams)
                    m_spCompleterDelegateForDistributionParams.reset(new CsvTableViewCompleterDelegate(this, spCompleter.get()));
                m_spCompleterDelegateForDistributionParams->m_spCompleter = spCompleter.get();
                // Settings delegate for the row in order to enable completer. Note that delegate should not be set for first column, but done here since there seems to be no easy way to set it only
                // for the second cell in row.
                m_spSettingsTable->setItemDelegateForRow(nTargetRow, m_spCompleterDelegateForDistributionParams.data()); // Does not transfer ownership, delegate is parent owned by 'this'.
            }
            else
            {
                m_spSettingsTable->setItemDelegateForRow(nTargetRow, nullptr);
            }
        }

        void updateDynamicHelp()
        {
            const auto genType = generatorType();
            if (genType == GeneratorTypeRandomIntegers)
            {
                m_spDynamicHelpWidget->setText(tr("Available integer distributions (hint: there's a completer in parameter input, trigger with ctrl+space):<br>"
                    "<li><b>uniform, min, max</b>: Uniformly distributed values in range [min, max]. Requires min &lt;= max.</li>"
                    "<li><b>binomial, count, probability</b>: Binomial distribution. Requires count &gt;= 0, probability within [0, 1]</li>"
                    "<li><b>bernoulli, probability</b>: Bernoulli distribution. Requires probability within [0, 1]</li>"
                    "<li><b>negative_binomial, count, probability</b>: Negative binomial distribution. Requires count &gt; 0, probability within ]0, 1]</li>"
                    "<li><b>geometric, probability</b>: Geometric distribution. Requires probability within ]0, 1[</li>"
                    "<li><b>poisson, mean</b>: Poisson distribution. Requires mean &gt; 0</li>"));
            }
            else if (genType == GeneratorTypeRandomDoubles)
            {
                m_spDynamicHelpWidget->setText(tr("Available real distributions (hint: there's a completer in input line, trigger with ctrl+space):<br>"
                    "<li><b>uniform, min, max</b>: Uniformly distributed values in range [min, max[. Requires min &lt; max.</li>"
                    "<li><b>normal, mean, stddev</b>: Normal distribution. Requires stddev &gt; 0</li>"
                    "<li><b>cauchy, a, b</b>: Cauchy (Lorentz) distribution. Requires b &gt; 0</li>"
                    "<li><b>exponential, lambda</b>: Exponential distribution. Requires lambda &gt; 0</li>"
                    "<li><b>gamma, alpha, beta</b>: Gamma distribution. Requires alpha &gt; 0, beta &gt; 0</li>"
                    "<li><b>weibull, a, b</b>: Geometric distribution. Requires a &gt; 0, b &gt; 0</li>"
                    "<li><b>extreme_value, a, b</b>: Extreme value distribution. Requires b &gt; 0</li>"
                    "<li><b>lognormal, m, s</b>: Lognormal distribution. Requires s &gt; 0</li>"
                    "<li><b>chi_squared, n</b>: Chi squared distribution. Requires n &gt; 0</li>"
                    "<li><b>fisher_f, a, b</b>: Fisher f distribution. Requires a &gt; 0, b &gt; 0</li>"
                    "<li><b>student_t, n</b>: Student's t distribution. Requires n &gt; 0</li>"));
            }
            else if (genType == GeneratorTypeFormula)
            {
                ::DFG_MODULE_NS(math)::FormulaParser parser;
                parser.defineRandomFunctions();
                QString sFuncNames;
                decltype(sFuncNames.length()) nLastBrPos = 0;
                parser.forEachDefinedFunctionNameWhile([&](const ::DFG_ROOT_NS::StringViewC& sv)
                {
                    sFuncNames += QString("%1, ").arg(QString::fromLatin1(sv.data(), sv.sizeAsInt()));
                    if (sFuncNames.length() - nLastBrPos > 80)
                    {
                        sFuncNames += "<br>&nbsp;&nbsp;&nbsp;&nbsp;";
                        nLastBrPos = sFuncNames.length();
                    }
                    return true;
                });
                sFuncNames.resize(sFuncNames.length() - 2); // Removing trailing ", ".
                m_spDynamicHelpWidget->setText(tr("Generates content using given formula.<br>"
                    "<b>Available variables and content-related functions</b>:"
                    "<li>&nbsp;&nbsp;&nbsp;&nbsp;<b>cellValue(row, column)</b>: Value of cell at (row, column) (1-based indexes). Content is generated from top to bottom.</li>"
                    "<li>&nbsp;&nbsp;&nbsp;&nbsp;<b>trow</b>: Row of cell to which content is being generated, 1-based index.</li>"
                    "<li>&nbsp;&nbsp;&nbsp;&nbsp;<b>tcol</b>: Column of cell to which content is being generated, 1-based index.</li>"
                    "<li>&nbsp;&nbsp;&nbsp;&nbsp;<b>rowcount</b>: Number of rows in table </li>"
                    "<li>&nbsp;&nbsp;&nbsp;&nbsp;<b>colcount</b>: Number of columns in table</li>"
                    "<b>Available functions:</b> %1<br>"
                    "<b>Note</b>: trow and tcol are table indexes: even when generating to sorted or filtered table, these are rows/columns of the underlying table, not those of shown.<br>"
                    "For example if a table of 5 rows is sorted so that row 5 is shown as first, trow value for that cell is 5, not 1. Currently there is no variable for accessing view rows/columns.<br>"
                    "<b>Example</b>: rowcount - trow + 1 (this generates descending row indexes, 1-based index)<br>"
                    "<b>Example</b>: cellValue(trow, tcol - 1) + cellValue(trow, tcol + 1) (value in each cell will be the sum of left and right neighbour cells)<br>"
                    "<b>Example</b>: cellValue(trow - 1, tcol) * 2 (value is each cell will be twice the value in cell above)"

                ).arg(sFuncNames));
            }
            else
            {
                m_spDynamicHelpWidget->setText(QString()); // There are no instructions for this generator type so clear help.
            }
        }

        void createPropertyParams(const PropertyId prop, const int itemIndex)
        {
            if (!m_spSettingsModel)
            {
                DFG_ASSERT(false);
                return;
            }
            if (prop == PropertyIdGenerator)
            {
                const auto& params = generatorParameters(itemIndex);
                const auto nParamCount = static_cast<int>(params.size());
                auto nBaseRow = propertyIdToRow(PropertyIdGenerator);
                if (nBaseRow < 0)
                {
                    DFG_ASSERT(false);
                    return;
                }
                ++nBaseRow;
                if (m_spSettingsModel->rowCount() < nBaseRow + nParamCount) // Need to add rows?
                    m_spSettingsModel->insertRows(nBaseRow, nBaseRow + nParamCount - m_spSettingsModel->rowCount());
                if (m_spSettingsModel->rowCount() > nBaseRow + nParamCount) // Need to remove rows?
                    m_spSettingsModel->removeRows(nBaseRow, m_spSettingsModel->rowCount() - (nBaseRow + nParamCount));
                for (int i = 0; i < nParamCount; ++i)
                {
                    const auto nTargetRow = nBaseRow + i;
                    m_spSettingsModel->setData(m_spSettingsModel->index(nTargetRow, 0), params[i].get().m_pszName);
                    m_spSettingsModel->setData(m_spSettingsModel->index(nTargetRow, 1), params[i].get().m_pszDefault);

                    setCompleter(nTargetRow, params[i].get().m_pCompleterItems);
                }
            }
            else
            {
                DFG_ASSERT_IMPLEMENTED(false);
            }
        }

        void setLatestComboBoxItemIndex(int index)
        {
            m_nLatestComboBoxItemIndex = index;
        }

        QVBoxLayout* m_pLayout;
        QGridLayout* m_pGeneratorControlsLayout;
        std::unique_ptr<DFG_CLASS_NAME(CsvTableView)> m_spSettingsTable;
        std::unique_ptr<DFG_CLASS_NAME(CsvItemModel)> m_spSettingsModel;
        QObjectStorage<QLabel> m_spDynamicHelpWidget;
        QObjectStorage<QLabel> m_spGenerateFailedNoteWidget;
        int m_nLatestComboBoxItemIndex;
        QObjectStorage<DFG_CLASS_NAME(CsvTableViewCompleterDelegate)> m_spCompleterDelegateForDistributionParams;
        std::map<const void*, QObjectStorage<QCompleter>> m_completers; // Maps completer definition (e.g. integerDistributionCompleters) to completer item.
    }; // Class ContentGeneratorDialog

    void ContentGeneratorDialog::setGenerateFailed(const bool bFailed, const QString& sFailReason)
    {
        if (m_pLayout)
        {
            if (bFailed)
            {
                if (!m_spGenerateFailedNoteWidget)
                {
                    m_spGenerateFailedNoteWidget.reset(new QLabel(this));
                    m_pLayout->insertWidget(m_pLayout->count() - 1, m_spGenerateFailedNoteWidget.get());
                }
                const QString sMsg = (sFailReason.isEmpty()) ?
                    tr("Note: Generating content failed; this may be caused by bad parameters") :
                    tr("Note: Generating content failed with reason:<br>%1").arg(sFailReason);
                m_spGenerateFailedNoteWidget->setText(QString("<font color=\"#ff0000\">%1</font>").arg(sMsg));
            }
            if (m_spGenerateFailedNoteWidget)
                m_spGenerateFailedNoteWidget->setVisible(bFailed);
        }
    }

    ComboBoxDelegate::ComboBoxDelegate(ContentGeneratorDialog* parent) :
        m_pParentDialog(parent)
    {
    }

    QWidget* ComboBoxDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        if (!index.isValid())
            return nullptr;
        const auto nRow = index.row();
        const auto nCol = index.column();
        if (nCol != 1) // Only second column is editable.
            return nullptr;

        else if (nRow < 2)
        {
            auto pComboBox = new QComboBox(parent);
            DFG_QT_VERIFY_CONNECT(connect(pComboBox, SIGNAL(currentIndexChanged(int)), pComboBox, SLOT(close()))); // TODO: check this, Qt's star delegate example connects differently here.
            return pComboBox;
        }
        else
            return BaseClass::createEditor(parent, option, index);
    }

    void ComboBoxDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
    {
        auto pModel = index.model();
        if (!pModel)
            return;
        auto pComboBoxDelegate = qobject_cast<QComboBox*>(editor);
        if (pComboBoxDelegate == nullptr)
        {
            BaseClass::setEditorData(editor, index);
            return;
        }
        //const auto keyVal = pModel->data(pModel->index(index.row(), index.column() - 1));
        const auto value = index.data(Qt::EditRole).toString();

        const auto& values = valueListFromProperty(rowToPropertyId(index.row()));

        pComboBoxDelegate->addItems(values);
        pComboBoxDelegate->setCurrentText(value);
    }

    void ComboBoxDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
    {
        if (!model)
            return;
        auto pComboBoxDelegate = qobject_cast<QComboBox*>(editor);
        if (pComboBoxDelegate)
        {
            const auto& value = pComboBoxDelegate->currentText();
            const auto selectionIndex = pComboBoxDelegate->currentIndex();
            if (m_pParentDialog)
                m_pParentDialog->setLatestComboBoxItemIndex(selectionIndex);
            model->setData(index, value, Qt::EditRole);
        }
        else
            BaseClass::setModelData(editor, model, index);

    }

    void ComboBoxDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        DFG_UNUSED(index);
        if (editor)
            editor->setGeometry(option.rect);
    }

    TargetType targetType(const DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvItemModel)& csvModel)
    {
        // TODO: use more reliable detection (string comparison does not work with tr())
        DFG_STATIC_ASSERT(TargetType_last == 2, "This implementation handles only two target types");
        const auto& sTarget = csvModel.data(csvModel.index(0, 1)).toString();
        if (sTarget == "Selection")
            return TargetTypeSelection;
        else if (sTarget == "Whole table")
            return TargetTypeWholeTable;
        else
        {
            DFG_ASSERT_IMPLEMENTED(false);
            return TargetTypeUnknown;
        }
    }

} // unnamed namespace

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
static void generateForEachInTarget(const TargetType targetType, const CsvTableView& view, CsvItemModel& rModel, Generator_T generator)
{
    DFG_STATIC_ASSERT(TargetType_last == 2, "This implementation handles only two target types");

    const auto preEditSelectionRanges = view.storeSelection();

    if (targetType == TargetTypeWholeTable)
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
                for (int r = 0; r < nRows; ++r, ++nCounter)
                {
                    generator(table, r, c, nCounter);
                }
            }
        });
    }
    else if (targetType == TargetTypeSelection)
    {
        size_t nCounter = 0;
        rModel.batchEditNoUndo([&](CsvItemModel::DataTable& table)
        {
            view.forEachCsvModelIndexInSelection([&](const QModelIndex& index, bool& bContinue)
            {
                DFG_UNUSED(bContinue);
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
    const char gszFloatTypes[] = "gGeEfaA";

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

    std::string handlePrecisionParameters(const CsvItemModel& settingsModel)
    {
        const auto sFormatType = settingsModel.data(settingsModel.index(LastNonParamPropertyId + 2, 1)).toString().trimmed();
        auto sPrecision = settingsModel.data(settingsModel.index(LastNonParamPropertyId + 3, 1)).toString();
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
            QMessageBox::information(nullptr, tr("Invalid parameter"), tr("Format type parameter is not accepted. Note: only a subset of printf-valid items can be used."));
            return std::string();
        }
        if (!sPrecision.isEmpty())
            sPrecision.prepend('.');
        return std::string(("%" + sPrecision + sFormatType).toLatin1());
    }
} // unnamed namespace

namespace
{
    typedef decltype(QString().split(' ')) RandQStringArgs;

    enum class GeneratorFormatType
    {
        number,
        dateFromSecondsLocal,
        dateFromMilliSecondsLocal,
        dateFromSecondsUtc,
        dateFromMilliSecondsUtc
    };

    template <class T>
    qint64 valToInt64(T val)
    {
        return val;
    }

    qint64 valToDateInt64(double val)
    {
        qint64 intVal;
        return (::DFG_MODULE_NS(math)::isFloatConvertibleTo(val, &intVal)) ? intVal : -1;
    }

    template <class T, size_t N>
    void setTableElement(const ::DFG_MODULE_NS(qt)::CsvTableView& rView, CsvItemModel::DataTable& table, const int r, const int c, const T val, char(&buffer)[N], const char *pFormat, const GeneratorFormatType type)
    {
        if (type == GeneratorFormatType::number)
        {
            if (pFormat)
                ::DFG_MODULE_NS(str)::toStr(val, buffer, pFormat);
            else
                ::DFG_MODULE_NS(str)::toStr(val, buffer);
            table.setElement(r, c, DFG_ROOT_NS::SzPtrUtf8R(buffer));
            buffer[0] = '\0';
        }
        else
        {
            const auto i64 = valToDateInt64(val);
            if (i64 < 0 || !pFormat)
            {
                table.setElement(r, c, DFG_UTF8(""));
                return;
            }
            QString sResult;
            if (type == GeneratorFormatType::dateFromSecondsLocal)
                sResult = rView.dateTimeToString(QDateTime::fromSecsSinceEpoch(i64, Qt::LocalTime), pFormat);
            else if (type == GeneratorFormatType::dateFromMilliSecondsLocal)
                sResult = rView.dateTimeToString(QDateTime::fromMSecsSinceEpoch(i64, Qt::LocalTime), pFormat);
            else if (type == GeneratorFormatType::dateFromSecondsUtc)
                sResult = rView.dateTimeToString(QDateTime::fromSecsSinceEpoch(i64, Qt::UTC), pFormat);
            else if (type == GeneratorFormatType::dateFromMilliSecondsUtc)
                sResult = rView.dateTimeToString(QDateTime::fromMSecsSinceEpoch(i64, Qt::UTC), pFormat);
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
    bool generateRandom(const TargetType target, CsvTableView& view, const RandQStringArgs& qstrArgs, const char* pFormat = nullptr)
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

    bool generateRandomInt(const TargetType target, CsvTableView& view, RandQStringArgs& params)
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

    bool generateRandomReal(const TargetType target, DFG_CLASS_NAME(CsvTableView)& view, RandQStringArgs& params, const char* pszFormat)
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
}

namespace
{
    double cellValueAsDouble(CsvItemModel::DataTable* pTable, const double rowDouble, const double colDouble)
    {
        using namespace ::DFG_MODULE_NS(math);
        if (!pTable)
            return std::numeric_limits<double>::quiet_NaN();
        int r, c;
        if (!isFloatConvertibleTo<int>(rowDouble, &r) || !isFloatConvertibleTo(colDouble, &c))
            return std::numeric_limits<double>::quiet_NaN();
        r = ::DFG_MODULE_NS(qt)::CsvItemModel::visibleRowIndexToInternal(r);
        c = ::DFG_MODULE_NS(qt)::CsvItemModel::visibleRowIndexToInternal(c);
        return ::DFG_MODULE_NS(str)::strTo<double>((*pTable)(r, c));
    }
};

bool CsvTableView::generateContentImpl(const CsvItemModel& settingsModel)
{
    const auto genType = generatorType(settingsModel);
    const auto target = targetType(settingsModel);
    auto pModel = csvModel();
    if (!pModel)
        return false;
    auto& rModel = *pModel;

    DFG_STATIC_ASSERT(GeneratorType_last == 4, "This implementation handles only 4 generator types");
    if (genType == GeneratorTypeRandomIntegers)
    {
        if (settingsModel.rowCount() < LastNonParamPropertyId + 2) // Not enough parameters in model?
        {
            DFG_ASSERT(false);
            return false;
        }
        auto params = settingsModel.data(settingsModel.index(LastNonParamPropertyId + 1, 1)).toString().split(',');
        return generateRandomInt(target, *this, params);
    }
    else if (genType == GeneratorTypeRandomDoubles)
    {
        if (settingsModel.rowCount() < LastNonParamPropertyId + 4) // Not enough parameters in model?
        {
            DFG_ASSERT(false);
            return false;
        }
        auto params = settingsModel.data(settingsModel.index(LastNonParamPropertyId + 1, 1)).toString().split(',');
        const std::string sFormat = handlePrecisionParameters(settingsModel);
        if (sFormat.empty())
            return false;
        const auto pszFormat = sFormat.c_str();
        return generateRandomReal(target, *this, params, pszFormat);
    }
    else if (genType == GeneratorTypeFill)
    {
        if (settingsModel.rowCount() < LastNonParamPropertyId + 2) // Not enough parameters in model?
        {
            DFG_ASSERT(false);
            return false;
        }
        const auto sFill = settingsModel.data(settingsModel.index(LastNonParamPropertyId + 1, 1)).toString().toUtf8();
        const auto pszFillU8 = SzPtrUtf8(sFill.data());
        const auto generator = [&](CsvItemModel::DataTable& table, int r, int c, size_t)
        {
            table.setElement(r, c, pszFillU8);
        };
        generateForEachInTarget(target, *this, rModel, generator);
        return true;
    }
    else if (genType == GeneratorTypeFormula)
    {
        if (settingsModel.rowCount() < LastNonParamPropertyId + 4) // Not enough parameters in model?
        {
            DFG_ASSERT(false);
            return false;
        }
        const std::string sFormat = handlePrecisionParameters(settingsModel);
        if (sFormat.empty())
            return false;

        auto pszFormat = sFormat.c_str();
        const auto sFormula = settingsModel.data(settingsModel.index(LastNonParamPropertyId + 1, 1)).toString().toUtf8();
        ::DFG_MODULE_NS(math)::FormulaParser parser;
        DFG_VERIFY(parser.defineRandomFunctions());
        parser.setFormula(sFormula.data());
        char buffer[32] = "";
        double tr = std::numeric_limits<double>::quiet_NaN();
        double tc = std::numeric_limits<double>::quiet_NaN();
        const double rowCount = rModel.rowCount();
        const double colCount = rModel.columnCount();
        CsvItemModel::DataTable* pTable = nullptr;
        parser.defineFunctor("cellValue", [&](double r, double c) { return cellValueAsDouble(pTable, r, c); }, false);
        parser.defineVariable("trow", &tr);
        parser.defineVariable("tcol", &tc);
        parser.defineConstant("rowcount", rowCount);
        parser.defineConstant("colcount", colCount);
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

bool DFG_CLASS_NAME(CsvTableView)::moveFirstRowToHeader()
{
    auto pModel = model();
    return (pModel && pModel->rowCount() > 0 && pModel->columnCount() > 0) ? executeAction<DFG_CLASS_NAME(CsvTableViewActionMoveFirstRowToHeader)>(this) : false;
}

bool DFG_CLASS_NAME(CsvTableView)::moveHeaderToFirstRow()
{
    auto pModel = model();
    return (pModel && pModel->columnCount() > 0) ? executeAction<DFG_CLASS_NAME(CsvTableViewActionMoveFirstRowToHeader)>(this, true) : false;
}

QAbstractProxyModel* DFG_CLASS_NAME(CsvTableView)::getProxyModelPtr()
{
    return qobject_cast<QAbstractProxyModel*>(model());
}

const QAbstractProxyModel* DFG_CLASS_NAME(CsvTableView)::getProxyModelPtr() const
{
    return qobject_cast<const QAbstractProxyModel*>(model());
}

bool DFG_CLASS_NAME(CsvTableView)::diffWithUnmodified()
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
        const auto manuallyLocatedDiffer = QFileDialog::getOpenFileName(this, tr("Locate diff viewer"));
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

bool DFG_MODULE_NS(qt)::CsvTableView::getAllowApplicationSettingsUsage() const
{
    return property(gPropertyIdAllowAppSettingsUsage).toBool();
}

void CsvTableView::setAllowApplicationSettingsUsage(const bool b)
{
    setProperty(gPropertyIdAllowAppSettingsUsage, b);
    setReadOnlyModeFromProperty(getCsvTableViewProperty<CsvTableViewPropertyId_editMode>(this));
    Q_EMIT sigOnAllowApplicationSettingsUsageChanged(b);
}

void DFG_CLASS_NAME(CsvTableView)::finishEdits()
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

int DFG_CLASS_NAME(CsvTableView)::getFindColumnIndex() const
{
    return m_nFindColumnIndex;
}

void DFG_CLASS_NAME(CsvTableView)::onFilterRequested()
{
    const QMetaMethod findActivatedSignal = QMetaMethod::fromSignal(&ThisClass::sigFilterActivated);
    if (isSignalConnected(findActivatedSignal))
        Q_EMIT sigFilterActivated();
    else
        QToolTip::showText(QCursor::pos(), tr("Sorry, standalone filter is not implemented."));
}

void ::DFG_MODULE_NS(qt)::CsvTableView::onGoToCellTriggered()
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

    pLayout->addRow(addOkCancelButtonBoxToDialog(&dlg, pLayout));

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

void DFG_CLASS_NAME(CsvTableView)::onFindRequested()
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

void DFG_CLASS_NAME(CsvTableView)::onFind(const bool forward)
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

void DFG_CLASS_NAME(CsvTableView)::onFindNext()
{
    onFind(true);
}

void DFG_CLASS_NAME(CsvTableView)::onFindPrevious()
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
void DFG_CLASS_NAME(CsvTableView)::forEachCompleterEnabledColumnIndex(Func_T func)
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

void ::DFG_MODULE_NS(qt)::CsvTableView::onNewSourceOpened()
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

DFG_OPAQUE_PTR_DEFINE(DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzerPanel)
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
            {
                const auto sDescription = rCollector.getProperty(SelectionDetailCollector::s_propertyName_description).toString();
                auto sType = rCollector.getProperty(SelectionDetailCollector::s_propertyName_type).toString();
                if (rCollector.isBuiltIn())
                    sType = tr("Built-in");
                auto sFormula = rCollector.getProperty(SelectionDetailCollector_formula::s_propertyName_formula).toString();
                if (!sFormula.isEmpty())
                    sFormula = tr("<li>Formula: %1</li>").arg(sFormula);
                const QString sToolTip = tr("<ul><li>Type: %1</li><li>Short name: %2</li>%3</ul>").arg(sType, rCollector.getUiName_short(), sFormula);
                pCheckBox->setToolTip(sToolTip);
            }

            // Adding context menu actions
            if (!rCollector.isBuiltIn())
            {
                const auto sId = rCollector.id().toString();
                QPointer<CsvTableViewBasicSelectionAnalyzerPanel> spPanel = &rPanel;

                const auto addAction = [&](const QString& s, std::function<void ()> slotHandler)
                {
                    auto pAct = new QAction(s, &rMenu); // Deletion through parentship
                    DFG_QT_VERIFY_CONNECT(connect(pAct, &QAction::triggered, pCheckBox, slotHandler));
                    pCheckBox->addAction(pAct);
                };

                // Delete action
                addAction(tr("Delete"), [=]() { if (spPanel) spPanel->deleteDetail(sId); });
                // Export to clipboard
                addAction(tr("Export definition to clipboard"), [=]()
                    { 
                        auto pClipboard = QApplication::clipboard();
                        if (pClipboard)
                            pClipboard->setText(pCollector->exportDefinitionToJson());
                    });
                pCheckBox->setContextMenuPolicy(Qt::ActionsContextMenu);
            }
        }
        rMenu.addAction(rCollector.getCheckBoxAction());
    }

    static void removeDetailMenuEntry(SelectionDetailCollector& rCollector, QMenu& rMenu)
    {
        rMenu.removeAction(rCollector.getCheckBoxAction());
    }
}; // CsvTableViewBasicSelectionAnalyzerPanel opaque class 

::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzerPanel::CsvTableViewBasicSelectionAnalyzerPanel(QWidget *pParent) :
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

        // Enable/disable all items
        {
            {
                auto pAct = pMenu->addAction(tr("Enable all"));
                DFG_QT_VERIFY_CONNECT(connect(pAct, &QAction::triggered, this, &CsvTableViewBasicSelectionAnalyzerPanel::onEnableAllDetails));
            }
            {
                auto pAct = pMenu->addAction(tr("Disable all"));
                DFG_QT_VERIFY_CONNECT(connect(pAct, &QAction::triggered, this, &CsvTableViewBasicSelectionAnalyzerPanel::onDisableAllDetails));
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

::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzerPanel::~CsvTableViewBasicSelectionAnalyzerPanel() = default;

void ::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzerPanel::setValueDisplayString(const QString& s)
{
    Q_EMIT sigSetValueDisplayString(s);
}

void ::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzerPanel::setValueDisplayString_myThread(const QString& s)
{
    if (m_spValueDisplay)
        m_spValueDisplay->setText(s);
}

void ::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzerPanel::onEvaluationStarting(const bool bEnabled)
{
    Q_EMIT sigEvaluationStartingHandleRequest(bEnabled);
}

void ::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzerPanel::onEvaluationStarting_myThread(const bool bEnabled)
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

void ::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzerPanel::onEvaluationEnded(const double timeInSeconds, const DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)::CompletionStatus completionStatus)
{
    Q_EMIT sigEvaluationEndedHandleRequest(timeInSeconds, static_cast<int>(completionStatus));
}

void ::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzerPanel::onEvaluationEnded_myThread(const double timeInSeconds, const int completionStatus)
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

void ::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzerPanel::onAddCustomCollector()
{
    const auto sLabel = tr("Adding new selection detail. Description of fields:"
                           "<ul>"
                           "<li><b>%1:</b> Formula used to calculate the value, predefined variables 'acc' and 'value' are available : 'acc' is current accumulant value and 'value' is current cell value</li>"
                           "<li><b>%2:</b> Initial value of the accumulant, for example when defining a sum accumulant, initial value is typically 0.</li>"
                           "<li><b>%3:</b> Short name version to show in UI.</li>"
                           "<li><b>%4:</b> Long name version to show in UI. if omitted, short name will be used</li>"
                           "<li><b>%5:</b> A more detailed description of the new detail. Can be omitted</li>"
                           "</ul>"
                           R"(Example: { "%1": "acc + value^2", "%2": "0", "%5": "Calculates sum of squares" } )")
                           .arg(SelectionDetailCollector_formula::s_propertyName_formula,
                                SelectionDetailCollector_formula::s_propertyName_initialValue,
                                SelectionDetailCollector::s_propertyName_uiNameShort,
                                SelectionDetailCollector::s_propertyName_uiNameLong,
                                SelectionDetailCollector::s_propertyName_description);

    QString sJson = tr("{\n  \"%1\": \"\",\n  \"%2\": \"\",\n  \"%3\": \"\",\n  \"%4\": \"\",\n  \"%5\": \"\"\n}")
                    .arg(SelectionDetailCollector_formula::s_propertyName_formula,
                         SelectionDetailCollector_formula::s_propertyName_initialValue,
                         SelectionDetailCollector::s_propertyName_uiNameShort,
                         SelectionDetailCollector::s_propertyName_uiNameLong,
                         SelectionDetailCollector::s_propertyName_description
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
    inputs[SelectionDetailCollector::s_propertyName_type] = QString("accumulator");

    if (!addDetail(inputs))
    {
        QMessageBox::warning(this, tr("Adding failed"), tr("Adding new detail failed"));
    }
}

void ::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzerPanel::setEnableStatusForAll(const bool b)
{
    if (!DFG_OPAQUE_REF().m_spCollectors)
        return;

    auto& collectors = *DFG_OPAQUE_REF().m_spCollectors;
    for (auto& spCollector : collectors)
    {
        if (spCollector)
            spCollector->enable(b);
    }
}

void ::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzerPanel::onEnableAllDetails()
{
    setEnableStatusForAll(true);
}

void ::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzerPanel::onDisableAllDetails()
{
    setEnableStatusForAll(false);
}

double ::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzerPanel::getMaxTimeInSeconds() const
{
    bool bOk = false;
    double val = 0;
    if (m_spTimeLimitDisplay)
        val = m_spTimeLimitDisplay->text().toDouble(&bOk);
    return (bOk) ? val : -1.0;
}

bool ::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzerPanel::isStopRequested() const
{
    return (m_spStopButton && m_spStopButton->isChecked());
}

void ::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzerPanel::clearAllDetails()
{
    auto pCollectors = DFG_OPAQUE_REF().m_spCollectors.get();
    if (!pCollectors)
        return;
    for (auto& spItem : *pCollectors)
    {
        if (!spItem)
            continue;
        spItem->enable(false);
    }
}

bool ::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzerPanel::addDetail(const QVariantMap& items)
{
    // Expected fields
    //      id             : identifier of the detail, must be unique.
    //      [type]         : Type of collector, either empty for built-in ones or "accumulator".
    //      [initial_value]: Initial value for collector if it needs one
    //                          -Needed by: accumulator
    //      [formula]      : Formula used to compute values if collector needs one
    //                          -Needed by: accumulator. Formula has variables "acc" (=accumulator value) and "value" (=value of new cell):
    //      [ui_name_short]: Short UI name for the detail
    //      [ui_name_long] : Long UI name for the detail
    //      [description]  : Longer description of the detail, shown as tooltip.
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
            std::generate(s.begin(), s.end(), distrEng);
            return s;
        }();
    }
    const auto id = qStringToStringUtf8(idQString);
    const bool bEnabled = items.value("enabled", true).toBool();
    using Detail = ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::BasicSelectionDetailCollector::BuiltInDetail;
    const auto sType = items.value("type").toString();
    if (sType.isEmpty())
    {
        const auto detail = ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::BasicSelectionDetailCollector::selectionDetailNameToId(id);
        if (detail == Detail::detailCount)
            return false;

        auto pExisting = collectors.find(id);
        if (!pExisting)
            return false; // Built-in details should already be created.
        pExisting->enable(bEnabled);
    }
    else if (sType == QLatin1String("accumulator"))
    {
        auto pExisting = collectors.find(id);
        if (pExisting)
        {
            // If exists, for now just updating enable-flag.
            pExisting->enable(bEnabled);
            return true;
        }

        const auto sInitialValueQString = items.value(SelectionDetailCollector_formula::s_propertyName_initialValue).toString();
        const auto sInitialValue = qStringToStringUtf8(sInitialValueQString);
        bool bOk = false;
        const auto initialValue = ::DFG_MODULE_NS(str)::strTo<double>(sInitialValue.rawStorage(), &bOk);
        if (!bOk)
            return false;

        const auto sFormulaQString = items.value(SelectionDetailCollector_formula::s_propertyName_formula).toString();
        const auto sFormula = qStringToStringUtf8(sFormulaQString);
        const auto sDescription = items.value(SelectionDetailCollector::s_propertyName_description).toString();
        const auto sUiNameShort = items.value(SelectionDetailCollector::s_propertyName_uiNameShort).toString();
        const auto sUiNameLong = items.value(SelectionDetailCollector::s_propertyName_uiNameLong).toString();

        auto spNewCollector = std::make_shared<SelectionDetailCollector_formula>(id, sFormula, initialValue);
        spNewCollector->setProperty(SelectionDetailCollector::s_propertyName_description, sDescription);
        spNewCollector->setProperty(SelectionDetailCollector_formula::s_propertyName_formula, sFormulaQString);
        spNewCollector->setProperty(SelectionDetailCollector_formula::s_propertyName_initialValue, sInitialValueQString);
        spNewCollector->setProperty(SelectionDetailCollector::s_propertyName_type, sType);
        spNewCollector->setProperty(SelectionDetailCollector::s_propertyName_uiNameShort, sUiNameShort);
        spNewCollector->setProperty(SelectionDetailCollector::s_propertyName_uiNameLong, sUiNameLong);

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

bool ::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzerPanel::deleteDetail(const StringViewUtf8& id)
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

void ::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzerPanel::setDefaultDetails()
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

auto ::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzerPanel::collectors() const -> CollectorContainerPtr
{
    auto pOpaq = DFG_OPAQUE_PTR();
    if (!pOpaq)
        return nullptr;
    return std::atomic_load(&pOpaq->m_spCollectors);
}

auto ::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzerPanel::detailConfigsToString() const -> QString
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

void DFG_CLASS_NAME(CsvTableView)::onSelectionModelChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    onSelectionModelOrContentChanged(selected, deselected, QItemSelection());
}

void DFG_CLASS_NAME(CsvTableView)::onViewModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    DFG_UNUSED(roles);
    // For now not checking if change actually happened within selection, to be improved later if there are practical cases where content gets changed outside the selection.
    onSelectionModelOrContentChanged(QItemSelection(), QItemSelection(), QItemSelection(topLeft, bottomRight));
}

void DFG_CLASS_NAME(CsvTableView)::onSelectionModelOrContentChanged(const QItemSelection& selected, const QItemSelection& deselected, const QItemSelection& editedViewModelItems)
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

void DFG_CLASS_NAME(CsvTableView)::addSelectionAnalyzer(std::shared_ptr<SelectionAnalyzer> spAnalyzer)
{
    if (!spAnalyzer)
        return;
    spAnalyzer->m_spView = this;
    if (m_selectionAnalyzers.end() == std::find(m_selectionAnalyzers.begin(), m_selectionAnalyzers.end(), spAnalyzer)) // If not already present?
        m_selectionAnalyzers.push_back(std::move(spAnalyzer));
}

void DFG_CLASS_NAME(CsvTableView)::removeSelectionAnalyzer(const SelectionAnalyzer* pAnalyzer)
{
    auto iter = std::find_if(m_selectionAnalyzers.begin(), m_selectionAnalyzers.end(), [=](const std::shared_ptr<SelectionAnalyzer>& sp)
    {
        return (sp.get() == pAnalyzer);
    });
    if (iter != m_selectionAnalyzers.end())
        m_selectionAnalyzers.erase(iter);
}

QModelIndex DFG_CLASS_NAME(CsvTableView)::mapToViewModel(const QModelIndex& index) const
{
    const auto pIndexModel = index.model();
    if (pIndexModel == model())
        return index;
    else if (pIndexModel == csvModel() && getProxyModelPtr())
        return getProxyModelPtr()->mapFromSource(index);
    else
        return QModelIndex();
}

QModelIndex DFG_CLASS_NAME(CsvTableView)::mapToDataModel(const QModelIndex& index) const
{
    const auto pIndexModel = index.model();
    if (pIndexModel == csvModel())
        return index;
    else if (pIndexModel == model() && getProxyModelPtr())
        return getProxyModelPtr()->mapToSource(index);
    else
        return QModelIndex();
}

void DFG_CLASS_NAME(CsvTableView)::forgetLatestFindPosition()
{
    m_latestFoundIndex = QModelIndex();
}

void DFG_CLASS_NAME(CsvTableView)::onColumnResizeAction_toViewEvenly()
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

void DFG_CLASS_NAME(CsvTableView)::onColumnResizeAction_toViewContentAware()
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

DFG_ROOT_NS_BEGIN
{
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
            const auto nNewSize = QInputDialog::getInt(pParent,
                                                       QApplication::tr("Header size"),
                                                       QApplication::tr("New header size"),
                                                       nCurrentSetting,
                                                       1,
                                                       1048575, // Maximum section size at least in Qt 5.2
                                                       1,
                                                       &bOk);
            if (bOk && nNewSize > 0)
            {
                pHeader->setDefaultSectionSize(nNewSize);
            }
        }
    }
} // namespace dfg

void DFG_CLASS_NAME(CsvTableView)::onColumnResizeAction_content()
{
    onHeaderResizeAction_content(horizontalHeader());
}

void DFG_CLASS_NAME(CsvTableView)::onRowResizeAction_content()
{
    onHeaderResizeAction_content(verticalHeader());
}

void DFG_CLASS_NAME(CsvTableView)::onColumnResizeAction_fixedSize()
{
    onHeaderResizeAction_fixedSize(this, horizontalHeader());
}

void DFG_CLASS_NAME(CsvTableView)::onRowResizeAction_fixedSize()
{
    onHeaderResizeAction_fixedSize(this, verticalHeader());
}

QModelIndex DFG_CLASS_NAME(CsvTableView)::mapToSource(const QAbstractItemModel* pModel, const QAbstractProxyModel* pProxy, const int r, const int c)
{
    if (pProxy)
        return pProxy->mapToSource(pProxy->index(r, c));
    else if (pModel)
        return pModel->index(r, c);
    else
        return QModelIndex();
}

QItemSelection DFG_CLASS_NAME(CsvTableView)::getSelection() const
{
    auto pSelectionModel = selectionModel();
    return (pSelectionModel) ? pSelectionModel->selection() : QItemSelection();
}

auto ::DFG_MODULE_NS(qt)::CsvTableView::privCreateActionBlockedDueToLockedContentMessage(const QString& actionname) -> QString
{
    if (isReadOnlyMode())
        return tr("Executing action '%1' was blocked: table is in read-only mode").arg(actionname);
    else
        return tr("Executing action '%1' was blocked: table is being accessed by some other operation. Please try again later.").arg(actionname);
}

void ::DFG_MODULE_NS(qt)::CsvTableView::showStatusInfoTip(const QString& sMsg)
{
    showInfoTip(sMsg, this);
}

void ::DFG_MODULE_NS(qt)::CsvTableView::privShowExecutionBlockedNotification(const QString& actionname)
{
    // Showing a note to user that execution was blocked. Not using QToolTip as it is a bit brittle in this use case:
    // it may disappear too quickly and disappear triggers are difficult to control in general.

    // This was the original tooltip version which worked ok with most notifications, but e.g. with
    // blocked edits in CsvTableViewDelegate was not shown at all.
    //QToolTip::showText(QCursor::pos(), privCreateActionBlockedDueToLockedContentMessage(actionname));

    showStatusInfoTip(privCreateActionBlockedDueToLockedContentMessage(actionname));
}

auto DFG_CLASS_NAME(CsvTableView)::tryLockForEdit() -> LockReleaser
{
    return (m_spEditLock && m_spEditLock->tryLockForWrite()) ? LockReleaser(m_spEditLock.get()) : LockReleaser();
}

auto DFG_CLASS_NAME(CsvTableView)::tryLockForRead() -> LockReleaser
{
    return (m_spEditLock && m_spEditLock->tryLockForRead()) ? LockReleaser(m_spEditLock.get()) : LockReleaser();
}

auto ::DFG_CLASS_NAME(CsvTableView)::horizontalTableHeader() -> TableHeaderView*
{
    return qobject_cast<TableHeaderView*>(horizontalHeader());
}

void ::DFG_CLASS_NAME(CsvTableView)::setColumnNames()
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
        columnNameModel.setData(columnNameModel.index(c, 0), pCsvModel->getHeaderName(c));

    // Creating a dialog that has CsvTableView and ok & cancel buttons.
    CsvTableView columnNameView(nullptr, this, ViewType::fixedDimensionEdit);
    columnNameView.setModel(&columnNameModel);
    spLayout->addWidget(&columnNameView);

    spLayout->addWidget(addOkCancelButtonBoxToDialog(&dlg, spLayout.get()));

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

void ::DFG_MODULE_NS(qt)::CsvTableView::addConfigSavePropertyFetcher(PropertyFetcher fetcher)
{
    DFG_OPAQUE_REF().m_propertyFetchers.push_back(std::move(fetcher));
}

auto ::DFG_MODULE_NS(qt)::CsvTableView::weekDayNames() const -> QStringList
{
    const auto s = getCsvModelOrViewProperty<CsvTableViewPropertyId_weekDayNames>(this);
    return s.split(',');
}

auto ::DFG_MODULE_NS(qt)::CsvTableView::dateTimeToString(const QDateTime& dateTime, const QString& sFormat) const -> QString
{
    QString rv = dateTime.toString(sFormat);
    ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::handleWeekDayFormat(rv, dateTime.date().dayOfWeek(), [&]() { return this-> weekDayNames(); });
    return rv;
}

auto ::DFG_MODULE_NS(qt)::CsvTableView::dateTimeToString(const QDate& date, const QString& sFormat) const -> QString
{
    QString rv = date.toString(sFormat);
    ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::handleWeekDayFormat(rv, date.dayOfWeek(), [&]() { return this-> weekDayNames(); });
    return rv;
}

auto ::DFG_MODULE_NS(qt)::CsvTableView::dateTimeToString(const QTime& qtime, const QString& sFormat) const -> QString
{
    return qtime.toString(sFormat);
}

/////////////////////////////////
// Start of dfg::qt namespace
DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt) {

namespace
{
    namespace ColumnPropertyId
    {
        const SzPtrUtf8R visible = DFG_UTF8("visible");
    } // namespace ColumnPropertyId

    template <class View_T>
    static auto getColInfo(View_T& rView, const int nCol) -> decltype(rView.csvModel()->getColInfo(nCol))
    {
        auto pCsvModel = rView.csvModel();
        return (pCsvModel) ? pCsvModel->getColInfo(nCol) : nullptr;
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

QVariant CsvTableView::getColumnPropertyByDataModelIndex(const int nCol, const StringViewUtf8& svKey, const QVariant defaultValue) const
{
    auto pColInfo = getColInfo(*this, nCol);
    return (pColInfo) ? getColumnProperty(*this, *pColInfo, svKey, defaultValue) : defaultValue;
}

void CsvTableView::setCaseSensitiveSorting(const bool bCaseSensitive)
{
    auto pProxy = qobject_cast<QSortFilterProxyModel*>(getProxyModelPtr());
    if (pProxy)
        pProxy->setSortCaseSensitivity((bCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive);
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
        showStatusInfoTip(tr("Unable to unhide column: there seems to be ongoing operations"));
        return;
    }

    auto pCsvModel = csvModel();
    if (!pCsvModel)
        return;

    auto pColInfo = pCsvModel->getColInfo(nCol);
    if (!pColInfo)
        return;

    if (pColInfo->setProperty(viewPropertyContextId(*this), ColumnPropertyId::visible, bVisible) && proxyInvalidation == ProxyModelInvalidation::ifNeeded)
        invalidateSortFilterProxyModel();
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

    pCsvModel->forEachColInfoWhile([&](CsvItemModel::ColInfo& colInfo)
    {
        if (!colInfo.getProperty(viewPropertyContextId(*this), ColumnPropertyId::visible, true).toBool())
            colInfo.setProperty(viewPropertyContextId(*this), ColumnPropertyId::visible, true);
        return true;
    });

    invalidateSortFilterProxyModel();
}

void CsvTableView::showSelectColumnVisibilityDialog()
{
    auto pCsvModel = this->csvModel();
    if (!pCsvModel)
        return;
    QStringList columns;
    std::vector<bool> visibilityFlags;
    pCsvModel->forEachColInfoWhile([&](const CsvItemModel::ColInfo& colInfo)
    {
        columns.push_back(tr("Column %1 ('%2')").arg(CsvItemModel::internalColumnIndexToVisible(colInfo.index())).arg(colInfo.name()));
        visibilityFlags.push_back(getColumnProperty(*this, colInfo, ColumnPropertyId::visible, true).toBool());
        return true;
    });
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

} } // namespace dfg::qt
/////////////////////////////////

DFG_OPAQUE_PTR_DEFINE(DFG_MODULE_NS(qt)::CsvTableViewDlg)
{
public:
    QObjectStorage<QDialog> m_spDialog;
    QObjectStorage<CsvTableView> m_spTableView;
};

::DFG_MODULE_NS(qt)::CsvTableViewDlg::CsvTableViewDlg(std::shared_ptr<QReadWriteLock> spReadWriteLock, QWidget* pParent, CsvTableView::ViewType viewType)
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

void ::DFG_MODULE_NS(qt)::CsvTableViewDlg::setModel(QAbstractItemModel* pModel)
{
    DFG_OPAQUE_REF().m_spTableView->setModel(pModel);
}

int ::DFG_MODULE_NS(qt)::CsvTableViewDlg::exec()
{
    DFG_OPAQUE_REF().m_spDialog->show(); // To get dimensions for column resize.
    csvView().onColumnResizeAction_toViewContentAware();
    return DFG_OPAQUE_REF().m_spDialog->exec();
}

void ::DFG_MODULE_NS(qt)::CsvTableViewDlg::resize(const int w, const int h)
{
    dialog().resize(w, h);
}

auto ::DFG_MODULE_NS(qt)::CsvTableViewDlg::dialog() -> QWidget&
{
    DFG_REQUIRE(DFG_OPAQUE_REF().m_spDialog != nullptr);
    return *DFG_OPAQUE_REF().m_spDialog;
}

auto ::DFG_MODULE_NS(qt)::CsvTableViewDlg::csvView() -> CsvTableView&
{
    DFG_REQUIRE(DFG_OPAQUE_REF().m_spTableView != nullptr);
    return *DFG_OPAQUE_REF().m_spTableView;
}

void ::DFG_MODULE_NS(qt)::CsvTableViewDlg::addVerticalLayoutWidget(int nPos, QWidget* pWidget)
{
    auto pLayout = qobject_cast<QVBoxLayout*>(dialog().layout());
    DFG_ASSERT_CORRECTNESS(pLayout != nullptr);
    if (!pLayout)
        return;
    pLayout->insertWidget(nPos, pWidget);
}

DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)::DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)()
    : m_abIsEnabled(true)
    , m_bPendingCheckQueue(false)
    , m_abNewSelectionPending(false)
{
    m_spMutexAnalyzeQueue.reset(new QMutex);
}

DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)::~DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)()
{

}

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)::addSelectionToQueueImpl(QItemSelection selection)
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

void DFG_MODULE_NS(qt)::DFG_CLASS_NAME(CsvTableViewSelectionAnalyzer)::onCheckAnalyzeQueue()
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



::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzer::CsvTableViewBasicSelectionAnalyzer(PanelT* uiPanel)
   : m_spUiPanel(uiPanel)
{
}

::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzer::~CsvTableViewBasicSelectionAnalyzer() = default;

void ::DFG_MODULE_NS(qt)::CsvTableViewBasicSelectionAnalyzer::analyzeImpl(const QItemSelection selection)
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
            sMessage = collector.resultString(selection);
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

/////////////////////////////////
// Start of dfg::qt namespace
DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt) {

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
    if (pCsvModel)
    {
        menu.addSeparator();

        // "Select column visibility"
        addViewAction(rView, menu, tr("Select column visibility..."), noShortCut, ActionFlags::viewEdit, false, &CsvTableView::showSelectColumnVisibilityDialog);

        const auto nMaxUnhideEntryCount = 10;
        auto nHiddenCount = 0;
        pCsvModel->forEachColInfoWhile([&](const CsvItemModel::ColInfo& colInfo)
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
        }
    }

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
// SelectionDetailCollector
//
/////////////////////////////////////////////////////////////////////////////////////

DFG_OPAQUE_PTR_DEFINE(SelectionDetailCollector)
{
public:
    StringUtf8 m_id;
    std::atomic<bool> m_abEnabled = true;
    QObjectStorage<QCheckBox> m_spCheckBox;
    QObjectStorage<QAction> m_spContextMenuAction;
    QVariantMap m_properties;
};

const char SelectionDetailCollector::s_propertyName_description[]  = "description";
const char SelectionDetailCollector::s_propertyName_id[]           = "id";
const char SelectionDetailCollector::s_propertyName_type[]         = "type";
const char SelectionDetailCollector::s_propertyName_uiNameShort[]  = "ui_name_short";
const char SelectionDetailCollector::s_propertyName_uiNameLong[]   = "ui_name_long";

SelectionDetailCollector::SelectionDetailCollector(StringUtf8 sId) { DFG_OPAQUE_REF().m_id = std::move(sId); }
SelectionDetailCollector::SelectionDetailCollector(const QString& sId) : SelectionDetailCollector(qStringToStringUtf8(sId)) {}
SelectionDetailCollector::~SelectionDetailCollector()
{
    auto pCheckBox = DFG_OPAQUE_REF().m_spCheckBox.release();
    if (pCheckBox)
        pCheckBox->deleteLater(); // Using deleteLater() as checkbox may be used when collector gets deleted.
    auto pContextMenuAction = DFG_OPAQUE_REF().m_spContextMenuAction.release();
    if (pContextMenuAction)
        pContextMenuAction->deleteLater();
}

auto SelectionDetailCollector::id() const -> StringViewUtf8
{
    auto pOpaq = DFG_OPAQUE_PTR();
    return (pOpaq) ? pOpaq->m_id : StringViewUtf8();
}

bool SelectionDetailCollector::isEnabled() const
{
    auto pOpaq = DFG_OPAQUE_PTR();
    return (pOpaq) ? pOpaq->m_abEnabled.load() : false;
}

void SelectionDetailCollector::enable(const bool b, const bool bUpdateCheckBox)
{
    DFG_OPAQUE_REF().m_abEnabled = b;
    if (bUpdateCheckBox && DFG_OPAQUE_REF().m_spCheckBox)
        DFG_OPAQUE_REF().m_spCheckBox->setChecked(b);
}

void SelectionDetailCollector::setProperty(const QString& sKey, const QVariant value)
{
    DFG_OPAQUE_REF().m_properties[sKey] = value;
}

QVariant SelectionDetailCollector::getProperty(const QString& sKey, const QVariant defaultValue) const
{
    auto pOpaq = DFG_OPAQUE_PTR();
    return (pOpaq) ? pOpaq->m_properties.value(sKey, defaultValue) : defaultValue;
}

QString SelectionDetailCollector::getUiName_long() const
{
    auto s = getProperty("ui_name_long").toString();
    if (s.isEmpty())
        s = getUiName_short();
    if (s.isEmpty())
        s = viewToQString(id());
    return s;
}

QString SelectionDetailCollector::getUiName_short() const
{
    auto s = getProperty("ui_name_short").toString();
    if (s.isEmpty())
        s = viewToQString(id());
    return s;
}

QCheckBox* SelectionDetailCollector::createCheckBox(QMenu* pParent)
{
    DFG_OPAQUE_REF().m_spCheckBox.reset(new QCheckBox(pParent));
    return DFG_OPAQUE_REF().m_spCheckBox.get();
}

QCheckBox* SelectionDetailCollector::getCheckBoxPtr()
{
    return DFG_OPAQUE_REF().m_spCheckBox.get();
}

QAction* SelectionDetailCollector::getCheckBoxAction()
{
    if (!DFG_OPAQUE_REF().m_spContextMenuAction)
    {
        auto pAct = new QWidgetAction(nullptr);
        pAct->setDefaultWidget(this->getCheckBoxPtr());
        DFG_OPAQUE_REF().m_spContextMenuAction.reset(pAct);
    }
    return DFG_OPAQUE_REF().m_spContextMenuAction.get();
}

bool SelectionDetailCollector::isBuiltIn() const
{
    return getProperty("type").toString().isEmpty();
}

QString SelectionDetailCollector::exportDefinitionToJson(const bool bIncludeId, const bool bSingleLine) const
{
    auto pOpaq = DFG_OPAQUE_PTR();
    if (!pOpaq)
        return QString();
    const QJsonDocument::JsonFormat format = (bSingleLine) ? QJsonDocument::Compact : QJsonDocument::Indented;
    if (isBuiltIn())
        return QString(R"({ "id":"%1"%2 })").arg(viewToQString(id()), isEnabled() ? "" : R"(", "enabled":"0")");
    else
    {
        auto m = pOpaq->m_properties;
        if (bIncludeId)
            m[s_propertyName_id] = viewToQString(id());
        return QString::fromUtf8(QJsonDocument::fromVariant(m).toJson(format));
    }
}

/////////////////////////////////////////////////////////////////////////////////////
// 
// SelectionDetailCollector_formula
//
/////////////////////////////////////////////////////////////////////////////////////

DFG_OPAQUE_PTR_DEFINE(SelectionDetailCollector_formula)
{
public:
    FormulaParser m_formulaParser;
    double m_initialValue;
    double m_accValue;
    double m_cellValue = std::numeric_limits<double>::quiet_NaN();
}; // Opaque class of SelectionDetailCollector_formula

const char SelectionDetailCollector_formula::s_propertyName_formula[]      = "formula";
const char SelectionDetailCollector_formula::s_propertyName_initialValue[] = "initial_value";

SelectionDetailCollector_formula::SelectionDetailCollector_formula(StringUtf8 sId, StringViewUtf8 svFormula, double initialValue)
    : BaseClass(std::move(sId))
{
    DFG_OPAQUE_REF().m_initialValue = initialValue;
    DFG_OPAQUE_REF().m_accValue = initialValue;
    this->m_bNeedsUpdate = true;
    auto& rFormulaParser = DFG_OPAQUE_REF().m_formulaParser;
    rFormulaParser.defineVariable("acc", &DFG_OPAQUE_REF().m_accValue);
    rFormulaParser.defineVariable("value", &DFG_OPAQUE_REF().m_cellValue);
    rFormulaParser.setFormula(svFormula);
}

SelectionDetailCollector_formula::~SelectionDetailCollector_formula() = default;

double SelectionDetailCollector_formula::valueImpl() const
{
    auto pOpaq = DFG_OPAQUE_PTR();
    return (pOpaq) ? pOpaq->m_accValue : std::numeric_limits<double>::quiet_NaN();
}

void SelectionDetailCollector_formula::updateImpl(const double val)
{
    DFG_OPAQUE_REF().m_cellValue = val;
    DFG_OPAQUE_REF().m_accValue = DFG_OPAQUE_REF().m_formulaParser.evaluateFormulaAsDouble();
}

void SelectionDetailCollector_formula::resetImpl()
{
    DFG_OPAQUE_REF().m_accValue = DFG_OPAQUE_REF().m_initialValue;
    DFG_OPAQUE_REF().m_cellValue = std::numeric_limits<double>::quiet_NaN();
}

/////////////////////////////////////////////////////////////////////////////////////
// 
// SelectionDetailCollectorContainer
//
/////////////////////////////////////////////////////////////////////////////////////

auto SelectionDetailCollectorContainer::find(const StringViewUtf8& id) -> SelectionDetailCollector*
{
    auto iter = findIter(id);
    return (iter != this->end()) ? iter->get() : nullptr;
}

auto SelectionDetailCollectorContainer::find(const QString& id) -> SelectionDetailCollector*
{
    return find(StringViewUtf8(SzPtrUtf8(id.toUtf8())));
}

auto SelectionDetailCollectorContainer::findIter(const StringViewUtf8& id) -> Container::iterator
{
    return std::find_if(this->begin(), this->end(), [&](const std::shared_ptr<SelectionDetailCollector>& sp)
    {
        return sp && sp->id() == id;
    });
}

bool SelectionDetailCollectorContainer::erase(const StringViewUtf8& id)
{
    auto iter = findIter(id);
    if (iter == this->end())
        return false;
    BaseClass::erase(iter);
    return true;
}

} } // namespace dfg::qt
/////////////////////////////////

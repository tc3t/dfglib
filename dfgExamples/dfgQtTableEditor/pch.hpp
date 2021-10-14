#pragma once

#include <dfg/dfgDefs.hpp>

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
    #include <boost/version.hpp>
    #if (BOOST_VERSION >= 107000) // Histogram is available since 1.70 (2019-04-12)
        #include <boost/histogram.hpp>
    #endif

    #include <boost/accumulators/framework/accumulator_set.hpp>
    #include <boost/accumulators/statistics/variance.hpp>
    #include <boost/accumulators/statistics/stats.hpp>

    #include <QAction>
    #include <QCoreApplication>
    #include <QDateTime>
    #include <QFile>
    #include <QFileInfo>
    #include <QDialog>
    #include <QGridLayout>
    #include <QMetaMethod>
    #include <QMutex>
    #include <QMutexLocker>
    #include <QReadWriteLock>
    #include <QSortFilterProxyModel>
    #include <QSpinBox>
    #include <QSqlError>
    #include <QSqlQuery>
    #include <QSqlRecord>
    #include <QThread>
    #include <QUndoStack>

    #if defined(DFGQTE_USING_QCUSTOMPLOT) && DFGQTE_USING_QCUSTOMPLOT == 1
        #include <dfg/qt/qcustomplot/qcustomplot.h>
    #endif

DFG_END_INCLUDE_WITH_DISABLED_WARNINGS

#include <dfg/alg.hpp>
#include <dfg/cont/IntervalSet.hpp>
#include <dfg/cont/IntervalSetSerialization.hpp>
#include <dfg/cont/MapVector.hpp>
#include <dfg/cont/SetVector.hpp>
#include <dfg/cont/SortedSequence.hpp>
#include <dfg/cont/table.hpp>
#include <dfg/cont/tableCsv.hpp>
#include <dfg/dfgBase.hpp>
#include <dfg/io.hpp>
#include <dfg/io/DelimitedTextReader.hpp>
#include <dfg/io/OmcByteStream.hpp>
#include <dfg/math.hpp>
#include <dfg/math/FormulaParser.hpp>
#include <dfg/qt/containerUtils.hpp>
#include <dfg/qt/CsvTableView.hpp>
#include <dfg/qt/CsvItemModel.hpp>
#include <dfg/qt/CsvTableViewActions.hpp>
#include <dfg/qt/qtBasic.hpp>
#include <dfg/qt/sqlTools.hpp>
#include <dfg/rand/distributionHelpers.hpp>
#include <dfg/str/string.hpp>
#include <dfg/str/strTo.hpp>

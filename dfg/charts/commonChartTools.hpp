#pragma once

#include "../dfgDefs.hpp"
#include <functional>
#include <memory>
#include <utility>

#include "../rangeIterator.hpp"
#include "../ReadOnlySzParam.hpp"
#include "../str/string.hpp"
#include "../str/strTo.hpp"

/*
Terminology used in dfglib:
    -'chart': general term referring to the whole graphical representation
        -a chart can include multiple elements such as function curves (e.g. curve of f(x) = 2*x in (x,y) coordinate system), legend, axis etc.
    -'graph': tentatively a graphical representation of a single data set (e.g. graph of (x,y) data); note, though, that in some places a synonym for 'chart' (e.g. qt/graphTools)
    -Classes:
        -ChartObject: An abstract class for various chart items
            -XySeries: (x,y) data with ordered x-values.
            -Histogram:
        -ChartObjectHolder: Container for (shared) ChartObject
        -ChartCanvas: Implements a 'chart' as specified above.
            -Offers functionality e.g. for adding ChartObjects and enabling/disabling legend.
*/


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(charts) {

/* Checklist for doing changes:
    -Reflect all changes done here to documentation in qt/graphTools.cpp getGuideString() (remember also examples).
    -if adding new types or properties for existing types, update forEachUnrecognizedPropertyId()
*/

constexpr char ChartObjectFieldIdStr_enabled[] = "enabled";
constexpr char ChartObjectFieldIdStr_type[] = "type";
    constexpr char ChartObjectChartTypeStr_xy[] = "xy";
        // xy-type has properties: line_style, point_style, x_source, y_source, x_rows, panel_id, line_colour

    constexpr char ChartObjectChartTypeStr_histogram[] = "histogram";
        // histogram-type has properties: bin_count, x_source, panel_id, line_colour

    constexpr char ChartObjectChartTypeStr_panelConfig[] = "panel_config";
        // panel_config-type has properties: panel_id, title, x_label, y_label

    constexpr char ChartObjectChartTypeStr_globalConfig[] = "global_config";
        // global_config-type has properties: show_legend, auto_axis_labels


// data_source: defines data source for ChartObject.
constexpr char ChartObjectFieldIdStr_dataSource[] = "data_source";

// name: this will show e.g. in legend.
constexpr char ChartObjectFieldIdStr_name[] = "name";

// bin_count
constexpr char ChartObjectFieldIdStr_binCount[] = "bin_count";

// title
constexpr char ChartObjectFieldIdStr_title[] = "title";

// show_legend
constexpr char ChartObjectFieldIdStr_showLegend[] = "show_legend";

// auto_axis_labels
constexpr char ChartObjectFieldIdStr_autoAxisLabels[] = "auto_axis_labels";

// x label
constexpr char ChartObjectFieldIdStr_xLabel[] = "x_label";

// y label
constexpr char ChartObjectFieldIdStr_yLabel[] = "y_label";

// x_rows, value is semicolon separated list defining an IntervalSet of row indexes. For example "1:4; 8; 12:13" means rows 1, 2, 3, 4, 8, 12, 13.
constexpr char ChartObjectFieldIdStr_xRows[] = "x_rows";

// x_source/y_source, value is parenthesis parametrisized value (e.g. x_source: column_name(header 1))
constexpr char ChartObjectFieldIdStr_xSource[] = "x_source";
constexpr char ChartObjectFieldIdStr_ySource[] = "y_source";
    constexpr char ChartObjectSourceTypeStr_columnName[]    = "column_name";
    constexpr char ChartObjectSourceTypeStr_rowIndex[]      = "row_index"; // Value is to be taken from row index of another column.

// panel_id: defines panel to which ChartObject is to be assigned to.
//      Possible values:
//          -grid(r, c): instructs to put chart object to position row = r, column = c on a panel grid. Indexing is 1-based. Value (1,1) means top left corner.
constexpr char ChartObjectFieldIdStr_panelId[] = "panel_id";

// Line style entries
constexpr char ChartObjectFieldIdStr_lineStyle[] = "line_style";
    constexpr char ChartObjectLineStyleStr_none[] = "none";
    constexpr char ChartObjectLineStyleStr_basic[] = "basic";
    constexpr char ChartObjectLineStyleStr_stepLeft[] = "step_left";
    constexpr char ChartObjectLineStyleStr_stepRight[] = "step_right";
    constexpr char ChartObjectLineStyleStr_stepMiddle[] = "step_middle";
    constexpr char ChartObjectLineStyleStr_pole[] = "pole";

// Point style entries
constexpr char ChartObjectFieldIdStr_pointStyle[] = "point_style";
constexpr char ChartObjectPointStyleStr_none[] = "none";
constexpr char ChartObjectPointStyleStr_basic[] = "basic";

// line_colour
constexpr char ChartObjectFieldIdStr_lineColour[] = "line_colour";

enum class ChartDataType
{
    unknown,
    dayTime,
    dayTimeMillisecond,
    dateOnly,
    dateAndTime,
    dateAndTimeMillisecond
};

// Abstract class representing an entry that defines what kind of ChartObjects to create on chart.
class AbstractChartControlItem
{
public:
    typedef StringUtf8 String;
    typedef StringViewUtf8 StringView;
    typedef StringViewC FieldIdStrView;
    typedef const StringViewC& FieldIdStrViewInputParam;

    virtual ~AbstractChartControlItem() {}

    // Returns value of field 'fieldId' if present, defaultValue otherwise.
    // Note: defaultValue is returned only when field is not found, not when e.g. field is present but has no value or has invalid value.
    template <class T>
    T fieldValue(FieldIdStrViewInputParam fieldId, const T& defaultValue, bool* pFieldExists = nullptr) const;

    String fieldValueStr(FieldIdStrViewInputParam fieldId, std::function<String()> defaultValueGenerator) const;
    String fieldValueStr(FieldIdStrViewInputParam fieldId) const; // Convenience overload, returns empty string as default.
    // Convenience overload, returns empty string as default and sets pFieldPresent to indicate existence of requested field.
    String fieldValueStr(FieldIdStrViewInputParam fieldId, bool* pFieldPresent) const;

    void forEachPropertyId(std::function<void(StringView svId)>) const;

private:
    virtual std::pair<bool, String> fieldValueStrImpl(FieldIdStrViewInputParam fieldId) const = 0;
    virtual void forEachPropertyIdImpl(std::function<void (StringView svId)>) const = 0;
}; // class AbstractChartControlItem


template <class T>
T AbstractChartControlItem::fieldValue(FieldIdStrViewInputParam fieldId, const T& defaultValue, bool* pFieldExists) const
{
    DFG_STATIC_ASSERT(std::is_arithmetic<T>::value, "Only arithmetic value are supported for now");

    auto rv = fieldValueStrImpl(fieldId);
    if (pFieldExists)
        *pFieldExists = rv.first;
    if (!rv.first) // Field not present?
        return defaultValue;
    T obj{};
    ::DFG_MODULE_NS(str)::strTo(rv.second.rawStorage(), obj);
    return obj;
}

inline auto AbstractChartControlItem::fieldValueStr(FieldIdStrViewInputParam fieldId, std::function<String()> defaultValueGenerator) const -> String
{
    auto rv = fieldValueStrImpl(fieldId);
    return (rv.first) ? rv.second : defaultValueGenerator();
}

inline auto AbstractChartControlItem::fieldValueStr(FieldIdStrViewInputParam fieldId) const -> String
{
    return fieldValueStr(fieldId, [] { return String(); });
}

inline auto AbstractChartControlItem::fieldValueStr(FieldIdStrViewInputParam fieldId, bool* pFieldPresent) const -> String
{
    if (pFieldPresent)
        *pFieldPresent = true;
    return fieldValueStr(fieldId, [&] { if (pFieldPresent) *pFieldPresent = false; return String(); });
}

inline void AbstractChartControlItem::forEachPropertyId(std::function<void(StringView svId)> func) const
{
    forEachPropertyIdImpl(func);
}

template <class T>
using InputSpan = RangeIterator_T<const T*>;

// Abstract base for items in chart.
class ChartObject
{
public:
    typedef DFG_CLASS_NAME(StringUtf8) ChartObjectString;
    typedef DFG_CLASS_NAME(StringViewUtf8) ChartObjectStringView;

    virtual ~ChartObject() {}

    void setName(ChartObjectStringView sv) { (m_spImplementation) ? m_spImplementation->setNameImpl(sv) : setNameImpl(sv); }
    void setLineColour(ChartObjectStringView sv) { (m_spImplementation) ? m_spImplementation->setLineColourImpl(sv) : setLineColourImpl(sv); }

protected:
    ChartObject() {}

    // Sets custom base to which virtual calls get passed.
    template <class T, class... ArgTypes_T>
    void setBaseImplementation(ArgTypes_T&&... args)
    {
        m_spImplementation.reset(new T(std::forward<ArgTypes_T>(args)...));
    }

private:
    virtual void setNameImpl(ChartObjectStringView) const {}
    virtual void setLineColourImpl(ChartObjectStringView) const {}

    std::unique_ptr<ChartObject> m_spImplementation;
}; // class ChartObject


class XySeries : public ChartObject
{
protected:
    XySeries() {}
public:
    virtual ~XySeries() {}
    virtual void setOrAppend(const size_t, const double, const double) = 0;
    virtual void resize(const size_t) = 0;

    virtual void setLineStyle(StringViewC) {}
    virtual void setPointStyle(StringViewC) {}

    // Sets x values and y values. If given x and y ranges have different size, request is ignored.
    virtual void setValues(InputSpan<double>, InputSpan<double>, const std::vector<bool>* pFilterFlags = nullptr) = 0;
}; // Class XySeries


class Histogram : public ChartObject
{
protected:
    Histogram() {}
public:
    virtual ~Histogram() {}

    // TODO: define meaning (e.g. bin centre)
    virtual void setValues(InputSpan<double>, InputSpan<double>) = 0;
}; // Class Histogram


template <class T>
using ChartObjectHolder = std::shared_ptr<T>;

class ChartConfigParam
{
public:
    using FieldIdStrViewInputParam = AbstractChartControlItem::FieldIdStrViewInputParam;

    ChartConfigParam(const AbstractChartControlItem* pPrimary = nullptr, const AbstractChartControlItem* pSecondary = nullptr)
    {
        m_configs[0] = pPrimary;
        m_configs[1] = pSecondary;
    }

    template <class T>
    T value(FieldIdStrViewInputParam id, T defaultVal = T()) const;

    // In order of use, most specific at index 0. The idea is to support setting hierarchy: e.g. first use value from panel settings, if not found, use global settings.
    // For now restricted to two levels without particular reason (i.e. can be increased).
    std::array<const AbstractChartControlItem*, 2> m_configs;
}; // Class ChartConfigParam

template <class T>
T ChartConfigParam::value(FieldIdStrViewInputParam fieldId, T defaultVal) const
{
    // Returning setting from most specific item that has requested ID
    for (auto pItem : m_configs)
    {
        if (!pItem)
            continue;
        bool bExists = false;
        auto rv = pItem->fieldValue<T>(fieldId, defaultVal, &bExists);
        if (bExists)
            return rv;
    }
    return defaultVal;
}

class ChartObjectCreationParam
{
public:
    ChartObjectCreationParam(ChartConfigParam configParam, const AbstractChartControlItem& defEntry);

    const AbstractChartControlItem& definitionEntry() const;
    StringViewUtf8 panelId() const;

    const ChartConfigParam& config() const { return m_config; }

    StringUtf8 sPanelId;
    ChartConfigParam m_config;
    const AbstractChartControlItem& m_rDefEntry;
};

inline ChartObjectCreationParam::ChartObjectCreationParam(ChartConfigParam configParam, const AbstractChartControlItem& defEntry)
    : m_config(configParam)
    , m_rDefEntry(defEntry)
{
    sPanelId = defEntry.fieldValueStr(ChartObjectFieldIdStr_panelId, []() { return StringUtf8(); });
}

inline const AbstractChartControlItem& ChartObjectCreationParam::definitionEntry() const
{
    return m_rDefEntry;
}

inline auto ChartObjectCreationParam::panelId() const -> StringViewUtf8
{
    return sPanelId;
}

class XySeriesCreationParam : public ChartObjectCreationParam
{
public:
    using BaseClass = ChartObjectCreationParam;
    XySeriesCreationParam(int argIndex, ChartConfigParam configParam, const AbstractChartControlItem& defEntry, ChartDataType, ChartDataType, StringUtf8 sXname = StringUtf8(), StringUtf8 sYname = StringUtf8());

    int nIndex;
    ChartDataType xType = ChartDataType::unknown;
    ChartDataType yType = ChartDataType::unknown;
    StringUtf8 m_sXname;
    StringUtf8 m_sYname;
};

inline XySeriesCreationParam::XySeriesCreationParam(const int argIndex, ChartConfigParam configParam, const AbstractChartControlItem& defEntry, ChartDataType argXtype, ChartDataType argYtype,
                                                    StringUtf8 sXname, StringUtf8 sYname)
    : BaseClass(configParam, defEntry)
    , nIndex(argIndex)
    , xType(argXtype)
    , yType(argYtype)
    , m_sXname(std::move(sXname))
    , m_sYname(std::move(sYname))
{
    
}

class HistogramCreationParam : public ChartObjectCreationParam
{
public:
    using BaseClass = ChartObjectCreationParam;
    HistogramCreationParam(ChartConfigParam configParam, const AbstractChartControlItem& defEntry, InputSpan<double>);

    InputSpan<double> valueRange;
};

inline HistogramCreationParam::HistogramCreationParam(ChartConfigParam configParam, const AbstractChartControlItem& defEntry, InputSpan<double> inputSpan)
    : BaseClass(configParam, defEntry)
    , valueRange(inputSpan)
{

}

class ChartCanvas
{
protected:
    ChartCanvas() {}
public:
    virtual ~ChartCanvas() {}

    virtual bool hasChartObjects() const = 0;

    virtual void addXySeries() = 0;

    virtual void setTitle(StringViewUtf8 /*svPanelId*/, StringViewUtf8 /*svTitle*/) {}

    virtual void setAxisForSeries(XySeries*, const double /*xMin*/, const double /*xMax*/, const double /*yMin*/, const double /*yMax*/) {}

    virtual void addContextMenuEntriesForChartObjects(void*) {} // TODO: no void*

    virtual void removeAllChartObjects() {}

    virtual ChartObjectHolder<XySeries> getSeriesByIndex(const XySeriesCreationParam&) { return nullptr; }
    virtual ChartObjectHolder<XySeries> getSeriesByIndex_createIfNonExistent(const XySeriesCreationParam&) { return nullptr; }

    virtual bool isLegendSupported() const { return false; }
    virtual bool isLegendEnabled() const { return false; }
    virtual bool enableLegend(bool) { return false; } // Returns true if enabled, false otherwise (e.g. if not supported)
    virtual void createLegends() {}

    virtual ChartObjectHolder<Histogram> createHistogram(const HistogramCreationParam&) { return nullptr; }

    virtual void setAxisLabel(StringViewUtf8 /*panelId*/, StringViewUtf8 /*axisId*/, StringViewUtf8 /*axisLabel*/) {}

    // Request to repaint canvas. Naming as repaintCanvas() instead of simply repaint() to avoid mixing with QWidget::repaint()
    virtual void repaintCanvas() = 0;

}; // class ChartCanvas

namespace DFG_DETAIL_NS
{
    static inline void checkForUnrecongnizedProperties(const AbstractChartControlItem& controlItem, std::function<void(AbstractChartControlItem::StringView)> func, const std::array<const char*, 10> knownItems)
    {
        controlItem.forEachPropertyId([&](StringViewUtf8 sv)
        {
            if (sv == SzPtrUtf8(ChartObjectFieldIdStr_type))
                return;
            auto iter = std::find_if(knownItems.begin(), knownItems.end(), [&](const char* psz)
            {
                return (psz && sv == SzPtrUtf8(psz));
            });
            if (iter == knownItems.end())
                func(sv);
        });
    }
} // namespace DFG_DETAIL_NS

// Calls given func for each unrecognized property.
inline void forEachUnrecognizedPropertyId(const AbstractChartControlItem& controlItem, std::function<void (AbstractChartControlItem::StringView)> func)
{
    using namespace DFG_DETAIL_NS;
    const auto sType = controlItem.fieldValueStr(ChartObjectFieldIdStr_type);
    const auto isType = [&](const char* pszId) { return sType == SzPtrUtf8(pszId); };
    if (isType(ChartObjectChartTypeStr_xy))
    {
        checkForUnrecongnizedProperties(controlItem, func, {
            ChartObjectFieldIdStr_enabled,
            ChartObjectFieldIdStr_name,
            ChartObjectFieldIdStr_lineStyle,
            ChartObjectFieldIdStr_pointStyle,
            ChartObjectFieldIdStr_xSource,
            ChartObjectFieldIdStr_ySource,
            ChartObjectFieldIdStr_xRows,
            ChartObjectFieldIdStr_panelId,
            ChartObjectFieldIdStr_lineColour,
            ChartObjectFieldIdStr_dataSource
            });
    }
    else if (isType(ChartObjectChartTypeStr_histogram))
    {
        checkForUnrecongnizedProperties(controlItem, func, {
            ChartObjectFieldIdStr_enabled,
            ChartObjectFieldIdStr_name,
            ChartObjectFieldIdStr_binCount,
            ChartObjectFieldIdStr_xSource,
            ChartObjectFieldIdStr_panelId,
            ChartObjectFieldIdStr_lineColour,
            ChartObjectFieldIdStr_dataSource
            });
    }
    else if (isType(ChartObjectChartTypeStr_panelConfig))
    {
        checkForUnrecongnizedProperties(controlItem, func, {
            ChartObjectFieldIdStr_enabled,
            ChartObjectFieldIdStr_panelId,
            ChartObjectFieldIdStr_title,
            ChartObjectFieldIdStr_xLabel,
            ChartObjectFieldIdStr_yLabel
            });
    }
    else if (isType(ChartObjectChartTypeStr_globalConfig))
    {
        checkForUnrecongnizedProperties(controlItem, func, {
            ChartObjectFieldIdStr_enabled,
            ChartObjectFieldIdStr_showLegend,
            ChartObjectFieldIdStr_autoAxisLabels,
            });
    }
    else
    {
        func(SzPtrUtf8(ChartObjectFieldIdStr_type));
    }
}


} } // Module namespace

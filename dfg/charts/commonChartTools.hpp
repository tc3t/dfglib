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

// Note: reflect all changes done here to documentation in qt/graphTools.cpp getGuideString() (remember also examples).
constexpr char ChartObjectFieldIdStr_enabled[] = "enabled";
constexpr char ChartObjectFieldIdStr_type[] = "type";
    constexpr char ChartObjectChartTypeStr_xy[] = "xy";
        // xy-type has properties: line_style, point_style, x_source, x_rows
    constexpr char ChartObjectChartTypeStr_histogram[] = "histogram";
    // histogram-type has properties: bin_count

// name: this will show e.g. in legend.
constexpr char ChartObjectFieldIdStr_name[] = "name";

// bin_count
constexpr char ChartObjectFieldIdStr_binCount[] = "bin_count";

// x_rows, value is semicolon separated list defining an IntervalSet of row indexes. For example "1:4; 8; 12:13" means rows 1, 2, 3, 4, 8, 12, 13.
constexpr char ChartObjectFieldIdStr_xRows[] = "x_rows";

// x_source, value is parenthesis parametrisized value (e.g. x_source: column_name(header 1))
constexpr char ChartObjectFieldIdStr_xSource[] = "x_source";
    constexpr char ChartObjectSourceTypeStr_columnName[] = "column_name";
    //constexpr char ChartObjectFieldIdStr_ySource[]      = "y_source";

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


// Abstract class representing an entry that defines what kind of ChartObjects to create on chart.
class AbstractChartControlItem
{
public:
    typedef StringUtf8 String;
    typedef StringViewC FieldIdStrView;
    typedef const StringViewC& FieldIdStrViewInputParam;

    virtual ~AbstractChartControlItem() {}

    // Returns value of field 'fieldId' if present, defaultValue otherwise.
    // Note: defaultValue is returned only when field is not found, not when e.g. field is present but has no value or has invalid value.
    template <class T>
    T fieldValue(FieldIdStrViewInputParam fieldId, const T& defaultValue) const;

    String fieldValueStr(FieldIdStrViewInputParam fieldId, std::function<String()> defaultValueGenerator) const;

private:
    virtual std::pair<bool, String> fieldValueStrImpl(FieldIdStrViewInputParam fieldId) const = 0;
}; // class AbstractChartControlItem


template <class T>
T AbstractChartControlItem::fieldValue(FieldIdStrViewInputParam fieldId, const T& defaultValue) const
{
    DFG_STATIC_ASSERT(std::is_arithmetic<T>::value, "Only arithmetic value are supported for now");

    auto rv = fieldValueStrImpl(fieldId);
    if (!rv.first) // Field not present?
        return defaultValue;
    T obj{};
    ::DFG_MODULE_NS(str)::strTo(rv.second.rawStorage(), obj);
    return obj;
}

auto AbstractChartControlItem::fieldValueStr(FieldIdStrViewInputParam fieldId, std::function<String()> defaultValueGenerator) const -> String
{
    auto rv = fieldValueStrImpl(fieldId);
    return (rv.first) ? rv.second : defaultValueGenerator();
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


class ChartCanvas
{
protected:
    ChartCanvas() {}
public:
    virtual ~ChartCanvas() {}

    virtual bool hasChartObjects() const = 0;

    virtual void addXySeries() = 0;

    virtual void setTitle(StringViewUtf8) {}

    virtual ChartObjectHolder<XySeries> getFirstXySeries() { return nullptr; }

    virtual void setAxisForSeries(XySeries*, const double /*xMin*/, const double /*xMax*/, const double /*yMin*/, const double /*yMax*/) {}

    virtual void addContextMenuEntriesForChartObjects(void*) {} // TODO: no void*

    virtual void removeAllChartObjects() {}

    virtual ChartObjectHolder<XySeries> getSeriesByIndex(int) { return nullptr; }
    virtual ChartObjectHolder<XySeries> getSeriesByIndex_createIfNonExistent(int) { return nullptr; }

    virtual bool isLegendSupported() const { return false; }
    virtual bool isLegendEnabled() const { return false; }
    virtual bool enableLegend(bool) { return false; } // Returns true if enabled, false otherwise (e.g. if not supported)

    virtual ChartObjectHolder<Histogram> createHistogram(const AbstractChartControlItem&, InputSpan<double>) { return nullptr; }

    // Request to repaint canvas. Naming as repaintCanvas() instead of simply repaint() to avoid mixing with QWidget::repaint()
    virtual void repaintCanvas() = 0;

}; // class ChartCanvas


} } // Module namespace

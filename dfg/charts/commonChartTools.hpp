#pragma once

#include "../dfgDefs.hpp"
#include <functional>
#include <memory>
#include <utility>

#include "../rangeIterator.hpp"
#include "../ReadOnlySzParam.hpp"
#include "../str/string.hpp"
#include "../str/strTo.hpp"
#include "../alg.hpp"
#include "../io/BasicImStream.hpp"
#include "../io/DelimitedTextReader.hpp"
#include "../cont/valueArray.hpp"
#include "../cont/IntervalSetSerialization.hpp"
#include "../cont/Flags.hpp"

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
    -if adding new types or new properties for existing types, update forEachUnrecognizedPropertyId()
*/

inline namespace fieldsIds
{

constexpr char ChartObjectFieldIdStr_enabled[] = "enabled";
constexpr char ChartObjectFieldIdStr_type[] = "type";
constexpr char ChartObjectFieldIdStr_errorString[] = "error_string"; // If for example contruction from string failed due to parse error, error message can be set to this field.
    constexpr char ChartObjectChartTypeStr_xy[] = "xy";
        // xy-type has properties: see list defined in forEachUnrecognizedPropertyId()

    constexpr char ChartObjectChartTypeStr_txy[] = "txy";
        // txy-type has properties: see list defined in forEachUnrecognizedPropertyId()

    constexpr char ChartObjectChartTypeStr_txys[] = "txys";
        // txys-type has properties: see list defined in forEachUnrecognizedPropertyId().

    constexpr char ChartObjectChartTypeStr_histogram[] = "histogram";
        // histogram-type has properties: see list defined in forEachUnrecognizedPropertyId()

    constexpr char ChartObjectChartTypeStr_bars[] = "bars";
        // bars-type has properties: see list defined in forEachUnrecognizedPropertyId()

    constexpr char ChartObjectChartTypeStr_panelConfig[] = "panel_config";
        // panel_config-type has properties: see list defined in forEachUnrecognizedPropertyId()

    constexpr char ChartObjectChartTypeStr_globalConfig[] = "global_config";
        // global_config-type has properties: see list defined in forEachUnrecognizedPropertyId()


// data_source: defines data source for ChartObject.
constexpr char ChartObjectFieldIdStr_dataSource[] = "data_source";

// name: this will show e.g. in legend.
constexpr char ChartObjectFieldIdStr_name[] = "name";

// log_level: defines how verbose logging should be produced by setting the least significant level shown. Possible values: {debug, info, warning, error, none}.
constexpr char ChartObjectFieldIdStr_logLevel[] = "log_level";

// bin_count
constexpr char ChartObjectFieldIdStr_binCount[] = "bin_count";

// bin_type
constexpr char ChartObjectFieldIdStr_binType[] = "bin_type";

// bar_width_factor
constexpr char ChartObjectFieldIdStr_barWidthFactor[] = "bar_width_factor";

// merge_identical_labels
constexpr char ChartObjectFieldIdStr_mergeIdenticalLabels[] = "merge_identical_labels";

// stack_on_existing_labels
constexpr char ChartObjectFieldIdStr_stackOnExistingLabels[] = "stack_on_existing_labels";

// bar_label
constexpr char ChartObjectFieldIdStr_barLabel[] = "bar_label";

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

// y2 label
constexpr char ChartObjectFieldIdStr_y2Label[] = "y2_label";

// x_tick_label_direction
constexpr char ChartObjectFieldIdStr_xTickLabelDirection[] = "x_tick_label_direction";

// y_tick_label_direction
constexpr char ChartObjectFieldIdStr_yTickLabelDirection[] = "y_tick_label_direction";

// background
constexpr char ChartObjectFieldIdStr_background[] = "background";

// x_rows, value is semicolon separated list defining an IntervalSet of row indexes. For example "1:4; 8; 12:13" means rows 1, 2, 3, 4, 8, 12, 13.
constexpr char ChartObjectFieldIdStr_xRows[] = "x_rows";

// x_source/y_source, value is parenthesis parametrisized value (e.g. x_source: column_name(header 1))
constexpr char ChartObjectFieldIdStr_xSource[] = "x_source";
constexpr char ChartObjectFieldIdStr_ySource[] = "y_source";
constexpr char ChartObjectFieldIdStr_zSource[] = "z_source";
    constexpr char ChartObjectSourceTypeStr_columnName[]    = "column_name";
    constexpr char ChartObjectSourceTypeStr_rowIndex[]      = "row_index"; // Value is to be taken from row index of another column.

// panel_id: defines panel to which ChartObject is to be assigned to.
//      Possible values:
//          -grid(r, c): instructs to put chart object to position row = r, column = c on a panel grid. Indexing is 1-based. Value (1,1) means top left corner.
constexpr char ChartObjectFieldIdStr_panelId[] = "panel_id";

// y_axis_id: defines y-axis to which ChartObject is to be assigned to.
//      Possible values:
//          -'y', 'y2'. Default value is 'y'
constexpr char ChartObjectFieldIdStr_yAxisId[] = "y_axis_id";

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

// fill_colour
constexpr char ChartObjectFieldIdStr_fillColour[] = "fill_colour";

// axis_label_colour: sets colour of labels and axis lines for all axes.
// Note: using this together with axis_properties_ in the same entry definition is not supported.
constexpr char ChartObjectFieldIdStr_axisLabelColour[] = "axis_label_colour";

// axis_properties_: Defines axis properties as (key, value) list for axis whose id appears after underscore.
//      Available properties:
//          -line_colour
//          -label_colour
//          -range_start
//          -range_end
// Example: "axis_properties_y": "(line_colour, red, label_colour, green, range_start, 10, range_end, 20)"
constexpr char ChartObjectFieldIdStr_axisProperties[] = "axis_properties_";
constexpr char ChartObjectFieldIdStr_axisProperty_lineColour[]  = "line_colour";
constexpr char ChartObjectFieldIdStr_axisProperty_labelColour[] = "label_colour";
constexpr char ChartObjectFieldIdStr_axisProperty_rangeStart[]  = "range_start";
constexpr char ChartObjectFieldIdStr_axisProperty_rangeEnd[]    = "range_end";

// operation_
constexpr char ChartObjectFieldIdStr_operation[] = "operation_";

} // namespace fieldsIds

template <class T> using ValueVectorT = ::DFG_MODULE_NS(cont)::ValueVector<double>;
using ValueVectorD = ValueVectorT<double>;

// Enum-like class for defining chart data type and for related queries.
class ChartDataType
{
public:
    enum DataType
    {
        unknown,
        // dayTime-domain
        dayTime,
        dayTimeMillisecond,
        firstDayTimeItem = dayTime,
        lastDayTimeItem = dayTimeMillisecond,
        // Date without timezone domain
        dateOnlyYearMonth,
        dateOnly,
        dateAndTime,
        dateAndTimeMillisecond,
        firstDateNoTzItem = dateOnlyYearMonth,
        lastDateNoTzItem = dateAndTimeMillisecond,
        // Date with timezone domain
        dateAndTimeTz,
        dateAndTimeMillisecondTz,
        firstDateWithTzItem = dateAndTimeTz,
        lastDateWithTzItem = dateAndTimeMillisecondTz
    };

    ChartDataType(DataType dt = unknown) :
        m_dataType(dt)
    {}

    operator DataType() const
    {
        return m_dataType;
    }

    bool isDateNoTzType() const
    {
        return m_dataType >= firstDateNoTzItem && m_dataType <= lastDateNoTzItem;
    }

    bool isDateWithTzType() const
    {
        return m_dataType >= firstDateWithTzItem && m_dataType <= lastDateWithTzItem;
    }

    bool isTimeType() const
    {
        return m_dataType >= firstDayTimeItem && m_dataType <= lastDayTimeItem;
    }

    // Sets new type if both are of the same domain and arg expands type in the domain; e.g. dateOnly -> dateAndTime is expanding, but not the other way around.
    void setIfExpands(const ChartDataType other);

    DataType m_dataType;
};

// Sets new type if both are of the same domain and arg expands type in the domain; e.g. dateOnly -> dateAndTime is expanding, but not the other way around.
inline void ChartDataType::setIfExpands(const ChartDataType other)
{
    if (m_dataType == unknown)
        return;
    else if (this->isTimeType())
    {
        if (other.isTimeType())
            m_dataType = Max(m_dataType, other.m_dataType);
    }
    else if (this->isDateNoTzType())
    {
        if (other.isDateNoTzType())
            m_dataType = Max(m_dataType, other.m_dataType);
    }
    else if (this->isDateWithTzType())
    {
        if (other.isDateWithTzType())
            m_dataType = Max(m_dataType, other.m_dataType);
    }
}

namespace DFG_DETAIL_NS
{
    // Helper class for dealing with items of format key_name(comma-separated list of args with csv-quoting)
    // Examples: 
    //      input: 'some_id(arg one,"arg two that has , in it",arg three)' -> {key="some_id", values[3]={"arg one", "arg two that has , in it", "arg three"}
    class ParenthesisItem
    {
    public:
        using StringT = StringUtf8;
        using StringView = StringViewUtf8;
        using StringViewSz = StringViewSzUtf8;
        using ValueContainer = std::vector<StringT>;

        // Given string must accessible for the lifetime of 'this'
        ParenthesisItem(const StringT& sv);
        ParenthesisItem(StringT&&) = delete;

        ValueContainer::const_iterator begin() const { return m_values.cbegin(); }
        ValueContainer::const_iterator end()   const { return m_values.cend(); }

        // Constructs from StringView promised to outlive 'this'
        static ParenthesisItem fromStableView(StringView sv);

        StringView key() const { return m_key; }

        size_t valueCount() const { return m_values.size(); }

        template <class T>
        T valueAs(const size_t nIndex, bool* pSuccess = nullptr) const
        {
            return ::DFG_MODULE_NS(str)::strTo<T>(value(nIndex), pSuccess);
        }

        StringViewSz value(const size_t nIndex) const
        {
            return (isValidIndex(m_values, nIndex)) ? m_values[nIndex] : StringViewSz(DFG_UTF8(""));
        }

    private:
        ParenthesisItem(const StringView& sv);

    public:
        StringView m_key;
        std::vector<StringT> m_values;
    };

    inline ParenthesisItem::ParenthesisItem(const StringT& s) :
        ParenthesisItem(StringView(s))
    {
    }

    inline ParenthesisItem::ParenthesisItem(const StringView& sv)
    {
        auto iterOpen = std::find(sv.beginRaw(), sv.endRaw(), '(');
        if (iterOpen == sv.endRaw())
            return; // Didn't find opening parenthesis
        m_key = StringView(sv.data(), SzPtrUtf8(iterOpen));
        iterOpen++; // Skipping opening parenthesis
        auto iterEnd = sv.endRaw();
        if (iterOpen == iterEnd)
            return;
        --iterEnd;
        // Skipping trailing whitespaces
        while (iterEnd != iterOpen && DFG_MODULE_NS(alg)::contains(" ", *iterEnd))
            --iterEnd;
        // Checking that items end with closing parenthesis.
        if (*iterEnd != ')')
            return;
        // Parsing item inside parenthesis as standard comma-delimited item with quotes.
        using DelimitedTextReader = ::DFG_MODULE_NS(io)::DelimitedTextReader;
        DelimitedTextReader::FormatDefinitionSingleChars format('"', DelimitedTextReader::s_nMetaCharNone, ',');
        format.setFlag(DelimitedTextReader::rfSkipLeadingWhitespaces, true);
        DFG_MODULE_NS(io)::BasicImStream istrm(iterOpen, iterEnd - iterOpen);
        DelimitedTextReader::read<char>(istrm, format, [&](size_t, size_t, const char* p, const size_t nSize)
        {
            m_values.push_back(StringT(TypedCharPtrUtf8R(p), TypedCharPtrUtf8R(p + nSize)));
        });
    }

    inline auto ParenthesisItem::fromStableView(StringView sv) -> ParenthesisItem
    {
        return ParenthesisItem(sv);
    }
} // namespace DFG_DETAIL_NS


// Abstract class representing an entry that defines what kind of ChartObjects to create on chart.
class AbstractChartControlItem
{
public:
    typedef StringUtf8 String;
    typedef StringViewUtf8 StringView;
    typedef StringViewC FieldIdStrView;
    typedef const StringViewC& FieldIdStrViewInputParam;

    using StringViewOrOwner = ::DFG_ROOT_NS::StringViewOrOwner<StringViewSzC, std::string>;

    // The bigger the level, the more verbose logging is.
    enum class LogLevel
    {
        none,       // 0
        error,      // 1
        warning,    // 2
        info,       // 3
        debug       // 4
    }; // LogLevel

    virtual ~AbstractChartControlItem() {}

    // Returns value of field 'fieldId' if present, defaultValue otherwise.
    // Note: defaultValue is returned only when field is not found, not when e.g. field is present but has no value or has invalid value.
    template <class T>
    T fieldValue(FieldIdStrViewInputParam fieldId, const T& defaultValue, bool* pFieldExists = nullptr) const;

    String fieldValueStr(FieldIdStrViewInputParam fieldId, std::function<String()> defaultValueGenerator) const;
    String fieldValueStr(FieldIdStrViewInputParam fieldId) const; // Convenience overload, returns empty string as default.
    // Convenience overload, returns empty string as default and sets pFieldPresent to indicate existence of requested field.
    String fieldValueStr(FieldIdStrViewInputParam fieldId, bool* pFieldPresent) const;

    bool hasField(FieldIdStrViewInputParam fieldId) const { return hasFieldImpl(fieldId); }

    void forEachPropertyId(std::function<void(StringView svId)>) const;
    void forEachPropertyIdStartingWith(FieldIdStrViewInputParam fieldId, std::function<void(StringView, StringView)>) const;

    LogLevel logLevel() const { return m_logLevel; }

    // Sets requested log level if given argument is valid level specifier. Returns logLevel that is in effect after this call.
    // If pValidInput is given, it receives flag indicating whether given input was valid.
    LogLevel logLevel(const StringView& sv, bool* pValidInput = nullptr);

    LogLevel logLevel(LogLevel newLevel);

    bool isLoggingAllowedForLevel(LogLevel logLevel) const;

    // Creates IntervalSet from x_rows-item. Returns &rInterval if x_rows was present and it was not default "include all", nullptr otherwise.
    template <class Interval_T>
    Interval_T* createXrowsSet(Interval_T& rInterval, const typename Interval_T::value_type nWrapAt) const;

    // Returns graph type as string. String view is guaranteed valid for lifetime of *this.
    virtual StringViewOrOwner graphTypeStr() const { return StringViewOrOwner::makeOwned(std::string()); }

private:
    virtual bool hasFieldImpl(FieldIdStrViewInputParam fieldId) const;
    virtual std::pair<bool, String> fieldValueStrImpl(FieldIdStrViewInputParam fieldId) const = 0;
    virtual void forEachPropertyIdImpl(std::function<void (StringView svId)>) const = 0;

public:
    LogLevel m_logLevel = LogLevel::info;
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

inline bool AbstractChartControlItem::hasFieldImpl(FieldIdStrViewInputParam fieldId) const
{
    // This default implementation is suboptimal; possibly fetches the string even though it is not needed.
    bool bHasField;
    fieldValueStr(fieldId, &bHasField);
    return bHasField;
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

inline void AbstractChartControlItem::forEachPropertyIdStartingWith(FieldIdStrViewInputParam fieldId, std::function<void(StringView, StringView)> func) const
{
    forEachPropertyId([&](StringView svId)
    {
        if (::DFG_MODULE_NS(str)::beginsWith(svId.asUntypedView(), fieldId))
        {
            const auto svPropId = svId.asUntypedView().substr_start(::DFG_MODULE_NS(str)::strLen(fieldId));
            func(svId, StringView(TypedCharPtrUtf8R(svPropId.beginRaw()), TypedCharPtrUtf8R(svPropId.endRaw())));
        }
    });
}

inline auto AbstractChartControlItem::logLevel(const StringView& sv, bool* pValidInput) -> LogLevel
{
    if (pValidInput)
        *pValidInput = true;
    if (sv == DFG_UTF8("debug"))
        logLevel(LogLevel::debug);
    else if (sv == DFG_UTF8("info"))
        logLevel(LogLevel::info);
    else if (sv == DFG_UTF8("warning"))
        logLevel(LogLevel::warning);
    else if (sv == DFG_UTF8("error"))
        logLevel(LogLevel::error);
    else if (sv == DFG_UTF8("none"))
        logLevel(LogLevel::none);
    else if (pValidInput)
        *pValidInput = false;
    return logLevel();
}

inline auto AbstractChartControlItem::logLevel(const LogLevel newLevel) -> LogLevel
{
    m_logLevel = newLevel;
    return logLevel();
}

inline bool AbstractChartControlItem::isLoggingAllowedForLevel(const LogLevel logLevel) const
{
    return logLevel <= this->logLevel();
}

template <class Interval_T>
inline Interval_T* AbstractChartControlItem::createXrowsSet(Interval_T& rInterval, const typename Interval_T::value_type nWrapAt) const
{
    const auto xRowsStr = this->fieldValueStr(ChartObjectFieldIdStr_xRows, []() { return String(DFG_UTF8("*")); });
    if (!xRowsStr.empty() && xRowsStr != DFG_UTF8("*"))
    {
        rInterval = ::DFG_MODULE_NS(cont)::intervalSetFromString<typename Interval_T::value_type>(xRowsStr.rawStorage(), nWrapAt);
        return &rInterval;
    }
    else
        return nullptr;
}

template <class T>
using InputSpan = RangeIterator_T<const T*>;

enum class ChartObjectType
{
    unknown,
    xySeries,
    histogram,
    barSeries
};

// Abstract base for items in chart.
class ChartObject
{
public:
    typedef StringUtf8 ChartObjectString;
    typedef StringViewUtf8 ChartObjectStringView;
    using StringViewOrOwnerUtf8 = StringViewOrOwner<StringViewUtf8, StringUtf8>;
    using InputSpanD = InputSpan<double>;

    virtual ~ChartObject() {}

    void setName(ChartObjectStringView sv) { (m_spImplementation) ? m_spImplementation->setNameImpl(sv) : setNameImpl(sv); }
    void setLineColour(ChartObjectStringView sv) { (m_spImplementation) ? m_spImplementation->setLineColourImpl(sv) : setLineColourImpl(sv); }
    void setFillColour(ChartObjectStringView sv) { (m_spImplementation) ? m_spImplementation->setFillColourImpl(sv) : setFillColourImpl(sv); }

    StringViewOrOwnerUtf8 name() const { return (m_spImplementation) ? m_spImplementation->nameImpl() : nameImpl(); }

          ChartObject* implementationObject()       { return (m_spImplementation) ? m_spImplementation.get() : this; }
    const ChartObject* implementationObject() const { return (m_spImplementation) ? m_spImplementation.get() : this; }

    ChartObjectType type() const { return m_type; }

protected:
    ChartObject(ChartObjectType type)
        : m_type(type)
    {}

    // Sets custom base to which virtual calls get passed.
    template <class T, class... ArgTypes_T>
    void setBaseImplementation(ArgTypes_T&&... args)
    {
        m_spImplementation.reset(new T(std::forward<ArgTypes_T>(args)...));
    }

private:
    virtual void setNameImpl(ChartObjectStringView) {}
    virtual StringViewOrOwnerUtf8 nameImpl() const { return StringViewOrOwnerUtf8::makeOwned(); }
    virtual void setLineColourImpl(ChartObjectStringView) {}
    virtual void setFillColourImpl(ChartObjectStringView) {}

    std::unique_ptr<ChartObject> m_spImplementation;
    ChartObjectType m_type = ChartObjectType::unknown;
}; // class ChartObject


class XySeries : public ChartObject
{
public:
    using BaseClass = ChartObject;
    class PointMetaData
    {
    public:
        PointMetaData() = default;

        // Constructs metadata from a string view that caller guarantees to be readable as long as call context requires.
        // For example with setMetaDataByFunctor() until the next call of callback function or end of setMetaDataByFunctor() function.
        static PointMetaData fromStableStringView(const StringViewUtf8 sv)
        {
            PointMetaData m;
            m.m_sv = sv;
            return m;
        }

        StringViewUtf8 string() const
        {
            return m_sv;
        }

        StringViewUtf8 m_sv;
    }; // class PointMetaData

    using MetaDataSetterCallback = std::function<PointMetaData(double, double, double)>;
protected:
    XySeries() : BaseClass(ChartObjectType::xySeries) {}
public:
    virtual ~XySeries() {}
    virtual void resize(const size_t) = 0;

    virtual void setLineStyle(StringViewC) {}
    void setLineStyle(StringViewUtf8 sv)  { setLineStyle(StringViewC(sv.beginRaw(), sv.endRaw())); }
    virtual void setPointStyle(StringViewC) {}
    void setPointStyle(StringViewUtf8 sv) { setPointStyle(StringViewC(sv.beginRaw(), sv.endRaw())); }

    // Sets x values and y values. If given x and y ranges have different size, request is ignored.
    virtual void setValues(InputSpanD, InputSpanD, const std::vector<bool>* pFilterFlags = nullptr) = 0;

    // When xy-series represents (x, y, text)-triplet, this function will call 'func'
    // for every (index, x, y)-triplet and sets returned PointMetaData as metadata for point (index, x, y).
    // @return The number of points for which 'func' was called.
    virtual size_t setMetaDataByFunctor(MetaDataSetterCallback func) { DFG_UNUSED(func); return 0; }
}; // Class XySeries


class Histogram : public ChartObject
{
public:
    using BaseClass = ChartObject;
protected:
    Histogram() : BaseClass(ChartObjectType::histogram) {}
public:
    virtual ~Histogram() {}

    // TODO: define meaning (e.g. bin centre)
    virtual void setValues(InputSpanD, InputSpanD) = 0;
}; // Class Histogram

class BarSeries : public ChartObject
{
public:
    using BaseClass = ChartObject;
protected:
    BarSeries() : BaseClass(ChartObjectType::barSeries) {}
public:
    virtual ~BarSeries() {}
}; // Class BarSeries

template <class T>
using ChartObjectHolder = std::shared_ptr<T>;

class ChartConfigParam
{
public:
    using FieldIdStrViewInputParam = AbstractChartControlItem::FieldIdStrViewInputParam;
    using String                   = AbstractChartControlItem::String;
    using StringView               = AbstractChartControlItem::StringView;

    ChartConfigParam(const AbstractChartControlItem* pPrimary = nullptr, const AbstractChartControlItem* pSecondary = nullptr)
    {
        m_configs[0] = pPrimary;
        m_configs[1] = pSecondary;
    }

    template <class T>
    T value(FieldIdStrViewInputParam id, T defaultVal = T()) const;

    String valueStr(FieldIdStrViewInputParam id, const StringView& svDefaultVal = StringView()) const;

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

inline auto ChartConfigParam::valueStr(FieldIdStrViewInputParam fieldId, const StringView& svDefaultVal) const -> String
{
    // Returning setting from most specific item that has requested ID
    for (auto pItem : m_configs)
    {
        if (!pItem)
            continue;
        bool bExists = true;
        auto rv = pItem->fieldValueStr(fieldId, [&]() { bExists = false; return StringUtf8(); });
        if (bExists)
            return rv;
    }
    return svDefaultVal.toString();
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
    HistogramCreationParam(ChartConfigParam configParam, const AbstractChartControlItem& defEntry, InputSpan<double>, InputSpan<double> counts, ChartDataType argXtype, StringUtf8 sXname);
    HistogramCreationParam(ChartConfigParam configParam, const AbstractChartControlItem& defEntry, InputSpan<StringUtf8> binNames, InputSpan<double> counts, StringUtf8 sXname);

    InputSpan<double> valueRange;
    InputSpan<StringUtf8> stringValueRange;
    InputSpan<double> countRange;
    ChartDataType xType = ChartDataType::unknown;
    StringUtf8 m_sXname;
};

inline HistogramCreationParam::HistogramCreationParam(ChartConfigParam configParam, const AbstractChartControlItem& defEntry, InputSpan<double> inputSpan, InputSpan<double> counts, const ChartDataType argXtype, StringUtf8 sXname)
    : BaseClass(configParam, defEntry)
    , valueRange(inputSpan)
    , countRange(counts)
    , xType(argXtype)
    , m_sXname(std::move(sXname))
{

}

inline HistogramCreationParam::HistogramCreationParam(ChartConfigParam configParam, const AbstractChartControlItem& defEntry, InputSpan<StringUtf8> strings, InputSpan<double> counts, StringUtf8 sXname)
    : BaseClass(configParam, defEntry)
    , stringValueRange(strings)
    , countRange(counts)
    , m_sXname(std::move(sXname))
{
}

class BarSeriesCreationParam : public ChartObjectCreationParam
{
public:
    using BaseClass = ChartObjectCreationParam;
    BarSeriesCreationParam(ChartConfigParam configParam, const AbstractChartControlItem& defEntry, InputSpan<StringUtf8>, InputSpan<double>, ChartDataType argXtype, StringUtf8 sXname, StringUtf8 sYname);

    InputSpan<StringUtf8> labelRange;
    InputSpan<double> valueRange;
    ChartDataType xType = ChartDataType::unknown;
    StringUtf8 m_sXname;
    StringUtf8 m_sYname;
};

inline BarSeriesCreationParam::BarSeriesCreationParam(ChartConfigParam configParam, const AbstractChartControlItem& defEntry, InputSpan<StringUtf8> argLabelRange, InputSpan<double> argValueRange, ChartDataType argXtype,
                                                      StringUtf8 sXname, StringUtf8 sYname)
    : BaseClass(configParam, defEntry)
    , labelRange(argLabelRange)
    , valueRange(argValueRange)
    , xType(argXtype)
    , m_sXname(std::move(sXname))
    , m_sYname(std::move(sYname))
{

}

class ChartPanel
{
public:
    virtual ~ChartPanel() {}

    static constexpr uint32 unknownCount() { return (std::numeric_limits<uint32>::max)(); }

    virtual uint32 countOf(AbstractChartControlItem::FieldIdStrViewInputParam) const { return unknownCount(); }
};

class ChartCanvas
{
protected:
    ChartCanvas() {}
public:
    using XySeries = ::DFG_MODULE_NS(charts)::XySeries;
    using Histogram = ::DFG_MODULE_NS(charts)::Histogram;
    using BarSeries = ::DFG_MODULE_NS(charts)::BarSeries;
    using ChartObjectCreationParam = ::DFG_MODULE_NS(charts)::ChartObjectCreationParam;
    using XySeriesCreationParam = ::DFG_MODULE_NS(charts)::XySeriesCreationParam;
    using HistogramCreationParam = ::DFG_MODULE_NS(charts)::HistogramCreationParam;
    using BarSeriesCreationParam = ::DFG_MODULE_NS(charts)::BarSeriesCreationParam;
    template <class T> using ChartObjectHolder = ::DFG_MODULE_NS(charts)::ChartObjectHolder<T>;
    using ArgList = ::DFG_MODULE_NS(charts)::DFG_DETAIL_NS::ParenthesisItem;

    DFG_DEFINE_SCOPED_ENUM_FLAGS(ToolTipTextRequestFlags, int,
        html            = 0x1,      // Request tooltip in HTML-format
        plainText       = 0x2,      // Request tooltip in plain text -format
        json_compact    = 0x4,      // Request tooltip in compact json-format
        json_indented   = 0x8,      // Request tooltip in intented json-format
        csv             = 0x10,     // Request tooltip in csv-format
        any             = html | plainText | json_compact | json_indented | csv
    )

    virtual ~ChartCanvas() {}

    virtual bool hasChartObjects() const = 0;

    virtual void setPanelTitle(StringViewUtf8 /*svPanelId*/, StringViewUtf8 /*svTitle*/, StringViewUtf8 /*svColor*/) {}

    virtual void optimizeAllAxesRanges() {}

    virtual void addContextMenuEntriesForChartObjects(void*) {} // TODO: no void*

    virtual void removeAllChartObjects(bool bRepaint = true) { DFG_UNUSED(bRepaint); }

    virtual int width() const { return 0; }
    virtual int height() const { return 0; }

    virtual void setBackground(const StringViewUtf8&) {};

    // Sets colour to all axes in panel including axes ticks.
    virtual void setPanelAxesColour(StringViewUtf8 /*svPanelId*/, StringViewUtf8 /*svColourDef*/) {}
    // Sets colour to all panel axes labels
    virtual void setPanelAxesLabelColour(StringViewUtf8 /*svPanelId*/, StringViewUtf8 /*svColourDef*/) {}

    virtual void setAxisProperties(const StringViewUtf8& /*svPanelId*/, const StringViewUtf8& /*svAxisId*/, const ArgList& /*args*/) {};

    virtual bool isLegendSupported() const { return false; }
    virtual bool isToolTipSupported() const { return false; }
    virtual bool isLegendEnabled() const { return false; }
    virtual bool isToolTipEnabled() const { return false; }

    StringUtf8 createToolTipTextAt(int x, int y, ToolTipTextRequestFlags flags = ToolTipTextRequestFlags::any) const { return createToolTipTextAtImpl(x, y, flags); } // Creates tooltip text at implementation specific canvas coordinates.
    virtual bool enableLegend(bool) { return false; } // Returns true if enabled, false otherwise (e.g. if not supported)
    virtual bool enableToolTip(bool) { return false; } // Returns true if enabled, false otherwise (e.g. if not supported)
    virtual void createLegends() {}
    virtual ChartPanel* findPanelOfChartObject(const ChartObject*) { return nullptr; }

    // Gives a hint to chart that it is now being updated so it can e.g. show some kind of update indicator.
    // If concrete instance has implementation for this, it must automatically reset the state on call to repaintCanvas()
    virtual void beginUpdateState() {};

    virtual ChartObjectHolder<XySeries>  createXySeries(const XySeriesCreationParam&)   { return nullptr; }
    virtual ChartObjectHolder<Histogram> createHistogram(const HistogramCreationParam&) { return nullptr; }
    virtual std::vector<ChartObjectHolder<BarSeries>> createBarSeries(const BarSeriesCreationParam&) { return std::vector<ChartObjectHolder<BarSeries>>(); }

    virtual void setAxisLabel(StringViewUtf8 /*panelId*/, StringViewUtf8 /*axisId*/, StringViewUtf8 /*axisLabel*/) {}

    virtual void setAxisTickLabelDirection(StringViewUtf8 /*panelId*/, StringViewUtf8 /*axisId*/, StringViewUtf8 /*value*/) {}

    // Request to repaint canvas. Naming as repaintCanvas() instead of simply repaint() to avoid mixing with QWidget::repaint()
    virtual void repaintCanvas() = 0;
    
private:
    virtual StringUtf8 createToolTipTextAtImpl(int, int, ToolTipTextRequestFlags) const { return StringUtf8(); }

}; // class ChartCanvas

namespace DFG_DETAIL_NS
{
    static inline void checkForUnrecongnizedProperties(const AbstractChartControlItem& controlItem, std::function<void(AbstractChartControlItem::StringView)> func, const std::array<const char*, 15> knownItems)
    {
        controlItem.forEachPropertyId([&](StringViewUtf8 sv)
        {
            if (sv == SzPtrUtf8(ChartObjectFieldIdStr_type))
                return;
            auto iter = std::find_if(knownItems.begin(), knownItems.end(), [&](const char* psz)
            {
                if (!psz)
                    return false;
                return (sv == SzPtrUtf8(psz)
                    || (StringViewC(ChartObjectFieldIdStr_operation) == psz && ::DFG_MODULE_NS(str)::beginsWith(sv, StringViewUtf8(SzPtrUtf8(ChartObjectFieldIdStr_operation))))
                    || (StringViewC(ChartObjectFieldIdStr_axisProperties) == psz && ::DFG_MODULE_NS(str)::beginsWith(sv, StringViewUtf8(SzPtrUtf8(ChartObjectFieldIdStr_axisProperties))))
                    );
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
    if (isType(ChartObjectChartTypeStr_xy) || isType(ChartObjectChartTypeStr_txy) || isType(ChartObjectChartTypeStr_txys))
    {
        checkForUnrecongnizedProperties(controlItem, func, {
            ChartObjectFieldIdStr_enabled,
            ChartObjectFieldIdStr_logLevel,
            ChartObjectFieldIdStr_name,
            ChartObjectFieldIdStr_lineStyle,
            ChartObjectFieldIdStr_pointStyle,
            ChartObjectFieldIdStr_xSource,
            ChartObjectFieldIdStr_ySource,
            (isType(ChartObjectChartTypeStr_txys) ? ChartObjectFieldIdStr_zSource : nullptr),
            ChartObjectFieldIdStr_xRows,
            ChartObjectFieldIdStr_panelId,
            ChartObjectFieldIdStr_yAxisId,
            ChartObjectFieldIdStr_lineColour,
            ChartObjectFieldIdStr_dataSource,
            ChartObjectFieldIdStr_operation
            });
    }
    else if (isType(ChartObjectChartTypeStr_histogram))
    {
        checkForUnrecongnizedProperties(controlItem, func, {
            ChartObjectFieldIdStr_enabled,
            ChartObjectFieldIdStr_logLevel,
            ChartObjectFieldIdStr_name,
            ChartObjectFieldIdStr_binCount,
            ChartObjectFieldIdStr_barWidthFactor,
            ChartObjectFieldIdStr_binType,
            ChartObjectFieldIdStr_xSource,
            ChartObjectFieldIdStr_xRows,
            ChartObjectFieldIdStr_panelId,
            ChartObjectFieldIdStr_yAxisId,
            ChartObjectFieldIdStr_lineColour,
            ChartObjectFieldIdStr_fillColour,
            ChartObjectFieldIdStr_dataSource,
            ChartObjectFieldIdStr_operation
            });
    }
    else if (isType(ChartObjectChartTypeStr_bars))
    {
        checkForUnrecongnizedProperties(controlItem, func, {
            ChartObjectFieldIdStr_enabled,
            ChartObjectFieldIdStr_logLevel,
            ChartObjectFieldIdStr_name,
            ChartObjectFieldIdStr_panelId,
            ChartObjectFieldIdStr_yAxisId,
            ChartObjectFieldIdStr_lineColour,
            ChartObjectFieldIdStr_fillColour,
            ChartObjectFieldIdStr_dataSource,
            ChartObjectFieldIdStr_xSource,
            ChartObjectFieldIdStr_ySource,
            ChartObjectFieldIdStr_xRows,
            ChartObjectFieldIdStr_mergeIdenticalLabels,
            ChartObjectFieldIdStr_stackOnExistingLabels,
            ChartObjectFieldIdStr_barLabel,
            ChartObjectFieldIdStr_operation
            });
    }
    else if (isType(ChartObjectChartTypeStr_panelConfig))
    {
        checkForUnrecongnizedProperties(controlItem, func, {
            ChartObjectFieldIdStr_enabled,
            ChartObjectFieldIdStr_panelId,
            ChartObjectFieldIdStr_title,
            ChartObjectFieldIdStr_xLabel,
            ChartObjectFieldIdStr_yLabel,
            ChartObjectFieldIdStr_y2Label,
            ChartObjectFieldIdStr_xTickLabelDirection,
            ChartObjectFieldIdStr_yTickLabelDirection,
            ChartObjectFieldIdStr_axisLabelColour,
            ChartObjectFieldIdStr_axisProperties
            });
    }
    else if (isType(ChartObjectChartTypeStr_globalConfig))
    {
        checkForUnrecongnizedProperties(controlItem, func, {
            ChartObjectFieldIdStr_enabled,
            ChartObjectFieldIdStr_logLevel,
            ChartObjectFieldIdStr_showLegend,
            ChartObjectFieldIdStr_autoAxisLabels,
            ChartObjectFieldIdStr_lineStyle,
            ChartObjectFieldIdStr_pointStyle,
            ChartObjectFieldIdStr_lineColour,
            ChartObjectFieldIdStr_fillColour,
            ChartObjectFieldIdStr_background
            });
    }
    else
    {
        func(SzPtrUtf8(ChartObjectFieldIdStr_type));
    }
}

} } // Module namespace

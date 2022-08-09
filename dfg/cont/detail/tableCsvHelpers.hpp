#pragma once

#include "../../CsvFormatDefinition.hpp"
#include "../../ReadOnlySzParam.hpp"
#include "../CsvConfig.hpp"
#include <limits>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont)
{
    namespace DFG_DETAIL_NS
    {
        template <class T> struct ReadStatBase
        {
            using SetterArgType = T;
            static T fromString(const StringViewC sv) { return ::DFG_MODULE_NS(str)::strTo<T>(sv); }
            static std::string toSetPropertyArg(const T val) { return ::DFG_MODULE_NS(str)::toStrC(val); }
        }; // struct ReadStatBase
        template <> struct ReadStatBase<StringAscii>
        {
            using SetterArgType = StringViewAscii;
            static StringAscii fromString(const StringViewC sv) { return StringAscii::fromRawString(sv.toString()); }
            static StringViewC toSetPropertyArg(const StringViewAscii sv) { return sv.asUntypedView(); }
        }; // struct ReadStatBase<StringAscii>

        template <> struct ReadStatBase<StringUtf8>
        {
            using SetterArgType = StringViewUtf8;
            static StringUtf8 fromString(const StringViewC sv) { return StringUtf8::fromRawString(sv.toString()); }
            static StringViewC toSetPropertyArg(const StringViewUtf8 sv) { return sv.asUntypedView(); }
        }; // struct ReadStatBase<StringUtf8>

        template <> struct ReadStatBase<CsvConfig>
        {
            using SetterArgType = const CsvConfig&;
            static CsvConfig fromString(const StringViewC sv) { return CsvConfig::fromUtf8(StringViewUtf8(TypedCharPtrUtf8R(sv.data()), sv.size())); }
            static std::string toSetPropertyArg(const CsvConfig& config) { return config.saveToMemory().rawStorage(); }
        }; // struct ReadStatBase<CsvConfig>
    } // DFG_DETAIL_NS

#define DFG_TEMP_DEFINE_TABLECSV_READSTAT(NAME, TYPE, DEFAULT_VALUE) \
    namespace TableCsvReadStat \
    { \
        struct NAME : public ::DFG_MODULE_NS(cont)::DFG_DETAIL_NS::ReadStatBase<TYPE> \
        { \
            using ReturnType = TYPE; \
            static StringViewC stringId() { return "TableCsvReadStat_"#NAME; } \
            static TYPE value() { return DEFAULT_VALUE; } \
        }; \
    }

    // Defining TableCsvReadStat-identifiers. Each identifier is a class inside namespace ::dfg::cont::TableCsvReadStat
    //                                 Name                 Type            Default value
    DFG_TEMP_DEFINE_TABLECSV_READSTAT(appenderType,         StringAscii,    StringAscii())
    DFG_TEMP_DEFINE_TABLECSV_READSTAT(errorInfo,            CsvConfig,      CsvConfig())
    DFG_TEMP_DEFINE_TABLECSV_READSTAT(streamType,           StringAscii,    StringAscii())
    DFG_TEMP_DEFINE_TABLECSV_READSTAT(threadCount,          uint32,         0)
    DFG_TEMP_DEFINE_TABLECSV_READSTAT(timeBlockMerge,       double,         std::numeric_limits<double>::quiet_NaN())
    DFG_TEMP_DEFINE_TABLECSV_READSTAT(timeBlockReads,       double,         std::numeric_limits<double>::quiet_NaN())
    DFG_TEMP_DEFINE_TABLECSV_READSTAT(timeTotal,            double,         std::numeric_limits<double>::quiet_NaN())

#undef DFG_TEMP_DEFINE_TABLECSV_READSTAT

    namespace DFG_DETAIL_NS
    {
        enum class TableCsvReadWriteOptionsPropertyId
        {
            readOpt_threadCount,               // Defines request for number of threads to use when reading, value 0 is interpreted as "autodetermine"
                                                //     Even if setting this to explicit value, TableCsv can choose to use fewer threads e.g. in case of small file.
                                                //     To force given thread count to be used even for small files, set threadReadBlockSizeMinimum to zero
            readOpt_threadBlockSizeMinimum,    // Defines minimum size (in bytes) of read block that thread should have. This is used to control that reading is 
                                                // distributed to multiple thread only if there is enough work to be done, e.g. to not spread reading of 1 kB file to 16 threads.
            lastPropertyId = readOpt_threadBlockSizeMinimum
        }; // enum PropertyId


    } // namespace DFG_DETAIL_NS

namespace TableCsvErrorInfoFields
{
    constexpr SzPtrUtf8R errorMsg("error_msg");
}

class TableCsvReadWriteOptions : public CsvFormatDefinition
{
public:
    using BaseClass = CsvFormatDefinition;

    using PropertyId = ::DFG_MODULE_NS(cont)::DFG_DETAIL_NS::TableCsvReadWriteOptionsPropertyId;

    template <PropertyId Id_T> using PropertyIntegralConstant = std::integral_constant<int, static_cast<int>(Id_T)>;

    static uint32     getDefaultValue(PropertyIntegralConstant<PropertyId::readOpt_threadCount>) { return uint32(1); }
    // Default value is 10 MB. In practice default should be dependent on runtime context (how fast processor etc.),
    // but simply hardcoding a value that is at least reasonable in some contexts; user should set a better value as needed.
    static uint64     getDefaultValue(PropertyIntegralConstant<PropertyId::readOpt_threadBlockSizeMinimum>) { return 10000000; }

    template <PropertyId Id_T> using IdType = decltype(getDefaultValue(PropertyIntegralConstant<Id_T>()));

    template <PropertyId Id_T> static IdType<Id_T> getDefaultValueT() { return getDefaultValue(PropertyIntegralConstant<Id_T>()); }

    using BaseClass::BaseClass;

    TableCsvReadWriteOptions() = default;
    TableCsvReadWriteOptions(const BaseClass& other) : BaseClass(other) {}
    TableCsvReadWriteOptions(BaseClass&& other) : BaseClass(std::move(other)) {}

    // Returns string identifier of given id. Note that string identifiers are implementation details
    // to may be subject to change.
    static inline StringViewAscii privPropertyIdAsString(PropertyId id);

    template <PropertyId Id_T>
    void setPropertyT(const IdType<Id_T> prop);

    template <PropertyId Id_T>
    bool hasPropertyT() const;

    template <PropertyId Id_T>
    static auto getPropertyT(const BaseClass& base, const IdType<Id_T> defaultValue) -> IdType<Id_T>
    {
        return base.getPropertyThroughStrTo(privPropertyIdAsString(Id_T), defaultValue);
    }

    template <PropertyId Id_T>
    auto getPropertyT(const IdType<Id_T> defaultValue) const -> IdType<Id_T>
    {
        return getPropertyT<Id_T>(*this, defaultValue);
    }

    template <class ReadStat_T>
    void setReadStat(typename ReadStat_T::SetterArgType val)
    {
        BaseClass::setProperty(ReadStat_T::stringId(), ReadStat_T::toSetPropertyArg(val));
    }

    template <class ReadStat_T>
    auto getReadStat() const -> typename ReadStat_T::ReturnType
    {
        return ReadStat_T::fromString(BaseClass::getProperty(ReadStat_T::stringId(), std::string()));
    }
}; // class TableCsvReadWriteOptions

StringViewAscii TableCsvReadWriteOptions::privPropertyIdAsString(const PropertyId id)
{
    DFG_STATIC_ASSERT(static_cast<int>(PropertyId::lastPropertyId) == 1, "PropertyId count has changed, privPropertyIdAsString() needs to be updated");
    switch (id)
    {
        case PropertyId::readOpt_threadCount:              return SzPtrAscii("TableCsvRwo_threadCount");
        case PropertyId::readOpt_threadBlockSizeMinimum:   return SzPtrAscii("TableCsvRwo_threadBlockSizeMinimum");
        default: DFG_ASSERT_CORRECTNESS(false);            return DFG_ASCII("");
    }
}

template <TableCsvReadWriteOptions::PropertyId Id_T>
void TableCsvReadWriteOptions::setPropertyT(const IdType<Id_T> prop)
{
    BaseClass::setProperty(privPropertyIdAsString(Id_T), ::DFG_MODULE_NS(str)::toStrC(prop));
}

template <TableCsvReadWriteOptions::PropertyId Id_T>
bool TableCsvReadWriteOptions::hasPropertyT() const
{
    return BaseClass::hasProperty(privPropertyIdAsString(Id_T));
}

}} // namespace dfg::cont

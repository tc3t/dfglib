#pragma once

#include "../../CsvFormatDefinition.hpp"
#include "../../ReadOnlySzParam.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont)
{

class TableCsvReadWriteOptions : public CsvFormatDefinition
{
public:
    using BaseClass = CsvFormatDefinition;

    enum class PropertyId
    {
        threadCount,                        // In read parameter: defines request for number of threads to use when reading, value 0 is interpreted as "autodetermine"
                                            //                    Even if setting this to explicit value, TableCsv can choose to use fewer threads e.g. in case of small file.
                                            //                    To force given thread count to be used even for small files, set threadReadBlockSizeMinimum to zero
                                            // In object returned by TableCsv::readFormat(): If reading was done with multiple threads, the number of threads used, in single-threaded read either 0 or 1

        threadReadBlockSizeMinimum          // Defines minimum size (in bytes) of read block that thread should have. This is used to control that reading is 
                                            // distributed to multiple thread only if there is enough work to be done, e.g. to not spread reading of 1 kB file to 16 threads.
    };

    static int getDefaultValue(std::integral_constant<int, static_cast<int>(PropertyId::threadCount)>) { return 1; }
    // Default value is 10 MB. In practice default should be dependent on runtime context (how fast processor etc.),
    // but simply hardcoding a value that is at least reasonable in some contexts; user should set a better value as needed.
    static uint64 getDefaultValue(std::integral_constant<int, static_cast<int>(PropertyId::threadReadBlockSizeMinimum)>) { return 10000000; }

    template <PropertyId Id_T> using IdType = decltype(getDefaultValue(std::integral_constant<int, static_cast<int>(Id_T)>()));

    using BaseClass::BaseClass;

    TableCsvReadWriteOptions() = default;
    TableCsvReadWriteOptions(const BaseClass& other) : BaseClass(other) {}
    TableCsvReadWriteOptions(const BaseClass&& other) : BaseClass(std::move(other)) {}

    static inline StringViewAscii propertyIdAsString(PropertyId id);

    template <PropertyId Id_T>
    void setPropertyT(const IdType<Id_T> prop);

    template <PropertyId Id_T>
    static auto getPropertyT(const BaseClass& base, const IdType<Id_T> defaultValue) -> IdType<Id_T>
    {
        return base.getPropertyThroughStrTo(propertyIdAsString(Id_T), defaultValue);
    }

    template <PropertyId Id_T>
    auto getPropertyT(const IdType<Id_T> defaultValue) const -> IdType<Id_T>
    {
        return getPropertyT<Id_T>(*this, defaultValue);
    }
}; // class TableCsvReadWriteOptions

StringViewAscii TableCsvReadWriteOptions::propertyIdAsString(const PropertyId id)
{
    switch (id)
    {
        case PropertyId::threadCount:                   return SzPtrAscii("TableCsvRwo_threadCount");
        case PropertyId::threadReadBlockSizeMinimum:    return SzPtrAscii("TableCsvRwo_threadReadBlockSizeMinimum");
        default: DFG_ASSERT_CORRECTNESS(false);         return DFG_ASCII("");
    }
}

template <TableCsvReadWriteOptions::PropertyId Id_T>
void TableCsvReadWriteOptions::setPropertyT(const IdType<Id_T> prop)
{
    BaseClass::setProperty(propertyIdAsString(Id_T), ::DFG_MODULE_NS(str)::toStrC(prop));
}

}} // namespace dfg::cont

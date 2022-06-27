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
        threadCount         // In read parameter: defines request for number of threads to use when reading.
                            // In object returned by TableCsv::readFormat(): If reading was done with multiple threads, the number of threads used.
    };

    static int getDefaultValue(std::integral_constant<int, static_cast<int>(PropertyId::threadCount)>) { return 1; }

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
    case PropertyId::threadCount: return SzPtrAscii("TableCsvRwo_threadCount");
    default: DFG_ASSERT_CORRECTNESS(false); return DFG_ASCII("");
    }
}

template <TableCsvReadWriteOptions::PropertyId Id_T>
void TableCsvReadWriteOptions::setPropertyT(const IdType<Id_T> prop)
{
    BaseClass::setProperty(propertyIdAsString(Id_T), ::DFG_MODULE_NS(str)::toStrC(prop));
}

}} // namespace dfg::cont

#pragma once

#include "QtApplication.hpp"
#include <QSettings>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

constexpr char gPropertyIdAllowAppSettingsUsage[] = "dfglib_allow_app_settings_usage";

#define DFG_QT_OBJECT_PROPERTY_CLASS_NAME(CLASS) CLASS##PropertyDefinition

#define DFG_QT_DEFINE_OBJECT_PROPERTY_CLASS(CLASS) \
template <int ID_T> struct DFG_QT_OBJECT_PROPERTY_CLASS_NAME(CLASS) {};

// If comma-separated string such as "a,b" is read from ini-file through QSettings,
// it's variant type may be QStringList and direct conversion to QString would fail.
// This function returns QString from QVariant and in case of QStringList,
// returns comma-joined QString from the list.
inline QString qStringFromVariantWithQStringListHandling(const QVariant& v)
{
    if (v.type() == QVariant::StringList)
        return v.value<QStringList>().join(',');
    else
        return v.value<QString>();
}

#define DFG_QT_DEFINE_OBJECT_PROPERTY_CUSTOM_TYPE(STR_ID, CLASS, ID, RV_TYPE, DEFAULT_FUNC, FROM_VARIANT_FUNC) \
template <> struct CLASS##PropertyDefinition<static_cast<int>(ID)> \
{ \
    typedef RV_TYPE PropertyType; \
    static PropertyType getDefault()                    { return DEFAULT_FUNC(); } \
    static const char* getStrId()                       { return STR_ID; } \
    static PropertyType fromVariant(const QVariant& v)  { return FROM_VARIANT_FUNC(v); } \
    static PropertyType defaultFromVariant(const QVariant& v) { return v.template value<PropertyType>(); } \
}

#define DFG_QT_DEFINE_OBJECT_PROPERTY(STR_ID, CLASS, ID, RV_TYPE, DEFAULT_FUNC) \
    DFG_QT_DEFINE_OBJECT_PROPERTY_CUSTOM_TYPE(STR_ID, CLASS, ID, RV_TYPE, DEFAULT_FUNC, defaultFromVariant)

// Defines QString property that handles QStringList to QString conversion.
#define DFG_QT_DEFINE_OBJECT_PROPERTY_QSTRING(STR_ID, CLASS, ID, RV_TYPE, DEFAULT_FUNC) \
    DFG_QT_DEFINE_OBJECT_PROPERTY_CUSTOM_TYPE(STR_ID, CLASS, ID, RV_TYPE, DEFAULT_FUNC, ::DFG_MODULE_NS(qt)::qStringFromVariantWithQStringListHandling)

// Returns object property. First tries object's internal property map, then from application settings.
// Note: The way how internal properties are stored (current in properties of 'obj') is an implementation detail that may change.
template <class PropertyClass_T, class Object_T>
auto getProperty(Object_T* pObj) -> typename PropertyClass_T::PropertyType
{
    typedef PropertyClass_T PropDef;
    if (!pObj)
        return PropDef::getDefault();

    const char* id = PropDef::getStrId();
    auto internalVal = pObj->property(id);
    if (internalVal.isValid())
        return PropDef::fromVariant(internalVal);

    // Checking for "allow app setting usage"-flag from this and from whole parent chain until flag is found.
    bool bAllowAppSettingUsage = false;
    for (const QObject* pSearch = pObj; pSearch != nullptr; pSearch = pSearch->parent())
    {
        const auto prop = pSearch->property(gPropertyIdAllowAppSettingsUsage);
        if (!prop.isNull())
        {
            bAllowAppSettingUsage = prop.toBool();
            break;
        }
    }

    if (!bAllowAppSettingUsage)
        return PropDef::getDefault();

    auto settings = QtApplication::getApplicationSettings();
    return (settings) ?
        PropDef::fromVariant(settings->value(QString("dfglib/%1").arg(PropDef::getStrId()), QVariant::fromValue(PropDef::getDefault()))) :
        PropDef::getDefault();
}

template <class PropertyClass_T, class Object_T>
void setProperty(Object_T* pObj, QVariant newVal)
{
    typedef PropertyClass_T PropDef;
    if (!pObj)
        return;
    if (pObj->property(gPropertyIdAllowAppSettingsUsage).toBool())
    {
        auto settings = DFG_CLASS_NAME(QtApplication)::getApplicationSettings();
        if (settings)
        {
            settings->setValue(QString("dfglib/%1").arg(PropDef::getStrId()), newVal);
            return;
        }
    }
    pObj->setProperty(PropDef::getStrId(), newVal);
}

}} // Module namespace

#pragma once

#include "QtApplication.hpp"
#include <QSettings>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

#define DFG_QT_OBJECT_PROPERTY_CLASS_NAME(CLASS) CLASS##PropertyDefinition

#define DFG_QT_DEFINE_OBJECT_PROPERTY_CLASS(CLASS) \
template <int ID_T> struct DFG_QT_OBJECT_PROPERTY_CLASS_NAME(CLASS) {};

#define DFG_QT_DEFINE_OBJECT_PROPERTY_CUSTOM_TYPE(STR_ID, CLASS, ID, RV_TYPE, DEFAULT_FUNC, FROM_VARIANT_FUNC) \
template <> struct CLASS##PropertyDefinition<ID> \
{ \
    typedef RV_TYPE PropertyType; \
    static PropertyType getDefault()                    { return DEFAULT_FUNC(); } \
    static const char* getStrId()                       { return STR_ID; } \
    static PropertyType fromVariant(const QVariant& v)  { return FROM_VARIANT_FUNC(v); } \
    static PropertyType defaultFromVariant(const QVariant& v) { return v.template value<PropertyType>(); } \
}

#define DFG_QT_DEFINE_OBJECT_PROPERTY(STR_ID, CLASS, ID, RV_TYPE, DEFAULT_FUNC) \
    DFG_QT_DEFINE_OBJECT_PROPERTY_CUSTOM_TYPE(STR_ID, CLASS, ID, RV_TYPE, DEFAULT_FUNC, defaultFromVariant)

template <class PropertyClass_T, class Object_T>
auto getProperty(Object_T* obj) -> typename PropertyClass_T::PropertyType
{
    typedef PropertyClass_T PropDef;
    if (!obj || !obj->property("dfglib_allow_app_settings_usage").toBool())
        return PropDef::getDefault();
    auto settings = DFG_CLASS_NAME(QtApplication)::getApplicationSettings();
    return (settings) ? PropDef::fromVariant(settings->value(QString("dfglib/%1").arg(PropDef::getStrId()))) :  PropDef::getDefault();
}

}} // Module namespace

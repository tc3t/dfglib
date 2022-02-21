#pragma once

#include "../dfgDefs.hpp"
#include "../../../str/string.hpp"
#include "../../../OpaquePtr.hpp"
#include "../../qtIncludeHelpers.hpp"
#include <limits>
#include <vector>

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QString>
DFG_END_INCLUDE_QT_HEADERS

class QAction;
class QCheckBox;
class QVariant;

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

namespace DFG_DETAIL_NS
{
    class FloatToStringParam;
} // namespace DFG_DETAIL_NS

// Baseclass of selection detail collectors
class SelectionDetailCollector
{
public:
    static const char s_propertyName_id[];
    static const char s_propertyName_type[];
    static const char s_propertyName_uiNameShort[];
    static const char s_propertyName_uiNameLong[];
    static const char s_propertyName_description[];
    static const char s_propertyName_resultPrecision[];

    using FloatToStringParam = DFG_DETAIL_NS::FloatToStringParam;

    SelectionDetailCollector(StringUtf8 sId);
    SelectionDetailCollector(const QString& sId);
    virtual ~SelectionDetailCollector();

    StringViewUtf8 id() const;

    void enable(bool b, bool bUpdateCheckBox = true);
    bool isEnabled() const;

    void setProperty(const QString& sKey, const QVariant value);
    QVariant getProperty(const QString& sKey, const QVariant defaultValue = QVariant()) const;
    bool deleteProperty(const QString& sKey); // Returns true iff property was deleted

    QString getUiName_long() const;
    QString getUiName_short() const;

    FloatToStringParam toStrParam(FloatToStringParam toStrParam) const;

    QCheckBox* createCheckBox(QMenu* pParent);
    QCheckBox* getCheckBoxPtr();
    QAction* getCheckBoxAction();

    void updateCheckBoxToolTip();

    bool isBuiltIn() const;

    double value() const { return (m_bNeedsUpdate) ? valueImpl() : std::numeric_limits<double>::quiet_NaN(); }
    void update(const double val) { if (m_bNeedsUpdate) updateImpl(val); }

    // Resets collector clearing memory from previous collection around.
    void reset() { if (m_bNeedsUpdate) resetImpl(); }

    QString exportDefinitionToJson(const bool bIncludeId = false, const bool bSingleLine = false) const;

private:
    virtual double valueImpl() const { return std::numeric_limits<double>::quiet_NaN(); }
    virtual void updateImpl(double val) { DFG_UNUSED(val); }
    virtual void resetImpl() {}

protected:
    bool m_bNeedsUpdate = false;
    DFG_OPAQUE_PTR_DECLARE();
}; // class SelectionDetailCollector

// Selection detail collector using formula parser.
class SelectionDetailCollector_formula : public SelectionDetailCollector
{
public:
    using BaseClass = SelectionDetailCollector;
    using FormulaParser = ::DFG_MODULE_NS(math)::FormulaParser;

    static const char s_propertyName_formula[];
    static const char s_propertyName_initialValue[];

    SelectionDetailCollector_formula(StringUtf8 sId, StringViewUtf8 svFormula, double initialValue);
    ~SelectionDetailCollector_formula();

private:
    double valueImpl() const override;
    void updateImpl(double val) override;
    void resetImpl() override;

    DFG_OPAQUE_PTR_DECLARE();
}; // Class SelectionDetailCollector_formula

// Selection detail collector for percentile.
class SelectionDetailCollector_percentile : public SelectionDetailCollector
{
public:
    using BaseClass = SelectionDetailCollector;

    static const char s_propertyName_percentage[];

    SelectionDetailCollector_percentile(StringUtf8 sId, double percentage);
    ~SelectionDetailCollector_percentile();

private:
    double valueImpl() const override;
    void updateImpl(double val) override;
    void resetImpl() override;

    DFG_OPAQUE_PTR_DECLARE();
}; // Class SelectionDetailCollector_formula

// Stores SelectionDetailCollectors
class SelectionDetailCollectorContainer : public std::vector<std::shared_ptr<SelectionDetailCollector>>
{
public:
    using BaseClass = std::vector<std::shared_ptr<SelectionDetailCollector>>;
    using Container = BaseClass;

    auto find(const StringViewUtf8& id)->SelectionDetailCollector*;
    auto find(const QString& id)->SelectionDetailCollector*;

    bool erase(const StringViewUtf8& id);

    void setEnableStatusForAll(bool b);

    void updateAll(const double val);

private:
    template <class Func_T>
    void forEachCollector(Func_T&& func);

    auto findIter(const StringViewUtf8& id)->Container::iterator;
}; // class SelectionDetailCollectorContainer

} } // Module dfg::qt

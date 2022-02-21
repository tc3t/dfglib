#include "SelectionDetailCollector.hpp"
#include "../../CsvTableView.hpp"

#include <atomic>

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt)
{

/////////////////////////////////////////////////////////////////////////////////////
// 
// SelectionDetailCollector
//
/////////////////////////////////////////////////////////////////////////////////////

DFG_OPAQUE_PTR_DEFINE(SelectionDetailCollector)
{
public:
    StringUtf8 m_id;
    std::atomic<bool> m_abEnabled{ true };
    QObjectStorage<QCheckBox> m_spCheckBox;
    QObjectStorage<QAction> m_spContextMenuAction;
    QVariantMap m_properties;
};

const char SelectionDetailCollector::s_propertyName_description[]     = "description";
const char SelectionDetailCollector::s_propertyName_id[]              = "id";
const char SelectionDetailCollector::s_propertyName_type[]            = "type";
const char SelectionDetailCollector::s_propertyName_uiNameShort[]     = "ui_name_short";
const char SelectionDetailCollector::s_propertyName_uiNameLong[]      = "ui_name_long";
const char SelectionDetailCollector::s_propertyName_resultPrecision[] = "result_precision";

SelectionDetailCollector::SelectionDetailCollector(StringUtf8 sId) { DFG_OPAQUE_REF().m_id = std::move(sId); }
SelectionDetailCollector::SelectionDetailCollector(const QString& sId) : SelectionDetailCollector(qStringToStringUtf8(sId)) {}
SelectionDetailCollector::~SelectionDetailCollector()
{
    auto pCheckBox = DFG_OPAQUE_REF().m_spCheckBox.release();
    if (pCheckBox)
        pCheckBox->deleteLater(); // Using deleteLater() as checkbox may be used when collector gets deleted.
    auto pContextMenuAction = DFG_OPAQUE_REF().m_spContextMenuAction.release();
    if (pContextMenuAction)
        pContextMenuAction->deleteLater();
}

auto SelectionDetailCollector::id() const -> StringViewUtf8
{
    auto pOpaq = DFG_OPAQUE_PTR();
    return (pOpaq) ? pOpaq->m_id : StringViewUtf8();
}

bool SelectionDetailCollector::isEnabled() const
{
    auto pOpaq = DFG_OPAQUE_PTR();
    return (pOpaq) ? pOpaq->m_abEnabled.load() : false;
}

void SelectionDetailCollector::enable(const bool b, const bool bUpdateCheckBox)
{
    DFG_OPAQUE_REF().m_abEnabled = b;
    if (bUpdateCheckBox && DFG_OPAQUE_REF().m_spCheckBox)
        DFG_OPAQUE_REF().m_spCheckBox->setChecked(b);
}

void SelectionDetailCollector::setProperty(const QString& sKey, const QVariant value)
{
    DFG_OPAQUE_REF().m_properties[sKey] = value;
}

QVariant SelectionDetailCollector::getProperty(const QString& sKey, const QVariant defaultValue) const
{
    auto pOpaq = DFG_OPAQUE_PTR();
    return (pOpaq) ? pOpaq->m_properties.value(sKey, defaultValue) : defaultValue;
}

bool SelectionDetailCollector::deleteProperty(const QString& sKey)
{
    return DFG_OPAQUE_REF().m_properties.remove(sKey) != 0;
}

QString SelectionDetailCollector::getUiName_long() const
{
    auto s = getProperty("ui_name_long").toString();
    if (s.isEmpty())
        s = getUiName_short();
    if (s.isEmpty())
        s = viewToQString(id());
    return s;
}

QString SelectionDetailCollector::getUiName_short() const
{
    auto s = getProperty("ui_name_short").toString();
    if (s.isEmpty())
        s = viewToQString(id());
    return s;
}

auto SelectionDetailCollector::toStrParam(const FloatToStringParam toStrParam) const -> FloatToStringParam
{
    return FloatToStringParam(getProperty(s_propertyName_resultPrecision, toStrParam.m_nPrecision).toInt());
}

QCheckBox* SelectionDetailCollector::createCheckBox(QMenu* pParent)
{
    DFG_OPAQUE_REF().m_spCheckBox.reset(new QCheckBox(pParent));
    return DFG_OPAQUE_REF().m_spCheckBox.get();
}

QCheckBox* SelectionDetailCollector::getCheckBoxPtr()
{
    return DFG_OPAQUE_REF().m_spCheckBox.get();
}

QAction* SelectionDetailCollector::getCheckBoxAction()
{
    if (!DFG_OPAQUE_REF().m_spContextMenuAction)
    {
        auto pAct = new QWidgetAction(nullptr);
        pAct->setDefaultWidget(this->getCheckBoxPtr());
        DFG_OPAQUE_REF().m_spContextMenuAction.reset(pAct);
    }
    return DFG_OPAQUE_REF().m_spContextMenuAction.get();
}

void SelectionDetailCollector::updateCheckBoxToolTip()
{
    auto pCheckBox = this->getCheckBoxPtr();
    if (!pCheckBox)
        return;
    auto sDescription = this->getProperty(SelectionDetailCollector::s_propertyName_description).toString();
    auto sType = this->getProperty(SelectionDetailCollector::s_propertyName_type).toString();
    if (this->isBuiltIn())
        sType = pCheckBox->tr("Built-in");
    auto sFormula = this->getProperty(SelectionDetailCollector_formula::s_propertyName_formula).toString().toHtmlEscaped();
    auto sInitialValue = this->getProperty(SelectionDetailCollector_formula::s_propertyName_initialValue).toString().toHtmlEscaped();
    auto sPrecision = this->getProperty(SelectionDetailCollector_formula::s_propertyName_resultPrecision).toString().toHtmlEscaped();
    if (!sFormula.isEmpty())
    {
        sFormula = pCheckBox->tr("<li>Formula: %1</li>").arg(sFormula);
        sInitialValue = pCheckBox->tr("<li>Initial value: %1</li>").arg(sInitialValue);
    }
    if (!sDescription.isEmpty())
        sDescription = pCheckBox->tr("<li>Description: %1</li>").arg(sDescription.toHtmlEscaped());
    if (!sPrecision.isEmpty())
        sPrecision = pCheckBox->tr("<li>Result precision: %1</li>").arg(sPrecision);
    const QString sToolTip = pCheckBox->tr("<ul><li>Type: %1</li><li>Short name: %2</li>%3%4%5%6</ul>")
        .arg(sType.toHtmlEscaped(), this->getUiName_short().toHtmlEscaped(), sFormula, sInitialValue, sDescription, sPrecision);
    pCheckBox->setToolTip(sToolTip);
}

bool SelectionDetailCollector::isBuiltIn() const
{
    return getProperty("type").toString().isEmpty();
}

QString SelectionDetailCollector::exportDefinitionToJson(const bool bIncludeId, const bool bSingleLine) const
{
    auto pOpaq = DFG_OPAQUE_PTR();
    if (!pOpaq)
        return QString();
    const QJsonDocument::JsonFormat format = (bSingleLine) ? QJsonDocument::Compact : QJsonDocument::Indented;
    if (isBuiltIn())
        return QString(R"({ "id":"%1"%2 })").arg(viewToQString(id()), isEnabled() ? "" : R"(", "enabled":"0")");
    else
    {
        auto m = pOpaq->m_properties;
        if (bIncludeId)
            m[s_propertyName_id] = viewToQString(id());
        return QString::fromUtf8(QJsonDocument::fromVariant(m).toJson(format));
    }
}

/////////////////////////////////////////////////////////////////////////////////////
// 
// SelectionDetailCollector_formula
//
/////////////////////////////////////////////////////////////////////////////////////

DFG_OPAQUE_PTR_DEFINE(SelectionDetailCollector_formula)
{
public:
    FormulaParser m_formulaParser;
    double m_initialValue;
    double m_accValue;
    double m_cellValue = std::numeric_limits<double>::quiet_NaN();
}; // Opaque class of SelectionDetailCollector_formula

const char SelectionDetailCollector_formula::s_propertyName_formula[]      = "formula";
const char SelectionDetailCollector_formula::s_propertyName_initialValue[] = "initial_value";

SelectionDetailCollector_formula::SelectionDetailCollector_formula(StringUtf8 sId, StringViewUtf8 svFormula, double initialValue)
    : BaseClass(std::move(sId))
{
    DFG_OPAQUE_REF().m_initialValue = initialValue;
    DFG_OPAQUE_REF().m_accValue = initialValue;
    this->m_bNeedsUpdate = true;
    auto& rFormulaParser = DFG_OPAQUE_REF().m_formulaParser;
    rFormulaParser.defineVariable("acc", &DFG_OPAQUE_REF().m_accValue);
    rFormulaParser.defineVariable("value", &DFG_OPAQUE_REF().m_cellValue);
    rFormulaParser.setFormula(svFormula);
}

SelectionDetailCollector_formula::~SelectionDetailCollector_formula() = default;

double SelectionDetailCollector_formula::valueImpl() const
{
    auto pOpaq = DFG_OPAQUE_PTR();
    return (pOpaq) ? pOpaq->m_accValue : std::numeric_limits<double>::quiet_NaN();
}

void SelectionDetailCollector_formula::updateImpl(const double val)
{
    DFG_OPAQUE_REF().m_cellValue = val;
    DFG_OPAQUE_REF().m_accValue = DFG_OPAQUE_REF().m_formulaParser.evaluateFormulaAsDouble();
}

void SelectionDetailCollector_formula::resetImpl()
{
    DFG_OPAQUE_REF().m_accValue = DFG_OPAQUE_REF().m_initialValue;
    DFG_OPAQUE_REF().m_cellValue = std::numeric_limits<double>::quiet_NaN();
}

/////////////////////////////////////////////////////////////////////////////////////
// 
// SelectionDetailCollector_percentile
//
/////////////////////////////////////////////////////////////////////////////////////

DFG_OPAQUE_PTR_DEFINE(SelectionDetailCollector_percentile)
{
public:
    ::DFG_MODULE_NS(func)::MemFuncPercentile_enclosingElem<double> m_memFunc{ 0 };
}; // Opaque class of SelectionDetailCollector_percentile

const char SelectionDetailCollector_percentile::s_propertyName_percentage[] = "percentage";

SelectionDetailCollector_percentile::SelectionDetailCollector_percentile(StringUtf8 sId, const double percentage)
    : BaseClass(std::move(sId))
{
    DFG_OPAQUE_REF().m_memFunc.m_ratiotile = percentage / 100;
    this->m_bNeedsUpdate = true;
}

SelectionDetailCollector_percentile::~SelectionDetailCollector_percentile() = default;

double SelectionDetailCollector_percentile::valueImpl() const
{
    auto pOpaq = DFG_OPAQUE_PTR();
    return (pOpaq) ? pOpaq->m_memFunc.percentile() : std::numeric_limits<double>::quiet_NaN();
}

void SelectionDetailCollector_percentile::updateImpl(const double val)
{
    DFG_OPAQUE_REF().m_memFunc(val);
}

void SelectionDetailCollector_percentile::resetImpl()
{
    DFG_OPAQUE_REF().m_memFunc.clear();
}

/////////////////////////////////////////////////////////////////////////////////////
// 
// SelectionDetailCollectorContainer
//
/////////////////////////////////////////////////////////////////////////////////////

auto SelectionDetailCollectorContainer::find(const StringViewUtf8 & id) -> SelectionDetailCollector*
{
    auto iter = findIter(id);
    return (iter != this->end()) ? iter->get() : nullptr;
}

auto SelectionDetailCollectorContainer::find(const QString & id) -> SelectionDetailCollector*
{
    return find(StringViewUtf8(SzPtrUtf8(id.toUtf8())));
}

auto SelectionDetailCollectorContainer::findIter(const StringViewUtf8 & id) -> Container::iterator
{
    return std::find_if(this->begin(), this->end(), [&](const std::shared_ptr<SelectionDetailCollector>& sp)
        {
            return sp && sp->id() == id;
        });
}

bool SelectionDetailCollectorContainer::erase(const StringViewUtf8 & id)
{
    auto iter = findIter(id);
    if (iter == this->end())
        return false;
    BaseClass::erase(iter);
    return true;
}

void SelectionDetailCollectorContainer::updateAll(const double val)
{
    forEachCollector([=](SelectionDetailCollector& collector)
        {
            if (collector.isEnabled())
                collector.update(val);
        });
}

void SelectionDetailCollectorContainer::setEnableStatusForAll(const bool b)
{
    forEachCollector([=](SelectionDetailCollector& collector)
        {
            collector.enable(b);
        });
}

template <class Func_T>
void SelectionDetailCollectorContainer::forEachCollector(Func_T && func)
{
    for (auto& sp : *this)
    {
        if (!sp)
            continue;
        func(*sp);
    }
}

} } // Module dfg::qt

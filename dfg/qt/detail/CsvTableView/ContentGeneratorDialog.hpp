#pragma once

#include "../../../dfgDefs.hpp"
#include "../../qtIncludeHelpers.hpp"
#include "../../containerUtils.hpp"
#include <map>
#include <memory>
#include <vector>

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QDialog>
    #include <QString>
    #include <QStringList>
    #include <QStyledItemDelegate>
    #include <QVector>
DFG_END_INCLUDE_QT_HEADERS

class QGridLayout;
class QLabel;
class QModelIndex;
class QVBoxLayout;

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

class CsvItemModel;

class ContentGeneratorDialog : public QDialog
{
public:
    typedef QDialog BaseClass;

    // Note: ID's should match values in arrPropDefs-array.
    enum PropertyId
    {
        PropertyIdInvalid = -1,
        PropertyIdTarget,
        PropertyIdGenerator,
        LastNonParamPropertyId = PropertyIdGenerator
    };

    enum GeneratorType
    {
        GeneratorTypeUnknown,
        GeneratorTypeRandomIntegers,
        GeneratorTypeRandomDoubles,
        GeneratorTypeFill,
        GeneratorTypeFormula,
        GeneratorType_last = GeneratorTypeFormula
    };

    enum TargetType
    {
        TargetTypeUnknown,
        TargetTypeSelection,
        TargetTypeWholeTable,
        TargetType_last = TargetTypeWholeTable
    };

    enum ValueType
    {
        ValueTypeKeyList,
        ValueTypeInteger,
        ValueTypeUInteger = ValueTypeInteger,
        ValueTypeDouble,
        ValueTypeString,
        ValueTypeCsvList,
    };

    struct PropertyDefinition
    {
        const char* m_pszName;
        int m_nType;
        const char* m_keyList;
        const char* m_pszDefault;
        const char** m_pCompleterItems; // Array of strings defining the list of completer items, must end with nullptr item.
    };

    static const char* integerDistributionCompleters[];
    static const char* realDistributionCompleters[];
    static const char szGenerators[];
    static const PropertyDefinition arrPropDefs[];

    ContentGeneratorDialog(QWidget* pParent);

    void setGenerateFailed(bool bFailed, const QString& sFailReason = QString());

    int propertyIdToRow(const PropertyId propId) const;

    std::vector<std::reference_wrapper<const PropertyDefinition>> generatorParameters(const int itemIndex);

    void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>&/* roles*/);

    GeneratorType generatorType() const;

    void setCompleter(const int nTargetRow, const char** pCompleterItems);

    void updateDynamicHelp();

    void createPropertyParams(const PropertyId prop, const int itemIndex);

    void setLatestComboBoxItemIndex(int index);

    PropertyId rowToPropertyId(int r) const;
    static GeneratorType generatorType(const CsvItemModel& csvModel);
    static TargetType targetType(const CsvItemModel& csvModel);

    QVBoxLayout* m_pLayout;
    QGridLayout* m_pGeneratorControlsLayout;
    std::unique_ptr<CsvTableView> m_spSettingsTable;
    std::unique_ptr<CsvItemModel> m_spSettingsModel;
    QObjectStorage<QLabel> m_spDynamicHelpWidget;
    QObjectStorage<QLabel> m_spGenerateFailedNoteWidget;
    int m_nLatestComboBoxItemIndex;
    QObjectStorage<CsvTableViewCompleterDelegate> m_spCompleterDelegateForDistributionParams;
    std::map<const void*, QObjectStorage<QCompleter>> m_completers; // Maps completer definition (e.g. integerDistributionCompleters) to completer item.
}; // Class ContentGeneratorDialog

class ComboBoxDelegate : public QStyledItemDelegate
{
    using PropertyId = ContentGeneratorDialog::PropertyId;
    using BaseClass = QStyledItemDelegate;

public:
    ComboBoxDelegate(ContentGeneratorDialog* parent);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    static PropertyId rowToPropertyId(int r);
    static QStringList valueListFromProperty(const PropertyId id);

public:
    ContentGeneratorDialog* m_pParentDialog;
}; // class ComboBoxDelegate

} } // Module dfg::qt

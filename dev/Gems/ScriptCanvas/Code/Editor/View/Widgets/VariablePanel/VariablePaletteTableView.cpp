/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <precompiled.h>

#include <qaction.h>
#include <qevent.h>
#include <qheaderview.h>
#include <qitemselectionmodel.h>
#include <qscrollbar.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/UserSettings/UserSettings.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <GraphCanvas/Types/TranslationTypes.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/View/Widgets/VariablePanel/VariablePaletteTableView.h>

#include <Editor/Settings.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/QtMetaTypes.h>

#include <ScriptCanvas/Data/DataRegistry.h>

namespace ScriptCanvasEditor
{
    /////////////////////////
    // VariablePaletteModel
    /////////////////////////
    VariablePaletteModel::VariablePaletteModel(QObject* parent)
        : QAbstractTableModel(parent)
        , m_pinIcon(":/ScriptCanvasEditorResources/Resources/pin.png")
    {
    }

    int VariablePaletteModel::columnCount(const QModelIndex&) const
    {
        return ColumnIndex::Count;
    }

    int VariablePaletteModel::rowCount(const QModelIndex& parent) const
    {
        return aznumeric_cast<int>(m_variableTypes.size());
    }

    QVariant VariablePaletteModel::data(const QModelIndex &index, int role /*= Qt::DisplayRole*/) const
    {
        ScriptCanvas::Data::Type varType;

        if (index.row() < m_variableTypes.size())
        {
            varType = m_variableTypes[index.row()];
        }

        switch (role)
        {
        case VarTypeRole:
            return QVariant::fromValue<ScriptCanvas::Data::Type>(varType);

        case Qt::DisplayRole:
        {
            if (index.column() == ColumnIndex::Type)
            {
                AZStd::string typeString = TranslationHelper::GetSafeTypeName(varType);
                return QString(typeString.c_str());
            }
        }
        break;

        case Qt::DecorationRole:
        {
            if (index.column() == ColumnIndex::Type)
            {
                const QPixmap* icon = nullptr;
                GraphCanvas::StyleManagerRequestBus::EventResult(icon, ScriptCanvasEditor::AssetEditorId, &GraphCanvas::StyleManagerRequests::GetDataTypeIcon, ScriptCanvas::Data::ToAZType(varType));

                if (icon)
                {
                    return *icon;
                }
            }
            else if (index.column() == ColumnIndex::Pinned)
            {
                AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);

                bool showPin = settings->m_pinnedDataTypes.find(ScriptCanvas::Data::ToAZType(varType)) != settings->m_pinnedDataTypes.end();

                if (m_pinningChanges.find(ScriptCanvas::Data::ToAZType(varType)) != m_pinningChanges.end())
                {
                    showPin = !showPin;
                }

                if (showPin)
                {
                    return m_pinIcon;
                }
            }
        }
        break;

        case Qt::EditRole:
        {
            if (index.column() == ColumnIndex::Type)
            {
                AZStd::string typeString = TranslationHelper::GetSafeTypeName(varType);
                return QString(typeString.c_str());
            }
        }
        break;

        default:
            break;
        }

        return QVariant();
    }

    Qt::ItemFlags VariablePaletteModel::flags(const QModelIndex&) const
    {
        return Qt::ItemFlags(
            Qt::ItemIsEnabled |
            Qt::ItemIsSelectable);
    }

    QItemSelectionRange VariablePaletteModel::GetSelectionRangeForRow(int row)
    {
        return QItemSelectionRange(createIndex(row, 0, nullptr), createIndex(row, columnCount(), nullptr));
    }

    void VariablePaletteModel::PopulateVariablePalette(const AZStd::unordered_set< AZ::Uuid >& objectTypes)
    {
        layoutAboutToBeChanged();

        m_variableTypes.clear();
        m_typeNameMapping.clear();

        auto dataRegistry = ScriptCanvas::GetDataRegistry();

        m_variableTypes.reserve((dataRegistry->m_typeIdTraitMap.size() - 1) + objectTypes.size());

        for (const auto& dataTraitsPair : dataRegistry->m_typeIdTraitMap)
        {
            if (dataTraitsPair.first == ScriptCanvas::Data::eType::BehaviorContextObject)
            {
                // Skip EntityID to avoid a duplicate
                // Skip BehaviorContextObject as it isn't a valid creatable type on its own
                continue;
            }

            ScriptCanvas::Data::Type type = dataTraitsPair.second.m_dataTraits.GetSCType();

            if (type.IsValid())
            {
                m_variableTypes.emplace_back(type);

                AZStd::string typeString = TranslationHelper::GetSafeTypeName(type);
                m_typeNameMapping[typeString] = type;
            }
        }

        for (const AZ::Uuid& objectId : objectTypes)
        {
            ScriptCanvas::Data::Type type = dataRegistry->m_typeIdTraitMap[ScriptCanvas::Data::eType::BehaviorContextObject].m_dataTraits.GetSCType(objectId);
            m_variableTypes.emplace_back(type);

            AZStd::string typeString = TranslationHelper::GetSafeTypeName(type);
            m_typeNameMapping[typeString] = type;
        }

        layoutChanged();
    }

    ScriptCanvas::Data::Type VariablePaletteModel::FindDataForIndex(const QModelIndex& index)
    {
        ScriptCanvas::Data::Type retVal;

        if (index.row() >= 0 && index.row() < m_variableTypes.size())
        {
            retVal = m_variableTypes[index.row()];
        }

        return retVal;
    }

    ScriptCanvas::Data::Type VariablePaletteModel::FindDataForTypeName(const AZStd::string& typeName)
    {
        ScriptCanvas::Data::Type retVal;

        auto mapIter = m_typeNameMapping.find(typeName);
        if (mapIter != m_typeNameMapping.end())
        {
            retVal = mapIter->second;
        }

        return retVal;
    }

    void VariablePaletteModel::TogglePendingPinChange(const AZ::Uuid& azVarType)
    {
        auto pinningIter = m_pinningChanges.find(azVarType);

        if (pinningIter != m_pinningChanges.end())
        {
            m_pinningChanges.erase(pinningIter);
        }
        else
        {
            m_pinningChanges.insert(azVarType);
        }
    }

    const AZStd::unordered_set< AZ::Uuid >& VariablePaletteModel::GetPendingPinChanges() const
    {
        return m_pinningChanges;
    }

    void VariablePaletteModel::SubmitPendingPinChanges()
    {
        AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);

        if (settings)
        {
            for (const AZ::Uuid& azVarType : m_pinningChanges)
            {
                size_t result = settings->m_pinnedDataTypes.erase(azVarType);

                if (result == 0)
                {
                    settings->m_pinnedDataTypes.insert(azVarType);
                }
            }

            m_pinningChanges.clear();
        }
    }

    ////////////////////////////////////////
    // VariablePaletteSortFilterProxyModel
    ////////////////////////////////////////

    VariablePaletteSortFilterProxyModel::VariablePaletteSortFilterProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {
    }

    bool VariablePaletteSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        if (m_filter.isEmpty())
        {
            return true;
        }
        
        VariablePaletteModel* model = qobject_cast<VariablePaletteModel*>(sourceModel());
        if (!model)
        {
            return false;
        }
        
        QModelIndex index = model->index(sourceRow, VariablePaletteModel::ColumnIndex::Type, sourceParent);
        QString test = model->data(index, Qt::DisplayRole).toString();
        return (test.lastIndexOf(m_testRegex) >= 0);
    }

    bool VariablePaletteSortFilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        VariablePaletteModel* model = qobject_cast<VariablePaletteModel*>(sourceModel());
        if (!model)
        {
            return false;
        }

        bool pinnedLeft = false;
        ScriptCanvas::Data::Type leftDataType = model->FindDataForIndex(left);

        bool pinnedRight = false;
        ScriptCanvas::Data::Type rightDataType = model->FindDataForIndex(right);

        AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);

        if (settings)
        {
            pinnedLeft = settings->m_pinnedDataTypes.find(ScriptCanvas::Data::ToAZType(leftDataType)) != settings->m_pinnedDataTypes.end();
            pinnedRight = settings->m_pinnedDataTypes.find(ScriptCanvas::Data::ToAZType(rightDataType)) != settings->m_pinnedDataTypes.end();
        }

        if (pinnedRight == pinnedLeft)
        {
            return QSortFilterProxyModel::lessThan(left, right);
        }
        else if (pinnedRight)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    
    void VariablePaletteSortFilterProxyModel::SetFilter(const QString& filter)
    {
        m_filter = filter;
        m_testRegex = QRegExp(m_filter, Qt::CaseInsensitive);
        invalidateFilter();
    }
    
    /////////////////////////////
    // VariablePaletteTableView
    /////////////////////////////
    
    VariablePaletteTableView::VariablePaletteTableView(QWidget* parent)
        : QTableView(parent)
    {
        m_model = aznew VariablePaletteModel(parent);
        m_proxyModel = aznew VariablePaletteSortFilterProxyModel(parent);
        
        m_proxyModel->setSourceModel(m_model);
        m_proxyModel->sort(VariablePaletteModel::ColumnIndex::Type);

        setModel(m_proxyModel);

        setItemDelegateForColumn(VariablePaletteModel::Type, aznew GraphCanvas::IconDecoratedNameDelegate(this));

        viewport()->installEventFilter(this);
        horizontalHeader()->setSectionResizeMode(VariablePaletteModel::ColumnIndex::Pinned, QHeaderView::ResizeMode::ResizeToContents);
        horizontalHeader()->setSectionResizeMode(VariablePaletteModel::ColumnIndex::Type, QHeaderView::ResizeMode::Stretch);

        QObject::connect(this, &QAbstractItemView::clicked, this, &VariablePaletteTableView::OnClicked);

        m_completer = new QCompleter();

        m_completer->setModel(m_model);
        m_completer->setCompletionColumn(VariablePaletteModel::ColumnIndex::Type);
        m_completer->setCompletionMode(QCompleter::CompletionMode::InlineCompletion);
        m_completer->setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);

        setMinimumSize(0,0);
    }

    VariablePaletteTableView::~VariablePaletteTableView()
    {
        m_model->SubmitPendingPinChanges();
    }

    void VariablePaletteTableView::PopulateVariablePalette(const AZStd::unordered_set< AZ::Uuid >& objectTypes)
    {
        clearSelection();
        m_model->PopulateVariablePalette(objectTypes);
    }

    void VariablePaletteTableView::SetFilter(const QString& filter)
    {
        m_model->SubmitPendingPinChanges();

        clearSelection();
        m_proxyModel->SetFilter(filter);
    }

    QCompleter* VariablePaletteTableView::GetVariableCompleter()
    {
        return m_completer;
    }

    void VariablePaletteTableView::TryCreateVariableByTypeName(const AZStd::string& typeName)
    {
        ScriptCanvas::Data::Type dataType = m_model->FindDataForTypeName(typeName);

        if (dataType.IsValid())
        {
            emit CreateVariable(dataType);
        }
    }

    void VariablePaletteTableView::hideEvent(QHideEvent* hideEvent)
    {
        m_model->SubmitPendingPinChanges();
        clearSelection();

        QTableView::hideEvent(hideEvent);
    }

    void VariablePaletteTableView::showEvent(QShowEvent* showEvent)
    {
        QTableView::showEvent(showEvent);

        clearSelection();
        scrollToTop();

        m_proxyModel->invalidate();
    }

    void VariablePaletteTableView::OnClicked(const QModelIndex& index)
    {
        ScriptCanvas::Data::Type varType = ScriptCanvas::Data::Type::Invalid();

        QModelIndex sourceIndex = m_proxyModel->mapToSource(index);

        if (sourceIndex.isValid())
        {
            varType = m_model->FindDataForIndex(sourceIndex);
        }

        if (varType.IsValid())
        {
            if (index.column() == VariablePaletteModel::ColumnIndex::Pinned)
            {
                AZ::Uuid azVarType = ScriptCanvas::Data::ToAZType(varType);
                m_model->TogglePendingPinChange(azVarType);
                m_model->dataChanged(sourceIndex, sourceIndex);
            }
            else
            {
                emit CreateVariable(varType);
            }
        }
    }

#include <Editor/View/Widgets/VariablePanel/VariablePaletteTableView.moc>
}
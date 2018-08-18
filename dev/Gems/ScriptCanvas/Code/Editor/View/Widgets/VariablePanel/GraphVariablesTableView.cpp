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
#include <qapplication.h>
#include <qheaderview.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>

#include <Editor/Include/ScriptCanvas/Bus/RequestBus.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/View/Widgets/VariablePanel/GraphVariablesTableView.h>

#include <Editor/Translation/TranslationHelper.h>
#include <Editor/QtMetaTypes.h>

#include <Editor/View/Widgets/ScriptCanvasNodePaletteDockWidget.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>
#include <ScriptCanvas/Bus/RequestBus.h>

namespace ScriptCanvasEditor
{
    ////////////////////////
    // GraphVariablesModel
    ////////////////////////

    GraphVariablesModel::GraphVariablesModel(QObject* parent /*= nullptr*/)
        : QAbstractTableModel(parent)
    {
    }

    GraphVariablesModel::~GraphVariablesModel()
    {
        ScriptCanvas::GraphVariableManagerNotificationBus::Handler::BusDisconnect();
    }

    int GraphVariablesModel::columnCount(const QModelIndex &parent) const
    {
        return ColumnIndex::Count;
    }

    int GraphVariablesModel::rowCount(const QModelIndex &parent) const
    {
        return aznumeric_cast<int>(m_variableIds.size());
    }

    QVariant GraphVariablesModel::data(const QModelIndex &index, int role) const
    {
        ScriptCanvas::VariableId varId = FindVariableIdForIndex(index);

        switch (role)
        {
        case VarIdRole:
            return QVariant::fromValue<ScriptCanvas::VariableId>(varId);

        case Qt::EditRole:
        {
            if (index.column() == ColumnIndex::Name)
            {
                AZStd::string_view title;

                ScriptCanvas::VariableRequestBus::EventResult(title, varId, &ScriptCanvas::VariableRequests::GetName);

                return QVariant(title.data());
            }
            else if (index.column() == ColumnIndex::DefaultValue)
            {
                ScriptCanvas::Data::Type varType;
                ScriptCanvas::VariableRequestBus::EventResult(varType, varId, &ScriptCanvas::VariableRequests::GetType);

                ScriptCanvas::VariableNameValuePair* varPair = nullptr;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(varPair, m_scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId);

                if (varPair && IsEditabletype(varType))
                {
                    if (varType.IS_A(ScriptCanvas::Data::Type::String()))
                    {
                        const ScriptCanvas::Data::StringType* stringValue = varPair->m_varDatum.GetData().GetAs<ScriptCanvas::Data::StringType>();
                        
                        return QVariant(stringValue->c_str());
                    }
                    else if (varType.IS_A(ScriptCanvas::Data::Type::Number()))
                    {
                        const ScriptCanvas::Data::NumberType* numberValue = varPair->m_varDatum.GetData().GetAs<ScriptCanvas::Data::NumberType>();

                        return QVariant((*numberValue));
                    }
                    else if (varType.IS_A(ScriptCanvas::Data::Type::Boolean()))
                    {
                        const ScriptCanvas::Data::BooleanType* booleanValue = varPair->m_varDatum.GetData().GetAs<ScriptCanvas::Data::BooleanType>();

                        return QVariant((*booleanValue));
                    }
                    else
                    {
                        AZ_Warning("ScriptCanvas", false, "Unhandled editable type found in GraphVariablesTableView.cpp");
                    }
                }
            }
        }
        break;

        case Qt::DisplayRole:
        {
            if (index.column() == ColumnIndex::Name)
            {
                AZStd::string_view title;

                ScriptCanvas::VariableRequestBus::EventResult(title, varId, &ScriptCanvas::VariableRequests::GetName);

                return QVariant(title.data());
            }
            else if (index.column() == ColumnIndex::Type)
            {
                ScriptCanvas::Data::Type varType;
                ScriptCanvas::VariableRequestBus::EventResult(varType, varId, &ScriptCanvas::VariableRequests::GetType);

                if (varType.IsValid())
                {
                    AZStd::string type = TranslationHelper::GetSafeTypeName(varType);

                    return QVariant(type.c_str());
                }

                return QVariant();
            }
            else if (index.column() == ColumnIndex::DefaultValue)
            {
                ScriptCanvas::Data::Type varType;
                ScriptCanvas::VariableRequestBus::EventResult(varType, varId, &ScriptCanvas::VariableRequests::GetType);

                ScriptCanvas::VariableNameValuePair* varPair = nullptr;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(varPair, m_scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId);

                if (varPair && IsEditabletype(varType))
                {
                    if (varType.IS_A(ScriptCanvas::Data::Type::String()))
                    {
                        const ScriptCanvas::Data::StringType* stringValue = varPair->m_varDatum.GetData().GetAs<ScriptCanvas::Data::StringType>();

                        if (stringValue->empty())
                        {
                            return QString("<None>");
                        }
                        else
                        {
                            return QVariant(stringValue->c_str());
                        }
                    }
                    else if (varType.IS_A(ScriptCanvas::Data::Type::Number()))
                    {
                        const ScriptCanvas::Data::NumberType* numberValue = varPair->m_varDatum.GetData().GetAs<ScriptCanvas::Data::NumberType>();

                        return QVariant((*numberValue));
                    }
                    else if (varType.IS_A(ScriptCanvas::Data::Type::Boolean()))
                    {
                        // Want to return nothing for the boolean, because we'll just use the check box
                        return QVariant();
                    }
                    else
                    {
                        AZ_Warning("ScriptCanvas", false, "Unhandled editable type found in GraphVariablesTableView.cpp");
                    }
                }

                return QVariant();
            }
        }
        break;

        case Qt::FontRole:
        {
            if (index.column() == ColumnIndex::DefaultValue)
            {
                ScriptCanvas::Data::Type varType;
                ScriptCanvas::VariableRequestBus::EventResult(varType, varId, &ScriptCanvas::VariableRequests::GetType);

                if (varType.IS_A(ScriptCanvas::Data::Type::String()))
                {
                    ScriptCanvas::VariableNameValuePair* varPair = nullptr;
                    ScriptCanvas::GraphVariableManagerRequestBus::EventResult(varPair, m_scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId);

                    if (varPair)
                    {
                        const ScriptCanvas::Data::StringType* stringValue = varPair->m_varDatum.GetData().GetAs<ScriptCanvas::Data::StringType>();

                        if (stringValue->empty())
                        {
                            QFont font;
                            font.setItalic(true);
                            return font;
                        }
                    }
                }
            }
        }
        break;
        case Qt::ToolTipRole:
        {
            ScriptCanvas::Data::Type varType;
            ScriptCanvas::VariableRequestBus::EventResult(varType, varId, &ScriptCanvas::VariableRequests::GetType);

            AZStd::string type;

            if (varType.IsValid())
            {
                type = TranslationHelper::GetSafeTypeName(varType);
            }

            if (index.column() == ColumnIndex::Type)
            {
                if (!type.empty())
                {
                    return QVariant(type.c_str());
                }
            }
            else
            {
                AZStd::string variableName;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(variableName, m_scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::GetVariableName, varId);

                QString tooltipString = QString("Drag to the canvas to Get or Set %1 (Shift+Drag to Get; Alt+Drag to Set)").arg(variableName.c_str());

                if (!type.empty())
                {
                    // Prefix the type if it is valid
                    return QString("%1 - %2").arg(type.c_str(), tooltipString);
                }

                return tooltipString;
            }
        }
        break;
        case Qt::DecorationRole:
        {
            if (index.column() == ColumnIndex::Name)
            {
                ScriptCanvas::VariableNameValuePair* varPair{};
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(varPair, m_scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId);

                if (varPair)
                {
                    AZ::Uuid type = ScriptCanvas::Data::ToAZType(varPair->m_varDatum.GetData().GetType());

                    const QPixmap* icon = nullptr;
                    GraphCanvas::StyleManagerRequestBus::EventResult(icon, ScriptCanvasEditor::AssetEditorId, &GraphCanvas::StyleManagerRequests::GetDataTypeIcon, type);

                    if (icon)
                    {
                        return *icon;
                    }
                }

                return QVariant();
            }
        }
        break;

        case Qt::CheckStateRole:
        {
            if (index.column() == ColumnIndex::DefaultValue)
            {
                ScriptCanvas::Data::Type varType;
                ScriptCanvas::VariableRequestBus::EventResult(varType, varId, &ScriptCanvas::VariableRequests::GetType);

                ScriptCanvas::VariableNameValuePair* varPair = nullptr;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(varPair, m_scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId);

                if (varPair && varType.IS_A(ScriptCanvas::Data::Type::Boolean()))
                {
                    const ScriptCanvas::Data::BooleanType* booleanType = varPair->m_varDatum.GetData().GetAs<ScriptCanvas::Data::BooleanType>();

                    if ((*booleanType))
                    {
                        return Qt::CheckState::Checked;
                    }
                    else
                    {
                        return Qt::CheckState::Unchecked;
                    }
                }
            }
        }
        break;

        case Qt::TextAlignmentRole:
        {
            if (index.column() == ColumnIndex::Type)
            {
                return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
            }
        }
        break;

        default:
            break;
        }

        return QVariant();
    }

    bool GraphVariablesModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        ScriptCanvas::VariableId varId = FindVariableIdForIndex(index);
        bool modifiedData = false;

        switch (role)
        {
        case Qt::CheckStateRole:
        {
            if (index.column() == ColumnIndex::DefaultValue)
            {
                ScriptCanvas::Data::Type varType;
                ScriptCanvas::VariableRequestBus::EventResult(varType, varId, &ScriptCanvas::VariableRequests::GetType);

                ScriptCanvas::VariableNameValuePair* varPair = nullptr;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(varPair, m_scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId);

                if (varType.IS_A(ScriptCanvas::Data::Type::Boolean()))
                {
                    modifiedData = varPair->m_varDatum.SetValueAs<ScriptCanvas::Data::BooleanType>(value.toBool());
                }
            }
        }
        break;
        case Qt::EditRole:
        {
            if (index.column() == ColumnIndex::Name)
            {
                AZ::Outcome<void, AZStd::string> outcome = AZ::Failure(AZStd::string());
                AZStd::string_view oldVariableName;
                ScriptCanvas::VariableRequestBus::EventResult(oldVariableName, varId, &ScriptCanvas::VariableRequests::GetName);
                AZStd::string newVariableName = value.toString().toUtf8().data();
                if (newVariableName != oldVariableName)
                {
                    ScriptCanvas::VariableRequestBus::EventResult(outcome, varId, &ScriptCanvas::VariableRequests::RenameVariable, newVariableName);

                    modifiedData = outcome.IsSuccess();
                }
            }
            else if (index.column() == ColumnIndex::DefaultValue)
            {
                ScriptCanvas::Data::Type varType;
                ScriptCanvas::VariableRequestBus::EventResult(varType, varId, &ScriptCanvas::VariableRequests::GetType);

                ScriptCanvas::VariableNameValuePair* varPair = nullptr;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(varPair, m_scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, varId);

                if (varPair && IsEditabletype(varType))
                {
                    if (varType.IS_A(ScriptCanvas::Data::Type::String()))
                    {
                        modifiedData = varPair->m_varDatum.SetValueAs<ScriptCanvas::Data::StringType>(ScriptCanvas::Data::StringType(value.toString().toUtf8().data()));

                    }
                    else if (varType.IS_A(ScriptCanvas::Data::Type::Number()))
                    {
                        modifiedData = varPair->m_varDatum.SetValueAs<ScriptCanvas::Data::NumberType>(value.toDouble());
                    }
                }
            }
        }
        break;
        default:
            break;
        }

        if (modifiedData)
        {
            GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, m_scriptCanvasGraphId);
        }

        return modifiedData;
    }

    Qt::ItemFlags GraphVariablesModel::flags(const QModelIndex &index) const
    {
        Qt::ItemFlags itemFlags = Qt::ItemFlags(Qt::ItemIsEnabled |
                                                Qt::ItemIsDragEnabled |
                                                Qt::ItemIsSelectable);

        if (index.column() == ColumnIndex::Name)
        {
            itemFlags |= Qt::ItemIsEditable;
        }
        else if (index.column() == ColumnIndex::DefaultValue)
        {
            ScriptCanvas::VariableId varId = FindVariableIdForIndex(index);

            ScriptCanvas::Data::Type varType;
            ScriptCanvas::VariableRequestBus::EventResult(varType, varId, &ScriptCanvas::VariableRequests::GetType);

            if (IsEditabletype(varType))
            {
                if (varType.IS_A(ScriptCanvas::Data::Type::Boolean()))
                {
                    itemFlags |= Qt::ItemIsUserCheckable;
                }
                else
                {
                    itemFlags |= Qt::ItemIsEditable;
                }
            }
        }

        return itemFlags;
    }

    QStringList GraphVariablesModel::mimeTypes() const
    {
        QStringList mimeTypes;
        mimeTypes.append(Widget::NodePaletteDockWidget::GetMimeType());
        return mimeTypes;
    }

    QMimeData* GraphVariablesModel::mimeData(const QModelIndexList &indexes) const
    {
        QMimeData* mimeData = nullptr;
        GraphCanvas::GraphCanvasMimeContainer container;

        bool isSet = ((QApplication::keyboardModifiers() & Qt::Modifier::ALT) != 0);
        bool isGet = ((QApplication::keyboardModifiers() & Qt::Modifier::SHIFT) != 0);

        for (QModelIndex modelIndex : indexes)
        {
            // We select by the row, but each row still has multiple columns.
            // So to avoid handling the same row more then once, we only handle column 0.
            if (modelIndex.column() != 0)
            {
                continue;
            }

            ScriptCanvas::VariableId variableId = FindVariableIdForIndex(modelIndex);

            GraphCanvas::GraphCanvasMimeEvent* mimeEvent = nullptr;

            if (isSet)
            {
                mimeEvent = aznew CreateSetVariableNodeMimeEvent(variableId);
            }
            else if (isGet)
            {
                mimeEvent = aznew CreateGetVariableNodeMimeEvent(variableId);
            }
            else
            {
                mimeEvent = aznew CreateGetOrSetVariableNodeMimeEvent(variableId);
            }

            if (mimeEvent)
            {
                container.m_mimeEvents.push_back(mimeEvent);
            }
        }

        if (container.m_mimeEvents.empty())
        {
            return nullptr;
        }

        AZStd::vector<char> encoded;
        if (!container.ToBuffer(encoded))
        {
            return nullptr;
        }

        QMimeData* mimeDataPtr = new QMimeData();
        QByteArray encodedData;
        encodedData.resize((int)encoded.size());
        memcpy(encodedData.data(), encoded.data(), encoded.size());
        mimeDataPtr->setData(Widget::NodePaletteDockWidget::GetMimeType(), encodedData);

        return mimeDataPtr;
    }
    
    void GraphVariablesModel::SetActiveScene(const AZ::EntityId& scriptCanvasGraphId)
    {
        ScriptCanvas::GraphVariableManagerNotificationBus::Handler::BusDisconnect();
        m_scriptCanvasGraphId = scriptCanvasGraphId;

        if (m_scriptCanvasGraphId.IsValid())
        {
            ScriptCanvas::GraphVariableManagerNotificationBus::Handler::BusConnect(scriptCanvasGraphId);
        }

        PopulateSceneVariables();
    }

    AZ::EntityId GraphVariablesModel::GetScriptCanvasGraphId() const
    {
        return m_scriptCanvasGraphId;
    }

    bool GraphVariablesModel::IsEditabletype(ScriptCanvas::Data::Type scriptCanvasDataType) const
    {
        return scriptCanvasDataType.IS_A(ScriptCanvas::Data::Type::String())
            || scriptCanvasDataType.IS_A(ScriptCanvas::Data::Type::Number())
            || scriptCanvasDataType.IS_A(ScriptCanvas::Data::Type::Boolean());
    }

    void GraphVariablesModel::PopulateSceneVariables()
    {
        layoutAboutToBeChanged();

        ScriptCanvas::VariableNotificationBus::MultiHandler::BusDisconnect();
        m_variableIds.clear();
    
        const AZStd::unordered_map<ScriptCanvas::VariableId, ScriptCanvas::VariableNameValuePair>* variableMap = nullptr;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(variableMap, m_scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::GetVariables);

        if (variableMap)
        {
            m_variableIds.reserve(variableMap->size());

            for (const AZStd::pair<ScriptCanvas::VariableId, ScriptCanvas::VariableNameValuePair>& element : *variableMap)
            {
                ScriptCanvas::VariableNotificationBus::MultiHandler::BusConnect(element.first);
                m_variableIds.push_back(element.first);
            }
        }

        layoutChanged();
    }

    void GraphVariablesModel::OnVariableAddedToGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view /*variableName*/)
    {
        int index = static_cast<int>(m_variableIds.size());

        beginInsertRows(QModelIndex(), index, index);
        m_variableIds.push_back(variableId);
        endInsertRows();

        ScriptCanvas::VariableNotificationBus::MultiHandler::BusConnect(variableId);

        QModelIndex modelIndex = createIndex(index, 0);
        emit VariableAdded(modelIndex);
    }

    void GraphVariablesModel::OnVariableRemovedFromGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view /*variableName*/)
    {
        int index = FindRowForVariableId(variableId);

        if (index >= 0)
        {
            beginRemoveRows(QModelIndex(), index, index);
            m_variableIds.erase(m_variableIds.begin() + index);
            endRemoveRows();

            ScriptCanvas::VariableNotificationBus::MultiHandler::BusDisconnect(variableId);
        }
    }

    void GraphVariablesModel::OnVariableNameChangedInGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view /*variableName*/)
    {
        int index = FindRowForVariableId(variableId);

        if (index >= 0)
        {
            QModelIndex modelIndex = createIndex(index, ColumnIndex::Name, nullptr);
            dataChanged(modelIndex, modelIndex);

        }
    }

    void GraphVariablesModel::OnVariableValueChanged()
    {
        const ScriptCanvas::VariableId* variableId = ScriptCanvas::VariableNotificationBus::GetCurrentBusId();

        int index = FindRowForVariableId((*variableId));

        if (index >= 0)
        {
            QModelIndex modelIndex = createIndex(index, ColumnIndex::DefaultValue, nullptr);
            dataChanged(modelIndex, modelIndex);
        }
    }

    ScriptCanvas::VariableId GraphVariablesModel::FindVariableIdForIndex(const QModelIndex& index) const
    {
        ScriptCanvas::VariableId variableId;

        if (index.row() >= 0 && index.row() < m_variableIds.size())
        {
            variableId = m_variableIds[index.row()];
        }

        return variableId;
    }

    int GraphVariablesModel::FindRowForVariableId(const ScriptCanvas::VariableId& variableId) const
    {
        for (int i = 0; i < static_cast<int>(m_variableIds.size()); ++i)
        {
            if (m_variableIds[i] == variableId)
            {
                return i;
            }
        }

        return -1;
    }

    ////////////////////////////////////////////
    // GraphVariablesModelSortFilterProxyModel
    ////////////////////////////////////////////

    GraphVariablesModelSortFilterProxyModel::GraphVariablesModelSortFilterProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {

    }

    bool GraphVariablesModelSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        if (m_filter.isEmpty())
        {
            return true;
        }

        GraphVariablesModel* model = qobject_cast<GraphVariablesModel*>(sourceModel());
        if (!model)
        {
            return false;
        }

        QModelIndex index = model->index(sourceRow, GraphVariablesModel::ColumnIndex::Name, sourceParent);
        QString test = model->data(index, Qt::DisplayRole).toString();

        return (test.lastIndexOf(m_filterRegex) >= 0);
    }

    void GraphVariablesModelSortFilterProxyModel::SetFilter(const QString& filter)
    {
        m_filter = filter;
        m_filterRegex = QRegExp(m_filter, Qt::CaseInsensitive);

        invalidateFilter();
    }

    ////////////////////////////
    // GraphVariablesTableView
    ////////////////////////////

    GraphVariablesTableView::GraphVariablesTableView(QWidget* parent)
        : QTableView(parent)
    {
        m_model = aznew GraphVariablesModel(this);
        m_proxyModel = aznew GraphVariablesModelSortFilterProxyModel(this);

        m_proxyModel->setSourceModel(m_model);
        m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        m_proxyModel->sort(GraphVariablesModel::ColumnIndex::Name);

        setModel(m_proxyModel);

        setItemDelegateForColumn(GraphVariablesModel::Name, aznew GraphCanvas::IconDecoratedNameDelegate(this));

        horizontalHeader()->setSectionResizeMode(GraphVariablesModel::Type, QHeaderView::ResizeMode::ResizeToContents);

        horizontalHeader()->setSectionResizeMode(GraphVariablesModel::Name, QHeaderView::ResizeMode::Stretch);
        horizontalHeader()->setSectionResizeMode(GraphVariablesModel::DefaultValue, QHeaderView::ResizeMode::Stretch);

        QAction* deleteAction = new QAction(this);
        deleteAction->setShortcut(QKeySequence(Qt::Key_Delete));

        QObject::connect(deleteAction, &QAction::triggered, this, &GraphVariablesTableView::OnDeleteSelected);

        addAction(deleteAction);

        QObject::connect(m_model, &GraphVariablesModel::VariableAdded, this, &GraphVariablesTableView::OnVariableAdded);

        setMinimumSize(0, 0);
    }

    void GraphVariablesTableView::SetActiveScene(const AZ::EntityId& scriptCanvasGraphId)
    {
        clearSelection();
        m_model->SetActiveScene(scriptCanvasGraphId);

        m_graphCanvasGraphId.SetInvalid();
        GeneralRequestBus::BroadcastResult(m_graphCanvasGraphId, &GeneralRequests::GetGraphCanvasGraphId, scriptCanvasGraphId);
    }

    void GraphVariablesTableView::SetFilter(const QString& filterString)
    {
        clearSelection();
        m_proxyModel->SetFilter(filterString);
    }

    void GraphVariablesTableView::hideEvent(QHideEvent* event)
    {
        QTableView::hideEvent(event);

        clearSelection();
        m_proxyModel->SetFilter("");
    }

    void GraphVariablesTableView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        QTableView::selectionChanged(selected, deselected);

        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();

        AZStd::unordered_set< ScriptCanvas::VariableId > variableSet;

        QModelIndexList indexList = selectedIndexes();

        for (const QModelIndex& index : indexList)
        {
            QModelIndex sourceIndex = m_proxyModel->mapToSource(index);

            variableSet.insert(m_model->FindVariableIdForIndex(sourceIndex));
        }

        emit SelectionChanged(variableSet);

        if (!selected.isEmpty())
        {
            GraphCanvas::SceneNotificationBus::Handler::BusConnect(m_graphCanvasGraphId);
        }
    }

    void GraphVariablesTableView::OnSelectionChanged()
    {
        clearSelection();
    }

    void GraphVariablesTableView::OnVariableAdded(QModelIndex modelIndex)
    {
        clearSelection();
        m_proxyModel->SetFilter("");

        QModelIndex proxyIndex = m_proxyModel->mapFromSource(modelIndex);

        scrollTo(proxyIndex);
        selectionModel()->select(QItemSelection(proxyIndex, proxyIndex), QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }

    void GraphVariablesTableView::OnDeleteSelected()
    {
        AZStd::unordered_set< ScriptCanvas::VariableId > variableSet;

        QModelIndexList indexList = selectedIndexes();

        for (const QModelIndex& index : indexList)
        {
            QModelIndex sourceIndex = m_proxyModel->mapToSource(index);

            variableSet.insert(m_model->FindVariableIdForIndex(sourceIndex));
        }

        emit DeleteVariables(variableSet);
    }

#include <Editor/View/Widgets/VariablePanel/GraphVariablesTableView.moc>
}
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
#include <qclipboard.h>
#include <qheaderview.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/GraphCanvasProfiler.h>

#include <Editor/Include/ScriptCanvas/Bus/RequestBus.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/View/Widgets/VariablePanel/GraphVariablesTableView.h>

#include <Editor/Settings.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/QtMetaTypes.h>

#include <Editor/View/Widgets/ScriptCanvasNodePaletteDockWidget.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>

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

                if (varPair && IsEditableType(varType))
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
                    else if (varType.IS_A(ScriptCanvas::Data::Type::CRC()))
                    {
                        const ScriptCanvas::Data::CRCType* crcValue = varPair->m_varDatum.GetData().GetAs<ScriptCanvas::Data::CRCType>();

                        AZStd::string crcString;
                        EditorGraphRequestBus::EventResult(crcString, GetScriptCanvasGraphId(), &EditorGraphRequests::DecodeCrc, (*crcValue));
                        return QVariant(crcString.c_str());
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

                if (varPair && IsEditableType(varType))
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
                    else if (varType.IS_A(ScriptCanvas::Data::Type::CRC()))
                    {
                        const ScriptCanvas::Data::CRCType* crcValue = varPair->m_varDatum.GetData().GetAs<ScriptCanvas::Data::CRCType>();

                        AZStd::string crcString;
                        EditorGraphRequestBus::EventResult(crcString, GetScriptCanvasGraphId(), &EditorGraphRequests::DecodeCrc, (*crcValue));

                        if (!crcString.empty())
                        {
                            return QVariant(crcString.c_str());
                        }
                        else
                        {
                            return QString("<Empty>");
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
                    const QPixmap* icon = nullptr;

                    ScriptCanvas::Data::Type varType = varPair->m_varDatum.GetData().GetType();

                    if (ScriptCanvas::Data::IsContainerType(varType))
                    {
                        AZStd::vector<ScriptCanvas::Data::Type > dataTypes = ScriptCanvas::Data::GetContainedTypes(varType);

                        AZStd::vector< AZ::Uuid> azTypes;
                        azTypes.reserve(dataTypes.size());

                        for (const ScriptCanvas::Data::Type& dataType : dataTypes)
                        {
                            azTypes.push_back(ScriptCanvas::Data::ToAZType(dataType));
                        }

                        GraphCanvas::StyleManagerRequestBus::EventResult(icon, ScriptCanvasEditor::AssetEditorId, &GraphCanvas::StyleManagerRequests::GetMultiDataTypeIcon, azTypes);
                    }
                    else
                    {
                        GraphCanvas::StyleManagerRequestBus::EventResult(icon, ScriptCanvasEditor::AssetEditorId, &GraphCanvas::StyleManagerRequests::GetDataTypeIcon, ScriptCanvas::Data::ToAZType(varType));
                    }

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

                if (varPair && IsEditableType(varType))
                {
                    if (varType.IS_A(ScriptCanvas::Data::Type::String()))
                    {
                        modifiedData = varPair->m_varDatum.SetValueAs<ScriptCanvas::Data::StringType>(ScriptCanvas::Data::StringType(value.toString().toUtf8().data()));
                    }
                    else if (varType.IS_A(ScriptCanvas::Data::Type::Number()))
                    {
                        modifiedData = varPair->m_varDatum.SetValueAs<ScriptCanvas::Data::NumberType>(value.toDouble());
                    }
                    else if (varType.IS_A(ScriptCanvas::Data::Type::CRC()))
                    {
                        AZStd::string newStringValue = value.toString().toUtf8().data();
                        AZ::Crc32 newCrcValue = AZ::Crc32(newStringValue.c_str());

                        const ScriptCanvas::Data::CRCType* oldCrcValue = varPair->m_varDatum.GetData().GetAs<ScriptCanvas::Data::CRCType>();

                        if (newCrcValue != (*oldCrcValue))
                        {
                            EditorGraphRequestBus::Event(GetScriptCanvasGraphId(), &EditorGraphRequests::RemoveCrcCache, (*oldCrcValue));
                            EditorGraphRequestBus::Event(GetScriptCanvasGraphId(), &EditorGraphRequests::AddCrcCache, newCrcValue, newStringValue);

                            modifiedData = varPair->m_varDatum.SetValueAs<ScriptCanvas::Data::CRCType>(newCrcValue);
                        }
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

            if (IsEditableType(varType))
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

    bool GraphVariablesModel::IsEditableType(ScriptCanvas::Data::Type scriptCanvasDataType) const
    {
        return scriptCanvasDataType.IS_A(ScriptCanvas::Data::Type::String())
            || scriptCanvasDataType.IS_A(ScriptCanvas::Data::Type::Number())
            || scriptCanvasDataType.IS_A(ScriptCanvas::Data::Type::Boolean())
            || scriptCanvasDataType.IS_A(ScriptCanvas::Data::Type::CRC());
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

    bool GraphVariablesTableView::HasCopyVariableData()
    {

        return QApplication::clipboard()->mimeData() && QApplication::clipboard()->mimeData()->hasFormat(ScriptCanvas::CopiedVariableData::k_variableKey);
    }

    void GraphVariablesTableView::CopyVariableToClipboard(const AZ::EntityId& scriptCanvasGraphId, const ScriptCanvas::VariableId& variableId)
    {
        ScriptCanvas::VariableNameValuePair* valuePair = nullptr;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(valuePair, scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, variableId);

        if (valuePair == nullptr)
        {
            return;
        }

        ScriptCanvas::CopiedVariableData copiedVariableData;
        copiedVariableData.m_variableMapping[variableId] = (*valuePair);

        AZStd::vector<char> buffer;
        AZ::SerializeContext* serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();        

        AZ::IO::ByteContainerStream<AZStd::vector<char>> stream(&buffer);
        AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_BINARY, &copiedVariableData, serializeContext);

        QMimeData* mime = new QMimeData();
        mime->setData(ScriptCanvas::CopiedVariableData::k_variableKey, QByteArray(buffer.cbegin(), static_cast<int>(buffer.size())));

        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setMimeData(mime);
    }

    void GraphVariablesTableView::HandleVariablePaste(const AZ::EntityId& scriptCanvasGraphId)
    {
        QClipboard* clipboard = QApplication::clipboard();

        // Trying to paste unknown data into our scene.
        if (!HasCopyVariableData())
        {            
            return;
        }

        ScriptCanvas::CopiedVariableData copiedVariableData;

        QByteArray byteArray = clipboard->mimeData()->data(ScriptCanvas::CopiedVariableData::k_variableKey);

        AZ::SerializeContext* serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();
        AZ::Utils::LoadObjectFromBufferInPlace(byteArray.constData(), static_cast<AZStd::size_t>(byteArray.size()), copiedVariableData, serializeContext);

        for (auto variableMapData : copiedVariableData.m_variableMapping)
        {
            ScriptCanvas::GraphVariableManagerRequestBus::Event(scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::CloneVariable, variableMapData.second);
        }
    }

    GraphVariablesTableView::GraphVariablesTableView(QWidget* parent)
        : QTableView(parent)
        , m_nextInstanceAction(nullptr)
        , m_previousInstanceAction(nullptr)
    {
        m_model = aznew GraphVariablesModel(this);
        m_proxyModel = aznew GraphVariablesModelSortFilterProxyModel(this);

        m_proxyModel->setSourceModel(m_model);
        m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

        setModel(m_proxyModel);

        ApplyPreferenceSort();
        setItemDelegateForColumn(GraphVariablesModel::Name, aznew GraphCanvas::IconDecoratedNameDelegate(this));

        horizontalHeader()->setStretchLastSection(false);
        horizontalHeader()->setSectionResizeMode(GraphVariablesModel::Type, QHeaderView::ResizeMode::Fixed);
        horizontalHeader()->setSectionResizeMode(GraphVariablesModel::Name, QHeaderView::ResizeMode::Stretch);
        horizontalHeader()->setSectionResizeMode(GraphVariablesModel::DefaultValue, QHeaderView::ResizeMode::Fixed);

        {
            QAction* deleteAction = new QAction(this);
            deleteAction->setShortcut(QKeySequence(Qt::Key_Delete));

            QObject::connect(deleteAction, &QAction::triggered, this, &GraphVariablesTableView::OnDeleteSelected);

            addAction(deleteAction);
        }

        {
            QAction* copyAction = new QAction(this);
            copyAction->setShortcut(QKeySequence::Copy);

            QObject::connect(copyAction, &QAction::triggered, this, &GraphVariablesTableView::OnCopySelected);

            addAction(copyAction);
        }

        {
            QAction* pasteAction = new QAction(this);
            pasteAction->setShortcut(QKeySequence::Paste);

            QObject::connect(pasteAction, &QAction::triggered, this, &GraphVariablesTableView::OnPaste);

            addAction(pasteAction);
        }

        {
            QAction* duplicateAction = new QAction(this);
            duplicateAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));

            QObject::connect(duplicateAction, &QAction::triggered, this, &GraphVariablesTableView::OnDuplicate);

            addAction(duplicateAction);
        }

        {
            m_nextInstanceAction = new QAction(this);
            m_nextInstanceAction->setShortcut(QKeySequence(Qt::Key_F8));

            QObject::connect(m_nextInstanceAction, &QAction::triggered, this, &GraphVariablesTableView::CycleToNextVariableNode);

            addAction(m_nextInstanceAction);
        }

        {
            m_previousInstanceAction = new QAction(this);
            m_previousInstanceAction->setShortcut(QKeySequence(Qt::Key_F7));

            QObject::connect(m_previousInstanceAction, &QAction::triggered, this, &GraphVariablesTableView::CycleToPreviousVariableNode);

            addAction(m_previousInstanceAction);
        }

        QObject::connect(m_model, &GraphVariablesModel::VariableAdded, this, &GraphVariablesTableView::OnVariableAdded);

        setMinimumSize(0, 0);
        ResizeColumns();
    }

    void GraphVariablesTableView::SetActiveScene(const AZ::EntityId& scriptCanvasGraphId)
    {
        clearSelection();
        m_model->SetActiveScene(scriptCanvasGraphId);

        m_scriptCanvasGraphId = scriptCanvasGraphId;

        m_graphCanvasGraphId.SetInvalid();
        GeneralRequestBus::BroadcastResult(m_graphCanvasGraphId, &GeneralRequests::GetGraphCanvasGraphId, scriptCanvasGraphId);

        m_cyclingHelper.SetActiveGraph(m_graphCanvasGraphId);
    }

    void GraphVariablesTableView::SetFilter(const QString& filterString)
    {
        clearSelection();
        m_proxyModel->SetFilter(filterString);
    }

    void GraphVariablesTableView::EditVariableName(ScriptCanvas::VariableId variableId)
    {
        int row = m_model->FindRowForVariableId(variableId);

        QModelIndex sourceIndex = m_model->index(row, GraphVariablesModel::ColumnIndex::Name);
        QModelIndex proxyIndex = m_proxyModel->mapFromSource(sourceIndex);

        setCurrentIndex(proxyIndex);
        edit(proxyIndex);
    }

    void GraphVariablesTableView::hideEvent(QHideEvent* event)
    {
        QTableView::hideEvent(event);

        clearSelection();        
        m_proxyModel->SetFilter("");
    }

    void GraphVariablesTableView::resizeEvent(QResizeEvent* resizeEvent)
    {
        QTableView::resizeEvent(resizeEvent);

        ResizeColumns();
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

        if (variableSet.size() == 1)
        {
            SetCycleTarget((*variableSet.begin()));
        }
        else
        {
            SetCycleTarget(ScriptCanvas::VariableId());
        }

        emit SelectionChanged(variableSet);

        if (!selected.isEmpty())
        {
            GraphCanvas::SceneNotificationBus::Handler::BusConnect(m_graphCanvasGraphId);
        }
    }

    void GraphVariablesTableView::OnSelectionChanged()
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        clearSelection();
    }

    void GraphVariablesTableView::ApplyPreferenceSort()
    {
        AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
        m_proxyModel->sort(settings->m_variablePanelSorting);
    }

    void GraphVariablesTableView::OnVariableAdded(QModelIndex modelIndex)
    {
        bool isUndo = false;
        GeneralRequestBus::BroadcastResult(isUndo, &GeneralRequests::IsActiveInUndoRedo);

        if (!isUndo)
        {
            clearSelection();
            m_proxyModel->SetFilter("");

            QModelIndex proxyIndex = m_proxyModel->mapFromSource(modelIndex);

            scrollTo(proxyIndex);
            selectionModel()->select(QItemSelection(proxyIndex, proxyIndex), QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }

        ResizeColumns();
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

        ResizeColumns();
    }

    void GraphVariablesTableView::ResizeColumns()
    {
        int availableWidth = width();

        int typeLength = static_cast<int>(availableWidth * 0.30f);

        int maxTypeLength = sizeHintForColumn(GraphVariablesModel::Type) + 10;

        if (typeLength >= maxTypeLength)
        {
            typeLength = maxTypeLength;
        }

        int defaultValueLength = static_cast<int>(availableWidth * 0.20f);

        horizontalHeader()->resizeSection(GraphVariablesModel::Type, typeLength);

        horizontalHeader()->resizeSection(GraphVariablesModel::DefaultValue, defaultValueLength);
    }

    void GraphVariablesTableView::OnCopySelected()
    {
        QModelIndexList indexList = selectedIndexes();

        if (!indexList.empty())
        {
            QModelIndex sourceIndex = m_proxyModel->mapToSource(indexList.front());

            ScriptCanvas::VariableId variableId = m_model->FindVariableIdForIndex(sourceIndex);
            CopyVariableToClipboard(m_scriptCanvasGraphId, variableId);
        }
    }

    void GraphVariablesTableView::OnPaste()
    {
        HandleVariablePaste(m_scriptCanvasGraphId);
    }

    void GraphVariablesTableView::SetCycleTarget(ScriptCanvas::VariableId variableId)
    {
        m_cyclingHelper.Clear();
        m_cyclingVariableId = variableId;

        if (m_nextInstanceAction)
        {
            m_nextInstanceAction->setEnabled(m_cyclingVariableId.IsValid());
            m_previousInstanceAction->setEnabled(m_cyclingVariableId.IsValid());
        }
    }

    void GraphVariablesTableView::OnDuplicate()
    {
        QModelIndexList indexList = selectedIndexes();

        if (!indexList.empty())
        {
            QModelIndex sourceIndex = m_proxyModel->mapToSource(indexList.front());

            ScriptCanvas::VariableId variableId = m_model->FindVariableIdForIndex(sourceIndex);

            ScriptCanvas::VariableNameValuePair* valuePair = nullptr;
            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(valuePair, m_scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, variableId);

            if (valuePair == nullptr)
            {
                return;
            }

            ScriptCanvas::GraphVariableManagerRequestBus::Event(m_scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::CloneVariable, (*valuePair));
        }
    }

    void GraphVariablesTableView::CycleToNextVariableNode()
    {
        ConfigureHelper();

        m_cyclingHelper.CycleToNextNode();
    }

    void GraphVariablesTableView::CycleToPreviousVariableNode()
    {
        ConfigureHelper();

        m_cyclingHelper.CycleToPreviousNode();
    }

    void GraphVariablesTableView::ConfigureHelper()
    {
        if (!m_cyclingHelper.IsConfigured() && m_cyclingVariableId.IsValid())
        {
            AZStd::vector< NodeIdPair > nodeIds;
            EditorGraphRequestBus::EventResult(nodeIds, m_scriptCanvasGraphId, &EditorGraphRequests::GetVariableNodes, m_cyclingVariableId);

            AZStd::vector< GraphCanvas::NodeId > canvasNodes;
            canvasNodes.reserve(nodeIds.size());

            for (const auto& nodeIdPair : nodeIds)
            {
                canvasNodes.emplace_back(nodeIdPair.m_graphCanvasId);
            }

            m_cyclingHelper.SetNodes(canvasNodes);
        }
    }



#include <Editor/View/Widgets/VariablePanel/GraphVariablesTableView.moc>
}
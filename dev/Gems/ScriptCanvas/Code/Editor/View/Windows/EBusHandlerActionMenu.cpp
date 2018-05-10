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
#include "precompiled.h"

#include <QEvent>
#include <QLineEdit>
#include <QListView>
#include <QWidgetAction>
#include <QKeyEvent>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeBus.h>

#include "Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h"
#include "Editor/Translation/TranslationHelper.h"
#include "Editor/View/Windows/EBusHandlerActionMenu.h"
#include <Editor/View/Widgets/NodePalette/EBusNodePaletteTreeItemTypes.h>
#include <Editor/View/Windows/ui_ebushandleractionlistwidget.h>

#include <ScriptCanvas/Bus/RequestBus.h>

namespace ScriptCanvasEditor
{
    /////////////////////////////////
    // EBusHandlerActionSourceModel
    /////////////////////////////////
    
    EBusHandlerActionSourceModel::EBusHandlerActionSourceModel(QObject* parent)
        : QAbstractListModel(parent)
    {
    }
    
    EBusHandlerActionSourceModel::~EBusHandlerActionSourceModel()
    {
    }    
    
    int EBusHandlerActionSourceModel::rowCount(const QModelIndex& parent) const
    {
        return static_cast<int>(m_actionItems.size());
    }
    
    QVariant EBusHandlerActionSourceModel::data(const QModelIndex& index, int role) const
    {
        int row = index.row();
        const EBusHandlerActionItem& actionItem = GetActionItemForRow(row);
        
        switch (role)
        {
            case Qt::DisplayRole:
                return actionItem.m_displayName;
            case Qt::CheckStateRole:
                if (actionItem.m_active)
                {
                    return Qt::Checked;
                }
                else
                {
                    return Qt::Unchecked;
                }
                break;
            default:
                break;
        }
        
        return QVariant();
    }    
    
    QVariant EBusHandlerActionSourceModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        return QVariant();
    }
    
    Qt::ItemFlags EBusHandlerActionSourceModel::flags(const QModelIndex &index) const
    {
        return Qt::ItemFlags(Qt::ItemIsUserCheckable
                             | Qt::ItemIsEnabled
                             | Qt::ItemIsSelectable);
    }

    void EBusHandlerActionSourceModel::OnItemClicked(const QModelIndex& index)
    {
        EBusHandlerActionItem& actionItem = GetActionItemForRow(index.row());
        actionItem.m_active = !actionItem.m_active;

        UpdateEBusItem(actionItem);

        emit dataChanged(index, index);
    }
    
    void EBusHandlerActionSourceModel::SetEBusNodeSource(const AZ::EntityId& ebusNode)
    {
        layoutAboutToBeChanged();
        m_actionItems.clear();

        m_ebusNode = ebusNode;

        EBusHandlerNodeDescriptorRequestBus::EventResult(m_busName, m_ebusNode, &EBusHandlerNodeDescriptorRequests::GetBusName);
        
        AZStd::vector< AZStd::string > eventNames;
        EBusHandlerNodeDescriptorRequestBus::EventResult(eventNames, m_ebusNode, &EBusHandlerNodeDescriptorRequests::GetEventNames);
        
        m_actionItems.resize(eventNames.size());
         
        for (unsigned int i=0; i < eventNames.size(); ++i)
        {
            EBusHandlerActionItem& actionItem = m_actionItems[i];
            actionItem.m_name = QString(eventNames[i].c_str());

            AZStd::string translatedName = TranslationHelper::GetKeyTranslation(TranslationContextGroup::EbusHandler, m_busName, eventNames[i], TranslationItemType::Node, TranslationKeyId::Name);

            if (translatedName.empty())
            {
                actionItem.m_displayName = QString(eventNames[i].c_str());
            }
            else
            {
                actionItem.m_displayName = QString(translatedName.c_str());
            }
            
            actionItem.m_index = i;
            
            EBusHandlerNodeDescriptorRequestBus::EventResult(actionItem.m_active, ebusNode, &EBusHandlerNodeDescriptorRequests::ContainsEvent, eventNames[i]);
        }

        layoutChanged();
    }
    
    EBusHandlerActionItem& EBusHandlerActionSourceModel::GetActionItemForRow(int row)
    {
        return const_cast<EBusHandlerActionItem&>(static_cast<const EBusHandlerActionSourceModel*>(this)->GetActionItemForRow(row));
    }
    
    const EBusHandlerActionItem& EBusHandlerActionSourceModel::GetActionItemForRow(int row) const
    {
        if (row >= 0 && row < m_actionItems.size())
        {
            return m_actionItems[row];
        }
        else
        {
            static EBusHandlerActionItem invalidItem;
            AZ_Warning("Script Canvas", false, "EBus Handler Action Source model being asked for invalid item.");
            return invalidItem;
        }
    }

    void EBusHandlerActionSourceModel::UpdateEBusItem(EBusHandlerActionItem& actionItem)
    {
        AZ::EntityId graphCanvasGraphId;
        GraphCanvas::SceneMemberRequestBus::EventResult(graphCanvasGraphId, m_ebusNode, &GraphCanvas::SceneMemberRequests::GetScene);

        AZStd::string eventName(actionItem.m_name.toUtf8().data());

        if (actionItem.m_active)
        {
            AZ::Vector2 dummyPosition(0, 0);
            CreateEBusHandlerEventMimeEvent mimeEvent(QString(m_busName.c_str()), actionItem.m_name);

            NodeIdPair idPair = mimeEvent.CreateEventNode(graphCanvasGraphId, dummyPosition);

            GraphCanvas::WrappedNodeConfiguration configuration;

            EBusHandlerNodeDescriptorRequestBus::EventResult(configuration, m_ebusNode, &EBusHandlerNodeDescriptorRequests::GetEventConfiguration, eventName);
            GraphCanvas::WrapperNodeRequestBus::Event(m_ebusNode, &GraphCanvas::WrapperNodeRequests::WrapNode, idPair.m_graphCanvasId, configuration);

            AZ::EntityId scriptCanvasGraphId;
            GeneralRequestBus::BroadcastResult(scriptCanvasGraphId, &GeneralRequests::GetScriptCanvasGraphId, graphCanvasGraphId);

            GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, scriptCanvasGraphId);
        }
        else
        {
            AZ::EntityId nodeId;
            EBusHandlerNodeDescriptorRequestBus::EventResult(nodeId, m_ebusNode, &EBusHandlerNodeDescriptorRequests::FindEventNodeId, eventName);

            if (nodeId.IsValid())
            {
                AZStd::unordered_set<AZ::EntityId> deleteNodes;
                deleteNodes.insert(nodeId);

                GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::Delete, deleteNodes);

                actionItem.m_active = false;
            }
        }
    }
    
    //////////////////////////////////////
    // EBusHandlerActionFilterProxyModel
    //////////////////////////////////////
    EBusHandlerActionFilterProxyModel::EBusHandlerActionFilterProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {
        m_regex.setCaseSensitivity(Qt::CaseInsensitive);
    }
    
    void EBusHandlerActionFilterProxyModel::SetFilterSource(QLineEdit* lineEdit)
    {
        if (lineEdit)
        {
            QObject::connect(lineEdit, &QLineEdit::textChanged, this, &EBusHandlerActionFilterProxyModel::OnFilterChanged);
        }
    }
    
    bool EBusHandlerActionFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        EBusHandlerActionSourceModel* sourceModel = static_cast<EBusHandlerActionSourceModel*>(this->sourceModel());

        const EBusHandlerActionItem& actionItem = sourceModel->GetActionItemForRow(sourceRow);

        return actionItem.m_name.lastIndexOf(m_regex) >= 0;
    }
    
    void EBusHandlerActionFilterProxyModel::OnFilterChanged(const QString& text)
    {
        m_filter = text;
        m_regex.setPattern(text);
        invalidate();
        layoutChanged();
    }

    //////////////////////////
    // EBusHandlerActionMenu
    //////////////////////////
    
    EBusHandlerActionMenu::EBusHandlerActionMenu(QWidget* parent)
        : QMenu(parent)
    {
        QWidgetAction* actionWidget = new QWidgetAction(this);
        
        QWidget* listWidget = new QWidget(this);
        m_listWidget = new Ui::EBusHandlerActionListWidget();
        
        m_listWidget->setupUi(listWidget);
        
        actionWidget->setDefaultWidget(listWidget);
        
        addAction(actionWidget);
        
        connect(this, &QMenu::aboutToShow, this, &EBusHandlerActionMenu::ResetFilter);

        m_model = aznew EBusHandlerActionSourceModel(this);

        m_proxyModel = aznew EBusHandlerActionFilterProxyModel(this);
        m_proxyModel->setSourceModel(m_model);

        m_proxyModel->SetFilterSource(m_listWidget->searchFilter);

        m_listWidget->actionListView->setModel(m_proxyModel);

        QObject::connect(m_listWidget->actionListView, &QListView::clicked, this, &EBusHandlerActionMenu::ItemClicked);
    }

    void EBusHandlerActionMenu::SetEbusHandlerNode(const AZ::EntityId& ebusWrapperNode)
    {
        m_proxyModel->layoutAboutToBeChanged();
        m_model->SetEBusNodeSource(ebusWrapperNode);
        m_proxyModel->layoutChanged();
        m_proxyModel->invalidate();
    }

    void EBusHandlerActionMenu::ResetFilter()
    {
        m_listWidget->actionListView->selectionModel()->clearSelection();
        m_listWidget->searchFilter->setText("");
    }

    void EBusHandlerActionMenu::ItemClicked(const QModelIndex& modelIndex)
    {
        QModelIndex sourceIndex = m_proxyModel->mapToSource(modelIndex);
        m_model->OnItemClicked(sourceIndex);
    }
    
    void EBusHandlerActionMenu::keyPressEvent(QKeyEvent* keyEvent)
    {
        // Only pass along escape keys(don't want any sort of selection state with this menu)
        if (keyEvent->key() == Qt::Key_Escape)
        {
            QMenu::keyPressEvent(keyEvent);
        }
    }
    
#include <Editor/View/Windows/EBusHandlerActionMenu.moc>
}
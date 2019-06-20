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

#include <QCoreApplication>
#include <qgraphicslayoutitem.h>
#include <qgraphicsscene.h>
#include <qsizepolicy.h>

#include <Components/Slots/Data/DataSlotLayoutComponent.h>

#include <GraphCanvas/Components/NodePropertyDisplay/NodePropertyDisplay.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <Components/Slots/Data/DataSlotConnectionPin.h>
#include <Widgets/GraphCanvasLabel.h>

namespace GraphCanvas
{
    ///////////////////
    // DataSlotLayout
    ///////////////////
	
    DataSlotLayout::DataSlotLayout(DataSlotLayoutComponent& owner)
        : m_connectionType(ConnectionType::CT_Invalid)
        , m_owner(owner)
        , m_nodePropertyDisplay(nullptr)
    {
        setInstantInvalidatePropagation(true);
        setOrientation(Qt::Horizontal);

        m_spacer = new QGraphicsWidget();
        m_spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        m_spacer->setAutoFillBackground(true);
        m_spacer->setMinimumSize(0, 0);
        m_spacer->setPreferredWidth(0);
        m_spacer->setMaximumHeight(0);

        m_nodePropertyDisplay = aznew NodePropertyDisplayWidget();
        m_slotConnectionPin = aznew DataSlotConnectionPin(owner.GetEntityId());
        m_slotText = aznew GraphCanvasLabel();
    }

    DataSlotLayout::~DataSlotLayout()
    {
    }

    void DataSlotLayout::Activate()
    {
        DataSlotNotificationBus::Handler::BusConnect(m_owner.GetEntityId());
        SceneMemberNotificationBus::Handler::BusConnect(m_owner.GetEntityId());
        SlotNotificationBus::Handler::BusConnect(m_owner.GetEntityId());
        StyleNotificationBus::Handler::BusConnect(m_owner.GetEntityId());
        DataSlotLayoutRequestBus::Handler::BusConnect(m_owner.GetEntityId());
        m_slotConnectionPin->Activate();
    }

    void DataSlotLayout::Deactivate()
    {
        m_slotConnectionPin->Deactivate();
        SceneMemberNotificationBus::Handler::BusDisconnect();
        SlotNotificationBus::Handler::BusDisconnect();
        StyleNotificationBus::Handler::BusDisconnect();
        DataSlotLayoutRequestBus::Handler::BusDisconnect();
        DataSlotNotificationBus::Handler::BusDisconnect();
        NodeDataSlotRequestBus::Handler::BusDisconnect();
    }

    void DataSlotLayout::OnSceneSet(const AZ::EntityId&)
    {
        SlotRequestBus::EventResult(m_connectionType, m_owner.GetEntityId(), &SlotRequests::GetConnectionType);

        TranslationKeyedString slotName;
        SlotRequestBus::EventResult(slotName, m_owner.GetEntityId(), &SlotRequests::GetTranslationKeyedName);

        m_slotText->SetLabel(slotName);

        TranslationKeyedString toolTip;
        SlotRequestBus::EventResult(toolTip, m_owner.GetEntityId(), &SlotRequests::GetTranslationKeyedTooltip);

        OnTooltipChanged(toolTip);
        
        TryAndSetupSlot();
    }

    void DataSlotLayout::OnSceneReady()
    {
        TryAndSetupSlot();
    }

    void DataSlotLayout::OnRegisteredToNode(const AZ::EntityId& nodeId)
    {
        NodeDataSlotRequestBus::Handler::BusDisconnect();
        NodeDataSlotRequestBus::Handler::BusConnect(nodeId);
        TryAndSetupSlot();
    }

    void DataSlotLayout::OnNameChanged(const TranslationKeyedString& name)
    {
        m_slotText->SetLabel(name);
    }

    void DataSlotLayout::OnTooltipChanged(const TranslationKeyedString& tooltip)
    {
        AZ::Uuid dataType;
        DataSlotRequestBus::EventResult(dataType, m_owner.GetEntityId(), &DataSlotRequests::GetDataTypeId);

        AZStd::string typeString;
        GraphModelRequestBus::EventResult(typeString, GetSceneId(), &GraphModelRequests::GetDataTypeString, dataType);

        AZStd::string displayText = tooltip.GetDisplayString();

        if (!typeString.empty())
        {
            if (displayText.empty())
            {
                displayText = typeString;
            }
            else
            {
                displayText = AZStd::string::format("%s - %s", typeString.c_str(), displayText.c_str());
            }
        }

        m_slotConnectionPin->setToolTip(displayText.c_str());
        m_slotText->setToolTip(displayText.c_str());
        m_nodePropertyDisplay->setToolTip(displayText.c_str());
    }

    void DataSlotLayout::OnStyleChanged()
    {
        m_style.SetStyle(m_owner.GetEntityId());

        m_nodePropertyDisplay->RefreshStyle();

        switch (m_connectionType)
        {
        case ConnectionType::CT_Input:
            m_slotText->SetStyle(m_owner.GetEntityId(), ".inputSlotName");
            break;
        case ConnectionType::CT_Output:
            m_slotText->SetStyle(m_owner.GetEntityId(), ".outputSlotName");
            break;
        default:
            m_slotText->SetStyle(m_owner.GetEntityId(), ".slotName");
            break;
        };

        m_slotConnectionPin->RefreshStyle();

        qreal padding = m_style.GetAttribute(Styling::Attribute::Padding, 2.);
        setContentsMargins(padding, padding, padding, padding);
        setSpacing(m_style.GetAttribute(Styling::Attribute::Spacing, 2.));

        UpdateGeometry();
    }

    const DataSlotConnectionPin* DataSlotLayout::GetConnectionPin() const
    {
        return m_slotConnectionPin;
    }

    void DataSlotLayout::UpdateDisplay()
    {
        if (m_nodePropertyDisplay != nullptr
            && m_nodePropertyDisplay->GetNodePropertyDisplay() != nullptr)
        {
            m_nodePropertyDisplay->GetNodePropertyDisplay()->UpdateDisplay();
        }
        if (m_slotConnectionPin != nullptr)
        {
            m_slotConnectionPin->RefreshStyle();
        }
    }

    void DataSlotLayout::OnDataSlotTypeChanged(const DataSlotType& dataSlotType)
    {
        RecreatePropertyDisplay();
    }

    void DataSlotLayout::OnDisplayTypeChanged(const AZ::Uuid& dataType, const AZStd::vector<AZ::Uuid>& typeIds)
    {
        RecreatePropertyDisplay();
        UpdateDisplay();
    }

    void DataSlotLayout::RecreatePropertyDisplay()
    {
        if (m_nodePropertyDisplay != nullptr)
        {
            m_nodePropertyDisplay->ClearDisplay();
            TryAndSetupSlot();
        }
    }

    AZ::EntityId DataSlotLayout::GetSceneId() const
    {
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, m_owner.GetEntityId(), &SceneMemberRequests::GetScene);
        return sceneId;
    }

    void DataSlotLayout::TryAndSetupSlot()
    {
        if (m_nodePropertyDisplay->GetNodePropertyDisplay() == nullptr)
        {
            CreateDataDisplay();
        }
    }

    void DataSlotLayout::CreateDataDisplay()
    {
        NodePropertyDisplay* nodePropertyDisplay = nullptr;

        if (m_connectionType == CT_Input)
        {
            DataSlotType dataSlotType = DataSlotType::Unknown;
            DataSlotRequestBus::EventResult(dataSlotType, m_owner.GetEntityId(), &DataSlotRequests::GetDataSlotType);

            AZ::EntityId nodeId;
            SlotRequestBus::EventResult(nodeId, m_owner.GetEntityId(), &SlotRequests::GetNode);

            AZ::Uuid typeId;
            DataSlotRequestBus::EventResult(typeId, m_owner.GetEntityId(), &DataSlotRequests::GetDataTypeId);

            AZ::EntityId sceneId;
            SceneMemberRequestBus::EventResult(sceneId, m_owner.GetEntityId(), &SceneMemberRequests::GetScene);

            if (dataSlotType == DataSlotType::Value)
            {
                GraphModelRequestBus::EventResult(nodePropertyDisplay, sceneId, &GraphModelRequests::CreateDataSlotPropertyDisplay, typeId, nodeId, m_owner.GetEntityId());
            }
            else if (dataSlotType == DataSlotType::Reference)
            {
                GraphModelRequestBus::EventResult(nodePropertyDisplay, sceneId, &GraphModelRequests::CreateDataSlotVariablePropertyDisplay, typeId, nodeId, m_owner.GetEntityId());
            }

            if (nodePropertyDisplay)
            {
                nodePropertyDisplay->SetNodeId(nodeId);
                nodePropertyDisplay->SetId(m_owner.GetEntityId());

                m_nodePropertyDisplay->SetNodePropertyDisplay(nodePropertyDisplay);
            }
        }

        UpdateLayout();
        OnStyleChanged();
    }

    void DataSlotLayout::UpdateLayout()
    {
        // make sure the connection type or visible items have changed before redoing the layout
        if (m_connectionType != m_atLastUpdate.connectionType || 
            m_slotConnectionPin != m_atLastUpdate.slotConnectionPin ||
            m_slotText != m_atLastUpdate.slotText ||  
            m_nodePropertyDisplay != m_atLastUpdate.nodePropertyDisplay || 
            m_spacer != m_atLastUpdate.spacer)
        {
            for (int i = count() - 1; i >= 0; --i)
            {
                removeAt(i);
            }

            switch (m_connectionType)
            {
            case ConnectionType::CT_Input:
                addItem(m_slotConnectionPin);
                setAlignment(m_slotConnectionPin, Qt::AlignLeft);

                addItem(m_slotText);
                setAlignment(m_slotText, Qt::AlignLeft);

                addItem(m_nodePropertyDisplay);
                setAlignment(m_slotText, Qt::AlignLeft);

                addItem(m_spacer);
                setAlignment(m_spacer, Qt::AlignLeft);
                break;
            case ConnectionType::CT_Output:
                addItem(m_spacer);
                setAlignment(m_spacer, Qt::AlignRight);

                addItem(m_slotText);
                setAlignment(m_slotText, Qt::AlignRight);

                addItem(m_slotConnectionPin);
                setAlignment(m_slotConnectionPin, Qt::AlignRight);
                break;
            default:
                addItem(m_slotConnectionPin);
                addItem(m_slotText);
                addItem(m_spacer);
                break;
            }
            UpdateGeometry();

            m_atLastUpdate.connectionType = m_connectionType;
            m_atLastUpdate.slotConnectionPin = m_slotConnectionPin;
            m_atLastUpdate.slotText = m_slotText;
            m_atLastUpdate.nodePropertyDisplay = m_nodePropertyDisplay;
            m_atLastUpdate.spacer = m_spacer;
        }
    }

    void DataSlotLayout::UpdateGeometry()
    {
        m_slotConnectionPin->updateGeometry();
        m_slotText->update();

        invalidate();
        updateGeometry();
    }

    ////////////////////////////
    // DataSlotLayoutComponent
    ////////////////////////////

    void DataSlotLayoutComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<DataSlotLayoutComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }

    DataSlotLayoutComponent::DataSlotLayoutComponent()
        : m_layout(nullptr)
    {
    }

    void DataSlotLayoutComponent::Init()
    {
        SlotLayoutComponent::Init();

        m_layout = aznew DataSlotLayout((*this));
        SetLayout(m_layout);
    }

    void DataSlotLayoutComponent::Activate()
    {
        SlotLayoutComponent::Activate();
        m_layout->Activate();
    }

    void DataSlotLayoutComponent::Deactivate()
    {
        SlotLayoutComponent::Deactivate();
        m_layout->Deactivate();
    }
}
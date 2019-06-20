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
#pragma once

#include <AzCore/Component/EntityId.h>

#include <AzQtComponents/Buses/ShortcutDispatch.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Widgets/NodePropertyBus.h>

#include <QGraphicsScene>
#include <QGraphicsView>

class QGraphicsLayoutItem;

namespace GraphCanvas
{
    // Base class for displaying a NodeProperty.
    //
    // Main idea is that in QGraphics, we want to use QWidgets
    // for a lot of our in-node editing, but this is slow with a large
    // number of instances.
    //
    // This provides an interface for allowing widgets to be swapped out depending
    // on state(thus letting us have a QWidget editable field, with a QGraphicsWidget display).
    class NodePropertyDisplay
        : public AzQtComponents::ShortcutDispatchTraits::Bus::Handler
    {
    public:
        NodePropertyDisplay() = default;
        virtual ~NodePropertyDisplay()
        {
            NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::UnlockEditState, this);
        }
        
        void SetId(const AZ::EntityId& id)
        {
            m_id = id;
            OnIdSet();
        }

        const AZ::EntityId& GetId() const { return m_id; }
        
        void SetNodeId(const AZ::EntityId& nodeId)
        {
            m_nodeId = nodeId;
        }
        
        const AZ::EntityId& GetNodeId() const
        {
            return m_nodeId;
        }        

        AZ::EntityId GetSceneId() const
        {
            AZ::EntityId sceneId;
            SceneMemberRequestBus::EventResult(sceneId, m_nodeId, &SceneMemberRequests::GetScene);
            return sceneId;
        }

        void TryAndSelectNode() const
        {
            bool isSelected = false;
            SceneMemberUIRequestBus::EventResult(isSelected, m_nodeId, &SceneMemberUIRequests::IsSelected);

            if (!isSelected)
            {
                SceneRequestBus::Event(GetSceneId(), &SceneRequests::ClearSelection);
                SceneMemberUIRequestBus::Event(GetNodeId(), &SceneMemberUIRequests::SetSelected, true);
            }
        }

        static AZStd::string CreateDisabledLabelStyle(const AZStd::string& type)
        {
            return AZStd::string::format("%sPropertyDisabledLabel", type.data());
        }

        static AZStd::string CreateDisplayLabelStyle(const AZStd::string& type)
        {
            return AZStd::string::format("%sPropertyDisplayLabel", type.data());
        }

        virtual void RefreshStyle() = 0;
        virtual void UpdateDisplay() = 0;

        // Display Widgets handles display the 'disabled' widget.
        virtual QGraphicsLayoutItem* GetDisabledGraphicsLayoutItem() = 0;

        // Display Widgets handles displaying the data in the non-editable base case.
        virtual QGraphicsLayoutItem* GetDisplayGraphicsLayoutItem() = 0;
        
        // Display Widgets handles displaying the data in an editable way
        virtual QGraphicsLayoutItem* GetEditableGraphicsLayoutItem() = 0;

        void RegisterShortcutDispatcher(QWidget* widget)
        {
            AzQtComponents::ShortcutDispatchBus::Handler::BusConnect(widget);
        }
        
        void UnregisterShortcutDispatcher(QWidget* widget)
        {
            AzQtComponents::ShortcutDispatchBus::Handler::BusDisconnect(widget);
        }

    protected:

        virtual void OnIdSet() { }

        // AzQtComponents::ShortcutDispatchBus::Handler
        virtual QWidget* GetShortcutDispatchScopeRoot(QWidget*)
        {
            QGraphicsScene* graphicsScene = nullptr;
            SceneRequestBus::EventResult(graphicsScene, GetSceneId(), &SceneRequests::AsQGraphicsScene);

            if (graphicsScene)
            {
                // Get the list of views. Which one it uses shouldn't matter.
                // Since they should all be parented to the same root window.
                QList<QGraphicsView*> graphicViews = graphicsScene->views();

                if (!graphicViews.empty())
                {
                    return graphicViews.front();
                }
            }

            return nullptr;
        }

    private:
        
        AZ::EntityId m_nodeId;
        AZ::EntityId m_id;
    };
}
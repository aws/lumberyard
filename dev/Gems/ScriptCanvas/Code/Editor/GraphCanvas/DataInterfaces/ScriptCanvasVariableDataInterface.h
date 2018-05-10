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

#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/NodePropertyDisplay/ItemModelDataInterface.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>

#include <Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>
#include <Editor/Include/ScriptCanvas/Bus/EditorSceneVariableManagerBus.h>
#include <Editor/Include/ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>

#include "ScriptCanvasDataInterface.h"

#include <ScriptCanvas/Libraries/Core/GetVariable.h>
#include <ScriptCanvas/Libraries/Core/SetVariable.h>

namespace ScriptCanvasEditor
{
    class ScriptCanvasVariableDataInterface
        : public ScriptCanvasDataInterface<GraphCanvas::ItemModelDataInterface>
        , public ScriptCanvas::VariableNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasVariableDataInterface, AZ::SystemAllocator, 0);
        
        ScriptCanvasVariableDataInterface(const AZ::EntityId& scriptCanvasGraphId, const AZ::EntityId& scriptCanvasNodeId, const ScriptCanvas::SlotId& scriptCanvasSlotId)
            : ScriptCanvasDataInterface(scriptCanvasNodeId, scriptCanvasSlotId)
            , m_scriptCanvasGraphId(scriptCanvasGraphId)
        {
            ScriptCanvas::VariableNotificationBus::Handler::BusConnect(GetVariableId());
        }
        
        ~ScriptCanvasVariableDataInterface()
        {
        }

        // GraphCanvas::VariableReferenceDataInterface
        QAbstractItemModel* GetItemModel() const override
        {
            QAbstractItemModel* retVal = nullptr;
            ScriptCanvasEditor::EditorSceneVariableManagerRequestBus::EventResult(retVal, m_scriptCanvasGraphId, &ScriptCanvasEditor::EditorSceneVariableManagerRequests::GetVariableItemModel);
            return retVal;
        }

        void AssignIndex(const QModelIndex& index) override
        {
            EditorGraphVariableItemModel* itemModel = static_cast<EditorGraphVariableItemModel*>(GetItemModel());

            if (itemModel)
            {
                ScriptCanvas::VariableId variableId = itemModel->FindVariableIdForIndex(index);

                ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();
                ScriptCanvas::VariableNotificationBus::Handler::BusConnect(variableId);

                ScriptCanvas::VariableNodeRequestBus::Event(GetNodeId(), &ScriptCanvas::VariableNodeRequests::SetId, variableId);
                PostUndoPoint();
            }
        }

        AZStd::string GetPlaceholderString() const override
        {
            return "Select Variable";
        }

        AZStd::string GetDisplayString() const override
        {
            ScriptCanvas::VariableId variableId = GetVariableId();

            AZStd::string_view variableName;
            ScriptCanvas::VariableRequestBus::EventResult(variableName, variableId, &ScriptCanvas::VariableRequests::GetName);

            return variableName;
        }

        bool IsItemValid() const override
        {
            ScriptCanvas::VariableId variableId = GetVariableId();
            return variableId.IsValid();
        }

        QString GetModelName() const override
        {
            return "Graph Variable Model";
        }
        ////

        // VariableNotificationBus::Handler
        void OnVariableRemoved() override
        {
            ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();
            SignalValueChanged();
        }

        void OnVariableRenamed(AZStd::string_view /*newVariableName*/) override
        {
            SignalValueChanged();
        }
        ////
        
    private:

        ScriptCanvas::VariableId GetVariableId() const
        {
            ScriptCanvas::VariableId variableId;
            ScriptCanvas::VariableNodeRequestBus::EventResult(variableId, GetNodeId(), &ScriptCanvas::VariableNodeRequests::GetId);
            return variableId;
        }

        AZ::EntityId m_scriptCanvasGraphId;
    };
}
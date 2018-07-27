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
#include "CryLegacy_precompiled.h"
#include "UiFlow.h"

#include <LyShine/Bus/World/UiCanvasRefBus.h>

namespace UiFlow
{
    AZ::EntityId GetCanvasEntityIdFromComponentEntity(IFlowNode::SActivationInfo* activationInfo, AZ::EntityId entityId, const string& nodeName)
    {
        AZ::EntityId canvasEntityId;

        if (!entityId.IsValid())
        {
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                "FlowGraph: %s Node: assigned component entity not initialized\n",
                nodeName.c_str());
        }
        else if (!UiCanvasRefBus::FindFirstHandler(entityId))
        {
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                "FlowGraph: %s Node: assigned component entity needs to have a UiCanvasRef component\n",
                nodeName.c_str());
        }
        else
        {
            EBUS_EVENT_ID_RESULT(canvasEntityId, entityId, UiCanvasRefBus, GetCanvas);

            if (!canvasEntityId.IsValid())
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                    "FlowGraph: %s Node: assigned component entity has no canvas loaded\n",
                    nodeName.c_str());
            }
        }

        return canvasEntityId;
    }

    AZ::EntityId GetCanvasEntityIdFromLegacyEntity(IFlowNode::SActivationInfo* activationInfo, IEntity* entity, const string& nodeName)
    {
        AZ::EntityId canvasEntityId;
        if (entity)
        {
            IScriptTable* scriptTable = entity->GetScriptTable();
            int canvasID;
            if (!scriptTable || !scriptTable->GetValue("canvasID", canvasID))
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                    "FlowGraph: %s Node: assigned entity needs to be a UiCanvasRefEntity\n",
                    nodeName.c_str());
            }
            else
            {
                canvasEntityId = gEnv->pLyShine->FindCanvasById(canvasID);
                if (!canvasEntityId.IsValid())
                {
                    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                        "FlowGraph: %s Node: couldn't find UI canvas with ID: %d. Make sure canvas is loaded\n",
                        nodeName.c_str(), canvasID);
                }
            }
        }
        else
        {
            AZ_Assert(false, "FlowGraph: %s Node: couldn't find entity\n", nodeName.c_str());
        }

        return canvasEntityId;
    }

    AZ::EntityId GetCanvasEntityIdFromEntityPort(IFlowNode::SActivationInfo* activationInfo, const string& nodeName)
    {
        FlowEntityId flowEntityId = activationInfo->entityId;

        FlowEntityType flowEntityType = GetFlowEntityTypeFromFlowEntityId(flowEntityId);

        // Get the canvas ID from the assigned entity
        AZ::EntityId canvasEntityId;
        if (flowEntityType == FlowEntityType::Legacy)
        {
            IEntity* entity = activationInfo->pEntity;
            canvasEntityId = GetCanvasEntityIdFromLegacyEntity(activationInfo, entity, nodeName);
        }
        else if (flowEntityType == FlowEntityType::Component)
        {
            canvasEntityId = GetCanvasEntityIdFromComponentEntity(activationInfo, flowEntityId, nodeName);
        }
        else if (flowEntityType == FlowEntityType::Invalid)
        {
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                "FlowGraph: %s Node: entity not assigned\n",
                nodeName.c_str());
        }
        else
        {
            AZ_Assert(false, "FlowGraph: %s Node: invalid flowEntityType\n", nodeName.c_str());
        }

        return canvasEntityId;
    }

    AZ::EntityId GetCanvasEntityId(IFlowNode::SActivationInfo* activationInfo, int canvasIdPort, const string& nodeName)
    {
        if (canvasIdPort < 0)
        {
            return GetCanvasEntityIdFromEntityPort(activationInfo, nodeName);
        }

        // Get the canvas ID from the port
        int canvasID = GetPortInt(activationInfo, canvasIdPort);

        AZ::EntityId canvasEntityId = gEnv->pLyShine->FindCanvasById(canvasID);
        if (!canvasEntityId.IsValid())
        {
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                "FlowGraph: %s Node: couldn't find UI canvas with ID: %d. Make sure canvas is loaded\n",
                nodeName.c_str(), canvasID);
        }

        return canvasEntityId;
    }

    AZ::EntityId GetElementEntityId(IFlowNode::SActivationInfo* activationInfo,
        int canvasIdPort, int elementPortStart, bool hasOptionalElementIdPort, const string& nodeName, int& elementId,
        string& elementName, AZ::EntityId& outCanvasEntityId)
    {
        AZ::EntityId elementEntityId;
        elementId = 0;
        elementName = "";

        // Get the canvas
        outCanvasEntityId = UiFlow::GetCanvasEntityId(activationInfo, canvasIdPort, nodeName);
        if (!outCanvasEntityId.IsValid())
        {
            return elementEntityId;
        }

        bool findElementById = true;

        bool firstElementPortIsName = (canvasIdPort < 0);
        if (firstElementPortIsName)
        {
            // Get the element name
            AZ::Entity* element = nullptr;
            elementName = GetPortString(activationInfo, elementPortStart);
            if (!elementName.empty())
            {
                EBUS_EVENT_ID_RESULT(element, outCanvasEntityId, UiCanvasBus, FindElementByName, elementName.c_str());

                if (element)
                {
                    elementEntityId = element->GetId();
                }
                else
                {
                    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                        "FlowGraph: %s Node: couldn't find UI element with name: %s\n",
                        nodeName.c_str(), elementName.c_str());
                }

                findElementById = false;
            }
            else
            {
                if (!hasOptionalElementIdPort)
                {
                    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                        "FlowGraph: %s Node: UI element name is empty\n",
                        nodeName.c_str());

                    findElementById = false;
                }
            }
        }

        if (findElementById)
        {
            // Get the element ID
            elementId = GetPortInt(activationInfo, firstElementPortIsName ? elementPortStart + 1 : elementPortStart);
            AZ::Entity* element = nullptr;
            EBUS_EVENT_ID_RESULT(element, outCanvasEntityId, UiCanvasBus, FindElementById, elementId);

            if (element)
            {
                elementEntityId = element->GetId();
            }
            else
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                    "FlowGraph: %s Node: couldn't find UI element with ID: %d\n",
                    nodeName.c_str(), elementId);
            }
        }

        return elementEntityId;
    }
}


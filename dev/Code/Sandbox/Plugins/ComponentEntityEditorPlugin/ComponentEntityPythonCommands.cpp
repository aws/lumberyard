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
#include "ComponentEntityEditorPlugin_precompiled.h"

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <Editor/Util/BoostPythonHelpers.h>
#include <LmbrCentral/Rendering/EditorMeshBus.h>

QString PyCreateEntity(const char* entityName)
{
    AzToolsFramework::ScopedUndoBatch undo("PyCreateEntity");
    AZ::Entity* newEntity = nullptr;
    // Don't need to check entityName for nullptr or empty, CreateEditorEntity handles that.
    AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
        newEntity,
        &AzToolsFramework::EditorEntityContextRequestBus::Events::CreateEditorEntity,
        entityName);
    if (!newEntity)
    {
        return AZ::EntityId().ToString().c_str();
    }

    // EntityId.ToString() adds [] around the entity ID, making it extra work to parse back.
    return QString(AZStd::string::format("%llu", AZ::u64(newEntity->GetId())).c_str());
}

bool PyAddMeshComponentWithMesh(const char* entityIdStr, const char* meshAssetIdStr)
{
    AZ::EntityId entityId(strtoull(entityIdStr, nullptr, 10));
    AZ::Uuid meshAssetId(AZ::Uuid::CreateString(meshAssetIdStr, strlen(meshAssetIdStr)));

    bool result = false;
    LmbrCentral::EditorMeshBus::BroadcastResult(
        result,
        &LmbrCentral::EditorMeshBus::Events::AddMeshComponentWithAssetId,
        entityId,
        meshAssetId);
    return result;
}

REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyCreateEntity, general, create_entity,
    "Creates an AZ::Entity, returns the entity's ID.",
    "general.create_entity(str entityName)");

REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyAddMeshComponentWithMesh, general, add_mesh_component_with_mesh,
    "Adds a mesh component and assets the mesh with the given asset ID to the entity passed in. Returns true on success, false on failure.",
    "general.add_mesh_component_with_mesh(str entityId, str meshAssetId)");

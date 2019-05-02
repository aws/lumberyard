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

#include "StdAfx.h"
#include "ObjectManagerLegacyUndo.h"

#include <AzToolsFramework/API/ComponentEntityObjectBus.h>
#include "PrefabObject.h"


//////////////////////////////////////////////////////////////////////////
//! Prefab helper functions
// Because a prefab consists of multiple objects but it is defined as a single GUID,
// we want to extract the GUIDs of the children. We need this info if we undo redo new/delete
// operations on prefabs, since we do NOT want to generate new IDs for the children, otherwise
// other UNDO operations depending on the GUIDs won't work
static void ExtractRemappingInformation(CBaseObject* prefab, TGUIDRemap& remapInfo)
{
    TBaseObjects children;
    prefab->GetAllPrefabFlagedChildren(children);

    for (size_t childIndex = 0, count = children.size(); childIndex < count; ++childIndex)
    {
        remapInfo.insert(std::make_pair(children[childIndex]->GetIdInPrefab(), children[childIndex]->GetId()));
    }
}

static void RemapObjectsInPrefab(CBaseObject* prefab, const TGUIDRemap& remapInfo)
{
    IObjectManager* objectManager = GetIEditor()->GetObjectManager();

    TBaseObjects children;
    prefab->GetAllPrefabFlagedChildren(children);

    for (size_t childIndex = 0, count = children.size(); childIndex < count; ++childIndex)
    {
        TGUIDRemap::const_iterator remapIt = remapInfo.find(children[childIndex]->GetIdInPrefab());
        if (remapIt != remapInfo.end())
        {
            objectManager->ChangeObjectId(children[childIndex]->GetId(), (*remapIt).second);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// CUndoBaseObjectNew implementation.
//////////////////////////////////////////////////////////////////////////

CUndoBaseObjectNew::CUndoBaseObjectNew(CBaseObject* object)
{
    m_object = object;

    m_isPrefab = qobject_cast<CPrefabObject*>(object) != nullptr;

    if (m_isPrefab)
    {
        ExtractRemappingInformation(object, m_remapping);
    }
}

void CUndoBaseObjectNew::Undo(bool bUndo)
{
    if (bUndo)
    {
        m_redo = XmlHelpers::CreateXmlNode("Redo");
        // Save current object state.
        CObjectArchive ar(GetIEditor()->GetObjectManager(), m_redo, false);
        ar.bUndo = true;
        m_object->Serialize(ar);
        m_object->SetLayerModified();
    }

    m_object->UpdatePrefab(eOCOT_Delete);

    // Delete this object.
    GetIEditor()->DeleteObject(m_object);
}

void CUndoBaseObjectNew::Redo()
{
    if (!m_redo)
    {
        return;
    }

    IObjectManager* objectManager = GetIEditor()->GetObjectManager();
    {
        CObjectArchive ar(objectManager, m_redo, true);
        ar.bUndo = true;
        ar.MakeNewIds(false);
        ar.LoadObject(m_redo, m_object);
    }

    objectManager->ClearSelection();
    objectManager->SelectObject(m_object);
    m_object->SetLayerModified();

    if (m_isPrefab)
    {
        RemapObjectsInPrefab(m_object, m_remapping);
    }

    m_object->UpdatePrefab(eOCOT_Add);
}

//////////////////////////////////////////////////////////////////////////
// CUndoBaseObjectDelete implementation.
//////////////////////////////////////////////////////////////////////////

CUndoBaseObjectDelete::CUndoBaseObjectDelete(CBaseObject* object)
{
    AZ_Assert(object, "Object does not exist");
    object->SetTransformDelegate(nullptr);
    m_object = object;

    // Save current object state.
    m_undo = XmlHelpers::CreateXmlNode("Undo");
    CObjectArchive ar(GetIEditor()->GetObjectManager(), m_undo, false);
    ar.bUndo = true;
    m_bSelected = m_object->IsSelected();
    m_object->Serialize(ar);
    m_object->SetLayerModified();

    m_isPrefab = qobject_cast<CPrefabObject*>(object);

    if (m_isPrefab)
    {
        ExtractRemappingInformation(m_object, m_remapping);
    }
}

void CUndoBaseObjectDelete::Undo(bool bUndo)
{
    IObjectManager* objectManager = GetIEditor()->GetObjectManager();
    {
        CObjectArchive ar(objectManager, m_undo, true);
        ar.bUndo = true;
        ar.MakeNewIds(false);
        ar.LoadObject(m_undo, m_object);
        m_object->ClearFlags(OBJFLAG_SELECTED);
    }

    if (m_bSelected)
    {
        objectManager->ClearSelection();
        objectManager->SelectObject(m_object);
    }
    m_object->SetLayerModified();

    if (m_isPrefab)
    {
        RemapObjectsInPrefab(m_object, m_remapping);
    }

    m_object->UpdatePrefab(eOCOT_Add);
}

void CUndoBaseObjectDelete::Redo()
{
    // Delete this object.
    m_object->SetLayerModified();
    m_object->UpdatePrefab(eOCOT_Delete);
    GetIEditor()->DeleteObject(m_object);
}

//////////////////////////////////////////////////////////////////////////
// CUndoBaseObjectSelect implementation.
//////////////////////////////////////////////////////////////////////////

CUndoBaseObjectSelect::CUndoBaseObjectSelect(CBaseObject* object)
{
    AZ_Assert(object, "Object does not exist");
    m_guid = object->GetId();
    m_bUndoSelect = object->IsSelected();
}

CUndoBaseObjectSelect::CUndoBaseObjectSelect(CBaseObject* object, bool isSelect)
{
    AZ_Assert(object, "Object does not exist");
    m_guid = object->GetId();
    m_bUndoSelect = !isSelect;
}

QString CUndoBaseObjectSelect::GetObjectName()
{
    CBaseObject* object = GetIEditor()->GetObjectManager()->FindObject(m_guid);
    if (!object)
    {
        return "";
    }

    return object->GetName();
}

void CUndoBaseObjectSelect::Undo(bool bUndo)
{
    CBaseObject* object = GetIEditor()->GetObjectManager()->FindObject(m_guid);
    if (!object)
    {
        return;
    }

    if (bUndo)
    {
        m_bRedoSelect = object->IsSelected();
    }

    if (m_bUndoSelect)
    {
        GetIEditor()->GetObjectManager()->SelectObject(object);
    }
    else
    {
        GetIEditor()->GetObjectManager()->UnselectObject(object);
    }
}

void CUndoBaseObjectSelect::Redo()
{
    if (CBaseObject* object = GetIEditor()->GetObjectManager()->FindObject(m_guid))
    {
        if (m_bRedoSelect)
        {
            GetIEditor()->GetObjectManager()->SelectObject(object);
        }
        else
        {
            GetIEditor()->GetObjectManager()->UnselectObject(object);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// CUndoBaseObjectBulkSelect implementation.
//////////////////////////////////////////////////////////////////////////

CUndoBaseObjectBulkSelect::CUndoBaseObjectBulkSelect(const AZStd::unordered_set<const CBaseObject*>& previousSelection, const CSelectionGroup& selectionGroup)
{
    int numSelectedObjects = selectionGroup.GetCount();
    m_entityIdList.reserve(numSelectedObjects);

    // Populate list of entities to restore selection to 
    for (int objectIndex = 0; objectIndex < numSelectedObjects; ++objectIndex)
    {
        CBaseObject* object = selectionGroup.GetObject(objectIndex);

        // Don't track Undo/Redo for Legacy Objects or entities that were not selected in this step
        if (object->GetType() != OBJTYPE_AZENTITY || previousSelection.find(object) != previousSelection.end())
        {
            continue;
        }

        AZ::EntityId id;
        AzToolsFramework::ComponentEntityObjectRequestBus::EventResult(
            id, 
            object, 
            &AzToolsFramework::ComponentEntityObjectRequestBus::Events::GetAssociatedEntityId);

        m_entityIdList.push_back(id);
    }
}

void CUndoBaseObjectBulkSelect::Undo(bool bUndo)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);
    if (!bUndo)
    {
        return;
    }

    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::MarkEntitiesDeselected,
        m_entityIdList);
}

void CUndoBaseObjectBulkSelect::Redo()
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);

    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::MarkEntitiesSelected,
        m_entityIdList);
}

//////////////////////////////////////////////////////////////////////////
// CUndoBaseObjectClearSelection implementation.
//////////////////////////////////////////////////////////////////////////

CUndoBaseObjectClearSelection::CUndoBaseObjectClearSelection(const CSelectionGroup& selectionGroup)
{
    int numSelectedObjects = selectionGroup.GetCount();
    m_entityIdList.reserve(numSelectedObjects);

    // Populate list of entities to restore selection to 
    for (int objectIndex = 0; objectIndex < numSelectedObjects; ++objectIndex)
    {
        CBaseObject* object = selectionGroup.GetObject(objectIndex);

        // Don't track Undo/Redo for Legacy Objects
        if (object->GetType() != OBJTYPE_AZENTITY)
        {
            continue;
        }

        AZ::EntityId id;
        AzToolsFramework::ComponentEntityObjectRequestBus::EventResult(
            id, 
            object, 
            &AzToolsFramework::ComponentEntityObjectRequestBus::Events::GetAssociatedEntityId);

        m_entityIdList.push_back(id);
    }
}

void CUndoBaseObjectClearSelection::Undo(bool bUndo)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);

    if (!bUndo)
    {
        return;
    }

    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities,
        m_entityIdList);
}

void CUndoBaseObjectClearSelection::Redo() 
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);

    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities,
        AzToolsFramework::EntityIdList());
}

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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"
#include "PrefabItem.h"

#include "PrefabLibrary.h"
#include "PrefabManager.h"
#include "Prefabs/PrefabEvents.h"
#include "BaseLibraryManager.h"

#include "Grid.h"
#include "Cry_Math.h"

#include "HyperGraph/FlowGraph.h"
#include "HyperGraph/FlowGraphManager.h"

#include "Objects/PrefabObject.h"
#include "Objects/SelectionGroup.h"
#include "Objects/EntityObject.h"

#include "Undo/Undo.h"

//////////////////////////////////////////////////////////////////////////
CPrefabItem::CPrefabItem()
{
    m_PrefabClassName = PREFAB_OBJECT_CLASS_NAME;
    m_objectsNode = XmlHelpers::CreateXmlNode("Objects");
}

void CPrefabItem::SetPrefabClassName(QString prefabClassNameString)
{
    m_PrefabClassName = prefabClassNameString;
}

//////////////////////////////////////////////////////////////////////////
CPrefabItem::~CPrefabItem()
{
}

//////////////////////////////////////////////////////////////////////////
void CPrefabItem::Serialize(SerializeContext& ctx)
{
    CBaseLibraryItem::Serialize(ctx);
    XmlNodeRef node = ctx.node;
    if (ctx.bLoading)
    {
        XmlNodeRef objects = node->findChild("Objects");
        if (objects)
        {
            m_objectsNode = objects;
            // Flatten groups if not flatten already
            // The following code will just transform the previous nested groups to flatten hierarchy for easier operation implementation
            std::queue<XmlNodeRef> groupObjects;
            std::deque<XmlNodeRef> objectsToReattach;

            for (int i = 0, count = objects->getChildCount(); i < count; ++i)
            {
                XmlNodeRef objectNode = objects->getChild(i);
                QString objectType;
                objectNode->getAttr("Type", objectType);
                if (!objectType.compare("Group", Qt::CaseInsensitive))
                {
                    XmlNodeRef groupObjectsNode = objectNode->findChild("Objects");
                    if (groupObjectsNode)
                    {
                        groupObjects.push(groupObjectsNode);
                    }
                }

                // Process group
                while (!groupObjects.empty())
                {
                    XmlNodeRef group = groupObjects.front();
                    groupObjects.pop();

                    for (int j = 0, childCount = group->getChildCount(); j < childCount; ++j)
                    {
                        XmlNodeRef child = group->getChild(j);
                        QString childType;

                        child->getAttr("Type", childType);
                        if (!childType.compare("Group", Qt::CaseInsensitive))
                        {
                            XmlNodeRef groupObjectsNode = child->findChild("Objects");
                            if (groupObjectsNode)
                            {
                                objectsToReattach.push_back(child);
                                groupObjects.push(groupObjectsNode);
                            }
                        }
                        else
                        {
                            objectsToReattach.push_back(child);
                        }
                    }

                    group->getParent()->removeAllChilds();
                }
            }

            for (int i = 0, count = objectsToReattach.size(); i < count; ++i)
            {
                m_objectsNode->addChild(objectsToReattach[i]);
            }
        }
    }
    else
    {
        if (m_objectsNode)
        {
            node->addChild(m_objectsNode);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabItem::Update()
{
    // Mark library as modified.
    SetModified();
}

//////////////////////////////////////////////////////////////////////////
void CPrefabItem::MakeFromSelection(CSelectionGroup& fromSelection)
{
    if (fromSelection.IsEmpty())
    {
        return;
    }

    IObjectManager* pObjMan = GetIEditor()->GetObjectManager();
    CSelectionGroup selection;

    selection.Copy(fromSelection);

    // Remove any AZ entities from the selection group (not supported by legacy prefabs).
    bool warned = false;
    for (int objectIndex = 0; objectIndex < selection.GetCount(); )
    {
        CBaseObject* object = selection.GetObject(objectIndex);
        if (object && object->GetType() == OBJTYPE_AZENTITY)
        {
            if (!warned)
            {
                Warning("Object %s is a component entity and not compatible with legacy prefabs. Use Slices instead.", object->GetName().toUtf8().constData());
                warned = true;
            }
            selection.RemoveObject(object);
            objectIndex = 0;
        }
        else
        {
            ++objectIndex;
        }
    }
    if (selection.IsEmpty())
    {
        return;
    }

    if (selection.GetCount() == 1)
    {
        CBaseObject* pObject = selection.GetObject(0);
        if (qobject_cast<CPrefabObject*>(pObject))
        {
            CPrefabObject* pPrefabObject = (CPrefabObject*)pObject;
            pObjMan->SelectObject(pObject);
            return;
        }
    }

    // Snap center to grid.
    Vec3 vPivot = gSettings.pGrid->Snap(selection.GetBounds().min);

    _smart_ptr<CGroup> pCommonGroupObj = selection.GetObject(0)->GetGroup();
    TBaseObjects objectList;

    int nSelectionCount = selection.GetCount();
    for (int i = 0; i < nSelectionCount; i++)
    {
        CBaseObject* pObj = selection.GetObject(i);

        if (!pObj->GetGroup())
        {
            objectList.push_back(pObj);
        }
    }

    if (objectList.empty())
    {
        return;
    }

    CBaseObject* pPrefabObj = NULL;
    {
        //////////////////////////////////////////////////////////////////////////
        // Transform all objects in selection into local space of prefab.
        Matrix34 invParentTM;
        invParentTM.SetIdentity();
        invParentTM.SetTranslation(vPivot);
        invParentTM.Invert();

        CUndo undo("Make Prefab");
        for (int i = 0, selectionCount(objectList.size()); i < selectionCount; i++)
        {
            CBaseObject* pObj = objectList[i];

            if (pCommonGroupObj)
            {
                pCommonGroupObj->RemoveMember(pObj);
            }
            else if (pObj->GetParent())
            {
                continue;
            }

            Matrix34 localTM = invParentTM * pObj->GetWorldTM();
            pObj->SetLocalTM(localTM);
        }

        //////////////////////////////////////////////////////////////////////////
        // Save all objects in flat selection to XML.
        GetIEditor()->SuspendUndo();

        CSelectionGroup flatSelection;
        selection.FlattenHierarchy(flatSelection, true);

        m_objectsNode = XmlHelpers::CreateXmlNode("Objects");
        CObjectArchive ar(pObjMan, m_objectsNode, false);
        for (int i = 0, iFlatSelectionCount(flatSelection.GetCount()); i < iFlatSelectionCount; i++)
        {
            CBaseObject* pObj = flatSelection.GetObject(i);
            if (!pObj->IsPartOfPrefab())
            {
                ar.SaveObject(pObj, true, true);
            }
        }

        GetIEditor()->ResumeUndo();

        //////////////////////////////////////////////////////////////////////////
        // Delete objects in original selection.
        //////////////////////////////////////////////////////////////////////////
        pObjMan->DeleteSelection(&selection);

        //////////////////////////////////////////////////////////////////////////
        // Create prefab object.
        pPrefabObj = pObjMan->NewObject(PREFAB_OBJECT_CLASS_NAME);
        if (qobject_cast<CPrefabObject*>(pPrefabObj))
        {
            CPrefabObject* pConcretePrefabObj = (CPrefabObject*)pPrefabObj;

            pConcretePrefabObj->SetUniqueName(GetName());
            pConcretePrefabObj->SetPos(vPivot);
            pConcretePrefabObj->SetPrefab(this, false);

            if (pCommonGroupObj)
            {
                pCommonGroupObj->AddMember(pConcretePrefabObj);
            }

            if (selection.GetCount())
            {
                pConcretePrefabObj->SetLayer(selection.GetObject(0)->GetLayer());
            }
        }
        else if (pPrefabObj)
        {
            pObjMan->DeleteObject(pPrefabObj);
            pPrefabObj = 0;
        }

        if (pPrefabObj)
        {
            pObjMan->SelectObject(pPrefabObj);
            CPrefabObject* pConcretePrefabObj = (CPrefabObject*)pPrefabObj;
            TBaseObjects allChildren;
            pConcretePrefabObj->GetAllChildren(allChildren);
            for (int i = 0, iChildCounter(allChildren.size()); i < iChildCounter; ++i)
            {
                pObjMan->RegisterObjectName(allChildren[i]->GetName());
            }
        }
    }

    SetModified();
}

//////////////////////////////////////////////////////////////////////////
void CPrefabItem::UpdateFromPrefabObject(CPrefabObject* pPrefabObject, const SObjectChangedContext& context)
{
    CPrefabManager* pPrefabManager = GetIEditor()->GetPrefabManager();
    // Skip other prefab types
    if (!pPrefabObject || !pPrefabManager || context.m_modifiedObjectGuidInPrefab == GUID_NULL || pPrefabManager->ShouldSkipPrefabUpdate())
    {
        return;
    }

    if (pPrefabObject->IsModifyInProgress())
    {
        return;
    }

    CBaseObject* pObj = GetIEditor()->GetObjectManager()->FindObject(context.m_modifiedObjectGlobalId);
    if (!pObj)
    {
        return;
    }

    pPrefabObject->SetModifyInProgress(true);

    IObjectManager* pObjMan = GetIEditor()->GetObjectManager();
    CSelectionGroup selection;
    CSelectionGroup flatSelection;

    // Get all objects part of the prefab
    pPrefabObject->GetAllPrefabFlagedChildren(selection);

    CBaseObjectsArray allInstancedPrefabs;
    GetIEditor()->GetObjectManager()->FindObjectsOfType(&CPrefabObject::staticMetaObject, allInstancedPrefabs);

    // While modifying the instances we don't want the FG manager to send events since they can call into GUI components and cause a crash
    // e.g. we are modifying one FG but in fact we are working on multiple instances of this FG the MFC code does not cope well with this
    GetIEditor()->GetFlowGraphManager()->DisableNotifyListeners(true);

    {
        //////////////////////////////////////////////////////////////////////////
        // Save all objects in flat selection to XML.
        selection.FlattenHierarchy(flatSelection);

        // Serialize in the prefab lib (the main XML representation of the prefab)
        {
            CScopedSuspendUndo suspendUndo;
            TObjectIdMapping mapping;
            ExtractObjectsPrefabIDtoGuidMapping(flatSelection, mapping);
            ModifyLibraryPrefab(flatSelection, pPrefabObject, context, mapping);
        }

        SetModified();

        if (context.m_operation == eOCOT_ModifyTransformInLibOnly)
        {
            GetIEditor()->GetFlowGraphManager()->DisableNotifyListeners(false);
            pPrefabObject->SetModifyInProgress(false);
            return;
        }

        SObjectChangedContext modifiedContext = context;

        // If modify transform case
        if (context.m_operation == eOCOT_ModifyTransform)
        {
            modifiedContext.m_localTM = pObj->GetLocalTM();
        }

        // Now change all the rest instances of this prefab in the level (skipping this one, since it is already been modified)
        pPrefabManager->SetSelectedItem(this);
        GetIEditor()->GetFlowGraphManager()->SetGUIControlsProcessEvents(false, false);

        {
            CScopedSuspendUndo suspendUndo;

            for (size_t i = 0, prefabsCount = allInstancedPrefabs.size(); i < prefabsCount; ++i)
            {
                CPrefabObject* const pInstancedPrefab = static_cast<CPrefabObject*>(allInstancedPrefabs[i]);
                if (pPrefabObject != pInstancedPrefab && pInstancedPrefab->GetPrefabGuid() == pPrefabObject->GetPrefabGuid())
                {
                    CSelectionGroup instanceSelection;
                    CSelectionGroup instanceFlatSelection;

                    pInstancedPrefab->GetAllPrefabFlagedChildren(instanceSelection);

                    instanceSelection.FlattenHierarchy(instanceFlatSelection);

                    TObjectIdMapping mapping;
                    ExtractObjectsPrefabIDtoGuidMapping(instanceFlatSelection, mapping);

                    // Modify instanced prefabs in the level
                    pInstancedPrefab->SetModifyInProgress(true);
                    ModifyInstancedPrefab(instanceFlatSelection, pInstancedPrefab, modifiedContext, mapping);
                    pInstancedPrefab->SetModifyInProgress(false);
                }
            }
        } // ~CScopedSuspendUndo suspendUndo;

        GetIEditor()->GetFlowGraphManager()->SetGUIControlsProcessEvents(true, true);
    }

    GetIEditor()->GetFlowGraphManager()->DisableNotifyListeners(false);

    pPrefabObject->SetModifyInProgress(false);
}

//////////////////////////////////////////////////////////////////////////
void CPrefabItem::UpdateObjects()
{
    std::vector<CBaseObject*> pPrefabObjects;
    GetIEditor()->GetObjectManager()->FindObjectsOfType(&CPrefabObject::staticMetaObject, pPrefabObjects);
    for (int i = 0, iObjListSize(pPrefabObjects.size()); i < iObjListSize; ++i)
    {
        CPrefabObject* pPrefabObject = (CPrefabObject*)pPrefabObjects[i];
        if (pPrefabObject->GetPrefabGuid() == GetGUID())
        {
            // Even though this is not undo process, the undo flags should be true to avoid crashes caused by light anim sequence.
            pPrefabObject->SetPrefab(this, true);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabItem::ModifyLibraryPrefab(CSelectionGroup& objectsInPrefabAsFlatSelection, CPrefabObject* pPrefabObject, const SObjectChangedContext& context, const TObjectIdMapping& guidMapping)
{
    IObjectManager* const pObjManager = GetIEditor()->GetObjectManager();

    if (CBaseObject* const pObj = objectsInPrefabAsFlatSelection.GetObjectByGuidInPrefab(context.m_modifiedObjectGuidInPrefab))
    {
        // MEMBER ADD
        if (context.m_operation == eOCOT_Add)
        {
            // If we are trying to add something which already exists in the prefab definition we are trying to modify it
            if (FindObjectInThisPrefabItemByGuid(pObj->GetIdInPrefab(), true))
            {
                SObjectChangedContext correctedContext = context;
                correctedContext.m_operation = eOCOT_Modify;
                ModifyLibraryPrefab(objectsInPrefabAsFlatSelection, pPrefabObject, correctedContext, guidMapping);
                return;
            }

            CObjectArchive ar(pObjManager, m_objectsNode, false);

            if (pObj->GetParent() == pPrefabObject)
            {
                pObj->DetachThis(false);
            }

            ar.SaveObject(pObj, false, true);

            if (!pObj->GetParent())
            {
                pPrefabObject->AttachChild(pObj, false);
            }

            XmlNodeRef savedObj = m_objectsNode->getChild(m_objectsNode->getChildCount() - 1);
            RemapIDsInNodeAndChildren(savedObj, guidMapping, false);
        }
        // Member MODIFY
        else if (context.m_operation == eOCOT_Modify || context.m_operation == eOCOT_ModifyTransform || context.m_operation == eOCOT_ModifyTransformInLibOnly)
        {
            // Find -> remove previous definition of the object and serialize the new changes
            if (XmlNodeRef object = FindObjectInThisPrefabItemByGuid(pObj->GetIdInPrefab(), true))
            {
                m_objectsNode->removeChild(object);

                CObjectArchive ar(pObjManager, m_objectsNode, false);

                if (pObj->GetParent() == pPrefabObject)
                {
                    pObj->DetachThis(false);
                }

                ar.SaveObject(pObj, false, true);

                if (!pObj->GetParent())
                {
                    pPrefabObject->AttachChild(pObj, false);
                }

                XmlNodeRef savedObj = m_objectsNode->getChild(m_objectsNode->getChildCount() - 1);
                RemapIDsInNodeAndChildren(savedObj, guidMapping, false);
            }
        }
        // Member DELETE
        else if (context.m_operation == eOCOT_Delete)
        {
            if (XmlNodeRef object = FindObjectInThisPrefabItemByGuid(pObj->GetIdInPrefab(), true))
            {
                m_objectsNode->removeChild(object);

                // Clear the prefab references to this if it is a parent to objects and it is a GROUP
                QString objType;
                object->getAttr("Type", objType);
                if (!objType.compare("Group", Qt::CaseInsensitive))
                {
                    RemoveAllChildsOf(pObj->GetIdInPrefab());
                }
                else
                {
                    // A parent was deleted (which wasn't a group!!!) so just remove the parent attribute
                    for (int i = 0, count = m_objectsNode->getChildCount(); i < count; ++i)
                    {
                        GUID parentGuid = GUID_NULL;
                        if (m_objectsNode->getChild(i)->getAttr("Parent", parentGuid) && parentGuid == pObj->GetIdInPrefab())
                        {
                            m_objectsNode->getChild(i)->delAttr("Parent");
                        }
                    }
                }
            }
        } // ~Member DELETE
    } // ~if (CBaseObject* const pObj = objectsInPrefabAsFlatSelection.GetObjectByGuidInPrefab(context.m_modifiedObjectGuidInPrefab))
}

//////////////////////////////////////////////////////////////////////////
void CPrefabItem::ModifyInstancedPrefab(CSelectionGroup& objectsInPrefabAsFlatSelection, CPrefabObject* pPrefabObject, const SObjectChangedContext& context, const TObjectIdMapping& guidMapping)
{
    IObjectManager* const pObjManager = GetIEditor()->GetObjectManager();

    if (context.m_operation == eOCOT_Add)
    {
        if (CBaseObject* const pObj = pObjManager->FindObject(context.m_modifiedObjectGlobalId))
        {
            if (XmlNodeRef changedObject = FindObjectInThisPrefabItemByGuid(pObj->GetIdInPrefab(), false))
            {
                // See if we already have this object as part of the instanced prefab
                TBaseObjects childs;
                pPrefabObject->GetAllPrefabFlagedChildren(childs);
                if (CBaseObject* pModified = FindObjectByPrefabGuid(childs, pObj->GetIdInPrefab()))
                {
                    SObjectChangedContext correctedContext = context;
                    correctedContext.m_operation = eOCOT_Modify;
                    ModifyInstancedPrefab(objectsInPrefabAsFlatSelection, pPrefabObject, correctedContext, guidMapping);
                    return;
                }

                CObjectArchive loadAr(pObjManager, changedObject, true);
                loadAr.MakeNewIds(true);
                loadAr.EnableProgressBar(false);

                // PrefabGUID -> GUID
                RemapIDsInNodeAndChildren(changedObject, guidMapping, true);

                // Load/Clone
                CBaseObject* pClone = loadAr.LoadObject(changedObject);
                loadAr.ResolveObjects();

                RegisterPrefabEventFlowNodes(pClone);

                pPrefabObject->SetObjectPrefabFlagAndLayer(pClone);

                if (!pClone->GetParent())
                {
                    pPrefabObject->AttachChild(pClone, false);
                }

                // GUID -> PrefabGUID
                RemapIDsInNodeAndChildren(changedObject, guidMapping, false);
            }
        }
    }
    else if (CBaseObject* const pObj = objectsInPrefabAsFlatSelection.GetObjectByGuidInPrefab(context.m_modifiedObjectGuidInPrefab))
    {
        if (context.m_operation == eOCOT_Modify)
        {
            if (XmlNodeRef changedObject = FindObjectInThisPrefabItemByGuid(pObj->GetIdInPrefab(), false))
            {
                // Build load archive
                CObjectArchive loadAr(pObjManager, changedObject, true);
                loadAr.bUndo = true;
                loadAr.SetShouldResetInternalMembers(true);

                // Load ids remapping info into the load archive
                for (int j = 0, count = guidMapping.size(); j < count; ++j)
                {
                    loadAr.RemapID(guidMapping[j].from, guidMapping[j].to);
                }

                // PrefabGUID -> GUID
                RemapIDsInNodeAndChildren(changedObject, guidMapping, true);

                // Set parent attr if we dont have any (because we don't store this in the prefab XML since it is implicitly given)
                bool noParentIdAttr = false;
                if (!changedObject->haveAttr("Parent"))
                {
                    noParentIdAttr = true;
                    changedObject->setAttr("Parent", pPrefabObject->GetId());
                }

                // Load the object
                pObj->Serialize(loadAr);

                RegisterPrefabEventFlowNodes(pObj);

                pPrefabObject->SetObjectPrefabFlagAndLayer(pObj);

                // Remove parent attribute if it is directly a child of the prefab
                if (noParentIdAttr)
                {
                    changedObject->delAttr("Parent");
                }

                if (!pObj->GetParent())
                {
                    pPrefabObject->AttachChild(pObj, false);
                }

                // GUID -> PrefabGUID
                RemapIDsInNodeAndChildren(changedObject, guidMapping, false);
            }
        }
        else if (context.m_operation == eOCOT_ModifyTransform)
        {
            if (XmlNodeRef changedObject = FindObjectInThisPrefabItemByGuid(pObj->GetIdInPrefab(), false))
            {
                GUID parentGUID = GUID_NULL;
                changedObject->getAttr("Parent", parentGUID);
                if (CBaseObject* pObjParent = objectsInPrefabAsFlatSelection.GetObjectByGuidInPrefab(parentGUID))
                {
                    pObjParent->AttachChild(pObj);
                }
                pObj->SetLocalTM(context.m_localTM, eObjectUpdateFlags_Undo);
            }
        }
        else if (context.m_operation == eOCOT_Delete)
        {
            pObjManager->DeleteObject(pObj);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CPrefabItem::FindObjectInThisPrefabItemByGuid(REFGUID guid, bool forwardSearch)
{
    GUID objectId = GUID_NULL;

    if (forwardSearch)
    {
        for (int i = 0, count = m_objectsNode->getChildCount(); i < count; ++i)
        {
            m_objectsNode->getChild(i)->getAttr("Id", objectId);
            if (objectId == guid)
            {
                return m_objectsNode->getChild(i);
            }
        }
    }
    else
    {
        for (int i = m_objectsNode->getChildCount() - 1; i >= 0; --i)
        {
            m_objectsNode->getChild(i)->getAttr("Id", objectId);
            if (objectId == guid)
            {
                return m_objectsNode->getChild(i);
            }
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CBaseObjectPtr CPrefabItem::FindObjectByPrefabGuid(const TBaseObjects& objects, REFGUID guidInPrefab)
{
    for (TBaseObjects::const_iterator it = objects.begin(), end = objects.end(); it != end; ++it)
    {
        if ((*it)->GetIdInPrefab() == guidInPrefab)
        {
            return (*it);
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabItem::ExtractObjectsPrefabIDtoGuidMapping(CSelectionGroup& objects, TObjectIdMapping& mapping)
{
    mapping.reserve(objects.GetCount());
    std::queue<CBaseObject*> groups;

    for (int i = 0, count = objects.GetCount(); i < count; ++i)
    {
        CBaseObject* pObject = objects.GetObject(i);
        // Save group mapping
        mapping.push_back(SObjectIdMapping(pObject->GetIdInPrefab(), pObject->GetId()));
        // If this is a group recursively get all the objects ID mapping inside
        if (qobject_cast<CGroup*>(pObject))
        {
            groups.push(pObject);
            while (!groups.empty())
            {
                CBaseObject* pGroup = groups.front();
                groups.pop();

                for (int j = 0, childCount = pGroup->GetChildCount(); j < childCount; ++j)
                {
                    CBaseObject* pChild = pGroup->GetChild(j);
                    mapping.push_back(SObjectIdMapping(pChild->GetIdInPrefab(), pChild->GetId()));
                    // If group add it for further processing
                    if (qobject_cast<CGroup*>(pChild))
                    {
                        groups.push(pChild);
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
GUID CPrefabItem::ResolveID(const TObjectIdMapping& prefabIdToGuidMapping, GUID id, bool prefabIdToGuidDirection)
{
    if (prefabIdToGuidDirection)
    {
        for (int j = 0, count = prefabIdToGuidMapping.size(); j < count; ++j)
        {
            if (prefabIdToGuidMapping[j].from == id)
            {
                return prefabIdToGuidMapping[j].to;
            }
        }
    }
    else
    {
        for (int j = 0, count = prefabIdToGuidMapping.size(); j < count; ++j)
        {
            if (prefabIdToGuidMapping[j].to == id)
            {
                return prefabIdToGuidMapping[j].from;
            }
        }
    }

    return id;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabItem::RemapIDsInNodeAndChildren(XmlNodeRef objectNode, const TObjectIdMapping& mapping, bool prefabIdToGuidDirection)
{
    std::queue<XmlNodeRef> objects;
    const char* kFlowGraphTag = "FlowGraph";

    if (!objectNode->isTag(kFlowGraphTag))
    {
        RemapIDsInNode(objectNode, mapping, prefabIdToGuidDirection);
    }

    for (int i = 0; i < objectNode->getChildCount(); ++i)
    {
        objects.push(objectNode->getChild(i));
        // Recursively traverse all objects and exclude flowgraph parts
        while (!objects.empty())
        {
            XmlNodeRef object = objects.front();
            // Skip flowgraph
            if (object->isTag(kFlowGraphTag))
            {
                objects.pop();
                continue;
            }

            RemapIDsInNode(object, mapping, prefabIdToGuidDirection);
            objects.pop();

            for (int j = 0, childCount = object->getChildCount(); j < childCount; ++j)
            {
                XmlNodeRef child = object->getChild(j);
                if (child->isTag(kFlowGraphTag))
                {
                    continue;
                }

                RemapIDsInNode(child, mapping, prefabIdToGuidDirection);

                if (child->getChildCount())
                {
                    objects.push(child);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabItem::RemapIDsInNode(XmlNodeRef objectNode, const TObjectIdMapping& mapping, bool prefabIdToGuidDirection)
{
    GUID patchedId;
    if (objectNode->getAttr("Id", patchedId))
    {
        objectNode->setAttr("Id", ResolveID(mapping, patchedId, prefabIdToGuidDirection));
    }
    if (objectNode->getAttr("Parent", patchedId))
    {
        objectNode->setAttr("Parent", ResolveID(mapping, patchedId, prefabIdToGuidDirection));
    }
    if (objectNode->getAttr("LookAt", patchedId))
    {
        objectNode->setAttr("LookAt", ResolveID(mapping, patchedId, prefabIdToGuidDirection));
    }
    if (objectNode->getAttr("TargetId", patchedId))
    {
        objectNode->setAttr("TargetId", ResolveID(mapping, patchedId, prefabIdToGuidDirection));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabItem::RemoveAllChildsOf(GUID guid)
{
    std::deque<XmlNodeRef> childrenToRemove;
    std::deque<GUID> guidsToCheckAgainst;
    GUID id = GUID_NULL;

    for (int i = 0, count = m_objectsNode->getChildCount(); i < count; ++i)
    {
        XmlNodeRef child = m_objectsNode->getChild(i);
        if (child->getAttr("Parent", id) && guid == id)
        {
            childrenToRemove.push_front(child);
            QString objType;
            if (child->getAttr("Type", objType) && !objType.compare("Group", Qt::CaseInsensitive))
            {
                GUID groupId;
                child->getAttr("Id", groupId);
                guidsToCheckAgainst.push_back(groupId);
            }
        }
    }

    for (std::deque<XmlNodeRef>::iterator it = childrenToRemove.begin(), end = childrenToRemove.end(); it != end; ++it)
    {
        m_objectsNode->removeChild((*it));
    }

    for (int i = 0, count = guidsToCheckAgainst.size(); i < count; ++i)
    {
        RemoveAllChildsOf(guidsToCheckAgainst[i]);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabItem::RegisterPrefabEventFlowNodes(CBaseObject* const pEntityObj)
{
    if (qobject_cast<CEntityObject*>(pEntityObj))
    {
        CEntityObject* pEntity = static_cast<CEntityObject*>(pEntityObj);
        if (CFlowGraph* pFG = pEntity->GetFlowGraph())
        {
            IHyperGraphEnumerator* pEnumerator = pFG->GetNodesEnumerator();
            IHyperNode* pNode = pEnumerator->GetFirst();
            while (pNode)
            {
                GetIEditor()->GetPrefabManager()->GetPrefabEvents()->OnHyperGraphManagerEvent(EHG_NODE_ADD, pNode->GetGraph(), pNode);
                pNode = pEnumerator->GetNext();
            }
            pEnumerator->Release();
        }
    }
}
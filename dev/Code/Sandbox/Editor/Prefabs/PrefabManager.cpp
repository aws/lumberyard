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
#include "PrefabManager.h"

#include "PrefabItem.h"
#include "PrefabLibrary.h"

#include "GameEngine.h"

#include "DataBaseDialog.h"
#include "PrefabDialog.h"

#include "Objects/PrefabObject.h"

#include "PrefabEvents.h"

#define PREFABS_LIBS_PATH "Prefabs/"

//////////////////////////////////////////////////////////////////////////
// CUndoGroupObjectOpenClose implementation.
//////////////////////////////////////////////////////////////////////////
class CUndoGroupObjectOpenClose
    : public IUndoObject
{
public:
    CUndoGroupObjectOpenClose(CPrefabObject* prefabObj)
    {
        m_prefabObject = prefabObj;
        m_bOpenForUndo = m_prefabObject->IsOpen();
    }
protected:
    virtual int GetSize() { return sizeof(*this); }; // Return size of xml state.
    virtual QString GetDescription() { return "Prefab's Open/Close"; };

    virtual void Undo(bool bUndo)
    {
        m_bRedo = bUndo;
        Apply(m_bOpenForUndo);
    }
    virtual void Redo()
    {
        if (m_bRedo)
        {
            Apply(!m_bOpenForUndo);
        }
    }

    void Apply(bool bOpen)
    {
        if (m_prefabObject)
        {
            if (bOpen)
            {
                m_prefabObject->Open();
            }
            else
            {
                m_prefabObject->Close();
            }
        }
    }

private:

    CPrefabObjectPtr m_prefabObject;
    bool m_bRedo;
    bool m_bOpenForUndo;
};

//////////////////////////////////////////////////////////////////////////
// CUndoAddObjectsToPrefab implementation.
//////////////////////////////////////////////////////////////////////////
CUndoAddObjectsToPrefab::CUndoAddObjectsToPrefab(CPrefabObject* prefabObj, TBaseObjects& objects)
{
    m_pPrefabObject = prefabObj;

    // Rearrange to parent-first
    for (int i = 0; i < objects.size(); i++)
    {
        if (objects[i]->GetParent())
        {
            // Find later in array.
            for (int j = i + 1; j < objects.size(); j++)
            {
                if (objects[j] == objects[i]->GetParent())
                {
                    // Swap the objects.
                    std::swap(objects[i], objects[j]);
                    i--;
                    break;
                }
            }
        }
    }

    m_addedObjects.reserve(objects.size());
    for (size_t i = 0, count = objects.size(); i < count; ++i)
    {
        m_addedObjects.push_back(SObjectsLinks());
        SObjectsLinks& addedObject = m_addedObjects.back();

        addedObject.m_object = objects[i]->GetId();
        // Store parent before the add operation
        addedObject.m_objectParent = objects[i]->GetParent() ? objects[i]->GetParent()->GetId() : GUID_NULL;
        // Store childs before the add operation
        if (const size_t childsCount = objects[i]->GetChildCount())
        {
            addedObject.m_objectsChilds.reserve(childsCount);
            for (size_t j = 0; j < childsCount; ++j)
            {
                addedObject.m_objectsChilds.push_back(objects[i]->GetChild(j)->GetId());
            }
        }
    }
}

void CUndoAddObjectsToPrefab::Undo(bool bUndo)
{
    // Start from the back where the childs are
    for (int i = m_addedObjects.size() - 1; i >= 0; --i)
    {
        IObjectManager* pObjMan = GetIEditor()->GetObjectManager();

        // Remove from prefab
        if (CBaseObject* pMember = pObjMan->FindObject(m_addedObjects[i].m_object))
        {
            m_pPrefabObject->RemoveMember(pMember);
            // Restore parent links
            if (CBaseObject* pParent = pObjMan->FindObject(m_addedObjects[i].m_objectParent))
            {
                pParent->AttachChild(pMember);
            }

            // Restore child links
            if (const int childsCount = m_addedObjects[i].m_objectsChilds.size())
            {
                for (int j = 0; j < childsCount; ++j)
                {
                    if (CBaseObject* pChild = pObjMan->FindObject(m_addedObjects[i].m_objectsChilds[j]))
                    {
                        pMember->AttachChild(pChild);
                    }
                }
            }
        }
    }
}

void CUndoAddObjectsToPrefab::Redo()
{
    for (int i = 0, count = m_addedObjects.size(); i < count; ++i)
    {
        if (CBaseObject* pMember = GetIEditor()->GetObjectManager()->FindObject(m_addedObjects[i].m_object))
        {
            m_pPrefabObject->AddMember(pMember);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// CPrefabManager implementation.
//////////////////////////////////////////////////////////////////////////
CPrefabManager::CPrefabManager()
    : m_pPrefabEvents(NULL)
{
    m_bUniqNameMap = true;
    m_pLevelLibrary = (CBaseLibrary*)AddLibrary("Level", true);

    m_pPrefabEvents = new CPrefabEvents();

    m_skipPrefabUpdate = false;
}

//////////////////////////////////////////////////////////////////////////
CPrefabManager::~CPrefabManager()
{
    SAFE_DELETE(m_pPrefabEvents);
}

//////////////////////////////////////////////////////////////////////////
void CPrefabManager::ClearAll()
{
    CBaseLibraryManager::ClearAll();

    m_pPrefabEvents->RemoveAllEventData();

    m_pLevelLibrary = (CBaseLibrary*)AddLibrary("Level", true);
}

//////////////////////////////////////////////////////////////////////////
CBaseLibraryItem* CPrefabManager::MakeNewItem()
{
    return new CPrefabItem;
}
//////////////////////////////////////////////////////////////////////////
CBaseLibrary* CPrefabManager::MakeNewLibrary()
{
    return new CPrefabLibrary(this);
}
//////////////////////////////////////////////////////////////////////////
QString CPrefabManager::GetRootNodeName()
{
    return "PrefabsLibrary";
}
//////////////////////////////////////////////////////////////////////////
QString CPrefabManager::GetLibsPath()
{
    if (m_libsPath.isEmpty())
    {
        m_libsPath = PREFABS_LIBS_PATH;
    }
    return m_libsPath;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabManager::Serialize(XmlNodeRef& node, bool bLoading)
{
    CBaseLibraryManager::Serialize(node, bLoading);
}

//////////////////////////////////////////////////////////////////////////
CPrefabItem* CPrefabManager::MakeFromSelection()
{
    // Prevent making legacy prefabs out of selections that include component entities
    // Component entities use slices as their prefab mechanism
    CSelectionGroup* selection = GetIEditor()->GetSelection();

    for (size_t i = 0; i < selection->GetCount(); ++i)
    {
        auto* object = selection->GetObject(i);
        if (OBJTYPE_AZENTITY == object->GetType())
        {
            CryMessageBox("Component Entities can not be added to prefab objects.\nPlease deselect any component entities and try again.\n\nTo construct pre-configured groups of component entities, create a slice instead.", "Invalid Selection", MB_OK | MB_ICONERROR);
            return 0;
        }
    }

    CBaseLibraryDialog* dlg = GetIEditor()->OpenDataBaseLibrary(EDB_TYPE_PREFAB);
    CPrefabDialog* pPrefabDialog = qobject_cast<CPrefabDialog*>(dlg);
    if (pPrefabDialog)
    {
        return pPrefabDialog->GetPrefabFromSelection();
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabManager::AddSelectionToPrefab()
{
    CSelectionGroup* pSel = GetIEditor()->GetSelection();
    CPrefabObject* pPrefab = 0;
    int selectedPrefabCount = 0;
    for (int i = 0; i < pSel->GetCount(); i++)
    {
        CBaseObject* pObj = pSel->GetObject(i);
        if (qobject_cast<CPrefabObject*>(pObj))
        {
            ++selectedPrefabCount;
            pPrefab = (CPrefabObject*) pObj;
        }
    }
    if (selectedPrefabCount == 0)
    {
        Warning("Select a prefab and objects");
        return;
    }
    if (selectedPrefabCount > 1)
    {
        Warning("Select only one prefab");
        return;
    }


    TBaseObjects objects;
    for (int i = 0; i < pSel->GetCount(); i++)
    {
        CBaseObject* pObj = pSel->GetObject(i);
        if (pObj != pPrefab)
        {
            objects.push_back(pObj);
        }
    }

    // Check objects if they can be added
    bool invalidAddOperation = false;
    for (int i = 0, count = objects.size(); i < count; ++i)
    {
        if (objects[i]->GetType() == OBJTYPE_AZENTITY)
        {
            // Component entities cannot be added to legacy prefabs.
            Warning("Object %s is a component entity and not compatible with legacy prefabs. Use Slices instead.", objects[i]->GetName().toUtf8().constData());
            invalidAddOperation = true;
        }
        else if (!pPrefab->CanObjectBeAddedAsMember(objects[i]))
        {
            Warning("Object %s is already part of a prefab (%s)", objects[i]->GetName().toUtf8().constData(), objects[i]->GetPrefab()->GetName().toUtf8().constData());
            invalidAddOperation = true;
        }
    }

    if (invalidAddOperation)
    {
        return;
    }

    CUndo undo("Add Objects To Prefab");
    if (CUndo::IsRecording())
    {
        CUndo::Record(new CUndoAddObjectsToPrefab(pPrefab, objects));
    }

    for (int i = 0; i < objects.size(); i++)
    {
        pPrefab->AddMember(objects[i]);
    }

    // If we have nested dependencies between these object send an modify event afterwards to resolve them properly (e.g. shape objects linked to area triggers)
    for (int i = 0; i < objects.size(); i++)
    {
        objects[i]->UpdatePrefab();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabManager::ExtractObjectsFromPrefabs(std::vector<CBaseObject*>& childObjects, bool bSelectExtracted)
{
    if (childObjects.empty())
    {
        return;
    }

    CUndo undo("Extract Object(s) from Prefab");
    CSelectionGroup extractedObjects;
    std::map<CPrefabObject*, CSelectionGroup> prefabObjectsToBeExtracted;

    for (int i = 0, childCount = childObjects.size(); i < childCount; ++i)
    {
        if (CPrefabObject* pPrefab = childObjects[i]->GetPrefab())
        {
            CSelectionGroup& selGroup = prefabObjectsToBeExtracted[pPrefab];
            ExpandGroup(childObjects[i], selGroup);
        }
    }

    std::map<CPrefabObject*, CSelectionGroup>::iterator it = prefabObjectsToBeExtracted.begin();
    std::map<CPrefabObject*, CSelectionGroup>::iterator end = prefabObjectsToBeExtracted.end();
    for (; it != end; ++it)
    {
        (it->first)->CloneSelected(&(it->second), &extractedObjects);
    }

    for (int i = 0, childCount = childObjects.size(); i < childCount; ++i)
    {
        CPrefabObject* pPrefab = childObjects[i]->GetPrefab();

        for (int j = 0, count = childObjects[i]->GetChildCount(); j < count; ++j)
        {
            CBaseObject* pChildToReparent = childObjects[i]->GetChild(j);
            pPrefab->AttachChild(pChildToReparent);
            pChildToReparent->UpdatePrefab();
        }

        GetIEditor()->GetObjectManager()->DeleteObject(childObjects[i]);
    }

    if (bSelectExtracted)
    {
        SelectObjectsIgnoringUndo(extractedObjects);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabManager::ExtractAllFromPrefabs(std::vector<CPrefabObject*>& pPrefabs, bool bSelectExtracted)
{
    if (pPrefabs.empty())
    {
        return;
    }

    CSelectionGroup selectedPrefabs;
    for (int i = 0, count = pPrefabs.size(); i < count; i++)
    {
        if (qobject_cast<CPrefabObject*>(pPrefabs[i]))
        {
            selectedPrefabs.AddObject(pPrefabs[i]);
        }
    }

    CUndo undo("Extract All from Prefab(s)");

    CSelectionGroup extractedObjects;
    for (int i = 0, count = selectedPrefabs.GetCount(); i < count; i++)
    {
        if (CPrefabObject* pPrefab = static_cast<CPrefabObject*>(selectedPrefabs.GetObject(i)))
        {
            pPrefab->CloneAll(&extractedObjects);
        }
    }

    if (bSelectExtracted)
    {
        SelectObjectsIgnoringUndo(extractedObjects);
    }

    for (int i = 0, count = selectedPrefabs.GetCount(); i < count; i++)
    {
        GetIEditor()->DeleteObject(selectedPrefabs.GetObject(i));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabManager::CloneObjectsFromPrefabs(std::vector<CBaseObject*>& childObjects, bool bSelectCloned)
{
    if (childObjects.empty())
    {
        return;
    }

    CUndo undo("Clone Object(s) from Prefab");
    CSelectionGroup clonedObjects;
    CSelectionGroup selectedObjects;

    for (int i = 0; i < childObjects.size(); ++i)
    {
        ExpandGroup(childObjects[i], selectedObjects);
    }


    if (CPrefabObject* pPrefab = childObjects[0]->GetPrefab())
    {
        pPrefab->CloneSelected(&selectedObjects, &clonedObjects);
    }

    if (bSelectCloned)
    {
        SelectObjectsIgnoringUndo(clonedObjects);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabManager::CloneAllFromPrefabs(std::vector<CPrefabObject*>& pPrefabs, bool bSelectCloned)
{
    if (pPrefabs.empty())
    {
        return;
    }

    CSelectionGroup selectedPrefabs;
    for (int i = 0, count = pPrefabs.size(); i < count; i++)
    {
        if (qobject_cast<CPrefabObject*>(pPrefabs[i]))
        {
            selectedPrefabs.AddObject(pPrefabs[i]);
        }
    }

    CUndo undo("Clone All from Prefab(s)");

    CSelectionGroup clonedObjects;
    for (int i = 0, count = selectedPrefabs.GetCount(); i < count; i++)
    {
        static_cast<CPrefabObject*>(selectedPrefabs.GetObject(i))->CloneAll(&clonedObjects);
    }

    if (bSelectCloned)
    {
        SelectObjectsIgnoringUndo(clonedObjects);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabManager::OpenPrefabs(std::vector<CPrefabObject*>& prefabObjects, const char* undoDescription)
{
    if (prefabObjects.empty())
    {
        return false;
    }

    bool bOpenedAtLeastOne = false;

    CUndo undo(undoDescription);

    for (int i = 0, iCount(prefabObjects.size()); i < iCount; ++i)
    {
        CPrefabObject* pPrefabObj = (CPrefabObject*)prefabObjects[i];
        if (!pPrefabObj->IsOpen())
        {
            if (CUndo::IsRecording())
            {
                CUndo::Record(new CUndoGroupObjectOpenClose(pPrefabObj));
            }

            pPrefabObj->Open();
            bOpenedAtLeastOne = true;
        }
    }

    return bOpenedAtLeastOne;
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabManager::ClosePrefabs(std::vector<CPrefabObject*>& prefabObjects, const char* undoDescription)
{
    if (prefabObjects.empty())
    {
        return false;
    }

    bool bClosedAtLeastOne = false;

    CUndo undo(undoDescription);

    for (int i = 0, iCount(prefabObjects.size()); i < iCount; ++i)
    {
        CPrefabObject* pPrefabObj = (CPrefabObject*)prefabObjects[i];
        if (pPrefabObj->IsOpen())
        {
            if (CUndo::IsRecording())
            {
                CUndo::Record(new CUndoGroupObjectOpenClose(pPrefabObj));
            }
            pPrefabObj->Close();
            bClosedAtLeastOne = true;
        }
    }

    return bClosedAtLeastOne;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabManager::GetPrefabObjects(std::vector<CPrefabObject*>& outPrefabObjects)
{
    std::vector<CBaseObject*> prefabObjects;
    GetIEditor()->GetObjectManager()->FindObjectsOfType(&CPrefabObject::staticMetaObject, prefabObjects);
    if (prefabObjects.empty())
    {
        return;
    }

    for (int i = 0, iCount(prefabObjects.size()); i < iCount; ++i)
    {
        CPrefabObject* pPrefabObj = (CPrefabObject*)prefabObjects[i];
        if (pPrefabObj == NULL)
        {
            continue;
        }

        outPrefabObjects.push_back(pPrefabObj);
    }
}

//////////////////////////////////////////////////////////////////////////
int CPrefabManager::GetPrefabInstanceCount(CPrefabItem* pPrefabItem)
{
    int instanceCount = 0;
    std::vector<CPrefabObject*> prefabObjects;
    GetPrefabObjects(prefabObjects);

    if (pPrefabItem)
    {
        for (int i = 0, prefabsFound(prefabObjects.size()); i < prefabsFound; ++i)
        {
            CPrefabObject* pPrefabObject = (CPrefabObject*)prefabObjects[i];
            if (pPrefabObject->GetPrefab() == pPrefabItem)
            {
                ++instanceCount;
            }
        }
    }

    return instanceCount;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabManager::DeleteItem(IDataBaseItem* pItem)
{
    assert(pItem);

    CPrefabItem* pPrefabItem = (CPrefabItem*)pItem;

    // Delete all objects from object manager that have this prefab item
    std::vector<GUID> guids;
    CBaseObjectsArray objects;
    IObjectManager* pObjMan = GetIEditor()->GetObjectManager();
    pObjMan->GetObjects(objects);
    for (int i = 0; i < objects.size(); ++i)
    {
        CBaseObject* pObj = objects[i];
        if (qobject_cast<CPrefabObject*>(pObj))
        {
            CPrefabObject* pPrefab = (CPrefabObject*) pObj;
            if (pPrefab->GetPrefab() == pPrefabItem)
            {
                // Collect guids first to delete objects later
                guids.push_back(pPrefab->GetId());
            }
        }
    }
    for (int i = 0; i < guids.size(); ++i)
    {
        CBaseObject* pObj = pObjMan->FindObject(guids[i]);
        if (pObj)
        {
            pObjMan->DeleteObject(pObj);
        }
    }

    CBaseLibraryManager::DeleteItem(pItem);
}

//////////////////////////////////////////////////////////////////////////
void CPrefabManager::SelectObjectsIgnoringUndo(CSelectionGroup& extractedObjects) const
{
    GetIEditor()->ClearSelection();
    GetIEditor()->SuspendUndo();
    for (int i = 0, count = extractedObjects.GetCount(); i < count; ++i)
    {
        GetIEditor()->SelectObject(extractedObjects.GetObject(i));
    }
    GetIEditor()->ResumeUndo();
}

//////////////////////////////////////////////////////////////////////////
void CPrefabManager::ExpandGroup(CBaseObject* pObject, CSelectionGroup& selection) const
{
    selection.AddObject(pObject);
    if (qobject_cast<CGroup*>(pObject) && !qobject_cast<CPrefabObject*>(pObject))
    {
        CGroup* pGroup = static_cast<CGroup*>(pObject);
        const TBaseObjects& groupMembers = pGroup->GetMembers();
        for (int i = 0, count = groupMembers.size(); i < count; ++i)
        {
            ExpandGroup(groupMembers[i], selection);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
IDataBaseLibrary* CPrefabManager::LoadLibrary(const QString& filename, bool bReload)
{
    IDataBaseLibrary* pLibrary = CBaseLibraryManager::LoadLibrary(filename, bReload);
    if (bReload && pLibrary)
    {
        CPrefabLibrary* pPrefabLibrary = (CPrefabLibrary*)pLibrary;
        pPrefabLibrary->UpdatePrefabObjects();
    }
    return pLibrary;
}
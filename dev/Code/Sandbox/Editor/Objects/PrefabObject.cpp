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
#include "PrefabObject.h"

#include "Prefabs/PrefabManager.h"
#include "Prefabs/PrefabDialog.h"
#include "Prefabs/PrefabLibrary.h"
#include "Prefabs/PrefabItem.h"
#include "PrefabPanel.h"
#include "BaseLibraryManager.h"
#include "QtViewPaneManager.h"
#include "../Controls/RollupBar.h"

#include "Grid.h"

#include "../Viewport.h"
#include "../DisplaySettings.h"

#include "Util/GuidUtil.h"
#include "Objects/EntityObject.h"

#include "Objects/EntityObject.h"
#include "Undo/Undo.h"

//////////////////////////////////////////////////////////////////////////
// CPrefabObject implementation.
//////////////////////////////////////////////////////////////////////////

namespace
{
    int s_rollupId = 0;
    CPrefabPanel* s_panel = 0;
}

//////////////////////////////////////////////////////////////////////////
CUndoChangePivot::CUndoChangePivot(CBaseObject* pObj, const char* undoDescription)
{
    // Stores the current state of this object.
    assert(pObj != nullptr);
    m_undoDescription = undoDescription;
    m_guid = pObj->GetId();

    m_undoPivotPos = pObj->GetWorldPos();
}

//////////////////////////////////////////////////////////////////////////
QString CUndoChangePivot::GetObjectName()
{
    CBaseObject* object = GetIEditor()->GetObjectManager()->FindObject(m_guid);
    if (!object)
    {
        return QString();
    }

    return object->GetName();
}

//////////////////////////////////////////////////////////////////////////
void CUndoChangePivot::Undo(bool bUndo)
{
    CBaseObject* object = GetIEditor()->GetObjectManager()->FindObject(m_guid);
    if (!object)
    {
        return;
    }

    if (bUndo)
    {
        m_redoPivotPos = object->GetWorldPos();
    }

    static_cast<CPrefabObject*>(object)->SetPivot(m_undoPivotPos);
}

//////////////////////////////////////////////////////////////////////////
void CUndoChangePivot::Redo()
{
    CBaseObject* object = GetIEditor()->GetObjectManager()->FindObject(m_guid);
    if (!object)
    {
        return;
    }

    static_cast<CPrefabObject*>(object)->SetPivot(m_redoPivotPos);
}

//////////////////////////////////////////////////////////////////////////
CPrefabObject::CPrefabObject()
{
    SetColor(QColor(255, 220, 0)); // Yellowish
    ZeroStruct(m_prefabGUID);

    m_bbox.min = m_bbox.max = Vec3(0, 0, 0);
    m_bBBoxValid = false;
    m_bModifyInProgress = false;
    m_bChangePivotPoint = false;
    m_bSettingPrefabObj = false;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::Done()
{
    m_pPrefabItem = 0;

    GetIEditor()->SuspendUndo();
    DeleteAllPrefabObjects();
    GetIEditor()->ResumeUndo();
    CBaseObject::Done();
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabObject::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    bool res = CBaseObject::Init(ie, prev, file);
    if (prev)
    {
        // Cloning.
        SetPrefab(((CPrefabObject*)prev)->m_pPrefabItem, false);

        CPrefabObject* prevObj = (CPrefabObject*)prev;

        if (prevObj)
        {
            if (prevObj->IsOpen())
            {
                Open();
            }
        }
    }
    else if (!file.isEmpty())
    {
        SetPrefab(GuidUtil::FromString(file.toUtf8().data()), false);
    }
    return res;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::BeginEditParams(IEditor* ie, int flags)
{
    CBaseObject::BeginEditParams(ie, flags);

    if (!s_panel)
    {
        s_panel = new CPrefabPanel;
        s_rollupId = ie->AddRollUpPage(ROLLUP_OBJECTS, tr("Prefab Parameters"), s_panel);
    }
    if (s_panel)
    {
        s_panel->SetObject(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::EndEditParams(IEditor* ie)
{
    if (s_panel)
    {
        ie->RemoveRollUpPage(ROLLUP_OBJECTS, s_rollupId);
    }
    s_rollupId = 0;
    s_panel = 0;

    CBaseObject::EndEditParams(ie);
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::BeginEditMultiSelParams(bool bAllOfSameType)
{
    CBaseObject::BeginEditMultiSelParams(bAllOfSameType);

    if (!bAllOfSameType)
    {
        return;
    }

    if (!s_panel)
    {
        s_panel = new CPrefabPanel;
        s_rollupId = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS, tr("Prefab Parameters"), s_panel);
    }
    if (s_panel)
    {
        s_panel->SetObject(NULL);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::EndEditMultiSelParams()
{
    if (s_panel)
    {
        GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS, s_rollupId);
    }
    s_rollupId = 0;
    s_panel = 0;

    CBaseObject::EndEditMultiSelParams();
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::OnMenuProperties()
{
    if (!IsSelected())
    {
        CUndo undo("Select Object");
        GetIEditor()->GetObjectManager()->ClearSelection();
        GetIEditor()->SelectObject(this);
    }

    QtViewPaneManager::instance()->OpenPane(LyViewPane::LegacyRollupBar);
    GetIEditor()->SelectRollUpBar(ROLLUP_OBJECTS);
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::ConvertToProceduralObject()
{
    GetIEditor()->GetObjectManager()->ClearSelection();

    GetIEditor()->SuspendUndo();
    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedEntities);
    CBaseObject* pObject = GetIEditor()->GetObjectManager()->NewObject("Entity", 0, "ProceduralObject");
    if (!pObject)
    {
        QString sError = "Could not convert prefab to " + this->GetName();
        CryMessageBox(sError.toUtf8().data(), "Conversion Failure", MB_OKCANCEL | MB_ICONERROR);
        return;
    }

    QString sName = this->GetName();
    pObject->SetName(sName + "_prc");
    pObject->SetWorldTM(this->GetWorldTM());

    pObject->SetLayer(GetLayer());
    GetIEditor()->SelectObject(pObject);

    GetIEditor()->SelectObject(pObject);

    CEntityObject* pEntityObject = static_cast<CEntityObject*>(pObject);

    CPrefabItem* pPrefab = static_cast<CPrefabObject*>(this)->GetPrefab();
    if (pPrefab)
    {
        IDataBaseLibrary* prefabLibrary = pPrefab->GetLibrary();
        if (prefabLibrary)
        {
            // The library needs to be saved because changing the prefab library path will trigger a load from disk of the library.
            prefabLibrary->Save();
            pEntityObject->SetEntityPropertyString("filePrefabLibrary", prefabLibrary->GetFilename());
        }

        QString sPrefabName = pPrefab->GetFullName();
        pEntityObject->SetEntityPropertyString("sPrefabVariation", sPrefabName);
    }

    GetIEditor()->GetObjectManager()->DeleteObject(this);

    QtViewPaneManager::instance()->OpenPane(LyViewPane::LegacyRollupBar);
    GetIEditor()->SelectRollUpBar(ROLLUP_OBJECTS);
    GetIEditor()->ResumeUndo();
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::OnContextMenu(QMenu* menu)
{
    if (!menu->isEmpty())
    {
        menu->addSeparator();
    }
    CUsedResources resources;
    GatherUsedResources(resources);

    menu->addAction("Properties", functor(*this, &CPrefabObject::OnMenuProperties));
    menu->addAction("Convert to Procedural Object", functor(*this, &CPrefabObject::ConvertToProceduralObject));

    //TODO: to be re-added when AssetBrowser is loading fast
    //menu->Add("Show in Asset Browser", functor(*this, &CBaseObject::OnMenuShowInAssetBrowser)).Enable(!resources.files.empty());
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::Display(DisplayContext& dc)
{
    DrawDefault(dc, GetColor());

    dc.PushMatrix(GetWorldTM());

    bool bSelected = IsSelected();
    if (bSelected)
    {
        dc.SetSelectedColor();
        dc.DrawWireBox(m_bbox.min, m_bbox.max);

        dc.DepthWriteOff();
        dc.SetSelectedColor(0.2f);
        dc.DrawSolidBox(m_bbox.min, m_bbox.max);
        dc.DepthWriteOn();
    }
    else
    {
        if (gSettings.viewports.bAlwaysDrawPrefabBox)
        {
            if (IsFrozen())
            {
                dc.SetFreezeColor();
            }
            else
            {
                dc.SetColor(GetColor(), 0.2f);
            }

            dc.DepthWriteOff();
            ;
            dc.DrawSolidBox(m_bbox.min, m_bbox.max);
            dc.DepthWriteOn();

            if (IsFrozen())
            {
                dc.SetFreezeColor();
            }
            else
            {
                dc.SetColor(GetColor());
            }
            dc.DrawWireBox(m_bbox.min, m_bbox.max);
        }
    }
    dc.PopMatrix();

    if (bSelected || gSettings.viewports.bAlwaysDrawPrefabInternalObjects || IsHighlighted() || IsOpen())
    {
        if (HaveChilds())
        {
            int numObjects = GetChildCount();
            for (int i = 0; i < numObjects; i++)
            {
                RecursivelyDisplayObject(GetChild(i), dc);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::RecursivelyDisplayObject(CBaseObject* obj, DisplayContext& dc)
{
    if (!obj->CheckFlags(OBJFLAG_PREFAB) || obj->IsHidden())
    {
        return;
    }

    AABB bbox;
    obj->GetBoundBox(bbox);
    if (dc.flags & DISPLAY_2D)
    {
        if (dc.box.IsIntersectBox(bbox))
        {
            obj->Display(dc);
        }
    }
    else
    {
        if (dc.camera && dc.camera->IsAABBVisible_F(AABB(bbox.min, bbox.max)))
        {
            obj->Display(dc);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    int numObjects = obj->GetChildCount();
    for (int i = 0; i < numObjects; i++)
    {
        RecursivelyDisplayObject(obj->GetChild(i), dc);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::Serialize(CObjectArchive& ar)
{
    bool bSuspended(SuspendUpdate(false));
    CBaseObject::Serialize(ar);
    if (bSuspended)
    {
        ResumeUpdate();
    }

    if (ar.bLoading)
    {
        ar.node->getAttr("PrefabGUID", m_prefabGUID);
    }
    else
    {
        ar.node->setAttr("PrefabGUID", m_prefabGUID);
        ar.node->setAttr("PrefabName", m_prefabName.toUtf8().data());
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::PostLoad(CObjectArchive& ar)
{
    CGroup::PostLoad(ar);

    SetPrefab(m_prefabGUID, true);
    uint32 nLayersMask = GetMaterialLayersMask();
    if (nLayersMask)
    {
        SetMaterialLayersMask(nLayersMask);
    }

    // If all children are Designer Objects, this prefab object should have an open status.
    int iChildCount(GetChildCount());

    if (iChildCount > 0)
    {
        bool bAllDesignerObject = true;

        for (int i = 0; i < iChildCount; ++i)
        {
            if (GetChild(i)->GetType() != OBJTYPE_SOLID)
            {
                bAllDesignerObject = false;
            }
        }

        if (bAllDesignerObject)
        {
            Open();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CPrefabObject::Export(const QString& levelPath, XmlNodeRef& xmlNode)
{
    // Do not export.
    return 0;
}

//////////////////////////////////////////////////////////////////////////
inline void RecursivelySendEventToPrefabChilds(CBaseObject* obj, ObjectEvent event)
{
    for (int i = 0; i < obj->GetChildCount(); i++)
    {
        CBaseObject* c = obj->GetChild(i);
        if (c->CheckFlags(OBJFLAG_PREFAB))
        {
            c->OnEvent(event);
            if (c->GetChildCount() > 0)
            {
                RecursivelySendEventToPrefabChilds(c, event);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::OnEvent(ObjectEvent event)
{
    switch (event)
    {
    case EVENT_PREFAB_REMAKE:
    {
        GetIEditor()->GetPrefabManager()->SetSkipPrefabUpdate(true);
        SetPrefab(GetPrefab(), true);
        GetIEditor()->GetPrefabManager()->SetSkipPrefabUpdate(false);
        break;
    }
    }
    ;
    // Send event to all prefab childs.
    RecursivelySendEventToPrefabChilds(this, event);
    CBaseObject::OnEvent(event);
}


//////////////////////////////////////////////////////////////////////////
void CPrefabObject::DeleteAllPrefabObjects()
{
    TBaseObjects childs;
    GetAllPrefabFlagedChildren(childs);
    DetachAll();
    for (int i = 0; i < childs.size(); i++)
    {
        GetObjectManager()->DeleteObject(childs[i]);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::SetPrefab(REFGUID guid, bool bForceReload)
{
    if (m_prefabGUID == guid && bForceReload == false)
    {
        return;
    }

    m_prefabGUID = guid;

    //m_fullPrototypeName = prototypeName;
    CPrefabManager* pManager = GetIEditor()->GetPrefabManager();
    CPrefabItem* pPrefab = (CPrefabItem*)pManager->FindItem(guid);
    if (pPrefab)
    {
        SetPrefab(pPrefab, bForceReload);
    }
    else
    {
        if (m_prefabName.isEmpty())
        {
            m_prefabName = "Unknown Prefab";
        }

        CErrorRecord err;
        err.error = tr("Cannot find Prefab %1 with GUID: %2 for Object %3").arg(m_prefabName, GuidUtil::ToString(guid), GetName());
        err.pObject = this;
        err.severity = CErrorRecord::ESEVERITY_WARNING;
        GetIEditor()->GetErrorReport()->ReportError(err);
        SetMinSpec(GetMinSpec(), true);   // to make sure that all children get the right spec
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::SetPrefab(CPrefabItem* pPrefab, bool bForceReload)
{
    assert(pPrefab);

    if (pPrefab == m_pPrefabItem && !bForceReload)
    {
        return;
    }

    // Prefab events needs to be notified to delay determining event data till after prefab is set (Only then is name + instance name determined)
    CPrefabManager* pPrefabManager = GetIEditor()->GetPrefabManager();
    CRY_ASSERT(pPrefabManager != NULL);

    GetIEditor()->SuspendUndo();

    // Delete all child objects.
    bool bSuspended(SuspendUpdate(false));
    DeleteAllPrefabObjects();
    if (bSuspended)
    {
        ResumeUpdate();
    }

    GetIEditor()->ResumeUndo();

    m_pPrefabItem = pPrefab;
    m_prefabGUID = pPrefab->GetGUID();
    m_prefabName = pPrefab->GetFullName();

    StoreUndo("Set Prefab");

    GetIEditor()->SuspendUndo();

    // Make objects from this prefab.
    XmlNodeRef objects = pPrefab->GetObjectsNode();
    if (!objects)
    {
        CErrorRecord err;
        err.error = tr("Prefab %1 does not contain objects").arg(m_prefabName);
        err.pObject = this;
        err.severity = CErrorRecord::ESEVERITY_WARNING;
        GetIEditor()->GetErrorReport()->ReportError(err);
        GetIEditor()->ResumeUndo();
        return;
    }

    CObjectLayer* pThisLayer = GetLayer();

    //////////////////////////////////////////////////////////////////////////
    // Spawn objects.
    //////////////////////////////////////////////////////////////////////////
    pPrefabManager->SetSkipPrefabUpdate(true);

    CObjectArchive ar(GetObjectManager(), objects, true);
    ar.EnableProgressBar(false); // No progress bar is shown when loading objects.
    ar.MakeNewIds(true);
    ar.EnableReconstructPrefabObject(true);
    ar.LoadObjects(objects);
    GetObjectManager()->ForceID(GetId().Data1);//force using this ID, incremental
    ar.ResolveObjects();
    int numObjects = ar.GetLoadedObjectsCount();
    for (int i = 0; i < numObjects; i++)
    {
        CBaseObject* obj = ar.GetLoadedObject(i);

        // Only attach objects without a parent object to this prefab.
        if (obj && !obj->GetParent())
        {
            AttachChild(obj, false);
        }

        SetObjectPrefabFlagAndLayer(obj);
    }
    GetObjectManager()->ForceID(0);//disable
    InvalidateBBox();

    SyncParentObject();

    //Previous calls can potentially set up the skip prefab update to false so reset it again since we are not changing the prefab skip the lib update
    pPrefabManager->SetSkipPrefabUpdate(true);

    SetMinSpec(GetMinSpec(), true);   // to make sure that all children get the right spec

    pPrefabManager->SetSkipPrefabUpdate(false);

    GetIEditor()->ResumeUndo();
}

//////////////////////////////////////////////////////////////////////////
CPrefabItem* CPrefabObject::GetPrefab() const
{
    return m_pPrefabItem;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::SetObjectPrefabFlagAndLayer(CBaseObject* object)
{
    object->SetFlags(OBJFLAG_PREFAB);
    object->SetLayer(GetLayer());
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::InitObjectPrefabId(CBaseObject* object)
{
    if (object->GetIdInPrefab() == GUID_NULL)
    {
        object->SetIdInPrefab(object->GetId());
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx)
{
    CBaseObject* pFromParent = pFromObject->GetParent();
    if (pFromParent)
    {
        CBaseObject* pChildParent = ctx.FindClone(pFromParent);
        if (pChildParent)
        {
            pChildParent->AddMember(this, false);
        }
        else
        {
            pFromParent->AddMember(this, false);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabObject::HitTest(HitContext& hc)
{
    if (IsOpen())
    {
        return CGroup::HitTest(hc);
    }
    else
    {
        if (CGroup::HitTest(hc))
        {
            hc.object = this;
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::CloneAll(CSelectionGroup* pExtractedObjects /* = NULL*/)
{
    if (!m_pPrefabItem || !m_pPrefabItem->GetObjectsNode())
    {
        return;
    }

    // Take the prefab lib representation and clone it
    XmlNodeRef objectsNode = m_pPrefabItem->GetObjectsNode();

    const Matrix34 prefabPivotTM = GetWorldTM();

    CObjectArchive clonedObjectArchive(GetObjectManager(), objectsNode, true);
    clonedObjectArchive.EnableProgressBar(false); // No progress bar is shown when loading objects.
    clonedObjectArchive.MakeNewIds(true);
    clonedObjectArchive.EnableReconstructPrefabObject(true);
    clonedObjectArchive.LoadObjects(objectsNode);
    clonedObjectArchive.ResolveObjects();

    if (pExtractedObjects)
    {
        CScopedSuspendUndo suspendUndo;
        for (int i = 0, numObjects = clonedObjectArchive.GetLoadedObjectsCount(); i < numObjects; ++i)
        {
            CBaseObject* pClonedObject = clonedObjectArchive.GetLoadedObject(i);

            // Add to selection
            pClonedObject->SetIdInPrefab(GUID_NULL);
            // If we don't have a parent transform with the world matrix
            if (!pClonedObject->GetParent())
            {
                pClonedObject->SetWorldTM(prefabPivotTM * pClonedObject->GetWorldTM());
            }

            pExtractedObjects->AddObject(pClonedObject);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::CloneSelected(CSelectionGroup* pSelectedGroup, CSelectionGroup* pClonedObjects)
{
    if (pSelectedGroup == NULL || !pSelectedGroup->GetCount())
    {
        return;
    }

    XmlNodeRef objectsNode = XmlHelpers::CreateXmlNode("Objects");
    std::map<GUID, XmlNodeRef, guid_less_predicate> objects;
    for (int i = 0, count = pSelectedGroup->GetCount(); i < count; ++i)
    {
        CBaseObject* pSelectedObj = pSelectedGroup->GetObject(i);
        XmlNodeRef serializedObject = m_pPrefabItem->FindObjectInThisPrefabItemByGuid(pSelectedObj->GetIdInPrefab(), true);
        if (!serializedObject)
        {
            return;
        }

        XmlNodeRef cloneObject = serializedObject->clone();

        GUID cloneObjectID = GUID_NULL;
        if (cloneObject->getAttr("Id", cloneObjectID))
        {
            objects[cloneObjectID] = cloneObject;
        }

        objectsNode->addChild(cloneObject);
    }

    CSelectionGroup allPrefabChilds;
    GetAllPrefabFlagedChildren(allPrefabChilds);

    std::vector<Matrix34> clonedObjectsPivotLocalTM;

    const Matrix34 prefabPivotTM = GetWorldTM();
    const Matrix34 prefabPivotInvTM = prefabPivotTM.GetInverted();

    // Delete outside referenced objects which were not part of the selected Group
    for (int i = 0, count = objectsNode->getChildCount(); i < count; ++i)
    {
        XmlNodeRef object = objectsNode->getChild(i);
        GUID objectID = GUID_NULL;
        object->getAttr("Id", objectID);
        // If parent is not part of the selection remove it
        if (object->getAttr("Parent", objectID) && objects.find(objectID) == objects.end())
        {
            object->delAttr("Parent");
        }

        const CBaseObject* pChild = pSelectedGroup->GetObjectByGuidInPrefab(objectID);
        const Matrix34 childTM = pChild->GetWorldTM();
        const Matrix34 childRelativeToPivotTM = prefabPivotInvTM * childTM;

        clonedObjectsPivotLocalTM.push_back(childRelativeToPivotTM);
    }

    CObjectArchive clonedObjectArchive(GetObjectManager(), objectsNode, true);
    clonedObjectArchive.EnableProgressBar(false); // No progress bar is shown when loading objects.
    clonedObjectArchive.MakeNewIds(true);
    clonedObjectArchive.EnableReconstructPrefabObject(true);
    clonedObjectArchive.LoadObjects(objectsNode);
    clonedObjectArchive.ResolveObjects();

    if (pClonedObjects)
    {
        CScopedSuspendUndo suspendUndo;
        for (int i = 0, numObjects = clonedObjectArchive.GetLoadedObjectsCount(); i < numObjects; ++i)
        {
            CBaseObject* pClonedObject = clonedObjectArchive.GetLoadedObject(i);

            // Add to selection
            pClonedObject->SetIdInPrefab(GUID_NULL);
            // If we don't have a parent transform with the world matrix
            if (!pClonedObject->GetParent())
            {
                pClonedObject->SetWorldTM(prefabPivotTM * clonedObjectsPivotLocalTM[i]);
            }

            pClonedObjects->AddObject(pClonedObject);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::AddMember(CBaseObject* pObj, bool bKeepPos /*=true */)
{
    if (!m_pPrefabItem)
    {
        SetPrefab(m_prefabGUID, true);
        if (!m_pPrefabItem)
        {
            return;
        }
    }
    if (qobject_cast<CPrefabObject*>(pObj))
    {
        if (static_cast<CPrefabObject*>(pObj)->m_pPrefabItem == m_pPrefabItem)
        {
            Warning("Object has the same prefab item");
            return;
        }
    }

    GetIEditor()->SuspendUndo();

    if (!pObj->IsChildOf(this) && !pObj->GetParent())
    {
        AttachChild(pObj, bKeepPos);
    }

    SetObjectPrefabFlagAndLayer(pObj);
    InitObjectPrefabId(pObj);

    SObjectChangedContext context;
    context.m_operation = eOCOT_Add;
    context.m_modifiedObjectGlobalId = pObj->GetId();
    context.m_modifiedObjectGuidInPrefab = pObj->GetIdInPrefab();

    SyncPrefab(context);

    GetIEditor()->ResumeUndo();

    GetIEditor()->GetObjectManager()->NotifyPrefabObjectChanged(this);
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::RemoveMember(CBaseObject* pObj, bool bKeepPos /*=true */)
{
    if (!m_pPrefabItem)
    {
        SetPrefab(m_prefabGUID, true);
        if (!m_pPrefabItem)
        {
            return;
        }
    }
    if (pObj)
    {
        SObjectChangedContext context;
        context.m_operation = eOCOT_Delete;
        context.m_modifiedObjectGuidInPrefab = pObj->GetIdInPrefab();
        context.m_modifiedObjectGlobalId = pObj->GetId();

        SyncPrefab(context);

        if (pObj->GetGroup() && pObj->GetGroup() == this)
        {
            pObj->DetachThis();
        }

        pObj->ClearFlags(OBJFLAG_PREFAB);

        GetIEditor()->GetObjectManager()->NotifyPrefabObjectChanged(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::SyncPrefab(const SObjectChangedContext& context)
{
    //Group delete
    if (CBaseObject* pObj = GetIEditor()->GetObjectManager()->FindObject(context.m_modifiedObjectGlobalId))
    {
        if (qobject_cast<CGroup*>(pObj) && !qobject_cast<CPrefabObject*>(pObj) && context.m_operation == eOCOT_Delete)
        {
            TBaseObjects childs;
            for (int i = 0, count = pObj->GetChildCount(); i < count; ++i)
            {
                childs.push_back(pObj->GetChild(i));
            }

            for (int i = 0, count = childs.size(); i < count; ++i)
            {
                RemoveMember(childs[i], true);
            }
        }
    }

    if (m_pPrefabItem)
    {
        m_pPrefabItem->UpdateFromPrefabObject(this, context);
    }

    //Group add
    if (CBaseObject* pObj = GetIEditor()->GetObjectManager()->FindObject(context.m_modifiedObjectGlobalId))
    {
        if (qobject_cast<CGroup*>(pObj) && !qobject_cast<CPrefabObject*>(pObj) && context.m_operation == eOCOT_Add)
        {
            for (int i = 0, count = pObj->GetChildCount(); i < count; ++i)
            {
                AddMember(pObj->GetChild(i), true);
            }
        }
    }

    InvalidateBBox();
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::SyncParentObject()
{
    if (GetParent() && GetParent()->GetType() == OBJTYPE_GROUP)
    {
        static_cast<CGroup*>(GetParent())->InvalidateBBox();
    }
}

//////////////////////////////////////////////////////////////////////////
static void Prefab_RecursivelyGetBoundBox(CBaseObject* object, AABB& box, const Matrix34& parentTM)
{
    if (!object->CheckFlags(OBJFLAG_PREFAB))
    {
        return;
    }

    Matrix34 worldTM = parentTM * object->GetLocalTM();
    AABB b;
    object->GetLocalBounds(b);
    b.SetTransformedAABB(worldTM, b);
    box.Add(b.min);
    box.Add(b.max);

    int numChilds = object->GetChildCount();
    if (numChilds > 0)
    {
        for (int i = 0; i < numChilds; i++)
        {
            Prefab_RecursivelyGetBoundBox(object->GetChild(i), box, worldTM);
        }
    }
}

/////////////////////////////////////////////////////////////////////////
void CPrefabObject::CalcBoundBox()
{
    Matrix34 identityTM;
    identityTM.SetIdentity();

    // Calc local bounds box..
    AABB box;
    box.Reset();

    int numChilds = GetChildCount();
    for (int i = 0; i < numChilds; i++)
    {
        if (GetChild(i)->CheckFlags(OBJFLAG_PREFAB))
        {
            Prefab_RecursivelyGetBoundBox(GetChild(i), box, identityTM);
        }
    }

    if (numChilds == 0)
    {
        box.min = Vec3(-1, -1, -1);
        box.max = Vec3(1, 1, 1);
    }

    m_bbox = box;
    m_bBBoxValid = true;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::AttachChild(CBaseObject* child, bool bKeepPos)
{
    CBaseObject::AttachChild(child, bKeepPos);
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::RemoveChild(CBaseObject* child)
{
    CBaseObject::RemoveChild(child);
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::SetMaterial(CMaterial* pMaterial)
{
    if (pMaterial)
    {
        for (int i = 0; i < GetChildCount(); i++)
        {
            GetChild(i)->SetMaterial(pMaterial);
        }
    }
    CBaseObject::SetMaterial(pMaterial);
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::SetWorldTM(const Matrix34& tm, int flags /* = 0 */)
{
    if (m_bChangePivotPoint)
    {
        SetPivot(tm.GetTranslation());
    }
    else
    {
        CBaseObject::SetWorldTM(tm, flags);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::SetWorldPos(const Vec3& pos, int flags /* = 0 */)
{
    if (m_bChangePivotPoint)
    {
        SetPivot(pos);
    }
    else
    {
        CBaseObject::SetWorldPos(pos, flags);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabObject::SetMaterialLayersMask(uint32 nLayersMask)
{
    for (int i = 0; i < GetChildCount(); i++)
    {
        if (GetChild(i)->CheckFlags(OBJFLAG_PREFAB))
        {
            GetChild(i)->SetMaterialLayersMask(nLayersMask);
        }
    }

    CBaseObject::SetMaterialLayersMask(nLayersMask);
}

void CPrefabObject::SetName(const QString& name)
{
    const QString oldName = GetName();

    CGroup::SetName(name);
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabObject::HitTestMembers(HitContext& hcOrg)
{
    float mindist = FLT_MAX;

    HitContext hc = hcOrg;

    CBaseObject* selected = 0;
    TBaseObjects allChildrenObj;
    GetAllPrefabFlagedChildren(allChildrenObj);
    int numberOfChildren = allChildrenObj.size();
    for (int i = 0; i < numberOfChildren; ++i)
    {
        CBaseObject* pObj = allChildrenObj[i];

        if (pObj == this || pObj->IsFrozen() || pObj->IsHidden())
        {
            continue;
        }

        if (!GetObjectManager()->HitTestObject(pObj, hc))
        {
            continue;
        }

        if (hc.dist >= mindist)
        {
            continue;
        }

        mindist = hc.dist;

        if (hc.object)
        {
            selected = hc.object;
        }
        else
        {
            selected = pObj;
        }

        hc.object = 0;
    }

    if (selected)
    {
        hcOrg.object = selected;
        hcOrg.dist = mindist;
        return true;
    }
    return false;
}

bool CPrefabObject::SuspendUpdate(bool bForceSuspend)
{
    if (m_bSettingPrefabObj)
    {
        return false;
    }

    if (!m_pPrefabItem)
    {
        if (!bForceSuspend)
        {
            return false;
        }
        if (m_prefabGUID == GUID_NULL)
        {
            return false;
        }
        m_bSettingPrefabObj = true;
        SetPrefab(m_prefabGUID, true);
        m_bSettingPrefabObj = false;
        if (!m_pPrefabItem)
        {
            return false;
        }
    }

    return true;
}

void CPrefabObject::ResumeUpdate()
{
    if (!m_pPrefabItem || m_bSettingPrefabObj)
    {
        return;
    }
}

bool CPrefabObject::CanObjectBeAddedAsMember(CBaseObject* pObject)
{
    if (pObject->CheckFlags(OBJFLAG_PREFAB))
    {
        CBaseObject* pParentObject = pObject->GetParent();
        while (pParentObject)
        {
            if (qobject_cast<CPrefabObject*>(pParentObject))
            {
                return GetPrefabGuid() == static_cast<CPrefabObject*>(pParentObject)->GetPrefabGuid();
            }
            pParentObject = pParentObject->GetParent();
        }
    }
    return true;
}

void CPrefabObject::UpdatePivot(const Vec3& newWorldPivotPos)
{
    // Update this prefab pivot
    SetModifyInProgress(true);
    const Matrix34 worldTM = GetWorldTM();
    const Matrix34 invWorldTM = worldTM.GetInverted();
    const Vec3 prefabPivotLocalSpace = invWorldTM.TransformPoint(newWorldPivotPos);

    CGroup::UpdatePivot(newWorldPivotPos);
    SetModifyInProgress(false);

    TBaseObjects childs;
    childs.reserve(GetChildCount());
    // Cache childs ptr because in the update prefab we are modifying the m_childs array since we are attaching/detaching before we save in the prefab lib xml
    for (int i = 0, iChildCount = GetChildCount(); i < iChildCount; ++i)
    {
        childs.push_back(GetChild(i));
    }

    // Update the prefab lib and reposition all prefab childs according to the new pivot
    for (int i = 0, iChildCount = childs.size(); i < iChildCount; ++i)
    {
        childs[i]->UpdatePrefab(eOCOT_ModifyTransformInLibOnly);
    }

    // Update all the rest prefab instance of the same type
    CBaseObjectsArray objects;
    GetObjectManager()->FindObjectsOfType(&CPrefabObject::staticMetaObject, objects);

    for (int i = 0, iCount(objects.size()); i < iCount; ++i)
    {
        CPrefabObject* const pPrefabInstanceObj = static_cast<CPrefabObject*>(objects[i]);
        if (pPrefabInstanceObj->GetPrefabGuid() != GetPrefabGuid() || pPrefabInstanceObj == this)
        {
            continue;
        }

        pPrefabInstanceObj->SetModifyInProgress(true);
        const Matrix34 prefabInstanceWorldTM = pPrefabInstanceObj->GetWorldTM();
        const Vec3 prefabInstancePivotPoint = prefabInstanceWorldTM.TransformPoint(prefabPivotLocalSpace);
        pPrefabInstanceObj->CGroup::UpdatePivot(prefabInstancePivotPoint);
        pPrefabInstanceObj->SetModifyInProgress(false);
    }
}

void CPrefabObject::SetPivot(const Vec3& newWorldPivotPos)
{
    if (CUndo::IsRecording())
    {
        CUndo::Record(new CUndoChangePivot(this, "Change pivot of Prefab"));
    }
    UpdatePivot(newWorldPivotPos);
}
void CPrefabObject::GenerateUniqueName()
{
    if (m_pPrefabItem)
    {
        SetUniqueName(m_pPrefabItem->GetName());
    }
    else
    {
        CBaseObject::GenerateUniqueName();
    }
}

#include <Objects/PrefabObject.moc>

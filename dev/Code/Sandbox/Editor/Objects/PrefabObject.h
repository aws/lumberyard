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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_PREFABOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_PREFABOBJECT_H
#pragma once

#include "Group.h"
#include "Prefabs/PrefabItem.h"

class CPopupMenuItem;

#define PREFAB_OBJECT_CLASS_NAME "Prefab"
#define CATEGORY_PREFABS "Prefab"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//! Undo object for change pivot point of a prefab
class CUndoChangePivot
    : public IUndoObject
{
public:
    CUndoChangePivot(CBaseObject* obj, const char* undoDescription);

protected:
    virtual int GetSize() { return sizeof(*this); }
    virtual QString GetDescription() { return m_undoDescription; };
    virtual QString GetObjectName();

    virtual void Undo(bool bUndo);
    virtual void Redo();

private:

    GUID m_guid;
    QString m_undoDescription;
    Vec3 m_undoPivotPos;
    Vec3 m_redoPivotPos;
};

/*!
*       CPrefabObject is prefabricated object which can contain multiple other objects, in a group like manner,
        but internal objects can not be modified, they are only created from PrefabItem.
*/
class SANDBOX_API CPrefabObject
    : public CGroup
{
    Q_OBJECT
public:
    //////////////////////////////////////////////////////////////////////////
    // CGroup overrides.
    //////////////////////////////////////////////////////////////////////////
    bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    void Done();

    void Display(DisplayContext& disp);
    bool HitTest(HitContext& hc);

    void BeginEditParams(IEditor* ie, int flags);
    void EndEditParams(IEditor* ie);

    void BeginEditMultiSelParams(bool bAllOfSameType);
    void EndEditMultiSelParams();

    void OnEvent(ObjectEvent event);

    void Serialize(CObjectArchive& ar);
    void PostLoad(CObjectArchive& ar);

    void SetMaterialLayersMask(uint32 nLayersMask);

    void SetName(const QString& name);

    void AddMember(CBaseObject* pMember, bool bKeepPos = true);
    void RemoveMember(CBaseObject* pMember, bool bKeepPos = true);

    //! Attach new child node.
    void AttachChild(CBaseObject* child, bool bKeepPos = true);

    REFGUID GetPrefabGuid() const   { return m_prefabGUID; }

    void SetMaterial(CMaterial* pMaterial);
    void SetWorldTM(const Matrix34& tm, int flags = 0) override;
    void SetWorldPos(const Vec3& pos, int flags = 0) override;
    void UpdatePivot(const Vec3& newWorldPivotPos) override;
    void SetPivot(const Vec3& newWorldPivotPos);

protected:
    virtual void RemoveChild(CBaseObject* child);

public:
    virtual XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);
    //////////////////////////////////////////////////////////////////////////

public:
    //////////////////////////////////////////////////////////////////////////
    // CPrefabObject.
    //////////////////////////////////////////////////////////////////////////
    virtual void SetPrefab(REFGUID guid, bool bForceReload);
    virtual void SetPrefab(CPrefabItem* pPrefab, bool bForceReload);
    CPrefabItem* GetPrefab() const;

    // Extract all objects inside.
    void CloneAll(CSelectionGroup* pExtractedObjects = NULL);
    void SyncPrefab(const SObjectChangedContext& context);
    bool SuspendUpdate(bool bForceSuspend = true);
    void ResumeUpdate();
    bool CanObjectBeAddedAsMember(CBaseObject* pObject);
    void InitObjectPrefabId(CBaseObject* object);
    bool IsModifyInProgress() const { return m_bModifyInProgress; }
    void SetModifyInProgress(const bool inProgress) { m_bModifyInProgress = inProgress; }
    void CloneSelected(CSelectionGroup* pSelectedGroup, CSelectionGroup* pClonedObjects);
    void SetObjectPrefabFlagAndLayer(CBaseObject* object);
    void SetChangePivotMode(bool changePivotMode) { m_bChangePivotPoint = changePivotMode; }

    void OnContextMenu(QMenu* menu) override;

    //! Generate unique object name based on the prefab's base name
    void GenerateUniqueName() override;

protected:
    friend class CTemplateObjectClassDesc<CPrefabObject>;

    //! Dtor must be protected.
    CPrefabObject();

    static const GUID& GetClassID()
    {
        // {931962ED-450F-443e-BFA4-1BBDAA061202}
        static const GUID guid = {
            0x931962ed, 0x450f, 0x443e, { 0xbf, 0xa4, 0x1b, 0xbd, 0xaa, 0x6, 0x12, 0x2 }
        };
        return guid;
    }

    bool HitTestMembers(HitContext& hcOrg);
    virtual void PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx);
    virtual void CalcBoundBox();
    void DeleteThis() { delete this; };

    void RecursivelyDisplayObject(CBaseObject* object, DisplayContext& dc);
    void DeleteAllPrefabObjects();

    void SyncParentObject();

    void OnMenuProperties();
    void ConvertToProceduralObject();

protected:
    _smart_ptr<CPrefabItem> m_pPrefabItem;

    QString m_prefabName;
    GUID m_prefabGUID;
    bool m_bModifyInProgress;
    bool m_bChangePivotPoint;
    bool m_bSettingPrefabObj;

    //////////////////////////////////////////////////////////////////////////
    // Per Instance Entity events.
    //////////////////////////////////////////////////////////////////////////
};


#endif // CRYINCLUDE_EDITOR_OBJECTS_PREFABOBJECT_H


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

// Description : CSelection group definition.


#ifndef CRYINCLUDE_EDITOR_OBJECTS_SELECTIONGROUP_H
#define CRYINCLUDE_EDITOR_OBJECTS_SELECTIONGROUP_H
#pragma once


class CBaseObject;
class CBrushObject;
class CGeomEntity;

#include "ObjectEvent.h"

#ifdef GetObject
#undef GetObject
#endif

/*!
 *  CSelectionGroup is a named selection group of objects.
 */
class SANDBOX_API CSelectionGroup
{
public:
    CSelectionGroup();

    //! Set name of selection.
    void SetName(const QString& name) { m_name = name; };
    //! Get name of selection.
    const QString& GetName() const { return m_name; };

    //! Adds object into selection list.
    void AddObject(CBaseObject* obj);
    //! Remove object from selection list.
    void RemoveObject(CBaseObject* obj);
    //! Remove all objects from selection.
    void RemoveAll();
    //! Remove all objects from selection except for the LegacyObjects list
    //! This is used in a performance improvement for deselecting legacy objects
    void RemoveAllExceptLegacySet();
    //! Check if object contained in selection list.
    bool IsContainObject(CBaseObject* obj);
    //! Return true if selection doesnt contain any object.
    bool IsEmpty() const;
    //! Check if all selected objects are of same type
    bool SameObjectType();
    //! Number of selected object.
    int  GetCount() const;
    //! Number of selected object.
    void     ObjectModified() const;
    //! Get object at given index.
    CBaseObject* GetObject(int index) const;
    //! Get object from a GUID
    CBaseObject* GetObjectByGuid(REFGUID guid) const;
    //! Get object from a GUID in a prefab
    CBaseObject* GetObjectByGuidInPrefab(REFGUID guid) const;
    //! Get set of legacy objects
    std::set<CBaseObjectPtr>& GetLegacyObjects();

    //! Get mass center of selected objects.
    Vec3    GetCenter() const;

    //! Get Bounding box of selection.
    AABB GetBounds() const;

    void    Copy(const CSelectionGroup& from);

    //! Remove from selection group all objects which have parent also in selection group.
    //! And save resulting objects to saveTo selection.
    void    FilterParents();
    //! Get number of child filtered objects.
    int GetFilteredCount() const { return m_filtered.size(); }
    CBaseObject* GetFilteredObject(int i) const { return m_filtered[i]; }

    //////////////////////////////////////////////////////////////////////////
    // Operations on selection group.
    //////////////////////////////////////////////////////////////////////////
    enum EMoveSelectionFlag
    {
        eMS_None = 0x00,
        eMS_FollowTerrain = 0x01,
        eMS_FollowGeometryPosNorm = 0x02
    };
    //! Move objects in selection by offset.
    void Move(const Vec3& offset, EMoveSelectionFlag moveFlag, int referenceCoordSys, const QPoint& point = QPoint(-1, -1));
    //! Move objects in selection to specific position.
    void MoveTo(const Vec3& pos, EMoveSelectionFlag moveFlag, int referenceCoordSys, const QPoint& point = QPoint(-1, -1));
    //! Rotate objects in selection by given quaternion.
    void Rotate(const Quat& qRot, int referenceCoordSys);
    //! Rotate objects in selection by given angle.
    void Rotate(const Ang3& angles, int referenceCoordSys);
    //! Rotate objects in selection by given rotation matrix.
    void Rotate(const Matrix34& matRot, int referenceCoordSys);
    //! Transforms objects
    void Transform(const Vec3& offset, EMoveSelectionFlag moveFlag, const Ang3& angles, const Vec3& scale, int referenceCoordSys);
    //! Resets rotation and scale to identity and (1.0f, 1.0f, 1.0f)
    void ResetTransformation();
    //! Scale objects in selection by given scale.
    void StartScaling();
    void Scale(const Vec3& scale, int referenceCoordSys);
    void SetScale(const Vec3& scale, int referenceCoordSys);
    void FinishScaling(const Vec3& scale, int referenceCoordSys);
    //! Align objects in selection to surface normal
    void Align();
    //! Very special method to move contents of a voxel.
    void MoveContent(const Vec3& offset);

    //////////////////////////////////////////////////////////////////////////
    //! Clone objects in this group and add cloned objects to new selection group.
    //! Only topmost parent  objects will be added to this selection group.
    void Clone(CSelectionGroup& newGroup);

    //! Same as Copy but will copy all objects from hierarchy of current selection to new selection group.
    void FlattenHierarchy(CSelectionGroup& newGroup, bool flattenGroups = false);

    void PickGroupAndAddToIt();

    // Send event to all objects in selection group.
    void SendEvent(ObjectEvent event);

    // Helper functions to begin and end param editing for all the objects
    // contained in a selection group.
    void BeginEditParams(IEditor* ie, int flags);
    void EndEditParams();
    ULONG STDMETHODCALLTYPE     AddRef();
    ULONG STDMETHODCALLTYPE     Release();

    void IndicateSnappingVertex(DisplayContext& dc) const;
    void FinishChanges();
private:
    QString m_name;
    typedef std::vector<TSmartPtr<CBaseObject> > Objects;
    Objects m_objects;
    // Objects set, for fast searches.
    std::set<CBaseObject*> m_objectsSet;

    // Legacy objects aren't deselected through Ebuses, so keeping a 
    // separate set for them helps improve performance of deselection
    std::set<CBaseObjectPtr> m_legacyObjectsSet;

    //! Selection list with child objecs filtered out.
    std::vector<CBaseObject*> m_filtered;

    bool m_bVertexSnapped;
    Vec3 m_snapVertex;

    const static int SnappingVertexNumThreshold = 700;

    EMoveSelectionFlag m_LastestMoveSelectionFlag;
    Quat m_LastestMovedObjectRot;

    // Description:
    //     Tries to snap a given position to the nearest vertex.
    // Arguments:
    //     pObj - An object to be vertex-snapped. This should be either a brush or a solid
    //     offset - The current offset applied to the object by user input
    // Return Value:
    //     A new offset vector needed to exactly snap to the nearest vertex. Just 'offset' in case that there is no proper snap vertex around.
    Vec3 SnapToCloseVertexIfAny(CBaseObject* pObj, const Vec3& offset);

    void CollectCandidates(std::vector<CBrushObject*>& brushes,
        std::vector<CGeomEntity*>& entities,
        CBaseObject* pObj, const Vec3& offset);
    Vec3 FindNearestSnapVertex(std::vector<Vec3>& srcVertices, const Vec3& offset,
        const std::vector<CBrushObject*>& brushes,
        const std::vector<CGeomEntity*>& entities);

protected:
    ULONG   m_ref;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_SELECTIONGROUP_H

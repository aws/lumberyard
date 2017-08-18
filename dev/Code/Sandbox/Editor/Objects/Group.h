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

// Description : Group object definition.


#ifndef CRYINCLUDE_EDITOR_OBJECTS_GROUP_H
#define CRYINCLUDE_EDITOR_OBJECTS_GROUP_H
#pragma once


#include "BaseObject.h"

class CGroupPanel;

/*!
 *  CGroup groups object together.
 *  Object can only be assigned to one group.
 *
 */
class SANDBOX_API CGroup
    : public CBaseObject
{
    Q_OBJECT
public:
    //////////////////////////////////////////////////////////////////////////
    // Overwrites from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    void Done();

    void Display(DisplayContext& disp);

    void BeginEditParams(IEditor* ie, int flags);
    void EndEditParams(IEditor* ie);

    //! Attach new child node.
    virtual void AddMember(CBaseObject* pMember, bool bKeepPos = true);
    virtual void RemoveMember(CBaseObject* pMember, bool bKeepPos = true);
    const TBaseObjects& GetMembers() const { return m_members; }
    virtual void DetachThis(bool bKeepPos = true);
    virtual void SetMaterial(CMaterial* mtl);

    void GetBoundBox(AABB& box);
    void GetLocalBounds(AABB& box);
    bool HitTest(HitContext& hc);
    virtual bool HitHelperTestForChildObjects(HitContext& hc);

    void Serialize(CObjectArchive& ar);

    //! Overwrite event handler from CBaseObject.
    void OnEvent(ObjectEvent event);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Group interface
    //////////////////////////////////////////////////////////////////////////
    //! Select objects within specified distance from given position.
    //! Return number of selected objects.
    int SelectObjects(const AABB& box, bool bUnselect = false);

    //! Remove all childs from this group.
    void    Ungroup();

    //! Open group.
    //! Make childs accessible from top user interface.
    void    Open();

    //! Close group.
    //! Make childs inaccessible from top user interface.
    //! In Closed state group only display objects but not allow to select them.
    void    Close();

    //! Return true if group is in Open state.
    bool    IsOpen() const { return m_opened; };

    //! Called by child object, when it changes.
    void    OnChildModified();

    void DeleteAllMembers();

    void BindToParent();

    void Sync()
    {
        InvalidateBBox();
    }
    virtual bool SuspendUpdate(bool bForceSuspend = true) {   return true; }
    virtual void ResumeUpdate()     {}

    virtual void PostLoad(CObjectArchive& ar)
    {
        CalcBoundBox();
    }

    void OnContextMenu(QMenu* menu) override;

    void PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx);
    void InvalidateBBox() { m_bBBoxValid = false; };

    virtual void UpdatePivot(const Vec3& newWorldPivotPos);

public:
	//! Dtor must be protected.
    CGroup();

    static const GUID& GetClassID()
    {
        // {1ED5BF40-BECA-4ba1-8E90-0906FF862A75}
        static const GUID guid = {
            0x1ed5bf40, 0xbeca, 0x4ba1, { 0x8e, 0x90, 0x9, 0x6, 0xff, 0x86, 0x2a, 0x75 }
        };
        return guid;
    }

protected:
    virtual bool HitTestMembers(HitContext& hc);
    void SerializeMembers(CObjectArchive& ar);
    virtual void CalcBoundBox();

    //! Get combined bounding box of all childs in hierarchy.
    static void RecursivelyGetBoundBox(CBaseObject* object, AABB& box, const Matrix34& parentTM);

    // Overwritten from CBaseObject.
    void RemoveChild(CBaseObject* node);
    void DeleteThis() { delete this; };

    // This list contains children which are actually members of this group, rather than regular attached ones.
    TBaseObjects m_members;

    AABB m_bbox;
    bool m_bBBoxValid;
    bool m_opened;
    bool m_bAlwaysDrawBox;
    bool m_ignoreChildModify;
    bool m_bUpdatingPivot;

    static CGroupPanel* s_pPanel;
    static int s_panelID;

    IEditor* m_ie;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_GROUP_H

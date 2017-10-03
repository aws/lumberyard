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

// Description : AreaBox object definition.


#ifndef CRYINCLUDE_EDITOR_OBJECTS_AREABOX_H
#define CRYINCLUDE_EDITOR_OBJECTS_AREABOX_H
#pragma once

class CPickEntitiesPanel;
class ReflectedPropertiesPanel;

#include "EntityObject.h"

//////////////////////////////////////////////////////////////////////////
// Base class for area objects.
//////////////////////////////////////////////////////////////////////////
class SANDBOX_API CAreaObjectBase
    : public CEntityObject
    , public IPickEntitesOwner
{
    Q_OBJECT
public:
    virtual void UpdateGameArea() {};
    //////////////////////////////////////////////////////////////////////////
    // Binded entities.
    //////////////////////////////////////////////////////////////////////////
    //! Bind entity to this shape.
    void AddEntity(CBaseObject* pEntity);
    void RemoveEntity(int index);
    CEntityObject* GetEntity(int index);
    int GetEntityCount() { return m_entities.size(); }
    bool HasMeasurementAxis() const {   return true;    }

    void BeginEditParams(IEditor* ie, int flags);
    void EndEditParams(IEditor* ie);

    virtual void Serialize(CObjectArchive& ar);

    void Reload(bool bReloadScript = false) override;

    virtual void OnAreaChange(IVariable*    pVariable) = 0;
protected:
    void DrawEntityLinks(DisplayContext& dc);
    virtual void PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx);

    //! List of binded entities.
    std::vector<GUID> m_entities;

    static int m_nEntitiesPickRollupId;
    static CPickEntitiesPanel* m_pEntitiesPickPanel;
};

/*!
 *  CAreaBox is a box volume in space where entites can be attached to.
 *
 */
class CAreaBox
    : public CAreaObjectBase
{
    Q_OBJECT
public:
    //////////////////////////////////////////////////////////////////////////
    // Ovverides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    void Done();
    bool CreateGameObject();
    virtual void InitVariables();
    void Display(DisplayContext& dc);
    void InvalidateTM(int nWhyFlags);
    void GetLocalBounds(AABB& box);
    bool HitTest(HitContext& hc);
    bool IsScalable() const { return false; };

    void BeginEditParams(IEditor* ie, int flags);
    void EndEditParams(IEditor* ie);
    void BeginEditMultiSelParams(bool bAllOfSameType);
    void EndEditMultiSelParams();

    virtual void PostLoad(CObjectArchive& ar);

    virtual void Serialize(CObjectArchive& ar);
    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);

    void SetAreaId(int nAreaId);
    int GetAreaId();
    void SetBox(AABB box);
    AABB GetBox();
    float GetWidth() { return mv_width; }
    float GetLength() { return mv_length; }
    float GetHeight() { return mv_height; }

    virtual void UpdateGameArea();

private:

    struct SBoxFace
    {
        SBoxFace(Vec3 const* const _pP1,
            Vec3 const* const _pP2,
            Vec3 const* const _pP3,
            Vec3 const* const _pP4)
            :   pP1(_pP1)
            , pP2(_pP2)
            , pP3(_pP3)
            , pP4(_pP4){}

        Vec3 const* const pP1;
        Vec3 const* const pP2;
        Vec3 const* const pP3;
        Vec3 const* const pP4;
    };

    void    UpdateSoundPanelParams();

protected:
    friend class CTemplateObjectClassDesc<CAreaBox>;

    //! Dtor must be protected.
    CAreaBox();

    static const GUID& GetClassID()
    {
        // {0EEA0A78-428C-4ad4-9EC1-97525AEB1BCB}
        static const GUID guid = {
            0xeea0a78, 0x428c, 0x4ad4, { 0x9e, 0xc1, 0x97, 0x52, 0x5a, 0xeb, 0x1b, 0xcb }
        };
        return guid;
    }

    void DeleteThis() { delete this; };

    void OnAreaChange(IVariable* pVar) override;
    void OnSizeChange(IVariable* pVar);
    void OnSoundParamsChange(IVariable* pVar);
    void OnPointChange(IVariable* var);

    //! AreaId
    CVariable<int> m_areaId;
    //! EdgeWidth
    CVariable<float> m_edgeWidth;
    //! Local volume space bounding box.
    CVariable<float> mv_width;
    CVariable<float> mv_length;
    CVariable<float> mv_height;
    CVariable<int> mv_groupId;
    CVariable<int> mv_priority;

    CVariable<bool> mv_filled;
    CVariable<bool> mv_displaySoundInfo;

    //! Local volume space bounding box.
    AABB m_box;

    static int m_rollupMultyId; // This atm just indicates if we're multi selected so we know not to display a sound-properties-panel

    uint32 m_bIgnoreGameUpdate : 1;

    typedef std::vector<bool>                                   tSoundObstruction;
    typedef tSoundObstruction::const_iterator   tSoundObstructionIterConst;

    // Sound specific members
    tSoundObstruction                   m_abObstructSound;
    static ReflectedPropertiesPanel*    m_pSoundPropertiesPanel;
    static int                              m_nSoundPanelID;
    static CVarBlockPtr             m_pSoundPanelVarBlock;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_AREABOX_H

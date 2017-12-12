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

// Description : TagPoint object definition.


#ifndef CRYINCLUDE_EDITOR_OBJECTS_ROPEOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_ROPEOBJECT_H
#pragma once


#include "ShapeObject.h"

//////////////////////////////////////////////////////////////////////////
class CRopeObject
    : public CShapeObject
{
    Q_OBJECT
public:
    CRopeObject();

    static const GUID& GetClassID()
    {
        // {DC346747-0A92-439b-9281-2EAB88593FCE}
        static const GUID guid = {
            0xdc346747, 0xa92, 0x439b, { 0x92, 0x81, 0x2e, 0xab, 0x88, 0x59, 0x3f, 0xce }
        };
        return guid;
    }

    //////////////////////////////////////////////////////////////////////////
    // Ovverides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    virtual bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    virtual void Done();
    virtual bool SetScale(const Vec3& vScale, int nWhyFlags = 0) const { return false; };
    virtual void SetSelected(bool bSelect);
    virtual bool CreateGameObject();
    virtual void InitVariables();
    virtual void UpdateGameArea(bool bRemove = false);
    virtual void InvalidateTM(int nWhyFlags);
    virtual void SetMaterial(CMaterial* mtl);
    virtual void Display(DisplayContext& dc);
    virtual void OnEvent(ObjectEvent event);

    virtual void BeginEditParams(IEditor* ie, int flags);
    virtual void EndEditParams(IEditor* ie);
    virtual void BeginEditMultiSelParams(bool bAllOfSameType);
    virtual void EndEditMultiSelParams();

    virtual void Serialize(CObjectArchive& ar);
    virtual void PostLoad(CObjectArchive& ar);
    virtual XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);
    //////////////////////////////////////////////////////////////////////////

protected:
    virtual bool IsOnlyUpdateOnUnselect() const { return false; }
    virtual int GetMinPoints() const { return 2; };
    virtual int GetMaxPoints() const { return 200; };
    virtual float GetShapeZOffset() const { return 0.0f; }
    virtual void CalcBBox();

    IRopeRenderNode* GetRenderNode() const;

    //! Called when Road variable changes.
    void OnParamChange(IVariable* var);

protected:
    friend class CRopePanelUI;

    IRopeRenderNode::SRopeParams m_ropeParams;

    struct SEndPointLinks
    {
        int m_nPhysEntityId;
        Vec3 offset;
        Vec3 object_pos;
        Quat object_q;
    };
    SEndPointLinks m_linkedObjects[2];
    int m_endLinksDisplayUpdateCounter;

private:
    void UpdateSoundData();

    struct SRopeSoundData
    {
        SRopeSoundData()
            :   sName("")
            , nSegementToAttachTo(1)
            , fOffset(0.0f){}

        QString sName;
        int nSegementToAttachTo;
        float fOffset;
    } m_ropeSoundData;

    bool m_bIsSimulating;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_ROPEOBJECT_H

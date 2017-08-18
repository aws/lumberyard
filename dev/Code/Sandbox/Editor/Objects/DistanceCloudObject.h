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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_DISTANCECLOUDOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_DISTANCECLOUDOBJECT_H
#pragma once



#include "BaseObject.h"


struct IDistanceCloudRenderNode;


class CDistanceCloudObject
    : public CBaseObject
{
    Q_OBJECT
public:
    // CBaseObject overrides
    virtual void Display(DisplayContext& dc);

    virtual bool HitTest(HitContext& hc);
    virtual void BeginEditParams(IEditor* ie, int flags);
    virtual void EndEditParams(IEditor* ie);

    virtual void GetLocalBounds(AABB& box);
    virtual void SetHidden(bool bHidden);
    virtual void UpdateVisibility(bool visible);
    virtual void SetMaterial(CMaterial* mtl);
    virtual void Serialize(CObjectArchive& ar);
    virtual XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);
    virtual IRenderNode* GetEngineNode() const { return m_pRenderNode; };
    virtual int MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);

    void UpdateEngineNode();

protected:
    // CBaseObject overrides
    virtual bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    virtual bool CreateGameObject();
    virtual void Done();
    virtual void DeleteThis() { delete this; };
    virtual void InvalidateTM(int nWhyFlags);

private:
    friend class CTemplateObjectClassDesc<CDistanceCloudObject>;
    CDistanceCloudObject();
    static const GUID& GetClassID()
    {
        // {cd77e11b-6c86-4e9f-8a4b-0cb5b0eb5314}
        static const GUID guid = {
            0xcd77e11b, 0x6c86, 0x4e9f, { 0x8a, 0x4b, 0x0c, 0xb5, 0xb0, 0xeb, 0x53, 0x14 }
        };

        return guid;
    }
    static bool IsEnabled();

    void OnProjectionTypeChange(IVariable* pVar);
    CMaterial* GetDefaultMaterial() const;

private:
    //CVariable< int > m_projectionType;
    IDistanceCloudRenderNode* m_pRenderNode;
    IDistanceCloudRenderNode* m_pPrevRenderNode;
    int m_renderFlags;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_DISTANCECLOUDOBJECT_H

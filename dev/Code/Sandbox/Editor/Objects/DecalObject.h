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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_DECALOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_DECALOBJECT_H
#pragma once



#include "BaseObject.h"


class CEdMesh;
struct IDecalRenderNode;


class CDecalObject
    : public CBaseObject
{
    Q_OBJECT
public:
    // CBaseObject overrides
    virtual void Display(DisplayContext& dc);

    virtual bool HitTest(HitContext& hc);
    virtual void BeginEditParams(IEditor* ie, int flags);
    virtual void EndEditParams(IEditor* ie);

    virtual int MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    virtual void GetLocalBounds(AABB& box);
    virtual void SetHidden(bool bHidden);
    virtual void UpdateVisibility(bool visible);
    virtual void SetMaterial(CMaterial* mtl);
    virtual void Serialize(CObjectArchive& ar);
    virtual XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);
    virtual void SetMinSpec(uint32 nSpec, bool bSetChildren = true);
    virtual void SetMaterialLayersMask(uint32 nLayersMask);
    virtual IRenderNode* GetEngineNode() const { return m_pRenderNode; };

    // special input handler (to be reused by this class as well as the decal object tool)
    bool MouseCallbackImpl(CViewport* view, EMouseEvent event, QPoint& point, int flags, bool callerIsMouseCreateCallback = false);

    int GetProjectionType() const;
    void UpdateEngineNode();

protected:
    // CBaseObject overrides
    virtual bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    virtual bool CreateGameObject();
    virtual void Done();
    virtual void DeleteThis() { delete this; };
    virtual void InvalidateTM(int nWhyFlags);

private:
    friend class CTemplateObjectClassDesc<CDecalObject>;
    CDecalObject();
    static const GUID& GetClassID()
    {
        // {90eac52c-1b65-4db2-88cd-a3be6508b383}
        static const GUID guid = {
            0x90eac52c, 0x1b65, 0x4db2, { 0x88, 0xcd, 0xa3, 0xbe, 0x65, 0x08, 0xb3, 0x83 }
        };
        return guid;
    }

    bool RayToLineDistance(const Vec3& rayLineP1, const Vec3& rayLineP2, const Vec3& pi, const Vec3& pj, float& distance, Vec3& intPnt);
    void OnFileChange(QString filename);
    void OnParamChanged(IVariable* pVar);
    void OnViewDistRatioChanged(IVariable* pVar);
    CMaterial* GetDefaultMaterial() const;

private:
    int m_renderFlags;
    CVariable<float> m_viewDistanceMultiplier;
    CVariableEnum<int> m_projectionType;
    CVariable<bool> m_deferredDecal;
    CVariable<int> m_sortPrio;
    CVariable<float> m_depth;
    IDecalRenderNode* m_pRenderNode;
    IDecalRenderNode* m_pPrevRenderNode;
    bool m_boWasModified;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_DECALOBJECT_H

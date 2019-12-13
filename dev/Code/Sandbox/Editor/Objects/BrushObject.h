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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_BRUSHOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_BRUSHOBJECT_H
#pragma once


#include "BaseObject.h"
#include "StatObjValidator.h"
#include "CollisionFilteringProperties.h"
#include "Geometry/EdMesh.h"
#include "StatObjBus.h"

/*!
 *  CTagPoint is an object that represent named 3d position in world.
 *
 */
class SANDBOX_API CBrushObject
    : public CBaseObject
    , private StatObjEventBus::Handler
{
    Q_OBJECT
public:
    //////////////////////////////////////////////////////////////////////////
    // Ovverides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    void Done();
    void Display(DisplayContext& dc);
    bool CreateGameObject();

    void GetLocalBounds(AABB& box);
    virtual QString GetTooltip() const;

    bool HitTest(HitContext& hc);
    int HitTestAxis(HitContext& hc);

    virtual void SetSelected(bool bSelect);
    virtual void SetMinSpec(uint32 nSpec, bool bSetChildren = true);
    virtual IPhysicalEntity* GetCollisionEntity() const;

    //////////////////////////////////////////////////////////////////////////
    void BeginEditParams(IEditor* ie, int flags);
    void EndEditParams(IEditor* ie);
    void BeginEditMultiSelParams(bool bAllOfSameType);
    void EndEditMultiSelParams();

    //! Called when object is being created.
    int MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);

    void Serialize(CObjectArchive& ar);
    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);

    virtual void Validate(IErrorReport* report) override;
    virtual void GatherUsedResources(CUsedResources& resources);
    virtual bool IsSimilarObject(CBaseObject* pObject);
    virtual bool StartSubObjSelection(int elemType);
    virtual void EndSubObjectSelection();
    virtual void CalculateSubObjectSelectionReferenceFrame(ISubObjectSelectionReferenceFrameCalculator* pCalculator);
    virtual void ModifySubObjSelection(SSubObjSelectionModifyContext& modCtx);
    virtual void AcceptSubObjectModify();

    virtual CEdGeometry* GetGeometry();
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    virtual IStatObj* GetIStatObj();
    int GetRenderFlags() const { return m_renderFlags; };
    IRenderNode* GetEngineNode() const { return m_pRenderNode; };
    float GetRatioLod() const { return mv_ratioLOD; };
    float GetViewDistanceMultiplier() const { return mv_viewDistanceMultiplier; };
    QString GetGeometryFile() { return mv_geometryFile; }
    void SetGeometryFile(const QString& geometryFile) { mv_geometryFile = geometryFile; }
    float GetIntegrationQuality() const { return mv_integrQuality; };

    //////////////////////////////////////////////////////////////////////////
    // Material
    //////////////////////////////////////////////////////////////////////////
    virtual void SetMaterial(CMaterial* mtl);
    virtual CMaterial* GetRenderMaterial() const;
    virtual void SetMaterialLayersMask(uint32 nLayersMask);
    virtual void OnMaterialChanged(MaterialChangeFlags change);

    //////////////////////////////////////////////////////////////////////////
    //  bool IsRecieveLightmap() const { return mv_recvLightmap; }
    bool ApplyIndirLighting() const { return !mv_noIndirLight; }

    void SaveToCGF(const QString& filename);
    void ReloadGeometry();

    bool IncludeForGI();

    float GetLightmapQuality() const { return mv_lightmapQuality;   }
    void SetLightmapQuality(const f32 fLightmapQuality) { mv_lightmapQuality = fLightmapQuality;  }
    void CreateBrushFromMesh(const char* meshFilname);

    void GetVerticesInWorld(std::vector<Vec3>& vertices) const;

protected:

    //////////////////////////////////////////////////////////////////////////
    // StatObjEvents
    void OnStatObjReloaded() override;
    //////////////////////////////////////////////////////////////////////////

    friend class CTemplateObjectClassDesc<CBrushObject>;

    //! Dtor must be protected.
    CBrushObject();

    static const GUID& GetClassID()
    {
        // {032B8809-71DB-44d7-AAA1-69F75C999463}
        static const GUID guid = {
            0x32b8809, 0x71db, 0x44d7, { 0xaa, 0xa1, 0x69, 0xf7, 0x5c, 0x99, 0x94, 0x63 }
        };
        return guid;
    }

    void FreeGameData();

    virtual void UpdateVisibility(bool visible);

    //! Convert ray given in world coordinates to the ray in local brush coordinates.
    void WorldToLocalRay(Vec3& raySrc, Vec3& rayDir);

    bool ConvertFromObject(CBaseObject* object);
    void DeleteThis() { delete this; };
    void InvalidateTM(int nWhyFlags);
    void OnEvent(ObjectEvent event);

    void UpdateEngineNode(bool bOnlyTransform = false);

    void OnFileChange(QString filename);

    //////////////////////////////////////////////////////////////////////////
    // Local callbacks.
    void OnGeometryChange(IVariable* var);
    void OnRenderVarChange(IVariable* var);
    void OnAIRadiusVarChange(IVariable* var);
    //////////////////////////////////////////////////////////////////////////

    virtual QString GetMouseOverStatisticsText() const;

    AABB m_bbox;
    Matrix34 m_invertTM;

    //! Engine node.
    //! Node that registered in engine and used to render brush prefab
    IRenderNode* m_pRenderNode;
    _smart_ptr<CEdMesh> m_pGeometry;
    bool m_bNotSharedGeom;

    //////////////////////////////////////////////////////////////////////////
    // Brush parameters.
    //////////////////////////////////////////////////////////////////////////
    CVariable<QString> mv_geometryFile;

    //////////////////////////////////////////////////////////////////////////
    // Brush rendering parameters.
    //////////////////////////////////////////////////////////////////////////

    CCollisionFilteringProperties m_collisionFiltering;
    CVariable<bool> mv_outdoor;
    //  CVariable<bool> mv_castShadows;
    //CVariable<bool> mv_selfShadowing;
    CVariable<bool> mv_castShadowMaps;
    CVariable<bool> mv_dynamicDistanceShadows;
    CVariable<bool> mv_rainOccluder;
    //  CVariable<bool> mv_recvLightmap;
    CVariable<bool> mv_registerByBBox;
    CVariableEnum<int> mv_hideable;
    //  CVariable<bool> mv_hideable;
    //  CVariable<bool> mv_hideableSecondary;
    CVariable<int> mv_ratioLOD;
    CVariable<float> mv_viewDistanceMultiplier;
    CVariable<bool> mv_excludeFromTriangulation;
    CVariable<bool> mv_noDynWater;
    CVariable<float> mv_aiRadius;
    CVariable<float> mv_lightmapQuality;
    CVariable<bool> mv_noDecals;
    CVariable<bool> mv_noIndirLight;
    CVariable<bool> mv_recvWind;
    CVariable<float> mv_integrQuality;
    CVariable<bool> mv_Occluder;
    CVariable<bool> mv_drawLast;
    CVariable<int> mv_shadowLodBias;

    //////////////////////////////////////////////////////////////////////////
    // Rendering flags.
    int m_renderFlags;

    bool m_bIgnoreNodeUpdate;
    CStatObjValidator m_statObjValidator;

private:
    // Brushes need to explicitly notify the NavMesh of their old AABB *before* it gets changed by transformation, because
    // scaling and rotation will re-physicalize the brush, and then the old AABB doesn't exist anymore and the physics system
    // will report the "wrong" old AABB to the NavMesh. In fact, what gets reported then is the new AABB, but in local space, and
    // that would cause the NavMesh to regenerate only a part of the affected area.
    virtual bool ShouldNotifyOfUpcomingAABBChanges() const override { return true; }
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_BRUSHOBJECT_H

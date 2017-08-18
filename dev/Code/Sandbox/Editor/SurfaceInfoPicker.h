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

#ifndef CRYINCLUDE_EDITOR_SURFACEINFOPICKER_H
#define CRYINCLUDE_EDITOR_SURFACEINFOPICKER_H

#pragma once

class CKDTree;

//////////////////////////////////////////////////////////////////////////
class SANDBOX_API CSurfaceInfoPicker
{
public:

    CSurfaceInfoPicker();

    class CExcludedObjects
    {
    public:

        CExcludedObjects(){}
        ~CExcludedObjects(){}
        CExcludedObjects(const CExcludedObjects& excluded)
        {
            objects = excluded.objects;
        }

        void Add(CBaseObject* pObject)
        {
            objects.insert(pObject);
        }

        void Clear()
        {
            objects.clear();
        }

        bool Contains(CBaseObject* pObject) const
        {
            return objects.find(pObject) != objects.end();
        }

    private:
        std::set<CBaseObject*> objects;
    };

    enum EPickedObjectGroup
    {
        ePOG_BrushObject = BIT(0),
        ePOG_Prefabs = BIT(1),
        ePOG_Solid = BIT(2),
        ePOG_DesignerObject = BIT(3),
        ePOG_Vegetation = BIT(4),
        ePOG_Entity = BIT(5),
        ePOG_Terrain = BIT(6),
        ePOG_GeneralObjects = ePOG_BrushObject | ePOG_Prefabs | ePOG_Solid | ePOG_DesignerObject | ePOG_Entity,
        ePOG_All = 0xFFFFFFFF,
    };

    enum EPickOption
    {
        ePickOption_IncludeFrozenObject = BIT(0),
    };

    void SetPickOptionFlag(int nFlag) { m_PickOption = nFlag; }

public:

    bool PickObject(const QPoint& point,
        SRayHitInfo& outHitInfo,
        CBaseObject* pObject);

    bool PickObject(const Vec3& vWorldRaySrc, const Vec3& vWorldRayDir,
        SRayHitInfo& outHitInfo,
        CBaseObject* pObject);

    bool Pick(const Vec3& vWorldRaySrc, const Vec3& vWorldRayDir,
        _smart_ptr<IMaterial>& ppOutLastMaterial,
        SRayHitInfo& outHitInfo,
        CExcludedObjects* pExcludedObjects = NULL,
        int nFlag = ePOG_All)
    {
        return PickImpl(vWorldRaySrc, vWorldRayDir, &ppOutLastMaterial, outHitInfo, pExcludedObjects, nFlag);
    }

    bool Pick(const Vec3& vWorldRaySrc, const Vec3& vWorldRayDir,
        SRayHitInfo& outHitInfo,
        CExcludedObjects* pExcludedObjects = NULL,
        int nFlag = ePOG_All)
    {
        return PickImpl(vWorldRaySrc, vWorldRayDir, NULL, outHitInfo, pExcludedObjects, nFlag);
    }

    bool Pick(const QPoint& point,
        SRayHitInfo& outHitInfo,
        CExcludedObjects* pExcludedObjects = NULL,
        int nFlag = ePOG_All)
    {
        return PickImpl(point, NULL, outHitInfo, pExcludedObjects, nFlag);
    }

    bool Pick(const QPoint& point,
        _smart_ptr<IMaterial>& ppOutLastMaterial,
        SRayHitInfo& outHitInfo,
        CExcludedObjects* pExcludedObjects = NULL,
        int nFlag = ePOG_All)
    {
        return PickImpl(point, &ppOutLastMaterial, outHitInfo, pExcludedObjects, nFlag);
    }

    bool PickByAABB(const QPoint& point, int nFlag = ePOG_All, IDisplayViewport* pView = NULL, CExcludedObjects* pExcludedObjects = NULL, std::vector<CBaseObjectPtr>* pOutObjects = NULL);

    void SetObjects(CBaseObjectsArray* pSetObjects) {   m_pSetObjects = pSetObjects; }

    CBaseObjectPtr GetPickedObject()
    {
        return m_pPickedObject;
    }

    void SetActiveView(IDisplayViewport* view);

public:

    static bool RayWorldToLocal(
        const Matrix34A& WorldTM,
        const Vec3& vWorldRaySrc,
        const Vec3& vWorldRayDir,
        Vec3& outRaySrc,
        Vec3& outRayDir);

private:

    bool IsFrozen(CBaseObject* pBaseObject) const
    {
        return !(m_PickOption & ePickOption_IncludeFrozenObject) && pBaseObject->IsFrozen();
    }

    bool PickImpl(const QPoint& point,
        _smart_ptr<IMaterial>* ppOutLastMaterial,
        SRayHitInfo& outHitInfo,
        CExcludedObjects* pExcludedObjects = NULL,
        int nFlag = ePOG_All);

    bool PickImpl(const Vec3& vWorldRaySrc, const Vec3& vWorldRayDir,
        _smart_ptr<IMaterial>* ppOutLastMaterial,
        SRayHitInfo& outHitInfo,
        CExcludedObjects* pExcludedObjects = NULL,
        int nFlag = ePOG_All);

    void FindNearestInfoFromBrushObjects(
        const Vec3& vWorldRaySrc,
        const Vec3& vWorldRayDir,
        _smart_ptr<IMaterial>* ppOutLastMaterial,
        SRayHitInfo& outHitInfo) const;

    void FindNearestInfoFromPrefabs(
        const Vec3& vWorldRaySrc,
        const Vec3& vWorldRayDir,
        _smart_ptr<IMaterial>* ppOutLastMaterial,
        SRayHitInfo& outHitInfo) const;

    void FindNearestInfoFromSolids(int nFlag,
        const Vec3& vWorldRaySrc,
        const Vec3& vWorldRayDir,
        _smart_ptr<IMaterial>* ppOutLastMaterial,
        SRayHitInfo& outHitInfo) const;

    void FindNearestInfoFromVegetations(
        const Vec3& vWorldRaySrc,
        const Vec3& vWorldRayDir,
        _smart_ptr<IMaterial>* ppOutLastMaterial,
        SRayHitInfo& outHitInfo) const;

    void FindNearestInfoFromDecals(
        const Vec3& vWorldRaySrc,
        const Vec3& vWorldRayDir,
        _smart_ptr<IMaterial>* ppOutLastMaterial,
        SRayHitInfo& outHitInfo) const;

    void FindNearestInfoFromEntities(
        const Vec3& vWorldRaySrc,
        const Vec3& vWorldRayDir,
        _smart_ptr<IMaterial>* ppOutLastMaterial,
        SRayHitInfo& outHitInfo) const;

    void FindNearestInfoFromTerrain(
        const Vec3& vWorldRaySrc,
        const Vec3& vWorldRayDir,
        _smart_ptr<IMaterial>* ppOutLastMaterial,
        SRayHitInfo& outHitInfo) const;

    /// Detect ray intersection with a IRenderNode, IEntity, IStatObj, or ICharacterInstance.
    /// But only if the intersection is closer than the one already in outHitInfo.
    static bool RayIntersection(
        const Vec3& vWorldRaySrc,
        const Vec3& vWorldRayDir,
        IRenderNode* pRenderNode,
        IEntity* pEntity,
        IStatObj* pStatObj,
        ICharacterInstance* pCharacterInstance,
        const Matrix34A& WorldTM,
        SRayHitInfo& outHitInfo,
        _smart_ptr<IMaterial>* ppOutLastMaterial);

    /// Detect ray intersection with a IStatObj
    static bool RayIntersection_IStatObj(
        const Vec3& vWorldRaySrc,
        const Vec3& vWorldRayDir,
        IStatObj* pStatObj,
        _smart_ptr<IMaterial>* ppOutLastMaterial,
        const Matrix34A& WorldTM,
        SRayHitInfo& outHitInfo);

    /// Detect ray intersection with a IGeomCacheRenderNode
    static bool RayIntersection_IGeomCacheRenderNode(
        const Vec3& vWorldRaySrc,
        const Vec3& vWorldRayDir,
        IGeomCacheRenderNode* pGeomCacheRenderNode,
        _smart_ptr<IMaterial>* ppOutLastMaterial,
        const Matrix34A& worldTM,
        SRayHitInfo& outHitInfo);

    /// Detect ray intersection with a IRenderNode
    static bool RayIntersection_IRenderNode(
        const Vec3& vWorldRaySrc,
        const Vec3& vWorldRayDir,
        IRenderNode* pRenderNode,
        _smart_ptr<IMaterial>* ppOutLastMaterial,
        const Matrix34A& WorldTM,
        SRayHitInfo& outHitInfo);

    /// Detect ray intersection with a IEntity
    static bool RayIntersection_IEntity(
        const Vec3& vWorldRaySrc,
        const Vec3& vWorldRayDir,
        IEntity* pEntity,
        _smart_ptr<IMaterial>* ppOutLastMaterial,
        const Matrix34A& WorldTM,
        SRayHitInfo& outHitInfo);

    /// Detect ray intersection with a ICharacterInstance
    static bool RayIntersection_ICharacterInstance(
        const Vec3& vWorldRaySrc,
        const Vec3& vWorldRayDir,
        ICharacterInstance* pCharacterInstance,
        _smart_ptr<IMaterial>* ppOutLastMaterial,
        const Matrix34A& WorldTM,
        SRayHitInfo& outHitInfo);

    /// Detect ray intersection with a CBaseObject
    static bool RayIntersection_CBaseObject(
        const Vec3& vWorldRaySrc,
        const Vec3& vWorldRayDir,
        CBaseObject* pBaseObject,
        _smart_ptr<IMaterial>* ppOutLastMaterial,
        SRayHitInfo& outHitInfo);

    static void AssignObjectMaterial(CBaseObject* pObject, const SRayHitInfo& outHitInfo, _smart_ptr<IMaterial>* pOutMaterial);
    static void AssignMaterial(_smart_ptr<IMaterial> pObject, const SRayHitInfo& outHitInfo, _smart_ptr<IMaterial>* pOutMaterial);
    static bool IsMaterialValid(CMaterial* pMaterial);

private:

    int m_PickOption;

    CBaseObjectsArray* m_pObjects;
    CBaseObjectsArray* m_pSetObjects;
    CBaseObjectsArray m_objects;
    IDisplayViewport* m_pActiveView;
    CExcludedObjects m_ExcludedObjects;
    mutable CBaseObjectPtr m_pPickedObject;
};

#endif // CRYINCLUDE_EDITOR_SURFACEINFOPICKER_H

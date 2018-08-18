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

#ifndef CRYINCLUDE_CRYENTITYSYSTEM_ENTITYOBJECT_H
#define CRYINCLUDE_CRYENTITYSYSTEM_ENTITYOBJECT_H
#pragma once

// forward declaration.
struct IStatObj;
struct ICharacterInstance;
struct IParticleEmitter;
struct SRendParams;
struct IMaterial;
struct SEntityUpdateContext;
struct IRenderNode;
struct IGeomCacheRenderNode;

// Transformation of entity object slot.
struct SEntityObjectXForm
{
    // Local space transformation relative to host entity or parent slot.
    Matrix34 localTM;

    SEntityObjectXForm()
    {
        localTM.SetIdentity();
    }
};

//////////////////////////////////////////////////////////////////////////
// Description:
//    CEntityObject defines a particular geometry object or character inside the entity.
//    Entity objects are organized in numbered slots, entity can have any number of slots,
//    where each slot is an instance of CEntityObject.
//    MAKE SURE ALL MEMEBERS ARE PROPERLY ALIGNED!
//////////////////////////////////////////////////////////////////////////
class CEntityObject
{
public:
    // Object slot transformation.
    SEntityObjectXForm* m_pXForm;

    Matrix34 m_worldTM;
    Matrix34 m_worldPrevTM;

    Vec3* m_pCameraSpacePos;

    // Parent slot in entity object.
    CEntityObject*      pParent;

    //////////////////////////////////////////////////////////////////////////
    // Entity object can be one of these.
    //////////////////////////////////////////////////////////////////////////
    IStatObj*           pStatObj;
    ICharacterInstance*  pCharacter;
    ILightSource*       pLight;
    IRenderNode*        pChildRenderNode;
    IFoliage*                       pFoliage;

    //! Override material for this specific slot.
    _smart_ptr<IMaterial> pMaterial;

    uint32 dwFObjFlags;
    // Flags are a combination of EEntityObjectFlags
    uint16 flags;
    uint8  type;
    // Internal usage flags.
    uint8  bWorldTMValid : 1;
    uint8  bObjectMoved : 1;
    uint8  bUpdate : 1;       // This slot needs to be updated, when entity is updated.
    uint8  bSerialize : 1;

    // sub object hide mask, store previous one to render correct object while the new rendermesh is created in a thread
    uint64 nSubObjHideMask;

    // pointer to LOD transition state (allocated in 3dngine only for visible objects)
    struct CRNTmpData* m_pRNTmpData;

public:
    ~CEntityObject();

    void SetLayerId(uint16 nLayerId) { }

    void GetMemoryUsage(ICrySizer* pSizer) const;

    IParticleEmitter* GetParticleEmitter() const;

    IGeomCacheRenderNode* GetGeomCacheRenderNode() const
    {
#if defined(USE_GEOM_CACHES)
        if (pChildRenderNode && pChildRenderNode->GetRenderNodeType() == eERType_GeomCache)
        {
            return static_cast<IGeomCacheRenderNode*>(pChildRenderNode);
        }
        else
#endif
        return NULL;
    }

    void SetParent(CEntityObject* pParentSlot);
    ILINE bool HaveLocalMatrix() const { return m_pXForm != NULL; }

    // Set local transformation of the slot object.
    void SetLocalTM(const Matrix34& localTM);

    // Set camera space position of the object slot
    void SetCameraSpacePos(const Vec3& cameraSpacePos);

    // Get camera space position of the object slot
    void GetCameraSpacePos(Vec3& cameraSpacePos);

    // Free memory used by camera space pos
    void FreeCameraSpacePos();

    // Add local object bounds to the one specified in parameter.
    bool GetLocalBounds(AABB& bounds);

    // Get local transformation matrix of entity object.
    const Matrix34& GetLocalTM() const;
    // Get world transformation matrix of entity object.
    const Matrix34& GetWorldTM(IEntity* pEntity);

    void UpdateWorldTM(IEntity* pEntity);

    // Release all content objects in this slot.
    void ReleaseObjects();

    // Compute lod for slot from wanted
    CLodValue ComputeLod(int wantedLod, const SRenderingPassInfo& passInfo);
    // Render slot.
    void Render(IEntity * pEntity, SRendParams & rParams, int nRndFlags, class CComponentRender * pRenderComponent, const SRenderingPassInfo &passInfo);
    // Update slot (when entity is updated)
    void Update(IEntity* pEntity, bool bVisible, bool& bBoundsChanged);

    // XForm slot.
    void OnXForm(IEntity* pEntity);

    void OnEntityEvent(IEntity* pEntity, SEntityEvent const& event);


    void OnNotSeenTimeout();

    //////////////////////////////////////////////////////////////////////////
    ILINE void SetFlags(uint32 nFlags)
    {
        flags = nFlags;
        if (nFlags & ENTITY_SLOT_RENDER_NEAREST)
        {
            dwFObjFlags |= FOB_NEAREST;
        }
        else
        {
            dwFObjFlags &= ~FOB_NEAREST;
        }

        /*if (nFlags & ENTITY_SLOT_RENDER_WITH_CUSTOM_CAMERA)
            dwFObjFlags |= FOB_CUSTOM_CAMERA;
        else
            dwFObjFlags &= ~FOB_CUSTOM_CAMERA;*/
    }

    //////////////////////////////////////////////////////////////////////////
    // Custom new/delete.
    //////////////////////////////////////////////////////////////////////////
    void* operator new(size_t nSize);
    void operator delete(void* ptr);
};

//////////////////////////////////////////////////////////////////////////
extern stl::PoolAllocatorNoMT<sizeof(CEntityObject), 8>* g_Alloc_EntitySlot;

//////////////////////////////////////////////////////////////////////////
// Custom new/delete.
//////////////////////////////////////////////////////////////////////////
inline void* CEntityObject::operator new(size_t nSize)
{
    void* ptr = g_Alloc_EntitySlot->Allocate();
    if (ptr)
    {
        memset(ptr, 0, nSize); // Clear objects memory.
    }
    return ptr;
}

//////////////////////////////////////////////////////////////////////////
inline void CEntityObject::operator delete(void* ptr)
{
    if (ptr)
    {
        g_Alloc_EntitySlot->Deallocate(ptr);
    }
}

#endif // CRYINCLUDE_CRYENTITYSYSTEM_ENTITYOBJECT_H

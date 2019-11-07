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
#ifndef CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTRENDER_H
#define CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTRENDER_H
#pragma once

#include <IComponent.h>
#include <ComponentType.h>

class CEntityObject;
struct IShaderPublicParams;
struct SCloudMovementProperties;
struct SVolumeObjectMovementProperties;
struct IParticleEffect;
class CDLight;
struct SFogVolumeProperties;
struct SpawnParams;
struct SGeometryDebugDrawInfo;

//! Interface for the entity's rendering component.
//! This is a renderable object that is registered in 3D engine sector.
//! <p>
//! The render component can contain multiple sub object slots.
//! Each slot can have its own relative transformation.
//! Each slot can represent a specific renderable interface (IStatObj,
//! ICharacterInstance, etc). Slots can also form an hierarchical
//! transformation tree by referencing a parent slot for transformation.
struct IComponentRender
    : public IComponent
{
    // <interfuscator:shuffle>
    DECLARE_COMPONENT_TYPE("ComponentRender", 0xC7FB27EF5DF0499F, 0xAEDC605A89A2B3C6)

    IComponentRender() {}

    ComponentEventPriority GetEventPriority(const int eventID) const override { return EntityEventPriority::Render; }

    // Description:
    //    Get world bounds of render component.
    // Arguments:
    //    bounds - Returns Bounding box in world space.
    virtual void GetWorldBounds(AABB& bounds) = 0;

    // Description:
    //    Get local space int the entity bounds of render component.
    // Arguments:
    //    bounds - Returns Bounding box in local entity space.
    virtual void GetLocalBounds(AABB& bounds) = 0;

    // Description:
    //    Force local bounds.
    // Arguments:
    //    bounds - Bounding box in local space.
    //    bDoNotRecalculate - when set to true entity will never try to recalculate local bounding box set by this call.
    virtual void SetLocalBounds(const AABB& bounds, bool bDoNotRecalculate) = 0;

    // Invalidates local or world space bounding box.
    virtual void InvalidateLocalBounds() = 0;

    // Description:
    //     Retrieve an actual material used for rendering specified slot.
    ///    Will return custom applied material or if custom material not set will return an actual material assigned to the slot geometry.
    // Arguments:
    //     nSlot - Slot to query used material from, if -1 material will be taken from the first renderable slot.
    // Return:
    //     Material used for rendering, or NULL if slot is not rendered.
    virtual _smart_ptr<IMaterial> GetRenderMaterial(int nSlot = -1) = 0;

    // Description:
    //     Assign custom material to the slot.
    // Arguments:
    //     nSlot - Slot to apply material to.
    virtual void SetSlotMaterial(int nSlot, _smart_ptr<IMaterial> pMaterial) = 0;

    // Description:
    //     Retrieve slot's custom material (This material Must have been applied before with the SetSlotMaterial).
    // Arguments:
    //     nSlot - Slot to query custom material from.
    // Return:
    //     Custom material applied on the slot.
    virtual _smart_ptr<IMaterial> GetSlotMaterial(int nSlot) = 0;

    // Description:
    //    Retrieve engine render node, used to render this entity.
    virtual IRenderNode* GetRenderNode() = 0;

    // Description:
    //    Retrieve and optionally create a shaders public params for this render component.
    // Arguments:
    //    bCreate - If Shader public params are not created for this entity, they will be created.
    virtual IShaderPublicParams* GetShaderPublicParams(bool bCreate = true) = 0;

    // Description:
    //    Add a render component callback
    virtual void AddShaderParamCallback(IShaderParamCallbackPtr pCallback) = 0;

    // Description:
    //    Remove a render component callback
    virtual bool RemoveShaderParamCallback(IShaderParamCallbackPtr pCallback) = 0;

    // Description
    //      Check which shader param callback should be activated or disactived depending on
    //      the current entity state
    virtual void CheckShaderParamCallbacks() = 0;

    // Description
    //      Clears all the shader param callback from this entity
    virtual void ClearShaderParamCallbacks() = 0;

    // Description:
    //     Assign sub-object hide mask to slot.
    // Arguments:
    //     nSlot - Slot to apply hide mask to.
    virtual void SetSubObjHideMask(int nSlot, uint64 nSubObjHideMask) = 0;
    virtual uint64 GetSubObjHideMask(int nSlot) const = 0;

    // Description:
    //    updates indirect lighting for children
    virtual void UpdateIndirLightForChildren(){};

    // Description:
    //    sets material layers masks
    virtual void SetMaterialLayersMask(uint8 nMtlLayersMask) = 0;
    virtual uint8 GetMaterialLayersMask() const = 0;

    // Description:
    //    overrides material layers blend amount
    virtual void SetMaterialLayersBlend(uint32 nMtlLayersBlend) = 0;
    virtual uint32 GetMaterialLayersBlend() const = 0;

    // Description:
    //    sets custom post effect
    virtual void SetCustomPostEffect(const char* pPostEffectName) = 0;

    // Description:
    //    sets hud render component to ignore hud interference filter
    virtual void SetIgnoreHudInterferenceFilter(const bool bIgnoreFiler) = 0;

    // Description:
    //    set/get hud silhouetes params
    virtual void SetHUDSilhouettesParams(float r, float g, float b, float a) = 0;
    virtual uint32 GetHUDSilhouettesParams() const = 0;

    // Description:
    //    set/get shadow dissolving (fade out for phantom perk etc)
    virtual void SetShadowDissolve(bool enable) = 0;
    virtual bool GetShadowDissolve() const = 0;

    // Description:
    //    set/get opacity
    virtual void SetOpacity(float fAmount) = 0;
    virtual float GetOpacity() const = 0;

    // Description:
    //  return the last time (as set by the system timer) when the rendercomponent was last seen.
    virtual float   GetLastSeenTime() const = 0;

    // Description:
    //  return true if entity visarea was visible during last frames
    virtual bool IsRenderComponentVisAreaVisible() const = 0;

    // Description:
    //  Removes all slots from the render component
    virtual void ClearSlots() = 0;

    // Description:
    //    Set the view distance multiplier on the render node. Valid input range
    //    [0.0f - 100.0f], other values will be clamped to this range.
    virtual void SetViewDistanceMultiplier(float fViewDistanceMultiplier) = 0;

    // Description:
    //    Sets the LodRatio on the render node.
    virtual void SetLodRatio(int nLodRatio) = 0;

    // Description:
    //    Returns IStatObj* if entity is set up as compound object, otherwise returns nullptr.
    // Note:
    //    Promoted function declaration to IComponentRender so it could be accessed by CBreakableManager.
    virtual IStatObj* GetCompoundObj() const = 0;

    // Description:
    //    Sets flags, completely replaces all flags which are already set in the entity.
    // Note:
    //    Promoted function to IEntity for access by CScriptBind_Entity.
    virtual void SetFlags(uint32 flags) = 0;

    // Note:
    //    Promoted function to IEntity for access by CScriptBind_Entity.
    virtual uint32 GetFlags() = 0;

    // Note:
    //    Promoted function to IComponentRender for access by CComponentPhysics
    virtual bool PhysicalizeFoliage(bool bPhysicalize = true, int iSource = 0, int nSlot = 0) = 0;

    // Note:
    //    Promoted function to IComponentRender for access by CComponentPhysics
    virtual int SetSlotGeometry(int nSlot, IStatObj* pStatObj) = 0;

    // Description:
    //    Invalidates local or world space bounding box.
    // Note:
    //    Promoted function to IComponentRender for access by CComponentPhysics
    virtual void InvalidateBounds(bool bLocal, bool bWorld) = 0;

    // Description:
    //
    virtual void SetLastSeenTime(float fNewTime) = 0;

    // Description:
    //
    virtual void UpdateEntityFlags() = 0;

    // Description:
    // Called after constructor.
    virtual void PostInit() = 0;

    // Description:
    // Retrieves the character in the specified slot.
    virtual ICharacterInstance* GetCharacter(int nSlot) = 0;

    // Description:
    // Allocate a new object slot.
    virtual CEntityObject* AllocSlot(int nIndex) = 0;

    // Description:
    // Returns true if there is a valid slot at the specified index.
    virtual bool IsSlotValid(int nIndex) const = 0;

    // Description:
    // Releases the entity object in the specified slot and removes the slot.
    virtual void FreeSlot(int nIndex) = 0;

    // Description:
    // Number of slots allocated on this entity.
    virtual int  GetSlotCount() const = 0;

    // Description:
    // Get the CEntityObject in the specified slot.
    virtual CEntityObject* GetSlot(int nIndex) const = 0;

    // Description:
    // Get the SEntitySlotInfo for the specified slot.
    virtual bool GetSlotInfo(int nIndex, SEntitySlotInfo& slotInfo) const = 0;

    // Description:
    // Get the world transform for the specified slot.
    virtual const Matrix34& GetSlotWorldTM(int nIndex) const = 0;

    // Description:
    // Returns local relative to host entity transformation matrix of the object slot.
    virtual const Matrix34& GetSlotLocalTM(int nIndex, bool bRelativeToParent) const = 0;

    // Description:
    // Set local transformation matrix of the object slot.
    virtual void SetSlotLocalTM(int nIndex, const Matrix34& localTM, int nWhyFlags = 0) = 0;

    // Description:
    // Set camera space position of the object slot
    virtual void SetSlotCameraSpacePos(int nIndex, const Vec3& cameraSpacePos) = 0;

    // Description:
    // Get camera space position of the object slot
    virtual void GetSlotCameraSpacePos(int nSlot, Vec3& cameraSpacePos) const = 0;

    // Description:
    // Establish slot parenting.
    virtual bool SetParentSlot(int nParentIndex, int nChildIndex) = 0;

    // Description:
    // Set the flags for the specified slot.
    virtual void SetSlotFlags(int nSlot, uint32 nFlags) = 0;

    // Description:
    // Retrieve the flags for the specified slot.
    virtual uint32 GetSlotFlags(int nSlot) const = 0;

    // Description:
    // Set a character instance on the specified slot.
    virtual int SetSlotCharacter(int nSlot, ICharacterInstance* pCharacter) = 0;

    // Description:
    // Get the static object data in the specified slot.
    virtual IStatObj* GetStatObj(int nSlot) = 0;

    // Description:
    // Retrieves the particle emitter in the specified slot.
    virtual IParticleEmitter* GetParticleEmitter(int nSlot) = 0;

#if defined(USE_GEOM_CACHES)
    // Description:
    // Get the geom cache render node in the specified slot if available.
    virtual IGeomCacheRenderNode* GetGeomCacheRenderNode(unsigned int nSlot, Matrix34A* pMatrix = NULL, bool bReturnOnlyVisible = false) = 0;

    // Description:
    // Get the geom cache render node in the specified slot if available.
    virtual IGeomCacheRenderNode* GetGeomCacheRenderNode(int nSlot) = 0;

    // Description:
    // Loads the specified geom cache from file into the specified slot.
    virtual int LoadGeomCache(int nSlot, const char* sFilename) = 0;
#endif

    // Description:
    // Loads static geometry from file into the specified slot.
    virtual int LoadGeometry(int nSlot, const char* sFilename, const char* sGeomName = NULL, int nLoadFlags = 0) = 0;

    // Description:
    // Loads a character from file into the specified slot.
    virtual int LoadCharacter(int nSlot, const char* sFilename, int nLoadFlags = 0) = 0;

#if !defined(RENDERNODES_LEAN_AND_MEAN)
    // Description:
    // Loads cloud data from file into the specified slot.
    virtual int LoadCloud(int nSlot, const char* sFilename) = 0;

    // Description:
    // Configure the cloud properties for the specified slot.
    virtual int SetCloudMovementProperties(int nSlot, const SCloudMovementProperties& properties) = 0;

    // Description:
    // Load volume data from file into the specified slot.
    virtual int LoadVolumeObject(int nSlot, const char* sFilename) = 0;

    // Description:
    // Configure the volume properties for the specified slot.
    virtual int SetVolumeObjectMovementProperties(int nSlot, const SVolumeObjectMovementProperties& properties) = 0;
#endif

    // Description:
    // Configure volumetric fog properties for volume in the specified slot.
    virtual int LoadFogVolume(int nSlot, const SFogVolumeProperties& properties) = 0;

    // Description:
    // Apply fade density to the volume in the specified slot.
    virtual int FadeGlobalDensity(int nSlot, float fadeTime, float newGlobalDensity) = 0;

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
    // Description:
    // Created a prism object in the specified slot.
    virtual int LoadPrismObject(int nSlot) = 0;
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

    // Description:
    // Sets the provided particle emitter into the specified slot.
    virtual int SetParticleEmitter(int nSlot, IParticleEmitter* pEmitter, bool bSerialize = false) = 0;

    // Description:
    // Spawns a particle effect from the particle emitter in the specified slot.
    virtual int LoadParticleEmitter(int nSlot, IParticleEffect* pEffect, SpawnParams const* params = NULL, bool bPrime = false, bool bSerialize = false) = 0;

    // Description:
    // Creates a light source in the specified slot.
    virtual int LoadLight(int nSlot, CDLight* pLight, uint16 layerId) = 0;

    // Description:
    // Draws debug information on-screen.
    virtual void  DebugDraw(const SGeometryDebugDrawInfo& info) = 0;
    // </interfuscator:shuffle>
};

DECLARE_SMART_POINTERS(IComponentRender);

#endif // CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTRENDER_H
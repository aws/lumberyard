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

#include "CryLegacy_precompiled.h"
#include "ScriptBind_Entity.h"
#include "Entity.h"
#include "EntitySystem.h"
#include "BreakableManager.h"

#include <ISystem.h>
#include <ILog.h>
#include <IEntitySystem.h>
#include <IMovieSystem.h>
#include <IAISystem.h>
#include <IAgent.h>
#include <ICryAnimation.h>
#include <IRenderer.h>
#include <IConsole.h>
#include <CryCharMorphParams.h>
#include <ILipSync.h>
#include <ITimer.h>
#include <I3DEngine.h>
#include <ITimeOfDay.h>
#include <IEntityRenderState.h>
#include "IBreakableManager.h"
#include "IRenderAuxGeom.h"
#include <CryPath.h>
#include <CryFile.h>
#include "EntityObject.h"
#include "AreaManager.h"
#include "Area.h"
#include <IFacialAnimation.h>

#if !defined(OTHER_ACTIVE)
#   include "CryTypeInfo.h"
#endif

#include "math.h"
#include "IAIObjectManager.h"
#include "IGoalPipe.h"
#include <MathConversion.h>

#include "Components/IComponentRender.h"
#include "Components/IComponentTrigger.h"
#include "Components/IComponentAudio.h"
#include "Components/IComponentPhysics.h"
#include "Components/IComponentRope.h"
#include "Components/IComponentCamera.h"
#include "DynamicResponseProxy.h"
#include "../Cry3DEngine/Environment/OceanEnvironmentBus.h"

// Macro for getting IEntity pointer for function.
#define GET_ENTITY IEntity * pEntity = GetEntity(pH); if (!pEntity) {return pH->EndFunction(); }

#define PHYSICPARAM_PARTICLE     0x00000001
#define PHYSICPARAM_VEHICLE      0x00000002
#define PHYSICPARAM_PLAYERDYN    0x00000003
#define PHYSICPARAM_PLAYERDIM    0x00000004
#define PHYSICPARAM_SIMULATION   0x00000005
#define PHYSICPARAM_ARTICULATED  0x00000006
#define PHYSICPARAM_JOINT        0x00000007
#define PHYSICPARAM_ROPE         0x00000008
#define PHYSICPARAM_BUOYANCY     0x00000009
#define PHYSICPARAM_CONSTRAINT   0x0000000A
#define PHYSICPARAM_REMOVE_CONSTRAINT 0x00B
#define PHYSICPARAM_FLAGS        0x0000000C
#define PHYSICPARAM_WHEEL        0x0000000D
#define PHYSICPARAM_SOFTBODY     0x0000000E
#define PHYSICPARAM_VELOCITY     0x0000000F
#define PHYSICPARAM_PART_FLAGS   0x00000010
#define PHYSICPARAM_SUPPORT_LATTICE 0x00011
#define PHYSICPARAM_GROUND_PLANE 0x00000012
#define PHYSICPARAM_FOREIGNDATA  0x00000013
#define PHYSICPARAM_AUTO_DETACHMENT 0x00014
#define PHYSICPARAM_COLLISION_CLASS 0x00015

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CScriptBind_Entity::CScriptBind_Entity(IScriptSystem* pSS, ISystem* pSystem, IEntitySystem* pEntitySystem)
    : m_pEntitySystem(pEntitySystem)
    , m_pISystem(pSystem)
    , m_bIsAudioEventListener(false)
{
    CScriptableBase::Init(pSS, pSystem, 1); // Use parameter offset 1 for self.
    SetGlobalName("Entity");

    //init temp tables
    m_pStats.Create(m_pSS);

    // For Xennon.
#undef SCRIPT_REG_CLASSNAME

#define SCRIPT_REG_CLASSNAME &CScriptBind_Entity::


    SCRIPT_REG_FUNC(DeleteThis);
    SCRIPT_REG_FUNC(CreateRenderComponent);
    SCRIPT_REG_FUNC(CheckShaderParamCallbacks);
    SCRIPT_REG_TEMPLFUNC(LoadObject, "nSlot,sFilename");
    SCRIPT_REG_TEMPLFUNC(LoadObjectWithFlags, "nSlot,sFilename, nFlags");
    SCRIPT_REG_TEMPLFUNC(LoadObjectLattice, "nSlot");
    SCRIPT_REG_TEMPLFUNC(LoadSubObject, "nSlot,sFilename,sGeomName");
    SCRIPT_REG_TEMPLFUNC(LoadCharacter, "nSlot,sFilename");
    SCRIPT_REG_TEMPLFUNC(LoadGeomCache, "nSlot,sFilename");
    SCRIPT_REG_TEMPLFUNC(LoadLight, "nSlot,tLightDescription");
    SCRIPT_REG_TEMPLFUNC(SetLightColorParams, "nSlot,vDir,nSpecularMultiplier");
    SCRIPT_REG_TEMPLFUNC(UpdateLightClipBounds, "nSlot");
    SCRIPT_REG_TEMPLFUNC(SetSelfAsLightCasterException, "nLightSlot");
    SCRIPT_REG_TEMPLFUNC(LoadCloud, "nSlot,sCloudFilename");
    SCRIPT_REG_TEMPLFUNC(SetCloudMovementProperties, "nSlot,tCloudMovementProperties");
    SCRIPT_REG_TEMPLFUNC(LoadFogVolume, "nSlot,tFogVolumeDescription");
    SCRIPT_REG_TEMPLFUNC(FadeGlobalDensity, "nSlot,fFadeTime,fNewGlobalDensity");

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
    SCRIPT_REG_TEMPLFUNC(LoadPrismObject, "nSlot");
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

    SCRIPT_REG_TEMPLFUNC(LoadVolumeObject, "nSlot,sFilename");
    SCRIPT_REG_TEMPLFUNC(SetVolumeObjectMovementProperties, "nSlot,tVolumeObjectMovementProperties");
    SCRIPT_REG_TEMPLFUNC(LoadParticleEffect, "nSlot,sEffectName,bPrime,fPulsePeriod,fScale,fCountScale,sAttachType,sAttachForm");
    SCRIPT_REG_TEMPLFUNC(PreLoadParticleEffect, "sEffectName");
    SCRIPT_REG_TEMPLFUNC(IsSlotParticleEmitter, "slot");
    SCRIPT_REG_TEMPLFUNC(IsSlotLight, "slot");
    SCRIPT_REG_TEMPLFUNC(IsSlotGeometry, "slot");
    SCRIPT_REG_TEMPLFUNC(IsSlotCharacter, "slot");
    //
    SCRIPT_REG_TEMPLFUNC(DeleteParticleEmitter, "slot");
    SCRIPT_REG_TEMPLFUNC(RegisterForAreaEvents, "enable");

    SCRIPT_REG_TEMPLFUNC(RenderAlways, "enable");

    SCRIPT_REG_TEMPLFUNC(SetParentSlot, "child, parent");
    SCRIPT_REG_TEMPLFUNC(GetParentSlot, "child");
    SCRIPT_REG_TEMPLFUNC(GetSlotCount, "");
    SCRIPT_REG_TEMPLFUNC(SetSlotPos, "nSlot,vPos");
    SCRIPT_REG_TEMPLFUNC(SetSlotPosAndDir, "nSlot,vPos,vDir");
    SCRIPT_REG_TEMPLFUNC(GetSlotPos, "nSlot");
    SCRIPT_REG_TEMPLFUNC(SetSlotAngles, "nSlot,vAngles");
    SCRIPT_REG_TEMPLFUNC(GetSlotAngles, "nSlot");
    SCRIPT_REG_TEMPLFUNC(SetSlotScale, "nSlot,fScale");
    SCRIPT_REG_TEMPLFUNC(GetSlotScale, "nSlot");
    SCRIPT_REG_TEMPLFUNC(IsSlotValid, "nSlot");
    SCRIPT_REG_TEMPLFUNC(CopySlotTM, "srcSlot,destSlot,includeTranslation");
    SCRIPT_REG_TEMPLFUNC(MultiplyWithSlotTM, "slot, pos");
    SCRIPT_REG_TEMPLFUNC(SetSlotWorldTM, "slot, pos, dir");
    SCRIPT_REG_TEMPLFUNC(GetSlotWorldPos, "slot");
    SCRIPT_REG_TEMPLFUNC(GetSlotWorldDir, "slot");

    SCRIPT_REG_TEMPLFUNC(GetCharacter, "nSlot");

    //////////////////////////////////////////////////////////////////////////
    SCRIPT_REG_TEMPLFUNC(SetPos, "vPos");
    SCRIPT_REG_TEMPLFUNC(SetAngles, "vAngles");
    SCRIPT_REG_TEMPLFUNC(SetScale, "fScale");
    SCRIPT_REG_TEMPLFUNC(CalcWorldAnglesFromRelativeDir, "vDir");
    SCRIPT_REG_FUNC(GetPos);
    SCRIPT_REG_FUNC(GetAngles);
    SCRIPT_REG_FUNC(GetScale);
    SCRIPT_REG_FUNC(GetCenterOfMassPos);
    SCRIPT_REG_FUNC(GetWorldBoundsCenter);
    SCRIPT_REG_FUNC(GetDistance);
    SCRIPT_REG_FUNC(UpdateAreas);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Local space orientation.
    //////////////////////////////////////////////////////////////////////////
    SCRIPT_REG_TEMPLFUNC(SetLocalPos, "vPos");
    SCRIPT_REG_TEMPLFUNC(SetLocalAngles, "vAngles");
    SCRIPT_REG_TEMPLFUNC(SetLocalScale, "fScale");

    SCRIPT_REG_FUNC(GetLocalPos);
    SCRIPT_REG_FUNC(GetLocalAngles);
    SCRIPT_REG_FUNC(GetLocalScale);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // World space orientation.
    //////////////////////////////////////////////////////////////////////////
    SCRIPT_REG_TEMPLFUNC(SetWorldPos, "vPos");
    SCRIPT_REG_TEMPLFUNC(SetWorldAngles, "vAngles");
    SCRIPT_REG_TEMPLFUNC(SetWorldScale, "fScale");

    SCRIPT_REG_FUNC(GetWorldPos);
    SCRIPT_REG_FUNC(GetWorldDir);
    SCRIPT_REG_FUNC(GetWorldAngles);
    SCRIPT_REG_FUNC(GetWorldScale);

    SCRIPT_REG_TEMPLFUNC(GetBoneLocal, "");
    SCRIPT_REG_TEMPLFUNC(IsEntityInside, "EntityId");
    SCRIPT_REG_TEMPLFUNC(LookAt, "vTarget");
    //////////////////////////////////////////////////////////////////////////

    SCRIPT_REG_TEMPLFUNC(DrawSlot, "nSlot,nEnable");
    SCRIPT_REG_TEMPLFUNC(IgnorePhysicsUpdatesOnSlot, "nSlot");
    SCRIPT_REG_TEMPLFUNC(FreeSlot, "nSlot");
    SCRIPT_REG_TEMPLFUNC(FreeAllSlots, "");

    //////////////////////////////////////////////////////////////////////////
    SCRIPT_REG_TEMPLFUNC(Physicalize, "nSlot,nPhysicsType,tPhysicsParams");
    SCRIPT_REG_FUNC(DestroyPhysics);
    SCRIPT_REG_FUNC(ResetPhysics);
    SCRIPT_REG_TEMPLFUNC(AwakeCharacterPhysics, "nSlot,sRootBoneName,nAwake");
    SCRIPT_REG_TEMPLFUNC(AwakePhysics, "nAwake");
    SCRIPT_REG_TEMPLFUNC(EnablePhysics, "bEnable");
    SCRIPT_REG_TEMPLFUNC(ActivatePlayerPhysics, "bEnable");
    SCRIPT_REG_FUNC(SetPhysicParams);
    SCRIPT_REG_FUNC(SetCharacterPhysicParams);
    SCRIPT_REG_TEMPLFUNC(RagDollize, "slot");

    SCRIPT_REG_TEMPLFUNC(PhysicalizeSlot, "slot, physicsParams");
    SCRIPT_REG_TEMPLFUNC(UpdateSlotPhysics, "slot");
    SCRIPT_REG_TEMPLFUNC(ReattachSoftEntityVtx, "EntityId, nPart");

    SCRIPT_REG_TEMPLFUNC(SetColliderMode, "SetColliderMode");
    //////////////////////////////////////////////////////////////////////////

    SCRIPT_REG_FUNC(SetName);
    SCRIPT_REG_FUNC(GetName);
    SCRIPT_REG_FUNC(GetRawId);

    SCRIPT_REG_FUNC(GetArchetype);

    SCRIPT_REG_FUNC(SetAIName);
    SCRIPT_REG_FUNC(GetAIName);
    SCRIPT_REG_TEMPLFUNC(SetFlags, "flags, mode");
    SCRIPT_REG_TEMPLFUNC(GetFlags, "");
    SCRIPT_REG_TEMPLFUNC(HasFlags, "flags");
    SCRIPT_REG_TEMPLFUNC(SetFlagsExtended, "flags, mode");
    SCRIPT_REG_TEMPLFUNC(GetFlagsExtended, "");
    SCRIPT_REG_TEMPLFUNC(HasFlagsExtended, "flags");

    SCRIPT_REG_TEMPLFUNC(IntersectRay, "slot, rayOrigin, rayDir, maxDistance");

    //////////////////////////////////////////////////////////////////////////
    SCRIPT_REG_TEMPLFUNC(AttachChild, "childEntityId, flags");
    SCRIPT_REG_FUNC(DetachThis);
    SCRIPT_REG_FUNC(DetachAll);
    SCRIPT_REG_FUNC(GetParent);
    SCRIPT_REG_FUNC(GetChildCount);
    SCRIPT_REG_TEMPLFUNC(GetChild, "nIndex");
    SCRIPT_REG_TEMPLFUNC(EnableInheritXForm, "bEnable");
    //////////////////////////////////////////////////////////////////////////

    SCRIPT_REG_TEMPLFUNC(SetGeomCachePlaybackTime, "time");
    SCRIPT_REG_TEMPLFUNC(SetGeomCacheParams, "looping, standIn, standInMaterial, firstFrameStandIn, firstFrameStandInMaterial, lastFrameStandIn, lastFrameStandInMaterial, standInDistance, streamInDistance");
    SCRIPT_REG_TEMPLFUNC(SetGeomCacheStreaming, "active, time");
    SCRIPT_REG_TEMPLFUNC(IsGeomCacheStreaming, "");
    SCRIPT_REG_TEMPLFUNC(SetGeomCacheDrawing, "active");
    SCRIPT_REG_TEMPLFUNC(GetGeomCachePrecachedTime, "");

    SCRIPT_REG_FUNC(NetPresent);
    SCRIPT_REG_FUNC(StartAnimation);
    SCRIPT_REG_TEMPLFUNC(StopAnimation, "characterSlot, layer");
    SCRIPT_REG_TEMPLFUNC(ResetAnimation, "characterSlot, layer");
    SCRIPT_REG_TEMPLFUNC(RedirectAnimationToLayer0, "characterSlot, redirect");
    SCRIPT_REG_TEMPLFUNC(SetAnimationBlendOut, "characterSlot, layer");

    SCRIPT_REG_TEMPLFUNC(EnableBoneAnimation, "characterSlot, layer, boneName, enable");
    SCRIPT_REG_TEMPLFUNC(EnableBoneAnimationAll, "characterSlot, layer, enable");

    SCRIPT_REG_TEMPLFUNC(EnableProceduralFacialAnimation, "enable");
    SCRIPT_REG_TEMPLFUNC(PlayFacialAnimation, "name, looping");

    SCRIPT_REG_FUNC(GetHelperPos);
    SCRIPT_REG_FUNC(GetHelperDir);
    SCRIPT_REG_FUNC(GetHelperAngles);
    SCRIPT_REG_TEMPLFUNC(GetSlotHelperPos, "slot, helperName, objectSpace");
    SCRIPT_REG_FUNC(RenderShadow);
    SCRIPT_REG_FUNC(SetRegisterInSectors);
    SCRIPT_REG_FUNC(IsColliding);
    SCRIPT_REG_FUNC(GetDirectionVector);
    SCRIPT_REG_TEMPLFUNC(SetDirectionVector, "direction, [up]");
    SCRIPT_REG_TEMPLFUNC(IsAnimationRunning, "characterSlot, layer");
    SCRIPT_REG_FUNC(AddImpulse);
    SCRIPT_REG_FUNC(AddConstraint);
    SCRIPT_REG_FUNC(TriggerEvent);

    SCRIPT_REG_TEMPLFUNC(SetLocalBBox, "min,max");
    SCRIPT_REG_FUNC(GetLocalBBox);
    SCRIPT_REG_FUNC(GetWorldBBox);
    SCRIPT_REG_FUNC(GetProjectedWorldBBox);
    SCRIPT_REG_TEMPLFUNC(SetTriggerBBox, "min,max");
    SCRIPT_REG_FUNC(GetTriggerBBox);
    SCRIPT_REG_FUNC(InvalidateTrigger);
    SCRIPT_REG_TEMPLFUNC(ForwardTriggerEventsTo, "entityId");

    SCRIPT_REG_FUNC(SetUpdateRadius);
    SCRIPT_REG_FUNC(GetUpdateRadius);
    SCRIPT_REG_TEMPLFUNC(Activate, "bActive");
    SCRIPT_REG_TEMPLFUNC(IsActive, "");
    SCRIPT_REG_TEMPLFUNC(IsFromPool, "");
    SCRIPT_REG_TEMPLFUNC(SetUpdatePolicy, "nUpdatePolicy");
    SCRIPT_REG_FUNC(SetPublicParam);
    SCRIPT_REG_TEMPLFUNC(SetAnimationEvent, "nSlot,sAnimation");
    SCRIPT_REG_TEMPLFUNC(SetAnimationTime, "nSlot,nLayer,fTime");
    SCRIPT_REG_TEMPLFUNC(GetAnimationTime, "nSlot,nLayer");
    //SCRIPT_REG_TEMPLFUNC(SetAnimationKeyEvent,"nSlot,sAnimation,nFrameID,sEvent");
    SCRIPT_REG_FUNC(SetAnimationKeyEvent);
    SCRIPT_REG_TEMPLFUNC(DisableAnimationEvent, "nSlot,sAnimation");
    SCRIPT_REG_TEMPLFUNC(SetAnimationSpeed, "characterSlot, layer, speed");
    SCRIPT_REG_FUNC(SelectPipe);
    SCRIPT_REG_FUNC(InsertSubpipe);
    SCRIPT_REG_FUNC(CancelSubpipe);
    SCRIPT_REG_FUNC(PassParamsToPipe);
    SCRIPT_REG_FUNC(IsUsingPipe);
    SCRIPT_REG_FUNC(GetCurAnimation);
    SCRIPT_REG_FUNC(SetTimer);
    SCRIPT_REG_FUNC(KillTimer);
    SCRIPT_REG_TEMPLFUNC(SetScriptUpdateRate, "nMilliseconds");
    SCRIPT_REG_TEMPLFUNC(GetAnimationLength, "characterSlot, animation");
    SCRIPT_REG_TEMPLFUNC(SetAnimationFlip, "characterSlot, flip");

    // Audio specific
    SCRIPT_REG_FUNC(GetAllAuxAudioProxiesID);
    SCRIPT_REG_FUNC(GetDefaultAuxAudioProxyID);
    SCRIPT_REG_FUNC(CreateAuxAudioProxy);
    SCRIPT_REG_TEMPLFUNC(RemoveAuxAudioProxy, "hAudioProxyLocalID");
    SCRIPT_REG_TEMPLFUNC(ExecuteAudioTrigger, "hTriggerID, hAudioProxyLocalID");
    SCRIPT_REG_TEMPLFUNC(StopAudioTrigger, "hTriggerID, hAudioProxyLocalID");
    SCRIPT_REG_TEMPLFUNC(SetAudioSwitchState, "hSwitchID, hSwitchStateID, hAudioProxyLocalID");
    SCRIPT_REG_TEMPLFUNC(SetAudioObstructionCalcType, "nObstructionCalcType, hAudioProxyLocalID");
    SCRIPT_REG_TEMPLFUNC(SetFadeDistance, "fFadeDistance");
    SCRIPT_REG_TEMPLFUNC(SetAudioProxyOffset, "vOffset, hAudioProxyLocalID");
    SCRIPT_REG_TEMPLFUNC(SetEnvironmentFadeDistance, "fEnvironmentFadeDistance");
    SCRIPT_REG_TEMPLFUNC(SetAudioEnvironmentID, "hEnvironmentID");
    SCRIPT_REG_FUNC(SetCurrentAudioEnvironments);
    SCRIPT_REG_TEMPLFUNC(SetAudioRtpcValue, "hRtpcID, fValue, hAudioProxyLocalID");
    SCRIPT_REG_TEMPLFUNC(ResetAudioRtpcValues, "hAudioProxyLocalID");
    SCRIPT_REG_TEMPLFUNC(AuxAudioProxiesMoveWithEntity, "bCanMoveWithEntity");
    // ~Audio specific

    SCRIPT_REG_FUNC(GetBonePos);
    SCRIPT_REG_FUNC(GetBoneDir);
    SCRIPT_REG_TEMPLFUNC(GetBoneVelocity, "characterSlot, boneName");
    SCRIPT_REG_TEMPLFUNC(GetBoneAngularVelocity, "characterSlot, boneName");
    SCRIPT_REG_FUNC(GetBoneNameFromTable);
    SCRIPT_REG_FUNC(GetTouchedSurfaceID);
    SCRIPT_REG_FUNC(GetTouchedPoint);
    SCRIPT_REG_TEMPLFUNC(CreateBoneAttachment, "characterSlot, boneName, attachmentName, bCallProjectAndSetDefaultPose");
    SCRIPT_REG_TEMPLFUNC(CreateSkinAttachment, "characterSlot, attachmentName");
    SCRIPT_REG_TEMPLFUNC(DestroyAttachment, "characterSlot, attachmentName");
    SCRIPT_REG_TEMPLFUNC(GetAttachmentBone, "characterSlot, attachmentName");
    SCRIPT_REG_TEMPLFUNC(GetAttachmentCGF, "characterSlot, attachmentName");
    SCRIPT_REG_TEMPLFUNC(ResetAttachment, "characterSlot, attachmentName");
    SCRIPT_REG_TEMPLFUNC(SetAttachmentEffect, "characterSlot, attachmentName, effectName, offset, dir, scale, flags");
    SCRIPT_REG_TEMPLFUNC(SetAttachmentObject, "characterSlot, attachmentName, entityId, slot, flags");
    SCRIPT_REG_TEMPLFUNC(SetAttachmentCGF, "characterSlot, attachmentName, filePath");
    SCRIPT_REG_TEMPLFUNC(SetAttachmentLight, "characterSlot, attachmentName, lightTable, flags");
    SCRIPT_REG_TEMPLFUNC(SetAttachmentPos, "characterSlot, attachmentName, pos");
    SCRIPT_REG_TEMPLFUNC(SetAttachmentAngles, "characterSlot, attachmentName, angles");
    SCRIPT_REG_TEMPLFUNC(SetAttachmentDir, "characterSlot, attachmentName, dir, worldSpace");
    SCRIPT_REG_TEMPLFUNC(HideAttachment, "characterSlot, attachmentName, hide, hideShadow");
    SCRIPT_REG_TEMPLFUNC(HideAllAttachments, "characterSlot, hide, hideShadow");
    SCRIPT_REG_TEMPLFUNC(HideAttachmentMaster, "characterSlot, hide");
    SCRIPT_REG_TEMPLFUNC(PhysicalizeAttachment, "characterSlot, attachmentName, physicalize");
    SCRIPT_REG_TEMPLFUNC(GotoState, "sState");
    SCRIPT_REG_TEMPLFUNC(IsInState, "sState");
    SCRIPT_REG_FUNC(GetState);
    SCRIPT_REG_FUNC(IsHidden);
    SCRIPT_REG_FUNC(Damage);
    SCRIPT_REG_FUNC(GetSpeed);

    //////////////////////////////////////////////////////////////////////////
    // Physics.
    SCRIPT_REG_FUNC(GetExplosionObstruction);
    SCRIPT_REG_FUNC(GetExplosionImpulse);
    //////////////////////////////////////////////////////////////////////////

    //  SCRIPT_REG_FUNC(TranslatePartIdToDeadBody);
    SCRIPT_REG_FUNC(SetMaterial);
    SCRIPT_REG_FUNC(GetMaterial);
    SCRIPT_REG_FUNC(GetEntityMaterial);
    SCRIPT_REG_TEMPLFUNC(ChangeAttachmentMaterial, "attachmentName, materialName");
    SCRIPT_REG_TEMPLFUNC(ReplaceMaterial,  "slot, name, replacement");
    SCRIPT_REG_TEMPLFUNC(ResetMaterial, "slot");
    SCRIPT_REG_TEMPLFUNC(CloneMaterial, "nSlot");
    SCRIPT_REG_TEMPLFUNC(SetMaterialFloat, "nSlot,nSubMtlId,sParamName,fValue");
    SCRIPT_REG_TEMPLFUNC(GetMaterialFloat, "nSlot,nSubMtlId,sParamName");
    SCRIPT_REG_TEMPLFUNC(SetMaterialVec3, "nSlot,nSubMtlId,sParamName,vVector");
    SCRIPT_REG_TEMPLFUNC(GetMaterialVec3, "nSlot,nSubMtlId,sParamName");

    /*
        SCRIPT_REG_TEMPLFUNC(AddMaterialLayer, "shader");
        SCRIPT_REG_TEMPLFUNC(RemoveMaterialLayer, "id");
        SCRIPT_REG_TEMPLFUNC(RemoveAllMaterialLayers, "");
        SCRIPT_REG_TEMPLFUNC(SetMaterialLayerParamF, "layerId, name, value");
        SCRIPT_REG_TEMPLFUNC(SetMaterialLayerParamV, "layerId, name, value");
        SCRIPT_REG_TEMPLFUNC(SetMaterialLayerParamC, "layerId, name, r, g, b, a");
        */
    SCRIPT_REG_TEMPLFUNC(EnableMaterialLayer, "enable, layer");
    SCRIPT_REG_TEMPLFUNC(ToLocal, "slot, pos");           // transform pos from world to local coordinates. uses object when slot<0
    SCRIPT_REG_TEMPLFUNC(ToGlobal, "slot, pos");          // transform pos from local to world coordinates. uses object when slot<0
    SCRIPT_REG_TEMPLFUNC(VectorToLocal, "slot, dir");     // transform vector from world to local coordinates. uses object when slot<0
    SCRIPT_REG_TEMPLFUNC(VectorToGlobal, "slot, dir");    // transform vector from local to world coordinates. uses object when slot<0

    SCRIPT_REG_FUNC(SetDefaultIdleAnimations);
    SCRIPT_REG_FUNC(GetVelocity);
    SCRIPT_REG_FUNC(GetVelocityEx);
    SCRIPT_REG_TEMPLFUNC(SetVelocity, "velocity");
    SCRIPT_REG_TEMPLFUNC(SetVelocityEx, "velocity, angularVelocity");
    SCRIPT_REG_FUNC(GetMass);

    SCRIPT_REG_TEMPLFUNC(GetVolume, "slot");
    SCRIPT_REG_TEMPLFUNC(GetGravity, "");
    SCRIPT_REG_TEMPLFUNC(GetSubmergedVolume, "slot, planeNormal, planeOrigin");

    SCRIPT_REG_TEMPLFUNC(CreateLink, "name");
    SCRIPT_REG_TEMPLFUNC(SetLinkTarget, "name, targetId");
    SCRIPT_REG_TEMPLFUNC(GetLinkTarget, "name");
    SCRIPT_REG_TEMPLFUNC(RemoveLink, "name");
    SCRIPT_REG_TEMPLFUNC(RemoveAllLinks, "");
    SCRIPT_REG_TEMPLFUNC(GetLink, "idx");
    SCRIPT_REG_TEMPLFUNC(GetLinkName, "targetId");
    SCRIPT_REG_TEMPLFUNC(CountLinks, "");

    SCRIPT_REG_FUNC(GetTimeSinceLastSeen);
    SCRIPT_REG_FUNC(GetViewDistanceMultiplier);
    SCRIPT_REG_FUNC(SetViewDistanceMultiplier);
    SCRIPT_REG_FUNC(SetViewDistUnlimited);
    SCRIPT_REG_FUNC(GetLodRatio);
    SCRIPT_REG_FUNC(SetLodRatio);
    SCRIPT_REG_FUNC(RemoveDecals);
    SCRIPT_REG_TEMPLFUNC(EnableDecals, "slot, enable");
    SCRIPT_REG_TEMPLFUNC(ForceCharacterUpdate, "characterSlot, alwaysUpdate");
    SCRIPT_REG_TEMPLFUNC(CharacterUpdateAlways, "characterSlot, alwaysUpdate");
    SCRIPT_REG_TEMPLFUNC(CharacterUpdateOnRender, "characterSlot, bUpdateOnRender");
    SCRIPT_REG_TEMPLFUNC(SetAnimateOffScreenShadow, "bAnimateOffScreenShadow");
    SCRIPT_REG_FUNC(Hide);
    SCRIPT_REG_FUNC(CheckCollisions);
    SCRIPT_REG_FUNC(AwakeEnvironment);
    SCRIPT_REG_FUNC(SetStateClientside);
    SCRIPT_REG_FUNC(NoExplosionCollision);
    SCRIPT_REG_FUNC(GetPhysicalStats);

    SCRIPT_REG_TEMPLFUNC(NoBulletForce, "state");

    SCRIPT_REG_TEMPLFUNC(UpdateAreas, "");
    SCRIPT_REG_TEMPLFUNC(IsPointInsideArea, "areaId, point")
    SCRIPT_REG_TEMPLFUNC(IsEntityInsideArea, "areaId, entityId");

    SCRIPT_REG_TEMPLFUNC(BreakToPieces, "nSlot, nPiecesSlot, fExplodeImp, vHitPt, vHitImp, fLifeTime, fSpeed");
    SCRIPT_REG_TEMPLFUNC(AttachSurfaceEffect, "nSlot, effect, countPerUnit, form, type, countScale, sizeScale");

    SCRIPT_REG_FUNC(ProcessBroadcastEvent);
    SCRIPT_REG_FUNC(ActivateOutput);
    SCRIPT_REG_FUNC(CreateCameraComponent);

    SCRIPT_REG_FUNC(UnSeenFrames);

    SCRIPT_REG_FUNC(GetTimeOfDayHour);

    SCRIPT_REG_FUNC(CreateDRSProxy);

    // avoid having to use integer zero for null entity handles
    ScriptHandle handle;
    handle.n = 0;
    pSS->SetGlobalValue("NULL_ENTITY", handle);

    pSS->SetGlobalValue("MTL_LAYER_FROZEN", MTL_LAYER_FROZEN);
    pSS->SetGlobalValue("MTL_LAYER_WET", MTL_LAYER_WET);

    pSS->SetGlobalValue("PHYSICPARAM_FLAGS", PHYSICPARAM_FLAGS);
    pSS->SetGlobalValue("PHYSICPARAM_COLLISION_CLASS", PHYSICPARAM_COLLISION_CLASS);
    pSS->SetGlobalValue("PHYSICPARAM_PARTICLE", PHYSICPARAM_PARTICLE);
    pSS->SetGlobalValue("PHYSICPARAM_VEHICLE", PHYSICPARAM_VEHICLE);
    pSS->SetGlobalValue("PHYSICPARAM_WHEEL", PHYSICPARAM_WHEEL);
    pSS->SetGlobalValue("PHYSICPARAM_SIMULATION", PHYSICPARAM_SIMULATION);
    pSS->SetGlobalValue("PHYSICPARAM_ARTICULATED", PHYSICPARAM_ARTICULATED);
    pSS->SetGlobalValue("PHYSICPARAM_JOINT", PHYSICPARAM_JOINT);
    pSS->SetGlobalValue("PHYSICPARAM_ROPE", PHYSICPARAM_ROPE);
    pSS->SetGlobalValue("PHYSICPARAM_SOFTBODY", PHYSICPARAM_SOFTBODY);
    pSS->SetGlobalValue("PHYSICPARAM_BUOYANCY", PHYSICPARAM_BUOYANCY);
    pSS->SetGlobalValue("PHYSICPARAM_CONSTRAINT", PHYSICPARAM_CONSTRAINT);
    pSS->SetGlobalValue("PHYSICPARAM_REMOVE_CONSTRAINT", PHYSICPARAM_REMOVE_CONSTRAINT);
    pSS->SetGlobalValue("PHYSICPARAM_PLAYERDYN", PHYSICPARAM_PLAYERDYN);
    pSS->SetGlobalValue("PHYSICPARAM_PLAYERDIM", PHYSICPARAM_PLAYERDIM);
    pSS->SetGlobalValue("PHYSICPARAM_VELOCITY", PHYSICPARAM_VELOCITY);
    pSS->SetGlobalValue("PHYSICPARAM_PART_FLAGS", PHYSICPARAM_PART_FLAGS);
    pSS->SetGlobalValue("PHYSICPARAM_SUPPORT_LATTICE", PHYSICPARAM_SUPPORT_LATTICE);
    pSS->SetGlobalValue("PHYSICPARAM_GROUND_PLANE", PHYSICPARAM_GROUND_PLANE);
    pSS->SetGlobalValue("PHYSICPARAM_FOREIGNDATA", PHYSICPARAM_FOREIGNDATA);
    pSS->SetGlobalValue("PHYSICPARAM_AUTO_DETACHMENT", PHYSICPARAM_AUTO_DETACHMENT);

#if ENABLE_CRY_PHYSICS
    //////////////////////////////////////////////////////////////////////////
    // Remapped physical flags.
    pSS->SetGlobalValue("pef_pushable_by_players", pef_pushable_by_players);
    pSS->SetGlobalValue("pef_cannot_squash_players", pef_cannot_squash_players);
    pSS->SetGlobalValue("pef_monitor_state_changes", pef_monitor_state_changes);
    //pSS->SetGlobalValue("pef_monitor_collisions", pef_monitor_collisions);
    pSS->SetGlobalValue("pef_never_affect_triggers", pef_never_affect_triggers);
    pSS->SetGlobalValue("pef_fixed_damping", pef_fixed_damping);
    pSS->SetGlobalValue("pef_ignore_areas", pef_ignore_areas);
    pSS->SetGlobalValue("lef_push_objects", lef_push_objects);
    pSS->SetGlobalValue("lef_push_players", lef_push_players);
    pSS->SetGlobalValue("particle_single_contact", particle_single_contact);
    pSS->SetGlobalValue("particle_constant_orientation", particle_constant_orientation);
    pSS->SetGlobalValue("particle_no_roll", particle_no_roll);
    pSS->SetGlobalValue("particle_no_path_alignment", particle_no_path_alignment);
    pSS->SetGlobalValue("particle_no_spin", particle_no_spin);
    pSS->SetGlobalValue("pef_traceable", pef_traceable);
    pSS->SetGlobalValue("pef_never_break", pef_never_break);
    pSS->SetGlobalValue("ref_use_simple_solver", ref_use_simple_solver);
    pSS->SetGlobalValue("particle_traceable", particle_traceable);
    pSS->SetGlobalValue("lef_snap_velocities", lef_snap_velocities);
    pSS->SetGlobalValue("pef_disabled", pef_disabled);
    pSS->SetGlobalValue("pef_override_impulse_scale", pef_override_impulse_scale);
    pSS->SetGlobalValue("pef_always_notify_on_deletion", pef_always_notify_on_deletion);

    pSS->SetGlobalValue("rope_collides", rope_collides);
    pSS->SetGlobalValue("rope_traceable", rope_traceable);
    pSS->SetGlobalValue("rope_subdivide_segs", rope_subdivide_segs);
    pSS->SetGlobalValue("geom_colltype_ray", geom_colltype_ray);
    pSS->SetGlobalValue("geom_colltype_vehicle", geom_colltype3);
    pSS->SetGlobalValue("geom_collides", geom_collides);
    pSS->SetGlobalValue("geom_floats", geom_floats);
    pSS->SetGlobalValue("geom_colltype0", geom_colltype0);
    pSS->SetGlobalValue("geom_colltype_player", geom_colltype_player);
    pSS->SetGlobalValue("geom_colltype_explosion", geom_colltype_explosion);
    pSS->SetGlobalValue("geom_colltype_foliage_proxy", geom_colltype_foliage_proxy);
    pSS->SetGlobalValue("ent_static", ent_static);
    pSS->SetGlobalValue("ent_sleeping_rigid", ent_sleeping_rigid);
    pSS->SetGlobalValue("ent_rigid", ent_rigid);
    pSS->SetGlobalValue("ent_living", ent_living);
    pSS->SetGlobalValue("ent_independent", ent_independent);
    pSS->SetGlobalValue("ent_terrain", ent_terrain);
    pSS->SetGlobalValue("ent_all", ent_all);
    pSS->SetGlobalValue("ent_sort_by_mass", ent_sort_by_mass);

    pSS->SetGlobalValue("AREA_BOX", SEntityPhysicalizeParams::AreaDefinition::AREA_BOX);
    pSS->SetGlobalValue("AREA_SPHERE", SEntityPhysicalizeParams::AreaDefinition::AREA_SPHERE);
    pSS->SetGlobalValue("AREA_GEOMETRY", SEntityPhysicalizeParams::AreaDefinition::AREA_GEOMETRY);
    pSS->SetGlobalValue("AREA_SHAPE", SEntityPhysicalizeParams::AreaDefinition::AREA_SHAPE);
    pSS->SetGlobalValue("AREA_CYLINDER", SEntityPhysicalizeParams::AreaDefinition::AREA_CYLINDER);
    pSS->SetGlobalValue("AREA_SPLINE", SEntityPhysicalizeParams::AreaDefinition::AREA_SPLINE);
#endif // ENABLE_CRY_PHYSICS

    pSS->SetGlobalValue("FOREIGNFLAGS_MOVING_PLATFORM", PFF_MOVING_PLATFORM);

    pSS->SetGlobalValue("AUDIO_OBSTRUCTION_TYPE_IGNORE", Audio::eAOOCT_IGNORE);
    pSS->SetGlobalValue("AUDIO_OBSTRUCTION_TYPE_SINGLE", Audio::eAOOCT_SINGLE_RAY);
    pSS->SetGlobalValue("AUDIO_OBSTRUCTION_TYPE_MULTI", Audio::eAOOCT_MULTI_RAY);
    pSS->SetGlobalValue("INVALID_AUDIO_ENVIRONMENT_ID", IntToHandle(INVALID_AUDIO_ENVIRONMENT_ID));

    // Watch our, more then 23 bits cannot be used!
    SCRIPT_REG_GLOBAL(ENTITY_FLAG_WRITE_ONLY);
    SCRIPT_REG_GLOBAL(ENTITY_FLAG_NOT_REGISTER_IN_SECTORS);
    SCRIPT_REG_GLOBAL(ENTITY_FLAG_CALC_PHYSICS);
    SCRIPT_REG_GLOBAL(ENTITY_FLAG_CLIENT_ONLY);
    SCRIPT_REG_GLOBAL(ENTITY_FLAG_SERVER_ONLY);
    SCRIPT_REG_GLOBAL(ENTITY_FLAG_CALCBBOX_USEALL);
    SCRIPT_REG_GLOBAL(ENTITY_FLAG_IGNORE_PHYSICS_UPDATE);
    SCRIPT_REG_GLOBAL(ENTITY_FLAG_TRIGGER_AREAS);
    SCRIPT_REG_GLOBAL(ENTITY_FLAG_NO_SAVE);
    SCRIPT_REG_GLOBAL(ENTITY_FLAG_NO_PROXIMITY);
    SCRIPT_REG_GLOBAL(ENTITY_FLAG_VOLUME_SOUND);
    SCRIPT_REG_GLOBAL(ENTITY_FLAG_AI_HIDEABLE);
    SCRIPT_REG_GLOBAL(ENTITY_FLAG_MODIFIED_BY_PHYSICS);

    // Extended entity flags.
    SCRIPT_REG_GLOBAL(ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE);
    SCRIPT_REG_GLOBAL(ENTITY_FLAG_EXTENDED_CAN_COLLIDE_WITH_MERGED_MESHES);

    // Shadows related.
    //  ETY_FLAG_CASTSHADOWVOLUME         = 0x01000,
    SCRIPT_REG_GLOBAL(ENTITY_FLAG_CASTSHADOW);
    SCRIPT_REG_GLOBAL(ENTITY_FLAG_GOOD_OCCLUDER);

    SCRIPT_REG_GLOBAL(ENTITY_UPDATE_NEVER);
    SCRIPT_REG_GLOBAL(ENTITY_UPDATE_IN_RANGE);
    SCRIPT_REG_GLOBAL(ENTITY_UPDATE_POT_VISIBLE);
    SCRIPT_REG_GLOBAL(ENTITY_UPDATE_VISIBLE);
    SCRIPT_REG_GLOBAL(ENTITY_UPDATE_PHYSICS);
    SCRIPT_REG_GLOBAL(ENTITY_UPDATE_PHYSICS_VISIBLE);
    SCRIPT_REG_GLOBAL(ENTITY_UPDATE_ALWAYS);

#if ENABLE_CRY_PHYSICS
    SCRIPT_REG_GLOBAL(PE_NONE);
    SCRIPT_REG_GLOBAL(PE_STATIC);
    SCRIPT_REG_GLOBAL(PE_LIVING);
    SCRIPT_REG_GLOBAL(PE_RIGID);
    SCRIPT_REG_GLOBAL(PE_WHEELEDVEHICLE);
    SCRIPT_REG_GLOBAL(PE_PARTICLE);
    SCRIPT_REG_GLOBAL(PE_ARTICULATED);
    SCRIPT_REG_GLOBAL(PE_ROPE);
    SCRIPT_REG_GLOBAL(PE_SOFT);
    SCRIPT_REG_GLOBAL(PE_AREA);
#endif // ENABLE_CRY_PHYSICS

    pSS->SetGlobalValue("ATTACHMENT_KEEP_TRANSFORMATION", IEntity::ATTACHMENT_KEEP_TRANSFORMATION);
}

//////////////////////////////////////////////////////////////////////////
CScriptBind_Entity::~CScriptBind_Entity()
{
    Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::RemoveRequestListener, &CScriptBind_Entity::OnAudioTriggerFinishedEvent, this);
}

//////////////////////////////////////////////////////////////////////////
void CScriptBind_Entity::DelegateCalls(IScriptTable* pInstanceTable)
{
    assert(pInstanceTable);
    pInstanceTable->Delegate(m_pMethodsTable);
}

//////////////////////////////////////////////////////////////////////////
IEntity* CScriptBind_Entity::GetEntity(IFunctionHandler* pH)
{
    IEntity* pEntity = NULL;
    //IEntity *pEntity = (IEntity*)pH->GetThis();
    //if (!pEntity)
    {
        ScriptHandle handle;
        // Check if self is a table..
        SmartScriptTable table;
        if (pH->GetSelf(table))
        {
            if (table->GetValue("id", handle))
            {
                EntityId id = (EntityId)handle.n;
                pEntity = m_pEntitySystem->GetEntity(id);
            }
        }
        else
        {
            // Check if self is an EntityId.
            if (pH->GetSelf(handle))
            {
                EntityId id = (EntityId)handle.n;
                pEntity = m_pEntitySystem->GetEntity(id);
            }
        }
    }
    //assert(pEntity);
    return pEntity;
}

//////////////////////////////////////////////////////////////////////////
EntityId CScriptBind_Entity::GetEntityID(IScriptTable* pEntityTable)
{
    ScriptHandle handle;
    if (pEntityTable)
    {
        pEntityTable->GetValue("id", handle);
    }
    return (EntityId)handle.n;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::DeleteThis(IFunctionHandler* pH)
{
    GET_ENTITY;

    // Request to delete entity with this id.
    m_pEntitySystem->RemoveEntity(pEntity->GetId());
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetPos(IFunctionHandler* pH, Vec3 vPos)
{
    GET_ENTITY;
    Matrix34 tm = pEntity->GetWorldTM();
    tm.SetTranslation(vPos);
    pEntity->SetWorldTM(tm);
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetWorldPos(IFunctionHandler* pH, Vec3 vPos)
{
    GET_ENTITY;
    Matrix34 tm = pEntity->GetWorldTM();
    tm.SetTranslation(vPos);
    pEntity->SetWorldTM(tm);
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetWorldPos(IFunctionHandler* pH)
{
    GET_ENTITY;
    return pH->EndFunction(Script::SetCachedVector(pEntity->GetWorldPos(), pH, 1));
}


//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetWorldDir(IFunctionHandler* pH)
{
    GET_ENTITY;
    return pH->EndFunction(Script::SetCachedVector(pEntity->GetForwardDir(), pH, 1));
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetPos(IFunctionHandler* pH)
{
    GET_ENTITY;
    return pH->EndFunction(Script::SetCachedVector(pEntity->GetWorldPos(), pH, 1));
}

/*! Get the position of the entity's physical center of mass
    @return Three component vector containing the entity's center of mass
*/
int CScriptBind_Entity::GetCenterOfMassPos(IFunctionHandler* pH)
{
    GET_ENTITY;
#if ENABLE_CRY_PHYSICS
    pe_status_dynamics sd;
    IPhysicalEntity* pent = pEntity->GetPhysics();
    if (pent)
    {
        pent->GetStatus(&sd);
    }
    else
    {
        sd.centerOfMass = pEntity->GetPos();
    }

    return pH->EndFunction(Script::SetCachedVector(sd.centerOfMass, pH, 1));
#else
    return pH->EndFunction(Script::SetCachedVector(pEntity->GetPos(), pH, 1));
#endif // ENABLE_CRY_PHYSICS
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetWorldBoundsCenter(IFunctionHandler* pH)
{
    GET_ENTITY;
    AABB entityBbox;
    pEntity->GetWorldBounds(entityBbox);

    const Vec3 worldBoundsCenter = (entityBbox.IsEmpty() == false) ? entityBbox.GetCenter() : pEntity->GetWorldPos();

    return pH->EndFunction(Script::SetCachedVector(worldBoundsCenter, pH, 1));
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetAngles(IFunctionHandler* pH, Ang3 vAngles)
{
    GET_ENTITY;

    pEntity->SetRotation(Quat::CreateRotationXYZ(vAngles));

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetWorldAngles(IFunctionHandler* pH, Ang3 vAngles)
{
    GET_ENTITY;

    Matrix34 tm = Matrix33(Quat::CreateRotationXYZ(vAngles));
    tm.SetTranslation(pEntity->GetWorldPos());
    pEntity->SetWorldTM(tm);

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetAngles(IFunctionHandler* pH)
{
    GET_ENTITY;

    Ang3 vAngles;
    vAngles.SetAnglesXYZ(Matrix33(pEntity->GetRotation()));
    return pH->EndFunction(Script::SetCachedVector((Vec3)vAngles, pH, 1));
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetWorldAngles(IFunctionHandler* pH)
{
    GET_ENTITY;

    Ang3 vAngles = pEntity->GetWorldAngles();
    return pH->EndFunction(Script::SetCachedVector((Vec3)vAngles, pH, 1));
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetScale(IFunctionHandler* pH, float fScale)
{
    GET_ENTITY;
    if (!gEnv->bMultiplayer)
    {
        pEntity->SetScale(Vec3(fScale, fScale, fScale));
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetWorldScale(IFunctionHandler* pH, float fScale)
{
    GET_ENTITY;

    pEntity->SetScale(Vec3(fScale, fScale, fScale));

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetScale(IFunctionHandler* pH)
{
    GET_ENTITY;
    float fScale = pEntity->GetScale().x;
    return pH->EndFunction(fScale);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetWorldScale(IFunctionHandler* pH)
{
    GET_ENTITY;
    float fScale = pEntity->GetScale().x;
    return pH->EndFunction(fScale);
}

int CScriptBind_Entity::GetBoneLocal(IFunctionHandler* pH, const char* boneName, Vec3 trgDir)
{
    GET_ENTITY;

    const char* sBoneName;
    if (!pH->GetParams(sBoneName))
    {
        return pH->EndFunction();
    }

    ICharacterInstance* pCharacter = pEntity->GetCharacter(0);
    if (!pCharacter)
    {
        return pH->EndFunction();
    }

    int16 pBone_id = pCharacter->GetIDefaultSkeleton().GetJointIDByName(sBoneName);
    if (pBone_id == -1)
    {
        gEnv->pLog->Log("ERROR: CScriptObjectWeapon::GetBoneLocal: Bone not found: %s", sBoneName);
        return pH->EndFunction();
    }
    Matrix33 trgMat(Matrix33::CreateRotationVDir(trgDir));
    //  Matrix33 boneMat(pEntity->GetSlotWorldTM(0)*pCharacter->GetISkeleton()->GetAbsJMatrixByID(pBone_id));
    Matrix33 boneMat(pEntity->GetSlotWorldTM(0) * Matrix34(pCharacter->GetISkeletonPose()->GetAbsJointByID(pBone_id)));
    Matrix33 localMat(boneMat.GetTransposed() * trgMat);

    Matrix33 m = boneMat * localMat;

    //  Vec3 p((pEntity->GetSlotWorldTM(0)*pCharacter->GetISkeleton()->GetAbsJMatrixByID(pBone_id)).GetTranslation());
    Vec3 p((pEntity->GetSlotWorldTM(0) * Matrix34(pCharacter->GetISkeletonPose()->GetAbsJointByID(pBone_id))).GetTranslation());

    //gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(p, ColorB(255, 255, 0, 255), p-(m.GetColumn(1)*100), ColorB(128, 0, 255, 255));
    //return pH->EndFunction(Ang3::GetAnglesXYZ(Quat(localMat)));
    return pH->EndFunction(localMat.GetColumn(1).normalized());
}


//////////////////////////////////////////////////////////////////////////

int CScriptBind_Entity::CalcWorldAnglesFromRelativeDir(IFunctionHandler* pH, Vec3 dir)
{
    GET_ENTITY;

    if (dir.x == 0 && dir.y == 0 && dir.z == 0)
    {
        dir.y = 1;  // neutral orientation
    }
    Matrix33 dirMat = Matrix33::CreateRotationVDir(dir);
    const Matrix33 entityRotMat(pEntity->GetWorldTM());
    Matrix33 finMat = entityRotMat * dirMat;
    Ang3 ang(finMat);

    return pH->EndFunction(ang);
}

int CScriptBind_Entity::IsEntityInside(IFunctionHandler* pH, ScriptHandle entityId)
{
    IEntity* pProbe = m_pEntitySystem->GetEntity((EntityId)entityId.n);
    if (!pProbe)
    {
        return pH->EndFunction(false);
    }

    GET_ENTITY;

    AABB aabb;
    pEntity->GetLocalBounds(aabb);

    OBB obb;
    obb.SetOBBfromAABB(Matrix33(pEntity->GetWorldTM()), aabb);

    AABB paabb;
    pProbe->GetLocalBounds(paabb);

    OBB pobb;
    pobb.SetOBBfromAABB(Matrix33(pProbe->GetWorldTM()), paabb);

    if (Overlap::OBB_OBB(pEntity->GetWorldPos(), obb, pProbe->GetWorldPos(), pobb))
    {
        return pH->EndFunction(true);
    }

    return pH->EndFunction(false);
}
/*! Orient the entity to look at a target position in world space.
@param target The target location to look at
@param axis Adjustment axis
@param angle Adjustment angle about the axis (radians)
*/
int CScriptBind_Entity::LookAt(IFunctionHandler* pH, Vec3 target, Vec3 axis, float angle)
{
    GET_ENTITY;

    // compose a rotation matrix from a desired view direction which is
    // from the current position to the target location
    auto pos = pEntity->GetWorldPos();
    auto forward = (target - pos);
    forward.normalize();

    Vec3 up(0, 0, 1);
    auto right = forward.Cross(up);
    right.Normalize();

    up = right.Cross(forward);

    // Support rotating the model by a correction or offset amounts
    auto rotation = Quat::CreateRotationAA(angle, axis);
    auto rot = Matrix33(rotation) * Matrix33(right, forward, up);

    // set the complete world transform
    pEntity->SetWorldTM(Matrix34(rot, pos));

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
/*! Set the position of the entity
@param vec Three component vector containing the entity position
*/
int CScriptBind_Entity::SetLocalPos(IFunctionHandler* pH, Vec3 vPos)
{
    GET_ENTITY;
    pEntity->SetPos(vPos);
    return pH->EndFunction();
}

/*! Get the position of the entity
@return Three component vector containing the entity position
*/
int CScriptBind_Entity::GetLocalPos(IFunctionHandler* pH)
{
    GET_ENTITY;
    return pH->EndFunction(Script::SetCachedVector(pEntity->GetPos(), pH, 1));
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetLocalAngles(IFunctionHandler* pH, Ang3 vAngles)
{
    GET_ENTITY;

    Quat q = Quat::CreateRotationXYZ(vAngles);
    pEntity->SetRotation(q);

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetLocalAngles(IFunctionHandler* pH)
{
    GET_ENTITY;

    Quat q = pEntity->GetRotation();
    Ang3 vAngles = Ang3::GetAnglesXYZ(Matrix33(q));
    return pH->EndFunction((Vec3)vAngles);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetLocalScale(IFunctionHandler* pH, float fScale)
{
    GET_ENTITY;

    pEntity->SetScale(Vec3(fScale, fScale, fScale));

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetLocalScale(IFunctionHandler* pH)
{
    GET_ENTITY;
    float fScale = pEntity->GetScale().x;
    return pH->EndFunction(fScale);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetAIName(IFunctionHandler* pH)
{
    const char* sName;
    if (!pH->GetParams(sName))
    {
        return pH->EndFunction();
    }

    GET_ENTITY;

    if (pEntity->HasAI())
    {
        pEntity->GetAI()->SetName(sName);
    }

    return pH->EndFunction();
}

/*! Set the name of the entity
    @param sName String containing the new entity name
*/
int CScriptBind_Entity::SetName(IFunctionHandler* pH)
{
    GET_ENTITY;

    SCRIPT_CHECK_PARAMETERS(1);
    const char* sName;
    pH->GetParam(1, sName);
    pEntity->SetName(sName);
    return pH->EndFunction();
}

/*! Get the AI name of the entity. Can be different than entity name.
@return AI Name of the entity
*/
int CScriptBind_Entity::GetAIName(IFunctionHandler* pH)
{
    GET_ENTITY;

    SCRIPT_CHECK_PARAMETERS(0);
    const char* sName = "";
    if (pEntity->HasAI())
    {
        sName = pEntity->GetAI()->GetName();
    }

    return pH->EndFunction(sName);
}

int CScriptBind_Entity::SetFlags(IFunctionHandler* pH, int flags, int mode)
{
    GET_ENTITY;

    switch (mode)
    {
    case 1:
        pEntity->SetFlags(pEntity->GetFlags() & flags);
        break;
    case 2:
        pEntity->SetFlags(pEntity->GetFlags() & (~flags));
        break;
    default:
        pEntity->SetFlags(pEntity->GetFlags() | flags);
        break;
    }

    return pH->EndFunction();
}

int CScriptBind_Entity::GetFlags(IFunctionHandler* pH)
{
    GET_ENTITY;

    return pH->EndFunction((int)pEntity->GetFlags());
}

int CScriptBind_Entity::HasFlags(IFunctionHandler* pH, int flags)
{
    GET_ENTITY;

    return pH->EndFunction((bool)((pEntity->GetFlags() & flags) == flags));
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetFlagsExtended(IFunctionHandler* pH, int flags, int mode)
{
    GET_ENTITY;

    switch (mode)
    {
    case 1:
        pEntity->SetFlagsExtended(pEntity->GetFlagsExtended() & flags);
        break;
    case 2:
        pEntity->SetFlagsExtended(pEntity->GetFlagsExtended() & (~flags));
        break;
    default:
        pEntity->SetFlagsExtended(pEntity->GetFlagsExtended() | flags);
        break;
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetFlagsExtended(IFunctionHandler* pH)
{
    GET_ENTITY;

    return pH->EndFunction((int)pEntity->GetFlagsExtended());
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::HasFlagsExtended(IFunctionHandler* pH, int flags)
{
    GET_ENTITY;

    return pH->EndFunction((bool)((pEntity->GetFlagsExtended() & flags) == flags));
}

/*! Get the name of the entity
    @return Name of the entity
*/
int CScriptBind_Entity::GetName(IFunctionHandler* pH)
{
    GET_ENTITY;
    SCRIPT_CHECK_PARAMETERS(0);
    const char* sName = pEntity->GetName();
    return pH->EndFunction(sName);
}

/*! Get the id of the entity in number form
    @return Entity id as number
*/
int CScriptBind_Entity::GetRawId(IFunctionHandler* pH)
{
    GET_ENTITY;
    SCRIPT_CHECK_PARAMETERS(0);
    EntityId id = pEntity->GetId();
    return pH->EndFunction(id);
}

// Return name of the archetype.
int CScriptBind_Entity::GetArchetype(IFunctionHandler* pH)
{
    GET_ENTITY;
    SCRIPT_CHECK_PARAMETERS(0);

    IEntityArchetype* pArchetype = pEntity->GetArchetype();
    if (pArchetype)
    {
        return pH->EndFunction(pArchetype->GetName());
    }
    return pH->EndFunction();
}

// Creates a render component on the entity but doesn't load anything in
int CScriptBind_Entity::CreateRenderComponent(IFunctionHandler* pH)
{
    GET_ENTITY;

    pEntity->GetOrCreateComponent<IComponentRender>();

    return pH->EndFunction();
}

// Check all the currently set shader param callbacks on the render component with the current state of the entity
int CScriptBind_Entity::CheckShaderParamCallbacks(IFunctionHandler* pH)
{
    GET_ENTITY;

    if (IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>())
    {
        pRenderComponent->ClearShaderParamCallbacks();
        pRenderComponent->CheckShaderParamCallbacks();
    }

    return pH->EndFunction();
}

/*! Load an animated character
    @param sFileName File name of the .cid file
    @param nPos Number of the slot for the character
    @see CScriptBind_Entity::StartAnimation
*/
int CScriptBind_Entity::LoadCharacter(IFunctionHandler* pH, int nSlot, const char* sFilename)
{
    GET_ENTITY;

    nSlot = pEntity->LoadCharacter(nSlot, sFilename);
    if (nSlot < 0)
    {
        return pH->EndFunction();
    }

    return pH->EndFunction(nSlot);
}

/*! Load an geom cache
*/
int CScriptBind_Entity::LoadGeomCache(IFunctionHandler* pH, int nSlot, const char* sFilename)
{
#if defined(USE_GEOM_CACHES)
    GET_ENTITY;

    nSlot = pEntity->LoadGeomCache(nSlot, sFilename);
    if (nSlot < 0)
    {
        return pH->EndFunction();
    }

    return pH->EndFunction(nSlot);
#endif
    return pH->EndFunction();
}

/*! Load a static object
@param sFileName File name of the .cgf file
@param nPos Number of the slot for the character (Zero for first 1st person weapons, one for 3rd person weapons)
@see CScriptBind_Entity::StartAnimation
*/
int CScriptBind_Entity::LoadObject(IFunctionHandler* pH, int nSlot, const char* sFilename)
{
    GET_ENTITY;

    const char* ext = PathUtil::GetExt(sFilename);
    if (azstricmp(ext, CRY_SKEL_FILE_EXT) == 0 || azstricmp(ext, CRY_CHARACTER_DEFINITION_FILE_EXT) == 0 || azstricmp(ext, CRY_ANIM_GEOMETRY_FILE_EXT) == 0)
    {
        nSlot = pEntity->LoadCharacter(nSlot, sFilename);
    }
    else
    {
        nSlot = pEntity->LoadGeometry(nSlot, sFilename);
    }

    if (nSlot < 0)
    {
        return pH->EndFunction();
    }

    return pH->EndFunction(nSlot);
}


int CScriptBind_Entity::LoadObjectWithFlags(IFunctionHandler* pH, int nSlot, const char* sFilename, const int loadFlags)
{
    GET_ENTITY;

    const char* ext = PathUtil::GetExt(sFilename);
    if (azstricmp(ext, CRY_SKEL_FILE_EXT) == 0 || azstricmp(ext, CRY_CHARACTER_DEFINITION_FILE_EXT) == 0 || azstricmp(ext, CRY_ANIM_GEOMETRY_FILE_EXT) == 0)
    {
        nSlot = pEntity->LoadCharacter(nSlot, sFilename);
    }
    else
    {
        nSlot = pEntity->LoadGeometry(nSlot, sFilename, NULL, loadFlags);
    }

    if (nSlot < 0)
    {
        return pH->EndFunction();
    }

    return pH->EndFunction(nSlot);
}


int CScriptBind_Entity::LoadObjectLattice(IFunctionHandler* pH, int nSlot)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;

    IStatObj* pStatObj = pEntity->GetStatObj(nSlot);

    if (pStatObj && pStatObj->GetTetrLattice())
    {
        IPhysicalEntity* pPE = pEntity->GetPhysics();

        if (pPE)
        {
            pe_params_part pp;
            pp.partid = nSlot;
            pp.pLattice = pStatObj->GetTetrLattice();
            pPE->SetParams(&pp);
        }
    }
#endif // ENABLE_CRY_PHYSICS

    return pH->EndFunction(nSlot);
}


//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::LoadSubObject(IFunctionHandler* pH, int nSlot, const char* sFilename, const char* sGeomName)
{
    GET_ENTITY;

    nSlot = pEntity->LoadGeometry(nSlot, sFilename, sGeomName);
    if (nSlot == -1)
    {
        return pH->EndFunction();
    }

    return pH->EndFunction(nSlot);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::LoadLight(IFunctionHandler* pH, int nSlot, SmartScriptTable table)
{
    GET_ENTITY;

    CDLight light;
    if (ParseLightParams(table, light))
    {
        pEntity->UpdateLightClipVolumes(light);
        nSlot = pEntity->LoadLight(nSlot, &light);
        if (nSlot < 0)
        {
            return pH->EndFunction();
        }
    }

    return pH->EndFunction(nSlot);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::UpdateLightClipBounds(IFunctionHandler* pH, int nSlot)
{
    GET_ENTITY;

    SEntitySlotInfo slotInfo;

    if (pEntity->GetSlotInfo(nSlot, slotInfo))
    {
        if (slotInfo.pLight)
        {
            CDLight& light = slotInfo.pLight->GetLightProperties();
            if (!pEntity->UpdateLightClipVolumes(light))
            {
                light.m_Flags &= ~DLF_HAS_CLIP_VOLUME;
            }
        }
    }

    return pH->EndFunction();
}


//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetLightColorParams(IFunctionHandler* pH, int nSlot, Vec3 color, float specular_multiplier)
{
    GET_ENTITY;

    SEntitySlotInfo slotInfo;

    if (pEntity->GetSlotInfo(nSlot, slotInfo))
    {
        if (slotInfo.pLight)
        {
            CDLight& light = slotInfo.pLight->GetLightProperties();
            light.SetLightColor(ColorF(color.x, color.y, color.z, 1.0f));
            light.SetSpecularMult(specular_multiplier);
        }
    }

    return pH->EndFunction();
}


//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetSelfAsLightCasterException(IFunctionHandler* pH, int nLightSlot)
{
    GET_ENTITY;

    SEntitySlotInfo slotLightInfo;

    if (!pEntity->GetSlotInfo(nLightSlot, slotLightInfo))
    {
        return pH->EndFunction();
    }

    ILightSource* pLightSource = slotLightInfo.pLight;
    IRenderNode* pRenderNode = NULL;
    if (IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>())
    {
        pRenderNode = pRenderComponent->GetRenderNode();
    }

    if (pLightSource && pRenderNode)
    {
        pLightSource->SetCastingException(pRenderNode);
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::LoadCloud(IFunctionHandler* pH, int nSlot, const char* sFilename)
{
    GET_ENTITY;

    pEntity->LoadCloud(nSlot, sFilename);

    return pH->EndFunction(nSlot);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetCloudMovementProperties(IFunctionHandler* pH, int nSlot, SmartScriptTable table)
{
    GET_ENTITY;

    SCloudMovementProperties properties;
    if (ParseCloudMovementProperties(table, pEntity, properties))
    {
        nSlot = pEntity->SetCloudMovementProperties(nSlot, properties);
        if (nSlot < 0)
        {
            return pH->EndFunction();
        }
    }
    return pH->EndFunction(nSlot);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::LoadFogVolume(IFunctionHandler* pH, int nSlot, SmartScriptTable table)
{
    GET_ENTITY;

    SFogVolumeProperties properties;
    if (false != ParseFogVolumesParams(table, pEntity, properties))
    {
        nSlot = pEntity->LoadFogVolume(nSlot, properties);

        if (nSlot < 0)
        {
            return(pH->EndFunction());
        }
    }

    return(pH->EndFunction(nSlot));
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::FadeGlobalDensity(IFunctionHandler* pH, int nSlot, float fadeTime, float newGlobalDensity)
{
    GET_ENTITY;
    nSlot = pEntity->FadeGlobalDensity(nSlot, fadeTime, newGlobalDensity);
    return pH->EndFunction(nSlot);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::LoadVolumeObject(IFunctionHandler* pH, int nSlot, const char* sFilename)
{
    GET_ENTITY;
    pEntity->LoadVolumeObject(nSlot, sFilename);
    return pH->EndFunction(nSlot);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetVolumeObjectMovementProperties(IFunctionHandler* pH, int nSlot, SmartScriptTable table)
{
    GET_ENTITY;

    SVolumeObjectMovementProperties properties;
    if (ParseVolumeObjectMovementProperties(table, pEntity, properties))
    {
        nSlot = pEntity->SetVolumeObjectMovementProperties(nSlot, properties);
        if (nSlot < 0)
        {
            return pH->EndFunction();
        }
    }
    return pH->EndFunction(nSlot);
}

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
int CScriptBind_Entity::LoadPrismObject(IFunctionHandler* pH, int nSlot)
{
    GET_ENTITY;
    pEntity->LoadPrismObject(nSlot);
    return pH->EndFunction(nSlot);
}
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::LoadParticleEffect(IFunctionHandler* pH, int nSlot, const char* sEffectName, SmartScriptTable table)
{
    GET_ENTITY;

    int nLoadedSlot = -1;
    IParticleEffect* pEffect = gEnv->pParticleManager->FindEffect(sEffectName, pEntity->GetEntityTextDescription());
    if (pEffect)
    {
        SpawnParams params;

        CScriptSetGetChain chain(table);

        // Check obsolete param.
        float _fSpawnPeriod = 0.f;
        chain.GetValue("SpawnPeriod", _fSpawnPeriod);
        if (_fSpawnPeriod != 0.f)
        {
            EntityWarning("Ignoring obsolete SpawnPeriod=%g: set effect '%s' Continuous or PulsePeriod, or entity PulsePeriod",
                _fSpawnPeriod, sEffectName);
        }

        chain.GetValue("PulsePeriod", params.fPulsePeriod);
        chain.GetValue("Strength", params.fStrength);
        chain.GetValue("Scale", params.fSizeScale);
        chain.GetValue("SpeedScale", params.fSpeedScale);
        chain.GetValue("TimeScale", params.fTimeScale);
        chain.GetValue("CountScale", params.fCountScale);
        chain.GetValue("bCountPerUnit", params.bCountPerUnit);
        chain.GetValue("bRegisterByBBox", params.bRegisterByBBox);

        SmartScriptTable audio;

        if (table->GetValueChain("Audio", audio))
        {
            CScriptSetGetChain audiochain(audio);
            audiochain.GetValue("bEnableAudio", params.bEnableAudio);

            const char* sAudioRTPC = "";
            audiochain.GetValue("audioRTPCRtpc", sAudioRTPC);
            params.sAudioRTPC = sAudioRTPC;
        }

        const char* sType = "";
        if (chain.GetValue("esAttachType", sType))
        {
#if !defined(OTHER_ACTIVE)
            TypeInfo(&params.eAttachType).FromString(&params.eAttachType, sType);
#endif
        }

        const char* sForm = "";
        if (chain.GetValue("esAttachForm", sForm))
        {
#if !defined(OTHER_ACTIVE)
            TypeInfo(&params.eAttachForm).FromString(&params.eAttachForm, sForm);
#endif
        }

        bool bPrime = false;
        chain.GetValue("bPrime", bPrime);

        // Load the effect; mark for no serialization; scripts will handle it if needed.
        nLoadedSlot = pEntity->LoadParticleEmitter(nSlot, pEffect, &params, bPrime, false);
    }

    if (nLoadedSlot < 0 && sEffectName && *sEffectName)
    {
        EntityWarning("LoadParticleEffect (from lua). effect: '%s' could not be loaded. Entity: '%s'", sEffectName, pEntity->GetName());
        return pH->EndFunction();
    }

    return pH->EndFunction(nLoadedSlot);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::PreLoadParticleEffect(IFunctionHandler* pH, const char* sEffectName)
{
    IParticleEffect* piEffect = NULL;
    if (sEffectName && sEffectName[0] != '\0')
    {
        piEffect = gEnv->pParticleManager->FindEffect(sEffectName);

        if (piEffect == NULL)
        {
            CryLog("LUA is Unable to Precache Effect <%s> because it does not exist!", sEffectName);

            return pH->EndFunction("");
        }
    }

    return pH->EndFunction(sEffectName);
}

int CScriptBind_Entity::IsSlotParticleEmitter(IFunctionHandler* pH, int slot)
{
    GET_ENTITY;

    SEntitySlotInfo info;
    if (pEntity->GetSlotInfo(slot, info))
    {
        if (info.pParticleEmitter)
        {
            return pH->EndFunction(true);
        }
    }
    return pH->EndFunction();
}

int CScriptBind_Entity::IsSlotLight(IFunctionHandler* pH, int slot)
{
    GET_ENTITY;

    SEntitySlotInfo info;
    if (pEntity->GetSlotInfo(slot, info))
    {
        if (info.pLight)
        {
            return pH->EndFunction(true);
        }
    }
    return pH->EndFunction();
}

int CScriptBind_Entity::IsSlotGeometry(IFunctionHandler* pH, int slot)
{
    GET_ENTITY;

    SEntitySlotInfo info;
    if (pEntity->GetSlotInfo(slot, info))
    {
        if (info.pStatObj || info.pCharacter)
        {
            return pH->EndFunction(true);
        }
    }
    return pH->EndFunction();
}

int CScriptBind_Entity::IsSlotCharacter(IFunctionHandler* pH, int slot)
{
    GET_ENTITY;

    SEntitySlotInfo info;
    if (pEntity->GetSlotInfo(slot, info))
    {
        if (info.pCharacter)
        {
            return pH->EndFunction(true);
        }
    }
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetParentSlot(IFunctionHandler* pH, int child, int parent)
{
    GET_ENTITY;

    pEntity->SetParentSlot(parent, child);
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetParentSlot(IFunctionHandler* pH, int child)
{
    GET_ENTITY;

    if (pEntity->IsSlotValid(child))
    {
        SEntitySlotInfo slotInfo;
        pEntity->GetSlotInfo(child, slotInfo);

        return pH->EndFunction(slotInfo.nParentSlot);
    }

    return pH->EndFunction(-1);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetSlotCount(IFunctionHandler* pH)
{
    GET_ENTITY;

    return pH->EndFunction(pEntity->GetSlotCount());
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetSlotPos(IFunctionHandler* pH, int nSlot)
{
    GET_ENTITY;

    Vec3 pos = pEntity->GetSlotLocalTM(nSlot, true).GetTranslation();
    return pH->EndFunction(Script::SetCachedVector(pos, pH, 2));
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetSlotPos(IFunctionHandler* pH, int nSlot, Vec3 vPos)
{
    GET_ENTITY;

    Matrix34 tm = pEntity->GetSlotLocalTM(nSlot, true);
    tm.SetTranslation(vPos);
    pEntity->SetSlotLocalTM(nSlot, tm);

    return pH->EndFunction();
}

///////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetSlotPosAndDir(IFunctionHandler* pH, int nSlot, Vec3 pos, Vec3 dir)
{
    GET_ENTITY;

    Matrix34 tm = Matrix34(Matrix33::CreateRotationVDir(dir.GetNormalizedSafe()));
    tm.SetTranslation(pos);
    pEntity->SetSlotLocalTM(nSlot, tm);

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetSlotAngles(IFunctionHandler* pH, int nSlot)
{
    GET_ENTITY;

    Matrix33 mNotScaled = Matrix33(pEntity->GetSlotLocalTM(nSlot, true));

    mNotScaled.OrthonormalizeFast();

    Ang3 vAngles = Ang3::GetAnglesXYZ(mNotScaled);

    return pH->EndFunction((Vec3)vAngles);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetSlotAngles(IFunctionHandler* pH, int nSlot, Ang3 vAngles)
{
    GET_ENTITY;

    Matrix34 tm = pEntity->GetSlotLocalTM(nSlot, true);
    Vec3 vPos = tm.GetTranslation();
    float fScale = tm.GetColumn(0).GetLength();
    tm = Matrix34::CreateRotationXYZ(vAngles);
    tm.ScaleColumn(Vec3(fScale, fScale, fScale));
    tm.SetTranslation(vPos);
    pEntity->SetSlotLocalTM(nSlot, tm);

    return pH->EndFunction();
}


//////////////////////////////////////////////////////////////////////////
/*int CScriptBind_Entity::SetSlotAnglesDeg(IFunctionHandler *pH, int nSlot,Vec3 vAnglesDeg)
{
    GET_ENTITY;

    Matrix34 tm = pEntity->GetSlotLocalTM(nSlot, true);
    Vec3 vPos = tm.GetTranslation();
    float fScale = tm.GetColumn(0).GetLength();
    Ang3 vAngles;
    tm = Matrix34::CreateRotationXYZ(vAngles);
    tm.Scale( Vec3(fScale,fScale,fScale) );
    tm.SetTranslation(vPos);
    pEntity->SetSlotLocalTM(nSlot,tm);

    return pH->EndFunction();
}*/


//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetSlotScale(IFunctionHandler* pH, int nSlot)
{
    GET_ENTITY;
    float fScale = pEntity->GetSlotLocalTM(nSlot, true).GetColumn(0).GetLength();
    return pH->EndFunction(fScale);
}

int CScriptBind_Entity::CopySlotTM(IFunctionHandler* pH, int destSlot, int srcSlot, bool includeTranslation)
{
    GET_ENTITY;

    if (destSlot | srcSlot)
    {
        Matrix34 mat = pEntity->GetSlotLocalTM(srcSlot, false);

        if (!includeTranslation) // keep translation
        {
            mat.SetTranslation(pEntity->GetSlotLocalTM(destSlot, false).GetTranslation());
        }

        pEntity->SetSlotLocalTM(destSlot, mat);
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::MultiplyWithSlotTM(IFunctionHandler* pH, int slot, Vec3 pos)
{
    GET_ENTITY;

    if (slot < 0)
    {
        return pH->EndFunction();
    }

    Vec3 multiplied = pEntity->GetSlotLocalTM(slot, false) * pos;
    return pH->EndFunction(multiplied);
}


//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetSlotWorldTM(IFunctionHandler* pH, int nSlot, Vec3 pos, Vec3 dir)
{
    GET_ENTITY;

    Matrix34 worldMat(Matrix33::CreateRotationVDir(dir));
    worldMat.AddTranslation(pos);
    Matrix34 entityMat(pEntity->GetWorldTM());

    pEntity->SetSlotLocalTM(nSlot, entityMat.GetInverted() * worldMat);
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetSlotWorldPos(IFunctionHandler* pH, int nSlot)
{
    GET_ENTITY;

    Matrix34 tm = pEntity->GetSlotWorldTM(nSlot);

    return pH->EndFunction(Script::SetCachedVector(tm.GetTranslation(), pH, 2));
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetSlotWorldDir(IFunctionHandler* pH, int nSlot)
{
    GET_ENTITY;

    Matrix34 tm = pEntity->GetSlotWorldTM(nSlot);

    return pH->EndFunction(Script::SetCachedVector(tm.GetTranslation(), pH, 2));
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetCharacter(IFunctionHandler* pH, int nSlot)
{
    GET_ENTITY;
    ICharacterInstance* pCharacter = pEntity->GetCharacter(nSlot);
    if (pCharacter)
    {
        return pH->EndFunction(ScriptHandle(pCharacter));
    }
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetSlotScale(IFunctionHandler* pH, int nSlot, float fScale)
{
    GET_ENTITY;

    Matrix34 tm = pEntity->GetSlotLocalTM(nSlot, true);
    if (tm.Determinant() != 0.f)
    {
        tm.OrthonormalizeFast();
    }
    else
    {
        tm.SetIdentity();
    }
    tm.ScaleColumn(Vec3(fScale, fScale, fScale));
    pEntity->SetSlotLocalTM(nSlot, tm);

    return pH->EndFunction();
}


//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::IsSlotValid(IFunctionHandler* pH, int nSlot)
{
    GET_ENTITY;
    return pH->EndFunction(pEntity->IsSlotValid(nSlot));
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::DrawSlot(IFunctionHandler* pH, int nSlot, int nEnable)
{
    GET_ENTITY;

    int flags = pEntity->GetSlotFlags(nSlot);

    if (nEnable)
    {
        flags |= ENTITY_SLOT_RENDER;
        if (nEnable == 2)
        {
            flags |= ENTITY_SLOT_RENDER_NEAREST;
        }
    }
    else
    {
        flags &= ~(ENTITY_SLOT_RENDER | ENTITY_SLOT_RENDER_NEAREST);
    }

    pEntity->SetSlotFlags(nSlot, flags);

#if ENABLE_CRY_PHYSICS
    IBreakableManager* pBreakableManager = m_pEntitySystem->GetBreakableManager();

    if (pBreakableManager)
    {
        pBreakableManager->EntityDrawSlot(pEntity, nSlot, flags);
    }
#endif

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::IgnorePhysicsUpdatesOnSlot(IFunctionHandler* pH, int nSlot)
{
    GET_ENTITY;

    int flags = pEntity->GetSlotFlags(nSlot) | ENTITY_SLOT_IGNORE_PHYSICS;
    pEntity->SetSlotFlags(nSlot, flags);

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::FreeSlot(IFunctionHandler* pH, int nSlot)
{
    GET_ENTITY;
    pEntity->FreeSlot(nSlot);
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::FreeAllSlots(IFunctionHandler* pH)
{
    GET_ENTITY;

    int s = 0;
    for (int i = 0; i < pEntity->GetSlotCount(); ++i)
    {
        while (pEntity->GetSlotFlags(s) != 0)
        {
            ++s;
        }
        pEntity->FreeSlot(s);
        ++s;
    }
    return pH->EndFunction();
}

/*! Destroys the physics of an entity object
*/
int CScriptBind_Entity::DestroyPhysics(IFunctionHandler* pH)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;
    IComponentPhysicsPtr pPhysicsComponent = pEntity->GetComponent<IComponentPhysics>();
    if (pPhysicsComponent)
    {
        SEntityPhysicalizeParams params;
        params.type = PE_NONE;
        pPhysicsComponent->Physicalize(params);
    }
#endif // ENABLE_CRY_PHYSICS
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::EnablePhysics(IFunctionHandler* pH, bool bEnable)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;

    IComponentPhysicsPtr pPhysicsComponent = pEntity->GetOrCreateComponent<IComponentPhysics>();
    if (!pPhysicsComponent)
    {
        return pH->EndFunction();
    }
    else
    {
        pPhysicsComponent->EnablePhysics(bEnable);
    }
#endif // ENABLE_CRY_PHYSICS

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::Physicalize(IFunctionHandler* pH, int nSlot, int nPhysicsType, SmartScriptTable physicsParams)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;

    SEntityPhysicalizeParams params;
    params.nSlot = nSlot;
    params.type = nPhysicsType;

    if (ParsePhysicsParams(physicsParams, params))
    {
        pEntity->Physicalize(params);
        IPhysicalEntity* pPhysicalEntity = pEntity->GetPhysics();
        if (pPhysicalEntity)
        {
            pe_simulation_params sim_params;
            sim_params.maxLoggedCollisions = 1; // By default only report max 1 collision event.
            pPhysicalEntity->SetParams(&sim_params);

            // By default such entities are marked as unimportant.
            pe_params_foreign_data pfd;
            pfd.iForeignFlagsOR = PFF_UNIMPORTANT;
            IComponentPhysicsPtr pPhysicsComponent = pEntity->GetComponent<IComponentPhysics>();
            if (!(pPhysicsComponent->GetFlags() & CComponentPhysics::FLAG_ATTACH_CLOTH_WHEN_VISIBLE))
            {
                pPhysicalEntity->SetParams(&pfd);
            }

            if (nPhysicsType == PE_WHEELEDVEHICLE)
            {
                pPhysicalEntity->SetParams(params.pCar);
            }
        }
    }
#endif // ENABLE_CRY_PHYSICS

    return pH->EndFunction();
}


int CScriptBind_Entity::ReattachSoftEntityVtx(IFunctionHandler* pH, ScriptHandle entityId, int partId)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;

    EntityId attachEntityToId = (EntityId)entityId.n;
    IEntity* pEntityAttach = gEnv->pEntitySystem->GetEntity(attachEntityToId);
    IPhysicalEntity* pPhysEntityAttach = pEntityAttach ? pEntityAttach->GetPhysics() : NULL;

    IComponentPhysicsPtr pPhysicsComponent = pEntity->GetComponent<IComponentPhysics>();
    pPhysicsComponent->ReattachSoftEntityVtx(pPhysEntityAttach, partId);
#endif // ENABLE_CRY_PHYSICS

    return pH->EndFunction();
}


//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::PhysicalizeSlot(IFunctionHandler* pH, int slot, SmartScriptTable physicsParams)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;

    SEntityPhysicalizeParams params;
    params.nSlot = slot;

    if (ParsePhysicsParams(physicsParams, params))
    {
        pEntity->PhysicalizeSlot(slot, params);

        IPhysicalEntity* pe = pEntity->GetPhysics();
        if (!pe)
        {
            return pH->EndFunction();
        }

        // handle flags
        pe_params_part partParams;
        partParams.partid = slot;

        int flagsMask = 0;
        int flagsCollider = 0;
        int flagsColliderMask = 0;

        physicsParams->GetValue("flagsMask", flagsMask);
        physicsParams->GetValue("flagsCollider", flagsCollider);
        physicsParams->GetValue("flagsColliderMask", flagsColliderMask);

        partParams.flagsAND = ~flagsMask;
        partParams.flagsColliderOR = flagsCollider;
        partParams.flagsColliderAND = ~flagsColliderMask;

        pe->SetParams(&partParams);
    }
#endif // ENABLE_CRY_PHYSICS

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::UpdateSlotPhysics(IFunctionHandler* pH, int slot)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;

    pEntity->UpdateSlotPhysics(slot);
#endif // ENABLE_CRY_PHYSICS

    return pH->EndFunction();
}

//  Get Forward direction if no parameters
//  if parameter is passed - get Up direction
//  [filippo]: now more complete, if the param is specified we can get the axis we want.
int CScriptBind_Entity::GetDirectionVector(IFunctionHandler* pH)
{
    GET_ENTITY;

    //forward default
    Vec3 vec(0, 1, 0);

    //if there is a parameter we want to get something different by the forward vector, 0=x, 1=y, 2=z
    if ((pH->GetParamCount() >= 1) && (pH->GetParamType(1) == svtNumber))
    {
        int dir = 0;
        pH->GetParam(1, dir);

        switch (dir)
        {
        case 0: // X
            vec.Set(1, 0, 0);
            break;
        case 1: // Y
            vec.Set(0, 1, 0);
            break;
        case 2: // Z
            vec.Set(0, 0, 1);
            break;
        }
    }

    vec = pEntity->GetWorldTM().TransformVector(vec);
    return pH->EndFunction(Script::SetCachedVector(vec, pH, 2));
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetDirectionVector(IFunctionHandler* pH, Vec3 dir)
{
    GET_ENTITY;

    Vec3 up(0, 0, 1);
    if (pH->GetParamCount() > 1)
    {
        pH->GetParam(2, up);
    }

    Vec3 right(dir ^ up);

    if (right.len2() < 0.01f)
    {
        right = dir.GetOrthogonal();
    }

    Matrix34 tm(pEntity->GetWorldTM());

    tm.SetColumn(1, dir);
    tm.SetColumn(0, right.normalize());
    tm.SetColumn(2, right ^ dir);

    pEntity->SetWorldTM(tm);

    return pH->EndFunction();
}


//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::CreateBoneAttachment(IFunctionHandler* pH, int characterSlot, const char* boneName, const char* attachmentName)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (!pCharacter)
    {
        return pH->EndFunction();
    }

    IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();
    IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByName(attachmentName);

    bool bCallProjectAndSetDefaultPose = true;
    if (pH->GetParamCount() > 3)
    {
        pH->GetParam(4, bCallProjectAndSetDefaultPose);
    }

    if (!pIAttachment)
    {
        pIAttachment = pIAttachmentManager->CreateAttachment(attachmentName, CA_BONE, boneName, bCallProjectAndSetDefaultPose);
    }

    return pH->EndFunction();
}

int CScriptBind_Entity::CreateSkinAttachment(IFunctionHandler* pH, int characterSlot, const char* attachmentName)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (!pCharacter)
    {
        return pH->EndFunction();
    }

    IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();
    IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByName(attachmentName);

    if (!pIAttachment)
    {
        pIAttachment = pIAttachmentManager->CreateAttachment(attachmentName, CA_SKIN);
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::DestroyAttachment(IFunctionHandler* pH, int characterSlot, const char* attachmentName)
{
    if (GetISystem()->IsSerializingFile())
    {
        return pH->EndFunction();
    }

    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (!pCharacter)
    {
        return pH->EndFunction();
    }

    IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();
    pIAttachmentManager->RemoveAttachmentByName(attachmentName);

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetAttachmentBone(IFunctionHandler* pH, int characterSlot, const char* attachmentName)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (!pCharacter)
    {
        return pH->EndFunction();
    }

    IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();

    if (pIAttachmentManager)
    {
        IAttachment* pAttachment = pIAttachmentManager->GetInterfaceByName(attachmentName);

        if (pAttachment)
        {
            return pH->EndFunction(pCharacter->GetIDefaultSkeleton().GetJointNameByID(pAttachment->GetJointID()));
        }
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetAttachmentCGF(IFunctionHandler* pH, int characterSlot, const char* attachmentName)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (!pCharacter)
    {
        return pH->EndFunction();
    }

    IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();

    if (pIAttachmentManager)
    {
        if (IAttachment* pAttachment = pIAttachmentManager->GetInterfaceByName(attachmentName))
        {
            if (IAttachmentObject* pAttachmentObject = pAttachment->GetIAttachmentObject())
            {
                if (IStatObj* pStatObj = pAttachmentObject->GetIStatObj())
                {
                    return pH->EndFunction(pStatObj->GetFilePath());
                }
            }
        }
    }

    return pH->EndFunction();
}



//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::ResetAttachment(IFunctionHandler* pH, int characterSlot, const char* attachmentName)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (!pCharacter)
    {
        return pH->EndFunction();
    }

    IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();
    IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByName(attachmentName);

    if (pIAttachment)
    {
        pIAttachment->ClearBinding();
    }

    return pH->EndFunction();
}

int CScriptBind_Entity::SetAttachmentEffect(IFunctionHandler* pH, int characterSlot, const char* attachmentName, const char* effectName, Vec3 offset, Vec3 dir, float scale, int flags)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (!pCharacter)
    {
        return pH->EndFunction();
    }

    IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();
    IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByName(attachmentName);

    if (!pIAttachment)
    {
        return pH->EndFunction();
    }

    CEffectAttachment* pEffectAttachment = new CEffectAttachment(effectName, offset, dir, scale, flags);

    pIAttachment->AddBinding(pEffectAttachment);
    pIAttachment->HideAttachment(0);

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetAttachmentObject(IFunctionHandler* pH, int characterSlot, const char* attachmentName, ScriptHandle entityId, int slot, int flags)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);
    IEntity* pAttachEntity = m_pEntitySystem->GetEntity((EntityId)entityId.n);

    if (!pCharacter || !pAttachEntity)
    {
        return pH->EndFunction();
    }

    IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();
    IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByName(attachmentName);

    if (!pIAttachment)
    {
        return pH->EndFunction();
    }

    // slot attachment
    if (slot >= 0)
    {
        IStatObj* pIStatObj = pAttachEntity->GetStatObj(slot);
        ICharacterInstance* pICharInstance = pAttachEntity->GetCharacter(slot);

        // static object
        if (pIStatObj)
        {
            CCGFAttachment* pStatAttachment = new CCGFAttachment();
            pStatAttachment->pObj  = pIStatObj;

            pIAttachment->AddBinding(pStatAttachment);
        }
        else if (pICharInstance)
        {
            CSKELAttachment* pCharacterAttachment = new CSKELAttachment();
            pCharacterAttachment->m_pCharInstance  = pICharInstance;

            pIAttachment->AddBinding(pCharacterAttachment);
        }
        else
        {
            return pH->EndFunction();
        }
    }
    else // entity attachment
    {
        CEntityAttachment* pEntityAttachment = new CEntityAttachment();
        pEntityAttachment->SetEntityId(pAttachEntity->GetId());

        pIAttachment->AddBinding(pEntityAttachment);
    }

    pIAttachment->HideAttachment(0);

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetAttachmentCGF(IFunctionHandler* pH, int characterSlot, const char* attachmentName, const char* filePath)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (!pCharacter)
    {
        return pH->EndFunction();
    }

    IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();
    IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByName(attachmentName);

    if (!pIAttachment)
    {
        return pH->EndFunction();
    }

    if (_smart_ptr<IStatObj> pIStatObj = gEnv->p3DEngine->LoadStatObjAutoRef(filePath))
    {
        CCGFAttachment* pStatAttachment = new CCGFAttachment();
        pStatAttachment->pObj = pIStatObj;

        pIAttachment->AddBinding(pStatAttachment);
    }

    pIAttachment->HideAttachment(0);

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetAttachmentLight(IFunctionHandler* pH, int characterSlot, const char* attachmentName, SmartScriptTable lightTable, int flags)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (!pCharacter)
    {
        return pH->EndFunction();
    }

    IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();
    IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByName(attachmentName);

    if (!pIAttachment)
    {
        return pH->EndFunction();
    }

    CDLight light;

    if (ParseLightParams(lightTable, light))
    {
        CLightAttachment* pLightAttachment = new CLightAttachment();
        pLightAttachment->LoadLight(light);

        pIAttachment->AddBinding(pLightAttachment);
        pIAttachment->HideAttachment(0);
    }

    return pH->EndFunction();
}

int CScriptBind_Entity::SetAttachmentPos(IFunctionHandler* pH, int characterSlot, const char* attachmentName, Vec3 pos)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (!pCharacter)
    {
        return pH->EndFunction();
    }

    IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();
    IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByName(attachmentName);

    if (pIAttachment)
    {
        QuatT lm = pIAttachment->GetAttRelativeDefault();
        lm.t = pos;

        pIAttachment->SetAttRelativeDefault(lm);
    }

    return pH->EndFunction();
}

int CScriptBind_Entity::SetAttachmentAngles(IFunctionHandler* pH, int characterSlot, const char* attachmentName, Vec3 angles)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (!pCharacter)
    {
        return pH->EndFunction();
    }

    IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();
    IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByName(attachmentName);

    if (pIAttachment)
    {
        QuatT lm = pIAttachment->GetAttRelativeDefault();
        //  Vec3 t = lm.t;
        lm.q = Quat::CreateRotationXYZ(Ang3(angles));
        //  lm.t=t;

        pIAttachment->SetAttRelativeDefault(lm);
    }

    return pH->EndFunction();
}

int CScriptBind_Entity::SetAttachmentDir(IFunctionHandler* pH, int characterSlot, const char* attachmentName, Vec3 dir, bool worldSpace)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (!pCharacter)
    {
        return pH->EndFunction();
    }

    IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();
    IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByName(attachmentName);

    if (pIAttachment)
    {
        Quat rot = Quat::CreateRotationVDir(dir);

        if (worldSpace && pIAttachment->GetType() == CA_BONE)
        {
            // worldspace -> bonespace
            Quat rotCharInv = Quat(pEntity->GetSlotWorldTM(characterSlot)).GetInverted();
            Quat rotBoneInv = pCharacter->GetISkeletonPose()->GetAbsJointByID(pIAttachment->GetJointID()).q.GetInverted();
            rot = rotBoneInv * rotCharInv * rot;
        }

        QuatT lm = pIAttachment->GetAttRelativeDefault();
        //Vec3 t = lm.GetTranslation();
        lm.q = rot;
        //lm.SetTranslation(t);
        pIAttachment->SetAttRelativeDefault(lm);

        /*
        // draw world transformation
        Matrix34 tm = pEntity->GetSlotWorldTM(characterSlot) * Matrix34(pIAttachment->GetLQuatT());
        IRenderAuxGeom* pGeom = gEnv->pRenderer->GetIRenderAuxGeom();
        pGeom->SetRenderFlags(e_Def3DPublicRenderflags);
        ColorB red(255,0,0,255);
        ColorB green(0,255,0,255);
        pGeom->DrawSphere(tm.GetTranslation(), 1.f, red);
        pGeom->DrawLine(tm.GetTranslation(), red, tm.GetTranslation()+3.f*tm.GetColumn0(), red);
        pGeom->DrawLine(tm.GetTranslation(), green, tm.GetTranslation()+3.f*tm.GetColumn1(), green);
        */
    }

    return pH->EndFunction();
}
//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::HideAttachment(IFunctionHandler* pH, int characterSlot, const char* attachmentName, bool hide, bool hideShadow)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (!pCharacter)
    {
        return pH->EndFunction();
    }

    IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();

    if (pIAttachmentManager)
    {
        IAttachment* pAttachment = pIAttachmentManager->GetInterfaceByName(attachmentName);

        if (pAttachment)
        {
            pAttachment->HideAttachment(hide);
            pAttachment->HideInRecursion(hideShadow);
            pAttachment->HideInShadow(hideShadow);
        }
    }

    return pH->EndFunction();
}


//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::HideAllAttachments(IFunctionHandler* pH, int characterSlot, bool hide, bool hideShadow)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (!pCharacter)
    {
        return pH->EndFunction();
    }

    IAttachmentManager* pIAttachmentManager = pCharacter->GetIAttachmentManager();

    if (pIAttachmentManager)
    {
        int n = pIAttachmentManager->GetAttachmentCount();
        for (int i = 0; i < n; i++)
        {
            IAttachment* pAttachment = pIAttachmentManager->GetInterfaceByIndex(i);
            if (pAttachment)
            {
                pAttachment->HideAttachment(hide);
                pAttachment->HideInRecursion(hideShadow);
                pAttachment->HideInShadow(hideShadow);
            }
        }
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::HideAttachmentMaster(IFunctionHandler* pH, int characterSlot, bool hide)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (pCharacter)
    {
        pCharacter->HideMaster(hide ? 1 : 0);
    }

    return pH->EndFunction();
}

int CScriptBind_Entity::PhysicalizeAttachment(IFunctionHandler* pH, int characterSlot, const char* attachmentName, bool physicalize)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;

    if (ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot))
    {
        if (IAttachmentManager* pAttachmentManager = pCharacter->GetIAttachmentManager())
        {
            int index = pAttachmentManager->GetIndexByName(attachmentName);

            if (index > -1)
            {
                if (physicalize)
                {
                    pAttachmentManager->PhysicalizeAttachment(index);
                }
                else
                {
                    pAttachmentManager->DephysicalizeAttachment(index);
                }
            }
        }
    }
#else
    AZ_UNUSED(characterSlot);
    AZ_UNUSED(attachmentName);
    AZ_UNUSED(physicalize);
#endif // ENABLE_CRY_PHYSICS

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::AttachChild(IFunctionHandler* pH, ScriptHandle childEntityId, int flags)
{
    GET_ENTITY;

    IEntity* pChildEntity = m_pEntitySystem->GetEntity((EntityId)childEntityId.n);

    if (pChildEntity)
    {
        if (pEntity == pChildEntity)
        {
            gEnv->pLog->LogError("FATAL ERROR: CScriptBind_Entity::AttachChild '%s' to itself", pEntity->GetName());
            assert(0);
        }
        else
        {
            pEntity->AttachChild(pChildEntity, flags);
        }
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::DetachThis(IFunctionHandler* pH)
{
    GET_ENTITY;

    int nFlags = 0;
    if (pH->GetParamCount() >= 1)
    {
        pH->GetParam(1, nFlags); // Optional flags.
    }
    pEntity->DetachThis(nFlags);

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::DetachAll(IFunctionHandler* pH)
{
    GET_ENTITY;

    int nFlags = 0;
    pH->GetParam(1, nFlags); // Optional flags.
    pEntity->DetachAll(nFlags);

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetParent(IFunctionHandler* pH)
{
    GET_ENTITY;

    IEntity* pParent = pEntity->GetParent();
    if (pParent)
    {
        IScriptTable* pParentTable = pParent->GetScriptTable();
        if (pParentTable)
        {
            return pH->EndFunction(pParentTable);
        }
    }
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetChildCount(IFunctionHandler* pH)
{
    GET_ENTITY;
    return pH->EndFunction(pEntity->GetChildCount());
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetChild(IFunctionHandler* pH, int nIndex)
{
    GET_ENTITY;

    if (nIndex >= 0 && nIndex < pEntity->GetChildCount())
    {
        IEntity* pChild = pEntity->GetChild(nIndex);
        if (pChild && pChild->GetScriptTable())
        {
            return pH->EndFunction(pChild->GetScriptTable());
        }
    }
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::EnableInheritXForm(IFunctionHandler* pH, bool bEnable)
{
    GET_ENTITY;

    pEntity->EnableInheritXForm(bEnable);
    return pH->EndFunction();
}

int CScriptBind_Entity::NetPresent(IFunctionHandler* pH)
{
    GET_ENTITY;
    SCRIPT_CHECK_PARAMETERS(1);

    bool bPresence;

    pH->GetParam(1, bPresence);
    //pEntity->SetNetPresence(bPresence);

    return pH->EndFunction();
}

// true=prevents error when state changes on the client and does not sync state changes to the client
int CScriptBind_Entity::SetStateClientside(IFunctionHandler* pH)
{
    GET_ENTITY;
    SCRIPT_CHECK_PARAMETERS(1);

    bool bEnable;

    pH->GetParam(1, bEnable);
    //pEntity->SetStateClientside(bEnable);

    return pH->EndFunction();
}

/*! Sets the playback time of a geometry cache
*/
int CScriptBind_Entity::SetGeomCachePlaybackTime(IFunctionHandler* pH, float time)
{
#if defined(USE_GEOM_CACHES)
    GET_ENTITY;

    IGeomCacheRenderNode* pGeomCacheRenderNode = pEntity->GetGeomCacheRenderNode(0);

    if (pGeomCacheRenderNode)
    {
        pGeomCacheRenderNode->SetPlaybackTime(time);
    }
#endif

    return pH->EndFunction();
}

/*! Sets the parameters of a geometry cache
*/
int CScriptBind_Entity::SetGeomCacheParams(IFunctionHandler* pH, bool looping, const char* standIn, const char* standInMaterial, const char* firstFrameStandIn,
    const char* firstFrameStandInMaterial, const char* lastFrameStandIn, const char* lastFrameStandInMaterial, float standInDistance, float streamInDistance)
{
#if defined(USE_GEOM_CACHES)
    GET_ENTITY;

    IGeomCacheRenderNode* pGeomCacheRenderNode = pEntity->GetGeomCacheRenderNode(0);
    if (pGeomCacheRenderNode)
    {
        pGeomCacheRenderNode->SetLooping(looping);

        if (strcmp(standIn, "") != 0)
        {
            pGeomCacheRenderNode->SetStandIn(standIn, standInMaterial);
        }

        if (strcmp(firstFrameStandIn, "") != 0)
        {
            pGeomCacheRenderNode->SetFirstFrameStandIn(firstFrameStandIn, firstFrameStandInMaterial);
        }

        if (strcmp(lastFrameStandIn, "") != 0)
        {
            pGeomCacheRenderNode->SetLastFrameStandIn(lastFrameStandIn, lastFrameStandInMaterial);
        }

        pGeomCacheRenderNode->SetStandInDistance(standInDistance);
        pGeomCacheRenderNode->SetStreamInDistance(streamInDistance);
    }
#endif

    return pH->EndFunction();
}

/*! Activates/deactivates geom cache streaming
*/
int CScriptBind_Entity::SetGeomCacheStreaming(IFunctionHandler* pH, bool active, float time)
{
#if defined(USE_GEOM_CACHES)
    GET_ENTITY;

    IGeomCacheRenderNode* pGeomCacheRenderNode = pEntity->GetGeomCacheRenderNode(0);
    if (pGeomCacheRenderNode)
    {
        if (active)
        {
            pGeomCacheRenderNode->StartStreaming(time);
        }
        else
        {
            pGeomCacheRenderNode->StopStreaming();
        }
    }
#endif

    return pH->EndFunction();
}

/*! Returns true if geom cache is streaming
*/
int CScriptBind_Entity::IsGeomCacheStreaming(IFunctionHandler* pH)
{
#if defined(USE_GEOM_CACHES)
    GET_ENTITY;

    bool bIsStreaming = false;

    IGeomCacheRenderNode* pGeomCacheRenderNode = pEntity->GetGeomCacheRenderNode(0);
    if (pGeomCacheRenderNode)
    {
        bIsStreaming = pGeomCacheRenderNode->IsStreaming();
    }

    return pH->EndFunction(bIsStreaming);
#endif

    return pH->EndFunction(false);
}

/*! Activates/deactivates geom cache drawing
*/
int CScriptBind_Entity::SetGeomCacheDrawing(IFunctionHandler* pH, bool active)
{
#if defined(USE_GEOM_CACHES)
    GET_ENTITY;

    IGeomCacheRenderNode* pGeomCacheRenderNode = pEntity->GetGeomCacheRenderNode(0);
    if (pGeomCacheRenderNode)
    {
        pGeomCacheRenderNode->SetDrawing(active);
    }
#endif

    return pH->EndFunction();
}

/*! Activates/deactivates geom cache drawing
*/
int CScriptBind_Entity::GetGeomCachePrecachedTime(IFunctionHandler* pH)
{
#if defined(USE_GEOM_CACHES)
    GET_ENTITY;

    float precachedTime = 0.0f;

    IGeomCacheRenderNode* pGeomCacheRenderNode = pEntity->GetGeomCacheRenderNode(0);
    if (pGeomCacheRenderNode)
    {
        precachedTime = pGeomCacheRenderNode->GetPrecachedTime();
    }

    return pH->EndFunction(precachedTime);
#endif

    return pH->EndFunction(0.0f);
}

/*! Starts an animation of a character
    @param pos Number of the character slot
    @param animname Name of the animation from the .chrparams file
    @see CScriptBind_Entity::LoadCharacter
    @see CEntity::StartAnimation
*/
int CScriptBind_Entity::StartAnimation(IFunctionHandler* pH)
{
    GET_ENTITY;

    //SCRIPT_CHECK_PARAMETERS(2);
    const char* animname;
    int pos, layer = 0;

    bool bLooping = false;
    bool bRecursive(false);
    bool partial(false);
    bool bForceSingleFrame(false);

    float fBlendTime = 0.15f;
    float fAniSpeed = 1.0f;

    pH->GetParam(1, pos);
    if (!pH->GetParam(2, animname))
    {
        m_pISystem->Warning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, 0,
            0, "CScriptBind_Entity::StartAnimation, animation name not specified, in Entity %s", pEntity->GetName());
        return pH->EndFunction(false);
    }

    if (pH->GetParamCount() > 2)
    {
        pH->GetParam(3, layer);
        if (pH->GetParamCount() > 3)
        {
            pH->GetParam(4, fBlendTime);
            if (pH->GetParamCount() > 4)
            {
                pH->GetParam(5, fAniSpeed);
                if (pH->GetParamCount() > 5)
                {
                    pH->GetParam(6, bLooping);

                    if (pH->GetParamCount() > 6)
                    {
                        pH->GetParam(7, bRecursive);

                        if (pH->GetParamCount() > 7)
                        {
                            pH->GetParam(8, partial);

                            if (pH->GetParamCount() > 8)
                            {
                                pH->GetParam(9, bForceSingleFrame);
                            }
                        }
                    }
                }
            }
        }
    }

    ICharacterInstance* pCharacter = pEntity->GetCharacter(pos);
    // no character no animation
    if (!pCharacter)
    {
        return pH->EndFunction(false);
    }

    if (azstricmp(animname, "NULL") == 0)
    {
        bool result = pCharacter->GetISkeletonAnim()->StopAnimationInLayer(layer, 0.0f);
        return pH->EndFunction(result);
    }


    CryCharAnimationParams params;
    params.m_nLayerID = layer;
    params.m_fTransTime = fBlendTime;
    params.m_nFlags = 0;

    //if (bRecursive)
    //  params.m_nFlags |= CA_RECURSIVE;

    bool result(false);

    ISkeletonAnim* pISkeletonAnim = pCharacter->GetISkeletonAnim();

    //Note: If not looping, repeat last key
    //New MT animation requires something to be playing always
    //Since most script calls are just trigger and forget (like aim objects), this should be a good assumption)
    params.m_nFlags |= bLooping ? CA_LOOP_ANIMATION : CA_REPEAT_LAST_KEY;
    if (bForceSingleFrame)
    {
        params.m_nFlags |= CA_FORCE_SKELETON_UPDATE | CA_FORCE_TRANSITION_TO_ANIM;
    }

    //CryLog("StartAnimation:%s %f",pEntity->GetName(),fAniSpeed);

    int animID = pCharacter->GetIAnimationSet()->GetAnimIDByName(animname);

    if (animID < 0)
    {
        CryLog("CScriptBind_Entity::StartAnimation Anim %s not found in entity %s", animname, pEntity->GetName());
        return pH->EndFunction(result);
    }

    float duration = pCharacter->GetIAnimationSet()->GetDuration_sec(animID);

    //pISkeleton->SetRedirectToLayer0(1);
    result = pISkeletonAnim->StartAnimationById(animID, params);

    pCharacter->GetISkeletonAnim()->SetLayerPlaybackScale(params.m_nLayerID, fAniSpeed);

    if (bForceSingleFrame)
    {
        if (!pCharacter->GetIAnimationSet()->IsAnimLoaded(animID))
        {
            gEnv->pCharacterManager->CAF_LoadSynchronously(pCharacter->GetIAnimationSet()->GetFilePathCRCByAnimID(animID));
        }
        SAnimationProcessParams app;
        app.locationAnimation = QuatTS(pEntity->GetRotation(), pEntity->GetPos(), pEntity->GetScale().x);
        app.bOnRender = 0;
        app.zoomAdjustedDistanceFromCamera = 0.1f;
        app.overrideDeltaTime = 0.001f;
        pCharacter->StartAnimationProcessing(app);
        pCharacter->FinishAnimationComputations();
        pISkeletonAnim->StopAnimationsAllLayers();
    }

    return pH->EndFunction(result, duration);

    //  return pH->EndFunction(pEntity->StartAnimation(pos, animname, layer, fBlendTime));

    //  pEntity->StartAnimation(pos, animname, layer, fBlendTime);
    //  return pH->EndFunction();
}


int CScriptBind_Entity::StopAnimation(IFunctionHandler* pH, int characterSlot, int layer)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (pCharacter)
    {
        if (layer < 0)
        {
            pCharacter->GetISkeletonAnim()->StopAnimationsAllLayers();
        }
        else
        {
            pCharacter->GetISkeletonAnim()->StopAnimationInLayer(layer, 0.0f);
        }
    }

    return pH->EndFunction();
}

/*! Resets the animation of a character
    @param pos Number of the character slot
    @see CScriptBind_Entity::LoadCharacter
    @see CEntity::StartAnimation
*/
int CScriptBind_Entity::ResetAnimation(IFunctionHandler* pH, int characterSlot, int layer)
{
    return StopAnimation(pH, characterSlot, layer);
}

int CScriptBind_Entity::RedirectAnimationToLayer0(IFunctionHandler* pH, int characterSlot, bool redirect)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (pCharacter)
    {
        //pCharacter->GetISkeleton()->SetRedirectToLayer0(redirect);
    }

    return pH->EndFunction();
}

int CScriptBind_Entity::SetAnimationBlendOut(IFunctionHandler* pH, int characterSlot, int layer, float blendOut)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (pCharacter)
    {
        //pCharacter->SetBlendOutTime(layer, blendOut);
    }

    return pH->EndFunction();
}

int CScriptBind_Entity::EnableBoneAnimation(IFunctionHandler* pH, int characterSlot, int layer, const char* boneName, bool status)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (pCharacter)
    {
        //      pCharacter->GetISkeletonAnim()->SetJointMask(boneName, layer, status ? 1 : 0);
    }

    return pH->EndFunction();
}


int CScriptBind_Entity::EnableBoneAnimationAll(IFunctionHandler* pH, int characterSlot, int layer, bool status)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (pCharacter)
    {
        //      pCharacter->GetISkeletonAnim()->SetLayerMask(layer, status ? 1 : 0);
    }

    return pH->EndFunction();
}


int CScriptBind_Entity::EnableProceduralFacialAnimation(IFunctionHandler* pH, bool enable)
{
    GET_ENTITY;

    if (ICharacterInstance* pCharacter = pEntity->GetCharacter(0))
    {
        pCharacter->EnableProceduralFacialAnimation(enable);
    }

    return pH->EndFunction();
}


int CScriptBind_Entity::PlayFacialAnimation(IFunctionHandler* pH, char* name, bool looping)
{
    GET_ENTITY;

    if (ICharacterInstance* pCharacter = pEntity->GetCharacter(0))
    {
        if (IFacialInstance* pInstance = pCharacter->GetFacialInstance())
        {
            if (IFacialAnimSequence* pSequence = pInstance->LoadSequence(name))
            {
                pInstance->PlaySequence(pSequence, eFacialSequenceLayer_FlowGraph, false, true);
            }
        }
    }

    return pH->EndFunction();
}


/*! Retrieves the position of a helper (placeholder) object
    @param helper Name of the helper object in the model
    @return Three component vector containing the position
*/
int CScriptBind_Entity::GetHelperPos(IFunctionHandler* pH)
{
    GET_ENTITY;
    //  SCRIPT_CHECK_PARAMETERS(1);
    //assert(pH->GetParamCount() == 1 || pH->GetParamCount() == 2 || pH->GetParamCount() == 3);

    const char* helper;
    bool bUseObjectSpace =  false;

    if (!pH->GetParam(1, helper))
    {
        return pH->EndFunction();
    }

    if (pH->GetParamCount() >= 2)
    {
        pH->GetParam(2, bUseObjectSpace);
    }

    Vec3 vPos(0, 0, 0);

    bool bDone = false;
    for (int i = 0; i < pEntity->GetSlotCount(); i++)
    {
        IStatObj* pObj = pEntity->GetStatObj(i);
        if (pObj)
        {
            if (pObj->GetGeoName() != NULL)
            {
                _smart_ptr<IStatObj> pRootObj = gEnv->p3DEngine->LoadStatObjAutoRef(pObj->GetFilePath());
                if (pRootObj)
                {
                    vPos = pRootObj->GetHelperPos(helper);
                }

                SEntitySlotInfo slotInfo;
                pEntity->GetSlotInfo(i, slotInfo);
                if (slotInfo.nParentSlot != -1)
                {
                    vPos = pEntity->GetSlotLocalTM(i, true) * vPos;
                    //vPos = pEntity->GetSlotLocalTM(slotInfo.nParentSlot, true) * vPos;
                }
            }
            else
            {
                vPos = pObj->GetHelperPos(helper);

                SEntitySlotInfo slotInfo;
                pEntity->GetSlotInfo(i, slotInfo);
                if (slotInfo.nParentSlot != -1)
                {
                    vPos = pEntity->GetSlotLocalTM(i, true) * vPos;
                    //vPos = pEntity->GetSlotLocalTM(slotInfo.nParentSlot, true) * vPos;
                }
            }

            if (!vPos.IsZero())
            {
                bDone = true;
                if (!bUseObjectSpace)
                {
                    vPos = pEntity->GetSlotWorldTM(i).TransformPoint(vPos);
                }
                break;
            }
        }
        ICharacterInstance* pCharacter = pEntity->GetCharacter(i);
        if (pCharacter)
        {
            int16 id = pCharacter->GetIDefaultSkeleton().GetJointIDByName(helper);
            if (id >= 0)
            {
                //vPos = pCharacter->GetISkeleton()->GetHelperPos(helper);
                vPos = pCharacter->GetISkeletonPose()->GetAbsJointByID(id).t;
                if (!vPos.IsZero())
                {
                    bDone = true;
                    if (!bUseObjectSpace)
                    {
                        vPos = pEntity->GetSlotWorldTM(i).TransformPoint(vPos);
                    }
                    break;
                }
            }
        }
    }

    if (!bDone && !bUseObjectSpace)
    {
        vPos = pEntity->GetWorldTM().TransformPoint(vPos);
    }

    return pH->EndFunction(Script::SetCachedVector(vPos, pH, 3));
}


/*! Retrieves the direction of a helper (placeholder) object
@param helper Name of the helper object in the model
@return Three component vector containing the forward direction
*/
int CScriptBind_Entity::GetHelperDir(IFunctionHandler* pH)
{
    GET_ENTITY;
    //  SCRIPT_CHECK_PARAMETERS(1);
    //assert(pH->GetParamCount() == 1 || pH->GetParamCount() == 2);

    const char* helper;
    bool bUseObjectSpace =  false;

    if (!pH->GetParam(1, helper))
    {
        return pH->EndFunction();
    }

    if (pH->GetParamCount() >= 2)
    {
        pH->GetParam(2, bUseObjectSpace);
    }

    Vec3 vDir(0, 0, 0);

    for (int i = 0; i < pEntity->GetSlotCount(); i++)
    {
        if (IStatObj* pObj = pEntity->GetStatObj(i))
        {
            const Matrix34& tm = pObj->GetHelperTM(helper);
            vDir = tm.TransformVector(FORWARD_DIRECTION);
            if (!bUseObjectSpace)
            {
                vDir = pEntity->GetSlotWorldTM(i).TransformVector(vDir);
            }
            break;
        }
        else if (ICharacterInstance* pCharacter = pEntity->GetCharacter(i))
        {
            int16 id = pCharacter->GetIDefaultSkeleton().GetJointIDByName(helper);
            if (id >= 0)
            {
                vDir = pCharacter->GetISkeletonPose()->GetAbsJointByID(id).q * FORWARD_DIRECTION;
            }

            if (!bUseObjectSpace)
            {
                vDir = pEntity->GetSlotWorldTM(i).TransformVector(vDir);
            }
            break;
        }
    }


    return pH->EndFunction(vDir);
}

/*! Retrieves the rotation angles of a helper (placeholder) object
@param helper Name of the helper object in the model
@return Three component vector containing the rotation angles
*/
int CScriptBind_Entity::GetHelperAngles(IFunctionHandler* pH)
{
    GET_ENTITY;
    //  SCRIPT_CHECK_PARAMETERS(1);
    //assert(pH->GetParamCount() == 1 || pH->GetParamCount() == 2);

    const char* helper;
    bool bUseObjectSpace =    false;

    if (!pH->GetParam(1, helper))
    {
        return pH->EndFunction();
    }

    if (pH->GetParamCount() >= 2)
    {
        pH->GetParam(2, bUseObjectSpace);
    }

    Ang3 vAngles(0, 0, 0);

    for (int i = 0; i < pEntity->GetSlotCount(); i++)
    {
        if (IStatObj* pObj = pEntity->GetStatObj(i))
        {
            const Matrix34& tm = bUseObjectSpace ? pObj->GetHelperTM(helper) : pEntity->GetSlotWorldTM(i);
            vAngles = Ang3::GetAnglesXYZ(Matrix33(tm));
        }
        else if (ICharacterInstance* pCharacter = pEntity->GetCharacter(i))
        {
            Quat rot;
            int16 id = pCharacter->GetIDefaultSkeleton().GetJointIDByName(helper);
            if (id >= 0)
            {
                rot = pCharacter->GetISkeletonPose()->GetAbsJointByID(id).q;
                if (!bUseObjectSpace)
                {
                    rot = rot * Quat(pEntity->GetSlotWorldTM(i));
                }
            }
            else
            {
                // not found
                rot = pEntity->GetRotation();
            }
            vAngles = Ang3::GetAnglesXYZ(rot);
        }
    }
    return pH->EndFunction(vAngles);
}


/*! Retrieves the position of a helper (placeholder) object
@param helper Name of the helper object in the model
@return Three component vector cotaining the position
*/
int CScriptBind_Entity::GetSlotHelperPos(IFunctionHandler* pH, int slot, const char* helperName, bool objectSpace)
{
    GET_ENTITY;

    IStatObj* pObject =  pEntity->GetStatObj(slot);
    ICharacterInstance* pCharacter = pEntity->GetCharacter(slot);

    Vec3 position(0, 0, 0);

    if (pObject)
    {
        if (pObject->GetGeoName())
        {
            _smart_ptr<IStatObj> pRootObj = gEnv->p3DEngine->LoadStatObjAutoRef(pObject->GetFilePath());

            if (pRootObj)
            {
                position = pRootObj->GetHelperPos(helperName);
            }
        }
        else
        {
            position = pObject->GetHelperPos(helperName);
        }

        if (!objectSpace)
        {
            position = pEntity->GetSlotWorldTM(slot).TransformPoint(position);
        }
        else
        {
            position = pEntity->GetSlotLocalTM(slot, false).TransformPoint(position);
        }
    }
    else if (pCharacter)
    {
        int16 id = pCharacter->GetIDefaultSkeleton().GetJointIDByName(helperName);
        if (id >= 0)
        {
            position = pCharacter->GetISkeletonPose()->GetAbsJointByID(id).t;
        }
        else
        {
            position.Set(0, 0, 0);
        }
        //position = pCharacter->GetISkeleton()->GetHelperPos(helperName);

        if (!objectSpace)
        {
            position = pEntity->GetSlotWorldTM(slot).TransformPoint(position);
        }
        else
        {
            position = pEntity->GetSlotLocalTM(slot, false).TransformPoint(position);
        }
    }

    return pH->EndFunction(Script::SetCachedVector(position, pH, 4));
}

int CScriptBind_Entity::RenderShadow(IFunctionHandler* pH)
{
    GET_ENTITY;

    bool bRender;
    if (!pH->GetParams(bRender))
    {
        return pH->EndFunction();
    }

    int IRenderNode = -1;
    if (pH->GetParamCount() > 1)
    {
        pH->GetParam(2, IRenderNode);
    }

    IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        struct IRenderNode* pRenderNode = pRenderComponent->GetRenderNode();
        if (pRenderNode)
        {
            if (IRenderNode == -1 || IRenderNode == 1)
            {
                pRenderNode->SetRndFlags(ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS, true);
            }
            else
            {
                pRenderNode->SetRndFlags(ERF_CASTSHADOWMAPS, false);
            }
        }
    }

    return pH->EndFunction();
}

int CScriptBind_Entity::SetRegisterInSectors(IFunctionHandler* pH)
{
    GET_ENTITY;
    SCRIPT_CHECK_PARAMETERS(1);

    bool bFlag;
    pH->GetParam(1, bFlag);

    if (pEntity)
    {
        //@TODO: Restore this.
        //(pEntity)->SetRegisterInSectors(bFlag);
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::AwakePhysics(IFunctionHandler* pH, int nAwake)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;

    IPhysicalEntity* pe = pEntity->GetPhysics();
    if (pe)
    {
        pe_action_awake aa;
        aa.bAwake = nAwake;
        pe->Action(&aa);
    }
#endif // ENABLE_CRY_PHYSICS
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::ResetPhysics(IFunctionHandler* pH)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;

    IPhysicalEntity* pe = pEntity->GetPhysics();
    if (pe)
    {
        pe_action_reset ra;
        pe->Action(&ra);
    }
#endif // ENABLE_CRY_PHYSICS

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::AwakeCharacterPhysics(IFunctionHandler* pH, int nSlot, const char* sRootBoneName, int nAwake)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;

    pe_action_awake aa;
    aa.bAwake = nAwake;

    IPhysicalEntity* pe;
    if (pEntity && pEntity->GetCharacter(nSlot) &&
        (pe = pEntity->GetCharacter(nSlot)->GetISkeletonPose()->GetCharacterPhysics(sRootBoneName)))
    {
        pe->Action(&aa);
    }
#endif // ENABLE_CRY_PHYSICS

    return pH->EndFunction();
}

int CScriptBind_Entity::SetCharacterPhysicParams(IFunctionHandler* pH)
{
    GET_ENTITY;

    int nSlot;
    const char* pRootBoneName;
#if ENABLE_CRY_PHYSICS
    int nType = PE_NONE;
#else
    int nType = 0;
#endif
    SmartScriptTable pTable;
    if (!pH->GetParams(nSlot, pRootBoneName, nType, pTable))
    {
        return pH->EndFunction();
    }

#if ENABLE_CRY_PHYSICS
    // need to find entity's physics
    IPhysicalEntity* pPhysicalEntity = nullptr;

    // is entity a character with physics?
    ICharacterInstance* pCharacter = pEntity->GetCharacter(nSlot);
    if (pCharacter)
    {
        pPhysicalEntity = pCharacter->GetISkeletonPose()->GetCharacterPhysics(pRootBoneName);
    }

    // is entity a tree with physics?
    if (!pPhysicalEntity)
    {
        if (IComponentPhysicsPtr pPhysicsComponent = pEntity->GetComponent<IComponentPhysics>())
        {
            IFoliage* pFoliage = pPhysicsComponent->GetFoliage(nSlot);

            // if foliage doesn't already exist, try to create it
            if (!pFoliage)
            {
                if (pPhysicsComponent->PhysicalizeFoliage(nSlot))
                {
                    pFoliage = pPhysicsComponent->GetFoliage(nSlot);
                }
            }

            if (pFoliage && pRootBoneName)
            {
                pPhysicalEntity = pFoliage->GetBranchPhysics(atol(pRootBoneName));
            }
        }
    }

    if (pPhysicalEntity)
    {
        // It's theoretically possible that a valid character is passed along with tree physics.
        // Not sure if this is desireable, but the logic matches an earlier version of the code.
        return pH->EndFunction(SetEntityPhysicParams(pH, pPhysicalEntity, nType, pTable, pCharacter));
    }
    else
#endif // ENABLE_CRY_PHYSICS
    {
        return pH->EndFunction();
    }
}

int CScriptBind_Entity::SetPhysicParams(IFunctionHandler* pH)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;

    int nType = PE_NONE;
    SmartScriptTable pTable;
    if (!pH->GetParams(nType, pTable))
    {
        return pH->EndFunction();
    }


    IPhysicalEntity* pe = pEntity->GetPhysics();
    if (!pe)
    {
        return pH->EndFunction();
    }

    return pH->EndFunction(SetEntityPhysicParams(pH, pe, nType, pTable));
#else
    return pH->EndFunction();
#endif // ENABLE_CRY_PHYSICS
}


int CScriptBind_Entity::SetEntityPhysicParams(IFunctionHandler* pH, IPhysicalEntity* pe, int nType, IScriptTable* pTable, ICharacterInstance* pIChar)
{
    SmartScriptTable pTempObj;
    Vec3 vec(0, 0, 0);
    int res = 1;
#if ENABLE_CRY_PHYSICS

    pe_params_particle particle_params;
    pe_simulation_params sim_params;
    pe_params_car vehicle_params;
    pe_params_wheel wheel_params;
    pe_player_dynamics playerdyn_params;
    pe_player_dimensions playerdim_params;
    pe_params_articulated_body artic_params;
    pe_params_joint joint_params;
    pe_params_rope rope_params, rope_params1;
    pe_params_softbody soft_params;
    pe_params_buoyancy buoy_params;
    pe_action_add_constraint constr_params;
    pe_action_update_constraint remove_constr_params;
    pe_params_flags flags_params, flags_params_old;
    pe_params_collision_class collision_class_params;
    pe_action_set_velocity asv;
    pe_params_part pp;
    pe_tetrlattice_params tlp;
    pe_params_ground_plane pgp;
    pe_params_foreign_data pfd;
    pe_action_target_vtx atv;
    pe_action_auto_part_detachment aapd;
    pe_params_timeout timeout_params;
    const char* strName;
    int idEnt;
    IEntity* pEnt;
    //  IJoint *pBone;
    int16 pBone_id;
    float gears[8];
    //float fDummy;
    Vec3 gravity(ZERO);
    float fSpeed;

    switch (nType)
    {
    case PHYSICPARAM_FOREIGNDATA:
    {
        bool dataFound = false;

        int foreignData;
        if (pTable->GetValue("foreignData", foreignData))
        {
            pfd.iForeignData = foreignData;
            dataFound = true;
        }

        int foreignFlags;
        if (pTable->GetValue("foreignFlags", foreignFlags))
        {
            pfd.iForeignFlagsOR = foreignFlags;
            dataFound = true;
        }

        if (dataFound)
        {
            pe->SetParams(&pfd);
        }
    }
    break;
    case PHYSICPARAM_FLAGS:
        if (pTable->GetValue("flags_mask", (int&)flags_params.flagsAND))
        {
            flags_params.flagsAND = ~flags_params.flagsAND;
            pTable->GetValue("flags", (int&)flags_params.flagsOR);
        }
        else
        {
            pTable->GetValue("flags", (int&)flags_params.flags);

            pe->GetParams(&flags_params_old);
            if (flags_params_old.flags & pef_log_collisions)
            {
                flags_params.flags |= pef_log_collisions;
            }
        }

        pe->SetParams(&flags_params);
        break;
    case PHYSICPARAM_COLLISION_CLASS:
        pTable->GetValue("collisionClass", (int&)collision_class_params.collisionClassOR.type);
        pTable->GetValue("collisionClassIgnore", (int&)collision_class_params.collisionClassOR.ignore);
        if (pTable->GetValue("collisionClassUNSET", (int&)collision_class_params.collisionClassAND.type))
        {
            collision_class_params.collisionClassAND.type = ~collision_class_params.collisionClassAND.type;
        }
        if (pTable->GetValue("collisionClassIgnoreUNSET", (int&)collision_class_params.collisionClassAND.ignore))
        {
            collision_class_params.collisionClassAND.ignore = ~collision_class_params.collisionClassAND.ignore;
        }
        pe->SetParams(&collision_class_params);
        break;
    case PHYSICPARAM_PART_FLAGS:
        if (pTable->GetValue("flags_mask", (int&)pp.flagsAND))
        {
            pp.flagsAND = ~pp.flagsAND;
        }
        pTable->GetValue("flags", (int&)pp.flagsOR);
        if (pTable->GetValue("flags_collider_mask", (int&)pp.flagsColliderAND))
        {
            pp.flagsColliderAND = ~pp.flagsColliderAND;
        }
        pTable->GetValue("flags_collider", (int&)pp.flagsColliderOR);
        pTable->GetValue("mat_breakable", pp.idmatBreakable);
        if (!pTable->GetValue("partid", pp.partid))
        {
            pe_status_nparts status_nparts;
            for (pp.ipart = pe->GetStatus(&status_nparts) - 1; pp.ipart >= 0; pp.ipart--)
            {
                pe->SetParams(&pp);
            }
        }
        else
        {
            pe->SetParams(&pp);
        }
        break;
    case PHYSICPARAM_PARTICLE:
    {
        pTable->GetValue("flags", (int&)particle_params.flags);
        pTable->GetValue("mass", particle_params.mass);
        pTable->GetValue("size", particle_params.size);
        if (pTable->GetValue("heading", vec) && fabsf(vec.GetLengthSquared() - 1.0f) < 0.01f)
        {
            particle_params.heading = vec;
        }
        pTable->GetValue("initial_velocity", particle_params.velocity);
        pTable->GetValue("k_air_resistance", particle_params.kAirResistance);
        pTable->GetValue("k_water_resistance", particle_params.kWaterResistance);
        pTable->GetValue("acc_thrust", particle_params.accThrust);
        pTable->GetValue("acc_lift", particle_params.accLift);
        pTable->GetValue("min_bounce_vel", particle_params.minBounceVel);
        pTable->GetValue("surface_idx", particle_params.surface_idx);
        pTable->GetValue("pierceability", particle_params.iPierceability);
        pTable->GetValue("normal", particle_params.normal);

        if (pTable->GetValue("w", vec))
        {
            particle_params.wspin = vec;
        }
        if (pTable->GetValue("gravity", vec))
        {
            particle_params.gravity = vec;
        }
        if (pTable->GetValue("water_gravity", vec))
        {
            particle_params.waterGravity = vec;
        }

        ScriptHandle ignore;
        if (pTable->GetValue("collider_to_ignore", ignore))
        {
            IEntity* pEntity = m_pEntitySystem->GetEntity((EntityId)ignore.n);
            if (pEntity)
            {
                IPhysicalEntity* pPE = pEntity->GetPhysics();
                if (pPE)
                {
                    particle_params.pColliderToIgnore = pPE;
                }
            }
        }
        bool bconstant_orientation = false;
        pTable->GetValue("constant_orientation", bconstant_orientation);
        if (bconstant_orientation)
        {
            particle_params.flags = particle_constant_orientation | particle_no_path_alignment | particle_no_roll | particle_traceable | particle_single_contact;
        }
        particle_params.flags |= pef_log_collisions;
        pe->SetParams(&particle_params);
        //          m_RocketParticlePar = particle_params;
    }
    break;
    case PHYSICPARAM_VEHICLE:
        pTable->GetValue("axle_friction", vehicle_params.axleFriction);
        pTable->GetValue("engine_power", vehicle_params.enginePower);
        pTable->GetValue("max_steer", vehicle_params.maxSteer);
        pTable->GetValue("engine_maxrpm", vehicle_params.engineMaxRPM);
        pTable->GetValue("engine_maxRPM", vehicle_params.engineMaxRPM);
        pTable->GetValue("intergration_type", vehicle_params.iIntegrationType);
        pTable->GetValue("max_time_step_vehicle", vehicle_params.maxTimeStep);
        if (pTable->GetValue("sleep_speed_vehicle", fSpeed))
        {
            vehicle_params.minEnergy = fSpeed * fSpeed;
        }
        pTable->GetValue("damping_vehicle", vehicle_params.damping);
        pTable->GetValue("max_braking_friction", vehicle_params.maxBrakingFriction);
        pTable->GetValue("engine_minRPM", vehicle_params.engineMinRPM);
        pTable->GetValue("engine_idleRPM", vehicle_params.engineIdleRPM);
        pTable->GetValue("engine_shiftupRPM", vehicle_params.engineShiftUpRPM);
        pTable->GetValue("engine_shiftdownRPM", vehicle_params.engineShiftDownRPM);
        pTable->GetValue("stabilizer", vehicle_params.kStabilizer);
        pTable->GetValue("clutch_speed", vehicle_params.clutchSpeed);
        if (pTable->GetValue("gears", pTempObj))
        {
            for (vehicle_params.nGears = 0; vehicle_params.nGears < sizeof(gears) / sizeof(gears[0]); vehicle_params.nGears++)
            {
                if (!pTempObj->GetAt(vehicle_params.nGears + 1, gears[vehicle_params.nGears]))
                {
                    break;
                }
            }
            vehicle_params.gearRatios = gears;
        }
        pTable->GetValue("brake_torque", vehicle_params.brakeTorque);
        pTable->GetValue("dyn_friction_ratio", vehicle_params.kDynFriction);
        pTable->GetValue("gear_dir_switch_RPM", vehicle_params.gearDirSwitchRPM);
        pTable->GetValue("slip_threshold", vehicle_params.slipThreshold);
        pTable->GetValue("engine_startRPM", vehicle_params.engineStartRPM);
        pTable->GetValue("minGear", vehicle_params.minGear);
        pTable->GetValue("maxGear", vehicle_params.maxGear);
        pe->SetParams(&vehicle_params);
        break;
    case PHYSICPARAM_WHEEL:
        pTable->GetValue("wheel", wheel_params.iWheel);
        pTable->GetValue("is_driving", wheel_params.bDriving);
        pTable->GetValue("susp_len", wheel_params.suspLenMax);
        pTable->GetValue("min_friction", wheel_params.minFriction);
        pTable->GetValue("max_friction", wheel_params.maxFriction);
        pTable->GetValue("surface_idx", wheel_params.surface_idx);
        pTable->GetValue("canBrake", wheel_params.bCanBrake);
        pe->SetParams(&wheel_params);
        break;
    case PHYSICPARAM_SIMULATION:
        pTable->GetValue("max_time_step", sim_params.maxTimeStep);
        if (pTable->GetValue("sleep_speed", fSpeed))
        {
            sim_params.minEnergy = fSpeed * fSpeed;
        }
        if (pTable->GetValue("gravity", vec))
        {
            sim_params.gravity = vec;
        }
        if (pTable->GetValue("gravityx", gravity.x) | pTable->GetValue("gravityy", gravity.y) | pTable->GetValue("gravityz", gravity.z))
        {
            sim_params.gravity = gravity;
        }
        if (pTable->GetValue("freefall_gravity", vec))
        {
            sim_params.gravityFreefall = vec;
        }

        gravity.Set(0, 0, 0);
        if (pTable->GetValue("freefall_gravityx", gravity.x) | pTable->GetValue("freefall_gravityy", gravity.y) |
            pTable->GetValue("freefall_gravityz", gravity.z))
        {
            sim_params.gravityFreefall = gravity;
        }
        pTable->GetValue("damping", sim_params.damping);
        pTable->GetValue("freefall_damping", sim_params.dampingFreefall);
        pTable->GetValue("mass", sim_params.mass);
        pTable->GetValue("density", sim_params.density);
        pTable->GetValue("min_energy", sim_params.minEnergy);
        pTable->GetValue("max_logged_collisions", sim_params.maxLoggedCollisions);
        //if (pTable->GetValue("water_density",fDummy))
        //  pEntity->SetWaterDensity(fDummy);

        pe->SetParams(&sim_params);

        {
            bool bFixedDamping = false;
            bool bUseSimpleSolver = false;
            pTable->GetValue("bFixedDamping", bFixedDamping);
            pTable->GetValue("bUseSimpleSolver", bUseSimpleSolver);
            if (bFixedDamping)
            {
                flags_params.flagsOR |= pef_fixed_damping;
            }
            else
            {
                flags_params.flagsAND &= ~pef_fixed_damping;
            }
            if (bUseSimpleSolver)
            {
                flags_params.flagsOR |= ref_use_simple_solver;
            }
            else
            {
                flags_params.flagsAND &= ~ref_use_simple_solver;
            }
        }
        pe->SetParams(&flags_params);
        break;
    case PHYSICPARAM_VELOCITY:
        if (pTable->GetValue("v", vec))
        {
            asv.v = vec;
        }
        if (pTable->GetValue("w", vec))
        {
            asv.w = vec;
        }
        pe->Action(&asv);
        break;
    case PHYSICPARAM_BUOYANCY:
        pTable->GetValue("water_density", buoy_params.waterDensity);
        pTable->GetValue("water_damping", buoy_params.waterDamping);
        pTable->GetValue("water_resistance", buoy_params.waterResistance);
        if (pTable->GetValue("water_sleep_speed", fSpeed))
        {
            buoy_params.waterEmin = fSpeed * fSpeed;
        }
        if (pTable->GetValue("water_normal", vec))
        {
            buoy_params.waterPlane.n = vec;
        }
        if (pTable->GetValue("water_origin", vec))
        {
            buoy_params.waterPlane.origin = vec;
        }
        pe->SetParams(&buoy_params);
        break;
    case PHYSICPARAM_ARTICULATED:
        pTable->GetValue("lying_mode_ncolls", artic_params.nCollLyingMode);
        if (pTable->GetValue("lying_gravity", vec))
        {
            artic_params.gravityLyingMode = vec;
        }
        if (pTable->GetValue("lying_gravityx", gravity.x) | pTable->GetValue("lying_gravityy", gravity.y) |
            pTable->GetValue("lying_gravityz", gravity.z))
        {
            artic_params.gravityLyingMode = gravity;
        }
        pTable->GetValue("lying_damping", artic_params.dampingLyingMode);
        if (pTable->GetValue("lying_sleep_speed", fSpeed))
        {
            artic_params.minEnergyLyingMode = fSpeed * fSpeed;
        }
        pTable->GetValue("is_grounded", artic_params.bGrounded);
        if (pTable->GetValue("check_collisions", artic_params.bCheckCollisions))
        {
            artic_params.bCollisionResp = artic_params.bCheckCollisions;
        }
        pTable->GetValue("sim_type", artic_params.iSimType);
        pTable->GetValue("lying_sim_type", artic_params.iSimTypeLyingMode);
        pe->SetParams(&artic_params);
        break;
    case PHYSICPARAM_JOINT:
        pTable->GetValue("bone_name", strName);
        pBone_id = pIChar ? pIChar->GetIDefaultSkeleton().GetJointIDByName(strName) : -1;
        if (pBone_id >= 0)
        {
            pTable->GetValue("flags", (int&)joint_params.flags);
            if (pTable->GetValue("min", vec))
            {
                joint_params.limits[0] = vec;
            }
            if (pTable->GetValue("max", vec))
            {
                joint_params.limits[1] = vec;
            }
            if (pTable->GetValue("stiffness", vec))
            {
                joint_params.ks = vec;
            }
            if (pTable->GetValue("damping", vec))
            {
                joint_params.kd = vec;
            }
            if (pTable->GetValue("dashpot", vec))
            {
                joint_params.qdashpot = vec;
            }
            if (pTable->GetValue("kdashpot", vec))
            {
                joint_params.kdashpot = vec;
            }
            pe->SetParams(&joint_params);
        }
        break;
    case PHYSICPARAM_ROPE:
        flags_params.flagsOR = 0;
        flags_params.flagsAND = ~0;
        if (pTable->GetValue("length", rope_params.length) && rope_params.length < 0)
        {
            pe_status_rope sr;
            sr.lock = 1;
            pe->GetStatus(&sr);
            float length = 0;
            sr.lock = -1;
            Vec3*& pvtx = sr.nVtx ? sr.pVtx : sr.pPoints;
            pvtx = new Vec3[max(sr.nSegments + 1, sr.nVtx)];
            pe->GetStatus(&sr);
            for (int i = 0; i < max(sr.nVtx - 1, sr.nSegments); i++)
            {
                length += (pvtx[i + 1] - pvtx[i]).len();
            }
            delete[] pvtx;
            pTable->SetValue("length", length);
            pTable->SetValue("strained", sr.bStrained != 0);
            MARK_UNUSED rope_params.length;
        }
        pTable->GetValue("mass", rope_params.mass);
        pTable->GetValue("coll_dist", rope_params.collDist);
        pTable->GetValue("surface_idx", rope_params.surface_idx);
        pTable->GetValue("friction", rope_params.friction);
        pTable->GetValue("friction_pull", rope_params.frictionPull);
        pTable->GetValue("wind_variance", rope_params.windVariance);
        pTable->GetValue("air_resistance", rope_params.airResistance);
        pTable->GetValue("sensor_size", rope_params.sensorRadius);
        if (pTable->GetValue("max_force", rope_params.maxForce))
        {
            if (rope_params.maxForce < 0)
            {
                flags_params.flagsOR |= rope_no_tears, rope_params.maxForce *= -1;
            }
            else
            {
                flags_params.flagsAND &= ~rope_no_tears;
            }
        }
        pTable->GetValue("num_segs", rope_params.nSegments);
        pTable->GetValue("num_subvtx", rope_params.nMaxSubVtx);
        pTable->GetValue("pose_stiffness", rope_params.stiffnessAnim);
        pTable->GetValue("pose_damping", rope_params.dampingAnim);
        pTable->GetValue("pose_type", rope_params.bTargetPoseActive);
        pTable->GetValue("unproj_limit", rope_params.unprojLimit);
        pTable->GetValue("stiffness", rope_params.stiffness);
        pTable->GetValue("hardness", rope_params.penaltyScale);
        if (pTable->GetValue("wind", vec))
        {
            rope_params.wind = vec;
        }

        pe->GetParams(&rope_params1);
        if (rope_params1.pEntTiedTo[0] == 0 || rope_params1.pEntTiedTo[1] == 0)
        {
            int iEnd = rope_params1.pEntTiedTo[1] == 0;
            if (pTable->GetValue("entity_phys_id", idEnt))
            {
                rope_params.pEntTiedTo[iEnd] = m_pISystem->GetIPhysicalWorld()->GetPhysicalEntityById(idEnt);
            }
            else if (pTable->GetValue("entity_name", strName))
            {
                if (!strcmp(strName, "#world#"))
                {
                    rope_params.pEntTiedTo[iEnd] = WORLD_ENTITY;
                }
                else if (*strName && (pEnt = m_pEntitySystem->FindEntityByName(strName)) && pEnt->GetPhysics())
                {
                    rope_params.pEntTiedTo[iEnd] = pEnt->GetPhysics();
                }
                else
                {
                    rope_params.pEntTiedTo[iEnd] = 0;
                }
            }
            pTable->GetValue("entity_part_id", rope_params.idPartTiedTo[iEnd]);
            if (pTable->GetValue("end", vec))
            {
                rope_params.ptTiedTo[iEnd] = vec;
            }
        }

        if (pTable->GetValue("entity_phys_id_2", idEnt))
        {
            rope_params.pEntTiedTo[1] = m_pISystem->GetIPhysicalWorld()->GetPhysicalEntityById(idEnt);
        }
        else if (pTable->GetValue("entity_name_2", strName))
        {
            if (!strcmp(strName, "#world#"))
            {
                rope_params.pEntTiedTo[1] = WORLD_ENTITY;
            }
            else if (*strName && (pEnt = m_pEntitySystem->FindEntityByName(strName)) && pEnt->GetPhysics())
            {
                rope_params.pEntTiedTo[1] = pEnt->GetPhysics();
            }
            else
            {
                rope_params.pEntTiedTo[1] = 0;
            }
        }
        pTable->GetValue("entity_part_id_2", rope_params.idPartTiedTo[1]);
        if (pTable->GetValue("end2", vec))
        {
            rope_params.ptTiedTo[1] = vec;
        }

        if (pTable->GetValue("entity_phys_id_1", idEnt))
        {
            rope_params.pEntTiedTo[0] = m_pISystem->GetIPhysicalWorld()->GetPhysicalEntityById(idEnt);
        }
        else if (pTable->GetValue("entity_name_1", strName))
        {
            if (!strcmp(strName, "#world#"))
            {
                rope_params.pEntTiedTo[0] = WORLD_ENTITY;
            }
            else if (*strName && (pEnt = m_pEntitySystem->FindEntityByName(strName)) && pEnt->GetPhysics())
            {
                rope_params.pEntTiedTo[0] = pEnt->GetPhysics();
            }
            else
            {
                rope_params.pEntTiedTo[0] = 0;
            }
        }
        pTable->GetValue("entity_part_id_1", rope_params.idPartTiedTo[0]);
        if (pTable->GetValue("end1", vec))
        {
            rope_params.ptTiedTo[0] = vec;
        }

        pe->SetParams(&rope_params);
        if (!is_unused(rope_params.bTargetPoseActive) && rope_params.bTargetPoseActive > 0)
        {
            pe->Action(&atv);
        }

        if (pTable->GetValue("timeout", timeout_params.maxTimeIdle))
        {
            pe->SetParams(&timeout_params);
        }

        if (pTable->GetValue("check_collisions", idEnt))
        {
            if (idEnt)
            {
                flags_params.flagsOR = rope_collides;
            }
            else
            {
                flags_params.flagsAND = ~rope_collides;
            }
        }
        if (pTable->GetValue("bCheckCollisions", idEnt))
        {
            if (idEnt)
            {
                flags_params.flagsOR = rope_collides;
            }
            else
            {
                flags_params.flagsAND = ~rope_collides;
            }
        }
        if (pTable->GetValue("bCheckTerrainCollisions", idEnt))
        {
            if (idEnt)
            {
                flags_params.flagsOR |= rope_collides_with_terrain;
            }
            else
            {
                flags_params.flagsAND &= ~rope_collides_with_terrain;
            }
        }
        if (pTable->GetValue("bCheckAttachmentCollisions", idEnt))
        {
            if (idEnt)
            {
                flags_params.flagsAND &= ~rope_ignore_attachments;
            }
            else
            {
                flags_params.flagsOR |= rope_ignore_attachments;
            }
        }
        if (pTable->GetValue("shootable", idEnt))
        {
            if (idEnt)
            {
                flags_params.flagsOR |= rope_traceable;
            }
            else
            {
                flags_params.flagsAND &= ~rope_traceable;
            }
        }
        if (pTable->GetValue("bShootable", idEnt))
        {
            if (idEnt)
            {
                flags_params.flagsOR |= rope_traceable;
            }
            else
            {
                flags_params.flagsAND &= ~rope_traceable;
            }
        }
        if (pTable->GetValue("bDisabled", idEnt))
        {
            if (idEnt)
            {
                flags_params.flagsOR |= pef_disabled;
            }
            else
            {
                flags_params.flagsAND &= ~pef_disabled;
            }
        }
        pe->SetParams(&flags_params);

        if (pTable->GetValue("break_point", fSpeed))
        {
            Vec3 pt[3];
            pe_action_slice as;
            as.pt = pt;
            as.npt = 1;
            pt[0].x = fSpeed;
            pe->Action(&as);
        }

        break;
    case PHYSICPARAM_SOFTBODY:
        pTable->GetValue("thickness", soft_params.thickness);
        pTable->GetValue("max_safe_step", soft_params.maxSafeStep);
        pTable->GetValue("hardness", soft_params.ks);
        pTable->GetValue("damping_ratio", soft_params.kdRatio);
        pTable->GetValue("air_resistance", soft_params.airResistance);
        if (pTable->GetValue("wind", vec))
        {
            soft_params.wind = vec;
        }
        pTable->GetValue("wind_variance", soft_params.windVariance);
        pTable->GetValue("max_iters", soft_params.nMaxIters);
        pTable->GetValue("accuracy", soft_params.accuracy);
        pTable->GetValue("friction", soft_params.friction);
        pTable->GetValue("impulse_scale", soft_params.impulseScale);
        pTable->GetValue("explosion_scale", soft_params.explosionScale);
        pTable->GetValue("collision_impulse_scale", soft_params.collisionImpulseScale);
        pTable->GetValue("max_collision_impulse", soft_params.maxCollisionImpulse);
        pTable->GetValue("collision_mask", soft_params.collTypes);
        pTable->GetValue("mass_decay", soft_params.massDecay);
        pTable->GetValue("stiffness_norm", soft_params.shapeStiffnessNorm);
        pTable->GetValue("stiffness_tang", soft_params.shapeStiffnessTang);
        pe->SetParams(&soft_params);
        break;
    case PHYSICPARAM_CONSTRAINT:
        if (!pTable->GetValue("phys_entity_id", idEnt) || !(constr_params.pBuddy = m_pISystem->GetIPhysicalWorld()->GetPhysicalEntityById(idEnt)))
        {
            constr_params.pBuddy = WORLD_ENTITY;
        }
        pTable->GetValue("entity_part_id_1", constr_params.partid[0]);
        pTable->GetValue("entity_part_id_2", constr_params.partid[1]);
        if (pTable->GetValue("pivot", vec))
        {
            constr_params.pt[0] = vec;     // the pivot is basically the position vector of the constraint entity
        }
        // frame0 and frame1 are set to the same value in the LUA script,
        // i.e. the constraint's frame or the first body's frame if bUseEntityFrame is set
        if (pTable->GetValue("frame0", vec))
        {
            constr_params.qframe[0] = Quat::CreateRotationXYZ(Ang3(vec));
            if (pTable->GetValue("frame0_inner", vec))
            {
                constr_params.qframe[0] = constr_params.qframe[0] * Quat::CreateRotationXYZ(DEG2RAD(Ang3(vec)));
            }
        }
        if (pTable->GetValue("frame1", vec))
        {
            constr_params.qframe[1] = Quat::CreateRotationXYZ(Ang3(vec));
            if (pTable->GetValue("frame1_inner", vec))
            {
                constr_params.qframe[1] = constr_params.qframe[1] * Quat::CreateRotationXYZ(DEG2RAD(Ang3(vec)));
            }
        }
        constr_params.flags = world_frames;
        if (pTable->GetValue("ignore_buddy", idEnt) && idEnt)
        {
            constr_params.flags |= constraint_ignore_buddy;
        }
        if (pTable->GetValue("xmin", constr_params.xlimits[0]))
        {
            constr_params.xlimits[0] = DEG2RAD(constr_params.xlimits[0]);
        }
        if (pTable->GetValue("xmax", constr_params.xlimits[1]))
        {
            constr_params.xlimits[1] = DEG2RAD(constr_params.xlimits[1]);
        }
        if (pTable->GetValue("yzmin", constr_params.yzlimits[0]))
        {
            constr_params.yzlimits[0] = DEG2RAD(constr_params.yzlimits[0]);
        }
        if (pTable->GetValue("yzmax", constr_params.yzlimits[1]))
        {
            constr_params.yzlimits[1] = DEG2RAD(constr_params.yzlimits[1]);
        }
        pTable->GetValue("damping", constr_params.damping);
        pTable->GetValue("sensor_size", constr_params.sensorRadius);
        if (pTable->GetValue("max_pull_force", constr_params.maxPullForce) && constr_params.maxPullForce < 0)
        {
            constr_params.maxPullForce *= -1.0f, constr_params.flags |= constraint_no_tears;
        }
        if (pTable->GetValue("max_bend_torque", constr_params.maxBendTorque) && constr_params.maxBendTorque < 0)
        {
            constr_params.maxBendTorque *= -1.0f, constr_params.flags |= constraint_no_tears;
        }
        if (pTable->GetValue("line", idEnt) && idEnt)
        {
            constr_params.flags |= constraint_line;
        }
        if (pTable->GetValue("plane", idEnt) && idEnt)
        {
            constr_params.flags |= constraint_plane;
        }
        if (pTable->GetValue("no_rotation", idEnt) && idEnt)
        {
            constr_params.flags |= constraint_no_rotation;
        }
        if (pTable->GetValue("full", idEnt) && idEnt == 0 && !(constr_params.flags & (constraint_line | constraint_plane)))
        {
            constr_params.flags |= constraint_free_position;
        }
        res = pe->Action(&constr_params);
        break;
    case PHYSICPARAM_REMOVE_CONSTRAINT:
        pTable->GetValue("id", remove_constr_params.idConstraint);
        remove_constr_params.bRemove = 1;
        pe->Action(&remove_constr_params);
        break;
    case PHYSICPARAM_PLAYERDYN:
        pTable->GetValue("k_inertia", playerdyn_params.kInertia);
        pTable->GetValue("k_air_control", playerdyn_params.kAirControl);
        pTable->GetValue("gravity", playerdyn_params.gravity.z);
        pTable->GetValue("bSwimming", playerdyn_params.bSwimming);
        pTable->GetValue("mass", playerdyn_params.mass);
        pTable->GetValue("surface_idx", playerdyn_params.surface_idx);
        pTable->GetValue("is_active", playerdyn_params.bActive);
        pTable->GetValue("max_vel_ground", playerdyn_params.maxVelGround);
        pe->SetParams(&playerdyn_params);
        break;
    case PHYSICPARAM_PLAYERDIM:
        pTable->GetValue("pivot_height", playerdim_params.heightPivot);
        pTable->GetValue("eye_height", playerdim_params.heightEye);
        pTable->GetValue("cyl_r", playerdim_params.sizeCollider.x);
        if (!pTable->GetValue("cyl_height", playerdim_params.sizeCollider.z))
        {
            playerdim_params.sizeCollider.z = playerdim_params.sizeCollider.x;
        }
        playerdim_params.sizeCollider.y = playerdim_params.sizeCollider.x;
        pTable->GetValue("cyl_pos", playerdim_params.heightCollider);
        pe->SetParams(&playerdim_params);
        break;
    case PHYSICPARAM_SUPPORT_LATTICE:
        pTable->GetValue("max_simultaneous_cracks", tlp.nMaxCracks);
        pTable->GetValue("max_push_force", tlp.maxForcePush);
        pTable->GetValue("max_pull_force", tlp.maxForcePull);
        pTable->GetValue("max_shift_force", tlp.maxForceShift);
        pTable->GetValue("max_twist_torque", tlp.maxTorqueTwist);
        pTable->GetValue("max_bend_torque", tlp.maxTorqueBend);
        pTable->GetValue("crack_weaken", tlp.crackWeaken);
        pTable->GetValue("density", tlp.density);
        for (pp.ipart = 0; pe->GetParams(&pp); pp.ipart++, MARK_UNUSED pp.partid)
        {
            if (pp.pLattice)
            {
                pp.pLattice->SetParams(&tlp);
            }
        }
        break;
    case PHYSICPARAM_GROUND_PLANE:
        pTable->GetValue("plane_index", pgp.iPlane);
        if (pTable->GetValue("origin", vec))
        {
            pgp.ground.origin = vec;
        }
        if (pTable->GetValue("normal", vec))
        {
            pgp.ground.n = vec;
        }
        pe->SetParams(&pgp);
        break;
    case PHYSICPARAM_AUTO_DETACHMENT:
        pTable->GetValue("threshold", aapd.threshold);
        pTable->GetValue("detach_distance", aapd.autoDetachmentDist);
        pe->Action(&aapd);
        break;
    }
#endif // ENABLE_CRY_PHYSICS

    return res;
}

int CScriptBind_Entity::IsColliding(IFunctionHandler* pH)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;

    IPhysicalEntity* pParticle = pEntity->GetPhysics();

    if (pParticle)
    {
        pe_status_collisions    sc;

        if (pParticle->GetStatus(&sc))
        {
            return pH->EndFunction(true);
            /*
            IPhysicalEntity *pPhysCollider = m_pISystem->GetIPhysicalWorld()->GetPhysicalEntityById(hit.idCollider);
            IEntity                 *pCollider = 0;

            if (pPhysCollider)
            {
                pCollider = m_pEntitySystem->GetEntityFromPhysics(pPhysCollider);
            }

            SmartScriptTable hitTbl(m_pSS);

            hitTbl->BeginSetGetChain();
            hitTbl->SetValueChain("normal", hit.n);
            hitTbl->SetValueChain("pos", hit.pt);
            hitTbl->SetValueChain("dir", hit.v[0].GetNormalized());
            hitTbl->SetValueChain("partid", hit.partid);

            if (pCollider)
            {
                ScriptHandle sh;
                sh.n = pCollider->GetId();

                hitTbl->SetValueChain("target_type", (int)pPhysCollider->GetType());
                hitTbl->SetValueChain("target_id", sh);

                if (pCollider->GetScriptTable())
                {
                    hitTbl->SetValueChain("target", pCollider->GetScriptTable());
                }
            }

            ISurfaceTypeManager *pSurfaceTypeManager = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
            ISurfaceType *pSurfaceType = pSurfaceTypeManager->GetSurfaceType(hit.idmat[1]);

            if (pSurfaceType && pSurfaceType->GetScriptTable())
            {
                hitTbl->SetValueChain("material", pSurfaceType->GetScriptTable());
            }

            hitTbl->EndSetGetChain();

            return pH->EndFunction(hitTbl);
            */
        }
    }
#endif // ENABLE_CRY_PHYSICS

    return pH->EndFunction(false);
}

/*! Retrieves if a characters currently plays an animation
    @param iAnimationPos Number of the character slot
    @return nil or not nil
    @see CScriptBind_Entity::StartAnimation
*/
int CScriptBind_Entity::IsAnimationRunning(IFunctionHandler* pH, int characterSlot, int layer)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);
    ;
    if (pCharacter)
    {
        //if there are problems with this function, ask Ivo
        uint32 numAnims = pCharacter->GetISkeletonAnim()->GetNumAnimsInFIFO(layer);
        if (numAnims)
        {
            return pH->EndFunction(true);
        }
    }

    return pH->EndFunction(false);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::AddImpulse(IFunctionHandler* pH)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;

    //return pH->EndFunction();
    //  float hitImpulse = m_pGame->p_HitImpulse->GetFVal();
    //  if(hitImpulse == 0)
    //      return pH->EndFunction();

    if (pH->GetParamCount() > 3)
    {
        int ipart;
        pH->GetParam(1, ipart);

        Vec3 pos(ZERO);
        bool bPos = (pH->GetParamType(2) == svtObject) && pH->GetParam(2, pos) && (pos.GetLengthSquared() > 0.f);

        Vec3 dir(0.f, 0.f, 1.f);
        pH->GetParam(3, dir);

        float impulse;
        pH->GetParam(4, impulse);

        float impulseScale = 1.f;
        if (pH->GetParamCount() > 4)
        {
            pH->GetParam(5, impulseScale);
        }

        bool angularImpulse(pH->GetParamCount() > 6);

        if (!angularImpulse && dir.GetLengthSquared() > 0)
        {
            IComponentPhysicsPtr pPhysicsComponent = pEntity->GetComponent<IComponentPhysics>();
            if (pPhysicsComponent)
            {
                pPhysicsComponent->AddImpulse(ipart, pos, dir.GetNormalized() * impulse, bPos, impulseScale);
            }
        }

        Vec3 angAxis;
        if (angularImpulse && pH->GetParam(6, angAxis))
        {
            float angImpulse;
            if (pH->GetParam(7, angImpulse))
            {
                IPhysicalEntity* ppEnt = pEntity->GetPhysics();
                if (ppEnt)
                {
                    pe_action_impulse aimpulse;

                    float massScale = 1.0f;
                    if (pH->GetParamCount() > 7 && pH->GetParam(8, massScale))
                    {
                        pe_params_part pp_part;
                        pp_part.partid = ipart;
                        if (ppEnt->GetParams(&pp_part))
                        {
                            if (massScale == 0)
                            {
                                massScale = 1.0f;
                            }
                            massScale = pp_part.mass / massScale;
                            if (massScale < 1.0f)
                            {
                                massScale = massScale * massScale;
                            }
                        }
                    }

                    //aimpulse.iApplyTime = 0;
                    aimpulse.partid = ipart;
                    aimpulse.impulse = dir.GetNormalized() * impulse * massScale;
                    aimpulse.angImpulse = angAxis.GetNormalized() * angImpulse * massScale;
                    if (bPos)
                    {
                        aimpulse.point = pos;
                    }

                    /*CryLogAlways(
                        "impulse:%.1f,%.1f,%.1f ang:%.1f,%.1f,%.1f part:%i",
                        aimpulse.impulse.x,aimpulse.impulse.y,aimpulse.impulse.z,
                        aimpulse.angImpulse.x,aimpulse.angImpulse.y,aimpulse.angImpulse.z,
                        aimpulse.partid);*/

                    ppEnt->Action(&aimpulse);
                }
            }
        }
    }
#endif // ENABLE_CRY_PHYSICS

    return pH->EndFunction();
}


int CScriptBind_Entity::AddConstraint(IFunctionHandler* pH)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;

    SmartScriptTable params(m_pSS, true);

    if (!pH->GetParams(params))
    {
        return pH->EndFunction();
    }

    pe_action_add_constraint constraint;
    IEntity* pBuddy = 0;

    {
        CScriptSetGetChain chain(params);
        ScriptHandle sh(0);
        Vec3 frame1v0, frame1v1, frame2v0, frame2v1;

        chain.GetValue("id", (int&)constraint.id);
        chain.GetValue("partid1", constraint.partid[0]);
        chain.GetValue("partid2", constraint.partid[1]);
        chain.GetValue("point1", constraint.pt[0]);
        chain.GetValue("point2", constraint.pt[1]);
        chain.GetValue("entityId", sh);
        chain.GetValue("xmin", constraint.xlimits[0]);
        chain.GetValue("xmax", constraint.xlimits[1]);
        chain.GetValue("yzmin", constraint.yzlimits[0]);
        chain.GetValue("yzmax", constraint.yzlimits[1]);
        chain.GetValue("flags", (int&)constraint.flags);
        chain.GetValue("frame1v0", frame1v0);
        chain.GetValue("frame1v1", frame1v1);
        chain.GetValue("frame2v0", frame2v0);
        chain.GetValue("frame2v1", frame2v1);

        constraint.qframe[0].SetRotationV0V1(frame1v0, frame1v1);
        constraint.qframe[1].SetRotationV0V1(frame2v0, frame2v1);

        pBuddy = m_pEntitySystem->GetEntity((EntityId)sh.n);
    }

    if (pBuddy)
    {
        constraint.pBuddy = pBuddy->GetPhysics();
    }

    if (!constraint.pBuddy)
    {
        return pH->EndFunction();
    }

    IPhysicalEntity* pSelf = pEntity->GetPhysics();
    pSelf->Action(&constraint);
#endif // ENABLE_CRY_PHYSICS

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
void CScriptBind_Entity::OnAudioTriggerFinishedEvent(const Audio::SAudioRequestInfo* const pAudioRequestInfo)
{
#if defined(INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE)
    if (gEnv->mMainThreadId != CryGetCurrentThreadId())
    {
        CryFatalError("CScriptBind_Entity::OnAudioTriggerFinishedEvent was not called from main thread but instead from: %llu", static_cast<AZ::u64>(CryGetCurrentThreadId()));
    }
#endif // INCLUDE_ENTITYSYSTEM_PRODUCTION_CODE

    const EntityId nEntityID = static_cast<EntityId>(reinterpret_cast<UINT_PTR>(pAudioRequestInfo->pUserData));
    IEntity* const pEntity = gEnv->pEntitySystem->GetEntity(nEntityID);

    if (pEntity)
    {
        SEntityEvent event;
        event.event = ENTITY_EVENT_SOUND_DONE;
        event.nParam[0] = pAudioRequestInfo->nAudioObjectID;
        event.nParam[1] = pAudioRequestInfo->nAudioControlID;
        pEntity->SendEvent(event);
    }
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetAllAuxAudioProxiesID(IFunctionHandler* pH)
{
    GET_ENTITY;

    IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();

    if (pAudioComponent)
    {
        return pH->EndFunction(IntToHandle(INVALID_AUDIO_PROXY_ID));
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetDefaultAuxAudioProxyID(IFunctionHandler* pH)
{
    GET_ENTITY;

    IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();

    if (pAudioComponent)
    {
        return pH->EndFunction(IntToHandle(DEFAULT_AUDIO_PROXY_ID));
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::CreateAuxAudioProxy(IFunctionHandler* pH)
{
    GET_ENTITY;

    IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();

    if (pAudioComponent)
    {
        return pH->EndFunction(IntToHandle(pAudioComponent->CreateAuxAudioProxy()));
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::RemoveAuxAudioProxy(IFunctionHandler* pH, const ScriptHandle hAudioProxyLocalID)
{
    GET_ENTITY;

    IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();

    if (pAudioComponent)
    {
        pAudioComponent->RemoveAuxAudioProxy(HandleToInt<Audio::TAudioProxyID>(hAudioProxyLocalID));
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::ExecuteAudioTrigger(IFunctionHandler* pH, const ScriptHandle hTriggerID, const ScriptHandle hAudioProxyLocalID)
{
    GET_ENTITY;

    IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();

    if (pAudioComponent)
    {
        // This is an optimizations as AddRequestListener is a blocking request.
        if (!m_bIsAudioEventListener)
        {
            Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::AddRequestListener,
                &CScriptBind_Entity::OnAudioTriggerFinishedEvent,
                this,
                Audio::eART_AUDIO_CALLBACK_MANAGER_REQUEST,
                Audio::eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE);
            m_bIsAudioEventListener = true;
        }

        const Audio::SAudioCallBackInfos callbackInfos(this, reinterpret_cast<void*>(static_cast<AZ::u64>(pEntity->GetId())), this, Audio::eARF_PRIORITY_NORMAL | Audio::eARF_SYNC_FINISHED_CALLBACK);
        pAudioComponent->ExecuteTrigger(HandleToInt<Audio::TAudioControlID>(hTriggerID), eLSM_None, HandleToInt<Audio::TAudioProxyID>(hAudioProxyLocalID), callbackInfos);
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::StopAudioTrigger(IFunctionHandler* pH, const ScriptHandle hTriggerID, const ScriptHandle hAudioProxyLocalID)
{
    GET_ENTITY;

    IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();

    if (pAudioComponent)
    {
        pAudioComponent->StopTrigger(HandleToInt<Audio::TAudioControlID>(hTriggerID), HandleToInt<Audio::TAudioProxyID>(hAudioProxyLocalID));
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetAudioSwitchState(IFunctionHandler* pH, const ScriptHandle hSwitchID, const ScriptHandle hSwitchStateID, const ScriptHandle hAudioProxyLocalID)
{
    GET_ENTITY;

    IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();

    if (pAudioComponent)
    {
        pAudioComponent->SetSwitchState(
            HandleToInt<Audio::TAudioControlID>(hSwitchID),
            HandleToInt<Audio::TAudioSwitchStateID>(hSwitchStateID),
            HandleToInt<Audio::TAudioProxyID>(hAudioProxyLocalID));
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetAudioObstructionCalcType(IFunctionHandler* pH, const int nObstructionCalcType, const ScriptHandle hAudioProxyLocalID)
{
    GET_ENTITY;

    IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();

    if (pAudioComponent)
    {
        switch (nObstructionCalcType)
        {
            case Audio::eAOOCT_IGNORE:
            case Audio::eAOOCT_SINGLE_RAY:
            case Audio::eAOOCT_MULTI_RAY:
            {
                pAudioComponent->SetObstructionCalcType(static_cast<Audio::EAudioObjectObstructionCalcType>(nObstructionCalcType), HandleToInt<Audio::TAudioProxyID>(hAudioProxyLocalID));
                break;
            }
            default:
            {
                pAudioComponent->SetObstructionCalcType(Audio::eAOOCT_IGNORE, HandleToInt<Audio::TAudioProxyID>(hAudioProxyLocalID));
                break;
            }
        }
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetFadeDistance(IFunctionHandler* pH, const float fFadeDistance)
{
    GET_ENTITY;

    IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();

    if (pAudioComponent)
    {
        pAudioComponent->SetFadeDistance(fFadeDistance);
        m_pEntitySystem->GetAreaManager()->SetAreasDirty();
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetAudioProxyOffset(IFunctionHandler* pH, const Vec3 vOffset, const ScriptHandle hAudioProxyLocalID)
{
    GET_ENTITY;

    IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();

    if (pAudioComponent)
    {
        pAudioComponent->SetAuxAudioProxyOffset(Audio::SATLWorldPosition(LYVec3ToAZVec3(vOffset)), HandleToInt<Audio::TAudioProxyID>(hAudioProxyLocalID));
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetEnvironmentFadeDistance(IFunctionHandler* pH, const float fEnvironmentFadeDistance)
{
    GET_ENTITY;

    IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();

    if (pAudioComponent)
    {
        pAudioComponent->SetEnvironmentFadeDistance(fEnvironmentFadeDistance);
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetAudioEnvironmentID(IFunctionHandler* pH, const ScriptHandle hAudioEnvironmentID)
{
    GET_ENTITY;

    IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();

    if (pAudioComponent)
    {
        const Audio::TAudioEnvironmentID nAudioEnvironmentIDToSet = HandleToInt<Audio::TAudioEnvironmentID>(hAudioEnvironmentID);
        const Audio::TAudioEnvironmentID nAudioEnvironmentIDToUnset = pAudioComponent->GetEnvironmentID();

        pAudioComponent->SetEnvironmentID(nAudioEnvironmentIDToSet);

        int nFlag = 0;
        nFlag |= (nAudioEnvironmentIDToSet == INVALID_AUDIO_ENVIRONMENT_ID && nAudioEnvironmentIDToUnset != INVALID_AUDIO_ENVIRONMENT_ID) << 0;
        nFlag |= (nAudioEnvironmentIDToSet != INVALID_AUDIO_ENVIRONMENT_ID && nAudioEnvironmentIDToUnset == INVALID_AUDIO_ENVIRONMENT_ID) << 1;
        nFlag |= (nAudioEnvironmentIDToSet != INVALID_AUDIO_ENVIRONMENT_ID && nAudioEnvironmentIDToUnset != INVALID_AUDIO_ENVIRONMENT_ID) << 2;

        // The audio environment is being tampered with, we need to inform all entities affected by the area.
        TAreaPointers apAreas;

        if (nFlag > 0 && static_cast<CAreaManager*>(m_pEntitySystem->GetAreaManager())->GetLinkedAreas(pEntity->GetId(), -1, apAreas))
        {
            TAreaPointers::const_iterator IterAreas(apAreas.begin());
            TAreaPointers::const_iterator const IterAreasEnd(apAreas.end());

            for (; IterAreas != IterAreasEnd; ++IterAreas)
            {
                CArea* const pArea = *IterAreas;
                SEntityProximityQuery oQuery;
                oQuery.nEntityFlags = 0;
                oQuery.pEntityClass = nullptr;
                Vec3 oMin;
                Vec3 oMax;
                pArea->GetWorldBox(oMin, oMax);
                const float fFadeDistance = pArea->GetGreatestFadeDistance();
                oQuery.box.min = oMin - Vec3(fFadeDistance, fFadeDistance, fFadeDistance);
                oQuery.box.max = oMax + Vec3(fFadeDistance, fFadeDistance, fFadeDistance);
                gEnv->pEntitySystem->QueryProximity(oQuery);

                for (int i = 0; i < oQuery.nCount; ++i)
                {
                    IEntity* const pIEntity = oQuery.pEntities[i];

                    if (pIEntity)
                    {
                        const IComponentAudioPtr pAudioComponentToAdjust = pIEntity->GetComponent<IComponentAudio>();

                        if (pAudioComponentToAdjust)
                        {
                            switch (nFlag)
                            {
                            case BIT(0):
                            {
                                pAudioComponentToAdjust->SetEnvironmentAmount(nAudioEnvironmentIDToUnset, 0.0f, INVALID_AUDIO_PROXY_ID);
                                break;
                            }
                            case BIT(1):
                            {
                                pAudioComponentToAdjust->SetCurrentEnvironments(INVALID_AUDIO_PROXY_ID);
                                break;
                            }
                            case BIT(2):
                            {
                                pAudioComponentToAdjust->SetEnvironmentAmount(nAudioEnvironmentIDToUnset, 0.0f, INVALID_AUDIO_PROXY_ID);
                                pAudioComponentToAdjust->SetCurrentEnvironments(INVALID_AUDIO_PROXY_ID);
                                break;
                            }
                            default:
                            {
                                assert(false);
                                break;
                            }
                            }
                        }
                    }
                }
            }
        }
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetCurrentAudioEnvironments(IFunctionHandler* pH)
{
    GET_ENTITY;

    IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();

    if (pAudioComponent)
    {
        // Passing INVALID_AUDIO_PROXY_ID to address all auxiliary AudioProxies on pAudioComponent.
        pAudioComponent->SetCurrentEnvironments(INVALID_AUDIO_PROXY_ID);
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetAudioRtpcValue(IFunctionHandler* pH, const ScriptHandle hRtpcID, const float fValue, const ScriptHandle hAudioProxyLocalID)
{
    GET_ENTITY;

    IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();

    if (pAudioComponent)
    {
        pAudioComponent->SetRtpcValue(HandleToInt<Audio::TAudioControlID>(hRtpcID), fValue, HandleToInt<Audio::TAudioProxyID>(hAudioProxyLocalID));
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::ResetAudioRtpcValues(IFunctionHandler* pH, const ScriptHandle hAudioProxyLocalID)
{
    GET_ENTITY;

    IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();

    if (pAudioComponent)
    {
        pAudioComponent->ResetRtpcValues(HandleToInt<Audio::TAudioProxyID>(hAudioProxyLocalID));
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::AuxAudioProxiesMoveWithEntity(IFunctionHandler* pH, const bool bCanMoveWithEntity)
{
    GET_ENTITY;

    IComponentAudioPtr pAudioComponent = pEntity->GetOrCreateComponent<IComponentAudio>();

    if (pAudioComponent)
    {
        pAudioComponent->AuxAudioProxiesMoveWithEntity(bCanMoveWithEntity);
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::TriggerEvent(IFunctionHandler* pH)
{
    GET_ENTITY;

    int eventType;

    pH->GetParam(1, eventType);
    SAIEVENT eventParams;

    switch (eventType)
    {
    case AIEVENT_AGENTDIED:
    {
        eventParams.targetId = 0;

        if (pH->GetParamCount() > 1)
        {
            ScriptHandle hdl;
            pH->GetParam(2, hdl);
            eventParams.targetId = (EntityId)hdl.n;
        }
    }
    break;
    case AIEVENT_DROPBEACON:
    {
        IPuppet* pPuppet = CastToIPuppetSafe(pEntity->GetAI());
        if (pPuppet)
        {
            pPuppet->UpdateBeacon();
        }
        break;
    }
    case AIEVENT_SLEEP:
    case AIEVENT_WAKEUP:
    case AIEVENT_ENABLE:
    case AIEVENT_DISABLE:
    case AIEVENT_PATHFINDON:
    case AIEVENT_PATHFINDOFF:
    case AIEVENT_CLEAR:
    case AIEVENT_DRIVER_IN:
    case AIEVENT_DRIVER_OUT:
        break;
    default:
        return pH->EndFunction();
    }

    if (IAIObject* aiObject = pEntity->GetAI())
    {
        aiObject->Event(eventType, &eventParams);
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetLocalBBox(IFunctionHandler* pH, Vec3 vMin, Vec3 vMax)
{
    GET_ENTITY;

    IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        pRenderComponent->SetLocalBounds(AABB(vMin, vMax), true);
    }
#if ENABLE_CRY_PHYSICS
    else if (IComponentRopePtr pRope = pEntity->GetComponent<IComponentRope>())
    {
        Vec3 pos = pEntity->GetWorldPos();
        pRope->GetRopeRenderNode()->SetBBox(AABB(pos + vMin, pos + vMax));
    }
#endif
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetLocalBBox(IFunctionHandler* pH)
{
    GET_ENTITY;

    AABB box(Vec3(0, 0, 0), Vec3(0, 0, 0));
    IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        pRenderComponent->GetLocalBounds(box);
        //apply scaling if there is any
        box.SetTransformedAABB(Matrix33::CreateScale(pEntity->GetScale()), box);
    }
    return pH->EndFunction(Script::SetCachedVector(box.min, pH, 1), Script::SetCachedVector(box.max, pH, 2));
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetWorldBBox(IFunctionHandler* pH)
{
    GET_ENTITY;

    AABB box(Vec3(0, 0, 0), Vec3(0, 0, 0));
    IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        pRenderComponent->GetWorldBounds(box);
    }
    return pH->EndFunction(Script::SetCachedVector(box.min, pH, 1), Script::SetCachedVector(box.max, pH, 2));
}


//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetProjectedWorldBBox(IFunctionHandler* pH)
{
    GET_ENTITY;

    AABB aabb(Vec3(0, 0, 0), Vec3(0, 0, 0));
    IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();

    if (pRenderComponent)
    {
        pRenderComponent->GetLocalBounds(aabb);

        Matrix33    tm = Matrix33(pEntity->GetWorldTM());
        Vec3            pt = pEntity->GetWorldTM().GetTranslation();

        Vec3 vertices[8];
        vertices[0] = (tm * Vec3(aabb.min.x, aabb.min.y, aabb.min.z)) + pt;
        vertices[1] = (tm * Vec3(aabb.min.x, aabb.max.y, aabb.min.z)) + pt;
        vertices[2] = (tm * Vec3(aabb.max.x, aabb.max.y, aabb.min.z)) + pt;
        vertices[3] = (tm * Vec3(aabb.max.x, aabb.min.y, aabb.min.z)) + pt;
        vertices[4] = (tm * Vec3(aabb.min.x, aabb.min.y, aabb.max.z)) + pt;
        vertices[5] = (tm * Vec3(aabb.min.x, aabb.max.y, aabb.max.z)) + pt;
        vertices[6] = (tm * Vec3(aabb.max.x, aabb.max.y, aabb.max.z)) + pt;
        vertices[7] = (tm * Vec3(aabb.max.x, aabb.min.y, aabb.max.z)) + pt;

        IRenderer* pRenderer = gEnv->pRenderer;

        for (int i = 0; i < 8; i++)
        {
            pRenderer->ProjectToScreen(vertices[i].x, vertices[i].y, vertices[i].z, &vertices[i].x, &vertices[i].y, &vertices[i].z);
            vertices[i].x *= 8;
            vertices[i].y *= 6;
        }

        aabb.min = vertices[0];
        aabb.max = vertices[0];

        for (int j = 0; j < 8; j++)
        {
            if (vertices[j].x < aabb.min.x)
            {
                aabb.min.x = vertices[j].x;
            }
            if (vertices[j].x > aabb.max.x)
            {
                aabb.max.x = vertices[j].x;
            }
            if (vertices[j].y < aabb.min.y)
            {
                aabb.min.y = vertices[j].y;
            }
            if (vertices[j].y > aabb.max.y)
            {
                aabb.max.y = vertices[j].y;
            }
        }
    }

    return pH->EndFunction(Script::SetCachedVector(aabb.min, pH, 1), Script::SetCachedVector(aabb.max, pH, 2));
}


//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetTriggerBBox(IFunctionHandler* pH, Vec3 vMin, Vec3 vMax)
{
    GET_ENTITY;

    IComponentTriggerPtr pTriggerComponent = pEntity->GetOrCreateComponent<IComponentTrigger>();

    if (pTriggerComponent)
    {
        pTriggerComponent->SetTriggerBounds(AABB(vMin, vMax));
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetTriggerBBox(IFunctionHandler* pH)
{
    GET_ENTITY;

    AABB box(Vec3(0, 0, 0), Vec3(0, 0, 0));
    IComponentTriggerPtr  pTriggerComponent = pEntity->GetComponent<IComponentTrigger>();
    if (pTriggerComponent)
    {
        pTriggerComponent->GetTriggerBounds(box);
    }
    return pH->EndFunction(box.min, box.max);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::InvalidateTrigger(IFunctionHandler* pH)
{
    GET_ENTITY;

    IComponentTriggerPtr  pTriggerComponent = pEntity->GetComponent<IComponentTrigger>();
    if (pTriggerComponent)
    {
        pTriggerComponent->InvalidateTrigger();
    }
    return pH->EndFunction();
}

int CScriptBind_Entity::ForwardTriggerEventsTo(IFunctionHandler* pH, ScriptHandle entityId)
{
    GET_ENTITY;

    IComponentTriggerPtr  pTriggerComponent = pEntity->GetComponent<IComponentTrigger>();
    if (pTriggerComponent)
    {
        pTriggerComponent->ForwardEventsTo((EntityId)entityId.n);
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetUpdateRadius(IFunctionHandler* pH)
{
    GET_ENTITY;

    float minRadius, maxRadius;
    if (!pH->GetParams(minRadius, maxRadius))
    {
        pH->EndFunction();
    }

    //pEntity->SetUpdateRadiuses(minRadius,maxRadius);
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetUpdateRadius(IFunctionHandler* pH)
{
    GET_ENTITY;

    float minRadius = 0.0f, maxRadius = 0.0f;
    //pEntity->GetUpdateRadiuses(minRadius,maxRadius);
    return pH->EndFunction(minRadius, maxRadius);
}

// parameters: shader param name, value, slot id
int CScriptBind_Entity::SetPublicParam(IFunctionHandler* pH)
{
    GET_ENTITY;

    if (pH->GetParamCount() < 2)
    {
        SCRIPT_CHECK_PARAMETERS(2);
    }

    // get params;
    const char* name = 0;
    const char* materialName = 0;
    int nSlotId = 0;

    pH->GetParam(1, name);

    if (pH->GetParamCount() > 2)
    {
        pH->GetParam(3, materialName);

        if (materialName)
        {
            // <<TODO>> currently there is no way to get a materialId!
            // it's not needed currently, anyway.
        }

        if (pH->GetParamCount() > 3 && (pH->GetParamType(4) == svtNumber))
        {
            pH->GetParam(4, nSlotId);
        }
    }

    SShaderParam shaderParam;
    shaderParam.m_Name = name;

    switch (pH->GetParamType(2))
    {
    case svtPointer:
    {
        ScriptHandle texId;
        pH->GetParam(2, texId);

        shaderParam.m_Type = eType_TEXTURE_HANDLE;
        shaderParam.m_Value.m_Int = (int)texId.n;
    }
    break;
    case svtNumber:
    {
        float number;
        pH->GetParam(2, number);

        shaderParam.m_Type = eType_FLOAT;
        shaderParam.m_Value.m_Float = number;
    }
    break;
    case svtObject:
    {
        Vec3 vec;


        if (pH->GetParam(2, vec))
        {
            shaderParam.m_Type = eType_VECTOR;
            shaderParam.m_Value.m_Vector[0] = vec.x;
            shaderParam.m_Value.m_Vector[1] = vec.y;
            shaderParam.m_Value.m_Vector[2] = vec.z;
        }
        else
        {
            SmartScriptTable col(pH->GetIScriptSystem(), true);

            if (pH->GetParam(2, col))
            {
                shaderParam.m_Type = eType_FCOLOR;

                if (col->GetValue("r", shaderParam.m_Value.m_Color[0]))
                {
                    col->GetValue("g", shaderParam.m_Value.m_Color[1]);
                    col->GetValue("b", shaderParam.m_Value.m_Color[2]);
                    col->GetValue("a", shaderParam.m_Value.m_Color[3]);
                }
                else if (col->GetAt(1, shaderParam.m_Value.m_Color[0]))
                {
                    col->GetAt(2, shaderParam.m_Value.m_Color[1]);
                    col->GetAt(3, shaderParam.m_Value.m_Color[2]);
                    col->GetAt(4, shaderParam.m_Value.m_Color[3]);
                }
            }
        }
    }
    break;
    }

    //shaderParam.m_nMaterial = materialId;

    //  CEntityObject *pObject = pEntity->GetComponent<IComponentRender>()->GetSlot(slotId);
    /*
    IComponentRenderPtr pComponent = pEntity->GetComponent<IComponentRender>();

  IShaderPublicParams *pPublicParams = NULL; //pComponent->GetShaderPublicParams();

    for (int i = 0; i < pPublicParams->GetParamCount(); i++)
    {
    // Check if parameter already exists, if so just update parameter
        if (!azstricmp(pPublicParams->GetParam(i).m_Name, name))
        {
            pPublicParams->SetParam(i, shaderParam);
            return pH->EndFunction();
        }
    }

  // parameters still doens't exist, add parameter
    pPublicParams->AddParam(shaderParam);
    */

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::Activate(IFunctionHandler* pH, int bActive)
{
    GET_ENTITY;
    pEntity->Activate(bActive != 0);
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::IsActive(IFunctionHandler* pH)
{
    GET_ENTITY;
    bool bActive = pEntity->IsActive();
    return pH->EndFunction(bActive);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::IsFromPool(IFunctionHandler* pH)
{
    GET_ENTITY;
    const bool bFromPool = pEntity->IsFromPool();
    return pH->EndFunction(bFromPool);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetUpdatePolicy(IFunctionHandler* pH, int nUpdatePolicy)
{
    GET_ENTITY;
    pEntity->SetUpdatePolicy((EEntityUpdatePolicy)nUpdatePolicy);
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetAnimationEvent(IFunctionHandler* pH, int nSlot, const char* sAnimation)
{
    //this is obsolete! Use IAnimEvents instead
    return 0;
}

//////////////////////////////////////////////////////////////////////////
//Now is using dynamic parameters because the argument can be both a string or a number
//int CScriptBind_Entity::SetAnimationKeyEvent(IFunctionHandler *pH,int nSlot,const char *sAnimation,int nFrameID,const char *sEvent)
int CScriptBind_Entity::SetAnimationKeyEvent(IFunctionHandler* pH)
{
    //this is obsolete! Use IAnimEvents instead
    return 0;
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::DisableAnimationEvent(IFunctionHandler* pH, int nSlot, const char* sAnimation)
{
    //this is obsolete! Use IAnimEvents instead
    return 0;

    /*  GET_ENTITY;

        ICharacterInstance* pCharacter = pEntity->GetCharacter(nSlot);
        if (!pCharacter)
            return pH->EndFunction();

        IComponentScriptPtr scriptComponent = pEntity->GetComponent<IComponentScript>();
        if (scriptComponent)
        {
            ICharInstanceSink *pSink = scriptComponent->GetCharacterScriptSink();
            if (pSink)
            {
                //pCharacter->RemoveAnimationEventSink(sAnimation,pSink);
            }
        }

        return pH->EndFunction();*/
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetAnimationSpeed(IFunctionHandler* pH, int characterSlot, int layer, float speed)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (pCharacter)
    {
        if (layer < 0)
        {
            pCharacter->SetPlaybackScale(speed);
        }
        else
        {
            pCharacter->GetISkeletonAnim()->SetLayerPlaybackScale(layer, speed);
        }
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetAnimationTime(IFunctionHandler* pH, int nSlot, int nLayer, float fNormalizedTime)
{
    GET_ENTITY;
    assert(fNormalizedTime >= 0.0f && fNormalizedTime <= 1.0f);
    ICharacterInstance* pCharacter = pEntity->GetCharacter(nSlot);
    if (pCharacter)
    {
        pCharacter->GetISkeletonAnim()->SetLayerNormalizedTime(nLayer, fNormalizedTime);
    }
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetAnimationTime(IFunctionHandler* pH, int nSlot, int nLayer)
{
    GET_ENTITY;

    float fTime = 0;
    ICharacterInstance* pCharacter = pEntity->GetCharacter(nSlot);
    if (pCharacter)
    {
        fTime = pCharacter->GetISkeletonAnim()->GetLayerNormalizedTime(nLayer);
    }
    return pH->EndFunction(fTime);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SelectPipe(IFunctionHandler* pH)
{
    GET_ENTITY;

    IAIObject* pAIObject = pEntity->GetAI();
    if (!pAIObject)
    {
        EntityWarning("SelectPipe: The entity does not have AI");
        return pH->EndFunction();
    }

    IPipeUser* pPipeUser = pAIObject->CastToIPipeUser();
    if (!pPipeUser)
    {
        EntityWarning("SelectPipe: The entity's AI is not a pipe user");
        return pH->EndFunction();
    }

    int nParamCount = pH->GetParamCount();
    if (nParamCount < 2)
    {
        EntityWarning("SelectPipe: Too few parameters");
        return pH->EndFunction();
    }

    if (pH->GetParamType(1) != svtNumber || pH->GetParamType(2) != svtString)
    {
        EntityWarning("SelectPipe: Wrong parameter types");
        return pH->EndFunction();
    }

    int id;
    pH->GetParam(1, id);

    const char* szPipeName = 0;
    pH->GetParam(2, szPipeName);

    int targetEntityId = 0;
    const char* szTargetName = 0;
    IAIObject* pTargetAI = 0;
    bool bResetAlways = false;

    int eventId = 0;

    if (nParamCount > 2)
    {
        switch (pH->GetParamType(3))
        {
        case svtNull:
            break;

        case svtPointer:
        {
            ScriptHandle hdl;
            pH->GetParam(3, hdl);
            targetEntityId = (int)hdl.n;
        }
        break;

        case svtNumber:
            pH->GetParam(3, targetEntityId);
            break;

        default:
            pH->GetParam(3, szTargetName);
            pTargetAI = pPipeUser->GetSpecialAIObject(szTargetName);
            if (pTargetAI)
            {
                szTargetName = 0;
            }
        }

        if ((nParamCount > 3) && (pH->GetParamType(4) == svtNumber))
        {
            pH->GetParam(4, eventId);
        }

        if (nParamCount > 4)
        {
            pH->GetParam(5, bResetAlways);
        }
    }

    if (targetEntityId)
    {
        IEntity* pTargetEntity = m_pEntitySystem->GetEntity(targetEntityId);
        if (pTargetEntity)
        {
            pTargetAI = pTargetEntity->GetAI();
        }
    }
    else if (szTargetName)
    {
        pTargetAI = gEnv->pAISystem->GetAIObjectManager()->GetAIObjectByName(0, szTargetName);
    }


    bool bResult = false;

    if (nParamCount > 5)
    {
        GoalParams node;
        GoalParamsHelper(pH, 6, node);
        if (node)
        {
            bResult = pPipeUser->SelectPipe(id, szPipeName, pTargetAI, eventId, bResetAlways, &node);
        }
    }
    else
    {
        bResult = pPipeUser->SelectPipe(id, szPipeName, pTargetAI, eventId, bResetAlways);
    }

    if (!bResult)
    {
        m_pISystem->Warning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, VALIDATOR_FLAG_AI, 0,
            "[AIWARNING] Entity %s failed to select goal pipe %s", pEntity->GetName(), szPipeName);
    }
    return pH->EndFunction(bResult);
}



void CScriptBind_Entity::GoalParamsHelper(IFunctionHandler* pH, uint32 index, GoalParams& node)
{
    if (pH->GetParamType(index) == svtObject)
    {
        SmartScriptTable table;
        if (pH->GetParam(index, table))
        {
            int i = 0;
            SmartScriptTable child;

            node.SetName("params");

            while (table->GetAt(++i, child))
            {
                const char* name;
                ScriptAnyValue value;

                if (child->GetValue("name", name) && child->GetValueAny("value", value))
                {
                    GoalParams childNode;
                    childNode.SetName(name);

                    switch (value.GetVarType())
                    {
                    case svtBool:
                    {
                        bool tmp;
                        value.CopyTo(tmp);
                        childNode.SetValue(tmp);
                    }
                    break;
                    case svtNumber:
                    {
                        float tmp;
                        value.CopyTo(tmp);
                        childNode.SetValue(tmp);
                    }
                    break;
                    case svtPointer:
                    {
                        ScriptHandle tmp;
                        value.CopyTo(tmp);
                        childNode.SetValue(static_cast<int>(tmp.n));
                    }
                    break;
                    case svtString:
                    {
                        const char* tmp;
                        value.CopyTo(tmp);
                        childNode.SetValue(tmp);
                    }
                    break;
                    }

                    int32 id;
                    if (child->GetValue("id", id))
                    {
                        GoalParams additional;
                        additional.SetName("id");
                        additional.SetValue(id);

                        childNode.AddChild(additional);
                    }

                    node.AddChild(childNode);
                }
            }
        }
    }
}


int CScriptBind_Entity::InsertSubpipe(IFunctionHandler* pH)
{
    GET_ENTITY;

    IAIObject* pAIObject = pEntity->GetAI();
    if (!pAIObject)
    {
        EntityWarning("InsertSubpipe: The entity does not have AI");
        return pH->EndFunction();
    }

    IPipeUser* pPipeUser = pAIObject->CastToIPipeUser();
    if (!pPipeUser)
    {
        EntityWarning("InsertSubpipe: The entity's AI is not a pipe user");
        return pH->EndFunction();
    }

    int nParamCount = pH->GetParamCount();
    if (nParamCount < 2)
    {
        EntityWarning("InsertSubpipe: Too few parameters");
        return pH->EndFunction();
    }

    if (pH->GetParamType(1) != svtNumber || pH->GetParamType(2) != svtString)
    {
        EntityWarning("InsertSubpipe: Wrong parameter types");
        return pH->EndFunction();
    }

    int id;
    pH->GetParam(1, id);

    const char* szPipeName = 0;
    pH->GetParam(2, szPipeName);

    int targetEntityId = 0;
    const char* szTargetName = 0;
    IAIObject* pTargetAI = 0;

    int eventId = 0;

    if (nParamCount > 2)
    {
        switch (pH->GetParamType(3))
        {
        case svtNull:
            break;

        case svtPointer:
        {
            ScriptHandle hdl;
            pH->GetParam(3, hdl);
            targetEntityId = (int)hdl.n;
        }
        break;

        case svtNumber:
            pH->GetParam(3, targetEntityId);
            break;

        default:
            pH->GetParam(3, szTargetName);
            pTargetAI = pPipeUser->GetSpecialAIObject(szTargetName);
            if (pTargetAI)
            {
                szTargetName = 0;
            }
        }

        if ((nParamCount > 3) && (pH->GetParamType(4) == svtNumber))
        {
            pH->GetParam(4, eventId);
        }
    }

    if (targetEntityId)
    {
        IEntity* pTargetEntity = m_pEntitySystem->GetEntity(targetEntityId);
        if (pTargetEntity)
        {
            pTargetAI = pTargetEntity->GetAI();
        }
    }
    else if (szTargetName)
    {
        pTargetAI = gEnv->pAISystem->GetAIObjectManager()->GetAIObjectByName(0, szTargetName);
    }

    bool bResult = false;

    if (nParamCount > 4)
    {
        GoalParams node;
        GoalParamsHelper(pH, 5, node);
        if (node)
        {
            bResult = pPipeUser->InsertSubPipe(id, szPipeName, pTargetAI, eventId, &node) != 0;
        }
    }
    else
    {
        bResult = pPipeUser->InsertSubPipe(id, szPipeName, pTargetAI, eventId) != 0;
    }

    //bool bResult = pPipeUser->InsertSubPipe(id, szPipeName, pTargetAI, eventId) != 0;

    /*
    gEnv->pLog->SetFileName("AILOG.txt");
    if (bResult)
        gEnv->pLog->LogToFile("[%d] %s  SELECT_PIPE %s",gEnv->pAISystem->GetAITickCount(),pObject->GetName(),pName);
    else
        gEnv->pLog->LogToFile("[%d] %s  PIPE_SELECTION_FAILED   %s",gEnv->pAISystem->GetAITickCount(),pObject->GetName(),pName);
    gEnv->pLog->SetFileName("Log.txt");
    */

    return pH->EndFunction(bResult);
}

int CScriptBind_Entity::CancelSubpipe(IFunctionHandler* pH)
{
    GET_ENTITY;

    IAIObject* pObject = pEntity->GetAI();
    int eventId = 0;
    if (pH->GetParamCount() > 1)
    {
        EntityWarning("CancelSubpipe: only 1 parameter expected");
    }

    if (pH->GetParamCount() == 1)
    {
        if (pH->GetParamType(1) != svtNumber)
        {
            EntityWarning("CancelSubpipe: wrong parameter type (number expected)");
            pH->EndFunction();
        }
        pH->GetParam(1, eventId);
    }

    bool res = false;
    IPipeUser* pPipeUser = CastToIPipeUserSafe(pObject);
    if (pPipeUser)
    {
        res = pPipeUser->CancelSubPipe(eventId);
    }

    return pH->EndFunction(res);
}


int CScriptBind_Entity::PassParamsToPipe(IFunctionHandler* pH)
{
    GET_ENTITY;

    if (IAIObject* pAIObject = pEntity->GetAI())
    {
        if (IPipeUser* pipeuser = pAIObject->CastToIPipeUser())
        {
            if (pH->GetParamCount() == 1)
            {
                GoalParams node;
                GoalParamsHelper(pH, 1, node);
                if (node)
                {
                    IGoalPipe* pipe = pipeuser->GetCurrentGoalPipe();
                    if (pipe)
                    {
                        pipe->ParseParams(node);
                    }
                }
            }
        }
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::IsUsingPipe(IFunctionHandler* pH)
{
    GET_ENTITY;
    if (pH->GetParamCount() < 1)
    {
        EntityWarning("IsUsingPipe: too few parameters");
        pH->EndFunction();
    }
    if (pH->GetParamType(1) != svtString)
    {
        EntityWarning("IsUsingPipe: wrong parameter type");
        pH->EndFunction();
    }

    const char* pName;
    pH->GetParam(1, pName);

    IAIObject* pObject = pEntity->GetAI();

    IPipeUser* pPipeUser = CastToIPipeUserSafe(pObject);
    if (pPipeUser)
    {
        return pH->EndFunction(pPipeUser->IsUsingPipe(pName));
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GotoState(IFunctionHandler* pH, const char* sStateName)
{
    GET_ENTITY;

    bool bRes = false;
    IComponentScriptPtr scriptComponent = pEntity->GetComponent<IComponentScript>();
    if (scriptComponent)
    {
        bRes = scriptComponent->GotoState(sStateName);
    }

    /*
        if(!pEntity->IsStateClientside() && !gEnv->pGame->GetModuleState( EGameServer ))
        {
            // GotoState should only be called on the client when StateClientSide is on for this entity
            EntityWarning("ScriptObjectEntity:GotoState on the client! (EntityClass:'%s' Name:'%s', State:'%s')",
                pEntity->GetEntityClassName(), pEntity->GetName(), sState);
        }
    }
    */
    return pH->EndFunction(bRes);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::IsInState(IFunctionHandler* pH, const char* sStateName)
{
    GET_ENTITY;

    bool bRes = false;
    IComponentScriptPtr scriptComponent = pEntity->GetComponent<IComponentScript>();
    if (scriptComponent)
    {
        bRes = scriptComponent->IsInState(sStateName);
    }

    return pH->EndFunction(bRes);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetState(IFunctionHandler* pH)
{
    GET_ENTITY;

    const char* sState = "";
    IComponentScriptPtr scriptComponent = pEntity->GetComponent<IComponentScript>();
    if (scriptComponent)
    {
        sState = scriptComponent->GetState();
    }
    return pH->EndFunction(sState);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetCurAnimation(IFunctionHandler* pH)
{
    GET_ENTITY;
    int iPos;
    ICharacterInstance* pCharacter = NULL;

    SCRIPT_CHECK_PARAMETERS(1);

    pH->GetParam(1, iPos);

    pCharacter = pEntity->GetCharacter(iPos);
    if (pCharacter)
    {
        //ask Ivo for details
        //if (pCharacter->GetCurAnimation() && pCharacter->GetCurAnimation()[0] != '\0')
        //  return pH->EndFunction(pCharacter->GetCurAnimation());
    }

    return pH->EndFunction();
}

int CScriptBind_Entity::SetTimer(IFunctionHandler* pH)
{
    GET_ENTITY;
    int timerId = 0;
    int msec = 0;
    pH->GetParams(timerId, msec);
    pEntity->SetTimer(timerId, msec);
    return pH->EndFunction();
}

int CScriptBind_Entity::KillTimer(IFunctionHandler* pH)
{
    GET_ENTITY;

    int timerId = 0;
    pH->GetParams(timerId);
    pEntity->KillTimer(timerId);
    return pH->EndFunction();
}

int CScriptBind_Entity::SetScriptUpdateRate(IFunctionHandler* pH, int nMillis)
{
    GET_ENTITY;

    IComponentScriptPtr scriptComponent = pEntity->GetComponent<IComponentScript>();
    if (scriptComponent)
    {
        scriptComponent->SetScriptUpdateRate(((float)nMillis) / 1000.0f);
    }

    return pH->EndFunction();
}

int CScriptBind_Entity::IsHidden(IFunctionHandler* pH)
{
    GET_ENTITY;

    return pH->EndFunction(pEntity->IsHidden());
}


/*! Retrieves the position of a bone
    @param bone Name of the helper object in the model
    @return Three component vector containing the position
*/
int CScriptBind_Entity::GetBonePos(IFunctionHandler* pH)
{
    GET_ENTITY;

    const char* sBoneName;
    if (!pH->GetParams(sBoneName))
    {
        return pH->EndFunction();
    }

    ICharacterInstance* pCharacter = pEntity->GetCharacter(0);
    if (!pCharacter)
    {
        return pH->EndFunction();
    }

    //IJoint *pBone = pCharacter->GetISkeleton()->GetIJointByName(sBoneName);
    int32 id = pCharacter->GetIDefaultSkeleton().GetJointIDByName(sBoneName);
    if (id == -1)
    {
        gEnv->pLog->Log("ERROR: CScriptObjectWeapon::GetBonePos: Bone not found: %s", sBoneName);
        return pH->EndFunction();
    }

    Vec3 JointPos = pCharacter->GetISkeletonPose()->GetAbsJointByID(id).t;
    Vec3 vBonePos = pEntity->GetSlotWorldTM(0) * JointPos; //pEntity->GetWorldTM().TransformPoint( pBone->GetBonePosition() );
    assert(!_isnan(vBonePos.x + vBonePos.y + vBonePos.z));

    //  Vec3 vBonePos = pEntity->GetSlotWorldTM(0) * pBone->GetBonePosition(); //pEntity->GetWorldTM().TransformPoint( pBone->GetBonePosition() );
    return pH->EndFunction(Script::SetCachedVector(vBonePos, pH, 2));
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetBoneDir(IFunctionHandler* pH)
{
    GET_ENTITY;

    const char* sBoneName;
    if (!pH->GetParams(sBoneName))
    {
        return pH->EndFunction();
    }

    ICharacterInstance* pCharacter = pEntity->GetCharacter(0);
    if (!pCharacter)
    {
        return pH->EndFunction();
    }

    //  IJoint * pBone = pCharacter->GetISkeleton()->GetIJointByName(sBoneName);
    int16 pBone_id = pCharacter->GetIDefaultSkeleton().GetJointIDByName(sBoneName);
    if (pBone_id == -1)
    {
        gEnv->pLog->Log("ERROR: CScriptObjectWeapon::GetBonePos: Bone not found: %s", sBoneName);
        return pH->EndFunction();
    }

    //  Vec3 vBoneDir = pCharacter->GetISkeleton()->GetAbsJMatrixByID(pBone_id).TransformVector(FORWARD_DIRECTION);
    Vec3 vBoneDir = pCharacter->GetISkeletonPose()->GetAbsJointByID(pBone_id).q * FORWARD_DIRECTION;
    vBoneDir = pEntity->GetSlotWorldTM(0).TransformVector(vBoneDir);
    return pH->EndFunction(vBoneDir);
}

int CScriptBind_Entity::GetBoneVelocity(IFunctionHandler* pH, int characterSlot, const char* boneName)
{
    GET_ENTITY;

    Vec3 v(0, 0, 0);

#if ENABLE_CRY_PHYSICS
    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);
    if (pCharacter)
    {
        int16 pBone_id = pCharacter->GetIDefaultSkeleton().GetJointIDByName(boneName);
        if (pBone_id >= 0)
        {
            IPhysicalEntity* pPhysics = pEntity->GetPhysics();

            if (pPhysics)
            {
                pe_status_dynamics dyn;
                dyn.partid = pBone_id;//pBone->GetId();

                pPhysics->GetStatus(&dyn);
                v = dyn.v;
            }
        }
    }
#endif // ENABLE_CRY_PHYSICS

    return pH->EndFunction(Script::SetCachedVector(v, pH, 3));
}


int CScriptBind_Entity::GetBoneAngularVelocity(IFunctionHandler* pH, int characterSlot, const char* boneName)
{
    GET_ENTITY;

    Vec3 w(0, 0, 0);

#if ENABLE_CRY_PHYSICS
    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);
    if (pCharacter)
    {
        //  IJoint *pBone = pCharacter->GetISkeleton()->GetIJointByName(boneName);
        int32 pBone_id = pCharacter->GetIDefaultSkeleton().GetJointIDByName(boneName);
        if (pBone_id >= 0)
        {
            IPhysicalEntity* pPhysics = pEntity->GetPhysics();
            if (pPhysics)
            {
                pe_status_dynamics dyn;
                dyn.partid = pBone_id;//pBone->GetId();

                pPhysics->GetStatus(&dyn);
                w = dyn.w;
            }
        }
    }
#endif // ENABLE_CRY_PHYSICS

    return pH->EndFunction(Script::SetCachedVector(w, pH, 3));
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetBoneNameFromTable(IFunctionHandler* pH)
{
    GET_ENTITY;
    SCRIPT_CHECK_PARAMETERS(1);
    int idx;
    pH->GetParam(1, idx);

    ICharacterInstance* pCharacter = pEntity->GetCharacter(0);
    if (!pCharacter)
    {
        return pH->EndFunction();
    }

    const char* pName = pCharacter->GetIDefaultSkeleton().GetJointNameByID(idx);
    return pH->EndFunction(pName);
}

/*!retrieve the material id(surfaceid) of the object in contact with the entity
    @return the material id of the colliding entity
*/
int CScriptBind_Entity::GetTouchedSurfaceID(IFunctionHandler* pH)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;
    IPhysicalEntity* pe;


    //pEntity=m_pPlayer->GetEntity();
    pe = pEntity->GetPhysics();
    if (pe)
    {
        coll_history_item hItem;
        pe_status_collisions status;
        status.pHistory = &hItem;
        status.age = .2f;
        if (pe->GetStatus(&status))// && (!status.bFlying))
        {
            //int tmpIdx = hItem.idmat[1];
            return pH->EndFunction(hItem.idmat[1]);
        }
    }
#endif // ENABLE_CRY_PHYSICS
    return pH->EndFunction(-1);
}

//
//!retrieves point of collision for rigid body
int CScriptBind_Entity::GetTouchedPoint(IFunctionHandler* pH)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;
    IPhysicalEntity* pe;

    //pEntity=m_pPlayer->GetEntity();
    pe = pEntity->GetPhysics();
    if (pe)
    {
        coll_history_item hItem;
        pe_status_collisions status;
        status.pHistory = &hItem;
        status.age = .2f;
        if (pe->GetStatus(&status))// && (!status.bFlying))
        {
            CScriptVector oVec(m_pSS);
            //  vec=pEntity->GetPos(false);
            Vec3 vec = hItem.pt;
            oVec = vec;
            return pH->EndFunction(*oVec);
        }
    }
#endif // ENABLE_CRY_PHYSICS
    return pH->EndFunction(-1);
}

//
//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::Damage(IFunctionHandler* pH)
{
    /*
    GET_ENTITY;

    SmartScriptTable pObj(m_pSS,true);
    if(pH->GetParam(1,pObj))
    {
        pEntity->OnDamage(pObj);
    }
    return pH->EndFunction();
    */
    return pH->EndFunction();
}


//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetExplosionObstruction(IFunctionHandler* pH)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;

    IPhysicalEntity* pPE = pEntity->GetPhysics();

    if (pPE)
    {
        return pH->EndFunction(1.0f - m_pISystem->GetIPhysicalWorld()->IsAffectedByExplosion(pPE));
    }
#endif // ENABLE_CRY_PHYSICS

    return pH->EndFunction(1.0f);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetExplosionImpulse(IFunctionHandler* pH)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;

    IPhysicalEntity* pPE = pEntity->GetPhysics();
    float impulse = 0.0f;

    if (pPE)
    {
        Vec3 impulseVec(0.0f, 0.0f, 0.0f);

        m_pISystem->GetIPhysicalWorld()->IsAffectedByExplosion(pPE, &impulseVec);
        impulse = impulseVec.GetLength();
    }

    return pH->EndFunction(impulse);
#else
    return pH->EndFunction(0.f);
#endif // ENABLE_CRY_PHYSICS
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::ActivatePlayerPhysics(IFunctionHandler* pH, bool bEnable)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;

    IPhysicalEntity* pPhysicalEntity = pEntity->GetPhysics();
    if (pPhysicalEntity)
    {
        pe_player_dynamics pd;
        pd.bActive = bEnable;
        pPhysicalEntity->SetParams(&pd);
    }
#endif // ENABLE_CRY_PHYSICS
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetEntitiesInContact(IFunctionHandler* pH)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;

    AABB bbox;

    IComponentPhysicsPtr pPhysicsComponent = pEntity->GetComponent<IComponentPhysics>();
    if (pPhysicsComponent)
    {
        pPhysicsComponent->GetWorldBounds(bbox);
    }
    else
    {
        pH->EndFunction();
    }


    IPhysicalWorld* pWorld = m_pISystem->GetIPhysicalWorld();
    IPhysicalEntity** ppColliders;
    int cnt = 0, valid = 0;
    if (cnt = pWorld->GetEntitiesInBox(bbox.min, bbox.max, ppColliders, ent_living | ent_rigid | ent_sleeping_rigid | ent_static))
    {
        // execute on collide for all of the entities
        SmartScriptTable pObj(m_pSS);
        for (int i = 0; i < cnt; i++)
        {
            IEntity* pFoundEntity = m_pEntitySystem->GetEntityFromPhysics(ppColliders[i]);
            if (pFoundEntity)
            {
                if (pFoundEntity->GetId() == pEntity->GetId())
                {
                    continue;
                }

                valid++;
                pObj->SetAt(pFoundEntity->GetId(), pFoundEntity->GetScriptTable());
            }
        }
        if (valid)
        {
            return pH->EndFunction(pObj);
        }
    }
#endif // ENABLE_CRY_PHYSICS

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetDefaultIdleAnimations(IFunctionHandler* pH)
{
    GET_ENTITY;
    //SCRIPT_CHECK_PARAMETERS(1);
    assert(pH->GetParamCount() == 1 || pH->GetParamCount() == 2);

    const char* animname = NULL;
    int pos;
    pH->GetParam(1, pos);
    if (pH->GetParamCount() > 1)
    {
        pH->GetParam(2, animname);
    }

    //ICharacterInstance *pCharacter = pEntity->GetCharacter(pos);
    //if (pCharacter)
    //  pCharacter->SetDefaultIdleAnimation( pos, animname );

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetAnimationLength(IFunctionHandler* pH, int characterSlot, const char* animation)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    float length = 0.0f;

    if (pCharacter)
    {
        int id = pCharacter->GetIAnimationSet()->GetAnimIDByName(animation);
        if (id < 0)
        {
            CryLog("CScriptBind_Entity::GetAnimationLength Anim %s not found in entity %s", animation, pEntity->GetName());
        }
        else
        {
            length = pCharacter->GetIAnimationSet()->GetDuration_sec(id);
        }
        /*
        int AnimID = pCharacter->GetIAnimationSet()->GetIDByName(animation);
        if (AnimID>=0)
            length = pCharacter->GetIAnimationSet()->GetDuration_sec( AnimID );
        //length = pCharacter->GetIAnimationSet()->GetLength(animation);
        */
    }

    return pH->EndFunction(length);
}

int CScriptBind_Entity::SetAnimationFlip(IFunctionHandler* pH, int characterSlot, Vec3 flip)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (pCharacter)
    {
        //  pCharacter->SetScale(flip);
    }

    return pH->EndFunction();
}

/*! Set the material of the entity
    @param materialName Name of material.
*/
int CScriptBind_Entity::SetMaterial(IFunctionHandler* pH)
{
    GET_ENTITY;
    SCRIPT_CHECK_PARAMETERS(1);
    const char* sMaterialName = 0;
    if (pH->GetParam(1, sMaterialName))
    {
        _smart_ptr<IMaterial> pMaterial = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(sMaterialName);
        pEntity->SetMaterial(pMaterial);
    }
    return pH->EndFunction();
}


int CScriptBind_Entity::GetEntityMaterial(IFunctionHandler* pH)
{
    GET_ENTITY;

    if (pEntity)
    {
        if (pEntity->GetMaterial())
        {
            return pH->EndFunction(pEntity->GetMaterial()->GetName());
        }
    }
    return pH->EndFunction();
}


/*! Get the name of custom entity material.
*/
int CScriptBind_Entity::GetMaterial(IFunctionHandler* pH)
{
    GET_ENTITY;
    //SCRIPT_CHECK_PARAMETERS(0);
    if (pEntity)
    {
        //if a slot is specified get the material on the corresponding slot
        int slot;
        if (pH->GetParamCount() > 0 && pH->GetParam(1, slot))
        {
            _smart_ptr<IMaterial> pMat = nullptr;
            if (ICharacterInstance* pCharacter = pEntity->GetCharacter(slot))
            {
                pMat = pCharacter->GetIMaterial();
            }
            else if (IStatObj* pStatObj = pEntity->GetStatObj(slot))
            {
                pMat = pStatObj->GetMaterial();
            }

            if (pMat)
            {
                return pH->EndFunction(pMat->GetName());
            }
        }
        else
        {
            if (pEntity->GetMaterial())
            {
                return pH->EndFunction(pEntity->GetMaterial()->GetName());
            }
        }
    }
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::ChangeAttachmentMaterial(IFunctionHandler* pH, const char* attachmentName, const char* materialName)
{
    IEntity* entity = GetEntity(pH);
    if (!entity)
    {
        return pH->EndFunction();
    }

    _smart_ptr<IMaterial> material = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(materialName);

    if (!material)
    {
        CryLogAlways("Material not found '%s' in %s:ChangeAttachmentMaterial()", materialName, entity->GetName());
        return pH->EndFunction();
    }

    int slotCount = entity->GetSlotCount();
    for (int slot = 0; slot < slotCount; ++slot)
    {
        SEntitySlotInfo slotInfo;
        if (!entity->GetSlotInfo(slot, slotInfo))
        {
            continue;
        }

        if (!slotInfo.pCharacter)
        {
            continue;
        }

        if (IAttachment* attachment = slotInfo.pCharacter->GetIAttachmentManager()->GetInterfaceByName(attachmentName))
        {
            if (IAttachmentObject* attachmentObject = attachment->GetIAttachmentObject())
            {
                attachmentObject->SetReplacementMaterial(material);
            }
        }
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::ReplaceMaterial(IFunctionHandler* pH, int slot, const char* name, const char* replacement)
{
    GET_ENTITY;

    int start = 0;
    int stop = pEntity->GetSlotCount();

    if (slot >= 0)
    {
        start = slot;
        stop = slot + 1;
    }

    _smart_ptr<IMaterial> pReplacement = gEnv->p3DEngine->GetMaterialManager()->FindMaterial(replacement);
    if (!pReplacement)
    {
        pReplacement = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(replacement, false);
    }

    if (!pReplacement)
    {
        CryLogAlways("Material not found '%s' in %s:ReplaceMaterial()", replacement, pEntity->GetName());
    }

    for (int i = start; i < stop; i++)
    {
        SEntitySlotInfo info;
        if (!pEntity->GetSlotInfo(i, info))
        {
            continue;
        }

        if (info.pMaterial && !azstricmp(info.pMaterial->GetName(), name))
        {
            pEntity->SetSlotMaterial(i, pReplacement);
        }

        if (info.pCharacter)
        {
            _smart_ptr<IMaterial> pMaterial = info.pCharacter->GetIMaterial();
            if (pMaterial && !azstricmp(pMaterial->GetName(), name))
            {
                pEntity->SetSlotMaterial(i, pReplacement);
            }


            IAttachmentManager* pManager = info.pCharacter->GetIAttachmentManager();
            int n = pManager->GetAttachmentCount();

            for (int a = 0; a < n; a++)
            {
                IAttachmentObject* pAttachmentObject = pManager->GetInterfaceByIndex(a)->GetIAttachmentObject();
                if (pAttachmentObject)
                {
                    _smart_ptr<IMaterial> pAttachMaterial = (_smart_ptr<IMaterial>)pAttachmentObject->GetBaseMaterial();
                    if (pAttachMaterial && !azstricmp(pAttachMaterial->GetName(), name))
                    {
                        pAttachmentObject->SetReplacementMaterial(pReplacement);
                    }
                }
            }
        }

        else if (info.pStatObj)
        {
            _smart_ptr<IMaterial> pMaterial = info.pStatObj->GetMaterial();
            if (pMaterial && !azstricmp(pMaterial->GetName(), name))
            {
                pEntity->SetSlotMaterial(i, pReplacement);
            }
        }
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::ResetMaterial(IFunctionHandler* pH, int slot)
{
    GET_ENTITY;

    int start = 0;
    int stop = pEntity->GetSlotCount();

    if (slot >= 0)
    {
        start = slot;
        stop = slot + 1;
    }

    for (int i = start; i < stop; i++)
    {
        SEntitySlotInfo info;
        if (!pEntity->GetSlotInfo(i, info))
        {
            continue;
        }

        pEntity->SetSlotMaterial(i, 0);

        if (info.pCharacter)
        {
            IAttachmentManager* pManager = info.pCharacter->GetIAttachmentManager();
            int n = pManager->GetAttachmentCount();

            for (int a = 0; a < n; a++)
            {
                IAttachmentObject* pAttachmentObject = pManager->GetInterfaceByIndex(a)->GetIAttachmentObject();
                if (pAttachmentObject)
                {
                    pAttachmentObject->SetReplacementMaterial(0);
                }
            }
        }
    }

    return pH->EndFunction();
}


//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::CloneMaterial(IFunctionHandler* pH, int slot)
{
    GET_ENTITY;

    IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        _smart_ptr<IMaterial> pMtl = pRenderComponent->GetRenderMaterial(slot);
        if (pMtl)
        {
            const char* sSubMtlName = NULL;
            if (pH->GetParamCount() > 1)
            {
                pH->GetParam(2, sSubMtlName);
            }
            _smart_ptr<IMaterial> pCloned = pMtl->GetMaterialManager()->CloneMultiMaterial(pMtl, sSubMtlName);
            pRenderComponent->SetSlotMaterial(slot, pCloned);
        }
    }
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetMaterialFloat(IFunctionHandler* pH, int slot, int nSubMtlId, const char* sParamName, float fValue)
{
    GET_ENTITY;

    IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        _smart_ptr<IMaterial> pMtl = pRenderComponent->GetRenderMaterial(slot);
        if (pMtl)
        {
            pMtl = pMtl->GetSafeSubMtl(nSubMtlId);
            if (!pMtl->SetGetMaterialParamFloat(sParamName, fValue, false))
            {
                EntityWarning("Material Parameter: %s is not valid material parameter name for material %s", sParamName, pMtl->GetName());
            }
        }
    }
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetMaterialFloat(IFunctionHandler* pH, int slot, int nSubMtlId, const char* sParamName)
{
    GET_ENTITY;

    float fValue = 0;

    IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        _smart_ptr<IMaterial> pMtl = pRenderComponent->GetRenderMaterial(slot);
        if (pMtl)
        {
            pMtl = pMtl->GetSafeSubMtl(nSubMtlId);
            if (!pMtl->SetGetMaterialParamFloat(sParamName, fValue, true))
            {
                EntityWarning("Material Parameter: %s is not valid material parameter name for material %s", sParamName, pMtl->GetName());
            }
        }
    }
    return pH->EndFunction(fValue);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetMaterialVec3(IFunctionHandler* pH, int slot, int nSubMtlId, const char* sParamName, Vec3 vValue)
{
    GET_ENTITY;

    IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        _smart_ptr<IMaterial> pMtl = pRenderComponent->GetRenderMaterial(slot);
        if (pMtl)
        {
            pMtl = pMtl->GetSafeSubMtl(nSubMtlId);
            if (!pMtl->SetGetMaterialParamVec3(sParamName, vValue, false))
            {
                EntityWarning("Material Parameter: %s is not valid material parameter name for material %s", sParamName, pMtl->GetName());
            }
        }
    }
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetMaterialVec3(IFunctionHandler* pH, int slot, int nSubMtlId, const char* sParamName)
{
    GET_ENTITY;

    Vec3 vValue(0, 0, 0);

    IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        _smart_ptr<IMaterial> pMtl = pRenderComponent->GetRenderMaterial(slot);
        if (pMtl)
        {
            pMtl = pMtl->GetSafeSubMtl(nSubMtlId);
            if (!pMtl->SetGetMaterialParamVec3(sParamName, vValue, true))
            {
                EntityWarning("Material Parameter: %s is not valid material parameter name for material %s", sParamName, pMtl->GetName());
            }
        }
    }
    return pH->EndFunction(vValue);
}

/*
//------------------------------------------------------------------------
int CScriptBind_Entity::AddMaterialLayer(IFunctionHandler *pH, int slotId, const char *shader)
{
    GET_ENTITY;

  CEntityObject *pAIObject = pEntity->GetComponent<IComponentRender>()->GetSlot(slotId);

  //  test - only activate first layer
  IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
  if( pRenderComponent )
  {

    uint8 nMtlLayers = pRenderComponent->GetMaterialLayersMask();
    pRenderComponent->SetMaterialLayersMask( nMtlLayers| (1<<0));
  }

    return pH->EndFunction(0);
}

//------------------------------------------------------------------------
int CScriptBind_Entity::RemoveMaterialLayer(IFunctionHandler *pH, int slotId, int id)
{
    GET_ENTITY;

  CEntityObject *pAIObject = pEntity->GetComponent<IComponentRender>()->GetSlot(slotId);

  // todo

    return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Entity::RemoveAllMaterialLayers(IFunctionHandler *pH, int slotId)
{
    GET_ENTITY;

  //  test
  IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
  if( pRenderComponent )
  {
    pRenderComponent->SetMaterialLayersMask( 0 );
    }

    return pH->EndFunction();
}

//------------------------------------------------------------------------
int CScriptBind_Entity::SetMaterialLayerParamF(IFunctionHandler *pH, int slotId, int layerId, const char *name, float value)
{
    GET_ENTITY;

  // todo

    return pH->EndFunction();
}


//------------------------------------------------------------------------
int CScriptBind_Entity::SetMaterialLayerParamV(IFunctionHandler *pH, int slotId, int layerId, const char *name, Vec3 vec)
{
    GET_ENTITY;

  // todo

    return pH->EndFunction();
}


//------------------------------------------------------------------------
int CScriptBind_Entity::SetMaterialLayerParamC(IFunctionHandler *pH, int slotId, int layerId, const char *name, float r, float g, float b, float a)
{
    GET_ENTITY;

  // todo

    return pH->EndFunction();
}*/

//------------------------------------------------------------------------
int CScriptBind_Entity::EnableMaterialLayer(IFunctionHandler* pH, bool enable, int layer)
{
    GET_ENTITY;

    IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        uint8 activeLayers = pRenderComponent->GetMaterialLayersMask();
        if (enable)
        {
            activeLayers |= layer;
        }
        else
        {
            activeLayers &= ~layer;
        }

        pRenderComponent->SetMaterialLayersMask(activeLayers);
    }

    return pH->EndFunction();
}

//------------------------------------------------------------------------
// transform pos from world to local coordinates. uses object when slot<0
int CScriptBind_Entity::ToLocal(IFunctionHandler* pH, int slotId, Vec3 point)
{
    GET_ENTITY;

    Matrix34 worldTM;

    if (slotId < 0)
    {
        worldTM = pEntity->GetWorldTM();
    }
    else
    {
        worldTM = pEntity->GetSlotWorldTM(slotId);
    }

    worldTM.InvertFast();
    Vec3 res(worldTM.TransformPoint(point));

    return pH->EndFunction(Script::SetCachedVector(res, pH, 3));
}

//------------------------------------------------------------------------
// transform pos from local to world coordinates. uses object when slot<0
int CScriptBind_Entity::ToGlobal(IFunctionHandler* pH, int slotId, Vec3 point)
{
    GET_ENTITY;

    Matrix34 worldTM;

    if (slotId < 0)
    {
        worldTM = pEntity->GetWorldTM();
    }
    else
    {
        worldTM = pEntity->GetSlotWorldTM(slotId);
    }

    Vec3 res(worldTM.TransformPoint(point));

    return pH->EndFunction(Script::SetCachedVector(res, pH, 3));
}


//------------------------------------------------------------------------
// transform vector from world to local coordinates. uses object when slot<0
int CScriptBind_Entity::VectorToLocal(IFunctionHandler* pH, int slotId, Vec3 dir)
{
    GET_ENTITY;

    Matrix34 worldTM;

    if (slotId < 0)
    {
        worldTM = pEntity->GetWorldTM();
    }
    else
    {
        worldTM = pEntity->GetSlotWorldTM(slotId);
    }

    worldTM.InvertFast();
    Vec3 res(worldTM.TransformVector(dir));

    return pH->EndFunction(Script::SetCachedVector(res, pH, 3));
}

//------------------------------------------------------------------------
// transform vector from local to world coordinates. uses object when slot<0
int CScriptBind_Entity::VectorToGlobal(IFunctionHandler* pH, int slotId, Vec3 dir)
{
    GET_ENTITY;

    Matrix34 worldTM;

    if (slotId < 0)
    {
        worldTM = pEntity->GetWorldTM();
    }
    else
    {
        worldTM = pEntity->GetSlotWorldTM(slotId);
    }

    Vec3 res(worldTM.TransformVector(dir));

    return pH->EndFunction(Script::SetCachedVector(res, pH, 3));
}


//////////////////////////////////////////////////////////////////////////
/*! Get the velocity of the entity
    @return Three component vector containing the velocity position
*/
int CScriptBind_Entity::GetVelocity(IFunctionHandler* pH)
{
    GET_ENTITY;
    //  SCRIPT_CHECK_PARAMETERS(0);
    Vec3 velocity(0, 0, 0);
#if ENABLE_CRY_PHYSICS
    IPhysicalEntity* phys = pEntity->GetPhysics();
    if (phys)
    {
        pe_status_dynamics  dyn;
        phys->GetStatus(&dyn);
        velocity = dyn.v;
    }
#endif // ENABLE_CRY_PHYSICS
    return pH->EndFunction(Script::SetCachedVector(velocity, pH, 1));
}


//////////////////////////////////////////////////////////////////////////
/*! Get the velocity and angular velocity of the entity
    @return Three component vector containing the velocity position and three component vector containing angular velocity
*/
int CScriptBind_Entity::GetVelocityEx(IFunctionHandler* pH)
{
    GET_ENTITY;

    Vec3 velocity(0, 0, 0);
    Vec3 angularVelocity(0, 0, 0);
#if ENABLE_CRY_PHYSICS
    IPhysicalEntity* phys = pEntity->GetPhysics();
    if (phys)
    {
        pe_status_dynamics  dyn;
        phys->GetStatus(&dyn);
        velocity = dyn.v;
        angularVelocity = dyn.w;
    }
#endif // ENABLE_CRY_PHYSICS
    return pH->EndFunction(Script::SetCachedVector(velocity, pH, 1), Script::SetCachedVector(angularVelocity, pH, 2));
}


//////////////////////////////////////////////////////////////////////////
/*! Set the velocity of the entity
*/
int CScriptBind_Entity::SetVelocity(IFunctionHandler* pH, Vec3 velocity)
{
    GET_ENTITY;

#if ENABLE_CRY_PHYSICS
    IPhysicalEntity* phys = pEntity->GetPhysics();
    if (phys)
    {
        pe_action_set_velocity request;
        request.v = velocity;
        phys->Action(&request);
    }
#endif // ENABLE_CRY_PHYSICS
    return pH->EndFunction();
}


//////////////////////////////////////////////////////////////////////////
/*! Set the velocity and angular velocity of the entity
*/
int CScriptBind_Entity::SetVelocityEx(IFunctionHandler* pH, Vec3 velocity, Vec3 angularVelocity)
{
    GET_ENTITY;

#if ENABLE_CRY_PHYSICS
    IPhysicalEntity* phys = pEntity->GetPhysics();
    if (phys)
    {
        pe_action_set_velocity request;
        request.v = velocity;
        request.w = angularVelocity;
        phys->Action(&request);
    }
#endif // ENABLE_CRY_PHYSICS
    return pH->EndFunction();
}


/*! Get the speed of the entity
@return Speed in float value
*/
int CScriptBind_Entity::GetSpeed(IFunctionHandler* pH)
{
    GET_ENTITY;
    SCRIPT_CHECK_PARAMETERS(0);
    Vec3 velocity(0, 0, 0);
#if ENABLE_CRY_PHYSICS
    IPhysicalEntity* phys = pEntity->GetPhysics();
    if (phys)
    {
        pe_status_dynamics  dyn;
        phys->GetStatus(&dyn);
        velocity = dyn.v;
    }
#endif // ENABLE_CRY_PHYSICS
    return pH->EndFunction(velocity.GetLength());
}


/*! Get the mass of the entity
@return mass of the entity;
*/
int CScriptBind_Entity::GetMass(IFunctionHandler* pH)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;
    SCRIPT_CHECK_PARAMETERS(0);
    IPhysicalEntity* phys = pEntity->GetPhysics();
    float mass = 0;
    if (phys)
    {
        pe_status_dynamics  dyn;
        phys->GetStatus(&dyn);
        mass = dyn.mass;
    }

    return pH->EndFunction(mass);
#else
    return pH->EndFunction(0);
#endif // ENABLE_CRY_PHYSICS
}


int CScriptBind_Entity::GetVolume(IFunctionHandler* pH, int slot)
{
    GET_ENTITY;

    float volume = 0.0f;
#if ENABLE_CRY_PHYSICS
    IPhysicalEntity* pPhysics = pEntity->GetPhysics();

    if (pPhysics)
    {
        pe_params_part part;
        part.ipart = slot;

        if (pPhysics->GetParams(&part) && part.pPhysGeom)
        {
            volume = part.pPhysGeom->V;
        }
    }
#endif // ENABLE_CRY_PHYSICS

    return pH->EndFunction(volume);
}

int CScriptBind_Entity::GetGravity(IFunctionHandler* pH)
{
    GET_ENTITY;
    SCRIPT_CHECK_PARAMETERS(1);

    Vec3 gravity(0.0f, 0.0f, -9.8f);
#if ENABLE_CRY_PHYSICS
    pe_params_buoyancy pb;
    gEnv->pPhysicalWorld->CheckAreas(pEntity->GetWorldPos(), gravity, &pb);
#endif // ENABLE_CRY_PHYSICS

    return pH->EndFunction(Script::SetCachedVector(gravity, pH, 1));
}


int CScriptBind_Entity::GetSubmergedVolume(IFunctionHandler* pH, int slot, Vec3 planeNormal, Vec3 planeOrigin)
{
    GET_ENTITY;

    float volume = 0.0f;
#if ENABLE_CRY_PHYSICS
    IPhysicalEntity* pPhysics = pEntity->GetPhysics();

    if (pPhysics)
    {
        pe_params_part part;
        part.ipart = slot;

        if (pPhysics->GetParams(&part) && part.pPhysGeom)
        {
            Vec3 com(0, 0, 0);
            primitives::plane plane;
            geom_world_data gwd;

            plane.n = planeNormal.normalized();
            plane.origin = planeOrigin;

            pe_status_pos pos;
            pos.ipart = slot;
            pos.pMtx3x3 = &gwd.R;

            if (pPhysics->GetStatus(&pos))
            {
                gwd.scale = pos.scale;
                gwd.offset = pos.pos;
                volume = part.pPhysGeom->pGeom->CalculateBuoyancy(&plane, &gwd, com);
            }
        }
    }
#endif // ENABLE_CRY_PHYSICS

    return pH->EndFunction(volume);
}


IEntityLink* GetLink(IEntityLink* pFirst, const char* name, int ith = 0)
{
    int i = 0;
    while (pFirst)
    {
        if (!azstricmp(name, pFirst->name) && ith == i++)
        {
            break;
        }
        pFirst = pFirst->next;
    }

    return pFirst;
}

int CScriptBind_Entity::CreateLink(IFunctionHandler* pH, const char* name)
{
    GET_ENTITY;

    EntityId targetId = 0;
    if (pH->GetParamCount() > 1 && pH->GetParamType(2) == svtPointer)
    {
        ScriptHandle target;
        pH->GetParam(2, target);
        targetId = (EntityId)target.n;
    }

    pEntity->AddEntityLink(name, targetId);

    return pH->EndFunction();
}

int CScriptBind_Entity::GetLinkName(IFunctionHandler* pH, ScriptHandle targetId)
{
    GET_ENTITY;

    IEntityLink* pLink = pEntity->GetEntityLinks();

    int ith = 0;
    if (pH->GetParamCount() > 1 && pH->GetParamType(2) == svtNumber)
    {
        pH->GetParam(2, ith);
    }

    int i = 0;
    while (pLink)
    {
        if (((EntityId)targetId.n) == pLink->entityId && ith == i++)
        {
            break;
        }
        pLink = pLink->next;
    }

    if (pLink)
    {
        return pH->EndFunction(pLink->name);
    }

    return pH->EndFunction();
}

int CScriptBind_Entity::SetLinkTarget(IFunctionHandler* pH, const char* name, ScriptHandle targetId)
{
    GET_ENTITY;

    int ith = 0;
    if (pH->GetParamCount() > 2 && pH->GetParamType(3) == svtNumber)
    {
        pH->GetParam(3, ith);
    }

    IEntityLink* pLink = ::GetLink(pEntity->GetEntityLinks(), name, ith);
    if (pLink)
    {
        pLink->entityId = (EntityId)targetId.n;
    }

    return pH->EndFunction();
}

int CScriptBind_Entity::GetLinkTarget(IFunctionHandler* pH, const char* name)
{
    GET_ENTITY;

    int ith = 0;
    if (pH->GetParamCount() > 1 && pH->GetParamType(2) == svtNumber)
    {
        pH->GetParam(2, ith);
    }

    IEntityLink* pLink = ::GetLink(pEntity->GetEntityLinks(), name, ith);
    if (pLink)
    {
        IEntity* pLinkedEntity = m_pEntitySystem->GetEntity(pLink->entityId);
        if (pLinkedEntity)
        {
            return pH->EndFunction(pLinkedEntity->GetScriptTable());
        }
    }

    return pH->EndFunction();
}

int CScriptBind_Entity::RemoveLink(IFunctionHandler* pH, const char* name)
{
    GET_ENTITY;

    int ith = 0;
    if (pH->GetParamCount() > 1 && pH->GetParamType(2) == svtNumber)
    {
        pH->GetParam(2, ith);
    }

    IEntityLink* pLink = ::GetLink(pEntity->GetEntityLinks(), name, ith);
    if (pLink)
    {
        pEntity->RemoveEntityLink(pLink);
    }

    return pH->EndFunction();
}

int CScriptBind_Entity::RemoveAllLinks(IFunctionHandler* pH)
{
    GET_ENTITY;

    pEntity->RemoveAllEntityLinks();

    return pH->EndFunction();
}

int CScriptBind_Entity::GetLink(IFunctionHandler* pH, int ith)
{
    GET_ENTITY;

    int i = 0;
    IEntityLink* pLink = pEntity->GetEntityLinks();
    while (pLink)
    {
        if (ith == i++)
        {
            break;
        }
        pLink = pLink->next;
    }

    if (pLink)
    {
        IEntity* pLinkedEntity = m_pEntitySystem->GetEntity(pLink->entityId);
        if (pLinkedEntity)
        {
            return pH->EndFunction(pLinkedEntity->GetScriptTable());
        }
    }
    //      return pH->EndFunction(ScriptHandle(pLink->entityId));
    return pH->EndFunction();
}

int CScriptBind_Entity::CountLinks(IFunctionHandler* pH)
{
    GET_ENTITY;

    int n = 0;
    IEntityLink* pLink = pEntity->GetEntityLinks();
    while (pLink)
    {
        pLink = pLink->next;
        ++n;
    }

    return pH->EndFunction(n);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetTimeSinceLastSeen(IFunctionHandler* pH)
{
    GET_ENTITY

    IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();

    return pH->EndFunction(pRenderComponent ? gEnv->pTimer->GetCurrTime() - pRenderComponent->GetLastSeenTime() : gEnv->pTimer->GetCurrTime());
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetViewDistanceMultiplier(IFunctionHandler* pH)
{
    GET_ENTITY
    float value = 0.0f;
    IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        IRenderNode* pRenderNode = pRenderComponent->GetRenderNode();
        if (pRenderNode)
        {
            value = pRenderNode->GetViewDistanceMultiplier();
        }
    }
    return pH->EndFunction(value);
}

//////////////////////////////////////////////////////////////////////////
//! 0..254 (value is autmatically clamed to to this range)
int CScriptBind_Entity::SetViewDistanceMultiplier(IFunctionHandler* pH)
{
    GET_ENTITY
        SCRIPT_CHECK_PARAMETERS(1);
    float value;
    pH->GetParam(1, value);

    IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        IRenderNode* pRenderNode = pRenderComponent->GetRenderNode();
        if (pRenderNode)
        {
            pRenderNode->SetViewDistanceMultiplier(value);
        }
    }

    return pH->EndFunction();
}


//////////////////////////////////////////////////////////////////////////
//!  do not fade out object - no matter how far away it is
int CScriptBind_Entity::SetViewDistUnlimited(IFunctionHandler* pH)
{
    GET_ENTITY;
    IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        IRenderNode* pRenderNode = pRenderComponent->GetRenderNode();
        if (pRenderNode)
        {
            pRenderNode->SetViewDistUnlimited();
        }
    }
    return pH->EndFunction();
}


//////////////////////////////////////////////////////////////////////////
//! 0..254 (value is autmatically clamed to to this range)
int CScriptBind_Entity::SetLodRatio(IFunctionHandler* pH)
{
    GET_ENTITY
        SCRIPT_CHECK_PARAMETERS(1);
    int value;
    pH->GetParam(1, value);

    IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        IRenderNode* pRenderNode = pRenderComponent->GetRenderNode();
        if (pRenderNode)
        {
            pRenderNode->SetLodRatio(value);
        }
    }

    return pH->EndFunction();
}


//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetLodRatio(IFunctionHandler* pH)
{
    GET_ENTITY
    int value = 0;
    IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        IRenderNode* pRenderNode = pRenderComponent->GetRenderNode();
        if (pRenderNode)
        {
            value = pRenderNode->GetLodRatio();
        }
    }
    return pH->EndFunction(value);
}


//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::RemoveDecals(IFunctionHandler* pH)
{
    GET_ENTITY;
    IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        IRenderNode* pRenderNode = pRenderComponent->GetRenderNode();
        if (pRenderNode)
        {
            gEnv->p3DEngine->DeleteEntityDecals(pRenderNode);
        }
    }

    return pH->EndFunction();
}

int CScriptBind_Entity::EnableDecals(IFunctionHandler* pH, int slot, bool enable)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(slot);
    if (pCharacter)
    {
        pCharacter->EnableDecals(enable ? 1 : 0);
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::ForceCharacterUpdate(IFunctionHandler* pH, int characterSlot, bool updateAlways)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);
    if (pCharacter)
    {
        ISkeletonAnim* pSkeletonAnim = pCharacter->GetISkeletonAnim();
        ISkeletonPose* pSkeletonPose = pCharacter->GetISkeletonPose();
        {
            //IMPORTANT: SetForceSkeletonUpdate() is setting a frame-counter for the amount of skeleton updates.
            //If we want a permanent update then we have to use 0x8000.
            //This is of course a really inefficient way force skeleton updates, because it will do updates even if the bones are not changing.
            //A more elegant solution is to attach the flag (CA_FORCE_SKELETON_UPDATE) directly to the animation. In that case we would update the
            //skeleton only when we play the animation and then we disable it automatically
            //      pSkeleton->SetForceSkeletonUpdate(updateAlways ? 0x8000:0);
            pSkeletonPose->SetForceSkeletonUpdate(0);

            for (uint32 layer = 0; layer < ISkeletonAnim::LayerCount; layer++)
            {
                int count = pSkeletonAnim->GetNumAnimsInFIFO(layer);
                if (count > 0)
                {
                    CAnimation& anim = pSkeletonAnim->GetAnimFromFIFO(layer, 0);
                    if (updateAlways)
                    {
                        anim.SetStaticFlag(CA_FORCE_SKELETON_UPDATE);
                    }
                }
            }
        }
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::CharacterUpdateAlways(IFunctionHandler* pH, int characterSlot, bool updateAlways)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);

    if (pCharacter)
    {
        pCharacter->SetFlags(pCharacter->GetFlags() | CS_FLAG_UPDATE_ALWAYS);
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::CharacterUpdateOnRender(IFunctionHandler* pH, int characterSlot, bool bUpdateOnRender)
{
    GET_ENTITY;

    ICharacterInstance* pCharacter = pEntity->GetCharacter(characterSlot);
    if (pCharacter)
    {
        if (bUpdateOnRender)
        {
            pCharacter->SetFlags(pCharacter->GetFlags() | CS_FLAG_UPDATE);
        }
        else
        {
            pCharacter->SetFlags(pCharacter->GetFlags() & (~CS_FLAG_UPDATE));
        }
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::SetAnimateOffScreenShadow(IFunctionHandler* pH, bool bAnimateOffScreenShadow)
{
    GET_ENTITY;

    IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
    if (pRenderComponent)
    {
        if (bAnimateOffScreenShadow)
        {
            pRenderComponent->SetFlags(pRenderComponent->GetFlags() | CComponentRender::FLAG_ANIMATE_OFFSCREEN_SHADOW);
        }
        else
        {
            pRenderComponent->SetFlags(pRenderComponent->GetFlags() & (~CComponentRender::FLAG_ANIMATE_OFFSCREEN_SHADOW));
        }
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::Hide(IFunctionHandler* pH)
{
    GET_ENTITY;
    SCRIPT_CHECK_PARAMETERS(1);
    int hide = 0;
    pH->GetParam(1, hide);
    pEntity->Hide(hide != 0);
    return pH->EndFunction();
}


//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::CheckCollisions(IFunctionHandler* pH)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;
    assert(pH->GetParamCount() <= 2);
    int iEntTypes = ent_sleeping_rigid | ent_rigid | ent_living, iCollTypes = -1;
    pH->GetParam(1, iEntTypes);
    pH->GetParam(2, iCollTypes);

    IPhysicalEntity** ppEnts, * pEnt = pEntity->GetPhysics();
    if (pEnt)
    {
        int nEnts, i, nParts, nCont, nTotCont, nEntCont, nContactEnts;
        pe_params_bbox pbb;
        pe_params_foreign_data pfd;
        pe_status_pos sp[2];
        pe_params_part pp[2];
        geom_world_data gwd[2];
        intersection_params ip;
        geom_contact* pContacts;
        IEntity* pIEnt;
        SmartScriptTable psoRes(m_pSS), psoContactList(m_pSS), psoEntList(m_pSS);
        IScriptTable* psoEnt, * psoNormals[32], * psoCenters[32], * psoContacts[32];

        pEnt->GetParams(&pbb);
        pEnt->GetParams(&pfd);
        pe_status_nparts statusTmp;
        nParts = pEnt->GetStatus(&statusTmp);
        pEnt->GetStatus(sp + 0);
        ip.bNoAreaContacts = true;
        ip.vrel_min = 1E10f;

        nEnts = m_pISystem->GetIPhysicalWorld()->GetEntitiesInBox(pbb.BBox[0], pbb.BBox[1], ppEnts, iEntTypes);
        nContactEnts = nTotCont = 0;
        for (i = 0; i < nEnts; i++)
        {
            if (ppEnts[i] != pEnt && !(pfd.pForeignData && ppEnts[i]->GetForeignData(pfd.iForeignData) == pfd.pForeignData))
            {
                ppEnts[i]->GetStatus(sp + 1);
                psoEnt = (pIEnt = m_pEntitySystem->GetEntityFromPhysics(ppEnts[i])) ? pIEnt->GetScriptTable() : 0;
                nEntCont = 0;

                pe_status_nparts statusTmp1;
                for (pp[1].ipart = ppEnts[i]->GetStatus(&statusTmp1) - 1; pp[1].ipart >= 0; pp[1].ipart--)
                {
                    MARK_UNUSED(pp[1].partid);
                    ppEnts[i]->GetParams(pp + 1);
                    gwd[1].offset = sp[1].pos + sp[1].q * pp[1].pos;
                    gwd[1].R = Matrix33(sp[1].q * pp[1].q);
                    gwd[1].scale = pp[1].scale;
                    for (pp[0].ipart = 0; pp[0].ipart < nParts; pp[0].ipart++)
                    {
                        MARK_UNUSED(pp[0].partid);
                        pEnt->GetParams(pp + 0);
                        if ((iCollTypes == -1 ? pp[0].flagsColliderOR : iCollTypes) & pp[1].flagsOR)
                        {
                            gwd[0].offset = sp[0].pos + sp[0].q * pp[0].pos;
                            gwd[0].R = Matrix33(sp[0].q * pp[0].q);
                            gwd[0].scale = pp[0].scale;
                            {
                                WriteLockCond lock;
                                for (nCont = pp[0].pPhysGeomProxy->pGeom->IntersectLocked(pp[1].pPhysGeomProxy->pGeom, gwd + 0, gwd + 1, &ip, pContacts, lock) - 1;
                                     nCont >= 0 && nTotCont < sizeof(psoContacts) / sizeof(psoContacts[0]); nCont--)
                                {
                                    psoCenters[nTotCont] = m_pSS->CreateTable();
                                    psoCenters[nTotCont]->AddRef();
                                    psoCenters[nTotCont]->BeginSetGetChain();
                                    psoCenters[nTotCont]->SetValueChain("x", pContacts[nCont].center.x);
                                    psoCenters[nTotCont]->SetValueChain("y", pContacts[nCont].center.y);
                                    psoCenters[nTotCont]->SetValueChain("z", pContacts[nCont].center.z);
                                    psoCenters[nTotCont]->EndSetGetChain();

                                    psoNormals[nTotCont] = m_pSS->CreateTable();
                                    psoNormals[nTotCont]->AddRef();
                                    psoNormals[nTotCont]->BeginSetGetChain();
                                    psoNormals[nTotCont]->SetValueChain("x", -pContacts[nCont].n.x);
                                    psoNormals[nTotCont]->SetValueChain("y", -pContacts[nCont].n.y);
                                    psoNormals[nTotCont]->SetValueChain("z", -pContacts[nCont].n.z);
                                    psoNormals[nTotCont]->EndSetGetChain();

                                    psoContacts[nTotCont] = m_pSS->CreateTable();
                                    psoContacts[nTotCont]->AddRef();
                                    psoContacts[nTotCont]->BeginSetGetChain();
                                    psoContacts[nTotCont]->SetValueChain("center", psoCenters[nTotCont]);
                                    psoContacts[nTotCont]->SetValueChain("normal", psoNormals[nTotCont]);
                                    psoContacts[nTotCont]->SetValueChain("partid0", pp[0].partid);
                                    psoContacts[nTotCont]->SetValueChain("partid1", pp[1].partid);
                                    if (psoEnt)
                                    {
                                        psoContacts[nTotCont]->SetValueChain("collider", psoEnt);
                                    }
                                    //else
                                    //psoContacts[nTotCont]->SetToNullChain("collider");
                                    psoContacts[nTotCont]->EndSetGetChain();

                                    psoContactList->SetAt(nTotCont + 1, psoContacts[nTotCont]);
                                    nTotCont++;
                                    nEntCont++;
                                }
                            } // lock
                        }
                    }
                }

                if (nEntCont && psoEnt)
                {
                    psoEntList->SetAt(nContactEnts++ + 1, psoEnt);
                }
            }
        }

        psoRes->SetValue("contacts", psoContactList);
        psoRes->SetValue("entities", psoEntList);
        for (i = 0; i < nTotCont; i++)
        {
            psoNormals[i]->Release(), psoCenters[i]->Release(), psoContacts[i]->Release();
        }

        return pH->EndFunction(psoRes);
    }
#endif // ENABLE_CRY_PHYSICS

    return pH->EndFunction();
}


int CScriptBind_Entity::AwakeEnvironment(IFunctionHandler* pH)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;
    pe_params_bbox pbb;
    pe_action_awake aa;
    IPhysicalEntity** ppEnts;
    Vec3 vDelta;
    int i, nEnts;

    AABB bbox;
    IComponentPhysicsPtr pPhysicsComponent = pEntity->GetComponent<IComponentPhysics>();
    if (pPhysicsComponent)
    {
        pPhysicsComponent->GetWorldBounds(bbox);
    }
    else
    {
        pH->EndFunction();
    }

    Vec3 vMin = bbox.min;
    Vec3 vMax = bbox.max;
    vDelta.x = vDelta.y = vDelta.z = m_pISystem->GetIPhysicalWorld()->GetPhysVars()->maxContactGap * 4;
    nEnts = m_pISystem->GetIPhysicalWorld()->GetEntitiesInBox(vMin - vDelta, vMax + vDelta, ppEnts, ent_sleeping_rigid | ent_living | ent_independent);
    for (i = 0; i < nEnts; i++)
    {
        ppEnts[i]->Action(&aa);
    }
#endif // ENABLE_CRY_PHYSICS

    return pH->EndFunction();
}

int CScriptBind_Entity::NoExplosionCollision(IFunctionHandler* pH)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;
    SCRIPT_CHECK_PARAMETERS(0);

    IPhysicalEntity* pPhysicalEntity = pEntity->GetPhysics();
    if (pPhysicalEntity)
    {
        pe_params_part ppart;
        ppart.flagsAND = ~geom_colltype_explosion;
        ppart.ipart = -1;
        do
        {
            ++ppart.ipart;
        } while (pPhysicalEntity->SetParams(&ppart));
    }
#endif // ENABLE_CRY_PHYSICS
    return pH->EndFunction();
}

int CScriptBind_Entity::NoBulletForce(IFunctionHandler* pH, bool state)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;
    //  SCRIPT_CHECK_PARAMETERS(0);

    IPhysicalEntity* pPhysicalEntity = pEntity->GetPhysics();
    if (pPhysicalEntity)
    {
        if (state)
        {
            pe_params_part ppart;
            ppart.flagsColliderOR = geom_no_particle_impulse;
            ppart.ipart = -1;
            do
            {
                ++ppart.ipart;
            } while (pPhysicalEntity->SetParams(&ppart));
        }
        else
        {
            pe_params_part ppart;
            ppart.flagsColliderAND = ~geom_no_particle_impulse;
            ppart.ipart = -1;
            do
            {
                ++ppart.ipart;
            } while (pPhysicalEntity->SetParams(&ppart));
        }
    }
#endif // ENABLE_CRY_PHYSICS
    return pH->EndFunction();
}

//get some physical infos such mass and gravity
int CScriptBind_Entity::GetPhysicalStats(IFunctionHandler* pH)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;
    IPhysicalEntity* pEnt = pEntity->GetPhysics();

    if (pEnt)
    {
        pe_player_dynamics movedyn;

        float gravity = -9.81f;
        float mass = 1.0f;

        if (pEnt->GetParams(&movedyn))
        {
            gravity = movedyn.gravity.z;
            mass = movedyn.mass;
        }

        pe_simulation_params sp;

        if (pEnt->GetParams(&sp))
        {
            gravity = sp.gravity.z;
            mass = sp.mass;
        }

        pe_params_particle pp;

        if (pEnt->GetParams(&pp))
        {
            gravity = pp.gravity.z;
            mass = pp.mass;
        }

        int flags;
        pe_params_flags fp;
        if (pEnt->GetParams(&fp))
        {
            flags = fp.flags;
        }

        m_pStats->SetValue("gravity", gravity);
        m_pStats->SetValue("mass", mass);
        m_pStats->SetValue("flags", flags);

        return pH->EndFunction(*m_pStats);
    }
    else
#endif // ENABLE_CRY_PHYSICS
    {
        return pH->EndFunction();
    }
}

int CScriptBind_Entity::UpdateAreas(IFunctionHandler* pH)
{
    GET_ENTITY;

    static_cast<CAreaManager*>(m_pEntitySystem->GetAreaManager())->MarkEntityForUpdate(pEntity->GetId());

    return pH->EndFunction();
}

int CScriptBind_Entity::IsPointInsideArea(IFunctionHandler* pH, int areaId, Vec3 point)
{
    GET_ENTITY;

    static std::vector<CArea*> areas;
    areas.resize(0);
    EntityId const nEntityID = pEntity->GetId();

    if (static_cast<CAreaManager*>(m_pEntitySystem->GetAreaManager())->GetLinkedAreas(nEntityID, areaId, areas))
    {
        for (unsigned int i = 0; i < areas.size(); i++)
        {
            CArea* pArea = areas[i];
            if (pArea->CalcPointWithin(nEntityID, point))
            {
                return pH->EndFunction(1);
            }
        }
    }

    return pH->EndFunction();
}

int CScriptBind_Entity::IsEntityInsideArea(IFunctionHandler* pH, int areaId, ScriptHandle entityId)
{
    GET_ENTITY;

    IEntity* pTarget = m_pEntitySystem->GetEntity((EntityId)entityId.n);
    if (!pTarget)
    {
        return pH->EndFunction();
    }

    static std::vector<CArea*> areas;
    areas.resize(0);

    Vec3 points[9];
    points[0] = pTarget->GetWorldPos();

    AABB aabb;
    pTarget->GetLocalBounds(aabb);
    Matrix33 m33(pTarget->GetRotation());

    points[1] = m33 * Vec3(aabb.min.x, aabb.min.y, aabb.max.z) + points[0];
    points[2] = m33 * Vec3(aabb.min.x, aabb.max.y, aabb.max.z) + points[0];
    points[3] = m33 * Vec3(aabb.max.x, aabb.max.y, aabb.max.z) + points[0];
    points[4] = m33 * Vec3(aabb.max.x, aabb.min.y, aabb.max.z) + points[0];

    points[5] = m33 * Vec3(aabb.min.x, aabb.min.y, aabb.min.z) + points[0];
    points[6] = m33 * Vec3(aabb.min.x, aabb.max.y, aabb.min.z) + points[0];
    points[7] = m33 * Vec3(aabb.max.x, aabb.max.y, aabb.min.z) + points[0];
    points[8] = m33 * Vec3(aabb.max.x, aabb.min.y, aabb.min.z) + points[0];

    EntityId const nEntityID = pEntity->GetId();

    if (static_cast<CAreaManager*>(m_pEntitySystem->GetAreaManager())->GetLinkedAreas(nEntityID, areaId, areas))
    {
        for (unsigned int i = 0; i < areas.size(); i++)
        {
            CArea* pArea = areas[i];

            for (int e = 0; e < 9; e++)
            {
                if (pArea->CalcPointWithin(nEntityID, points[e]))
                {
                    return pH->EndFunction(1);
                }
            }
        }
    }

    return pH->EndFunction();
}

bool CScriptBind_Entity::ParseLightParams(IScriptTable* pLightTable, CDLight& light)
{
    light.m_nLightStyle = 0;
    light.SetPosition(Vec3(ZERO));
    light.m_fLightFrustumAngle = 45.0f;
    light.m_fRadius = 4.0f;
    light.m_Flags = 0;
    light.m_LensOpticsFrustumAngle = 255;

    CScriptSetGetChain chain(pLightTable);
    int nLightStyle = 0;
    if (chain.GetValue("style", nLightStyle))
    {
        light.m_nLightStyle = (uint8) nLightStyle;
    }

    float fAnimSpeed = 0.0f;
    chain.GetValue("anim_speed", fAnimSpeed);
    light.SetAnimSpeed(fAnimSpeed);

    int nAnimPhase = 0;
    if (chain.GetValue("anim_phase", nAnimPhase))
    {
        light.m_nLightPhase = (uint8) nAnimPhase;
    }

    chain.GetValue("radius", light.m_fRadius);

    light.m_ProbeExtents = Vec3(light.m_fRadius, light.m_fRadius, light.m_fRadius);

    float boxSizeX, boxSizeY, boxSizeZ;
    if (chain.GetValue("box_size_x", boxSizeX))
    {
        light.m_ProbeExtents.x = boxSizeX * 0.5f;
    }
    if (chain.GetValue("box_size_y", boxSizeY))
    {
        light.m_ProbeExtents.y = boxSizeY * 0.5f;
    }
    if (chain.GetValue("box_size_z", boxSizeZ))
    {
        light.m_ProbeExtents.z = boxSizeZ * 0.5f;
    }

    const char* specularCubemap = 0;
    if (chain.GetValue("deferred_cubemap", specularCubemap))
    {
        light.ReleaseCubemaps();

        if (specularCubemap && strlen(specularCubemap) > 0)
        {
            string sSpecularName(specularCubemap);
            int strIndex = sSpecularName.find("_diff");
            if (strIndex >= 0)
            {
                sSpecularName = sSpecularName.substr(0, strIndex) + sSpecularName.substr(strIndex + 5, sSpecularName.length());
                specularCubemap = sSpecularName.c_str();
            }

            char diffuseCubemap[ICryPak::g_nMaxPath];
            azsnprintf(diffuseCubemap, sizeof(diffuseCubemap), "%s%s%s.%s", PathUtil::AddSlash(PathUtil::GetPath(specularCubemap)).c_str(),
                PathUtil::GetFileName(specularCubemap).c_str(), "_diff", PathUtil::GetExt(specularCubemap));

            // '\\' in filename causing texture duplication
            string specularCubemapUnix = PathUtil::ToUnixPath(specularCubemap);
            string diffuseCubemapUnix = PathUtil::ToUnixPath(diffuseCubemap);

            light.SetSpecularCubemap(gEnv->pRenderer->EF_LoadCubemapTexture(specularCubemapUnix.c_str(), FT_DONT_STREAM));
            light.SetDiffuseCubemap(gEnv->pRenderer->EF_LoadCubemapTexture(diffuseCubemapUnix.c_str(), FT_DONT_STREAM));

            if (!light.GetSpecularCubemap() || !light.GetSpecularCubemap()->IsTextureLoaded())
            {
                GetISystem()->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, 0, specularCubemap,
                    "Deferred cubemap texture not found: %s", specularCubemap);

                light.ReleaseCubemaps();
            }
            if (!light.GetDiffuseCubemap() || !light.GetDiffuseCubemap()->IsTextureLoaded())
            {
                GetISystem()->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, 0, diffuseCubemap,
                    "Deferred diffuse cubemap texture not found: %s", diffuseCubemap);

                light.ReleaseCubemaps();
            }

            if (light.GetDiffuseCubemap() && light.GetSpecularCubemap())
            {
                light.m_Flags |= DLF_DEFERRED_CUBEMAPS;
            }
            else
            {
                light.m_Flags &= ~DLF_DEFERRED_CUBEMAPS;
                light.m_Flags |= DLF_DISABLED;
                light.ReleaseCubemaps();
            }
        }
    }

    bool flag;
    if (chain.GetValue("this_area_only", flag) && flag)
    {
        light.m_Flags |= DLF_THIS_AREA_ONLY;
    }
    if (chain.GetValue("ignore_visareas", flag) && flag)
    {
        light.m_Flags |= DLF_IGNORES_VISAREAS;
    }
    if (chain.GetValue("fake", flag) && flag)
    {
        light.m_Flags |= DLF_FAKE;
    }
    if (chain.GetValue("indoor_only", flag) && flag)
    {
        light.m_Flags |= DLF_INDOOR_ONLY;
    }
    if (chain.GetValue("ambient", flag) && flag)
    {
        light.m_Flags |= DLF_AMBIENT;
    }
    if (chain.GetValue("area_light", flag) && flag)
    {
        light.m_Flags |= DLF_AREA_LIGHT;
    }
    if (chain.GetValue("deferred_light", flag) && flag)
    {
        light.m_Flags |= DLF_DEFERRED_LIGHT;
    }
    if (chain.GetValue("volumetric_fog", flag) && flag)
    {
        light.m_Flags |= DLF_VOLUMETRIC_FOG;
    }
    if (chain.GetValue("volumetric_fog_only", flag) && flag)
    {
        light.m_Flags |= DLF_VOLUMETRIC_FOG_ONLY;
    }
    Vec3 color;
    if (chain.GetValue("diffuse_color", color))
    {
        light.SetLightColor(ColorF(color.x, color.y, color.z, 1.0f));
    }
    light.SetSpecularMult(1.0f);
    float fMult;
    if (chain.GetValue("specular_multiplier", fMult))
    {
        light.SetSpecularMult(fMult);
    }

    int castShadowSpec;
    if (chain.GetValue("cast_shadow", castShadowSpec))
    {
        if (castShadowSpec != 0)
        {
            int configSpec = CVar::pSysSpecLight ? CVar::pSysSpecLight->GetIVal() : gEnv->pSystem->GetConfigSpec(true);

            if (configSpec == CONFIG_AUTO_SPEC) // TODO: improve this case (is there a shadow/light spec to check? )
            {
                configSpec = CONFIG_MEDIUM_SPEC;
            }

            if (castShadowSpec <= configSpec) // Only cast shadows when quality spec. specified is lower or equal to current spec.
            {
                light.m_Flags |= DLF_CASTSHADOW_MAPS;
            }
        }
    }

    float fAreaWidth = 0;
    if (chain.GetValue("area_width", fAreaWidth))
    {
        light.m_fAreaWidth = fAreaWidth;
    }

    float fAreaHeight = 0;
    if (chain.GetValue("area_height", fAreaHeight))
    {
        light.m_fAreaHeight = fAreaHeight;
    }

    bool bBoxProjected = false;
    if (chain.GetValue("box_projection", bBoxProjected) && bBoxProjected)
    {
        light.m_Flags |= DLF_BOX_PROJECTED_CM;
    }

    float fBoxWidth = 0;
    if (chain.GetValue("box_width", fBoxWidth))
    {
        light.m_fBoxWidth = fBoxWidth;
    }

    float fBoxHeight = 0;
    if (chain.GetValue("box_height", fBoxHeight))
    {
        light.m_fBoxHeight = fBoxHeight;
    }

    float fBoxLength = 0;
    if (chain.GetValue("box_length", fBoxLength))
    {
        light.m_fBoxLength = fBoxLength;
    }

    float fShadowBias = 1.0f;
    float fShadowSlopeBias = 1.0f;
    if (chain.GetValue("shadow_bias", fShadowBias) && chain.GetValue("shadow_slope_bias", fShadowSlopeBias))
    {
        light.SetShadowBiasParams(fShadowBias, fShadowSlopeBias);
    }

    f32 fShadowResolutionScale = light.m_fShadowResolutionScale;
    if (chain.GetValue("shadowResolutionScale", fShadowResolutionScale))
    {
        light.m_fShadowResolutionScale = fShadowResolutionScale;
    }

    uint32 nShadowMinResolution = light.m_nShadowMinResolution;
    if (chain.GetValue("shadowMinResolution", nShadowMinResolution))
    {
        light.m_nShadowMinResolution = nShadowMinResolution;
    }

    float fShadowUpdate_MinRadius = light.m_fRadius;
    if (chain.GetValue("shadowUpdate_MinRadius", fShadowUpdate_MinRadius))
    {
        light.m_fShadowUpdateMinRadius = fShadowUpdate_MinRadius;
    }

    float fShadowUpdateRatio = 1.0f;
    if (chain.GetValue("shadowUpdate_ratio", fShadowUpdateRatio))
    {
        light.m_nShadowUpdateRatio = max((uint16)1, (uint16)(fShadowUpdateRatio * (1 << DL_SHADOW_UPDATE_SHIFT)));
    }

    light.m_fHDRDynamic = 0.0f;
    chain.GetValue("hdrdyn", light.m_fHDRDynamic);

    int nSortPriority = 0;
    chain.GetValue("sort_priority", nSortPriority);
    light.m_nSortPriority = nSortPriority;

    float fAttenFalloffMax = 1.0f;
    chain.GetValue("attenuation_falloff_max", fAttenFalloffMax);
    light.SetFalloffMax(fAttenFalloffMax);

    float fAttenuationBulbSize = 0.0f;
    if (chain.GetValue("attenuation_bulbsize", fAttenuationBulbSize))
    {
        light.m_fAttenuationBulbSize = fAttenuationBulbSize;
    }

    float fFogRadialLobe = 0.0f;
    if (chain.GetValue("fog_radial_lobe", fFogRadialLobe))
    {
        light.m_fFogRadialLobe = fFogRadialLobe;
    }

    const char* flareName;
    if (chain.GetValue("flare_Flare", flareName) && flareName[0])
    {
        int nLensOpticsID(0);
        if (gEnv->pOpticsManager->Load(flareName, nLensOpticsID))
        {
            bool bEnableLensOptics = false;
            chain.GetValue("flare_enable", bEnableLensOptics);
            if (bEnableLensOptics)
            {
                IOpticsElementBase* pOptics = gEnv->pOpticsManager->GetOptics(nLensOpticsID);
                if (pOptics)
                {
                    light.SetLensOpticsElement(pOptics);
                    float fLensOpticsFOV(360.0f);
                    if (chain.GetValue("flare_FOV", fLensOpticsFOV))
                    {
                        if (fLensOpticsFOV == 0)
                        {
                            light.m_LensOpticsFrustumAngle = 0;
                        }
                        else
                        {
                            int modularAngle = ((int)fLensOpticsFOV) % 360;
                            if (modularAngle == 0)
                            {
                                light.m_LensOpticsFrustumAngle = 255;
                            }
                            else
                            {
                                light.m_LensOpticsFrustumAngle = (uint8)(fLensOpticsFOV * (255.0f / 360.0f));
                            }
                        }
                    }
                }
            }
            else
            {
                light.SetLensOpticsElement(NULL);
            }
        }
        else if (flareName && flareName[0] && strcmp(flareName, "@root"))
        {
            CryLog("[WARNING] flare lens optics '%s' in light '%s' doesn't exist!", flareName, light.m_sName);
        }
    }

    // Always visible and no actual lighting when attached to sun
    bool bAttachToSun = 0;
    if (chain.GetValue("attach_to_sun", bAttachToSun) && bAttachToSun)
    {
        light.m_Flags |= DLF_ATTACH_TO_SUN | DLF_FAKE | DLF_IGNORES_VISAREAS;
        light.m_Flags &= ~DLF_THIS_AREA_ONLY;
    }


    //////////////////////////////////////////////////////////////////////////
    //load projectors params
    if (chain.GetValue("proj_fov", light.m_fLightFrustumAngle))
    {
        light.m_fLightFrustumAngle *= 0.5;
    }

    chain.GetValue("proj_nearplane", light.m_fProjectorNearPlane);

    const char* projectorTexture = 0;
    if (chain.GetValue("projector_texture", projectorTexture))
    {
        SAFE_RELEASE(light.m_pLightImage);

        if (projectorTexture && strlen(projectorTexture) > 0)
        {
            int flags = FT_DONT_STREAM;
            bool bProjectAllSides = false;
            
            light.m_pLightImage = gEnv->pRenderer->EF_LoadTexture(projectorTexture, flags);

            if (!light.m_pLightImage || !light.m_pLightImage->IsTextureLoaded())
            {
                GetISystem()->Warning(VALIDATOR_MODULE_ENTITYSYSTEM, VALIDATOR_WARNING, 0, projectorTexture,
                    "Light projector texture not found: %s", projectorTexture);
                light.m_pLightImage = gEnv->pRenderer->EF_LoadTexture("Textures/defaults/red.dds", flags);
            }
        }
    }

    const char* pLightAnimationNodeName = 0;
    if (chain.GetValue("light_animation", pLightAnimationNodeName))
    {
        if (pLightAnimationNodeName && strlen(pLightAnimationNodeName) > 0)
        {
            light.m_pLightAnim = gEnv->pMovieSystem->CreateLightAnimWrapper(pLightAnimationNodeName);
        }
    }

    bool bTimeScrubbingInTrackView = false;

    if (chain.GetValue("time_scrubbing_in_trackview", bTimeScrubbingInTrackView))
    {
        if (bTimeScrubbingInTrackView && gEnv->IsEditing())
        {
            chain.GetValue("time_scrubbed", light.m_fTimeScrubbed);
        }
    }

    if (bTimeScrubbingInTrackView)
    {
        light.m_Flags |= DLF_TRACKVIEW_TIMESCRUBBING;
    }

    if (!(light.m_Flags & DLF_AREA_LIGHT) && light.m_fLightFrustumAngle && (light.m_pLightImage != NULL) && light.m_pLightImage->IsTextureLoaded())
    {
        light.m_Flags |= DLF_PROJECT;
    }
    else
    {
        SAFE_RELEASE(light.m_pLightImage);
        light.m_Flags |= DLF_POINT;
    }

    return true;
}

bool CScriptBind_Entity::ParseFogVolumesParams(IScriptTable* pTable, IEntity* pEntity, SFogVolumeProperties& properties)
{
    CScriptSetGetChain chain(pTable);

    int volumeType;
    properties.m_volumeType = 0;
    if (false != chain.GetValue("eiVolumeType", volumeType))
    {
        properties.m_volumeType = clamp_tpl(volumeType, 0, 1);
    }

    Vec3 size;
    properties.m_size = Vec3(1.0f, 1.0f, 1.0f);
    if (false != chain.GetValue("Size", size))
    {
        if (size.x <= 0)
        {
            size.x = 1e-2f;
        }
        if (size.y <= 0)
        {
            size.y = 1e-2f;
        }
        if (size.z <= 0)
        {
            size.z = 1e-2f;
        }
        properties.m_size = size;
    }

    Vec3 color;
    properties.m_color = Vec3(1.0f, 1.0f, 1.0f);
    if (false != chain.GetValue("color_Color", color))
    {
        properties.m_color = color;
    }

    bool useGlobalFogColor;
    properties.m_useGlobalFogColor = false;
    if (false != chain.GetValue("bUseGlobalFogColor", useGlobalFogColor))
    {
        properties.m_useGlobalFogColor = useGlobalFogColor;
    }

    bool ignoresVisAreas;
    properties.m_ignoresVisAreas = false;
    if (false != chain.GetValue("bIgnoresVisAreas", ignoresVisAreas))
    {
        properties.m_ignoresVisAreas = ignoresVisAreas;
    }

    bool affectsThisAreaOnly;
    properties.m_affectsThisAreaOnly = false;
    if (false != chain.GetValue("bAffectsThisAreaOnly", affectsThisAreaOnly))
    {
        properties.m_affectsThisAreaOnly = affectsThisAreaOnly;
    }

    float globalDensity;
    properties.m_globalDensity = 1.0f;
    if (false != chain.GetValue("GlobalDensity", globalDensity))
    {
        if (globalDensity < 0.0f)
        {
            globalDensity = 0.0f;
        }
        properties.m_globalDensity = globalDensity;
    }

    float densityOffset;
    properties.m_densityOffset = 0.0f;
    if (false != chain.GetValue("DensityOffset", densityOffset))
    {
        properties.m_densityOffset = densityOffset;
    }

    float nearCutoff;
    properties.m_nearCutoff = 0.0f;
    if (false != chain.GetValue("NearCutoff", nearCutoff))
    {
        properties.m_nearCutoff = nearCutoff;
    }

    float fHDRDynamic;
    properties.m_fHDRDynamic = 0.0f;
    if (false != chain.GetValue("fHDRDynamic", fHDRDynamic))
    {
        properties.m_fHDRDynamic = fHDRDynamic;
    }

    float softEdges;
    properties.m_softEdges = 1.0f;
    if (false != chain.GetValue("SoftEdges", softEdges))
    {
        properties.m_softEdges = clamp_tpl(softEdges, 0.0f, 1.0f);
    }

    float heightFallOffDirLong;
    properties.m_heightFallOffDirLong = 0.0f;
    if (false != chain.GetValue("FallOffDirLong", heightFallOffDirLong))
    {
        properties.m_heightFallOffDirLong = heightFallOffDirLong;
    }

    float heightFallOffDirLati;
    properties.m_heightFallOffDirLati = 0.0f;
    if (false != chain.GetValue("FallOffDirLati", heightFallOffDirLati))
    {
        properties.m_heightFallOffDirLati = heightFallOffDirLati;
    }

    float heightFallOffShift;
    properties.m_heightFallOffShift = 0.0f;
    if (false != chain.GetValue("FallOffShift", heightFallOffShift))
    {
        properties.m_heightFallOffShift = heightFallOffShift;
    }

    float heightFallOffScale;
    properties.m_heightFallOffScale = 1.0f;
    if (false != chain.GetValue("FallOffScale", heightFallOffScale))
    {
        properties.m_heightFallOffScale = heightFallOffScale;
    }

    float rampStart;
    properties.m_rampStart = 0.0f;
    if (false != chain.GetValue("RampStart", rampStart))
    {
        properties.m_rampStart = clamp_tpl(rampStart, 0.0f, 30000.0f);
    }

    float rampEnd;
    properties.m_rampEnd = 50.0f;
    if (false != chain.GetValue("RampEnd", rampEnd))
    {
        properties.m_rampEnd = clamp_tpl(rampEnd, 0.0f, 30000.0f);
    }

    float rampInfluence;
    properties.m_rampInfluence = 0.0f;
    if (false != chain.GetValue("RampInfluence", rampInfluence))
    {
        properties.m_rampInfluence = clamp_tpl(rampInfluence, 0.0f, 1.0f);
    }

    float windInfluence;
    properties.m_windInfluence = 1.0f;
    if (false != chain.GetValue("WindInfluence", windInfluence))
    {
        properties.m_windInfluence = clamp_tpl(windInfluence, 0.0f, 20.0f);
    }

    float densityNoiseScale;
    properties.m_densityNoiseScale = 0.0f;
    if (false != chain.GetValue("DensityNoiseScale", densityNoiseScale))
    {
        properties.m_densityNoiseScale = clamp_tpl(densityNoiseScale, 0.0f, 20.0f);
    }

    float densityNoiseOffset;
    properties.m_densityNoiseOffset = 0.0f;
    if (false != chain.GetValue("DensityNoiseOffset", densityNoiseOffset))
    {
        properties.m_densityNoiseOffset = clamp_tpl(densityNoiseOffset, -20.0f, 20.0f);
    }

    float densityNoiseTimeFrequency;
    properties.m_densityNoiseTimeFrequency = 0.0f;
    if (false != chain.GetValue("DensityNoiseTimeFrequency", densityNoiseTimeFrequency))
    {
        properties.m_densityNoiseTimeFrequency = clamp_tpl(densityNoiseTimeFrequency, 0.0f, 10.0f);
    }

    Vec3 densityNoiseFrequency;
    properties.m_densityNoiseFrequency = Vec3(1.0f, 1.0f, 1.0f);
    if (false != chain.GetValue("DensityNoiseFrequency", densityNoiseFrequency))
    {
        densityNoiseFrequency.x = clamp_tpl(densityNoiseFrequency.x, 0.01f, 10000.0f);
        densityNoiseFrequency.y = clamp_tpl(densityNoiseFrequency.y, 0.01f, 10000.0f);
        densityNoiseFrequency.z = clamp_tpl(densityNoiseFrequency.z, 0.01f, 10000.0f);
        properties.m_densityNoiseFrequency = densityNoiseFrequency;
    }

    return(true);
}

//////////////////////////////////////////////////////////////////////////
bool CScriptBind_Entity::ParseCloudMovementProperties(IScriptTable* pTable, IEntity* pEntity, SCloudMovementProperties& properties)
{
    CScriptSetGetChain chain(pTable);

    bool autoMove;
    properties.m_autoMove = false;
    if (chain.GetValue("bAutoMove", autoMove))
    {
        properties.m_autoMove = autoMove;
    }

    Vec3 speed;
    properties.m_speed = Vec3(0, 0, 0);
    if (chain.GetValue("vector_Speed", speed))
    {
        properties.m_speed = speed;
    }

    Vec3 spaceLoopBox;
    properties.m_spaceLoopBox = Vec3(2000.0f, 2000.0f, 2000.0f);
    if (chain.GetValue("vector_SpaceLoopBox", spaceLoopBox))
    {
        properties.m_spaceLoopBox = spaceLoopBox;
    }

    float fadeDistance;
    properties.m_fadeDistance = 0;
    if (chain.GetValue("fFadeDistance", fadeDistance))
    {
        properties.m_fadeDistance = fadeDistance;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CScriptBind_Entity::ParseVolumeObjectMovementProperties(IScriptTable* pTable, IEntity* pEntity, SVolumeObjectMovementProperties& properties)
{
    CScriptSetGetChain chain(pTable);

    bool autoMove;
    properties.m_autoMove = false;
    if (chain.GetValue("bAutoMove", autoMove))
    {
        properties.m_autoMove = autoMove;
    }

    Vec3 speed;
    properties.m_speed = Vec3(0, 0, 0);
    if (chain.GetValue("vector_Speed", speed))
    {
        properties.m_speed = speed;
    }

    Vec3 spaceLoopBox;
    properties.m_spaceLoopBox = Vec3(2000.0f, 2000.0f, 2000.0f);
    if (chain.GetValue("vector_SpaceLoopBox", spaceLoopBox))
    {
        properties.m_spaceLoopBox = spaceLoopBox;
    }

    float fadeDistance;
    properties.m_fadeDistance = 0;
    if (chain.GetValue("fFadeDistance", fadeDistance))
    {
        properties.m_fadeDistance = fadeDistance;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CScriptBind_Entity::ParsePhysicsParams(IScriptTable* pTable, SEntityPhysicalizeParams& params)
{
#if ENABLE_CRY_PHYSICS
    CScriptSetGetChain chain(pTable);
    chain.GetValue("mass", params.mass);
    chain.GetValue("density", params.density);
    chain.GetValue("flags", params.nFlagsOR);
    chain.GetValue("partid", params.nAttachToPart);
    chain.GetValue("stiffness_scale", params.fStiffnessScale);
    chain.GetValue("lod", params.nLod);
    chain.GetValue("retain_joint_vel", params.bCopyJointVelocities = false);
    if (!chain.GetValue("CGFprops", params.szPropsOverride) || params.szPropsOverride && !*params.szPropsOverride)
    {
        params.szPropsOverride = 0;
    }

    /*
    pe_params_particle   m_particleParams;
    pe_params_buoyancy   m_buoyancyParams;
    pe_player_dimensions m_playerDimensions;
    pe_player_dynamics   m_playerDynamics;
    pe_params_area_gravity  m_areaGravityParams;
    SEntityPhysicalizeParams::AreaDefinition m_areaDefinition;
    std::vector<Vec3> m_areaPoints;
    */

    /*
    {
        SmartScriptTable subTable;
        if (chain.GetValue( "buoyancy",subTable ))
        {
            CScriptSetGetChain buoyancy(subTable);
            buoyancy.GetValue( "water_density",m_buoyancyParams.waterDensity );
            buoyancy.GetValue( "water_resistance",m_buoyancyParams.waterResistance );
            buoyancy.GetValue( "water_damping",m_buoyancyParams.waterResistance );
        }
    }
    */

    switch (params.type)
    {
    case PE_PARTICLE:
    {
        SmartScriptTable subTable;
        if (chain.GetValue("Particle", subTable))
        {
            pe_params_particle tempParticleParams;
            tempParticleParams.q0.SetIdentity(); // Initialize the orientation quaternion to prevent the assert when it's assigned at the next line

            m_particleParams = tempParticleParams;

            params.pParticle = &m_particleParams;

            CScriptSetGetChain particle(subTable);
            Vec3 vel(0, 0, 0);
            float thickness = 0;
            int flags = 0;
            bool bValue;
            char const* szValue = 0;
            particle.GetValue("mass", m_particleParams.mass);
            particle.GetValue("radius", m_particleParams.size);
            particle.GetValue("air_resistance", m_particleParams.kAirResistance);
            particle.GetValue("water_resistance", m_particleParams.kWaterResistance);
            particle.GetValue("gravity", m_particleParams.gravity);
            particle.GetValue("water_gravity", m_particleParams.waterGravity);
            particle.GetValue("min_bounce_vel", m_particleParams.minBounceVel);
            particle.GetValue("accel_thrust", m_particleParams.accThrust);
            particle.GetValue("accel_lift", m_particleParams.accLift);
            particle.GetValue("velocity", vel);
            particle.GetValue("thickness", thickness);
            particle.GetValue("wspin", m_particleParams.wspin);
            particle.GetValue("normal", m_particleParams.normal);
            particle.GetValue("pierceability", m_particleParams.iPierceability);
            if (particle.GetValue("constant_orientation", bValue) && bValue)
            {
                flags |= particle_constant_orientation;
            }
            if (particle.GetValue("single_contact", bValue) && bValue)
            {
                flags |= particle_single_contact;
            }
            if (particle.GetValue("no_roll", bValue) && bValue)
            {
                flags |= particle_no_roll;
            }
            if (particle.GetValue("no_spin", bValue) && bValue)
            {
                flags |= particle_no_spin;
            }
            if (particle.GetValue("no_path_alignment", bValue) && bValue)
            {
                flags |= particle_no_path_alignment;
            }
            if (thickness != 0)
            {
                m_particleParams.thickness = thickness;
            }
            m_particleParams.velocity = vel.GetLength();
            if (m_particleParams.velocity != 0)
            {
                m_particleParams.heading = vel / m_particleParams.velocity;
            }
            m_particleParams.flags = flags | params.nFlagsOR;

            ScriptHandle ignore;
            if (particle.GetValue("collider_to_ignore", ignore))
            {
                IEntity* pEntity = m_pEntitySystem->GetEntity((EntityId)ignore.n);
                if (pEntity)
                {
                    IPhysicalEntity* pPE = pEntity->GetPhysics();
                    if (pPE)
                    {
                        m_particleParams.pColliderToIgnore = pPE;
                    }
                }
            }

            if (particle.GetValue("material", szValue))
            {
                ISurfaceType* pSurfaceType = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeByName(szValue);
                if (pSurfaceType)
                {
                    m_particleParams.surface_idx = pSurfaceType->GetId();
                }
            }
            ;
        }
    }
    break;

    case PE_LIVING:
    {
        SmartScriptTable subTable;
        if (chain.GetValue("Living", subTable))
        {
            Construct(m_playerDimensions);
            Construct(m_playerDynamics);
            params.pPlayerDimensions = &m_playerDimensions;
            params.pPlayerDynamics = &m_playerDynamics;

            CScriptSetGetChain living(subTable);
            // Player Dimensions
            living.GetValue("height", m_playerDimensions.heightCollider);
            living.GetValue("vector_size", m_playerDimensions.sizeCollider);
            living.GetValue("height_eye", m_playerDimensions.heightEye);
            living.GetValue("height_pivot", m_playerDimensions.heightPivot);
            living.GetValue("head_radius", m_playerDimensions.headRadius);
            living.GetValue("height_head", m_playerDimensions.heightHead);
            living.GetValue("groundContactEps", m_playerDimensions.groundContactEps);
            living.GetValue("bUseCapsule", m_playerDimensions.bUseCapsule);

            // Player Dynamics.
            living.GetValue("inertia", m_playerDynamics.kInertia);
            living.GetValue("inertiaAccel", m_playerDynamics.kInertiaAccel);
            living.GetValue("air_control", m_playerDynamics.kAirControl);
            living.GetValue("air_resistance", m_playerDynamics.kAirResistance);
            living.GetValue("gravity", m_playerDynamics.gravity.z);
            living.GetValue("mass", m_playerDynamics.mass);
            living.GetValue("min_slide_angle", m_playerDynamics.minSlideAngle);
            living.GetValue("max_climb_angle", m_playerDynamics.maxClimbAngle);
            living.GetValue("max_jump_angle", m_playerDynamics.maxJumpAngle);
            living.GetValue("min_fall_angle", m_playerDynamics.minFallAngle);
            living.GetValue("max_vel_ground", m_playerDynamics.maxVelGround);
            living.GetValue("timeImpulseRecover", m_playerDynamics.timeImpulseRecover);
            living.GetValue("nod_speed", m_playerDynamics.nodSpeed);
            living.GetValue("bActive", m_playerDynamics.bActive);
            living.GetValue("collision_types", m_playerDynamics.collTypes);
        }
    }
    break;

    case PE_WHEELEDVEHICLE:
    {
        SmartScriptTable subTable;
        if (chain.GetValue("Wheeled", subTable))
        {
            m_carParams = pe_params_car();
            params.pCar = &m_carParams;

            CScriptSetGetChain wheeled(subTable);

            wheeled.GetValue("axleFriction", m_carParams.axleFriction);
            wheeled.GetValue("brakeTorque", m_carParams.brakeTorque);
            wheeled.GetValue("clutchSpeed", m_carParams.clutchSpeed);
            wheeled.GetValue("damping", m_carParams.damping);
            wheeled.GetValue("engineIdleRPM", m_carParams.engineIdleRPM);
            wheeled.GetValue("engineMaxRPM", m_carParams.engineMaxRPM);
            wheeled.GetValue("engineMinRPM", m_carParams.engineMinRPM);
            wheeled.GetValue("enginePower", m_carParams.enginePower);
            wheeled.GetValue("engineShiftDownRPM", m_carParams.engineShiftDownRPM);
            wheeled.GetValue("engineShiftUpRPM", m_carParams.engineShiftUpRPM);
            wheeled.GetValue("engineStartRPM", m_carParams.engineStartRPM);
            wheeled.GetValue("integrationType", m_carParams.iIntegrationType);
            wheeled.GetValue("stabilizer", m_carParams.kStabilizer);
            wheeled.GetValue("maxBrakingFriction", m_carParams.maxBrakingFriction);
            wheeled.GetValue("maxSteer", m_carParams.maxSteer);
            wheeled.GetValue("maxTimeStep", m_carParams.maxTimeStep);
            wheeled.GetValue("minEnergy", m_carParams.minEnergy);
            wheeled.GetValue("slipThreshold", m_carParams.slipThreshold);
            wheeled.GetValue("gearDirSwitchRPM", m_carParams.gearDirSwitchRPM);
            wheeled.GetValue("dynFriction", m_carParams.kDynFriction);
            wheeled.GetValue("wheels", m_carParams.nWheels);
            wheeled.GetValue("steerTrackNeutralTurn", m_carParams.steerTrackNeutralTurn);

            std::vector<float> gearRatios;
            gearRatios.resize(0);
            SmartScriptTable gearRatiosTable;

            if (wheeled.GetValue("gearRatios", gearRatiosTable))
            {
                gearRatios.reserve(gearRatiosTable->Count());
                for (int i = 1; i <= gearRatiosTable->Count(); i++)
                {
                    float value;
                    if (!gearRatiosTable->GetAt(i, value))
                    {
                        break;
                    }
                    gearRatios.push_back(value);
                }

                m_carParams.nGears = gearRatios.size();
                m_carParams.gearRatios = new float[gearRatios.size()];
                for (int i = 0; i < m_carParams.nGears; i++)
                {
                    m_carParams.gearRatios[i] = gearRatios[i];
                }
            }
        }
    }
    break;


    case PE_AREA:
    {
        SmartScriptTable subTable;
        if (chain.GetValue("Area", subTable))
        {
            Construct(m_areaGravityParams);
            Construct(m_areaDefinition);
            m_areaDefinition.pGravityParams = &m_areaGravityParams;
            params.pAreaDef = &m_areaDefinition;

            float height = 0;

            CScriptSetGetChain areaTable(subTable);
            areaTable.GetValue("uniform", m_areaGravityParams.bUniform);
            areaTable.GetValue("falloffInner", m_areaGravityParams.falloff0);
            areaTable.GetValue("gravity", m_areaGravityParams.gravity);
            areaTable.GetValue("damping", m_areaGravityParams.damping);

            Vec3 vWind;
            if (areaTable.GetValue("wind", vWind))
            {
                new(&m_buoyancyParams)pe_params_buoyancy;
                params.pBuoyancy = &m_buoyancyParams;
                m_buoyancyParams.waterFlow = vWind;
                areaTable.GetValue("variance", m_buoyancyParams.flowVariance);
                // Set medium plane at water level, facing down.
                m_buoyancyParams.iMedium = 1;
                m_buoyancyParams.waterDensity = 1;
                m_buoyancyParams.waterResistance = 1;
                areaTable.GetValue("density", m_buoyancyParams.waterDensity);
                areaTable.GetValue("resistance", m_buoyancyParams.waterResistance);
                m_buoyancyParams.waterPlane.n = Vec3(0, 0, -1);
                const float noOceanPlaneLevel = -100000000.0f; // If there's no ocean, set this plane very low so it's effectively global.
                m_buoyancyParams.waterPlane.origin = Vec3(0, 0, OceanToggle::IsActive() ? OceanRequest::GetOceanLevelOrDefault(noOceanPlaneLevel) : gEnv->p3DEngine->GetWaterLevel());
            }

            int type = -1;
            areaTable.GetValue("type", type);
            params.pAreaDef->areaType = SEntityPhysicalizeParams::AreaDefinition::EAreaType(type);

            areaTable.GetValue("radius", params.pAreaDef->fRadius);
            areaTable.GetValue("box_min", params.pAreaDef->boxmin);
            areaTable.GetValue("box_max", params.pAreaDef->boxmax);
            areaTable.GetValue("height", height);
            areaTable.GetValue("center", params.pAreaDef->center);
            areaTable.GetValue("axis", params.pAreaDef->axis);
            SmartScriptTable points;
            if (areaTable.GetValue("points", points))
            {
                m_areaPoints.resize(0);
                float minz = 1e+8;
                IScriptTable::Iterator iter = points->BeginIteration();
                while (points->MoveNext(iter))
                {
                    SmartScriptTable pointTable;
                    if (iter.value.CopyTo(pointTable))
                    {
                        Vec3 v;
                        if (pointTable->GetValue("x", v.x) && pointTable->GetValue("y", v.y) && pointTable->GetValue("z", v.z))
                        {
                            m_areaPoints.push_back(v);
                            if (v.z < minz)
                            {
                                minz = v.z;
                            }
                        }
                    }

                    if (iter.value.type == ANY_TVECTOR)
                    {
                        Vec3 v = Vec3(iter.value.vec3.x, iter.value.vec3.y, iter.value.vec3.z);
                        m_areaPoints.push_back(v);
                        if (v.z < minz)
                        {
                            minz = v.z;
                        }
                    }
                }
                params.pAreaDef->zmin = minz;
                params.pAreaDef->zmax = minz + height;
                params.pAreaDef->pPoints = &m_areaPoints[0];
                params.pAreaDef->nNumPoints = m_areaPoints.size();
                points->EndIteration(iter);
            }
            else
            {
                params.pAreaDef->zmin = 0;
                params.pAreaDef->zmax = height;
            }
        }
    }
    break;

    case PE_SOFT:
    {
        int id;
        ScriptHandle idEnt;
        if (chain.GetValue("AttachmentIdEnt", idEnt))
        {
            if (IEntity* pEntity = gEnv->pEntitySystem->GetEntity((EntityId)idEnt.n))
            {
                params.pAttachToEntity = pEntity->GetPhysics();
            }
        }
        if (chain.GetValue("AttachmentId", id))
        {
            params.pAttachToEntity = gEnv->pPhysicalWorld->GetPhysicalEntityById(id);
        }
        chain.GetValue("AttachmentPartId", params.nAttachToPart);
        if (chain.GetValue("bRigidCore", id) && id)
        {
            params.nFlagsOR = se_rigid_core;
        }
    }
    }
#endif // ENABLE_CRY_PHYSICS

    return true;
}

int CScriptBind_Entity::GetDistance(IFunctionHandler* pH)
{
    GET_ENTITY;
    ScriptHandle hdl;
    pH->GetParam(1, hdl);
    //retrieve the other entity
    IEntity* pEntity2 = m_pISystem->GetIEntitySystem()->GetEntity((EntityId)hdl.n);
    if (pEntity2)
    {
        return pH->EndFunction((pEntity2->GetWorldPos() - pEntity->GetWorldPos()).GetLength());
    }
    else
    {
        return pH->EndFunction(-1);
    }
}


int __cdecl CScriptBind_Entity::cmpIntersectionResult(const void* v1, const void* v2)
{
    SIntersectionResult* a = (SIntersectionResult*)v1;
    SIntersectionResult* b = (SIntersectionResult*)v1;

    if (a->distance < b->distance)
    {
        return 1;
    }
    else if (a->distance > b->distance)
    {
        return -1;
    }

    return 0;
}


int CScriptBind_Entity::IntersectRay(IFunctionHandler* pH, int slot, Vec3 rayOrigin, Vec3 rayDir, float maxDistance)
{
    GET_ENTITY;

    static SIntersectionResult results[32];
    int resultCount = 0;

    int count = pEntity->GetSlotCount();
    int startSlot = 0;

    // if slot is specified, use only that slot
    if (slot > -1)
    {
        startSlot = slot;
        count = startSlot + 1;
    }

    for (int i = 0; i < count; i++)
    {
        IStatObj* pStatObj = pEntity->GetStatObj(i);

        if (pStatObj)
        {
            Matrix34 slotMatrix = pEntity->GetSlotWorldTM(i);
            Matrix34 invSlotMatrix = slotMatrix.GetInverted();

            Vec3 o = invSlotMatrix.TransformPoint(rayOrigin);
            Vec3 d = invSlotMatrix.TransformVector(rayDir);

            int hits = IStatObjRayIntersect(pStatObj, o, d, maxDistance, &results[resultCount], 32 - resultCount);

            if (hits)
            {
                for (int r = 0; r < hits; r++)
                {
                    slotMatrix.TransformPoint(results[resultCount + i].position);
                }

                resultCount += hits;

                assert(resultCount <= 32);
                if (resultCount >= 32)
                {
                    break;
                }
            }
        }
    }

    if (resultCount)
    {
        // sort the results by distance
        qsort(&results, resultCount, sizeof(SIntersectionResult), cmpIntersectionResult);

        SmartScriptTable hitTable(m_pSS);

        for (int j = 0; j < resultCount; j++)
        {
            SmartScriptTable resultTable(m_pSS);

            {
                CScriptSetGetChain resultChain(resultTable);

                resultChain.SetValue("position", results[j].position);
                resultChain.SetValue("v0", results[j].v0);
                resultChain.SetValue("v1", results[j].v1);
                resultChain.SetValue("v2", results[j].v2);
                resultChain.SetValue("uv0", Vec3(results[j].uv0.x, results[j].uv0.y, 0));
                resultChain.SetValue("uv1", Vec3(results[j].uv1.x, results[j].uv1.y, 0));
                resultChain.SetValue("uv2", Vec3(results[j].uv2.x, results[j].uv2.y, 0));
                resultChain.SetValue("baricentric", results[j].baricentric);
                resultChain.SetValue("distance", results[j].distance);
                resultChain.SetValue("backface", results[j].backface);
                resultChain.SetValue("material", results[j].material);
            }

            hitTable->SetAt(j + 1, resultTable);
        }

        return pH->EndFunction(hitTable);
    }

    return pH->EndFunction();
}


int CScriptBind_Entity::IStatObjRayIntersect(IStatObj* pStatObj, const Vec3& rayOrigin, const Vec3& rayDir, float maxDistance, SIntersectionResult* pOutResult, unsigned int maxResults)
{
    IRenderMesh* pRenderMesh = pStatObj->GetRenderMesh();

    int posStride = 0;
    int uvStride = 0;

    byte* pPosBuf = pRenderMesh->GetPosPtr(posStride, FSL_READ);
    byte* pUVBuf = pRenderMesh->GetUVPtr(uvStride, FSL_READ);


    int indexCount = pRenderMesh->GetIndicesCount();
    vtx_idx* pIndex = pRenderMesh->GetIndexPtr(FSL_READ);

    int hitCount = 0;

    assert((indexCount % 3) == 0);

    for (int i = 0; i < indexCount; i += 3)
    {
        bool backfacing = false;
        const Vec3& v0 = *(Vec3*)&pPosBuf[posStride * pIndex[i + 0]];
        const Vec3& v1 = *(Vec3*)&pPosBuf[posStride * pIndex[i + 1]];
        const Vec3& v2 = *(Vec3*)&pPosBuf[posStride * pIndex[i + 2]];

        Vec3 e1 = v1 - v0;
        Vec3 e2 = v2 - v0;

        if (rayDir.Dot(e1.Cross(e2)) > 0.0001)
        {
            backfacing = true;
        }

        Vec3 p  = rayDir.Cross(e2);
        float tmp = p.Dot(e1);

        if ((tmp > -0.001) && (tmp < 0.001))
        {
            continue;
        }

        tmp = 1.0f / tmp;
        Vec3 s = rayOrigin - v0;

        float u = tmp * s.Dot(p);

        if (u < 0.0f || u > 1.0f)
        {
            continue;
        }

        Vec3 q = s.Cross(e1);
        float v = tmp * rayDir.Dot(q);

        if (v < 0.0f || v > 1.0f)
        {
            continue;
        }

        if (u + v > 1.0f)
        {
            continue;
        }

        float t = tmp * e2.Dot(q);

        if (t > maxDistance || t < -maxDistance)
        {
            continue;
        }

        SIntersectionResult& result = pOutResult[hitCount++];

        result.baricentric.x = 1 - (u + v);
        result.baricentric.y = u;
        result.baricentric.z = v;
        result.distance = t;
        result.position = rayOrigin + t * rayDir;
        result.v0 = v0;
        result.v1 = v1;
        result.v2 = v2;
        result.uv0 = *(Vec2*)&pUVBuf[uvStride * pIndex[i + 0]];
        result.uv1 = *(Vec2*)&pUVBuf[uvStride * pIndex[i + 1]];
        result.uv2 = *(Vec2*)&pUVBuf[uvStride * pIndex[i + 2]];
        result.backface = backfacing;
        result.material[0] = 0;

        if (pStatObj->GetMaterial())
        {
            cry_strcpy(result.material, pStatObj->GetMaterial()->GetName());
        }

        if (hitCount >= (int)maxResults)
        {
            break;
        }
    }

    return hitCount;
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::ProcessBroadcastEvent(IFunctionHandler* pH)
{
    GET_ENTITY;

    const char* sEventName;
    if (!pH->GetParams(sEventName))
    {
        return pH->EndFunction();
    }

    SEntityEvent event(ENTITY_EVENT_SCRIPT_EVENT);
    event.nParam[0] = (INT_PTR)sEventName;
    event.nParam[1] = IEntityClass::EVT_BOOL;
    bool bValue = true;
    event.nParam[2] = (INT_PTR)&bValue;
    pEntity->SendEvent(event);

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::ActivateOutput(IFunctionHandler* pH)
{
    GET_ENTITY;

    const char* sEventName;
    if (!pH->GetParams(sEventName))
    {
        return pH->EndFunction();
    }

    IComponentScriptPtr scriptComponent = pEntity->GetComponent<IComponentScript>();
    if (!scriptComponent)
    {
        return pH->EndFunction();
    }

    IEntityClass::SEventInfo eventInfo;
    IEntityClass* pEntityClass = pEntity->GetClass();
    if (!pEntityClass->FindEventInfo(sEventName, eventInfo))
    {
        EntityWarning("ActivateOutput called with undefined event %s for entity %s", sEventName, pEntity->GetEntityTextDescription());
        return pH->EndFunction();
    }

    SEntityEvent event(ENTITY_EVENT_SCRIPT_EVENT);
    event.nParam[0] = (INT_PTR)sEventName;
    event.nParam[1] = eventInfo.type;

    switch (eventInfo.type)
    {
    case IEntityClass::EVT_INT:
    {
        int value = 0;
        if (!pH->GetParam(2, value))
        {
            EntityWarning("ActivateOutput(%s,value) called with wrong type for 2nd parameter for entity %s", sEventName, pEntity->GetEntityTextDescription());
        }
        event.nParam[2] = (INT_PTR)&value;
        pEntity->SendEvent(event);
    }
    break;
    case IEntityClass::EVT_FLOAT:
    {
        float value = 0;
        if (!pH->GetParam(2, value))
        {
            EntityWarning("ActivateOutput(%s,value) called with wrong type for 2nd parameter for entity %s", sEventName, pEntity->GetEntityTextDescription());
        }
        event.nParam[2] = (INT_PTR)&value;
        pEntity->SendEvent(event);
    }
    break;
    case IEntityClass::EVT_BOOL:
    {
        bool value = false;
        if (!pH->GetParam(2, value))
        {
            EntityWarning("ActivateOutput(%s,value) called with wrong type for 2nd parameter for entity %s", sEventName, pEntity->GetEntityTextDescription());
        }
        event.nParam[2] = (INT_PTR)&value;
        pEntity->SendEvent(event);
    }
    break;
    case IEntityClass::EVT_VECTOR:
    {
        Vec3 value(0, 0, 0);
        if (!pH->GetParam(2, value))
        {
            EntityWarning("ActivateOutput(%s,value) called with wrong type for 2nd parameter for entity %s", sEventName, pEntity->GetEntityTextDescription());
        }
        event.nParam[2] = (INT_PTR)&value;
        pEntity->SendEvent(event);
    }
    break;
    case IEntityClass::EVT_ENTITY:
    {
        EntityId value = 0;
        ScriptHandle entityId;
        if (!pH->GetParam(2, entityId))
        {
            EntityWarning("ActivateOutput(%s,value) called with wrong type for 2nd parameter for entity %s", sEventName, pEntity->GetEntityTextDescription());
        }
        value = (EntityId)entityId.n;
        event.nParam[2] = (INT_PTR)&value;
        pEntity->SendEvent(event);
    }
    break;
    case IEntityClass::EVT_STRING:
    {
        const char* value = "";
        if (!pH->GetParam(2, value))
        {
            EntityWarning("ActivateOutput(%s,value) called with wrong type for 2nd parameter for entity %s", sEventName, pEntity->GetEntityTextDescription());
        }
        event.nParam[2] = (INT_PTR)value;
        pEntity->SendEvent(event);
    }
    break;
    default:
        assert(0);
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::CreateCameraComponent(IFunctionHandler* pH)
{
    GET_ENTITY;

    pEntity->GetOrCreateComponent<IComponentCamera>();
    pEntity->SetFlags(pEntity->GetFlags() | ENTITY_FLAG_CAMERA_SOURCE);

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::BreakToPieces(IFunctionHandler* pH, int nSlot, int nPiecesSlot, float fExplodeImp, Vec3 vHitPt, Vec3 vHitImp, float fLifeTime, bool bMaterialEffects)
{
#if ENABLE_CRY_PHYSICS
    GET_ENTITY;

    CBreakableManager::BreakageParams bp;
    bp.fParticleLifeTime = fLifeTime;
    bp.bMaterialEffects = bMaterialEffects;
    bp.fExplodeImpulse = fExplodeImp;
    bp.vHitPoint = vHitPt;
    bp.vHitImpulse = vHitImp;
    m_pEntitySystem->GetBreakableManager()->BreakIntoPieces(pEntity, nSlot, nPiecesSlot, bp);
#endif // ENABLE_CRY_PHYSICS
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::AttachSurfaceEffect(IFunctionHandler* pH, int nSlot, const char* effect, bool countPerUnit, const char* form, const char* typ, float countScale, float sizeScale)
{
    GET_ENTITY;

    IParticleEffect* pEffect = gEnv->pParticleManager->FindEffect(effect, pEntity->GetEntityTextDescription());
    if (!pEffect)
    {
        return pH->EndFunction();
    }

    // Check whether already attached.
    SEntitySlotInfo slotInfo;
    if (pEntity->GetSlotInfo(nSlot, slotInfo))
    {
        if (slotInfo.pParticleEmitter && !strcmp(slotInfo.pParticleEmitter->GetName(), pEffect->GetName()))
        {
            return pH->EndFunction();
        }
    }


    SpawnParams params;
    params.bCountPerUnit = countPerUnit;

#if !defined(OTHER_ACTIVE)
    TypeInfo(&params.eAttachForm).FromString(&params.eAttachForm, form);
    TypeInfo(&params.eAttachType).FromString(&params.eAttachType, typ);
#endif

    params.fCountScale = countScale;
    params.fSizeScale = sizeScale;

    pEntity->LoadParticleEmitter(nSlot, pEffect, &params);

    return pH->EndFunction();
}


//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::RagDollize(IFunctionHandler* pH, int slot)
{
    IEntity* pEntity = GetEntity(pH);
    if (!pEntity)
    {
        return pH->EndFunction();
    }
#if ENABLE_CRY_PHYSICS

    pEntity->SetSlotLocalTM(slot, Matrix34::CreateTranslationMat(Vec3(ZERO)));

    float mass = 80.f;

    pe_status_dynamics stats;
    if (pEntity->GetPhysics() && pEntity->GetPhysics()->GetStatus(&stats))
    {
        mass = stats.mass;
    }

    SEntityPhysicalizeParams pp;

    pp.type = PE_ARTICULATED;
    pp.nSlot = 0;
    pp.mass = mass;
    pp.fStiffnessScale = 73.0f;

    pe_player_dimensions playerDim;
    pe_player_dynamics playerDyn;

    playerDyn.gravity = Vec3(0, 0, -9.81f);
    playerDyn.mass = mass;
    playerDyn.kInertia = 5.5f;

    pp.pPlayerDimensions = &playerDim;
    pp.pPlayerDynamics = &playerDyn;
    pp.nLod = 1;

    pp.bCopyJointVelocities = true;

    pEntity->Physicalize(pp);
    //GetGameObject()->ResetPhysics();

    IPhysicalEntity* pPhysEnt = pEntity->GetPhysics();

    if (pPhysEnt)
    {
        pe_simulation_params sp;
        sp.gravity = sp.gravityFreefall = playerDyn.gravity;
        sp.damping = 1.0f;
        sp.dampingFreefall = 0.0f;
        sp.mass = mass;

        pPhysEnt->SetParams(&sp);

        pPhysEnt->GetStatus(&stats);
        CryLog("RagDollized with mass %f", stats.mass);
    }
#endif // ENABLE_CRY_PHYSICS

    ICharacterInstance* pCharacter = pEntity->GetCharacter(slot);
    if (pCharacter)
    {
        pCharacter->GetISkeletonAnim()->StopAnimationsAllLayers();

        IAnimationPoseBlenderDir* pIPoseBlenderLook = pCharacter->GetISkeletonPose()->GetIPoseBlenderLook();
        if (pIPoseBlenderLook)
        {
            pIPoseBlenderLook->SetState(false);
        }

        pCharacter->EnableStartAnimation(false);
    }

    return pH->EndFunction();
}


//////////////////////////////////////////////////////////////////////////
/*! Set the position of the entity
@param vec Three component vector containing the entity position
*/
int CScriptBind_Entity::SetColliderMode(IFunctionHandler* pH, int mode)
{
    GET_ENTITY;
    SEntityEvent e;
    e.event = ENTITY_EVENT_SCRIPT_REQUEST_COLLIDERMODE;
    e.nParam[0] = mode;
    pEntity->SendEvent(e);

    return pH->EndFunction();
}



//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::UnSeenFrames(IFunctionHandler* pH)
{
    GET_ENTITY;

    IComponentRenderPtr pComponent = pEntity->GetComponent<IComponentRender>();
    int     frameDiff(0);
    if (pComponent)
    {
        frameDiff = gEnv->pRenderer->GetFrameID() - pComponent->GetRenderNode()->GetDrawFrame();
    }

    return pH->EndFunction(frameDiff);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::DeleteParticleEmitter(IFunctionHandler* pH, int slot)
{
    GET_ENTITY;

    int res = IsSlotParticleEmitter(pH, slot);
    if (res)
    {
        IParticleEmitter* pEmmitter = pEntity->GetParticleEmitter(slot);
        if (pEmmitter)
        {
            gEnv->pParticleManager->DeleteEmitter(pEmmitter);
        }
    }
    else
    {
        CryLog("Entity %s, slot %d is not particle emitter", pEntity->GetName(), slot);
    }
    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::RegisterForAreaEvents(IFunctionHandler* pH, int enable)
{
    GET_ENTITY;

    IComponentScriptPtr scriptComponent = pEntity->GetComponent<IComponentScript>();
    if (scriptComponent)
    {
        scriptComponent->RegisterForAreaEvents(enable != 0);
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::RenderAlways(IFunctionHandler* pH, int enable)
{
    GET_ENTITY;

    IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
    IRenderNode* pRenderNode = (pRenderComponent != NULL) ? pRenderComponent->GetRenderNode() : NULL;
    if (pRenderNode)
    {
        pRenderNode->SetRndFlags(ERF_RENDER_ALWAYS, (enable != 0) ? true : false);
    }

    return pH->EndFunction();
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::GetTimeOfDayHour(IFunctionHandler* pH)
{
    float curTime = gEnv->p3DEngine->GetTimeOfDay()->GetTime();
    return pH->EndFunction(curTime);
}

//////////////////////////////////////////////////////////////////////////
int CScriptBind_Entity::CreateDRSProxy(IFunctionHandler* pH)
{
    GET_ENTITY;

    IComponentDynamicResponsePtr pDrsComponent = pEntity->GetComponent<IComponentDynamicResponse>();

    return pH->EndFunction();
}

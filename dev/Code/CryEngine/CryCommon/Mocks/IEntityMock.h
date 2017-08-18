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
#pragma once
#include <IComponent.h>
#include <IAgent.h>
#include <IShader.h>
#include <IEntity.h>

class EntityMock
    : public IEntity
{
public:
    MOCK_CONST_METHOD0(GetId,
        EntityId());
    MOCK_CONST_METHOD0(GetGuid,
        EntityGUID());
    MOCK_CONST_METHOD0(GetClass,
        IEntityClass * ());
    MOCK_METHOD0(GetArchetype,
        IEntityArchetype * ());
    MOCK_METHOD1(SetFlags,
        void(uint32 flags));
    MOCK_CONST_METHOD0(GetFlags,
        uint32());
    MOCK_METHOD1(AddFlags,
        void(uint32 flagsToAdd));
    MOCK_METHOD1(ClearFlags,
        void(uint32 flagsToClear));
    MOCK_CONST_METHOD1(CheckFlags,
        bool(uint32 flagsToCheck));
    MOCK_METHOD1(SetFlagsExtended,
        void(uint32 flags));
    MOCK_CONST_METHOD0(GetFlagsExtended,
        uint32());
    MOCK_CONST_METHOD0(IsGarbage,
        bool());
    MOCK_METHOD1(SetName,
        void(const char* sName));
    MOCK_CONST_METHOD0(GetName,
        const char*());
    MOCK_CONST_METHOD0(GetEntityTextDescription,
        const char*());
    MOCK_METHOD2(SerializeXML,
        void(XmlNodeRef & entityNode, bool bLoading));
    MOCK_CONST_METHOD0(IsLoadedFromLevelFile,
        bool());
    MOCK_METHOD2(AttachChild,
        void(IEntity*, const SChildAttachParams&));
    MOCK_METHOD1(DetachAll,
        void(int));
    MOCK_METHOD2(DetachThis,
        void(int, int));
    MOCK_CONST_METHOD0(GetChildCount,
        int());
    MOCK_CONST_METHOD1(GetChild,
        IEntity * (int nIndex));
    MOCK_CONST_METHOD0(GetParent,
        IEntity * ());
    MOCK_CONST_METHOD0(GetParentAttachPointWorldTM,
        Matrix34());
    MOCK_CONST_METHOD0(IsParentAttachmentValid,
        bool());
    MOCK_METHOD2(SetWorldTM,
        void(const Matrix34&, int));
    MOCK_METHOD2(SetLocalTM,
        void(const Matrix34&, int));
    MOCK_CONST_METHOD0(GetWorldTM,
        const Matrix34&());
    MOCK_CONST_METHOD0(GetLocalTM,
        Matrix34());
    MOCK_CONST_METHOD1(GetWorldBounds,
        void(AABB & bbox));
    MOCK_CONST_METHOD1(GetLocalBounds,
        void(AABB & bbox));
    MOCK_METHOD4(SetPos,
        void(const Vec3&, int, bool, bool));
    MOCK_CONST_METHOD0(GetPos,
        const Vec3&());
    MOCK_METHOD2(SetRotation,
        void(const Quat&, int));
    MOCK_CONST_METHOD0(GetRotation,
        const Quat&());
    MOCK_METHOD2(SetScale,
        void(const Vec3&, int));
    MOCK_CONST_METHOD0(GetScale,
        const Vec3&());
    MOCK_METHOD4(SetPosRotScale,
        void(const Vec3&, const Quat&, const Vec3&, int));
    MOCK_CONST_METHOD0(GetWorldPos,
        Vec3());
    MOCK_CONST_METHOD0(GetWorldAngles,
        Ang3());
    MOCK_CONST_METHOD0(GetWorldRotation,
        Quat());
    MOCK_CONST_METHOD0(GetForwardDir,
        const Vec3&());
    MOCK_METHOD1(Activate,
        void(bool bActive));
    MOCK_CONST_METHOD0(IsActive,
        bool());
    MOCK_CONST_METHOD0(IsFromPool,
        bool());
    MOCK_METHOD1(PrePhysicsActivate,
        void(bool bActive));
    MOCK_METHOD0(IsPrePhysicsActive,
        bool());
    MOCK_METHOD2(Serialize,
        void(TSerialize, int));
    MOCK_METHOD1(SendEvent,
        bool(SEntityEvent & event));
    MOCK_METHOD2(SetTimer,
        void(int nTimerId, int nMilliSeconds));
    MOCK_METHOD1(KillTimer,
        void(int nTimerId));
    MOCK_METHOD1(Hide,
        void(bool bHide));
    MOCK_CONST_METHOD0(IsHidden,
        bool());
    MOCK_METHOD1(Invisible,
        void(bool bInvisible));
    MOCK_CONST_METHOD0(IsInvisible,
        bool());
    MOCK_METHOD0(GetAI,
        IAIObject * ());
    MOCK_CONST_METHOD0(HasAI,
        bool());
    MOCK_CONST_METHOD0(GetAIObjectID,
        tAIObjectID());
    MOCK_METHOD1(SetAIObjectID,
        void(tAIObjectID id));
    MOCK_METHOD1(RegisterInAISystem,
        bool(const AIObjectParams &params));
    MOCK_METHOD1(SetUpdatePolicy,
        void(EEntityUpdatePolicy eUpdatePolicy));
    MOCK_CONST_METHOD0(GetUpdatePolicy,
        EEntityUpdatePolicy());
    MOCK_CONST_METHOD1(GetProxy,
        IEntityProxy * (EEntityProxy proxy));
    MOCK_METHOD2(SetProxy,
        void(EEntityProxy proxy, IEntityProxyPtr pProxy));
    MOCK_METHOD1(CreateProxy,
        IEntityProxyPtr(EEntityProxy proxy));
    // Compiler warning C4373 due to meaningless "const int" *declaration* in IEntity.h
    // https://code.google.com/p/googlemock/wiki/FrequentlyAskedQuestions#MSVC_gives_me_warning_C4301_or_C4373_when_I_define_a_mock_method
    MOCK_METHOD2(RegisterComponent,
        void(IComponentPtr pComponent, const int flags));
    MOCK_METHOD1(Physicalize,
        void(SEntityPhysicalizeParams & params));
    MOCK_CONST_METHOD0(GetPhysics,
        IPhysicalEntity * ());
    MOCK_METHOD2(PhysicalizeSlot,
        int(int slot, SEntityPhysicalizeParams & params));
    MOCK_METHOD1(UnphysicalizeSlot,
        void(int slot));
    MOCK_METHOD1(UpdateSlotPhysics,
        void(int slot));
    MOCK_METHOD1(SetPhysicsState,
        void(XmlNodeRef & physicsState));
    MOCK_METHOD1(SetMaterial,
        void(_smart_ptr<IMaterial> pMaterial));
    MOCK_METHOD0(GetMaterial,
        _smart_ptr<IMaterial> ());
    MOCK_CONST_METHOD1(IsSlotValid,
        bool(int nIndex));
    MOCK_METHOD1(FreeSlot,
        void(int nIndex));
    MOCK_CONST_METHOD0(GetSlotCount,
        int());
    MOCK_CONST_METHOD2(GetSlotInfo,
        bool(int nIndex, SEntitySlotInfo & slotInfo));
    MOCK_CONST_METHOD1(GetSlotWorldTM,
        const Matrix34&(int nSlot));
    MOCK_CONST_METHOD2(GetSlotLocalTM,
        const Matrix34&(int nSlot, bool bRelativeToParent));
    MOCK_METHOD3(SetSlotLocalTM,
        void(int, const Matrix34&, int));
    MOCK_METHOD2(SetSlotCameraSpacePos,
        void(int nSlot, const Vec3 &cameraSpacePos));
    MOCK_CONST_METHOD2(GetSlotCameraSpacePos,
        void(int nSlot, Vec3 & cameraSpacePos));
    MOCK_METHOD2(SetParentSlot,
        bool(int nParentIndex, int nChildIndex));
    MOCK_METHOD2(SetSlotMaterial,
        void(int nSlot, _smart_ptr<IMaterial> pMaterial));
    MOCK_METHOD2(SetSlotFlags,
        void(int nSlot, uint32 nFlags));
    MOCK_CONST_METHOD1(GetSlotFlags,
        uint32(int nSlot));
    MOCK_CONST_METHOD1(ShouldUpdateCharacter,
        bool(int nSlot));
    MOCK_METHOD1(GetCharacter,
        ICharacterInstance * (int nSlot));
    MOCK_METHOD2(SetCharacter,
        int(ICharacterInstance * pCharacter, int nSlot));
    MOCK_METHOD1(GetStatObj,
        IStatObj * (int nSlot));
    MOCK_METHOD1(GetParticleEmitter,
        IParticleEmitter * (int nSlot));
    MOCK_METHOD1(GetGeomCacheRenderNode,
        IGeomCacheRenderNode * (int nSlot));
    MOCK_METHOD2(MoveSlot,
        void(IEntity * targetIEnt, int nSlot));
    MOCK_METHOD4(SetStatObj,
        int(IStatObj*, int, bool, float));
    MOCK_METHOD4(LoadGeometry,
        int(int, const char*, const char*, int));
    MOCK_METHOD3(LoadCharacter,
        int(int, const char*, int));
    MOCK_METHOD2(LoadGeomCache,
        int(int nSlot, const char* sFilename));
    MOCK_METHOD5(LoadParticleEmitter,
        int(int, IParticleEffect*, SpawnParams const*, bool, bool));
    MOCK_METHOD3(SetParticleEmitter,
        int(int, IParticleEmitter*, bool));
    MOCK_METHOD2(LoadLight,
        int(int nSlot, CDLight * pLight));
    MOCK_METHOD2(InvalidateTM,
        void(int, bool));
    MOCK_METHOD1(EnablePhysics,
        void(bool enable));
    MOCK_METHOD0(GetEntityLinks,
        IEntityLink * ());
    MOCK_METHOD3(AddEntityLink,
        IEntityLink * (const char*, EntityId, EntityGUID));
    MOCK_METHOD1(RemoveEntityLink,
        void(IEntityLink * pLink));
    MOCK_METHOD0(RemoveAllEntityLinks,
        void());
    MOCK_METHOD1(UnmapAttachedChild,
        IEntity * (int& partId));
    MOCK_CONST_METHOD0(IsInitialized,
        bool());
    MOCK_METHOD1(DebugDraw,
        void(const struct SGeometryDebugDrawInfo& info));
    MOCK_CONST_METHOD1(GetMemoryUsage,
        void(ICrySizer * pSizer));
    MOCK_METHOD0(IncKeepAliveCounter,
        void());
    MOCK_METHOD0(DecKeepAliveCounter,
        void());
    MOCK_METHOD0(ResetKeepAliveCounter,
        void());
    MOCK_CONST_METHOD0(IsKeptAlive,
        bool());
    MOCK_METHOD2(HandleVariableChange,
        bool(const char* szVarName, const void* pVarData));
    MOCK_CONST_METHOD0(GetSwObjDebugFlag,
        unsigned int());
    MOCK_METHOD1(SetSwObjDebugFlag,
        void(unsigned int uiVal));
};

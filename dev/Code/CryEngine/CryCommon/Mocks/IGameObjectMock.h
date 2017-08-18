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
#include <IGameObject.h>

class GameObjectMock
    : public IGameObject
{
public:
    // IGameObject
    MOCK_METHOD1(BindToNetwork,
        bool(EBindToNetworkMode));
    MOCK_METHOD2(BindToNetworkWithParent,
        bool(EBindToNetworkMode mode, EntityId parentId));
    MOCK_METHOD1(ChangedNetworkState,
        void(NetworkAspectType aspects));
    MOCK_METHOD2(EnableAspect,
        void(NetworkAspectType aspects, bool enable));
    MOCK_METHOD2(EnableDelegatableAspect,
        void(NetworkAspectType aspects, bool enable));
    MOCK_METHOD0(DontSyncPhysics,
        void());
    MOCK_CONST_METHOD1(QueryExtension,
        IGameObjectExtension * (IGameObjectSystem::ExtensionID id));
    MOCK_CONST_METHOD1(QueryExtension,
        IGameObjectExtension * (const char* name));
    MOCK_METHOD2(SetExtensionParams,
        bool(const char* extension, SmartScriptTable params));
    MOCK_METHOD2(GetExtensionParams,
        bool(const char* extension, SmartScriptTable params));
    MOCK_METHOD1(SendEvent,
        void(const SGameObjectEvent&));
    MOCK_METHOD1(ForceUpdate,
        void(bool force));
    MOCK_METHOD2(ForceUpdateExtension,
        void(IGameObjectExtension * pGOE, int slot));
    MOCK_CONST_METHOD0(GetChannelId,
        ChannelId());
    MOCK_METHOD1(SetChannelId,
        void(ChannelId));
    MOCK_METHOD1(FullSerialize,
        void(TSerialize ser));
    MOCK_METHOD4(NetSerialize,
        bool(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags));
    MOCK_METHOD0(PostSerialize,
        void());
    MOCK_METHOD0(IsProbablyVisible,
        bool());
    MOCK_METHOD0(IsProbablyDistant,
        bool());
    MOCK_METHOD0(GetNetSerializeAspects,
        NetworkAspectType());
    MOCK_METHOD3(SetAspectProfile,
        bool(EEntityAspects, uint8, bool));
    MOCK_METHOD1(GetAspectProfile,
        uint8(EEntityAspects aspect));
    MOCK_METHOD1(GetExtensionWithRMIBase,
        IGameObjectExtension * (const void* pBase));
    MOCK_METHOD1(EnablePrePhysicsUpdate,
        void(EPrePhysicsUpdate updateRule));
    MOCK_METHOD1(SetNetworkParent,
        void(EntityId id));
    MOCK_METHOD1(Pulse,
        void(uint32 pulse));
    MOCK_METHOD0(RegisterAsPredicted,
        void());
    MOCK_METHOD2(RegisterAsValidated,
        void(IGameObject * pGO, int predictionHandle));
    MOCK_METHOD0(GetPredictionHandle,
        int());
    MOCK_METHOD3(RegisterExtForEvents,
        void(IGameObjectExtension * piExtension, const int* pEvents, const int numEvents));
    MOCK_METHOD3(UnRegisterExtForEvents,
        void(IGameObjectExtension * piExtension, const int* pEvents, const int numEvents));
    MOCK_METHOD2(EnablePhysicsEvent,
        void(bool enable, int events));
    MOCK_METHOD1(WantsPhysicsEvent,
        bool(int events));
    MOCK_METHOD0(AttachDistanceChecker,
        void());
    MOCK_METHOD1(SetAIActivation,
        bool(EGameObjectAIActivationMode mode));
    MOCK_METHOD1(SetAutoDisablePhysicsMode,
        void(EAutoDisablePhysicsMode mode));
    MOCK_METHOD0(ShouldUpdate,
        bool());
    MOCK_METHOD1(RequestRemoteUpdate,
        void(NetworkAspectType aspectMask));
    MOCK_METHOD2(SerializeSpawnInfo,
        void(TSerialize ser, IEntity * entity));
    MOCK_METHOD1(GetExtensionWithRMIRep,
        IGameObjectExtension * (uint32 repId));
    MOCK_METHOD1(SetAuthority,
        void(bool auth));
    MOCK_METHOD1(CaptureView,
        bool(IGameObjectView * pGOV));
    MOCK_METHOD1(ReleaseView,
        void(IGameObjectView * pGOV));
    MOCK_METHOD1(CaptureActions,
        bool(IActionListener * pAL));
    MOCK_METHOD1(ReleaseActions,
        void(IActionListener * pAL));
    MOCK_METHOD1(CaptureProfileManager,
        bool(IGameObjectProfileManager * pPH));
    MOCK_METHOD1(ReleaseProfileManager,
        void(IGameObjectProfileManager * pPH));
    MOCK_METHOD2(EnableUpdateSlot,
        void(IGameObjectExtension * pExtension, int slot));
    MOCK_METHOD2(DisableUpdateSlot,
        void(IGameObjectExtension * pExtension, int slot));
    MOCK_METHOD2(GetUpdateSlotEnables,
        uint8(IGameObjectExtension * pExtension, int slot));
    MOCK_METHOD1(EnablePreUpdates,
        void(IGameObjectExtension * pExtension));
    MOCK_METHOD1(DisablePreUpdates,
        void(IGameObjectExtension * pExtension));
    MOCK_METHOD1(EnablePostUpdates,
        void(IGameObjectExtension * pExtension));
    MOCK_METHOD1(DisablePostUpdates,
        void(IGameObjectExtension * pExtension));
    MOCK_METHOD3(SetUpdateSlotEnableCondition,
        void(IGameObjectExtension * pExtension, int slot, EUpdateEnableCondition condition));
    MOCK_METHOD1(PostUpdate,
        void(float frameTime));
    MOCK_METHOD0(GetWorldQuery,
        IWorldQuery * ());
    MOCK_METHOD0(IsJustExchanging,
        bool());
    MOCK_METHOD0(GetMovementController,
        ILINE IMovementController * ());
    MOCK_CONST_METHOD1(GetMemoryUsage,
        void(ICrySizer * pSizer));
    MOCK_METHOD0(GetSyncDataShim,
        CSyncDataShim * ());

    // IGameObject is not a pure interface, the implementation must provide
    // a getter/setter to store an entity ID.
    void SetEntity(IEntity* entity, EntityId entityId)
    {
        m_pEntity = entity;
        m_entityId = entityId;
    }
    // ~IGameObject


    // IActionListener
    MOCK_METHOD3(OnAction, void(const ActionId&action, int activationMode, float value));
    //MOCK_METHOD0(AfterAction, void());
    // ~IActionListener


private:
    MOCK_METHOD2(ChangeExtension,
        IGameObjectExtension * (const char* extension, EChangeExtension change));
};

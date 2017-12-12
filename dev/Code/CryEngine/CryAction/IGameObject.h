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

#ifndef CRYINCLUDE_CRYACTION_IGAMEOBJECT_H
#define CRYINCLUDE_CRYACTION_IGAMEOBJECT_H
#pragma once

// FIXME: Cell SDK GCC bug workaround.
#ifndef CRYINCLUDE_IGAMEOBJECTSYSTEM_H
#include "IGameObjectSystem.h"
#endif

#define GAME_OBJECT_SUPPORTS_CUSTOM_USER_DATA 1

#include <IComponent.h>

#include "SerializeFwd.h"
#include "IActionMapManager.h"
#include "PoolAllocator.h"
#include "IFlowSystem.h"

#include "StringUtils.h"
#include "CryFixedArray.h"
#include "GameWarning.h"

struct IGameObjectExtension;
struct IGameObjectView;
struct IActionListener;
struct IMovementController;
struct IGameObjectProfileManager;
struct IWorldQuery;

enum EEntityAspects
{
    eEA_All                     = NET_ASPECT_ALL,

    #define ADD_ASPECT(x, y)   x = BIT(y),
    #include "../CryNetwork/GridMate/Compatibility/GridMateNetSerializeAspects.inl"
    #undef ADD_ASPECT
};

enum EEntityPhysicsEvents
{
    eEPE_OnCollisionLogged                      = 1 << 0, // Logged events on lower byte.
    eEPE_OnPostStepLogged                           = 1 << 1,
    eEPE_OnStateChangeLogged                    = 1 << 2,
    eEPE_OnCreateEntityPartLogged           = 1 << 3,
    eEPE_OnUpdateMeshLogged           = 1 << 4,
    eEPE_AllLogged                                      = eEPE_OnCollisionLogged | eEPE_OnPostStepLogged |
        eEPE_OnStateChangeLogged | eEPE_OnCreateEntityPartLogged |
        eEPE_OnUpdateMeshLogged,

    eEPE_OnCollisionImmediate                   = 1 << 8, // Immediate events on higher byte.
    eEPE_OnPostStepImmediate                    = 1 << 9,
    eEPE_OnStateChangeImmediate             = 1 << 10,
    eEPE_OnCreateEntityPartImmediate    = 1 << 11,
    eEPE_OnUpdateMeshImmediate        = 1 << 12,
    eEPE_AllImmediate                                   = eEPE_OnCollisionImmediate | eEPE_OnPostStepImmediate |
        eEPE_OnStateChangeImmediate | eEPE_OnCreateEntityPartImmediate |
        eEPE_OnUpdateMeshImmediate,
};

static const int MAX_UPDATE_SLOTS_PER_EXTENSION = 5;

enum ERMInvocation
{
    eRMI_ToClientChannel        =    0x01,  // Send RMI from server to a specific client
    eRMI_ToOwningClient         =    0x04,  // Send RMI from server to client that owns the actor
    eRMI_ToOtherClients         =    0x08,  // Send RMI from server to all clients except the specified client
    eRMI_ToOtherRemoteClients   =    0x10,  // Send RMI from server to all remote clients except the specified client
    eRMI_ToAllClients           =    0x20,  // Send RMI from server to all clients

    eRMI_ToServer               =   0x100,  // Send RMI from client to server

    eRMI_NoLocalCalls           =  0x1000,  // For internal use only

    // IMPORTANT: Using the RMI shim through GridMate, do not exceed 16 bits or flags will be lost in transit.

    eRMI_ToRemoteClients = eRMI_NoLocalCalls | eRMI_ToAllClients,   // Send RMI from server to all remote clients

    // Mask aggregating all bits that require dispatching to non-server clients.
    eRMI_ClientsMask     =  eRMI_ToAllClients | eRMI_ToOtherClients | eRMI_ToOtherRemoteClients | eRMI_ToOwningClient | eRMI_ToClientChannel,
};

enum EUpdateEnableCondition
{
    eUEC_Never,
    eUEC_Always,
    eUEC_Visible,
    eUEC_InRange,
    eUEC_VisibleAndInRange,
    eUEC_VisibleOrInRange,
    eUEC_VisibleOrInRangeIgnoreAI,
    eUEC_VisibleIgnoreAI,
    eUEC_WithoutAI,
};

enum EPrePhysicsUpdate
{
    ePPU_Never,
    ePPU_Always,
    ePPU_WhenAIActivated
};

enum EGameObjectAIActivationMode
{
    eGOAIAM_Never,
    eGOAIAM_Always,
    eGOAIAM_VisibleOrInRange,
    // Must be last.
    eGOAIAM_COUNT_STATES,
};

enum EAutoDisablePhysicsMode
{
    eADPM_Never,
    eADPM_WhenAIDeactivated,
    eADPM_WhenInvisibleAndFarAway,
    // Must be last.
    eADPM_COUNT_STATES,
};

enum EBindToNetworkMode
{
    eBTNM_Normal,
    eBTNM_Force,
    eBTNM_NowInitialized
};

struct SGameObjectExtensionRMI
{
    void GetMemoryUsage(ICrySizer* pSizer) const{}
    typedef struct INetAtSyncItem* (* DecoderFunction)(TSerialize, EntityId*, struct INetChannel*);

    DecoderFunction decoder;
    const char* description;
    const void* pBase;
    const struct SNetMessageDef* pMsgDef;
    ERMIAttachmentType attach;
    bool isServerCall;
    bool lowDelay;
    ENetReliabilityType reliability;
};

template <size_t N>
class CRMIAllocator
{
public:
    static ILINE void* Allocate()
    {
        ScopedSwitchToGlobalHeap useGlobalHeap;
        if (!m_pAllocator)
        {
            m_pAllocator = new stl::PoolAllocator<N>;
        }
        return m_pAllocator->Allocate();
    }
    static ILINE void Deallocate(void* p)
    {
        CRY_ASSERT(m_pAllocator);
        m_pAllocator->Deallocate(p);
    }

private:
    static stl::PoolAllocator<N>* m_pAllocator;
};
template <size_t N>
stl::PoolAllocator<N>* CRMIAllocator<N>::m_pAllocator = 0;

// Summary
//   Interface used to interact with a game object
// See Also
//   IGameObjectExtension
struct IGameObject
    : public IActionListener
{
public:
    // bind this entity to the network system (it gets synchronized then...)
    virtual bool BindToNetwork(EBindToNetworkMode mode = eBTNM_Normal) = 0;
    // bind this entity to the network system, with a dependency on it's parent
    virtual bool BindToNetworkWithParent(EBindToNetworkMode mode, EntityId parentId) = 0;
    // flag that we have changed the state of the game object aspect
    virtual void ChangedNetworkState(NetworkAspectType aspects) = 0;
    // enable/disable network aspects on game object
    virtual void EnableAspect(NetworkAspectType aspects, bool enable) = 0;
    // enable/disable delegatable aspects
    virtual void EnableDelegatableAspect(NetworkAspectType aspects, bool enable) = 0;
    // A one off call to never enable the physics aspect, this needs to be done *before* the BindToNetwork (typically in the GameObject's Init function)
    virtual void DontSyncPhysics() = 0;
    // query extension by ID.  Returns 0 if extension is not there.
    virtual IGameObjectExtension* QueryExtension(IGameObjectSystem::ExtensionID id) const = 0;
    // query extension. returns 0 if extension is not there.
    virtual IGameObjectExtension* QueryExtension(const char* extension) const = 0;

    // set extension parameters
    virtual bool SetExtensionParams(const char* extension, SmartScriptTable params) = 0;
    // get extension parameters
    virtual bool GetExtensionParams(const char* extension, SmartScriptTable params) = 0;
    // send a game object event
    virtual void SendEvent(const SGameObjectEvent&) = 0;
    // force the object to update even if extensions' slots are "sleeping"...
    virtual void ForceUpdate(bool force) = 0;
    virtual void ForceUpdateExtension(IGameObjectExtension* pGOE, int slot) = 0;
    // get/set network channel
    virtual ChannelId GetChannelId() const = 0;
    virtual void SetChannelId(ChannelId) = 0;

    // serialize some aspects of the game object
    virtual void FullSerialize(TSerialize ser) = 0;
    virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags) = 0;
    // in case things have to be set after serialization
    virtual void PostSerialize() = 0;
    // is the game object probably visible?
    virtual bool IsProbablyVisible() = 0;
    virtual bool IsProbablyDistant() = 0;
    virtual NetworkAspectType GetNetSerializeAspects() = 0;
    // change the profile of an aspect
    virtual bool SetAspectProfile(EEntityAspects aspect, uint8 profile, bool fromNetwork = false) = 0;
    virtual uint8 GetAspectProfile(EEntityAspects aspect) = 0;
    virtual IGameObjectExtension* GetExtensionWithRMIBase(const void* pBase) = 0;
    virtual void EnablePrePhysicsUpdate(EPrePhysicsUpdate updateRule) = 0;
    virtual void SetNetworkParent(EntityId id) = 0;
    virtual void Pulse(uint32 pulse) = 0;
    virtual void RegisterAsPredicted() = 0;
    virtual void RegisterAsValidated(IGameObject* pGO, int predictionHandle) = 0;
    virtual int GetPredictionHandle() = 0;

    virtual void RegisterExtForEvents(IGameObjectExtension* piExtension, const int* pEvents, const int numEvents) = 0;
    virtual void UnRegisterExtForEvents(IGameObjectExtension* piExtension, const int* pEvents, const int numEvents) = 0;

    // enable/disable sending physics events to this game object
    virtual void EnablePhysicsEvent(bool enable, int events) = 0;
    virtual bool WantsPhysicsEvent(int events) = 0;
    virtual void AttachDistanceChecker() = 0;

    // enable/disable AI activation flag
    virtual bool SetAIActivation(EGameObjectAIActivationMode mode) = 0;
    // enable/disable auto-disabling of physics
    virtual void SetAutoDisablePhysicsMode(EAutoDisablePhysicsMode mode) = 0;
    // for debugging updates
    virtual bool ShouldUpdate() = 0;

    // register a partial update in the netcode without actually serializing - useful only for working around other bugs
    virtual void RequestRemoteUpdate(NetworkAspectType aspectMask) = 0;

    virtual void SerializeSpawnInfo(TSerialize ser, IEntity* entity) = 0;

    virtual IGameObjectExtension* GetExtensionWithRMIRep(uint32 repId) = 0;

    template <class ParamsType>
    void InvokeRMI(IRMIRep& rep, ParamsType&& params, uint32 where, ChannelId channel = kInvalidChannelId)
    {
        gEnv->pNetwork->InvokeRMI(this, rep, const_cast<void*>(reinterpret_cast<const void*>(&params)), channel, where);
    }

    template <class ParamsType>
    void InvokeRMIWithDependentObject(IRMIRep& rep, ParamsType&& params, uint32 where, EntityId ent, ChannelId channel = kInvalidChannelId)
    {
        // \todo - We may need to support object-dependent RMIs in the Shim.
        gEnv->pNetwork->InvokeRMI(this, rep, const_cast<void*>(reinterpret_cast<const void*>(&params)), channel, where);
    }

    virtual void SetAuthority(bool auth) = 0;

    // turn an extension on
    ILINE bool ActivateExtension(const char* extension) { return ChangeExtension(extension, eCE_Activate) != 0; }
    // turn an extension off
    ILINE void DeactivateExtension(const char* extension) { ChangeExtension(extension, eCE_Deactivate); }
    // forcefully get a pointer to an extension (may instantiate if needed)
    ILINE IGameObjectExtension* AcquireExtension(const char* extension) { return ChangeExtension(extension, eCE_Acquire); }
    // release a previously acquired extension
    ILINE void ReleaseExtension(const char* extension) { ChangeExtension(extension, eCE_Release); }

    // retrieve the hosting entity
    ILINE IEntity* GetEntity() const
    {
        return m_pEntity;
    }

    ILINE EntityId GetEntityId() const
    {
        return m_entityId;
    }

    // for extensions to register for special things
    virtual bool CaptureView(IGameObjectView* pGOV) = 0;
    virtual void ReleaseView(IGameObjectView* pGOV) = 0;
    virtual bool CaptureActions(IActionListener* pAL) = 0;
    virtual void ReleaseActions(IActionListener* pAL) = 0;
    virtual bool CaptureProfileManager(IGameObjectProfileManager* pPH) = 0;
    virtual void ReleaseProfileManager(IGameObjectProfileManager* pPH) = 0;
    virtual void EnableUpdateSlot(IGameObjectExtension* pExtension, int slot) = 0;
    virtual void DisableUpdateSlot(IGameObjectExtension* pExtension, int slot) = 0;
    virtual uint8 GetUpdateSlotEnables(IGameObjectExtension* pExtension, int slot) = 0;
    virtual void EnablePreUpdates(IGameObjectExtension* pExtension) = 0;
    virtual void DisablePreUpdates(IGameObjectExtension* pExtension) = 0;
    virtual void EnablePostUpdates(IGameObjectExtension* pExtension) = 0;
    virtual void DisablePostUpdates(IGameObjectExtension* pExtension) = 0;
    virtual void SetUpdateSlotEnableCondition(IGameObjectExtension* pExtension, int slot, EUpdateEnableCondition condition) = 0;
    virtual void PostUpdate(float frameTime) = 0;
    virtual IWorldQuery* GetWorldQuery() = 0;

    virtual bool IsJustExchanging() = 0;

    ILINE void SetMovementController(IMovementController* pMC) { m_pMovementController = pMC; }
    virtual ILINE IMovementController* GetMovementController() { return m_pMovementController; }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const {};

#if GAME_OBJECT_SUPPORTS_CUSTOM_USER_DATA
    virtual void* GetUserData() const = 0;
    virtual void SetUserData(void* ptr) = 0;
#endif

protected:
    enum EChangeExtension
    {
        eCE_Activate,
        eCE_Deactivate,
        eCE_Acquire,
        eCE_Release
    };

    IGameObject()
        : m_pEntity(0)
        , m_entityId(0)
        , m_pMovementController(0) {}
    EntityId m_entityId;
    IMovementController* m_pMovementController;
    IEntity* m_pEntity;

private:
    // change extension activation/reference somehow
    virtual IGameObjectExtension* ChangeExtension(const char* extension, EChangeExtension change) = 0;
};

struct IGameObject;
struct SViewParams;

template <class T_Derived, class T_Parent, size_t MAX_STATIC_MESSAGES = 64>
class CGameObjectExtensionHelper
    : public T_Parent
{
public:
    CGameObjectExtensionHelper() {}

public:

    struct RMIInfo
    {
        size_t m_numMessages;
        IRMIRep* m_rmiReps[ MAX_STATIC_MESSAGES ];
    };

private:

    static RMIInfo s_RMIInfo;

public:

    static IRMIRep* RegisterRMIRep(IRMIRep* rep, uint32 extensionCrc, uint32 rmiCrc)
    {
        CRY_ASSERT_MESSAGE(s_RMIInfo.m_numMessages < MAX_STATIC_MESSAGES, "Too many static RMIs for this object.");

        if (s_RMIInfo.m_numMessages < MAX_STATIC_MESSAGES)
        {
            uint32 repId = 0;
            repId ^= extensionCrc + 0x9e3779b9 + (repId << 6) + (repId >> 2);
            repId ^= rmiCrc + 0x9e3779b9 + (repId << 6) + (repId >> 2);

            rep->SetUniqueId(repId);

            s_RMIInfo.m_rmiReps[ s_RMIInfo.m_numMessages++ ] = rep;

            auto begin = std::begin(s_RMIInfo.m_rmiReps);
            auto end = begin + s_RMIInfo.m_numMessages;
            std::sort(begin, end,
                [](const IRMIRep* lhs, const IRMIRep* rhs)
                {
                    return lhs->GetUniqueId() < rhs->GetUniqueId();
                }
                );

            return rep;
        }

        return nullptr;
    }

    bool HasRMIRep(uint32 repId) const override
    {
        return !!GetRMIRep(repId);
    }

    IRMIRep* GetRMIRep(uint32 repId) const override
    {
        auto begin = std::begin(s_RMIInfo.m_rmiReps);
        auto end = begin + s_RMIInfo.m_numMessages;
        auto iter = std::lower_bound(begin, end, repId,
                [](const IRMIRep* rmiRep, uint32 rmiRepId)
                {
                    return rmiRep->GetUniqueId() < rmiRepId;
                }
                );

        if (iter != end && (*iter)->GetUniqueId() == repId)
        {
            return *iter;
        }

        return nullptr;
    }

    static void GetGameObjectExtensionRMIData(void** ppRMI, size_t* nCount) {}

    const void* GetRMIBase() const override { return nullptr; }
};

/// In Gridmate, all RPCs are reliable ordered, so the reliability macro parameter
/// is irrelevant, but exists for backward compatibility with existing RMI declarations.
/// The same applies for attachment.
#define DECLARE_RMI(name, paramsType, reliability, attachment, isServer, lowDelay) \
public:                                                                            \
    template<typename T>                                                           \
    class RMIRep_##name                                                            \
        : public IRMIRep                                                           \
    {                                                                              \
    public:                                                                        \
        typedef paramsType ParamsType;                                             \
        typedef bool (T::* InvocationHandler)(const ParamsType& p);                \
        InvocationHandler m_handler;                                               \
        RMIRep_##name(InvocationHandler handler)                                   \
            : m_handler(handler) {}                                                \
        bool IsServerRMI() const override { return isServer; }                     \
        const char* GetDebugName() const override                                  \
        {                                                                          \
            return #name;                                                          \
        }                                                                          \
        void SerializeParamsToBuffer(TSerialize ser, void* params) override        \
        {                                                                          \
            ParamsType* p = reinterpret_cast<ParamsType*>(params);                 \
            p->SerializeWith(ser);                                                 \
        }                                                                          \
        void* SerializeParamsFromBuffer(TSerialize ser) override                   \
        {                                                                          \
            CRY_ASSERT(gEnv->mMainThreadId == CryGetCurrentThreadId());            \
            static ParamsType p;                                                   \
            p.SerializeWith(ser);                                                  \
            return &p;                                                             \
        }                                                                          \
        void Invoke(IGameObjectExtension * extension, void* params) override       \
        {                                                                          \
            ParamsType* p = reinterpret_cast<ParamsType*>(params);                 \
            CRY_ASSERT(m_handler);                                                 \
            (static_cast<T*>(extension)->*m_handler)(*p);                          \
        }                                                                          \
    };                                                                             \
    static IRMIRep* RMI_##name;                                                    \
    bool RMIHandler_##name(const paramsType&params);                               \
    static IRMIRep& name() { return *RMI_##name; }

#define IMPLEMENT_RMI(cls, name)                                   \
    cls::RMIRep_##name<cls> rep_##name(&cls::RMIHandler_##name);   \
    IRMIRep* cls::RMI_##name = &rep_##name;                        \
    IRMIRep* reg_##cls##_##name = cls::RegisterRMIRep(&rep_##name, \
            CryStringUtils::CalculateHashLowerCase(#cls),          \
            CryStringUtils::CalculateHashLowerCase(#name));        \
    bool cls::RMIHandler_##name(const cls::RMIRep_##name<cls>::ParamsType & params)

/*
 * _FAST versions may send the RMI without waiting for the frame to end; be sure that consistency with the entity is not important!
 */

//
// PreAttach/PostAttach RMI's cannot have their reliability specified (see CGameObjectDispatch::RegisterInterface() for details)
#define DECLARE_SERVER_RMI_PREATTACH(name, params) DECLARE_RMI(name, params, eNRT_UnreliableOrdered, eRAT_PreAttach, true, false)
#define DECLARE_CLIENT_RMI_PREATTACH(name, params) DECLARE_RMI(name, params, eNRT_UnreliableOrdered, eRAT_PreAttach, false, false)
#define DECLARE_SERVER_RMI_POSTATTACH(name, params) DECLARE_RMI(name, params, eNRT_UnreliableOrdered, eRAT_PostAttach, true, false)
#define DECLARE_CLIENT_RMI_POSTATTACH(name, params) DECLARE_RMI(name, params, eNRT_UnreliableOrdered, eRAT_PostAttach, false, false)
#define DECLARE_SERVER_RMI_NOATTACH(name, params, reliability) DECLARE_RMI(name, params, reliability, eRAT_NoAttach, true, false)
#define DECLARE_CLIENT_RMI_NOATTACH(name, params, reliability) DECLARE_RMI(name, params, reliability, eRAT_NoAttach, false, false)

// PreAttach/PostAttach RMI's cannot have their reliability specified (see CGameObjectDispatch::RegisterInterface() for details)
#define DECLARE_SERVER_RMI_PREATTACH_FAST(name, params) DECLARE_RMI(name, params, eNRT_UnreliableOrdered, eRAT_PreAttach, true, true)
#define DECLARE_CLIENT_RMI_PREATTACH_FAST(name, params) DECLARE_RMI(name, params, eNRT_UnreliableOrdered, eRAT_PreAttach, false, true)
#define DECLARE_SERVER_RMI_POSTATTACH_FAST(name, params) DECLARE_RMI(name, params, eNRT_UnreliableOrdered, eRAT_PostAttach, true, true)
#define DECLARE_CLIENT_RMI_POSTATTACH_FAST(name, params) DECLARE_RMI(name, params, eNRT_UnreliableOrdered, eRAT_PostAttach, false, true)
#define DECLARE_SERVER_RMI_NOATTACH_FAST(name, params, reliability) DECLARE_RMI(name, params, reliability, eRAT_NoAttach, true, true)
#define DECLARE_CLIENT_RMI_NOATTACH_FAST(name, params, reliability) DECLARE_RMI(name, params, reliability, eRAT_NoAttach, false, true)

#if ENABLE_URGENT_RMIS
#define DECLARE_SERVER_RMI_URGENT(name, params, reliability) DECLARE_RMI(name, params, reliability, eRAT_Urgent, true, false)
#define DECLARE_CLIENT_RMI_URGENT(name, params, reliability) DECLARE_RMI(name, params, reliability, eRAT_Urgent, false, false)
#else
#define DECLARE_SERVER_RMI_URGENT(name, params, reliability) DECLARE_SERVER_RMI_NOATTACH(name, params, reliability)
#define DECLARE_CLIENT_RMI_URGENT(name, params, reliability) DECLARE_CLIENT_RMI_NOATTACH(name, params, reliability)
#endif // ENABLE_URGENT_RMIS

#if ENABLE_INDEPENDENT_RMIS
#define DECLARE_SERVER_RMI_INDEPENDENT(name, params, reliability) DECLARE_RMI(name, params, reliability, eRAT_Independent, true, false)
#define DECLARE_CLIENT_RMI_INDEPENDENT(name, params, reliability) DECLARE_RMI(name, params, reliability, eRAT_Independent, false, false)
#else
#define DECLARE_SERVER_RMI_INDEPENDENT(name, params, reliability) DECLARE_SERVER_RMI_NOATTACH(name, params, reliability)
#define DECLARE_CLIENT_RMI_INDEPENDENT(name, params, reliability) DECLARE_CLIENT_RMI_NOATTACH(name, params, reliability)
#endif // ENABLE_INDEPENDENT_RMIS

/*
// Todo:
//      Temporary, until a good solution for sending noattach fast messages can be found
#define DECLARE_SERVER_RMI_NOATTACH_FAST(a,b,c) DECLARE_SERVER_RMI_NOATTACH(a,b,c)
#define DECLARE_CLIENT_RMI_NOATTACH_FAST(a,b,c) DECLARE_CLIENT_RMI_NOATTACH(a,b,c)
*/


//
#define DECLARE_INTERFACE_SERVER_RMI_PREATTACH(name, params, reliability) DECLARE_INTERFACE_RMI(name, params, reliability, eRAT_PreAttach, true, false)
#define DECLARE_INTERFACE_CLIENT_RMI_PREATTACH(name, params, reliability) DECLARE_INTERFACE_RMI(name, params, reliability, eRAT_PreAttach, false, false)
#define DECLARE_INTERFACE_SERVER_RMI_POSTATTACH(name, params, reliability) DECLARE_INTERFACE_RMI(name, params, reliability, eRAT_PostAttach, true, false)
#define DECLARE_INTERFACE_CLIENT_RMI_POSTATTACH(name, params, reliability) DECLARE_INTERFACE_RMI(name, params, reliability, eRAT_PostAttach, false, false)

#define DECLARE_INTERFACE_SERVER_RMI_PREATTACH_FAST(name, params, reliability) DECLARE_INTERFACE_RMI(name, params, reliability, eRAT_PreAttach, true, true)
#define DECLARE_INTERFACE_CLIENT_RMI_PREATTACH_FAST(name, params, reliability) DECLARE_INTERFACE_RMI(name, params, reliability, eRAT_PreAttach, false, true)
#define DECLARE_INTERFACE_SERVER_RMI_POSTATTACH_FAST(name, params, reliability) DECLARE_INTERFACE_RMI(name, params, reliability, eRAT_PostAttach, true, true)
#define DECLARE_INTERFACE_CLIENT_RMI_POSTATTACH_FAST(name, params, reliability) DECLARE_INTERFACE_RMI(name, params, reliability, eRAT_PostAttach, false, true)

template <class T, class U, size_t N>
typename CGameObjectExtensionHelper<T, U, N>::RMIInfo CGameObjectExtensionHelper<T, U, N>::s_RMIInfo;

struct IGameObjectView
{
    virtual ~IGameObjectView(){}
    virtual void UpdateView(SViewParams& params) = 0;
    virtual void PostUpdateView(SViewParams& params) = 0;
};

struct IGameObjectProfileManager
{
    virtual ~IGameObjectProfileManager(){}
    virtual bool SetAspectProfile(EEntityAspects aspect, uint8 profile) = 0;
    virtual uint8 GetDefaultProfile(EEntityAspects aspect) = 0;
};

// Summary
//   Interface used to implement a game object extension
struct IGameObjectExtension
    : public IComponent
{
    virtual ~IGameObjectExtension(){}
    virtual void GetMemoryUsage(ICrySizer* pSizer) const { }

    IGameObjectExtension()
        : m_pGameObject(0)
        , m_entityId(0)
        , m_pEntity(0) {}

    // IComponent

    // Default implementations
    ComponentEventPriority GetEventPriority(const int eventID) const override { return EntityEventPriority::User; }
    void UpdateComponent(SEntityUpdateContext& ctx) override {}

    // Until IGameObjectExtension is truly no different than a vanilla IComponent,
    // IGameObjectExtensions should not implement the functions marked final.
    bool InitComponent(IEntity* pEntity, SEntitySpawnParams& params) final { return true; }
    void Done() final {}
    // ~IComponent

    // Summary
    //   Initialize the extension
    // Parameters
    //   pGameObject - a pointer to the game object which will use the extension
    // Remarks
    //   IMPORTANT: It's very important that the implementation of this function
    //   call the protected function SetGameObject() during the execution of the
    //   Init() function. Unexpected results would happen otherwise.
    virtual bool Init(IGameObject* pGameObject) = 0;

    // Summary
    //   Post-initialize the extension
    // Description
    //   During the post-initialization, the extension is now contained in the
    //   game object
    // Parameters
    //   pGameObject - a pointer to the game object which owns the extension
    virtual void PostInit(IGameObject* pGameObject) { }

    // Summary
    //   Initialize the extension (client only)
    // Description
    //   This initialization function should be use to initialize resource only
    //   used in the client
    // Parameters
    //   channelId - id of the server channel of the client to receive the
    //               initialization
    virtual void InitClient(ChannelId channelId) { }

    // Summary
    //   Post-initialize the extension (client only)
    // Description
    //   This initialization function should be use to initialize resource only
    //   used in the client. During the post-initialization, the extension is now
    //   contained in the game object
    // Parameters
    //   channelId - id of the server channel of the client to receive the
    //               initialization
    virtual void PostInitClient(ChannelId channelId) { }

    // Summary
    //   Reload the extension
    // Description
    //   Called when owning entity is reloaded
    // Parameters
    //   pGameObject - a pointer to the game object which owns the extension
    // Returns
    //   TRUE if the extension should be kept, FALSE if it should be removed
    // Remarks
    //   IMPORTANT: It's very important that the implementation of this function
    //   call the protected function ResetGameObject() during the execution of the
    //   ReloadExtension() function. Unexpected results would happen otherwise.
    virtual bool ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) { return true; }

    // Summary
    //   Post-reload the extension
    // Description
    //   Called when owning entity is reloaded and all its extensions have either
    //   either been reloaded or destroyed
    // Parameters
    //   pGameObject - a pointer to the game object which owns the extension
    virtual void PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) { }

    // Summary
    //   Builds a signature to describe the dynamic hierarchy of the parent Entity container
    // Arguments:
    //    signature - the object to serialize with, forming the signature
    // Returns:
    //    true - If the signature is thus far valid
    // Note:
    //    It's the responsibility of the proxy to identify its internal state which may complicate the hierarchy
    //    of the parent Entity i.e., sub-proxies and which actually exist for this instantiation.
    virtual bool GetEntityPoolSignature(TSerialize signature) { return false; }

    // Summary
    //   Performs the serialization the extension
    // Parameters
    //   ser - object used to serialize values
    //   aspect - serialization aspect, used for network serialization
    //   profile - which profile to serialize; 255 == don't care
    //   flags - physics flags to be used for serialization
    // See Also
    //   ISerialize
    virtual void FullSerialize(TSerialize ser) { }
    virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags) { return true; }

    // Summary
    //   Return the aspects NetSerialize serializes.
    //   Overriding this to return only the aspects used will speed up the net bind of the object.
    virtual NetworkAspectType GetNetSerializeAspects() { return eEA_All; }

    // Summary
    //   Performs post serialization fixes
    virtual void PostSerialize() { }

    // Summary
    //   Performs the serialization of special spawn information
    // Parameters
    //   ser - object used to serialize values
    // See Also
    //   Serialize, ISerialize
    virtual void SerializeSpawnInfo(TSerialize ser, IEntity* entity) { }

    virtual ISerializableInfoPtr GetSpawnInfo() { return nullptr; }

    // Summary
    //   Performs frame dependent extension updates
    // Parameters
    //   ctx - Update context
    //   updateSlot - updateSlot
    // See Also
    //   PostUpdate, SEntityUpdateContext, IGameObject::EnableUpdateSlot
    virtual void Update(SEntityUpdateContext& ctx, int updateSlot) { }

    // Summary
    //   Processes game specific events
    // Parameters
    //   event - game event
    // See Also
    //   SGameObjectEvent
    virtual void HandleEvent(const SGameObjectEvent& event) { }

    // Summary
    //   Processes entity specific events.
    //   This is distinct from ProcessEvent(), which is called on all IComponents.
    // Parameters
    //   event - entity event, see SEntityEvent for more information
    virtual void ProcessEvent(SEntityEvent& event) = 0;

    virtual void SetChannelId(ChannelId id) { }
    virtual void SetAuthority(bool auth) { }

    // Summary
    //   Retrieves the RMI Base pointer
    // Description
    //   Internal function used for RMI. It's usually implemented by
    //   CGameObjectExtensionHelper provides a way of checking who should
    //   receive some RMI call.
    virtual const void* GetRMIBase() const { return nullptr; }

    // Summary
    //   Performs an update at the start of the frame, must call IGameObject::EnablePreUpdates
    //   to receive this call
    // Parameters
    //   frameTime - time elapsed since the last frame update
    // See Also
    //   Update, IGameObject::EnablePreUpdates, IGameObject::DisablePreUpdates
    virtual void PreUpdate(float frameTime) { }

    // Summary
    //   Performs an additional update
    // Parameters
    //   frameTime - time elapsed since the last frame update
    // See Also
    //   Update, IGameObject::EnablePostUpdates, IGameObject::DisablePostUpdates
    virtual void PostUpdate(float frameTime) { }

    // Summary
    virtual void PostRemoteSpawn() { }

    // Summary
    //   Retrieves the pointer to the game object
    // Returns
    //   A pointer to the game object which hold this extension
    ILINE IGameObject* GetGameObject() const { return m_pGameObject; }

    // Summary
    //   Retrieves the pointer to the entity
    // Returns
    //   A pointer to the entity which hold this game object extension
    ILINE IEntity* GetEntity() const { return m_pEntity; }

    // Summary
    //   Retrieves the EntityId
    // Returns
    //   An EntityId to the entity which hold this game object extension
    ILINE EntityId GetEntityId() const { return m_entityId; }

    virtual bool HasRMIRep(uint32 repId) const = 0;
    virtual IRMIRep* GetRMIRep(uint32 repId) const = 0;

protected:
    void SetGameObject(IGameObject* pGameObject)
    {
        m_pGameObject = pGameObject;
        if (pGameObject)
        {
            m_pEntity = pGameObject->GetEntity();
            m_entityId = pGameObject->GetEntityId();
        }
    }

    void ResetGameObject()
    {
        m_pEntity = (m_pGameObject ? m_pGameObject->GetEntity() : 0);
        m_entityId = (m_pGameObject ? m_pGameObject->GetEntityId() : 0);
    }

private:
    IGameObject* m_pGameObject;
    EntityId m_entityId;
    IEntity* m_pEntity;
};

DECLARE_COMPONENT_POINTERS(IGameObjectExtension);

#define CHANGED_NETWORK_STATE(object, aspects)       do { /* IEntity * pEntity = object->GetGameObject()->GetEntity(); CryLogAlways("%s changed aspect %x (%s %d)", pEntity ? pEntity->GetName() : "NULL", aspects, __FILE__, __LINE__); */ object->GetGameObject()->ChangedNetworkState(aspects); } while (0)
#define CHANGED_NETWORK_STATE_GO(object, aspects)        do { /* IEntity * pEntity = object->GetEntity(); CryLogAlways("%s changed aspect %x (%s %d)", pEntity ? pEntity->GetName() : "NULL", aspects, __FILE__, __LINE__); */ object->ChangedNetworkState(aspects); } while (0)
#define CHANGED_NETWORK_STATE_REF(object, aspects)       do { /* IEntity * pEntity = object.GetGameObject()->GetEntity(); CryLogAlways("%s changed aspect %x (%s %d)", pEntity ? pEntity->GetName() : "NULL", aspects, __FILE__, __LINE__); */ object.GetGameObject()->ChangedNetworkState(aspects); } while (0)

#endif // CRYINCLUDE_CRYACTION_IGAMEOBJECT_H

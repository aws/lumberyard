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

// Description : Implementation of IAnimSequence interface.


#include "StdAfx.h"

#include <Maestro/Bus/EditorSequenceComponentBus.h>

#include "AnimSequence.h"
#include "EntityNode.h"
#include "AnimAZEntityNode.h"
#include "AnimComponentNode.h"
#include "CVarNode.h"
#include "ScriptVarNode.h"
#include "AnimCameraNode.h"
#include "SceneNode.h"
#include "StlUtils.h"
#include "MaterialNode.h"
#include "EventNode.h"
#include "LayerNode.h"
#include "CommentNode.h"
#include "AnimPostFXNode.h"
#include "AnimScreenFaderNode.h"
#include "I3DEngine.h"
#include "AnimLightNode.h"
#include "AnimGeomCacheNode.h"
#include "ShadowsSetupNode.h"
#include "AnimEnvironmentNode.h"
#include "SequenceTrack.h"

#include "IScriptSystem.h"
#include "Components/IComponentRender.h"

//////////////////////////////////////////////////////////////////////////
CAnimSequence::CAnimSequence(IMovieSystem* pMovieSystem, uint32 id, ESequenceType sequenceType)
    : m_refCount(0)
{
    m_lastGenId = 1;
    m_pMovieSystem = pMovieSystem;
    m_flags = 0;
    m_pParentSequence = NULL;
    m_timeRange.Set(0, 10);
    m_bPaused = false;
    m_bActive = false;
    m_pOwner = NULL;
    m_pActiveDirector = NULL;
    m_precached = false;
    m_bResetting = false;
    m_sequenceType = sequenceType;
    m_time = -FLT_MAX;
    SetId(id);

    m_pEventStrings = aznew CAnimStringTable;
}

//////////////////////////////////////////////////////////////////////////
CAnimSequence::CAnimSequence()
    : CAnimSequence((gEnv) ? gEnv->pMovieSystem : nullptr, 0, eSequenceType_SequenceComponent)
{
}

CAnimSequence::~CAnimSequence()
{
    // clear reference to me from all my nodes
    for (int i = m_nodes.size(); --i >= 0;)
    {
        if (m_nodes[i])
        {
            m_nodes[i]->SetSequence(nullptr);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::add_ref()
{
    ++m_refCount;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::release()
{
    if (--m_refCount <= 0)
    {
        delete this;
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::SetName(const char* name)
{
    if (!m_pMovieSystem)
    {
        return;   // should never happen, null pointer guard
    }

    string originalName = GetName();

    m_name = name;
    m_pMovieSystem->OnSequenceRenamed(originalName, m_name.c_str());

    // the sequence named LIGHT_ANIMATION_SET_NAME is a singleton sequence to hold all light animations.
    if (m_name == LIGHT_ANIMATION_SET_NAME)
    {
        // ensure it stays a singleton. If one already exists, deregister it.
        if (CLightAnimWrapper::GetLightAnimSet())
        {
            CLightAnimWrapper::InvalidateAllNodes();
            CLightAnimWrapper::SetLightAnimSet(0);
        }

        CLightAnimWrapper::SetLightAnimSet(this);
    }
}

//////////////////////////////////////////////////////////////////////////
const char* CAnimSequence::GetName() const
{
    return m_name.c_str();
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::ResetId()
{
    if (!m_pMovieSystem)
    {
        return;   // should never happen, null pointer guard
    }

    SetId(m_pMovieSystem->GrabNextSequenceId());
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::SetFlags(int flags)
{
    m_flags = flags;
}

//////////////////////////////////////////////////////////////////////////
int CAnimSequence::GetFlags() const
{
    return m_flags;
}

//////////////////////////////////////////////////////////////////////////
int CAnimSequence::GetCutSceneFlags(const bool localFlags) const
{
    int currentFlags = m_flags & (eSeqFlags_NoHUD | eSeqFlags_NoPlayer | eSeqFlags_NoGameSounds | eSeqFlags_NoAbort);

    if (m_pParentSequence != NULL)
    {
        if (localFlags == true)
        {
            currentFlags &= ~m_pParentSequence->GetCutSceneFlags();
        }
        else
        {
            currentFlags |= m_pParentSequence->GetCutSceneFlags();
        }
    }

    return currentFlags;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::SetParentSequence(IAnimSequence* pParentSequence)
{
    m_pParentSequence = pParentSequence;
}

//////////////////////////////////////////////////////////////////////////
const IAnimSequence* CAnimSequence::GetParentSequence() const
{
    return m_pParentSequence;
}

//////////////////////////////////////////////////////////////////////////
int CAnimSequence::GetNodeCount() const
{
    return m_nodes.size();
}

//////////////////////////////////////////////////////////////////////////
IAnimNode* CAnimSequence::GetNode(int index) const
{
    assert(index >= 0 && index < (int)m_nodes.size());
    return m_nodes[index].get();
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSequence::AddNode(IAnimNode* pAnimNode)
{
    assert(pAnimNode != 0);

    pAnimNode->SetSequence(this);
    pAnimNode->SetTimeRange(m_timeRange);

    // Check if this node already in sequence. If found, don't add it again.
    bool found = false;
    for (int i = 0; i < (int)m_nodes.size(); i++)
    {
        if (pAnimNode == m_nodes[i].get())
        {
            found = true;
            break;
        }
    }
    if (!found)
    {
        m_nodes.push_back(AZStd::intrusive_ptr<IAnimNode>(pAnimNode));
    }

    const int nodeId = static_cast<CAnimNode*>(pAnimNode)->GetId();
    if (nodeId >= (int)m_lastGenId)
    {
        m_lastGenId = nodeId + 1;
    }

    if (pAnimNode->NeedToRender())
    {
        AddNodeNeedToRender(pAnimNode);
    }

    bool bNewDirectorNode = m_pActiveDirector == NULL && pAnimNode->GetType() == eAnimNodeType_Director;
    if (bNewDirectorNode)
    {
        m_pActiveDirector = pAnimNode;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
IAnimNode* CAnimSequence::CreateNodeInternal(EAnimNodeType nodeType, uint32 nNodeId)
{
    if (!m_pMovieSystem)
    {
        return nullptr;   // should never happen, null pointer guard
    }

    CAnimNode* pAnimNode = NULL;

    if (nNodeId == -1)
    {
        nNodeId = m_lastGenId;
    }

    switch (nodeType)
    {
        case eAnimNodeType_Entity:
            // legacy entities are only allowed to be added to legacy sequences
            if (m_sequenceType == eSequenceType_Legacy)
            {
                pAnimNode = aznew CAnimEntityNode(nNodeId, eAnimNodeType_Entity);
            }
            else
            {
                m_pMovieSystem->LogUserNotificationMsg("Object Entities (Legacy) are not supported in Component Entity Sequences. Skipping...");
            }
            break;
        case eAnimNodeType_AzEntity:
            // AZ entities are only allowed to be added to SequenceComponent sequences
            if (m_sequenceType == eSequenceType_SequenceComponent)
            {
                pAnimNode = aznew CAnimAzEntityNode(nNodeId);
            }
            else
            {
                m_pMovieSystem->LogUserNotificationMsg("Component Entities are not supported in Object Entity Sequences (Legacy). Skipping...");
            }
            break;
        case eAnimNodeType_Component:
            // Components are only allowed to be added to SequenceComponent sequences
            if (m_sequenceType == eSequenceType_SequenceComponent)
            {
                pAnimNode = aznew CAnimComponentNode(nNodeId);
            }
            else
            {
                m_pMovieSystem->LogUserNotificationMsg("Component Nodes are not supported in Object Entity Sequences (Legacy). Skipping...");
            }
            break;
        case eAnimNodeType_Camera:
            // legacy cameras are only allowed to be added to legacy sequences
            if (m_sequenceType == eSequenceType_Legacy)
            {
                pAnimNode = aznew CAnimCameraNode(nNodeId);
            }
            else
            {
                m_pMovieSystem->LogUserNotificationMsg("Object Entities (Legacy) are not supported in Component Entity Sequences. Skipping...");
            }
            break;
        case eAnimNodeType_CVar:
            pAnimNode = aznew CAnimCVarNode(nNodeId);
            break;
        case eAnimNodeType_ScriptVar:
            pAnimNode = aznew CAnimScriptVarNode(nNodeId);
            break;
        case eAnimNodeType_Director:
            pAnimNode = aznew CAnimSceneNode(nNodeId);
            break;
        case eAnimNodeType_Material:
            pAnimNode = aznew CAnimMaterialNode(nNodeId);
            break;
        case eAnimNodeType_Event:
            pAnimNode = aznew CAnimEventNode(nNodeId);
            break;
        case eAnimNodeType_Group:
            pAnimNode = aznew CAnimNodeGroup(nNodeId);
            break;
        case  eAnimNodeType_Layer:
            pAnimNode = aznew CLayerNode(nNodeId);
            break;
        case eAnimNodeType_Comment:
            pAnimNode = aznew CCommentNode(nNodeId);
            break;
        case eAnimNodeType_RadialBlur:
        case eAnimNodeType_ColorCorrection:
        case eAnimNodeType_DepthOfField:
            pAnimNode = CAnimPostFXNode::CreateNode(nNodeId, nodeType);
            break;
        case eAnimNodeType_ShadowSetup:
            pAnimNode = aznew CShadowsSetupNode(nNodeId);
            break;
        case eAnimNodeType_ScreenFader:
            pAnimNode = aznew CAnimScreenFaderNode(nNodeId);
            break;
        case eAnimNodeType_Light:
            pAnimNode = aznew CAnimLightNode(nNodeId);
            break;
#if defined(USE_GEOM_CACHES)
        case eAnimNodeType_GeomCache:
            pAnimNode = aznew CAnimGeomCacheNode(nNodeId);
            break;
#endif
        case eAnimNodeType_Environment:
            pAnimNode = aznew CAnimEnvironmentNode(nNodeId);
            break;
        default:     
            m_pMovieSystem->LogUserNotificationMsg("AnimNode cannot be added because it is an unsupported object type.");
            break;
    }

    if (pAnimNode)
    {
        AddNode(pAnimNode);
    }

    return pAnimNode;
}

//////////////////////////////////////////////////////////////////////////
IAnimNode* CAnimSequence::CreateNode(EAnimNodeType nodeType)
{
    return CreateNodeInternal(nodeType);
}

//////////////////////////////////////////////////////////////////////////
IAnimNode* CAnimSequence::CreateNode(XmlNodeRef node)
{
    if (!GetMovieSystem())
    {
        return 0;   // should never happen, null pointer guard
    }

    EAnimNodeType type;
    GetMovieSystem()->SerializeNodeType(type, node, true, IAnimSequence::kSequenceVersion, 0);

    XmlString name;
    if (!node->getAttr("Name", name))
    {
        return 0;
    }

    IAnimNode* pNewNode = CreateNode(type);
    if (!pNewNode)
    {
        return 0;
    }

    pNewNode->SetName(name);
    pNewNode->Serialize(node, true, true);

    return pNewNode;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::RemoveNode(IAnimNode* node, bool removeChildRelationships)
{
    assert(node != 0);

    static_cast<CAnimNode*>(node)->Activate(false);
    static_cast<CAnimNode*>(node)->OnReset();

    for (int i = 0; i < (int)m_nodes.size(); )
    {
        if (node == m_nodes[i].get())
        {
            m_nodes.erase(m_nodes.begin() + i);

            if (node->NeedToRender())
            {
                RemoveNodeNeedToRender(node);
            }

            continue;
        }
        if (removeChildRelationships && m_nodes[i]->GetParent() == node)
        {
            m_nodes[i]->SetParent(0);
        }

        i++;
    }

    // Request the node removal from the SequenceComponent if it's an AZ::Entity
    if (m_sequenceType == eSequenceType_SequenceComponent)
    {
        AZ::EntityId removedNodeId = node->GetAzEntityId();
        if (removedNodeId.IsValid() && m_ownerId.IsValid())
        {
            Maestro::EditorSequenceComponentRequestBus::Event(m_ownerId, &Maestro::EditorSequenceComponentRequestBus::Events::RemoveEntityToAnimate, removedNodeId);
        }
    }

    // The removed one was the active director node.
    if (m_pActiveDirector == node)
    {
        // Clear the active one.
        m_pActiveDirector = NULL;
        // If there is another director node, set it as active.
        for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
        {
            IAnimNode* pNode = it->get();
            if (pNode->GetType() == eAnimNodeType_Director)
            {
                SetActiveDirector(pNode);
                break;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::RemoveAll()
{
    stl::free_container(m_nodes);
    stl::free_container(m_events);
    stl::free_container(m_nodesNeedToRender);
    m_pActiveDirector = NULL;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Reset(bool bSeekToStart)
{
    if (GetFlags() & eSeqFlags_LightAnimationSet)
    {
        return;
    }

    m_precached = false;
    m_bResetting = true;

    if (!bSeekToStart)
    {
        for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
        {
            IAnimNode* pAnimNode = it->get();
            static_cast<CAnimNode*>(pAnimNode)->OnReset();
        }
        m_bResetting = false;
        return;
    }

    bool bWasActive = m_bActive;

    if (!bWasActive)
    {
        Activate();
    }

    SAnimContext ec;
    ec.bSingleFrame = true;
    ec.bResetting = true;
    ec.pSequence = this;
    ec.time = m_timeRange.start;
    Animate(ec);

    if (!bWasActive)
    {
        Deactivate();
    }
    else
    {
        for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
        {
            IAnimNode* pAnimNode = it->get();
            static_cast<CAnimNode*>(pAnimNode)->OnReset();
        }
    }

    m_bResetting = false;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::ResetHard()
{
    if (GetFlags() & eSeqFlags_LightAnimationSet)
    {
        return;
    }

    m_bResetting = true;

    bool bWasActive = m_bActive;

    if (!bWasActive)
    {
        Activate();
    }

    SAnimContext ec;
    ec.bSingleFrame = true;
    ec.bResetting = true;
    ec.pSequence = this;
    ec.time = m_timeRange.start;
    Animate(ec);

    if (!bWasActive)
    {
        Deactivate();
    }
    else
    {
        for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
        {
            IAnimNode* pAnimNode = it->get();
            static_cast<CAnimNode*>(pAnimNode)->OnResetHard();
        }
    }

    m_bResetting = false;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Pause()
{
    if (GetFlags() & eSeqFlags_LightAnimationSet || m_bPaused)
    {
        return;
    }

    m_bPaused = true;

    // Detach animation block from all nodes in this sequence.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* pAnimNode = it->get();
        static_cast<CAnimNode*>(pAnimNode)->OnPause();
    }

    if (GetSequenceType() == eSequenceType_SequenceComponent)
    {
        // Notify EBus listeners
        Maestro::SequenceComponentNotificationBus::Event(GetOwnerId(), &Maestro::SequenceComponentNotificationBus::Events::OnPause);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Resume()
{
    if (GetFlags() & eSeqFlags_LightAnimationSet)
    {
        return;
    }

    if (m_bPaused)
    {
        m_bPaused = false;

        for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
        {
            IAnimNode* pAnimNode = it->get();
            static_cast<CAnimNode*>(pAnimNode)->OnResume();
        }

        if (GetSequenceType() == eSequenceType_SequenceComponent)
        {
            // Notify EBus listeners
            Maestro::SequenceComponentNotificationBus::Event(GetOwnerId(), &Maestro::SequenceComponentNotificationBus::Events::OnResume);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::OnLoop()
{
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* pAnimNode = it->get();
        static_cast<CAnimNode*>(pAnimNode)->OnLoop();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSequence::IsPaused() const
{
    return m_bPaused;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::OnStart()
{
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* pAnimNode = it->get();
        static_cast<CAnimNode*>(pAnimNode)->OnStart();
    }

    if (GetSequenceType() == eSequenceType_SequenceComponent)
    {
        // notify listeners
        Maestro::SequenceComponentNotificationBus::Event(GetOwnerId(), &Maestro::SequenceComponentNotificationBus::Events::OnStart, m_time);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::OnStop()
{
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* pAnimNode = it->get();
        static_cast<CAnimNode*>(pAnimNode)->OnStop();
    }

    if (GetSequenceType() == eSequenceType_SequenceComponent)
    {
        // notify listeners
        Maestro::SequenceComponentNotificationBus::Event(GetOwnerId(), &Maestro::SequenceComponentNotificationBus::Events::OnStop, m_time);
    }
}

void CAnimSequence::TimeChanged(float newTime)
{
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* pAnimNode = it->get();
        pAnimNode->TimeChanged(newTime);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::StillUpdate()
{
    if (GetFlags() & eSeqFlags_LightAnimationSet)
    {
        return;
    }

    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* pAnimNode = it->get();
        pAnimNode->StillUpdate();
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Animate(const SAnimContext& ec)
{
    assert(m_bActive);

    if (GetFlags() & eSeqFlags_LightAnimationSet)
    {
        return;
    }

#if !defined(_RELEASE)
    if (CMovieSystem::m_mov_DebugEvents)
    {
        gEnv->pScriptSystem->SetGlobalValue("CurrentCinematicName", m_name.c_str());
    }
#endif

    SAnimContext animContext = ec;
    animContext.pSequence = this;
    m_time = animContext.time;

    // Evaluate all animation nodes in sequence.
    // The director first.
    if (m_pActiveDirector)
    {
        m_pActiveDirector->Animate(animContext);
    }

    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        // Make sure correct animation block is binded to node.
        IAnimNode* pAnimNode = it->get();

        // All other (inactive) director nodes are skipped.
        if (pAnimNode->GetType() == eAnimNodeType_Director)
        {
            continue;
        }

        // If this is a descendant of a director node and that director is currently not active, skip this one.
        IAnimNode* pParentDirector = pAnimNode->HasDirectorAsParent();
        if (pParentDirector && pParentDirector != m_pActiveDirector)
        {
            continue;
        }

        if (pAnimNode->AreFlagsSetOnNodeOrAnyParent(eAnimNodeFlags_Disabled))
        {
            continue;
        }

        // Animate node.
        pAnimNode->Animate(animContext);
    }
#if !defined(_RELEASE)
    if (CMovieSystem::m_mov_DebugEvents)
    {
        gEnv->pScriptSystem->SetGlobalToNull("CurrentCinematicName");
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Render()
{
    for (AnimNodes::iterator it = m_nodesNeedToRender.begin(); it != m_nodesNeedToRender.end(); ++it)
    {
        IAnimNode* pAnimNode = it->get();
        pAnimNode->Render();
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Activate()
{
    if (m_bActive)
    {
        return;
    }

    m_bActive = true;
    // Assign animation block to all nodes in this sequence.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* pAnimNode = it->get();
        static_cast<CAnimNode*>(pAnimNode)->OnReset();
        static_cast<CAnimNode*>(pAnimNode)->Activate(true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Deactivate()
{
    if (!m_bActive)
    {
        return;
    }

    // Detach animation block from all nodes in this sequence.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* pAnimNode = it->get();
        static_cast<CAnimNode*>(pAnimNode)->Activate(false);
        static_cast<CAnimNode*>(pAnimNode)->OnReset();
    }

    // Remove a possibly cached game hint associated with this anim sequence.
    stack_string sTemp("anim_sequence_");
    sTemp += m_name.c_str();
    // Audio: Release precached sound

    m_bActive = false;
    m_precached = false;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::PrecacheData(float startTime)
{
    PrecacheStatic(startTime);
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::PrecacheStatic(const float startTime)
{
    // pre-cache animation keys
    for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* pAnimNode = it->get();
        static_cast<CAnimNode*>(pAnimNode)->PrecacheStatic(startTime);
    }

    m_precachedEntitiesSet.clear();

    PrecacheDynamic(startTime);

    if (m_precached)
    {
        return;
    }

    // Try to cache this sequence's game hint if one exists.
    stack_string sTemp("anim_sequence_");
    sTemp += m_name.c_str();

    //if (gEnv->pAudioSystem)
    {
        // Make sure to use the non-serializable game hint type as trackview sequences get properly reactivated after load
        // Audio: Precache sound
    }

    gEnv->pLog->Log("=== Precaching render data for cutscene: %s ===", GetName());

    m_precached = true;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::PrecacheDynamic(float time)
{
    // pre-cache animation keys
    for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* pAnimNode = it->get();
        static_cast<CAnimNode*>(pAnimNode)->PrecacheDynamic(time);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::PrecacheEntity(IEntity* pEntity)
{
    if (m_precachedEntitiesSet.find(pEntity) != m_precachedEntitiesSet.end())
    {
        if (IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>())
        {
            if (IRenderNode* pRenderNode = pRenderComponent->GetRenderNode())
            {
                gEnv->p3DEngine->PrecacheRenderNode(pRenderNode, 4.f);
            }
        }

        m_precachedEntitiesSet.insert(pEntity);
    }
}

void CAnimSequence::SetId(uint32 newId)
{
    // Notify movie system of new Id
    if (GetMovieSystem())
    {
        GetMovieSystem()->OnSetSequenceId(newId);
    }
    m_id = newId;

    if (m_sequenceType == eSequenceType_Legacy)
    {
        // For legacy sequences we store the sequence ID in m_ownerId,
        // for component entity sequences m_ownerId stores the EntityId of the component entity.
        // The rationale for storing the sequence ID for legacy sequences is that, eventually legacy
        // support will be dropped. For component entity sequences it makes sense that the component
        // entity ID is the unique way to identify sequences since that works with slices. So we want
        // to write code that gets sequences by EntityId but have that work for legacy sequences also.

        // Note that this will put the sequence ID in the lower 32 bits only. If necessary IsLegacyEntityId
        // could be used to determine if this is a valid component entity ID or a legacy sequence ID. In
        // practice though this has not so far proved necessary because the sequence type can be used to
        // determine that.
        m_ownerId = AZ::EntityId(m_id);
    }
}

//////////////////////////////////////////////////////////////////////////
/// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence Component
void CAnimSequence::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks, uint32 overrideId, bool bResetLightAnimSet)
{
    if (!GetMovieSystem())
    {
        return;   // should never happen, null pointer guard
    }

    if (bLoading)
    {
        // Load.
        RemoveAll();

        int sequenceVersion = 0;
        xmlNode->getAttr("SequenceVersion", sequenceVersion);

        int   sequenceTypeAttr = eSequenceType_Legacy;
        Range timeRange;
        SetName(xmlNode->getAttr("Name"));
        xmlNode->getAttr("Flags", m_flags);
        xmlNode->getAttr("StartTime", timeRange.start);
        xmlNode->getAttr("EndTime", timeRange.end);
        uint32 u32Id;
        if (xmlNode->getAttr("ID", u32Id))
        {
            SetId(u32Id);
        }
        if (xmlNode->getAttr("SequenceType", sequenceTypeAttr))
        {
            m_sequenceType = static_cast<ESequenceType>(sequenceTypeAttr);
        }

        if (m_sequenceType == eSequenceType_SequenceComponent)
        {
            // 64-bit unsigned int is serialized in two 32-bit ints
            AZ::EntityId sequenceComponentEntityId;
            AZ::u64 id64;
            bool foundId = false;

            if (sequenceVersion >= 4)
            {
                foundId = xmlNode->getAttr("SequenceComponentEntityId", id64);
            }
            else
            {
                // in sequenceVersion 3 we stored the 64-bit int Id separated as hi and low unsigned ints, but this caused
                // issues with iOS and OS X (likely big vs little Endian differences), so we switched to serializing the id64 directly
                // in version 4
                unsigned long idHi;
                unsigned long idLo;
                if (xmlNode->getAttr("SequenceComponentEntityIdHi", idHi) && xmlNode->getAttr("SequenceComponentEntityIdLo", idLo))
                {
                    id64 = ((AZ::u64)idHi) << 32 | idLo;
                    foundId = true;
                }
            }

            if (foundId)
            {
                sequenceComponentEntityId = AZ::EntityId(id64);
                SetOwner(sequenceComponentEntityId);
            }
        }     

        if (overrideId != 0)
        {
            SetId(overrideId);
        }

        INDENT_LOG_DURING_SCOPE(true, "Loading sequence '%s' (start time = %.2f, end time = %.2f) %s ID #%u",
            m_name.c_str(), timeRange.start, timeRange.end, overrideId ? "override" : "default", m_id);

        // Loading.
        XmlNodeRef nodes = xmlNode->findChild("Nodes");
        if (nodes)
        {
            uint32 id;
            EAnimNodeType nodeType;
            for (int i = 0; i < nodes->getChildCount(); ++i)
            {
                XmlNodeRef childNode = nodes->getChild(i);
                childNode->getAttr("Id", id);

                GetMovieSystem()->SerializeNodeType(nodeType, childNode, bLoading, sequenceVersion, m_flags);

                if (nodeType == eAnimNodeType_Invalid)
                {
                    continue;
                }

                IAnimNode* pAnimNode = CreateNodeInternal((EAnimNodeType)nodeType, id);
                if (!pAnimNode)
                {
                    continue;
                }

                pAnimNode->Serialize(childNode, bLoading, bLoadEmptyTracks);
            }

            // When all nodes loaded restore group hierarchy
            for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
            {
                CAnimNode* pAnimNode = static_cast<CAnimNode*>((*it).get());
                pAnimNode->PostLoad();

                // And properly adjust the 'm_lastGenId' to prevent the id clash.
                if (pAnimNode->GetId() >= (int)m_lastGenId)
                {
                    m_lastGenId = pAnimNode->GetId() + 1;
                }
            }
        }
        XmlNodeRef events = xmlNode->findChild("Events");
        if (events)
        {
            AZStd::string eventName;
            for (int i = 0; i < events->getChildCount(); i++)
            {
                XmlNodeRef xn = events->getChild(i);
                eventName = xn->getAttr("Name");
                m_events.push_back(eventName);

                // Added
                for (TTrackEventListeners::iterator j = m_listeners.begin(); j != m_listeners.end(); ++j)
                {
                    (*j)->OnTrackEvent(this, ITrackEventListener::eTrackEventReason_Added, eventName.c_str(), NULL);
                }
            }
        }
        // Setting the time range must be done after the loading of all nodes
        // since it sets the time range of tracks, also.
        SetTimeRange(timeRange);
        Deactivate();
        //ComputeTimeRange();

        if (GetOwner())
        {
            GetOwner()->OnNameChanged();
        }
    }
    else
    {
        // Save.
        xmlNode->setAttr("SequenceVersion", IAnimSequence::kSequenceVersion);

        const char* fullname = GetName();
        xmlNode->setAttr("Name", fullname);     // Save the full path as a name.
        xmlNode->setAttr("Flags", m_flags);
        xmlNode->setAttr("StartTime", m_timeRange.start);
        xmlNode->setAttr("EndTime", m_timeRange.end);
        xmlNode->setAttr("ID", m_id);
        xmlNode->setAttr("SequenceType", m_sequenceType);

        if (m_sequenceType == eSequenceType_SequenceComponent && m_ownerId.IsValid())
        {
            AZ::u64 id64 = static_cast<AZ::u64>(m_ownerId);
            xmlNode->setAttr("SequenceComponentEntityId", id64);
        }

        XmlNodeRef nodes = xmlNode->newChild("Nodes");
        int num = GetNodeCount();
        for (int i = 0; i < num; i++)
        {
            IAnimNode* pAnimNode = GetNode(i);
            if (pAnimNode)
            {
                XmlNodeRef xn = nodes->newChild("Node");
                pAnimNode->Serialize(xn, bLoading, true);
            }
        }

        XmlNodeRef events = xmlNode->newChild("Events");
        for (TrackEvents::iterator iter = m_events.begin(); iter != m_events.end(); ++iter)
        {
            XmlNodeRef xn = events->newChild("Event");
            xn->setAttr("Name", iter->c_str());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<CAnimSequence>()
        ->Version(1)
        ->Field("Name", &CAnimSequence::m_name)
        ->Field("SequenceEntityId", &CAnimSequence::m_ownerId)
        ->Field("Flags", &CAnimSequence::m_flags)
        ->Field("TimeRange", &CAnimSequence::m_timeRange)
        ->Field("ID", &CAnimSequence::m_id)
        ->Field("Nodes", &CAnimSequence::m_nodes)
        ->Field("SequenceType", &CAnimSequence::m_sequenceType)
        ->Field("Events", &CAnimSequence::m_events);
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::InitPostLoad()
{
    int num = GetNodeCount();
    for (int i = 0; i < num; i++)
    {
        IAnimNode* animNode = GetNode(i);
        if (animNode)
        {
            AddNode(animNode);
            animNode->InitPostLoad(this);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::SetTimeRange(Range timeRange)
{
    m_timeRange = timeRange;
    // Set this time range for every track in animation.
    // Set time range to be in range of largest animation track.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* anode = it->get();
        anode->SetTimeRange(timeRange);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::AdjustKeysToTimeRange(const Range& timeRange)
{
    float offset = timeRange.start - m_timeRange.start;
    // Calculate scale ratio.
    float scale = timeRange.Length() / m_timeRange.Length();
    m_timeRange = timeRange;

    // Set time range to be in range of largest animation track.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* pAnimNode = it->get();

        int trackCount = pAnimNode->GetTrackCount();
        for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
        {
            IAnimTrack* pTrack = pAnimNode->GetTrackByIndex(paramIndex);
            int nkey = pTrack->GetNumKeys();
            for (int k = 0; k < nkey; k++)
            {
                float keytime = pTrack->GetKeyTime(k);
                keytime = offset + keytime * scale;
                pTrack->SetKeyTime(k, keytime);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::ComputeTimeRange()
{
    Range timeRange = m_timeRange;

    // Set time range to be in range of largest animation track.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* pAnimNode = it->get();

        int trackCount = pAnimNode->GetTrackCount();
        for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
        {
            IAnimTrack* pTrack = pAnimNode->GetTrackByIndex(paramIndex);
            int nkey = pTrack->GetNumKeys();
            if (nkey > 0)
            {
                timeRange.start = std::min(timeRange.start, pTrack->GetKeyTime(0));
                timeRange.end = std::max(timeRange.end, pTrack->GetKeyTime(nkey - 1));
            }
        }
    }

    if (timeRange.start > 0)
    {
        timeRange.start = 0;
    }

    m_timeRange = timeRange;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSequence::AddTrackEvent(const char* szEvent)
{
    CRY_ASSERT(szEvent && szEvent[0]);
    if (stl::push_back_unique(m_events, szEvent))
    {
        NotifyTrackEvent(ITrackEventListener::eTrackEventReason_Added, szEvent);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSequence::RemoveTrackEvent(const char* szEvent)
{
    CRY_ASSERT(szEvent && szEvent[0]);
    if (stl::find_and_erase(m_events, szEvent))
    {
        NotifyTrackEvent(ITrackEventListener::eTrackEventReason_Removed, szEvent);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSequence::RenameTrackEvent(const char* szEvent, const char* szNewEvent)
{
    CRY_ASSERT(szEvent && szEvent[0]);
    CRY_ASSERT(szNewEvent && szNewEvent[0]);

    for (size_t i = 0; i < m_events.size(); ++i)
    {
        if (m_events[i] == szEvent)
        {
            m_events[i] = szNewEvent;
            NotifyTrackEvent(ITrackEventListener::eTrackEventReason_Renamed, szEvent, szNewEvent);
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSequence::MoveUpTrackEvent(const char* szEvent)
{
    CRY_ASSERT(szEvent && szEvent[0]);

    for (size_t i = 0; i < m_events.size(); ++i)
    {
        if (m_events[i] == szEvent)
        {
            assert(i > 0);
            if (i > 0)
            {
                std::swap(m_events[i - 1], m_events[i]);
                NotifyTrackEvent(ITrackEventListener::eTrackEventReason_MovedUp, szEvent);
            }
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSequence::MoveDownTrackEvent(const char* szEvent)
{
    CRY_ASSERT(szEvent && szEvent[0]);

    for (size_t i = 0; i < m_events.size(); ++i)
    {
        if (m_events[i] == szEvent)
        {
            assert(i < m_events.size() - 1);
            if (i < m_events.size() - 1)
            {
                std::swap(m_events[i], m_events[i + 1]);
                NotifyTrackEvent(ITrackEventListener::eTrackEventReason_MovedDown, szEvent);
            }
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::ClearTrackEvents()
{
    m_events.clear();
}

//////////////////////////////////////////////////////////////////////////
int CAnimSequence::GetTrackEventsCount() const
{
    return (int)m_events.size();
}

//////////////////////////////////////////////////////////////////////////
char const* CAnimSequence::GetTrackEvent(int iIndex) const
{
    char const* szResult = NULL;
    const bool bValid = (iIndex >= 0 && iIndex < GetTrackEventsCount());
    CRY_ASSERT(bValid);

    if (bValid)
    {
        szResult = m_events[iIndex].c_str();
    }

    return szResult;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::NotifyTrackEvent(ITrackEventListener::ETrackEventReason reason,
    const char* event, const char* param)
{
    // Notify listeners
    for (TTrackEventListeners::iterator j = m_listeners.begin(); j != m_listeners.end(); ++j)
    {
        (*j)->OnTrackEvent(this, reason, event, (void*)param);
    }

    // Notification via Event Bus
    if (GetSequenceType() == eSequenceType_SequenceComponent)
    {
        Maestro::SequenceComponentNotificationBus::Event(GetOwnerId(), &Maestro::SequenceComponentNotificationBus::Events::OnTrackEventTriggered, event, param);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::TriggerTrackEvent(const char* event, const char* param)
{
    NotifyTrackEvent(ITrackEventListener::eTrackEventReason_Triggered, event, param);
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::AddTrackEventListener(ITrackEventListener* pListener)
{
    if (std::find(m_listeners.begin(), m_listeners.end(), pListener) == m_listeners.end())
    {
        m_listeners.push_back(pListener);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::RemoveTrackEventListener(ITrackEventListener* pListener)
{
    TTrackEventListeners::iterator it = std::find(m_listeners.begin(), m_listeners.end(), pListener);
    if (it != m_listeners.end())
    {
        m_listeners.erase(it);
    }
}

//////////////////////////////////////////////////////////////////////////
IAnimNode* CAnimSequence::FindNodeById(int nNodeId)
{
    for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* pAnimNode = it->get();
        if (static_cast<CAnimNode*>(pAnimNode)->GetId() == nNodeId)
        {
            return pAnimNode;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
IAnimNode* CAnimSequence::FindNodeByName(const char* sNodeName, const IAnimNode* pParentDirector)
{
    for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* pAnimNode = it->get();
        // Case insensitive name comparison.
        if (_stricmp(((CAnimNode*)pAnimNode)->GetNameFast(), sNodeName) == 0)
        {
            bool bParentDirectorCheck = pAnimNode->HasDirectorAsParent() == pParentDirector;
            if (bParentDirectorCheck)
            {
                return pAnimNode;
            }
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::ReorderNode(IAnimNode* pNode, IAnimNode* pPivotNode, bool bNext)
{
    if (pNode == pPivotNode || !pNode)
    {
        return;
    }

    AZStd::intrusive_ptr<IAnimNode> pTempHolder(pNode); // Keep reference to node so it is not deleted by erasing from list.
    stl::find_and_erase_if(m_nodes, [pNode](const AZStd::intrusive_ptr<IAnimNode>& sp) { return sp.get() == pNode; });

    AnimNodes::iterator it;
    for (it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* pAnimNode = it->get();
        if (pAnimNode == pPivotNode)
        {
            if (bNext)
            {
                m_nodes.insert(it + 1, AZStd::intrusive_ptr<IAnimNode>(pNode));
            }
            else
            {
                m_nodes.insert(it, AZStd::intrusive_ptr<IAnimNode>(pNode));
            }
            break;
        }
    }

    if (it == m_nodes.end())
    {
        m_nodes.insert(m_nodes.begin(), AZStd::intrusive_ptr<IAnimNode>(pNode));
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::CopyNodeChildren(XmlNodeRef& xmlNode, IAnimNode* pAnimNode)
{
    for (int k = 0; k < GetNodeCount(); ++k)
    {
        if (GetNode(k)->GetParent() == pAnimNode)
        {
            XmlNodeRef childNode = xmlNode->newChild("Node");
            GetNode(k)->Serialize(childNode, false, true);
            if (GetNode(k)->GetType() == eAnimNodeType_Group
                || pAnimNode->GetType() == eAnimNodeType_Director)
            {
                CopyNodeChildren(xmlNode, GetNode(k));
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::CopyNodes(XmlNodeRef& xmlNode, IAnimNode** pSelectedNodes, uint32 count)
{
    for (uint32 i = 0; i < count; ++i)
    {
        IAnimNode* pAnimNode = pSelectedNodes[i];
        if (pAnimNode)
        {
            XmlNodeRef xn = xmlNode->newChild("Node");
            pAnimNode->Serialize(xn, false, true);
            // If it is a group node, copy its children also.
            if (pAnimNode->GetType() == eAnimNodeType_Group || pAnimNode->GetType() == eAnimNodeType_Director)
            {
                CopyNodeChildren(xmlNode, pAnimNode);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::PasteNodes(const XmlNodeRef& xmlNode, IAnimNode* pParent)
{
    int type, id;
    std::map<int, IAnimNode*> idToNode;
    for (int i = 0; i < xmlNode->getChildCount(); i++)
    {
        XmlNodeRef xn = xmlNode->getChild(i);

        if (!xn->getAttr("Type", type))
        {
            continue;
        }

        xn->getAttr("Id", id);

        IAnimNode* node = CreateNode((EAnimNodeType)type);
        if (!node)
        {
            continue;
        }

        idToNode[id] = node;

        xn->setAttr("Id", static_cast<CAnimNode*>(node)->GetId());
        node->Serialize(xn, true, true);

        int parentId = 0;
        if (xn->getAttr("ParentNode", parentId))
        {
            node->SetParent(idToNode[parentId]);
        }
        else
        // This means a top-level node.
        {
            if (pParent)
            {
                node->SetParent(pParent);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSequence::AddNodeNeedToRender(IAnimNode* pNode)
{
    assert(pNode != 0);
    return stl::push_back_unique(m_nodesNeedToRender, AZStd::intrusive_ptr<IAnimNode>(pNode));
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::RemoveNodeNeedToRender(IAnimNode* pNode)
{
    assert(pNode != 0);
    stl::find_and_erase_if(m_nodesNeedToRender, [pNode](const AZStd::intrusive_ptr<IAnimNode>& sp) { return sp.get() == pNode; });
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::SetOwner(const AZ::EntityId& entityOwnerId)
{ 
    m_ownerId = entityOwnerId; 
}
//////////////////////////////////////////////////////////////////////////
void CAnimSequence::SetActiveDirector(IAnimNode* pDirectorNode)
{
    if (!pDirectorNode)
    {
        return;
    }

    assert(pDirectorNode->GetType() == eAnimNodeType_Director);
    if (pDirectorNode->GetType() != eAnimNodeType_Director)
    {
        return;     // It's not a director node.
    }

    if (pDirectorNode->GetSequence() != this)
    {
        return;     // It's not a node belong to this sequence.
    }

    m_pActiveDirector = pDirectorNode;
}

//////////////////////////////////////////////////////////////////////////
IAnimNode* CAnimSequence::GetActiveDirector() const
{
    return m_pActiveDirector;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSequence::IsAncestorOf(const IAnimSequence* pSequence) const
{
    assert(this != pSequence);
    if (this == pSequence)
    {
        return true;
    }

    if (!GetMovieSystem())
    {
        return false;   // should never happen, null pointer guard
    }

    for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IAnimNode* pNode = it->get();
        if (pNode->GetType() == eAnimNodeType_Director)
        {
            IAnimTrack* pSequenceTrack = pNode->GetTrackForParameter(eAnimParamType_Sequence);
            if (pSequenceTrack)
            {
                PREFAST_ASSUME(pSequence);
                for (int i = 0; i < pSequenceTrack->GetNumKeys(); ++i)
                {
                    ISequenceKey key;
                    pSequenceTrack->GetKey(i, &key);
                    if (_stricmp(key.szSelection.c_str(), pSequence->GetName()) == 0)
                    {
                        return true;
                    }

                    IAnimSequence* pChild = GetMovieSystem()->FindSequence(key.szSelection.c_str());
                    if (pChild && pChild->IsAncestorOf(pSequence))
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}



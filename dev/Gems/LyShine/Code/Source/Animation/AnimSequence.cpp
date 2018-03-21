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

#include "LyShine_precompiled.h"
#include "AnimSequence.h"

#include "StlUtils.h"
#include "I3DEngine.h"
#include "AzEntityNode.h"
#include "UiAnimSerialize.h"

#include "IScriptSystem.h"
#include "Components/IComponentRender.h"

//////////////////////////////////////////////////////////////////////////
CUiAnimSequence::CUiAnimSequence()
    : CUiAnimSequence(nullptr, 0)
{
}

//////////////////////////////////////////////////////////////////////////
CUiAnimSequence::CUiAnimSequence(IUiAnimationSystem* pUiAnimationSystem, uint32 id)
    : m_refCount(0)
{
    m_lastGenId = 1;
    m_pUiAnimationSystem = pUiAnimationSystem;
    m_flags = 0;
    m_pParentSequence = NULL;
    m_timeRange.Set(0, 10);
    m_bPaused = false;
    m_bActive = false;
    m_pOwner = NULL;
    m_pActiveDirector = NULL;
    m_precached = false;
    m_bResetting = false;
    m_id = id;
    m_time = -FLT_MAX;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimSequence::~CUiAnimSequence()
{
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::add_ref()
{
    ++m_refCount;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::release()
{
    if (--m_refCount <= 0)
    {
        delete this;
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::SetName(const char* name)
{
    string originalName = GetName();

    m_name = name;
    m_pUiAnimationSystem->OnSequenceRenamed(originalName, m_name.c_str());

    if (GetOwner())
    {
        GetOwner()->OnModified();
    }
}

//////////////////////////////////////////////////////////////////////////
const char* CUiAnimSequence::GetName() const
{
    return m_name.c_str();
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::SetFlags(int flags)
{
    m_flags = flags;
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimSequence::GetFlags() const
{
    return m_flags;
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimSequence::GetCutSceneFlags(const bool localFlags) const
{
    int currentFlags = m_flags & (eSeqFlags_NoHUD | eSeqFlags_NoPlayer | eSeqFlags_16To9 | eSeqFlags_NoGameSounds | eSeqFlags_NoAbort);

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
void CUiAnimSequence::SetParentSequence(IUiAnimSequence* pParentSequence)
{
    m_pParentSequence = pParentSequence;
}

//////////////////////////////////////////////////////////////////////////
const IUiAnimSequence* CUiAnimSequence::GetParentSequence() const
{
    return m_pParentSequence;
}

//////////////////////////////////////////////////////////////////////////
int CUiAnimSequence::GetNodeCount() const
{
    return m_nodes.size();
}

//////////////////////////////////////////////////////////////////////////
IUiAnimNode* CUiAnimSequence::GetNode(int index) const
{
    assert(index >= 0 && index < (int)m_nodes.size());
    return m_nodes[index].get();
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimSequence::AddNode(IUiAnimNode* pAnimNode)
{
    assert(pAnimNode != 0);

    // Check if this node already in sequence.
    for (int i = 0; i < (int)m_nodes.size(); i++)
    {
        if (pAnimNode == m_nodes[i].get())
        {
            // Fail to add node second time.
            return false;
        }
    }

    static_cast<CUiAnimNode*>(pAnimNode)->SetSequence(this);
    pAnimNode->SetTimeRange(m_timeRange);
    m_nodes.push_back(AZStd::intrusive_ptr<IUiAnimNode>(pAnimNode));

    const int nodeId = static_cast<CUiAnimNode*>(pAnimNode)->GetId();
    if (nodeId >= (int)m_lastGenId)
    {
        m_lastGenId = nodeId + 1;
    }

    if (pAnimNode->NeedToRender())
    {
        AddNodeNeedToRender(pAnimNode);
    }

    bool bNewDirectorNode = m_pActiveDirector == NULL && pAnimNode->GetType() == eUiAnimNodeType_Director;
    if (bNewDirectorNode)
    {
        m_pActiveDirector = pAnimNode;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
IUiAnimNode* CUiAnimSequence::CreateNodeInternal(EUiAnimNodeType nodeType, uint32 nNodeId)
{
    CUiAnimNode* pAnimNode = NULL;

    if (nNodeId == -1)
    {
        nNodeId = m_lastGenId;
    }

    switch (nodeType)
    {
    case eUiAnimNodeType_AzEntity:
        pAnimNode = aznew CUiAnimAzEntityNode(nNodeId);
        break;
    }

    if (pAnimNode)
    {
        AddNode(pAnimNode);
    }

    return pAnimNode;
}

//////////////////////////////////////////////////////////////////////////
IUiAnimNode* CUiAnimSequence::CreateNode(EUiAnimNodeType nodeType)
{
    return CreateNodeInternal(nodeType);
}

//////////////////////////////////////////////////////////////////////////
IUiAnimNode* CUiAnimSequence::CreateNode(XmlNodeRef node)
{
    EUiAnimNodeType type;
    static_cast<UiAnimationSystem*>(GetUiAnimationSystem())->SerializeNodeType(type, node, true, IUiAnimSequence::kSequenceVersion, 0);

    XmlString name;
    if (!node->getAttr("Name", name))
    {
        return 0;
    }

    IUiAnimNode* pNewNode = CreateNode(type);
    if (!pNewNode)
    {
        return 0;
    }

    pNewNode->SetName(name);
    pNewNode->Serialize(node, true, true);

    return pNewNode;
}

//////////////////////////////////////////////////////////////////////////
// Only called from undo/redo
void CUiAnimSequence::RemoveNode(IUiAnimNode* node)
{
    assert(node != 0);

    static_cast<CUiAnimNode*>(node)->Activate(false);
    static_cast<CUiAnimNode*>(node)->OnReset();

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
        if (m_nodes[i]->GetParent() == node)
        {
            m_nodes[i]->SetParent(0);
        }

        i++;
    }

    // The removed one was the active director node.
    if (m_pActiveDirector == node)
    {
        // Clear the active one.
        m_pActiveDirector = NULL;
        // If there is another director node, set it as active.
        for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
        {
            IUiAnimNode* pNode = it->get();
            if (pNode->GetType() == eUiAnimNodeType_Director)
            {
                SetActiveDirector(pNode);
                break;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::RemoveAll()
{
    stl::free_container(m_nodes);
    stl::free_container(m_nodesNeedToRender);
    m_pActiveDirector = NULL;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::Reset(bool bSeekToStart)
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
            IUiAnimNode* pAnimNode = it->get();
            static_cast<CUiAnimNode*>(pAnimNode)->OnReset();
        }
        m_bResetting = false;
        return;
    }

    bool bWasActive = m_bActive;

    if (!bWasActive)
    {
        Activate();
    }

    SUiAnimContext ec;
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
            IUiAnimNode* pAnimNode = it->get();
            static_cast<CUiAnimNode*>(pAnimNode)->OnReset();
        }
    }

    m_bResetting = false;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::ResetHard()
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

    SUiAnimContext ec;
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
            IUiAnimNode* pAnimNode = it->get();
            static_cast<CUiAnimNode*>(pAnimNode)->OnResetHard();
        }
    }

    m_bResetting = false;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::Pause()
{
    if (GetFlags() & eSeqFlags_LightAnimationSet || m_bPaused)
    {
        return;
    }

    m_bPaused = true;

    // Detach animation block from all nodes in this sequence.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        static_cast<CUiAnimNode*>(pAnimNode)->OnPause();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::Resume()
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
            IUiAnimNode* pAnimNode = it->get();
            static_cast<CUiAnimNode*>(pAnimNode)->OnResume();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::OnLoop()
{
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        static_cast<CUiAnimNode*>(pAnimNode)->OnLoop();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimSequence::IsPaused() const
{
    return m_bPaused;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::OnStart()
{
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        static_cast<CUiAnimNode*>(pAnimNode)->OnStart();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::OnStop()
{
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        static_cast<CUiAnimNode*>(pAnimNode)->OnStop();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::StillUpdate()
{
    if (GetFlags() & eSeqFlags_LightAnimationSet)
    {
        return;
    }

    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        pAnimNode->StillUpdate();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::Animate(const SUiAnimContext& ec)
{
    assert(m_bActive);

    if (GetFlags() & eSeqFlags_LightAnimationSet)
    {
        return;
    }

    SUiAnimContext animContext = ec;
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
        IUiAnimNode* pAnimNode = it->get();

        // All other (inactive) director nodes are skipped.
        if (pAnimNode->GetType() == eUiAnimNodeType_Director)
        {
            continue;
        }

        // If this is a descendant of a director node and that director is currently not active, skip this one.
        IUiAnimNode* pParentDirector = pAnimNode->HasDirectorAsParent();
        if (pParentDirector && pParentDirector != m_pActiveDirector)
        {
            continue;
        }

        if (pAnimNode->GetFlags() & eUiAnimNodeFlags_Disabled)
        {
            continue;
        }

        // Animate node.
        pAnimNode->Animate(animContext);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::Render()
{
    for (AnimNodes::iterator it = m_nodesNeedToRender.begin(); it != m_nodesNeedToRender.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        pAnimNode->Render();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::Activate()
{
    if (m_bActive)
    {
        return;
    }

    m_bActive = true;
    // Assign animation block to all nodes in this sequence.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        static_cast<CUiAnimNode*>(pAnimNode)->Activate(true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::Deactivate()
{
    if (!m_bActive)
    {
        return;
    }

    // Detach animation block from all nodes in this sequence.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        static_cast<CUiAnimNode*>(pAnimNode)->Activate(false);
        static_cast<CUiAnimNode*>(pAnimNode)->OnReset();
    }

    // Remove a possibly cached game hint associated with this anim sequence.
    stack_string sTemp("anim_sequence_");
    sTemp += m_name.c_str();
    // Audio: Release precached sound

    m_bActive = false;
    m_precached = false;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::PrecacheData(float startTime)
{
    PrecacheStatic(startTime);
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::PrecacheStatic(const float startTime)
{
    // pre-cache animation keys
    for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        static_cast<CUiAnimNode*>(pAnimNode)->PrecacheStatic(startTime);
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

    // Make sure to use the non-serializable game hint type as UI AnimView sequences get properly reactivated after load
    // Audio: Precache sound

    gEnv->pLog->Log("=== Precaching render data for cutscene: %s ===", GetName());

    m_precached = true;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::PrecacheDynamic(float time)
{
    // pre-cache animation keys
    for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        static_cast<CUiAnimNode*>(pAnimNode)->PrecacheDynamic(time);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::PrecacheEntity(IEntity* pEntity)
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

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks, uint32 overrideId, bool bResetLightAnimSet)
{
    if (bLoading)
    {
        // Load.
        RemoveAll();

        int sequenceVersion = 0;
        xmlNode->getAttr("SequenceVersion", sequenceVersion);

        Range timeRange;
        m_name = xmlNode->getAttr("Name");
        xmlNode->getAttr("Flags", m_flags);
        xmlNode->getAttr("StartTime", timeRange.start);
        xmlNode->getAttr("EndTime", timeRange.end);
        xmlNode->getAttr("ID", m_id);

        if (overrideId != 0)
        {
            m_id = overrideId;
        }

        INDENT_LOG_DURING_SCOPE(true, "Loading sequence '%s' (start time = %.2f, end time = %.2f) %s ID #%u",
            m_name.c_str(), timeRange.start, timeRange.end, overrideId ? "override" : "default", m_id);

        // Loading.
        XmlNodeRef nodes = xmlNode->findChild("Nodes");
        if (nodes)
        {
            uint32 id;
            EUiAnimNodeType nodeType;
            for (int i = 0; i < nodes->getChildCount(); ++i)
            {
                XmlNodeRef childNode = nodes->getChild(i);
                childNode->getAttr("Id", id);

                static_cast<UiAnimationSystem*>(GetUiAnimationSystem())->SerializeNodeType(nodeType, childNode, bLoading, sequenceVersion, m_flags);

                if (nodeType == eUiAnimNodeType_Invalid)
                {
                    continue;
                }

                IUiAnimNode* pAnimNode = CreateNodeInternal((EUiAnimNodeType)nodeType, id);
                if (!pAnimNode)
                {
                    continue;
                }

                pAnimNode->Serialize(childNode, bLoading, bLoadEmptyTracks);
            }

            // When all nodes loaded restore group hierarchy
            for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
            {
                CUiAnimNode* pAnimNode = static_cast<CUiAnimNode*>((*it).get());
                pAnimNode->PostLoad();

                // And properly adjust the 'm_lastGenId' to prevent the id clash.
                if (pAnimNode->GetId() >= (int)m_lastGenId)
                {
                    m_lastGenId = pAnimNode->GetId() + 1;
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
            GetOwner()->OnModified();
        }
    }
    else
    {
        xmlNode->setAttr("SequenceVersion", IUiAnimSequence::kSequenceVersion);

        const char* fullname = GetName();
        xmlNode->setAttr("Name", fullname);     // Save the full path as a name.
        xmlNode->setAttr("Flags", m_flags);
        xmlNode->setAttr("StartTime", m_timeRange.start);
        xmlNode->setAttr("EndTime", m_timeRange.end);
        xmlNode->setAttr("ID", m_id);

        // Save.
        XmlNodeRef nodes = xmlNode->newChild("Nodes");
        int num = GetNodeCount();
        for (int i = 0; i < num; i++)
        {
            IUiAnimNode* pAnimNode = GetNode(i);
            if (pAnimNode)
            {
                XmlNodeRef xn = nodes->newChild("Node");
                pAnimNode->Serialize(xn, bLoading, true);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::InitPostLoad(IUiAnimationSystem* pUiAnimationSystem, bool remapIds, LyShine::EntityIdMap* entityIdMap)
{
    m_pUiAnimationSystem = pUiAnimationSystem;
    int num = GetNodeCount();
    for (int i = 0; i < num; i++)
    {
        IUiAnimNode* pAnimNode = GetNode(i);
        if (pAnimNode)
        {
            pAnimNode->InitPostLoad(this, remapIds, entityIdMap);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::SetTimeRange(Range timeRange)
{
    m_timeRange = timeRange;
    // Set this time range for every track in animation.
    // Set time range to be in range of largest animation track.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* anode = it->get();
        anode->SetTimeRange(timeRange);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::AdjustKeysToTimeRange(const Range& timeRange)
{
    float offset = timeRange.start - m_timeRange.start;
    // Calculate scale ratio.
    float scale = timeRange.Length() / m_timeRange.Length();
    m_timeRange = timeRange;

    // Set time range to be in range of largest animation track.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();

        int trackCount = pAnimNode->GetTrackCount();
        for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
        {
            IUiAnimTrack* pTrack = pAnimNode->GetTrackByIndex(paramIndex);
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
void CUiAnimSequence::ComputeTimeRange()
{
    Range timeRange = m_timeRange;

    // Set time range to be in range of largest animation track.
    for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();

        int trackCount = pAnimNode->GetTrackCount();
        for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
        {
            IUiAnimTrack* pTrack = pAnimNode->GetTrackByIndex(paramIndex);
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
IUiAnimNode* CUiAnimSequence::FindNodeById(int nNodeId)
{
    for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        if (static_cast<CUiAnimNode*>(pAnimNode)->GetId() == nNodeId)
        {
            return pAnimNode;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
IUiAnimNode* CUiAnimSequence::FindNodeByName(const char* sNodeName, const IUiAnimNode* pParentDirector)
{
    for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        // Case insensitive name comparison.
        if (_stricmp(((CUiAnimNode*)pAnimNode)->GetNameFast(), sNodeName) == 0)
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
void CUiAnimSequence::ReorderNode(IUiAnimNode* pNode, IUiAnimNode* pPivotNode, bool bNext)
{
    if (pNode == pPivotNode || !pNode)
    {
        return;
    }

    AZStd::intrusive_ptr<IUiAnimNode> pTempHolder(pNode); // Keep reference to node so it is not deleted by erasing from list.
    stl::find_and_erase_if(m_nodes, [pNode](const AZStd::intrusive_ptr<IUiAnimNode>& sp) { return sp.get() == pNode; });


    AnimNodes::iterator it;
    for (it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        IUiAnimNode* pAnimNode = it->get();
        if (pAnimNode == pPivotNode)
        {
            if (bNext)
            {
                m_nodes.insert(it + 1, AZStd::intrusive_ptr<IUiAnimNode>(pNode));
            }
            else
            {
                m_nodes.insert(it, AZStd::intrusive_ptr<IUiAnimNode>(pNode));
            }
            break;
        }
    }

    if (it == m_nodes.end())
    {
        m_nodes.insert(m_nodes.begin(), AZStd::intrusive_ptr<IUiAnimNode>(pNode));
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::CopyNodeChildren(XmlNodeRef& xmlNode, IUiAnimNode* pAnimNode)
{
    for (int k = 0; k < GetNodeCount(); ++k)
    {
        if (GetNode(k)->GetParent() == pAnimNode)
        {
            XmlNodeRef childNode = xmlNode->newChild("Node");
            GetNode(k)->Serialize(childNode, false, true);
            if (GetNode(k)->GetType() == eUiAnimNodeType_Group
                || pAnimNode->GetType() == eUiAnimNodeType_Director)
            {
                CopyNodeChildren(xmlNode, GetNode(k));
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::CopyNodes(XmlNodeRef& xmlNode, IUiAnimNode** pSelectedNodes, uint32 count)
{
    for (uint32 i = 0; i < count; ++i)
    {
        IUiAnimNode* pAnimNode = pSelectedNodes[i];
        if (pAnimNode)
        {
            XmlNodeRef xn = xmlNode->newChild("Node");
            pAnimNode->Serialize(xn, false, true);
            // If it is a group node, copy its children also.
            if (pAnimNode->GetType() == eUiAnimNodeType_Group   || pAnimNode->GetType() == eUiAnimNodeType_Director)
            {
                CopyNodeChildren(xmlNode, pAnimNode);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::PasteNodes(const XmlNodeRef& xmlNode, IUiAnimNode* pParent)
{
    int type, id;
    std::map<int, IUiAnimNode*> idToNode;
    for (int i = 0; i < xmlNode->getChildCount(); i++)
    {
        XmlNodeRef xn = xmlNode->getChild(i);

        if (!xn->getAttr("Type", type))
        {
            continue;
        }

        xn->getAttr("Id", id);

        IUiAnimNode* node = CreateNode((EUiAnimNodeType)type);
        if (!node)
        {
            continue;
        }

        idToNode[id] = node;

        xn->setAttr("Id", static_cast<CUiAnimNode*>(node)->GetId());
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
void CUiAnimSequence::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<CUiAnimSequence>()
        ->Version(1)
        ->Field("Name", &CUiAnimSequence::m_name)
        ->Field("Flags", &CUiAnimSequence::m_flags)
        ->Field("TimeRange", &CUiAnimSequence::m_timeRange)
        ->Field("ID", &CUiAnimSequence::m_id)
        ->Field("Nodes", &CUiAnimSequence::m_nodes);
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimSequence::AddNodeNeedToRender(IUiAnimNode* pNode)
{
    assert(pNode != 0);
    return stl::push_back_unique(m_nodesNeedToRender, AZStd::intrusive_ptr<IUiAnimNode>(pNode));
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::RemoveNodeNeedToRender(IUiAnimNode* pNode)
{
    assert(pNode != 0);
    stl::find_and_erase_if(m_nodesNeedToRender, [pNode](const AZStd::intrusive_ptr<IUiAnimNode>& sp) { return sp.get() == pNode; });
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimSequence::SetActiveDirector(IUiAnimNode* pDirectorNode)
{
    assert(pDirectorNode->GetType() == eUiAnimNodeType_Director);
    if (pDirectorNode->GetType() != eUiAnimNodeType_Director)
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
IUiAnimNode* CUiAnimSequence::GetActiveDirector() const
{
    return m_pActiveDirector;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimSequence::IsAncestorOf(const IUiAnimSequence* pSequence) const
{
    assert(this != pSequence);
    if (this == pSequence)
    {
        return true;
    }

    // UI_ANIMATION_REVISIT: was only doing anything for sequence tracks

    return false;
}



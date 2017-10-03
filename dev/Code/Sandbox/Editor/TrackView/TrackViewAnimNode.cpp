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

#include "StdAfx.h"
#include <MathConversion.h>

#include <Maestro/Bus/SequenceComponentBus.h>
#include <Maestro/Bus/EditorSequenceComponentBus.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>

#include "TrackViewDialog.h"
#include "TrackViewAnimNode.h"
#include "TrackViewTrack.h"
#include "TrackViewSequence.h"
#include "TrackViewUndo.h"
#include "TrackViewNodeFactories.h"
#include "AnimationContext.h"
#include "CommentNodeAnimator.h"
#include "LayerNodeAnimator.h"
#include "DirectorNodeAnimator.h"
#include <Plugins/ComponentEntityEditorPlugin/Objects/ComponentEntityObject.h>
#include "Objects/EntityObject.h"
#include "Objects/CameraObject.h"
#include "Objects/SequenceObject.h"
#include "Objects/ObjectLayerManager.h"
#include "Objects/MiscEntities.h"
#include "Objects/GizmoManager.h"
#include "Objects/ObjectManager.h"
#include "ViewManager.h"
#include "RenderViewport.h"
#include "Clipboard.h"

// static class data
const AZ::Uuid CTrackViewAnimNode::s_nullUuid = AZ::Uuid::CreateNull();

//////////////////////////////////////////////////////////////////////////
static void CreateDefaultTracksForEntityNode(CTrackViewAnimNode* pNode, const DynArray<unsigned int>& trackCount)
{
    if (pNode->GetType() == eAnimNodeType_Entity)
    {
        for (size_t i = 0; i < trackCount.size(); ++i)
        {
            unsigned int count = trackCount[i];
            CAnimParamType paramType = GetIEditor()->GetMovieSystem()->GetEntityNodeParamType(i);

            for (unsigned int j = 0; j < count; ++j)
            {
                pNode->CreateTrack(paramType);
            }
        }
    }
    else if (pNode->GetType() == eAnimNodeType_AzEntity)
    {
        bool haveTrackToAdd = false;

        // check that the trackCount array passed in has at least one non-zero element in it for Pos/Rot/Scale, meaning we have at least one track to add
        IMovieSystem* pMovieSystem = GetIEditor()->GetMovieSystem();
        assert(trackCount.size() == pMovieSystem->GetEntityNodeParamCount());
        for (int i = 0; i < pMovieSystem->GetEntityNodeParamCount(); ++i)
        {
            CAnimParamType paramType = pMovieSystem->GetEntityNodeParamType(i);
            if ((paramType == eAnimParamType_Position || paramType == eAnimParamType_Rotation || paramType == eAnimParamType_Scale) && trackCount[i])
            {
                haveTrackToAdd = true;
                break;
            }
        }

        if (haveTrackToAdd)
        {
            // add a Transform Component anim node if needed, then go through and look for Position,
            // Rotation and Scale default tracks and adds them by hard-coded Virtual Property name. This is not a scalable way to do this,
            // but fits into the legacy Track View entity property system. This will all be re-written in a new TrackView for components in the future.
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, pNode->GetAzEntityId());
            if (entity)
            {
                AZ::Component* transformComponent = entity->FindComponent(AzToolsFramework::Components::TransformComponent::TYPEINFO_Uuid());
                if (transformComponent)
                {
                    // find a transform Component Node if it exists, otherwise create one.
                    CTrackViewAnimNode* transformComponentNode = nullptr;

                    for (int i = pNode->GetChildCount(); --i >= 0;)
                    {
                        if (pNode->GetChild(i)->GetNodeType() == eTVNT_AnimNode)
                        {
                            CTrackViewAnimNode* childAnimNode = static_cast<CTrackViewAnimNode*>(pNode->GetChild(i));
                            AZ::ComponentId componentId = childAnimNode->GetComponentId();
                            AZ::Uuid componentTypeId;
                            AzFramework::ApplicationRequests::Bus::BroadcastResult(componentTypeId, &AzFramework::ApplicationRequests::Bus::Events::GetComponentTypeId, entity->GetId(), componentId);
                            if (componentTypeId == AzToolsFramework::Components::TransformComponent::TYPEINFO_Uuid())
                            {
                                transformComponentNode = childAnimNode;
                                break;
                            }
                        }
                    }

                    if (!transformComponentNode)
                    {
                        // no existing Transform Component node found - create one.
                        transformComponentNode = pNode->AddComponent(transformComponent);
                    }

                    if (transformComponentNode)
                    {
                        for (size_t i = 0; i < trackCount.size(); ++i)
                        {
                            if (trackCount[i])
                            {
                                // This is not ideal - we hard-code the VirtualProperty names for "Postion", "Rotation", and "Scale" here, which creates an
                                // implicity name dependency, but these are unlikely to change.
                                CAnimParamType paramType = GetIEditor()->GetMovieSystem()->GetEntityNodeParamType(i);
                                CAnimParamType transformPropertyParamType;
                                bool createTransformTrack = false;
                                if (paramType.GetType() == eAnimParamType_Position)
                                {
                                    transformPropertyParamType = "Position";
                                    createTransformTrack = true;
                                }
                                else if (paramType.GetType() == eAnimParamType_Rotation)
                                {
                                    transformPropertyParamType = "Rotation";
                                    createTransformTrack = true;
                                }
                                else if (paramType.GetType() == eAnimParamType_Scale)
                                {
                                    transformPropertyParamType = "Scale";
                                    createTransformTrack = true;
                                }

                                if (createTransformTrack)
                                {
                                    transformPropertyParamType = paramType.GetType();              // this sets the type to one of eAnimParamType_Position, eAnimParamType_Rotation or eAnimParamType_Scale  but maintains the name
                                    transformComponentNode->CreateTrack(transformPropertyParamType);
                                }
                            }
                        }
                    } 
                }
            }
        }      
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNodeBundle::AppendAnimNode(CTrackViewAnimNode* pNode)
{
    stl::push_back_unique(m_animNodes, pNode);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNodeBundle::AppendAnimNodeBundle(const CTrackViewAnimNodeBundle& bundle)
{
    for (auto iter = bundle.m_animNodes.begin(); iter != bundle.m_animNodes.end(); ++iter)
    {
        AppendAnimNode(*iter);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNodeBundle::ExpandAll(bool bAlsoExpandParentNodes)
{
    std::set<CTrackViewNode*> nodesToExpand;
    std::copy(m_animNodes.begin(), m_animNodes.end(), std::inserter(nodesToExpand, nodesToExpand.end()));

    if (bAlsoExpandParentNodes)
    {
        for (auto iter = nodesToExpand.begin(); iter != nodesToExpand.end(); ++iter)
        {
            CTrackViewNode* pNode = *iter;

            for (CTrackViewNode* pParent = pNode->GetParentNode(); pParent; pParent = pParent->GetParentNode())
            {
                nodesToExpand.insert(pParent);
            }
        }
    }

    for (auto iter = nodesToExpand.begin(); iter != nodesToExpand.end(); ++iter)
    {
        (*iter)->SetExpanded(true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNodeBundle::CollapseAll()
{
    for (auto iter = m_animNodes.begin(); iter != m_animNodes.end(); ++iter)
    {
        (*iter)->SetExpanded(false);
    }
}

//////////////////////////////////////////////////////////////////////////
const bool CTrackViewAnimNodeBundle::DoesContain(const CTrackViewNode* pTargetNode)
{
    return stl::find(m_animNodes, pTargetNode);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNodeBundle::Clear()
{
    m_animNodes.clear();
}

//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNode::CTrackViewAnimNode(IAnimSequence* pSequence, IAnimNode* pAnimNode, CTrackViewNode* pParentNode)
    : CTrackViewNode(pParentNode)
    , m_pAnimSequence(pSequence)
    , m_pAnimNode(pAnimNode)
    , m_pNodeEntity(nullptr)
    , m_pNodeAnimator(nullptr)
    , m_trackGizmo(nullptr)
{
    if (pAnimNode)
    {
        // Search for child nodes
        const int nodeCount = pSequence->GetNodeCount();
        for (int i = 0; i < nodeCount; ++i)
        {
            IAnimNode* pNode = pSequence->GetNode(i);
            IAnimNode* pParentNode = pNode->GetParent();

            // If our node is the parent, then the current node is a child of it
            if (pAnimNode == pParentNode)
            {
                CTrackViewAnimNodeFactory animNodeFactory;
                CTrackViewAnimNode* pNewTVAnimNode = animNodeFactory.BuildAnimNode(pSequence, pNode, this);
                m_childNodes.push_back(std::unique_ptr<CTrackViewNode>(pNewTVAnimNode));
            }
        }

        // Copy tracks from pAnimNode
        const int trackCount = pAnimNode->GetTrackCount();
        for (int i = 0; i < trackCount; ++i)
        {
            IAnimTrack* pTrack = pAnimNode->GetTrackByIndex(i);

            CTrackViewTrackFactory trackFactory;
            CTrackViewTrack* pNewTVTrack = trackFactory.BuildTrack(pTrack, this, this);
            m_childNodes.push_back(std::unique_ptr<CTrackViewNode>(pNewTVTrack));
        }

        // Set owner to update entity CryMovie entity IDs and remove it again
        SetNodeEntity(GetNodeEntity());
    }

    SortNodes();

    m_bExpanded = IsGroupNode() || (pParentNode == nullptr);

    switch (GetType())
    {
    case eAnimNodeType_Comment:
        m_pNodeAnimator.reset(new CCommentNodeAnimator(this));
        break;
    case eAnimNodeType_Layer:
        m_pNodeAnimator.reset(new CLayerNodeAnimator());
        break;
    case eAnimNodeType_Director:
        m_pNodeAnimator.reset(new CDirectorNodeAnimator(this));
        break;
    }

    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
}

//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNode::~CTrackViewAnimNode()
{
    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
    if (m_pAnimNode && m_pAnimNode->GetAzEntityId().IsValid())
    {
        AZ::EntityBus::Handler::BusDisconnect(m_pAnimNode->GetAzEntityId());
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::BindToEditorObjects()
{
    if (!IsActive())
    {
        return;
    }

    CTrackViewSequenceNotificationContext context(GetSequence());

    CTrackViewAnimNode* pDirector = GetDirector();
    const bool bBelongsToActiveDirector = pDirector ? pDirector->IsActiveDirector() : true;

    if (bBelongsToActiveDirector)
    {
        IObjectManager* pObjectManager = GetIEditor()->GetObjectManager();
        CEntityObject* pEntity = (CEntityObject*)pObjectManager->FindAnimNodeOwner(this);
        bool ownerChanged = false;
        if (m_pNodeAnimator)
        {
            m_pNodeAnimator->Bind(this);
        }

        if (m_pAnimNode)
        {
            m_pAnimNode->SetNodeOwner(this);
            ownerChanged = true;
        }

        if (pEntity)
        {
            pEntity->SetTransformDelegate(this);
            pEntity->RegisterListener(this);
            SetNodeEntity(pEntity);
        }

        if (ownerChanged)
        {
            GetSequence()->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_NodeOwnerChanged);
        }

        for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
        {
            CTrackViewNode* pChildNode = (*iter).get();
            if (pChildNode->GetNodeType() == eTVNT_AnimNode)
            {
                CTrackViewAnimNode* pChildAnimNode = (CTrackViewAnimNode*)pChildNode;
                pChildAnimNode->BindToEditorObjects();
            }
        }
    }

    UpdateTrackGizmo();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::UnBindFromEditorObjects()
{
    CTrackViewSequenceNotificationContext context(GetSequence());

    IObjectManager* pObjectManager = GetIEditor()->GetObjectManager();
    CEntityObject* pEntity = (CEntityObject*)pObjectManager->FindAnimNodeOwner(this);

    if (pEntity)
    {
        pEntity->SetTransformDelegate(nullptr);
        pEntity->UnregisterListener(this);
    }

    if (m_pAnimNode)
    {
        // 'Owner' is the TrackViewNode, as opposed to the EditorEntityNode (as 'owner' is used in animSequence, or the pEntity 
        // returned from FindAnimNodeOwner() - confusing, isn't it?
        m_pAnimNode->SetNodeOwner(nullptr);
    }

    if (m_pNodeAnimator)
    {
        m_pNodeAnimator->UnBind(this);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = (CTrackViewAnimNode*)pChildNode;
            pChildAnimNode->UnBindFromEditorObjects();
        }
    }

    GetIEditor()->GetObjectManager()->GetGizmoManager()->RemoveGizmo(m_trackGizmo);
    m_trackGizmo = nullptr;
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsBoundToEditorObjects() const
{
    if (m_pAnimNode)
    {
        if (m_pAnimNode->GetType() == eAnimNodeType_AzEntity)
        {
            // check if bound to comoponent entity
            return m_pAnimNode->GetAzEntityId().IsValid();
        }
        else
        {
            // check if bound to legacy entity
            return (m_pAnimNode->GetNodeOwner() != NULL);
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SyncToConsole(SAnimContext& animContext)
{
    switch (GetType())
    {
    case eAnimNodeType_Camera:
    {
        IEntity* pEntity = GetEntity();
        if (pEntity)
        {
            CBaseObject* pCameraObject = GetIEditor()->GetObjectManager()->FindObject(GetIEditor()->GetViewManager()->GetCameraObjectId());
            IEntity* pCameraEntity = pCameraObject ? ((CEntityObject*)pCameraObject)->GetIEntity() : NULL;
            if (pCameraEntity && pEntity->GetId() == pCameraEntity->GetId())
            // If this camera is currently active,
            {
                Matrix34 viewTM = pEntity->GetWorldTM();
                Vec3 oPosition(viewTM.GetTranslation());
                Vec3 oDirection(viewTM.TransformVector(FORWARD_DIRECTION));
            }
        }
    }
    break;
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = (CTrackViewAnimNode*)pChildNode;
            pChildAnimNode->SyncToConsole(animContext);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNode* CTrackViewAnimNode::CreateSubNode(const QString& name, const EAnimNodeType animNodeType, CEntityObject* pOwner , AZ::Uuid componentTypeId, AZ::ComponentId componentId)
{
    assert(CUndo::IsRecording());

    const bool bIsGroupNode = IsGroupNode();
    assert(bIsGroupNode);
    if (!bIsGroupNode)
    {
        return nullptr;
    }

    CTrackViewAnimNode* pDirector = nullptr;
    const auto nameStr = name.toLatin1();

    // Check if the node's director or sequence already contains a node with this name, unless it's a component, for which we allow duplicate names since
    // Components are children of unique AZEntities in Track View
    if (animNodeType != eAnimNodeType_Component)
    {
        pDirector = (GetType() == eAnimNodeType_Director) ? this : GetDirector();
        pDirector = pDirector ? pDirector : GetSequence();
        if (pDirector->GetAnimNodesByName(nameStr.constData()).GetCount() > 0)
        {
            GetIEditor()->GetMovieSystem()->LogUserNotificationMsg(AZStd::string::format("'%s' already exists in sequence '%s', skipping...", nameStr.constData(), pDirector->GetName()));
            return nullptr;
        }
    }

    // Create CryMovie and TrackView node
    IAnimNode* pNewAnimNode = m_pAnimSequence->CreateNode(animNodeType);
    if (!pNewAnimNode)
    {
        GetIEditor()->GetMovieSystem()->LogUserNotificationMsg(AZStd::string::format("Failed to add '%s' to sequence '%s'.", nameStr.constData(), pDirector->GetName()));
        return nullptr;
    }

    pNewAnimNode->SetName(nameStr.constData());
    pNewAnimNode->CreateDefaultTracks();
    pNewAnimNode->SetParent(m_pAnimNode.get());
    pNewAnimNode->SetComponent(componentId, componentTypeId);

    CTrackViewAnimNodeFactory animNodeFactory;
    CTrackViewAnimNode* pNewNode = animNodeFactory.BuildAnimNode(m_pAnimSequence, pNewAnimNode, this);
    pNewNode->m_bExpanded = true;

    // Make sure that camera and entity nodes get created with an owner
    assert((animNodeType != eAnimNodeType_Camera && animNodeType != eAnimNodeType_Entity) || pOwner);

    pNewNode->SetNodeEntity(pOwner);
    pNewAnimNode->SetNodeOwner(pNewNode);

    pNewNode->BindToEditorObjects();

    AddNode(pNewNode);

    // Legacy sequence use Track View Undo, New Component based Sequence
    // use the AZ Undo system and allow sequence entity to be the source of truth.
    if (m_pAnimSequence->GetSequenceType() == eSequenceType_Legacy)
    {
        CUndo::Record(new CUndoAnimNodeAdd(pNewNode));
    }
    else
    {
        // Add node to sequence, let AZ Undo take care of undo/redo
        m_pAnimSequence->AddNode(pNewNode->m_pAnimNode.get());
    }

    return pNewNode;
}

// Helper function to remove a child node
void CTrackViewAnimNode::RemoveChildNode(CTrackViewAnimNode* child)
{
    assert(child);
    auto parent = static_cast<CTrackViewAnimNode*>(child->m_pParentNode);
    assert(parent);

    child->UnBindFromEditorObjects();

    for (auto iter = parent->m_childNodes.begin(); iter != parent->m_childNodes.end(); ++iter)
    {
        std::unique_ptr<CTrackViewNode>& currentNode = *iter;

        if (currentNode.get() == child)
        {
            parent->m_childNodes.erase(iter);
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::RemoveSubNode(CTrackViewAnimNode* pSubNode)
{
    assert(CUndo::IsRecording());

    const bool bIsGroupNode = IsGroupNode();
    assert(bIsGroupNode);
    if (!bIsGroupNode)
    {
        return;
    }

    // also remove all child component nodes of AzEntity nodes when we remove and AzEntity node
    if (pSubNode->GetType() == eAnimNodeType_AzEntity)
    {
        for (int i = pSubNode->GetChildCount(); --i >= 0;)
        {
            if (pSubNode->GetChild(i)->GetNodeType() == eTVNT_AnimNode)
            {
                CTrackViewAnimNode* childAnimNode = static_cast<CTrackViewAnimNode*>(pSubNode->GetChild(i));
                if (childAnimNode->GetType() == eAnimNodeType_Component)
                {
                    // Legacy sequence use Track View Undo, New Component based Sequence
                    // use the AZ Undo system and allow sequence entity to be the source of truth.
                    if (m_pAnimSequence->GetSequenceType() == eSequenceType_Legacy)
                    {
                        CUndo::Record(new CUndoAnimNodeRemove(childAnimNode));
                    }
                    else
                    {
                        // Remove node from sequence entity, let AZ Undo take care of undo/redo
                        m_pAnimSequence->RemoveNode(childAnimNode->m_pAnimNode.get(), /*removeChildRelationships=*/ false);
                        childAnimNode->GetSequence()->OnNodeChanged(childAnimNode, ITrackViewSequenceListener::eNodeChangeType_Removed);
                        RemoveChildNode(childAnimNode);
                    }
                }
            }
        }
    }

    // Legacy sequence use Track View Undo, New Component based Sequence
    // use the AZ Undo system and allow sequence entity to be the source of truth.
    if (m_pAnimSequence->GetSequenceType() == eSequenceType_Legacy)
    {
        CUndo::Record(new CUndoAnimNodeRemove(pSubNode));
    }
    else
    {
        // Remove node from sequence entity, let AZ Undo take care of undo/redo
        m_pAnimSequence->RemoveNode(pSubNode->m_pAnimNode.get(), /*removeChildRelationships=*/ false);
        pSubNode->GetSequence()->OnNodeChanged(pSubNode, ITrackViewSequenceListener::eNodeChangeType_Removed);
        RemoveChildNode(pSubNode);
    }
}

//////////////////////////////////////////////////////////////////////////
CTrackViewTrack* CTrackViewAnimNode::CreateTrack(const CAnimParamType& paramType)
{
    assert(CUndo::IsRecording());

    if (GetTrackForParameter(paramType) && !(GetParamFlags(paramType) & IAnimNode::eSupportedParamFlags_MultipleTracks))
    {
        return nullptr;
    }

    // Create CryMovie track
    IAnimTrack* pNewAnimTrack = m_pAnimNode->CreateTrack(paramType);
    if (!pNewAnimTrack)
    {
        return nullptr;
    }

    // Create Track View Track
    CTrackViewTrackFactory trackFactory;
    CTrackViewTrack* pNewTrack = trackFactory.BuildTrack(pNewAnimTrack, this, this);

    AddNode(pNewTrack);
    // Legacy sequence use Track View Undo, New Component based Sequence
    // use the AZ Undo system and allow sequence entity to be the source of truth.
    if (m_pAnimSequence->GetSequenceType() == eSequenceType_Legacy)
    {
        CUndo::Record(new CUndoTrackAdd(pNewTrack));
    }

    MarkAsModified();

    SetPosRotScaleTracksDefaultValues();

    return pNewTrack;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::RemoveTrack(CTrackViewTrack* pTrack)
{
    assert(CUndo::IsRecording());
    assert(!pTrack->IsSubTrack());

    if (!pTrack->IsSubTrack())
    {
        CUndo::Record(new CUndoTrackRemove(pTrack));
    }
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::SnapTimeToPrevKey(float& time) const
{
    const float startTime = time;
    float closestTrackTime = std::numeric_limits<float>::min();
    bool bFoundPrevKey = false;

    for (size_t i = 0; i < m_childNodes.size(); ++i)
    {
        const CTrackViewNode* pNode = m_childNodes[i].get();

        float closestNodeTime = startTime;
        if (pNode->SnapTimeToPrevKey(closestNodeTime))
        {
            closestTrackTime = std::max(closestNodeTime, closestTrackTime);
            bFoundPrevKey = true;
        }
    }

    if (bFoundPrevKey)
    {
        time = closestTrackTime;
    }

    return bFoundPrevKey;
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::SnapTimeToNextKey(float& time) const
{
    const float startTime = time;
    float closestTrackTime = std::numeric_limits<float>::max();
    bool bFoundNextKey = false;

    for (size_t i = 0; i < m_childNodes.size(); ++i)
    {
        const CTrackViewNode* pNode = m_childNodes[i].get();

        float closestNodeTime = startTime;
        if (pNode->SnapTimeToNextKey(closestNodeTime))
        {
            closestTrackTime = std::min(closestNodeTime, closestTrackTime);
            bFoundNextKey = true;
        }
    }

    if (bFoundNextKey)
    {
        time = closestTrackTime;
    }

    return bFoundNextKey;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewKeyBundle CTrackViewAnimNode::GetSelectedKeys()
{
    CTrackViewKeyBundle bundle;

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        bundle.AppendKeyBundle((*iter)->GetSelectedKeys());
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewKeyBundle CTrackViewAnimNode::GetAllKeys()
{
    CTrackViewKeyBundle bundle;

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        bundle.AppendKeyBundle((*iter)->GetAllKeys());
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewKeyBundle CTrackViewAnimNode::GetKeysInTimeRange(const float t0, const float t1)
{
    CTrackViewKeyBundle bundle;

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        bundle.AppendKeyBundle((*iter)->GetKeysInTimeRange(t0, t1));
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewTrackBundle CTrackViewAnimNode::GetAllTracks()
{
    return GetTracks(false, CAnimParamType());
}

//////////////////////////////////////////////////////////////////////////
CTrackViewTrackBundle CTrackViewAnimNode::GetSelectedTracks()
{
    return GetTracks(true, CAnimParamType());
}

//////////////////////////////////////////////////////////////////////////
CTrackViewTrackBundle CTrackViewAnimNode::GetTracksByParam(const CAnimParamType& paramType) const
{
    return GetTracks(false, paramType);
}

//////////////////////////////////////////////////////////////////////////
CTrackViewTrackBundle CTrackViewAnimNode::GetTracks(const bool bOnlySelected, const CAnimParamType& paramType) const
{
    CTrackViewTrackBundle bundle;

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pNode = (*iter).get();

        if (pNode->GetNodeType() == eTVNT_Track)
        {
            CTrackViewTrack* pTrack = static_cast<CTrackViewTrack*>(pNode);

            if (paramType != eAnimParamType_Invalid && pTrack->GetParameterType() != paramType)
            {
                continue;
            }

            if (!bOnlySelected || pTrack->IsSelected())
            {
                bundle.AppendTrack(pTrack);
            }

            const unsigned int subTrackCount = pTrack->GetChildCount();
            for (unsigned int subTrackIndex = 0; subTrackIndex < subTrackCount; ++subTrackIndex)
            {
                CTrackViewTrack* pSubTrack = static_cast<CTrackViewTrack*>(pTrack->GetChild(subTrackIndex));
                if (!bOnlySelected || pSubTrack->IsSelected())
                {
                    bundle.AppendTrack(pSubTrack);
                }
            }
        }
        else if (pNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pAnimNode = static_cast<CTrackViewAnimNode*>(pNode);
            bundle.AppendTrackBundle(pAnimNode->GetTracks(bOnlySelected, paramType));
        }
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
EAnimNodeType CTrackViewAnimNode::GetType() const
{
    return m_pAnimNode ? m_pAnimNode->GetType() : eAnimNodeType_Invalid;
}

//////////////////////////////////////////////////////////////////////////
EAnimNodeFlags CTrackViewAnimNode::GetFlags() const
{
    return m_pAnimNode ? (EAnimNodeFlags)m_pAnimNode->GetFlags() : (EAnimNodeFlags)0;
}

bool CTrackViewAnimNode::AreFlagsSetOnNodeOrAnyParent(EAnimNodeFlags flagsToCheck) const
{
    return m_pAnimNode ? m_pAnimNode->AreFlagsSetOnNodeOrAnyParent(flagsToCheck) : false;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetAsActiveDirector()
{
    if (GetType() == eAnimNodeType_Director)
    {
        m_pAnimSequence->SetActiveDirector(m_pAnimNode.get());

        GetSequence()->UnBindFromEditorObjects();
        GetSequence()->BindToEditorObjects();

        GetSequence()->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_SetAsActiveDirector);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsActiveDirector() const
{
    return m_pAnimNode == m_pAnimSequence->GetActiveDirector();
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsParamValid(const CAnimParamType& param) const
{
    return m_pAnimNode ? m_pAnimNode->IsParamValid(param) : false;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewTrack* CTrackViewAnimNode::GetTrackForParameter(const CAnimParamType& paramType, uint32 index) const
{
    uint32 currentIndex = 0;

    if (GetType() == eAnimNodeType_AzEntity)
    {
        // For AzEntity, search for track on all child components - returns first track match found (note components searched in reverse)
        for (int i = GetChildCount(); --i >= 0;)
        {
            if (GetChild(i)->GetNodeType() == eTVNT_AnimNode)
            {
                CTrackViewAnimNode* componentNode = static_cast<CTrackViewAnimNode*>(GetChild(i));
                if (componentNode->GetType() == eAnimNodeType_Component)
                {
                    if (CTrackViewTrack* track = componentNode->GetTrackForParameter(paramType, index))
                    {
                        return track;
                    }
                }
            }
        }
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pNode = (*iter).get();

        if (pNode->GetNodeType() == eTVNT_Track)
        {
            CTrackViewTrack* pTrack = static_cast<CTrackViewTrack*>(pNode);

            if (pTrack->GetParameterType() == paramType)
            {
                if (currentIndex == index)
                {
                    return pTrack;
                }
                else
                {
                    ++currentIndex;
                }
            }

            if (pTrack->IsCompoundTrack())
            {
                unsigned int numChildTracks = pTrack->GetChildCount();
                for (unsigned int i = 0; i < numChildTracks; ++i)
                {
                    CTrackViewTrack* pChildTrack = static_cast<CTrackViewTrack*>(pTrack->GetChild(i));
                    if (pChildTrack->GetParameterType() == paramType)
                    {
                        if (currentIndex == index)
                        {
                            return pChildTrack;
                        }
                        else
                        {
                            ++currentIndex;
                        }
                    }
                }
            }
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::Render(const SAnimContext& ac)
{
    if (m_pNodeAnimator && IsActive())
    {
        m_pNodeAnimator->Render(this, ac);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);
            pChildAnimNode->Render(ac);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::Animate(const SAnimContext& animContext)
{
    if (m_pNodeAnimator && IsActive())
    {
        m_pNodeAnimator->Animate(this, animContext);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);
            pChildAnimNode->Animate(animContext);
        }
    }

    UpdateTrackGizmo();
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::SetName(const char* pName)
{
    // Check if the node's director already contains a node with this name
    CTrackViewAnimNode* pDirector = GetDirector();
    pDirector = pDirector ? pDirector : GetSequence();

    CTrackViewAnimNodeBundle nodes = pDirector->GetAnimNodesByName(pName);
    const uint numNodes = nodes.GetCount();
    for (uint i = 0; i < numNodes; ++i)
    {
        if (nodes.GetNode(i) != this)
        {
            return false;
        }
    }

    string oldName = GetName();
    m_pAnimNode->SetName(pName);

    if (CUndo::IsRecording())
    {
        CUndo::Record(new CUndoAnimNodeRename(this, oldName));
    }

    GetSequence()->OnNodeRenamed(this, oldName);

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::CanBeRenamed() const
{
    return (GetFlags() & eAnimNodeFlags_CanChangeName) != 0;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetNodeEntity(CEntityObject* pEntity)
{
    bool entityPointerChanged = (pEntity != m_pNodeEntity);

    m_pNodeEntity = pEntity;

    if (pEntity)
    {
        const EntityGUID guid = ToEntityGuid(pEntity->GetId());
        SetEntityGuid(guid);

        if (m_pAnimNode->GetType() == eAnimNodeType_AzEntity)
        {
            // We're connecting to a new AZ::Entity
            AZ::EntityId    sequenceComponentEntityId(m_pAnimSequence->GetOwnerId());

            AZ::EntityId id(AZ::EntityId::InvalidEntityId);
            AzToolsFramework::ComponentEntityObjectRequestBus::EventResult(id, pEntity, &AzToolsFramework::ComponentEntityObjectRequestBus::Events::GetAssociatedEntityId);

            if (!id.IsValid() && m_pAnimNode->GetAzEntityId().IsValid())
            {
                // When undoing, pEntity may not have an associated entity Id. Fall back to our stored entityId if we have one.
                id = m_pAnimNode->GetAzEntityId();
            }

            // Notify the SequenceComponent that we're binding an entity to the sequence
            Maestro::EditorSequenceComponentRequestBus::Event(sequenceComponentEntityId, &Maestro::EditorSequenceComponentRequestBus::Events::AddEntityToAnimate, id);

            if (id != m_pAnimNode->GetAzEntityId())
            {
                if (m_pAnimNode->GetAzEntityId().IsValid())
                {
                    // disconnect from bus with previous entity ID before we reset it
                    AZ::EntityBus::Handler::BusDisconnect(m_pAnimNode->GetAzEntityId());
                }

                m_pAnimNode->SetAzEntityId(id);
            }

            // connect to EntityBus for OnEntityActivated() notifications to sync components on the entity
            if (!AZ::EntityBus::Handler::BusIsConnectedId(m_pAnimNode->GetAzEntityId()))
            {
                AZ::EntityBus::Handler::BusConnect(m_pAnimNode->GetAzEntityId());
            }
        }

        if (qobject_cast<CEntityObject*>(pEntity->GetLookAt()))
        {
            CEntityObject* target = static_cast<CEntityObject*>(pEntity->GetLookAt());
            SetEntityGuidTarget(ToEntityGuid(target->GetId()));
        }
        if (qobject_cast<CEntityObject*>(pEntity->GetLookAtSource()))
        {
            CEntityObject* source = static_cast<CEntityObject*>(pEntity->GetLookAtSource());
            SetEntityGuidSource(ToEntityGuid(source->GetId()));
        }

        if (entityPointerChanged)
        {
            SetPosRotScaleTracksDefaultValues();
        }

        OnSelectionChanged(pEntity->IsSelected());
    }
}

//////////////////////////////////////////////////////////////////////////
CEntityObject* CTrackViewAnimNode::GetNodeEntity(const bool bSearch)
{
    CEntityObject* entityObject = nullptr;

    if (m_pAnimNode)
    {
        if (m_pNodeEntity)
        {
            entityObject = m_pNodeEntity;
        }
        else if (bSearch)
        {
            // First Search with object manager
            EntityGUID* pGuid = GetEntityGuid();
            entityObject = static_cast<CEntityObject*>(GetIEditor()->GetObjectManager()->FindAnimNodeOwner(this));

            // if not found, search with AZ::EntityId
            if (!entityObject && IsBoundToAzEntity())
            {
                // Search AZ::Entities
                AzToolsFramework::ComponentEntityEditorRequestBus::EventResult(entityObject, GetAzEntityId(), &AzToolsFramework::ComponentEntityEditorRequestBus::Events::GetSandboxObject);
            }
        }
    }

    return entityObject;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNodeBundle CTrackViewAnimNode::GetAllAnimNodes()
{
    CTrackViewAnimNodeBundle bundle;

    if (GetNodeType() == eTVNT_AnimNode)
    {
        bundle.AppendAnimNode(this);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);
            bundle.AppendAnimNodeBundle(pChildAnimNode->GetAllAnimNodes());
        }
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNodeBundle CTrackViewAnimNode::GetSelectedAnimNodes()
{
    CTrackViewAnimNodeBundle bundle;

    if ((GetNodeType() == eTVNT_AnimNode || GetNodeType() == eTVNT_Sequence) && IsSelected())
    {
        bundle.AppendAnimNode(this);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);
            bundle.AppendAnimNodeBundle(pChildAnimNode->GetSelectedAnimNodes());
        }
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNodeBundle CTrackViewAnimNode::GetAllOwnedNodes(const CEntityObject* pOwner)
{
    CTrackViewAnimNodeBundle bundle;

    if (GetNodeType() == eTVNT_AnimNode && GetNodeEntity() == pOwner)
    {
        bundle.AppendAnimNode(this);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);
            bundle.AppendAnimNodeBundle(pChildAnimNode->GetAllOwnedNodes(pOwner));
        }
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNodeBundle CTrackViewAnimNode::GetAnimNodesByType(EAnimNodeType animNodeType)
{
    CTrackViewAnimNodeBundle bundle;

    if (GetNodeType() == eTVNT_AnimNode && GetType() == animNodeType)
    {
        bundle.AppendAnimNode(this);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);
            bundle.AppendAnimNodeBundle(pChildAnimNode->GetAnimNodesByType(animNodeType));
        }
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNodeBundle CTrackViewAnimNode::GetAnimNodesByName(const char* pName)
{
    CTrackViewAnimNodeBundle bundle;

    QString nodeName = GetName();
    if (GetNodeType() == eTVNT_AnimNode && QString::compare(pName, nodeName, Qt::CaseInsensitive) == 0)
    {
        bundle.AppendAnimNode(this);
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);
            bundle.AppendAnimNodeBundle(pChildAnimNode->GetAnimNodesByName(pName));
        }
    }

    return bundle;
}

//////////////////////////////////////////////////////////////////////////
const char* CTrackViewAnimNode::GetParamName(const CAnimParamType& paramType) const
{
    const char* pName = m_pAnimNode->GetParamName(paramType);
    return pName ? pName : "";
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsGroupNode() const
{
    EAnimNodeType nodeType = GetType();

    // AZEntities are really just containers for components, so considered a 'Group' node
    return nodeType == eAnimNodeType_Director || nodeType == eAnimNodeType_Group || nodeType == eAnimNodeType_AzEntity;
}

//////////////////////////////////////////////////////////////////////////
QString CTrackViewAnimNode::GetAvailableNodeNameStartingWith(const QString& name) const
{
    QString newName = name;
    unsigned int index = 2;

    while (const_cast<CTrackViewAnimNode*>(this)->GetAnimNodesByName(newName.toLatin1().data()).GetCount() > 0)
    {
        newName = QStringLiteral("%1%2").arg(name).arg(index);
        ++index;
    }

    return newName;
}

EAnimNodeType CTrackViewAnimNode::GetAnimNodeTypeFromObject(const CBaseObject* object) const
{
    EAnimNodeType retType = eAnimNodeType_Invalid;

    if (qobject_cast<const CCameraObject*>(object))
    {
        retType = eAnimNodeType_Camera;
    }
#if defined(USE_GEOM_CACHES)
    else if (qobject_cast<const CGeomCacheEntity*>(object))
    {
        retType = eAnimNodeType_GeomCache;
    }
#endif
    else if (qobject_cast<const CEntityObject*>(object))
    {
        const AZ::EntityId entityId(qobject_cast<const CEntityObject*>(object)->GetEntityId());
        if (IsLegacyEntityId(entityId))
        {
            retType = eAnimNodeType_Entity;
        }
        else
        {
            retType = eAnimNodeType_AzEntity;
        }
    }
    
    return retType;
}


//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNodeBundle CTrackViewAnimNode::AddSelectedEntities(const DynArray<unsigned int>& defaultTrackCount)
{
    assert(IsGroupNode());
    assert(CUndo::IsRecording());

    CTrackViewAnimNodeBundle addedNodes;

    // Add selected nodes.
    CSelectionGroup* pSelection = GetIEditor()->GetSelection();
    for (int i = 0; i < pSelection->GetCount(); i++)
    {
        CBaseObject* pObject = pSelection->GetObject(i);
        if (!pObject)
        {
            continue;
        }

        // Check if object already assigned to some AnimNode.
        CTrackViewAnimNode* pExistingNode = GetIEditor()->GetSequenceManager()->GetActiveAnimNode(static_cast<const CEntityObject*>(pObject));
        if (pExistingNode)
        {
            // If it has the same director than the current node, reject it
            if (pExistingNode->GetDirector() == GetDirector())
            {
                GetIEditor()->GetMovieSystem()->LogUserNotificationMsg(AZStd::string::format("'%s' was already added to '%s', skipping...", pObject->GetName().toLatin1().data(), GetDirector()->GetName()));
                continue;
            }
        }

        EAnimNodeType nodeType = GetAnimNodeTypeFromObject(pObject);
        CTrackViewAnimNode* pAnimNode = CreateSubNode(pObject->GetName(), nodeType, static_cast<CEntityObject*>(pObject));

        if (pAnimNode)
        {
            CUndo undo("Add Default Tracks");

            CreateDefaultTracksForEntityNode(pAnimNode, defaultTrackCount);

            addedNodes.AppendAnimNode(pAnimNode);
        }
    }

    return addedNodes;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::AddCurrentLayer()
{
    assert(IsGroupNode());

    CObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetLayersManager();
    CObjectLayer* pLayer = pLayerManager->GetCurrentLayer();
    const QString name = pLayer->GetName();

    CreateSubNode(name, eAnimNodeType_Entity);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetEntityGuidTarget(const EntityGUID& guid)
{
    if (m_pAnimNode)
    {
        m_pAnimNode->SetEntityGuidTarget(guid);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetEntityGuid(const EntityGUID& guid)
{
    if (m_pAnimNode)
    {
        m_pAnimNode->SetEntityGuid(guid);
    }
}

//////////////////////////////////////////////////////////////////////////
EntityGUID* CTrackViewAnimNode::GetEntityGuid() const
{
    return m_pAnimNode ? m_pAnimNode->GetEntityGuid() : nullptr;
}

//////////////////////////////////////////////////////////////////////////
IEntity* CTrackViewAnimNode::GetEntity() const
{
    return m_pAnimNode ? m_pAnimNode->GetEntity() : nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetEntityGuidSource(const EntityGUID& guid)
{
    if (m_pAnimNode)
    {
        m_pAnimNode->SetEntityGuidSource(guid);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetAsViewCamera()
{
    assert (GetType() == eAnimNodeType_Camera);

    if (GetType() == eAnimNodeType_Camera)
    {
        CEntityObject* pCameraEntity = GetNodeEntity();
        CRenderViewport* pRenderViewport = static_cast<CRenderViewport*>(GetIEditor()->GetViewManager()->GetGameViewport());
        pRenderViewport->SetCameraObject(pCameraEntity);
    }
}

//////////////////////////////////////////////////////////////////////////
unsigned int CTrackViewAnimNode::GetParamCount() const
{
    return m_pAnimNode ? m_pAnimNode->GetParamCount() : 0;
}

//////////////////////////////////////////////////////////////////////////
CAnimParamType CTrackViewAnimNode::GetParamType(unsigned int index) const
{
    unsigned int paramCount = GetParamCount();
    if (!m_pAnimNode || index >= paramCount)
    {
        return eAnimParamType_Invalid;
    }

    return m_pAnimNode->GetParamType(index);
}

//////////////////////////////////////////////////////////////////////////
IAnimNode::ESupportedParamFlags CTrackViewAnimNode::GetParamFlags(const CAnimParamType& paramType) const
{
    if (m_pAnimNode)
    {
        return m_pAnimNode->GetParamFlags(paramType);
    }

    return IAnimNode::ESupportedParamFlags(0);
}

//////////////////////////////////////////////////////////////////////////
EAnimValue CTrackViewAnimNode::GetParamValueType(const CAnimParamType& paramType) const
{
    if (m_pAnimNode)
    {
        return m_pAnimNode->GetParamValueType(paramType);
    }

    return eAnimValue_Unknown;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::UpdateDynamicParams()
{
    if (m_pAnimNode)
    {
        m_pAnimNode->UpdateDynamicParams();
    }

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);
            pChildAnimNode->UpdateDynamicParams();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::CopyKeysToClipboard(XmlNodeRef& xmlNode, const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks)
{
    XmlNodeRef childNode = xmlNode->createNode("Node");
    childNode->setAttr("name", GetName());
    childNode->setAttr("type", GetType());

    for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        pChildNode->CopyKeysToClipboard(childNode, bOnlySelectedKeys, bOnlyFromSelectedTracks);
    }

    if (childNode->getChildCount() > 0)
    {
        xmlNode->addChild(childNode);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::CopyNodesToClipboard(const bool bOnlySelected, QWidget* context)
{
    XmlNodeRef animNodesRoot = XmlHelpers::CreateXmlNode("CopyAnimNodesRoot");

    CopyNodesToClipboardRec(this, animNodesRoot, bOnlySelected);

    CClipboard clipboard(context);
    clipboard.Put(animNodesRoot, "Track view entity nodes");
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::CopyNodesToClipboardRec(CTrackViewAnimNode* pCurrentAnimNode, XmlNodeRef& xmlNode, const bool bOnlySelected)
{
    if (pCurrentAnimNode->m_pAnimNode && (!bOnlySelected || pCurrentAnimNode->IsSelected()))
    {
        XmlNodeRef childXmlNode = xmlNode->newChild("Node");
        pCurrentAnimNode->m_pAnimNode->Serialize(childXmlNode, false, true);
    }

    for (auto iter = pCurrentAnimNode->m_childNodes.begin(); iter != pCurrentAnimNode->m_childNodes.end(); ++iter)
    {
        CTrackViewNode* pChildNode = (*iter).get();
        if (pChildNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);

            // If selected and group node, force copying of children
            const bool bSelectedAndGroupNode = pCurrentAnimNode->IsSelected() && pCurrentAnimNode->IsGroupNode();
            CopyNodesToClipboardRec(pChildAnimNode, xmlNode, !bSelectedAndGroupNode && bOnlySelected);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::PasteTracksFrom(XmlNodeRef& xmlNodeWithTracks)
{
    assert(CUndo::IsRecording());

    // we clear our own tracks first because calling SerializeAnims() will clear out m_pAnimNode's tracks below
    CTrackViewTrackBundle allTracksBundle = GetAllTracks();
    for (int i = allTracksBundle.GetCount(); --i >= 0;)
    {
        RemoveTrack(allTracksBundle.GetTrack(i));
    }

    // serialize all the tracks from xmlNode - note this will first delete all existing tracks on m_pAnimNode
    m_pAnimNode->SerializeAnims(xmlNodeWithTracks, true, true);

    // create TrackView tracks
    const int trackCount = m_pAnimNode->GetTrackCount();
    for (int i = 0; i < trackCount; ++i)
    {
        IAnimTrack* pTrack = m_pAnimNode->GetTrackByIndex(i);

        CTrackViewTrackFactory trackFactory;
        CTrackViewTrack* newTrackNode = trackFactory.BuildTrack(pTrack, this, this);

        AddNode(newTrackNode);
        // Legacy sequence use Track View Undo, New Component based Sequence
        // use the AZ Undo system and allow sequence entity to be the source of truth.
        if (m_pAnimSequence->GetSequenceType() == eSequenceType_Legacy)
        {
            CUndo::Record(new CUndoTrackAdd(newTrackNode));
        }
        MarkAsModified();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::PasteNodesFromClipboard(QWidget* context)
{
    assert(CUndo::IsRecording());

    CClipboard clipboard(context);
    if (clipboard.IsEmpty())
    {
        return false;
    }

    XmlNodeRef animNodesRoot = clipboard.Get();
    if (animNodesRoot == NULL || strcmp(animNodesRoot->getTag(), "CopyAnimNodesRoot") != 0)
    {
        return false;
    }

    const bool bLightAnimationSetActive = GetSequence()->GetFlags() & IAnimSequence::eSeqFlags_LightAnimationSet;

    AZStd::map<int, IAnimNode*> copiedIdToNodeMap;
    const unsigned int numNodes = animNodesRoot->getChildCount();
    for (int i = 0; i < numNodes; ++i)
    {
        XmlNodeRef xmlNode = animNodesRoot->getChild(i);

        // skip non-light nodes in light animation sets
        int type;
        if (!xmlNode->getAttr("Type", type) || (bLightAnimationSetActive && (EAnimNodeType)type != eAnimNodeType_Light))
        {
            continue;
        }

        PasteNodeFromClipboard(copiedIdToNodeMap, xmlNode);
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::PasteNodeFromClipboard(AZStd::map<int, IAnimNode*>& copiedIdToNodeMap, XmlNodeRef xmlNode)
{
    QString name;

    if (!xmlNode->getAttr("Name", name))
    {
        return;
    }

    // can only paste nodes into a groupNode (i.e. accepts children)
    const bool bIsGroupNode = IsGroupNode();
    assert(bIsGroupNode);
    if (!bIsGroupNode)
    {
        return;
    }

    EAnimNodeType nodeType;
    GetIEditor()->GetMovieSystem()->SerializeNodeType(nodeType, xmlNode, /*bLoading=*/ true, IAnimSequence::kSequenceVersion, m_pAnimSequence->GetFlags());
    
    if (nodeType == eAnimNodeType_Component)
    {
        // When pasting Component Nodes, the parent Component Entity Node would have already added all its Components as part of its OnEntityActivated() sync.
        // Here we need to go copy any Components Tracks as well. For pasting, Components matched by ComponentId, which assumes the pasted Track View Component Node
        // refers to the copied Component Entity referenced in the level

        // Find the pasted parent Component Entity Node
        int parentId = 0;
        xmlNode->getAttr("ParentNode", parentId);
        if (copiedIdToNodeMap.find(parentId) != copiedIdToNodeMap.end())
        {
            CTrackViewAnimNode* componentEntityNode = FindNodeByAnimNode(copiedIdToNodeMap[parentId]);
            if (componentEntityNode)
            {
                // Find the copied Component Id on the pasted Component Entity Node, if it exists
                AZ::ComponentId componentId = AZ::InvalidComponentId;
                xmlNode->getAttr("ComponentId", componentId);

                for (int i = componentEntityNode->GetChildCount(); --i >= 0;)
                {
                    CTrackViewNode* childNode = componentEntityNode->GetChild(i);
                    if (childNode->GetNodeType() == eTVNT_AnimNode)
                    {
                        CTrackViewAnimNode* componentNode = static_cast<CTrackViewAnimNode*>(childNode);

                        if (componentNode->GetComponentId() == componentId)
                        {
                            componentNode->PasteTracksFrom(xmlNode);
                            break;
                        }
                    }
                }
            }
        }   
    }
    else
    {
        // Pasting a non-Component Node - create and add nodes to CryMovie and TrackView

        // Check if the node's director or sequence already contains a node with this name
        CTrackViewAnimNode* pDirector = GetDirector();
        pDirector = pDirector ? pDirector : GetSequence();
        if (pDirector->GetAnimNodesByName(name.toLatin1().data()).GetCount() > 0)
        {
            return;
        }

        IAnimNode* pNewAnimNode = m_pAnimSequence->CreateNode(xmlNode);
        if (!pNewAnimNode)
        {
            return;
        }

        // add new node to mapping of copied Id's to pasted nodes
        int id;
        xmlNode->getAttr("Id", id);
        copiedIdToNodeMap[id] = pNewAnimNode;

        // search for the parent Node among the pasted nodes - if not found, parent to the group node doing the pasting
        IAnimNode* parentAnimNode = m_pAnimNode.get();
        int parentId = 0;
        if (xmlNode->getAttr("ParentNode", parentId))
        {
            if (copiedIdToNodeMap.find(parentId) != copiedIdToNodeMap.end())
            {
                parentAnimNode = copiedIdToNodeMap[parentId];
            }
        }
        pNewAnimNode->SetParent(parentAnimNode);

        // Find the TrackViewNode corresponding to the parentNode
        CTrackViewAnimNode* parentNode = FindNodeByAnimNode(parentAnimNode);
        if (!parentNode)
        {
            parentNode = this;
        }

        CTrackViewAnimNodeFactory animNodeFactory;
        CTrackViewAnimNode* pNewNode = animNodeFactory.BuildAnimNode(m_pAnimSequence, pNewAnimNode, parentNode);
        pNewNode->m_bExpanded = true;

        parentNode->AddNode(pNewNode);

        // Legacy sequence use Track View Undo, New Component based Sequence
        // use the AZ Undo system and allow sequence entity to be the source of truth.
        if (m_pAnimSequence->GetSequenceType() == eSequenceType_Legacy)
        {
            CUndo::Record(new CUndoAnimNodeAdd(pNewNode));
        }
        else
        {
            // Add node to sequence, let AZ Undo take care of undo/redo
            m_pAnimSequence->AddNode(pNewNode->m_pAnimNode.get());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNode* CTrackViewAnimNode::FindNodeByAnimNode(const IAnimNode* animNode)
{
    // Depth-first search for TrackViewAnimNode associated with the given animNode. Returns the first match found.
    CTrackViewAnimNode* retNode = nullptr;

    for (const std::unique_ptr<CTrackViewNode>& childNode : m_childNodes)
    {
        if (childNode->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* childAnimNode = static_cast<CTrackViewAnimNode*>(childNode.get());

            // recurse to search children of group nodes
            if (childNode->IsGroupNode())
            {
                retNode = childAnimNode->FindNodeByAnimNode(animNode);
                if (retNode)
                {
                    break;
                }
            }

            if (childAnimNode->GetAnimNode() == animNode)
            {
                retNode = childAnimNode;
                break;
            }
        }
    }
    return retNode;
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsValidReparentingTo(CTrackViewAnimNode* pNewParent)
{
    if (pNewParent == GetParentNode() || !pNewParent->IsGroupNode() || pNewParent->GetType() == eAnimNodeType_AzEntity)
    {
        return false;
    }

    // Check if the new parent already contains a node with this name
    CTrackViewAnimNodeBundle foundNodes = pNewParent->GetAnimNodesByName(GetName());
    if (foundNodes.GetCount() > 1 || (foundNodes.GetCount() == 1 && foundNodes.GetNode(0) != this))
    {
        return false;
    }

    // Check if another node already owns this entity in the new parent's tree
    CEntityObject* pOwner = GetNodeEntity();
    if (pOwner)
    {
        CTrackViewAnimNodeBundle ownedNodes = pNewParent->GetAllOwnedNodes(pOwner);
        if (ownedNodes.GetCount() > 0 && ownedNodes.GetNode(0) != this)
        {
            return false;
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetNewParent(CTrackViewAnimNode* pNewParent)
{
    if (pNewParent == GetParentNode())
    {
        return;
    }

    assert(CUndo::IsRecording());
    assert(IsValidReparentingTo(pNewParent));

    CUndo::Record(new CUndoAnimNodeReparent(this, pNewParent));
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetDisabled(bool bDisabled)
{
    if (m_pAnimNode)
    {
        if (bDisabled)
        {
            m_pAnimNode->SetFlags(m_pAnimNode->GetFlags() | eAnimNodeFlags_Disabled);
            GetSequence()->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Disabled);
        }
        else
        {
            m_pAnimNode->SetFlags(m_pAnimNode->GetFlags() & ~eAnimNodeFlags_Disabled);
            GetSequence()->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Enabled);
        }
    }
    MarkAsModified();
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsDisabled() const
{
    return m_pAnimNode ? m_pAnimNode->GetFlags() & eAnimNodeFlags_Disabled : false;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetPos(const Vec3& position)
{
    const float time = GetSequence()->GetTime();
    CTrackViewTrack* pTrack = GetTrackForParameter(eAnimParamType_Position);
    CRenderViewport* pRenderViewport = static_cast<CRenderViewport*>(GetIEditor()->GetViewManager()->GetGameViewport());

    if (pTrack)
    {
        if (!GetIEditor()->GetAnimation()->IsRecording())
        {
            // Offset all keys by move amount.
            Vec3 posPrev;
            pTrack->GetValue(time, posPrev);
            Vec3 offset = position - posPrev;
            pTrack->OffsetKeyPosition(offset);

            GetSequence()->OnKeysChanged();
        }
        else if (m_pNodeEntity->IsSelected() || pRenderViewport->GetCameraObject() == m_pNodeEntity)
        {
            CUndo::Record(new CUndoTrackObject(pTrack, GetSequence()));
            const int flags = m_pAnimNode->GetFlags();
            // Set the selected flag to enable record when unselected camera is moved through viewport
            m_pAnimNode->SetFlags(flags | eAnimNodeFlags_EntitySelected);
            m_pAnimNode->SetPos(GetSequence()->GetTime(), position);
            m_pAnimNode->SetFlags(flags);
            GetSequence()->OnKeysChanged();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetScale(const Vec3& scale)
{
    CTrackViewTrack* pTrack = GetTrackForParameter(eAnimParamType_Scale);

    if (GetIEditor()->GetAnimation()->IsRecording() && m_pNodeEntity->IsSelected() && pTrack)
    {
        CUndo::Record(new CUndoTrackObject(pTrack, GetSequence()));
        m_pAnimNode->SetScale(GetSequence()->GetTime(), scale);
        GetSequence()->OnKeysChanged();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetRotation(const Quat& rotation)
{
    CTrackViewTrack* pTrack = GetTrackForParameter(eAnimParamType_Rotation);
    CRenderViewport* pRenderViewport = static_cast<CRenderViewport*>(GetIEditor()->GetViewManager()->GetGameViewport());

    if (GetIEditor()->GetAnimation()->IsRecording() && (m_pNodeEntity->IsSelected() || pRenderViewport->GetCameraObject() == m_pNodeEntity) && pTrack)
    {
        CUndo::Record(new CUndoTrackObject(pTrack, GetSequence()));
        const int flags = m_pAnimNode->GetFlags();
        // Set the selected flag to enable record when unselected camera is moved through viewport
        m_pAnimNode->SetFlags(flags | eAnimNodeFlags_EntitySelected);
        m_pAnimNode->SetRotate(GetSequence()->GetTime(), rotation);
        m_pAnimNode->SetFlags(flags);
        GetSequence()->OnKeysChanged();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsActive()
{
    CTrackViewSequence* pSequence = GetSequence();
    const bool bInActiveSequence = pSequence ? GetSequence()->IsBoundToEditorObjects() : false;

    CTrackViewAnimNode* pDirector = GetDirector();
    const bool bMemberOfActiveDirector = pDirector ? GetDirector()->IsActiveDirector() : true;

    return bInActiveSequence && bMemberOfActiveDirector;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::OnSelectionChanged(const bool bSelected)
{
    if (m_pAnimNode)
    {
        const EAnimNodeType animNodeType = GetType();
        assert(animNodeType == eAnimNodeType_Camera || animNodeType == eAnimNodeType_Entity || animNodeType == eAnimNodeType_GeomCache || animNodeType == eAnimNodeType_AzEntity);

        const EAnimNodeFlags flags = (EAnimNodeFlags)m_pAnimNode->GetFlags();
        m_pAnimNode->SetFlags(bSelected ? (flags | eAnimNodeFlags_EntitySelected) : (flags & ~eAnimNodeFlags_EntitySelected));
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetPosRotScaleTracksDefaultValues()
{
    const CEntityObject* entity = nullptr;
    bool entityIsBoundToEditorObjects = false;

    if (m_pAnimNode)
    {
        if (m_pAnimNode->GetType() == eAnimNodeType_Component)
        {
            // get entity from the parent Component Entity
           CTrackViewNode* parentNode = GetParentNode();
           if (parentNode && parentNode->GetNodeType() == eTVNT_AnimNode)
           {
               CTrackViewAnimNode* parentAnimNode = static_cast<CTrackViewAnimNode*>(parentNode);
               if (parentAnimNode)
               {
                   entity = parentAnimNode->GetNodeEntity(false);
                   entityIsBoundToEditorObjects = parentAnimNode->IsBoundToEditorObjects();
               }
           }
        }
        else
        {
            // not a component - get the entity on this node directly
            entity = GetNodeEntity(false);
            entityIsBoundToEditorObjects = IsBoundToEditorObjects();
        }

        if (entity && entityIsBoundToEditorObjects)
        {
            const float time = GetSequence()->GetTime();
            m_pAnimNode->SetPos(time, entity->GetPos());
            m_pAnimNode->SetRotate(time, entity->GetRotation());
            m_pAnimNode->SetScale(time, entity->GetScale());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::UpdateTrackGizmo()
{
    if (IsActive() && m_pNodeEntity && !m_pNodeEntity->IsHidden())
    {
        if (!m_trackGizmo)
        {
            CTrackGizmo* pTrackGizmo = new CTrackGizmo;
            pTrackGizmo->SetAnimNode(this);
            m_trackGizmo = pTrackGizmo;
            GetIEditor()->GetObjectManager()->GetGizmoManager()->AddGizmo(m_trackGizmo);
        }
    }
    else
    {
        GetIEditor()->GetObjectManager()->GetGizmoManager()->RemoveGizmo(m_trackGizmo);
        m_trackGizmo = nullptr;
    }

    if (m_pNodeEntity && m_trackGizmo)
    {
        const Matrix34 gizmoMatrix = m_pNodeEntity->GetParentAttachPointWorldTM();
        m_trackGizmo->SetMatrix(gizmoMatrix);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::CheckTrackAnimated(const CAnimParamType& paramType) const
{
    if (!m_pAnimNode)
    {
        return false;
    }

    CTrackViewTrack* pTrack = GetTrackForParameter(paramType);
    return pTrack && pTrack->GetKeyCount() > 0;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::OnNodeAnimated(IAnimNode* pNode)
{
    if (m_pNodeEntity)
    {
        m_pNodeEntity->InvalidateTM(0);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::OnNodeVisibilityChanged(IAnimNode* pNode, const bool bHidden)
{
    if (m_pNodeEntity)
    {
        m_pNodeEntity->SetHidden(bHidden);

        // Need to do this to force recreation of gizmos
        bool bUnhideSelected = !m_pNodeEntity->IsHidden() && m_pNodeEntity->IsSelected();
        if (bUnhideSelected)
        {
            GetIEditor()->GetObjectManager()->UnselectObject(m_pNodeEntity);
            GetIEditor()->GetObjectManager()->SelectObject(m_pNodeEntity);
        }

        UpdateTrackGizmo();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::OnNodeReset(IAnimNode* pNode)
{
    if (gEnv->IsEditing() && m_pNodeEntity)
    {
        // If the node has an event track, one should also reload the script when the node is reset.
        CTrackViewTrack* pAnimTrack = GetTrackForParameter(eAnimParamType_Event);
        if (pAnimTrack && pAnimTrack->GetKeyCount())
        {
            CEntityScript* script = m_pNodeEntity->GetScript();
            script->Reload();
            m_pNodeEntity->Reload(true);
        }
    }
}

void CTrackViewAnimNode::SetComponent(AZ::ComponentId componentId, const AZ::Uuid& componentTypeId)
{
    if (m_pAnimNode)
    {
        m_pAnimNode->SetComponent(componentId, componentTypeId);
    }
}

//////////////////////////////////////////////////////////////////////////
AZ::ComponentId CTrackViewAnimNode::GetComponentId() const
{
    return m_pAnimNode ? m_pAnimNode->GetComponentId() : AZ::InvalidComponentId;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::MarkAsModified()
{
    GetSequence()->MarkAsModified();
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::ContainsComponentWithId(AZ::ComponentId componentId) const
{
    bool retFound = false;

    if (GetType() == eAnimNodeType_AzEntity)
    {
        // search for a matching componentId on all children
        for (int i = 0; i < GetChildCount(); i++)
        {
            CTrackViewNode* childNode = GetChild(i);
            if (childNode->GetNodeType() == eTVNT_AnimNode)
            {
                if (static_cast<CTrackViewAnimNode*>(childNode)->GetComponentId() == componentId)
                {
                    retFound = true;
                    break;
                }
            }
        }
    }

    return retFound;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::OnStartPlayInEditor()
{
    if (m_pAnimSequence && m_pAnimSequence->GetSequenceType() == eSequenceType_SequenceComponent &&
        m_pAnimSequence->GetOwnerId().IsValid())
    {
        AZ::EntityId remappedId;
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequestBus::Events::MapEditorIdToRuntimeId, m_pAnimSequence->GetOwnerId(), remappedId);
            
        if (remappedId.IsValid())
        {
            // stash and remap the AZ::EntityId of the SequenceComponent entity to restore it when we switch back to Edit mode
            m_stashedAnimSequenceEditorAzEntityId = m_pAnimSequence->GetOwnerId();
            m_pAnimSequence->SetOwner(remappedId);
        }
    }

    if (m_pAnimNode && m_pAnimNode->GetAzEntityId().IsValid())
    {
        AZ::EntityId remappedId;
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequestBus::Events::MapEditorIdToRuntimeId, m_pAnimNode->GetAzEntityId(), remappedId);

        if (remappedId.IsValid())
        {
            // stash the AZ::EntityId of the SequenceComponent entity to restore it when we switch back to Edit mode
            m_stashedAnimNodeEditorAzEntityId = m_pAnimNode->GetAzEntityId();
            m_pAnimNode->SetAzEntityId(remappedId);
        }
    }
    
    if (m_pAnimNode)
    {
        m_pAnimNode->OnStartPlayInEditor();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::OnStopPlayInEditor()
{
    // restore sequenceComponent entity Ids back to their original Editor Ids
    if (m_pAnimSequence && m_stashedAnimSequenceEditorAzEntityId.IsValid())
    {
        m_pAnimSequence->SetOwner(m_stashedAnimSequenceEditorAzEntityId);

        // invalidate the stashed Id now that we've restored it
        m_stashedAnimSequenceEditorAzEntityId.SetInvalid();
    }

    if (m_pAnimNode && m_stashedAnimNodeEditorAzEntityId.IsValid())
    {
        m_pAnimNode->SetAzEntityId(m_stashedAnimNodeEditorAzEntityId);

        // invalidate the stashed Id now that we've restored it
        m_stashedAnimNodeEditorAzEntityId.SetInvalid();
    }

    if (m_pAnimNode)
    {
        m_pAnimNode->OnStopPlayInEditor();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::OnEntityActivated(const AZ::EntityId& activatedEntityId)
{
    if (GetAzEntityId() != activatedEntityId)
    {
        // This can happen when we're exiting Game/Sim Mode and entity Id's are remapped. Do nothing in such a circumstance.
        return;
    }

    CTrackViewDialog* dialog = CTrackViewDialog::GetCurrentInstance();
    if ((dialog && dialog->IsDoingUndoOperation()) || GetAzEntityId() != activatedEntityId)
    {
        // Do not respond during Undo. We'll get called when we connect to the AZ::EntityBus in SetEntityNode(),
        // which would result in adding component nodes twice during Undo.

        // Also do not respond to entity activation notifications for entities not associated with this animNode,
        // although this should never happen
        return;
    }

    // Ensure the components on the Entity match the components on the Entity Node in Track View.
    //
    // Note this gets called as soon as we connect to AZ::EntityBus - so in effect SetNodeEntity() on an AZ::Entity results
    // in all of it's component nodes being added.
    //
    // If the component exists in Track View but not in the entity, we remove it from Track View.

    AZ::Entity* entity = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, activatedEntityId);

    // check if all Track View components are (still) on the entity. If not, remove it from TrackView
    for (int i = GetChildCount(); --i >= 0;)
    {
        if (GetChild(i)->GetNodeType() == eTVNT_AnimNode)
        {
            CTrackViewAnimNode* childAnimNode = static_cast<CTrackViewAnimNode*>(GetChild(i));
            if (childAnimNode->GetComponentId() != AZ::InvalidComponentId && !(entity->FindComponent(childAnimNode->GetComponentId())))
            {
                CUndo undo("Remove Track View Component Node");

                RemoveSubNode(childAnimNode);
            }
        }
    }

    /////////////////////////////////////////////////////////////////////////
    // check that all animatable components on the Entity are in Track View

    AZStd::vector<AZ::ComponentId> animatableComponentIds;

    // Get all components animated through the behavior context
    Maestro::EditorSequenceComponentRequestBus::Event(GetSequence()->GetSequenceComponentEntityId(), &Maestro::EditorSequenceComponentRequestBus::Events::GetAnimatableComponents,
                                                          animatableComponentIds, activatedEntityId);

    // Append all components animated outside the behavior context
    AppendNonBehaviorAnimatableComponents(animatableComponentIds);

    for (const AZ::ComponentId& componentId : animatableComponentIds)
    {
        bool componentFound = false;
        for (int i = GetChildCount(); --i >= 0;)
        {
            if (GetChild(i)->GetNodeType() == eTVNT_AnimNode)
            {
                CTrackViewAnimNode* childAnimNode = static_cast<CTrackViewAnimNode*>(GetChild(i));
                if (childAnimNode->GetComponentId() == componentId)
                {
                    componentFound = true;
                    break;
                }
            }
        }
        if (!componentFound)
        {
            AddComponent(entity->FindComponent(componentId));
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::OnEntityRemoved()
{
    // This is called by CTrackViewSequenceManager for both legacy and AZ Entities. When we deprecate legacy entities,
    // we could (should) probably handles this via ComponentApplicationEventBus::Events::OnEntityRemoved

    m_pNodeEntity = nullptr;    // invalidate cached node entity pointer

    if (GetAzEntityId().IsValid())
    {
        AZ::EntityBus::Handler::BusDisconnect(m_pAnimNode->GetAzEntityId());
    }

    // notify the change. This leads to Track View updating it's UI to account for the entity removal
    GetSequence()->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_NodeOwnerChanged);
}


//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNode* CTrackViewAnimNode::AddComponent(const AZ::Component* component)
{
    CTrackViewAnimNode* retNewComponentNode = nullptr;
    AZStd::string componentName;
    AZ::Uuid      componentTypeId(AZ::Uuid::CreateNull());

    AzFramework::ApplicationRequests::Bus::BroadcastResult(componentTypeId, &AzFramework::ApplicationRequests::Bus::Events::GetComponentTypeId, GetAzEntityId(), component->GetId());

    AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(componentName, &AzToolsFramework::EntityCompositionRequests::GetComponentName, component);

    if (!componentName.empty() && !componentTypeId.IsNull())
    {
        CUndo undo("Add TrackView Component");
        retNewComponentNode = CreateSubNode(componentName.c_str(), eAnimNodeType_Component, nullptr, componentTypeId, component->GetId());
    }
    else
    {
        AZ_Warning("TrackView", false, "Could not determine component name or type for adding component - skipping...");
    }

    return retNewComponentNode;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::MatrixInvalidated()
{
    UpdateTrackGizmo();
}

//////////////////////////////////////////////////////////////////////////
Vec3 CTrackViewAnimNode::GetTransformDelegatePos(const Vec3& basePos) const
{
    const Vec3 position = GetPos();

    return Vec3(CheckTrackAnimated(eAnimParamType_PositionX) ? position.x : basePos.x,
        CheckTrackAnimated(eAnimParamType_PositionY) ? position.y : basePos.y,
        CheckTrackAnimated(eAnimParamType_PositionZ) ? position.z : basePos.z);
}

//////////////////////////////////////////////////////////////////////////
Quat CTrackViewAnimNode::GetTransformDelegateRotation(const Quat& baseRotation) const
{
    if (!CheckTrackAnimated(eAnimParamType_Rotation))
    {
        return baseRotation;
    }

    const Ang3 angBaseRotation(baseRotation);
    const Ang3 angNodeRotation(GetRotation());

    return Quat(Ang3(CheckTrackAnimated(eAnimParamType_RotationX) ? angNodeRotation.x : angBaseRotation.x,
            CheckTrackAnimated(eAnimParamType_RotationY) ? angNodeRotation.y : angBaseRotation.y,
            CheckTrackAnimated(eAnimParamType_RotationZ) ? angNodeRotation.z : angBaseRotation.z));
}

//////////////////////////////////////////////////////////////////////////
Vec3 CTrackViewAnimNode::GetTransformDelegateScale(const Vec3& baseScale) const
{
    const Vec3 scale = GetScale();

    return Vec3(CheckTrackAnimated(eAnimParamType_ScaleX) ? scale.x : baseScale.x,
        CheckTrackAnimated(eAnimParamType_ScaleY) ? scale.y : baseScale.y,
        CheckTrackAnimated(eAnimParamType_ScaleZ) ? scale.z : baseScale.z);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetTransformDelegatePos(const Vec3& position)
{
    SetPos(position);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetTransformDelegateRotation(const Quat& rotation)
{
    SetRotation(rotation);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetTransformDelegateScale(const Vec3& scale)
{
    SetScale(scale);
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsPositionDelegated() const
{
    const bool bDelegated = (GetIEditor()->GetAnimation()->IsRecording() && m_pNodeEntity->IsSelected() && GetTrackForParameter(eAnimParamType_Position)) || CheckTrackAnimated(eAnimParamType_Position);
    return bDelegated;
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsRotationDelegated() const
{
    const bool bDelegated = (GetIEditor()->GetAnimation()->IsRecording() && m_pNodeEntity->IsSelected() && GetTrackForParameter(eAnimParamType_Rotation)) || CheckTrackAnimated(eAnimParamType_Rotation);
    return bDelegated;
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsScaleDelegated() const
{
    const bool bDelegated = (GetIEditor()->GetAnimation()->IsRecording() && m_pNodeEntity->IsSelected() && GetTrackForParameter(eAnimParamType_Scale)) || CheckTrackAnimated(eAnimParamType_Scale);
    return bDelegated;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::OnDone()
{
    SetNodeEntity(nullptr);
    UpdateTrackGizmo();
}

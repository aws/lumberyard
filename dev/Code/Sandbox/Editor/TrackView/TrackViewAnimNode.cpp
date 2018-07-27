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
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionBus.h>
#include <AzFramework/API/ApplicationAPI.h>

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
#include <Maestro/Types/AnimNodeType.h>
#include <Maestro/Types/AnimValueType.h>
#include <Maestro/Types/AnimParamType.h>
#include <Maestro/Types/SequenceType.h>

// static class data
const AZ::Uuid CTrackViewAnimNode::s_nullUuid = AZ::Uuid::CreateNull();

//////////////////////////////////////////////////////////////////////////
static void CreateDefaultTracksForEntityNode(CTrackViewAnimNode* pNode, const DynArray<unsigned int>& trackCount)
{
    if (pNode->GetType() == AnimNodeType::Entity)
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
    else if (pNode->GetType() == AnimNodeType::AzEntity)
    {
        bool haveTrackToAdd = false;

        // check that the trackCount array passed in has at least one non-zero element in it for Pos/Rot/Scale, meaning we have at least one track to add
        IMovieSystem* pMovieSystem = GetIEditor()->GetMovieSystem();
        assert(trackCount.size() == pMovieSystem->GetEntityNodeParamCount());
        for (int i = 0; i < pMovieSystem->GetEntityNodeParamCount(); ++i)
        {
            CAnimParamType paramType = pMovieSystem->GetEntityNodeParamType(i);
            if ((paramType == AnimParamType::Position || paramType == AnimParamType::Rotation || paramType == AnimParamType::Scale) && trackCount[i])
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
                                if (paramType.GetType() == AnimParamType::Position)
                                {
                                    transformPropertyParamType = "Position";
                                    createTransformTrack = true;
                                }
                                else if (paramType.GetType() == AnimParamType::Rotation)
                                {
                                    transformPropertyParamType = "Rotation";
                                    createTransformTrack = true;
                                }
                                else if (paramType.GetType() == AnimParamType::Scale)
                                {
                                    transformPropertyParamType = "Scale";
                                    createTransformTrack = true;
                                }

                                if (createTransformTrack)
                                {
                                    transformPropertyParamType = paramType.GetType();              // this sets the type to one of AnimParamType::Position, AnimParamType::Rotation or AnimParamType::Scale  but maintains the name
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

    switch (GetType())
    {
    case AnimNodeType::Comment:
        m_pNodeAnimator.reset(new CCommentNodeAnimator(this));
        break;
    case AnimNodeType::Layer:
        m_pNodeAnimator.reset(new CLayerNodeAnimator());
        break;
    case AnimNodeType::Director:
        m_pNodeAnimator.reset(new CDirectorNodeAnimator(this));
        break;
    }

    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();

    if (IsBoundToAzEntity())
    {
        AZ::TransformNotificationBus::Handler::BusConnect(GetAzEntityId());
    }
}

//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNode::~CTrackViewAnimNode()
{
    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();

    if (IsBoundToAzEntity())
    {
        AZ::EntityId entityId = GetAzEntityId();
        AZ::TransformNotificationBus::Handler::BusDisconnect(entityId);
        AZ::EntityBus::Handler::BusDisconnect(entityId);
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
        if (m_pAnimNode->GetType() == AnimNodeType::AzEntity)
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
    case AnimNodeType::Camera:
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
CTrackViewAnimNode* CTrackViewAnimNode::CreateSubNode(const QString& name, const AnimNodeType animNodeType, CEntityObject* pOwner , AZ::Uuid componentTypeId, AZ::ComponentId componentId)
{
    assert(CUndo::IsRecording());

    const bool bIsGroupNode = IsGroupNode();
    assert(bIsGroupNode);
    if (!bIsGroupNode)
    {
        return nullptr;
    }

    CTrackViewAnimNode* pDirector = nullptr;
    const auto nameStr = name.toUtf8();

    // Check if the node's director or sequence already contains a node with this name, unless it's a component, for which we allow duplicate names since
    // Components are children of unique AZEntities in Track View
    if (animNodeType != AnimNodeType::Component)
    {
        pDirector = (GetType() == AnimNodeType::Director) ? this : GetDirector();
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

    // If this is an AzEntity, make sure there is an associated entity id
    if (pOwner != nullptr && animNodeType == AnimNodeType::AzEntity)
    {
        AZ::EntityId id(AZ::EntityId::InvalidEntityId);
        AzToolsFramework::ComponentEntityObjectRequestBus::EventResult(id, pOwner, &AzToolsFramework::ComponentEntityObjectRequestBus::Events::GetAssociatedEntityId);

        if (!id.IsValid())
        {
            GetIEditor()->GetMovieSystem()->LogUserNotificationMsg(AZStd::string::format("Failed to add '%s' to sequence '%s', could not find associated entity. Please try adding the entity associated with '%s'.", nameStr.constData(), pDirector->GetName(), nameStr.constData()));
            return nullptr;
        }
    }

    pNewAnimNode->SetName(nameStr.constData());
    pNewAnimNode->CreateDefaultTracks();
    pNewAnimNode->SetParent(m_pAnimNode.get());
    pNewAnimNode->SetComponent(componentId, componentTypeId);

    CTrackViewAnimNodeFactory animNodeFactory;
    CTrackViewAnimNode* pNewNode = animNodeFactory.BuildAnimNode(m_pAnimSequence, pNewAnimNode, this);

    // Make sure that camera and entity nodes get created with an owner
    assert((animNodeType != AnimNodeType::Camera && animNodeType != AnimNodeType::Entity) || pOwner);

    pNewNode->SetNodeEntity(pOwner);
    pNewAnimNode->SetNodeOwner(pNewNode);

    pNewNode->BindToEditorObjects();

    AddNode(pNewNode);

    // Legacy sequence use Track View Undo, New Component based Sequence
    // use the AZ Undo system and allow sequence entity to be the source of truth.
    if (m_pAnimSequence->GetSequenceType() == SequenceType::Legacy)
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

    // remove anim node children
    for (int i = pSubNode->GetChildCount(); --i >= 0;)
    {
        CTrackViewAnimNode* childAnimNode = static_cast<CTrackViewAnimNode*>(pSubNode->GetChild(i));
        if (childAnimNode->GetNodeType() == eTVNT_AnimNode)
        {
            RemoveSubNode(childAnimNode);
        }
    }

    // Legacy sequence use Track View Undo, New Component based Sequence
    // use the AZ Undo system and allow sequence entity to be the source of truth.
    if (m_pAnimSequence->GetSequenceType() == SequenceType::Legacy)
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
    if (m_pAnimSequence->GetSequenceType() == SequenceType::Legacy)
    {
        CUndo::Record(new CUndoTrackAdd(pNewTrack));
    }

    MarkAsModified();

    SetPosRotScaleTracksDefaultValues();

    return pNewTrack;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::RemoveTrack(CTrackViewTrack* track)
{
    assert(CUndo::IsRecording());
    assert(!track->IsSubTrack());

    if (!track->IsSubTrack())
    {
        CTrackViewSequence* sequence = track->GetSequence();
        if (nullptr != sequence)
        {
            if (sequence->GetSequenceType() == SequenceType::Legacy)
            {
                CUndo::Record(new CUndoTrackRemove(track));
            }
            else
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Remove Track");
                CTrackViewAnimNode* parentNode = track->GetAnimNode();
                std::unique_ptr<CTrackViewNode> foundTrack;

                if (nullptr != parentNode)
                {
                    for (auto iter = parentNode->m_childNodes.begin(); iter != parentNode->m_childNodes.end(); ++iter)
                    {
                        std::unique_ptr<CTrackViewNode>& currentNode = *iter;
                        if (currentNode.get() == track)
                        {
                            // Hang onto a reference until after OnNodeChanged is called.
                            currentNode.swap(foundTrack);

                            // Remove from parent node and sequence
                            parentNode->m_childNodes.erase(iter);
                            parentNode->m_pAnimNode->RemoveTrack(track->GetAnimTrack());
                            break;
                        }
                    }

                    m_pParentNode->GetSequence()->OnNodeChanged(track, ITrackViewSequenceListener::eNodeChangeType_Removed);

                    // Release the track now that OnNodeChanged is complete.
                    if (nullptr != foundTrack.get())
                    {
                        foundTrack.release();
                    }

                }
                undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }
        }
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
void CTrackViewAnimNode::SetExpanded(bool expanded)
{
    if (GetExpanded() != expanded)
    {
        CTrackViewSequence* sequence = GetSequence();
        AZ_Assert(nullptr != sequence, "Every node should have a sequence.");
        if (nullptr != sequence)
        {
            AZ_Assert(m_pAnimNode, "Expected m_pAnimNode to be valid.");
            if (m_pAnimNode)
            {
                m_pAnimNode->SetExpanded(expanded);
            }

            if (expanded)
            {
                sequence->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Expanded);
            }
            else
            {
                sequence->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Collapsed);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::GetExpanded() const
{
    bool result = true;

    AZ_Assert(m_pAnimNode, "Expected m_pAnimNode to be valid.");
    if (m_pAnimNode)
    {
        result = m_pAnimNode->GetExpanded();
    }

    return result;
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

            if (paramType != AnimParamType::Invalid && pTrack->GetParameterType() != paramType)
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
AnimNodeType CTrackViewAnimNode::GetType() const
{
    return m_pAnimNode ? m_pAnimNode->GetType() : AnimNodeType::Invalid;
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
    if (GetType() == AnimNodeType::Director)
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

    if (GetType() == AnimNodeType::AzEntity)
    {
        // For AzEntity, search for track on all child components - returns first track match found (note components searched in reverse)
        for (int i = GetChildCount(); --i >= 0;)
        {
            if (GetChild(i)->GetNodeType() == eTVNT_AnimNode)
            {
                CTrackViewAnimNode* componentNode = static_cast<CTrackViewAnimNode*>(GetChild(i));
                if (componentNode->GetType() == AnimNodeType::Component)
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

    CTrackViewSequence* sequence = GetSequence();
    AZ_Assert(sequence, "Nodes should never have a null sequence.");

    if (CUndo::IsRecording() && sequence->GetSequenceType() == SequenceType::Legacy)
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

        if (m_pAnimNode->GetType() == AnimNodeType::AzEntity)
        {
            // We're connecting to a new AZ::Entity
            AZ::EntityId    sequenceComponentEntityId(m_pAnimSequence->GetSequenceEntityId());

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
                    AZ::TransformNotificationBus::Handler::BusDisconnect(m_pAnimNode->GetAzEntityId());
                }

                m_pAnimNode->SetAzEntityId(id);
            }

            // connect to EntityBus for OnEntityActivated() notifications to sync components on the entity
            if (!AZ::EntityBus::Handler::BusIsConnectedId(m_pAnimNode->GetAzEntityId()))
            {
                AZ::EntityBus::Handler::BusConnect(m_pAnimNode->GetAzEntityId());
            }

            if (!AZ::TransformNotificationBus::Handler::BusIsConnectedId(m_pAnimNode->GetAzEntityId()))
            {
                AZ::TransformNotificationBus::Handler::BusConnect(m_pAnimNode->GetAzEntityId());
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
CTrackViewAnimNodeBundle CTrackViewAnimNode::GetAnimNodesByType(AnimNodeType animNodeType)
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
    AnimNodeType nodeType = GetType();

    // AZEntities are really just containers for components, so considered a 'Group' node
    return nodeType == AnimNodeType::Director || nodeType == AnimNodeType::Group || nodeType == AnimNodeType::AzEntity;
}

//////////////////////////////////////////////////////////////////////////
QString CTrackViewAnimNode::GetAvailableNodeNameStartingWith(const QString& name) const
{
    QString newName = name;
    unsigned int index = 2;

    while (const_cast<CTrackViewAnimNode*>(this)->GetAnimNodesByName(newName.toUtf8().data()).GetCount() > 0)
    {
        newName = QStringLiteral("%1%2").arg(name).arg(index);
        ++index;
    }

    return newName;
}

AnimNodeType CTrackViewAnimNode::GetAnimNodeTypeFromObject(const CBaseObject* object) const
{
    AnimNodeType retType = AnimNodeType::Invalid;

    if (qobject_cast<const CCameraObject*>(object))
    {
        retType = AnimNodeType::Camera;
    }
#if defined(USE_GEOM_CACHES)
    else if (qobject_cast<const CGeomCacheEntity*>(object))
    {
        retType = AnimNodeType::GeomCache;
    }
#endif
    else if (qobject_cast<const CEntityObject*>(object))
    {
        const AZ::EntityId entityId(qobject_cast<const CEntityObject*>(object)->GetEntityId());
        if (IsLegacyEntityId(entityId))
        {
            retType = AnimNodeType::Entity;
        }
        else
        {
            retType = AnimNodeType::AzEntity;
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
                GetIEditor()->GetMovieSystem()->LogUserNotificationMsg(AZStd::string::format("'%s' was already added to '%s', skipping...", pObject->GetName().toUtf8().data(), GetDirector()->GetName()));
                continue;
            }
        }

        AnimNodeType nodeType = GetAnimNodeTypeFromObject(pObject);
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

    CreateSubNode(name, AnimNodeType::Entity);
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
    assert (GetType() == AnimNodeType::Camera);

    if (GetType() == AnimNodeType::Camera)
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
        return AnimParamType::Invalid;
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
AnimValueType CTrackViewAnimNode::GetParamValueType(const CAnimParamType& paramType) const
{
    if (m_pAnimNode)
    {
        return m_pAnimNode->GetParamValueType(paramType);
    }

    return AnimValueType::Unknown;
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
    childNode->setAttr("type", static_cast<int>(GetType()));

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
        if (m_pAnimSequence->GetSequenceType() == SequenceType::Legacy)
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
        if (!xmlNode->getAttr("Type", type) || (bLightAnimationSetActive && (AnimNodeType)type != AnimNodeType::Light))
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

    AnimNodeType nodeType;
    GetIEditor()->GetMovieSystem()->SerializeNodeType(nodeType, xmlNode, /*bLoading=*/ true, IAnimSequence::kSequenceVersion, m_pAnimSequence->GetFlags());
    
    if (nodeType == AnimNodeType::Component)
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
        if (pDirector->GetAnimNodesByName(name.toUtf8().data()).GetCount() > 0)
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

        parentNode->AddNode(pNewNode);

        // Legacy sequence use Track View Undo, New Component based Sequence
        // use the AZ Undo system and allow sequence entity to be the source of truth.
        if (m_pAnimSequence->GetSequenceType() == SequenceType::Legacy)
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
    if (pNewParent == GetParentNode() || !pNewParent->IsGroupNode() || pNewParent->GetType() == AnimNodeType::AzEntity)
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

void CTrackViewAnimNode::SetParentsInChildren(CTrackViewAnimNode* currentNode)
{
    const uint numChildren = currentNode->GetChildCount();

    for (uint childIndex = 0; childIndex < numChildren; ++childIndex)
    {
        CTrackViewAnimNode* childAnimNode = static_cast<CTrackViewAnimNode*>(currentNode->GetChild(childIndex));

        if (childAnimNode->GetNodeType() != eTVNT_Track)
        {
            childAnimNode->m_pAnimNode->SetParent(currentNode->m_pAnimNode.get());

            if (childAnimNode->GetChildCount() > 0 && childAnimNode->GetNodeType() != eTVNT_AnimNode)
            {
                SetParentsInChildren(childAnimNode);
            }
        }
    }
}
//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetNewParent(CTrackViewAnimNode* newParent)
{
    if (newParent == GetParentNode())
    {
        return;
    }

    assert(IsValidReparentingTo(newParent));

    CTrackViewSequence* sequence = newParent->GetSequence();
    if (sequence->GetSequenceType() == SequenceType::Legacy)
    {
        assert(CUndo::IsRecording());
        CUndo::Record(new CUndoAnimNodeReparent(this, newParent));
    }
    else
    {
        AzToolsFramework::ScopedUndoBatch undoBatch("Set New Track View Anim Node Parent");

        UnBindFromEditorObjects();

        // Remove from the old parent's children and hang on to a ref.
        std::unique_ptr<CTrackViewNode> storedTrackViewNode;
        CTrackViewAnimNode* lastParent = static_cast<CTrackViewAnimNode*>(m_pParentNode);
        if (nullptr != lastParent)
        {
            for (auto iter = lastParent->m_childNodes.begin(); iter != lastParent->m_childNodes.end(); ++iter)
            {
                std::unique_ptr<CTrackViewNode>& currentNode = *iter;

                if (currentNode.get() == this)
                {
                    currentNode.swap(storedTrackViewNode);
                    lastParent->m_childNodes.erase(iter);
                    break;
                }
            }
        }
        AZ_Assert(nullptr != storedTrackViewNode.get(), "Existing Parent of node not found");

        sequence->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Removed);

        // Set new parent
        m_pParentNode = newParent;
        m_pAnimNode->SetParent(newParent->m_pAnimNode.get());
        SetParentsInChildren(this);

        // Add node to the new parent's children.
        static_cast<CTrackViewAnimNode*>(m_pParentNode)->AddNode(storedTrackViewNode.release());

        BindToEditorObjects();

        undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
    }
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
    CTrackViewTrack* track = GetTrackForParameter(AnimParamType::Position);
    CRenderViewport* renderViewport = static_cast<CRenderViewport*>(GetIEditor()->GetViewManager()->GetGameViewport());

    if (track)
    {
        if (!GetIEditor()->GetAnimation()->IsRecording())
        {
            // Offset all keys by move amount.
            Vec3 offset = m_pAnimNode->GetOffsetPosition(position);
            
            track->OffsetKeyPosition(offset);

            GetSequence()->OnKeysChanged();
        }
        else if (m_pNodeEntity->IsSelected() || renderViewport->GetCameraObject() == m_pNodeEntity)
        {
            CTrackViewSequence* sequence = GetSequence();

            AZ_Assert(sequence, "Expected valid sequence");
            if (sequence != nullptr)
            {
                const int flags = m_pAnimNode->GetFlags();
                if (sequence->GetSequenceType() == SequenceType::Legacy)
                {
                    CUndo::Record(new CUndoTrackObject(track, sequence));
                    // Set the selected flag to enable record when unselected camera is moved through viewport
                    m_pAnimNode->SetFlags(flags | eAnimNodeFlags_EntitySelected);
                    m_pAnimNode->SetPos(sequence->GetTime(), position);
                    m_pAnimNode->SetFlags(flags);
                }
                else
                {
                    // This is required because the entity movement system uses Undo to
                    // undo a previous move delta as the entity is dragged.
                    CUndo::Record(new CUndoComponentEntityTrackObject(track));

                    // Set the selected flag to enable record when unselected camera is moved through viewport
                    m_pAnimNode->SetFlags(flags | eAnimNodeFlags_EntitySelected);
                    m_pAnimNode->SetPos(sequence->GetTime(), position);
                    m_pAnimNode->SetFlags(flags);
                    
                    // We don't want to use ScopedUndoBatch here because we don't want a separate Undo operation
                    // generate for every frame as the user moves an entity.
                    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                        &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity, 
                        sequence->GetSequenceComponentEntityId()
                    );                    
                }

                sequence->OnKeysChanged();
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetScale(const Vec3& scale)
{
    CTrackViewTrack* track = GetTrackForParameter(AnimParamType::Scale);

    if (GetIEditor()->GetAnimation()->IsRecording() && m_pNodeEntity->IsSelected() && track)
    {
        CTrackViewSequence* sequence = GetSequence();

        AZ_Assert(sequence, "Expected valid sequence");
        if (sequence != nullptr)
        {
            if (sequence->GetSequenceType() == SequenceType::Legacy)
            {
                CUndo::Record(new CUndoTrackObject(track, sequence));
                m_pAnimNode->SetScale(sequence->GetTime(), scale);
            }
            else
            {
                // This is required because the entity movement system uses Undo to
                // undo a previous move delta as the entity is dragged.
                CUndo::Record(new CUndoComponentEntityTrackObject(track));

                m_pAnimNode->SetScale(sequence->GetTime(), scale);

                // We don't want to use ScopedUndoBatch here because we don't want a separate Undo operation
                // generate for every frame as the user scales an entity.
                AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                    &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity,
                    sequence->GetSequenceComponentEntityId()
                );
            }
            sequence->OnKeysChanged();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetRotation(const Quat& rotation)
{
    CTrackViewTrack* track = GetTrackForParameter(AnimParamType::Rotation);
    CRenderViewport* renderViewport = static_cast<CRenderViewport*>(GetIEditor()->GetViewManager()->GetGameViewport());

    if (GetIEditor()->GetAnimation()->IsRecording() && (m_pNodeEntity->IsSelected() || renderViewport->GetCameraObject() == m_pNodeEntity) && track)
    {
        CTrackViewSequence* sequence = GetSequence();

        AZ_Assert(sequence, "Expected valid sequence");
        if (sequence != nullptr)
        {
            const int flags = m_pAnimNode->GetFlags();
            if (sequence->GetSequenceType() == SequenceType::Legacy)
            {
                CUndo::Record(new CUndoTrackObject(track, GetSequence()));
                // Set the selected flag to enable record when unselected camera is moved through viewport
                m_pAnimNode->SetFlags(flags | eAnimNodeFlags_EntitySelected);
                m_pAnimNode->SetRotate(sequence->GetTime(), rotation);
                m_pAnimNode->SetFlags(flags);
            }
            else
            {
                // This is required because the entity movement system uses Undo to
                // undo a previous move delta as the entity is dragged.
                CUndo::Record(new CUndoComponentEntityTrackObject(track));

                // Set the selected flag to enable record when unselected camera is moved through viewport
                m_pAnimNode->SetFlags(flags | eAnimNodeFlags_EntitySelected);
                m_pAnimNode->SetRotate(sequence->GetTime(), rotation);
                m_pAnimNode->SetFlags(flags);

                // We don't want to use ScopedUndoBatch here because we don't want a separate Undo operation
                // generate for every frame as the user rotates an entity.
                AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                    &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity,
                    sequence->GetSequenceComponentEntityId()
                );
            }
            sequence->OnKeysChanged();
        }
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
        const AnimNodeType animNodeType = GetType();
        assert(animNodeType == AnimNodeType::Camera || animNodeType == AnimNodeType::Entity || animNodeType == AnimNodeType::GeomCache || animNodeType == AnimNodeType::AzEntity);

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
        if (m_pAnimNode->GetType() == AnimNodeType::Component)
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
        Matrix34 gizmoMatrix;
        gizmoMatrix.SetIdentity();

        if (GetType() == AnimNodeType::AzEntity)
        {
            // Key data are always relative to the parent (or world if there is no parent). So get the parent
            // entity id if there is one.
            AZ::EntityId parentId;
            AZ::TransformBus::EventResult(parentId, GetAzEntityId(), &AZ::TransformBus::Events::GetParentId);
            if (parentId.IsValid())
            {
                AZ::Transform azWorldTM = GetEntityWorldTM(parentId);
                gizmoMatrix = AZTransformToLYTransform(azWorldTM);
            }
        }
        else
        {
            // Legacy system.
            gizmoMatrix = m_pNodeEntity->GetParentAttachPointWorldTM();
        }

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
        CTrackViewTrack* pAnimTrack = GetTrackForParameter(AnimParamType::Event);
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

    if (GetType() == AnimNodeType::AzEntity)
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
    if (m_pAnimSequence && m_pAnimSequence->GetSequenceType() == SequenceType::SequenceComponent &&
        m_pAnimSequence->GetSequenceEntityId().IsValid())
    {
        AZ::EntityId remappedId;
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequestBus::Events::MapEditorIdToRuntimeId, m_pAnimSequence->GetSequenceEntityId(), remappedId);
            
        if (remappedId.IsValid())
        {
            // stash and remap the AZ::EntityId of the SequenceComponent entity to restore it when we switch back to Edit mode
            m_stashedAnimSequenceEditorAzEntityId = m_pAnimSequence->GetSequenceEntityId();
            m_pAnimSequence->SetSequenceEntityId(remappedId);
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
        m_pAnimSequence->SetSequenceEntityId(m_stashedAnimSequenceEditorAzEntityId);

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
                // Check to see if the component is still on the entity, but just disabled. Don't remove it in that case.
                AZ::Entity::ComponentArrayType disabledComponents;
                AzToolsFramework::EditorDisabledCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);

                bool isDisabled = false;
                for (auto disabledComponent : disabledComponents)
                {
                    if (disabledComponent->GetId() == childAnimNode->GetComponentId())
                    {
                        isDisabled = true;
                        break;
                    }
                }

                if (!isDisabled)
                {
                    AzToolsFramework::ScopedUndoBatch undoBatch("Remove Track View Component Node");
                    RemoveSubNode(childAnimNode);
                    CTrackViewSequence* sequence = GetSequence();
                    AZ_Assert(sequence != nullptr, "Sequence should not be null");
                    undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
                }
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

    if (IsBoundToAzEntity())
    {
        AZ::EntityId entityId = GetAzEntityId();
        AZ::TransformNotificationBus::Handler::BusDisconnect(entityId);
        AZ::EntityBus::Handler::BusDisconnect(entityId);
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
        retNewComponentNode = CreateSubNode(componentName.c_str(), AnimNodeType::Component, nullptr, componentTypeId, component->GetId());
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

    return Vec3(CheckTrackAnimated(AnimParamType::PositionX) ? position.x : basePos.x,
        CheckTrackAnimated(AnimParamType::PositionY) ? position.y : basePos.y,
        CheckTrackAnimated(AnimParamType::PositionZ) ? position.z : basePos.z);
}

//////////////////////////////////////////////////////////////////////////
Quat CTrackViewAnimNode::GetTransformDelegateRotation(const Quat& baseRotation) const
{
    if (!CheckTrackAnimated(AnimParamType::Rotation))
    {
        return baseRotation;
    }

    // Pass the sequence time to get the rotation from the
    // track data if it is present. We don't want to go all the way out
    // to the current rotation in component transform because that would mean
    // we are going from Quat to Euler and then back to Quat and that could lead
    // to the data drifting away from the original value.
    Quat nodeRotation = GetRotation(GetSequenceConst()->GetTime());

    const Ang3 angBaseRotation(baseRotation);
    const Ang3 angNodeRotation(nodeRotation);
    return Quat(Ang3(CheckTrackAnimated(AnimParamType::RotationX) ? angNodeRotation.x : angBaseRotation.x,
            CheckTrackAnimated(AnimParamType::RotationY) ? angNodeRotation.y : angBaseRotation.y,
            CheckTrackAnimated(AnimParamType::RotationZ) ? angNodeRotation.z : angBaseRotation.z));
}

//////////////////////////////////////////////////////////////////////////
Vec3 CTrackViewAnimNode::GetTransformDelegateScale(const Vec3& baseScale) const
{
    const Vec3 scale = GetScale();

    return Vec3(CheckTrackAnimated(AnimParamType::ScaleX) ? scale.x : baseScale.x,
        CheckTrackAnimated(AnimParamType::ScaleY) ? scale.y : baseScale.y,
        CheckTrackAnimated(AnimParamType::ScaleZ) ? scale.z : baseScale.z);
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
    const bool bDelegated = (GetIEditor()->GetAnimation()->IsRecording() && m_pNodeEntity->IsSelected() && GetTrackForParameter(AnimParamType::Position)) || CheckTrackAnimated(AnimParamType::Position);
    return bDelegated;
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsRotationDelegated() const
{
    const bool bDelegated = (GetIEditor()->GetAnimation()->IsRecording() && m_pNodeEntity->IsSelected() && GetTrackForParameter(AnimParamType::Rotation)) || CheckTrackAnimated(AnimParamType::Rotation);
    return bDelegated;
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsScaleDelegated() const
{
    const bool bDelegated = (GetIEditor()->GetAnimation()->IsRecording() && m_pNodeEntity->IsSelected() && GetTrackForParameter(AnimParamType::Scale)) || CheckTrackAnimated(AnimParamType::Scale);
    return bDelegated;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::OnDone()
{
    SetNodeEntity(nullptr);
    UpdateTrackGizmo();
}

//////////////////////////////////////////////////////////////////////////
AZ::Transform CTrackViewAnimNode::GetEntityWorldTM(const AZ::EntityId entityId)
{
    AZ::Entity* entity = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);

    AZ::Transform worldTM = AZ::Transform::Identity();
    if (entity != nullptr)
    {
        AZ::TransformInterface* transformInterface = entity->GetTransform();
        if (transformInterface != nullptr)
        {
            worldTM = transformInterface->GetWorldTM();
        }
    }

    return worldTM;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::UpdateKeyDataAfterParentChanged(const AZ::Transform& oldParentWorldTM, const AZ::Transform& newParentWorldTM)
{
    // Update the Position, Rotation and Scale tracks.
    AZStd::vector<AnimParamType> animParamTypes{ AnimParamType::Position, AnimParamType::Rotation, AnimParamType::Scale };
    for (AnimParamType animParamType : animParamTypes)
    {
        CTrackViewTrack* track = GetTrackForParameter(animParamType);
        if (track != nullptr)
        {
            track->UpdateKeyDataAfterParentChanged(oldParentWorldTM, newParentWorldTM);
        }
    }

    // Refresh after key data changed or parent changed.
    CTrackViewSequence* sequence = GetSequence();
    if (sequence != nullptr)
    {
        sequence->OnKeysChanged();
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::OnParentChanged(AZ::EntityId oldParent, AZ::EntityId newParent)
{
    // If the change is from no parent to parent, or the other way around,
    // update the key data, because that action is like going from world space to
    // relative to a new parent.
    if (!oldParent.IsValid() || !newParent.IsValid())
    {
        // Get the world transforms, Identity if there was no parent
        AZ::Transform oldParentWorldTM = GetEntityWorldTM(oldParent);
        AZ::Transform newParentWorldTM = GetEntityWorldTM(newParent);

        UpdateKeyDataAfterParentChanged(oldParentWorldTM, newParentWorldTM);
    }

    // Refresh after key data changed or parent changed.
    CTrackViewSequence* sequence = GetSequence();
    if (sequence != nullptr)
    {
        sequence->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_NodeOwnerChanged);
    }
}

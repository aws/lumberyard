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

#include <AzToolsFramework/API/ComponentEntityObjectBus.h>
#include <Maestro/Bus/EditorSequenceComponentBus.h>

#include "TrackViewUndo.h"
#include "TrackViewSequenceManager.h"
#include "TrackViewSequence.h"
#include "TrackViewTrack.h"
#include "Objects/ObjectLayer.h"
#include "Objects/EntityObject.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimParamType.h"
#include "Maestro/Types/SequenceType.h"

//////////////////////////////////////////////////////////////////////////
CUndoSequenceSettings::CUndoSequenceSettings(CTrackViewSequence* pSequence)
    : m_pSequence(pSequence)
    , m_oldTimeRange(pSequence->GetTimeRange())
    , m_oldFlags(pSequence->GetFlags())
{
}

//////////////////////////////////////////////////////////////////////////
void CUndoSequenceSettings::Undo(bool bUndo)
{
    m_newTimeRange = m_pSequence->GetTimeRange();
    m_newFlags = m_pSequence->GetFlags();

    m_pSequence->SetTimeRange(m_oldTimeRange);
    m_pSequence->SetFlags(m_oldFlags);
}

//////////////////////////////////////////////////////////////////////////
void CUndoSequenceSettings::Redo()
{
    m_pSequence->SetTimeRange(m_newTimeRange);
    m_pSequence->SetFlags(m_newFlags);
}

//////////////////////////////////////////////////////////////////////////
CUndoAnimKeySelection::CUndoAnimKeySelection(CTrackViewSequence* pSequence)
    : m_pSequence(pSequence)
{
    // Stores the current state of this sequence.
    assert(pSequence);

    // Save sequence name and key selection states
    if (pSequence)
    {
        m_undoKeyStates = SaveKeyStates(pSequence);
    }
}

//////////////////////////////////////////////////////////////////////////
CUndoAnimKeySelection::CUndoAnimKeySelection(CTrackViewTrack* pTrack)
    : m_pSequence(pTrack->GetSequence())
{
    // This is called from CUndoTrackObject which will save key states itself
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimKeySelection::Undo(bool bUndo)
{
    {
        CTrackViewSequenceNoNotificationContext context(m_pSequence);

        // Save key selection states for redo if necessary
        if (bUndo)
        {
            m_redoKeyStates = SaveKeyStates(m_pSequence);
        }

        // Undo key selection state
        RestoreKeyStates(m_pSequence, m_undoKeyStates);
    }

    if (bUndo)
    {
        m_pSequence->OnKeySelectionChanged();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimKeySelection::Redo()
{
    // Redo key selection state
    RestoreKeyStates(m_pSequence, m_redoKeyStates);
}

//////////////////////////////////////////////////////////////////////////
std::vector<bool> CUndoAnimKeySelection::SaveKeyStates(CTrackViewSequence* pSequence) const
{
    CTrackViewKeyBundle keys = pSequence->GetAllKeys();
    const unsigned int numkeys = keys.GetKeyCount();

    std::vector<bool> selectionState;
    selectionState.reserve(numkeys);

    for (unsigned int i = 0; i < numkeys; ++i)
    {
        const CTrackViewKeyHandle& keyHandle = keys.GetKey(i);
        selectionState.push_back(keyHandle.IsSelected());
    }

    return selectionState;
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimKeySelection::RestoreKeyStates(CTrackViewSequence* pSequence, const std::vector<bool> keyStates)
{
    CTrackViewKeyBundle keys = pSequence->GetAllKeys();
    const unsigned int numkeys = keys.GetKeyCount();
    
    // if we were constructed by CUndoTrackObject via CUndoAnimKeySelection(CTrackViewTrack* pTrack), we won't have
    // keyStates filled and there is no key state to restore
    if (keyStates.size() >= numkeys)
    {
        CTrackViewSequenceNotificationContext context(pSequence);
        for (unsigned int i = 0; i < numkeys; ++i)
        {
            CTrackViewKeyHandle keyHandle = keys.GetKey(i);
            keyHandle.Select(keyStates[i]);
        }
    }  
}

//////////////////////////////////////////////////////////////////////////
bool CUndoAnimKeySelection::IsSelectionChanged() const
{
    const std::vector<bool> currentKeyState = SaveKeyStates(m_pSequence);
    return m_undoKeyStates != currentKeyState;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CUndoTrackObject::CUndoTrackObject(CTrackViewTrack* pTrack, bool bStoreKeySelection)
    : CUndoAnimKeySelection(pTrack)
    , m_pTrack(pTrack)
    , m_bStoreKeySelection(bStoreKeySelection)
{
    assert(pTrack);

    if (m_bStoreKeySelection)
    {
        m_undoKeyStates = SaveKeyStates(m_pSequence);
    }

    // Stores the current state of this track.
    assert(pTrack != 0);

    // Store undo info.
    m_undo = pTrack->GetMemento();

    CObjectLayer* objectLayer = m_pSequence->GetSequenceObjectLayer();

    if (objectLayer)
    {
        objectLayer->SetModified();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackObject::Undo(bool bUndo)
{
    assert(m_pSequence);

    {
        CTrackViewSequenceNoNotificationContext context(m_pSequence);

        if (bUndo)
        {
            m_redo = m_pTrack->GetMemento();

            if (m_bStoreKeySelection)
            {
                // Save key selection states for redo if necessary
                m_redoKeyStates = SaveKeyStates(m_pSequence);
            }
        }

        // Undo track state.
        m_pTrack->RestoreFromMemento(m_undo);

        CObjectLayer* objectLayer = m_pSequence->GetSequenceObjectLayer();

        assert(objectLayer);
        if (objectLayer && bUndo)
        {
            objectLayer->SetModified();
        }

        if (m_bStoreKeySelection)
        {
            // Undo key selection state
            RestoreKeyStates(m_pSequence, m_undoKeyStates);
        }
    }

    if (bUndo)
    {
        m_pSequence->OnKeysChanged();
    }
    else
    {
        m_pSequence->ForceAnimation();
    }
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackObject::Redo()
{
    assert(m_pSequence);

    // Redo track state.
    m_pTrack->RestoreFromMemento(m_redo);

    CObjectLayer* objectLayer = m_pSequence->GetSequenceObjectLayer();

    assert(objectLayer);
    if (objectLayer)
    {
        objectLayer->SetModified();
    }

    if (m_bStoreKeySelection)
    {
        // Redo key selection state
        RestoreKeyStates(m_pSequence, m_redoKeyStates);
    }

    m_pSequence->OnKeysChanged();
}

//////////////////////////////////////////////////////////////////////////
CAbstractUndoSequenceTransaction::CAbstractUndoSequenceTransaction(CTrackViewSequence* pSequence)
    : m_pSequence(pSequence)
{
}

//////////////////////////////////////////////////////////////////////////
void CAbstractUndoSequenceTransaction::AddSequence()
{
    m_pStoredTrackViewSequence.release();

    // We only need to add the sequence for legacy sequences because restored adds for Component Entity Sequences will restore
    // the Sequence Components, which are added back to TrackView and the movie system by the Sequence Component's Init() method
    if (m_pSequence->GetSequenceType() == SequenceType::Legacy)
    {
        CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();

        // For Legacy sequences
        // add sequence to the CryMovie system
        IMovieSystem* pMovieSystem = GetIEditor()->GetMovieSystem();
        pMovieSystem->AddSequence(m_pSequence->m_pAnimSequence.get());

        // Release ownership and add node back to CTrackViewSequenceManager
        pSequenceManager->m_sequences.push_back(std::unique_ptr<CTrackViewSequence>(m_pSequence));
        pSequenceManager->OnSequenceAdded(m_pSequence);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAbstractUndoSequenceTransaction::RemoveSequence(bool bAquireOwnership)
{
    CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();

    for (auto iter = pSequenceManager->m_sequences.begin(); iter != pSequenceManager->m_sequences.end(); ++iter)
    {
        std::unique_ptr<CTrackViewSequence>& currentSequence = *iter;

        if (currentSequence.get() == m_pSequence)
        {
            if (bAquireOwnership)
            {
                // Acquire ownership of sequence
                currentSequence.swap(m_pStoredTrackViewSequence);
                assert(m_pStoredTrackViewSequence.get());
            }

            // Remove from CryMovie and TrackView
            pSequenceManager->m_sequences.erase(iter);
            IMovieSystem* pMovieSystem = GetIEditor()->GetMovieSystem();
            pMovieSystem->RemoveSequence(m_pSequence->m_pAnimSequence.get());

            break;
        }
    }

    pSequenceManager->OnSequenceRemoved(m_pSequence);
}

//////////////////////////////////////////////////////////////////////////
void CUndoSequenceAdd::Undo(bool bUndo)
{
    RemoveSequence(bUndo);
}

//////////////////////////////////////////////////////////////////////////
void CUndoSequenceAdd::Redo()
{
    AddSequence();
}

//////////////////////////////////////////////////////////////////////////
CUndoSequenceRemove::CUndoSequenceRemove(CTrackViewSequence* pRemovedSequence)
    : CAbstractUndoSequenceTransaction(pRemovedSequence)
{
    RemoveSequence(true);
}

//////////////////////////////////////////////////////////////////////////
void CUndoSequenceRemove::Undo(bool bUndo)
{
    AddSequence();
}

//////////////////////////////////////////////////////////////////////////
void CUndoSequenceRemove::Redo()
{
    RemoveSequence(true);
}

//////////////////////////////////////////////////////////////////////////
CAbstractUndoAnimNodeTransaction::CAbstractUndoAnimNodeTransaction(CTrackViewAnimNode* pNode)
    : m_pParentNode(static_cast<CTrackViewAnimNode*>(pNode->GetParentNode()))
    , m_pNode(pNode)
{
}

//////////////////////////////////////////////////////////////////////////
void CAbstractUndoAnimNodeTransaction::AddNode()
{
    // Add node back to sequence
    m_pParentNode->m_pAnimSequence->AddNode(m_pNode->m_pAnimNode.get());

    // Release ownership and add node back to parent node
    CTrackViewNode* pNode = m_pStoredTrackViewNode.release();

    // Reset the sequence ptr in the node. This isn't usually required, but it can
    // be invalid in this case:
    // - Add new Legacy sequence.
    // - Add new node.
    // - Undo add node.
    // - Undo add sequence (sequence ptr in node is now invalid).
    // - Redo add sequence.
    // - Redo add node (this operation will have an invalid ptr to the deleted sequence).
    m_pNode->m_pAnimSequence = m_pParentNode->m_pAnimSequence;

    m_pParentNode->AddNode(m_pNode);

    m_pNode->BindToEditorObjects();
    m_pNode->GetSequence()->OnNodeChanged(m_pNode, ITrackViewSequenceListener::eNodeChangeType_Added);
}

//////////////////////////////////////////////////////////////////////////
void CAbstractUndoAnimNodeTransaction::RemoveNode(bool bAquireOwnership)
{
    m_pNode->UnBindFromEditorObjects();

    for (auto iter = m_pParentNode->m_childNodes.begin(); iter != m_pParentNode->m_childNodes.end(); ++iter)
    {
        std::unique_ptr<CTrackViewNode>& currentNode = *iter;

        if (currentNode.get() == m_pNode)
        {
            if (bAquireOwnership)
            {
                // Acquire ownership of node
                currentNode.swap(m_pStoredTrackViewNode);
                assert(m_pStoredTrackViewNode.get());
            }

            // Remove from parent node and sequence. Do not remove child relationships as we use this in the
            // stashed animNode data (which we acquire ownership of) for restore
            m_pParentNode->m_childNodes.erase(iter);

            if (m_pNode->GetType() == AnimNodeType::AzEntity)
            {
                // Disconnect the AzEntity from the EntityBus - this needs to happen before we remove it from the parent sequence
                static_cast<AZ::EntityBus::Handler*>(m_pNode)->BusDisconnect(m_pNode->GetAzEntityId());
            }
            
            m_pParentNode->m_pAnimSequence->RemoveNode(m_pNode->m_pAnimNode.get(), /*removeChildRelationships=*/ false);

            break;
        }
    }

    m_pNode->GetSequence()->OnNodeChanged(m_pNode, ITrackViewSequenceListener::eNodeChangeType_Removed);
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimNodeAdd::Undo(bool bUndo)
{
    RemoveNode(bUndo);
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimNodeAdd::Redo()
{
    AddNode();
}

//////////////////////////////////////////////////////////////////////////
CUndoAnimNodeRemove::CUndoAnimNodeRemove(CTrackViewAnimNode* pRemovedNode)
    : CAbstractUndoAnimNodeTransaction(pRemovedNode)
{
    RemoveNode(true);
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimNodeRemove::Undo(bool bUndo)
{
    AddNode();
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimNodeRemove::Redo()
{
    RemoveNode(true);
}

//////////////////////////////////////////////////////////////////////////
CAbstractUndoTrackTransaction::CAbstractUndoTrackTransaction(CTrackViewTrack* pTrack)
    : m_pParentNode(pTrack->GetAnimNode())
    , m_pTrack(pTrack)
{
    assert(!pTrack->IsSubTrack());
}

//////////////////////////////////////////////////////////////////////////
void CAbstractUndoTrackTransaction::AddTrack()
{
    // Add node back to sequence
    m_pParentNode->m_pAnimNode->AddTrack(m_pTrack->m_pAnimTrack.get());

    // Add track back to parent node and release ownership
    CTrackViewNode* pTrack = m_pStoredTrackViewTrack.release();
    m_pParentNode->AddNode(pTrack);
}

//////////////////////////////////////////////////////////////////////////
void CAbstractUndoTrackTransaction::RemoveTrack(bool bAquireOwnership)
{
    for (auto iter = m_pParentNode->m_childNodes.begin(); iter != m_pParentNode->m_childNodes.end(); ++iter)
    {
        std::unique_ptr<CTrackViewNode>& currentNode = *iter;

        if (currentNode.get() == m_pTrack)
        {
            if (bAquireOwnership)
            {
                // Acquire ownership of track
                currentNode.swap(m_pStoredTrackViewTrack);
            }

            // Remove from parent node and sequence
            m_pParentNode->m_childNodes.erase(iter);
            m_pParentNode->m_pAnimNode->RemoveTrack(m_pTrack->m_pAnimTrack.get());
            break;
        }
    }

    m_pParentNode->GetSequence()->OnNodeChanged(m_pStoredTrackViewTrack.get(), ITrackViewSequenceListener::eNodeChangeType_Removed);
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackAdd::Undo(bool bUndo)
{
    RemoveTrack(bUndo);
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackAdd::Redo()
{
    AddTrack();
}

//////////////////////////////////////////////////////////////////////////
CUndoTrackRemove::CUndoTrackRemove(CTrackViewTrack* pRemovedTrack)
    : CAbstractUndoTrackTransaction(pRemovedTrack)
{
    RemoveTrack(true);
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackRemove::Undo(bool bUndo)
{
    AddTrack();
}

//////////////////////////////////////////////////////////////////////////
void CUndoTrackRemove::Redo()
{
    RemoveTrack(true);
}

//////////////////////////////////////////////////////////////////////////
CUndoAnimNodeReparent::CUndoAnimNodeReparent(CTrackViewAnimNode* pAnimNode, CTrackViewAnimNode* pNewParent)
    : CAbstractUndoAnimNodeTransaction(pAnimNode)
    , m_pNewParent(pNewParent)
    , m_pOldParent(m_pParentNode)
{
    CTrackViewSequence* pSequence = pAnimNode->GetSequence();
    assert(pSequence == m_pNewParent->GetSequence() && pSequence == m_pOldParent->GetSequence());

    Reparent(pNewParent);
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimNodeReparent::Undo(bool bUndo)
{
    Reparent(m_pOldParent);
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimNodeReparent::Redo()
{
    Reparent(m_pNewParent);
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimNodeReparent::Reparent(CTrackViewAnimNode* pNewParent)
{
    RemoveNode(true);
    m_pParentNode = pNewParent;
    m_pNode->m_pAnimNode->SetParent(pNewParent->m_pAnimNode.get());
    AddParentsInChildren(m_pNode);
    AddNode();

    // This undo object must never have ownership of the node
    assert(m_pStoredTrackViewNode.get() == nullptr);
}

void CUndoAnimNodeReparent::AddParentsInChildren(CTrackViewAnimNode* pCurrentNode)
{
    const uint numChildren = pCurrentNode->GetChildCount();

    for (uint childIndex = 0; childIndex < numChildren; ++childIndex)
    {
        CTrackViewAnimNode* pChildAnimNode = static_cast<CTrackViewAnimNode*>(pCurrentNode->GetChild(childIndex));

        if (pChildAnimNode->GetNodeType() != eTVNT_Track)
        {
            pChildAnimNode->m_pAnimNode->SetParent(pCurrentNode->m_pAnimNode.get());

            if (pChildAnimNode->GetChildCount() > 0 && pChildAnimNode->GetNodeType() != eTVNT_AnimNode)
            {
                AddParentsInChildren(pChildAnimNode);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CUndoAnimNodeRename::CUndoAnimNodeRename(CTrackViewAnimNode* pNode, const string& oldName)
    : m_pNode(pNode)
    , m_newName(pNode->GetName())
    , m_oldName(oldName)
{
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimNodeRename::Undo(bool bUndo)
{
    m_pNode->SetName(m_oldName);
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimNodeRename::Redo()
{
    m_pNode->SetName(m_newName);
}

//////////////////////////////////////////////////////////////////////////
CUndoAnimNodeObjectRename::CUndoAnimNodeObjectRename(CBaseObject* entityObj, const string& newName)
    : m_baseObject(entityObj)
    , m_newName(newName)
    , m_oldName(entityObj->GetName().toUtf8().data())
{
    Rename(m_newName);
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimNodeObjectRename::Undo(bool bUndo)
{
    Rename(m_oldName);
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimNodeObjectRename::Redo()
{
    Rename(m_newName);
}

//////////////////////////////////////////////////////////////////////////
void CUndoAnimNodeObjectRename::Rename(const string& newName)
{
    // delegate renaming to object manager. TrackView is eventually  notified of via ON_RENAME listener callbacks or via
    // the Sequence Component rename
    static_cast<CObjectManager*>(GetIEditor()->GetObjectManager())->ChangeObjectName(m_baseObject, newName.c_str());
}

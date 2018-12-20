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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Maestro/Bus/EditorSequenceComponentBus.h>
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

#include "TrackViewSequenceManager.h"
#include "TrackViewUndo.h"
#include "Maestro/Types/SequenceType.h"
#include "Material/MaterialManager.h"
#include "AnimationContext.h"
#include "GameEngine.h"
#include "Maestro/Types/SequenceType.h"


////////////////////////////////////////////////////////////////////////////
CTrackViewSequenceManager::CTrackViewSequenceManager()
{
    GetIEditor()->RegisterNotifyListener(this);
    GetIEditor()->GetMaterialManager()->AddListener(this);
    GetIEditor()->GetObjectManager()->AddObjectEventListener(functor(*this, &CTrackViewSequenceManager::OnObjectEvent));
}

////////////////////////////////////////////////////////////////////////////
CTrackViewSequenceManager::~CTrackViewSequenceManager()
{
    GetIEditor()->GetObjectManager()->RemoveObjectEventListener(functor(*this, &CTrackViewSequenceManager::OnObjectEvent));
    GetIEditor()->GetMaterialManager()->RemoveListener(this);
    GetIEditor()->UnregisterNotifyListener(this);
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginGameMode:
        ResumeAllSequences();
        break;
    case eNotify_OnCloseScene:
    // Fall through
    case eNotify_OnBeginLoad:
        m_bUnloadingLevel = true;
        break;
    case eNotify_OnEndNewScene:
    // Fall through
    case eNotify_OnEndSceneOpen:
    // Fall through
    case eNotify_OnEndLoad:
    // Fall through
    case eNotify_OnLayerImportEnd:
        m_bUnloadingLevel = false;
        SortSequences();
        break;
    }   
}

////////////////////////////////////////////////////////////////////////////
CTrackViewSequence* CTrackViewSequenceManager::GetSequenceByName(QString name) const
{
    for (auto iter = m_sequences.begin(); iter != m_sequences.end(); ++iter)
    {
        CTrackViewSequence* pSequence = (*iter).get();

        if (pSequence->GetName() == name)
        {
            return pSequence;
        }
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////
CTrackViewSequence* CTrackViewSequenceManager::GetLegacySequenceByName(QString name) const
{
    for (auto iter = m_sequences.begin(); iter != m_sequences.end(); ++iter)
    {
        CTrackViewSequence* pSequence = (*iter).get();

        if (pSequence->GetSequenceType() == SequenceType::Legacy && pSequence->GetName() == name)
        {
            return pSequence;
        }
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////
CTrackViewSequence* CTrackViewSequenceManager::GetSequenceByEntityId(const AZ::EntityId& entityId) const
{
    for (auto iter = m_sequences.begin(); iter != m_sequences.end(); ++iter)
    {
        CTrackViewSequence* pSequence = (*iter).get();

        if (pSequence->GetSequenceComponentEntityId() == entityId)
        {
            return pSequence;
        }
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////
CTrackViewSequence* CTrackViewSequenceManager::GetSequenceByAnimSequence(IAnimSequence* pAnimSequence) const
{
    for (auto iter = m_sequences.begin(); iter != m_sequences.end(); ++iter)
    {
        CTrackViewSequence* pSequence = (*iter).get();

        if (pSequence->m_pAnimSequence == pAnimSequence)
        {
            return pSequence;
        }
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////
CTrackViewSequence* CTrackViewSequenceManager::GetSequenceByIndex(unsigned int index) const
{
    if (index >= m_sequences.size())
    {
        return nullptr;
    }

    return m_sequences[index].get();
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::CreateSequence(QString name, SequenceType sequenceType)
{
    CGameEngine* pGameEngine = GetIEditor()->GetGameEngine();
    if (!pGameEngine || !pGameEngine->IsLevelLoaded())
    {
        return;
    }

    CTrackViewSequence* pExistingSequence = GetSequenceByName(name);
    if (pExistingSequence)
    {
        return;
    }

    CUndo undo("Create TrackView Sequence");
    if (sequenceType == SequenceType::Legacy)
    {
        GetIEditor()->GetObjectManager()->NewObject("SequenceObject", 0, name);
    }
    else if (sequenceType == SequenceType::SequenceComponent)
    {
        AzToolsFramework::ScopedUndoBatch undoBatch("Create TrackView Sequence");

        // create AZ::Entity at the current center of the viewport, but don't select it

        // Store the current selection for selection restore after the sequence component is created
        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

        AZ::EntityId newEntityId;   // initialized with InvalidEntityId
        EBUS_EVENT_RESULT(newEntityId, AzToolsFramework::EditorRequests::Bus, CreateNewEntity, AZ::EntityId());
        if (newEntityId.IsValid())
        {
            // set the entity name
            AZ::Entity* entity = nullptr;
            EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, newEntityId);
            if (entity)
            {
                entity->SetName(static_cast<const char*>(name.toUtf8().data()));
            }

            // add the SequenceComponent. The SequenceComponent's Init() method will call OnCreateSequenceObject() which will actually create
            // the sequence and connect it
            // #TODO LY-21846: Use "SequenceService" to find component, rather than specific component-type.
            AzToolsFramework::EntityCompositionRequestBus::Broadcast(&AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities, AzToolsFramework::EntityIdList{ newEntityId }, AZ::ComponentTypeList{ "{C02DC0E2-D0F3-488B-B9EE-98E28077EC56}" });

            // restore the Editor selection
            AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::Bus::Events::SetSelectedEntities, selectedEntities);

            undoBatch.MarkEntityDirty(newEntityId);
        }
    }
}

////////////////////////////////////////////////////////////////////////////
IAnimSequence* CTrackViewSequenceManager::OnCreateSequenceObject(QString name, bool isLegacySequence, AZ::EntityId entityId)
{
    SequenceType sequenceType = (isLegacySequence ? SequenceType::Legacy : SequenceType::SequenceComponent);
    IAnimSequence* pNewCryMovieSequence = GetIEditor()->GetMovieSystem()->CreateSequence(name.toUtf8().data(), /*bload =*/ false, /*id =*/ 0U, sequenceType, entityId);
    CTrackViewSequence* newTrackViewSequence = new CTrackViewSequence(pNewCryMovieSequence);

    AddTrackViewSequence(newTrackViewSequence);

    return pNewCryMovieSequence;
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::OnSequenceActivated(const AZ::EntityId& entityId)
{
    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    if (pAnimationContext != nullptr)
    {
        pAnimationContext->OnSequenceActivated(entityId);
    }
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::OnLegacySequencePostLoad(CTrackViewSequence* sequence, bool undo)
{
    for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
    {
        (*iter)->OnLegacySequencePostLoad(sequence, undo);
    }
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::OnCreateSequenceComponent(AZStd::intrusive_ptr<IAnimSequence>& sequence)
{
    // Fix up the internal pointers in the sequence to match the deserialized structure
    sequence->InitPostLoad();

    // Add the sequence to the movie system
    GetIEditor()->GetMovieSystem()->AddSequence(sequence.get());

    // Create the TrackView Sequence
    CTrackViewSequence* newTrackViewSequence = new CTrackViewSequence(sequence);

    AddTrackViewSequence(newTrackViewSequence);
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::AddTrackViewSequence(CTrackViewSequence* sequenceToAdd)
{
    m_sequences.push_back(std::unique_ptr<CTrackViewSequence>(sequenceToAdd));
    SortSequences();
    OnSequenceAdded(sequenceToAdd);
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::DeleteSequence(CTrackViewSequence* pSequence)
{
    const int numSequences = m_sequences.size();
    for (int sequenceIndex = 0; sequenceIndex < numSequences; ++sequenceIndex)
    {
        if (m_sequences[sequenceIndex].get() == pSequence)
        {
            if (pSequence->GetSequenceType() == SequenceType::Legacy)
            {
                CUndo undo("Delete TrackView Sequence");
                // delete legacy sequence object
                CSequenceObject* pSequenceObject = static_cast<CSequenceObject*>(pSequence->m_pAnimSequence->GetLegacySequenceObject());
                GetIEditor()->GetObjectManager()->DeleteObject(pSequenceObject);
            }
            else
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Delete TrackView Sequence");

                // delete Sequence Component (and entity if there's no other components left on the entity except for the Transform Component)
                AZ::Entity* entity = nullptr;
                AZ::EntityId entityId = pSequence->m_pAnimSequence->GetSequenceEntityId();
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
                if (entity)
                {
                    const AZ::Uuid editorSequenceComponentTypeId(EditorSequenceComponentTypeId);
                    AZ::Component* sequenceComponent = entity->FindComponent(editorSequenceComponentTypeId);
                    if (sequenceComponent)
                    {
                        AZ::ComponentTypeList requiredComponents;
                        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(requiredComponents, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetRequiredComponentTypes);
                        const int numComponentToDeleteEntity = requiredComponents.size() + 1;

                        AZ::Entity::ComponentArrayType entityComponents = entity->GetComponents();
                        if (entityComponents.size() == numComponentToDeleteEntity)
                        {
                            // if the entity only has required components + 1 (the found sequenceComponent), delete the Entity. No need to start undo here
                            // AzToolsFramework::ToolsApplicationRequests::DeleteEntities will take care of that
                            AzToolsFramework::EntityIdList entitiesToDelete;
                            entitiesToDelete.push_back(entityId);

                            AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::DeleteEntities, entitiesToDelete);
                        }
                        else
                        {
                            // just remove the sequence component from the entity
                            CUndo undo("Delete TrackView Sequence");

                            AzToolsFramework::EntityCompositionRequestBus::Broadcast(&AzToolsFramework::EntityCompositionRequests::RemoveComponents, AZ::Entity::ComponentArrayType{ sequenceComponent });
                        }

                        undoBatch.MarkEntityDirty(entityId);
                    }
                }
            }

            // sequence was deleted, we can stop searching
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::RenameNode(CTrackViewAnimNode* pAnimNode, const char* newName) const
{
    CBaseObject*    baseObj = nullptr;
    CTrackViewSequence* sequence = pAnimNode->GetSequence();

    AZ_Assert(sequence, "Nodes should never have a null sequence.");

    bool isLegacySequence = sequence->GetSequenceType() == SequenceType::Legacy;

    if (pAnimNode->IsBoundToEditorObjects())
    {
        if (pAnimNode->GetNodeType() == eTVNT_Sequence)
        {
            CTrackViewSequence* sequenceNode = static_cast<CTrackViewSequence*>(pAnimNode);

            // Find the baseObject that represents and contains the Sequence data
            if (sequenceNode->GetSequenceType() == SequenceType::Legacy)
            {
                baseObj = sequenceNode->GetSequenceObject();
            }
            else if (sequenceNode->GetSequenceType() == SequenceType::SequenceComponent)
            {
                AzToolsFramework::ComponentEntityEditorRequestBus::EventResult(baseObj, sequenceNode->GetSequenceComponentEntityId(), &AzToolsFramework::ComponentEntityEditorRequestBus::Events::GetSandboxObject);
            }
        }
        else if (pAnimNode->GetNodeType() == eTVNT_AnimNode)
        {
            baseObj = pAnimNode->GetNodeEntity();
        }
    }

    if (baseObj)
    {
        if (isLegacySequence)
        {
            // We use AzToolsFramework::ScopedUndoBatch instead of the legacy CUndo to get on the AzToolsFramework undo stack for renaming
            // The AzToolsFramework::ScopedUndoBatch is also a wrapper for the legacy CUndo stack, so CUndo::Record works as expected.
            AzToolsFramework::ScopedUndoBatch undoBatch("ModifyEntityName");
            CUndo::Record(new CUndoAnimNodeObjectRename(baseObj, newName));
        }
        else
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("ModifyEntityName");
            static_cast<CObjectManager*>(GetIEditor()->GetObjectManager())->ChangeObjectName(baseObj, newName);
            undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
        }
    }
    else
    {
        // this is an internal TrackView Node - handle it internally
        if (isLegacySequence)
        {
            CUndo undo("Rename TrackView Node");
            pAnimNode->SetName(newName);
        }
        else
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("Rename TrackView Node");
            pAnimNode->SetName(newName);
            undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
        }
    }
}

void CTrackViewSequenceManager::RemoveSequenceInternal(CTrackViewSequence* sequence)
{
    std::unique_ptr<CTrackViewSequence> storedTrackViewSequence;
    
    for (auto iter = m_sequences.begin(); iter != m_sequences.end(); ++iter)
    {
        std::unique_ptr<CTrackViewSequence>& currentSequence = *iter;

        if (currentSequence.get() == sequence)
        {
            // Hang onto this until we finish this function.
            currentSequence.swap(storedTrackViewSequence);

            // Remove from CryMovie and TrackView
            m_sequences.erase(iter);
            IMovieSystem* pMovieSystem = GetIEditor()->GetMovieSystem();
            pMovieSystem->RemoveSequence(sequence->m_pAnimSequence.get());

            break;
        }
    }

    OnSequenceRemoved(sequence);
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::OnDeleteSequenceEntity(const AZ::EntityId& entityId)
{
    CTrackViewSequence* pSequence = GetSequenceByEntityId(entityId);
    assert(pSequence);

    if (pSequence)
    {
        const bool bUndoWasSuspended = GetIEditor()->IsUndoSuspended();
        bool isDuringUndo = false;

        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(isDuringUndo, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsDuringUndoRedo);

        if (bUndoWasSuspended)
        {
            GetIEditor()->ResumeUndo();
        }

        // Component based sequence use AZ Undo, not Track View custom undo code
        auto useTrackViewUndo = pSequence->GetSequenceType() == SequenceType::Legacy;

        if (m_bUnloadingLevel || isDuringUndo)
        {
            if (useTrackViewUndo)
            {
                // While unloading or during AZ::Undo, there is no recording so
                // only make the undo object destroy the sequence
                std::unique_ptr<CUndoSequenceRemove> sequenceRemove(new CUndoSequenceRemove(pSequence));
            }
            else
            {
                RemoveSequenceInternal(pSequence);
            }
        }
        else
        {
            // Remove the sequence from the managers.
            // Allow AZ::Undo take care of Undo/Redo in the case of component entity sequences
            // and generic CUndoBaseObject take care of Undo/Redo in the case of Legacy sequences.
            RemoveSequenceInternal(pSequence);
        }

        if (bUndoWasSuspended)
        {
            GetIEditor()->SuspendUndo();
        }
    }
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::SortSequences()
{
    std::stable_sort(m_sequences.begin(), m_sequences.end(),
        [](const std::unique_ptr<CTrackViewSequence>& a, const std::unique_ptr<CTrackViewSequence>& b) -> bool
        {
            QString aName = a.get()->GetName();
            QString bName = b.get()->GetName();
            return aName < bName;
        });
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::ResumeAllSequences()
{
    for (auto iter = m_sequences.begin(); iter != m_sequences.end(); ++iter)
    {
        CTrackViewSequence* pSequence = (*iter).get();
        if (pSequence)
        {
            pSequence->Resume();
        }
    }
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::OnSequenceAdded(CTrackViewSequence* pSequence)
{
    for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
    {
        (*iter)->OnSequenceAdded(pSequence);
    }
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::OnSequenceRemoved(CTrackViewSequence* pSequence)
{
    for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
    {
        (*iter)->OnSequenceRemoved(pSequence);
    }
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event)
{
    if (event != EDataBaseItemEvent::EDB_ITEM_EVENT_ADD)
    {
        const uint numSequences = m_sequences.size();

        for (uint i = 0; i < numSequences; ++i)
        {
            m_sequences[i]->UpdateDynamicParams();
        }
    }
}

////////////////////////////////////////////////////////////////////////////
CTrackViewAnimNodeBundle CTrackViewSequenceManager::GetAllRelatedAnimNodes(const CEntityObject* pEntityObject) const
{
    CTrackViewAnimNodeBundle nodeBundle;

    const uint sequenceCount = GetCount();

    for (uint sequenceIndex = 0; sequenceIndex < sequenceCount; ++sequenceIndex)
    {
        CTrackViewSequence* pSequence = GetSequenceByIndex(sequenceIndex);
        nodeBundle.AppendAnimNodeBundle(pSequence->GetAllOwnedNodes(pEntityObject));
    }

    return nodeBundle;
}

////////////////////////////////////////////////////////////////////////////
CTrackViewAnimNode* CTrackViewSequenceManager::GetActiveAnimNode(const CEntityObject* pEntityObject) const
{
    CTrackViewAnimNodeBundle nodeBundle = GetAllRelatedAnimNodes(pEntityObject);

    const uint nodeCount = nodeBundle.GetCount();
    for (uint nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex)
    {
        CTrackViewAnimNode* pAnimNode = nodeBundle.GetNode(nodeIndex);
        if (pAnimNode->IsActive())
        {
            return pAnimNode;
        }
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::OnObjectEvent(CBaseObject* pObject, int event)
{
    if (event == CBaseObject::ON_PREATTACHED || event == CBaseObject::ON_PREDETACHED
        || event == CBaseObject::ON_ATTACHED || event == CBaseObject::ON_DETACHED)
    {
        HandleAttachmentChange(pObject, event);
    }
    else if (event == CBaseObject::ON_RENAME)
    {
        HandleObjectRename(pObject);
    }
    else if (event == CBaseObject::ON_PREDELETE)
    {
        HandleObjectPreDelete(pObject);
    }
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::HandleAttachmentChange(CBaseObject* pObject, int event)
{
    // If an object gets attached/detached from its parent we need to update all related anim nodes, otherwise
    // they will end up very near the origin or very far away from the attached object when animated

    if (!qobject_cast<CEntityObject*>(pObject) || pObject->CheckFlags(OBJFLAG_DELETED))
    {
        return;
    }

    CEntityObject* pEntityObject = static_cast<CEntityObject*>(pObject);
    CTrackViewAnimNodeBundle bundle = GetAllRelatedAnimNodes(pEntityObject);

    const uint numAffectedAnimNodes = bundle.GetCount();
    if (numAffectedAnimNodes == 0)
    {
        return;
    }

    std::unordered_set<CTrackViewSequence*> affectedSequences;
    for (uint i = 0; i < numAffectedAnimNodes; ++i)
    {
        CTrackViewAnimNode* pAnimNode = bundle.GetNode(i);
        affectedSequences.insert(pAnimNode->GetSequence());
    }

    CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
    CTrackViewSequence* pActiveSequence = pAnimationContext->GetSequence();
    const float time = pAnimationContext->GetTime();

    for (auto iter = affectedSequences.begin(); iter != affectedSequences.end(); ++iter)
    {
        CTrackViewSequence* pSequence = *iter;
        pAnimationContext->SetSequence(pSequence, true, true);

        if (pSequence == pActiveSequence)
        {
            pAnimationContext->SetTime(time);
        }

        for (uint i = 0; i < numAffectedAnimNodes; ++i)
        {
            CTrackViewAnimNode* pNode = bundle.GetNode(i);
            if (pNode->GetSequence() == pSequence)
            {
                if (event == CBaseObject::ON_PREATTACHEDKEEPXFORM || event == CBaseObject::ON_PREDETACHEDKEEPXFORM)
                {
                    const Matrix34 transform = pNode->GetNodeEntity()->GetWorldTM();
                    m_prevTransforms.emplace(pNode, transform);
                }
                else if (event == CBaseObject::ON_ATTACHED || event == CBaseObject::ON_DETACHED)
                {
                    auto findIter = m_prevTransforms.find(pNode);
                    if (findIter != m_prevTransforms.end())
                    {
                        pNode->GetNodeEntity()->SetWorldTM(findIter->second);
                    }
                }
            }
        }
    }

    if (event == CBaseObject::ON_ATTACHED || event == CBaseObject::ON_DETACHED)
    {
        m_prevTransforms.clear();
    }

    pAnimationContext->SetSequence(pActiveSequence, true, true);
    pAnimationContext->SetTime(time);
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::HandleObjectRename(CBaseObject* pObject)
{
    CTrackViewAnimNodeBundle bundle;

    if (qobject_cast<CEntityObject*>(pObject))
    {
        // entity or component entity sequence object
        CEntityObject* pEntityObject = static_cast<CEntityObject*>(pObject);
        bundle = GetAllRelatedAnimNodes(pEntityObject);

        // GetAllRelatedAnimNodes only accounts for entities in the sequences, not the sequence entities themselves. We additionally check for sequence
        // entities that have pObject as their entity object for renaming
        const uint sequenceCount = GetCount();
        for (uint sequenceIndex = 0; sequenceIndex < sequenceCount; ++sequenceIndex)
        {
            CTrackViewSequence* sequence = GetSequenceByIndex(sequenceIndex);
            if (sequence->GetSequenceType() == SequenceType::SequenceComponent)
            {
                CBaseObject* sequenceObject = nullptr;
                AzToolsFramework::ComponentEntityEditorRequestBus::EventResult(sequenceObject, sequence->GetSequenceComponentEntityId(), &AzToolsFramework::ComponentEntityEditorRequestBus::Events::GetSandboxObject);
                if (pObject == sequenceObject)
                {
                    bundle.AppendAnimNode(sequence);
                }
            }
        }
    }
    else if (qobject_cast<CSequenceObject*>(pObject))
    {
        // renaming a legacy sequence object - find it and add it to the bundle
        const uint sequenceCount = GetCount();
        for (uint sequenceIndex = 0; sequenceIndex < sequenceCount; ++sequenceIndex)
        {
            CTrackViewSequence* sequence = GetSequenceByIndex(sequenceIndex);
            if (sequence->GetSequenceType() == SequenceType::Legacy)
            {
                if (pObject == sequence->GetSequenceObject())
                {
                    bundle.AppendAnimNode(sequence);
                }
            }
        }
    }

    const uint numAffectedNodes = bundle.GetCount();
    for (uint i = 0; i < numAffectedNodes; ++i)
    {
        CTrackViewAnimNode* pAnimNode = bundle.GetNode(i);
        pAnimNode->SetName(pObject->GetName().toUtf8().data());
    }
    
    if (numAffectedNodes > 0)
    {
        GetIEditor()->Notify(eNotify_OnReloadTrackView);
    }
}

void CTrackViewSequenceManager::HandleObjectPreDelete(CBaseObject* pObject)
{
    if (!qobject_cast<CEntityObject*>(pObject))
    {
        return;
    }

    // we handle pre-delete instead of delete because GetAllRelatedAnimNodes() uses the ObjectManager to find node owners
    CEntityObject* pEntityObject = static_cast<CEntityObject*>(pObject);
    CTrackViewAnimNodeBundle bundle = GetAllRelatedAnimNodes(pEntityObject);

    const uint numAffectedAnimNodes = bundle.GetCount();
    for (uint i = 0; i < numAffectedAnimNodes; ++i)
    {
        CTrackViewAnimNode* pAnimNode = bundle.GetNode(i);
        pAnimNode->OnEntityRemoved();
    }

    if (numAffectedAnimNodes > 0)
    {
        // Only reload trackview if the object being deleted has related anim nodes.
        GetIEditor()->Notify(eNotify_OnReloadTrackView);
    }
}


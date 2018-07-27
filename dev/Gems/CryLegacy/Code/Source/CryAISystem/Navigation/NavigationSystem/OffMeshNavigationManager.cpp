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
#include "OffMeshNavigationManager.h"

#include "SmartObjects.h"
#include "NavigationSystem.h"


OffMeshNavigationManager::OffMeshNavigationManager(const int offMeshMapSize)
    : m_offMeshMap(offMeshMapSize)
    , m_objectRegistrationEnabled(false)
    , m_operations(128)
    , m_listeners(32)
{
}

const MNM::OffMeshNavigation& OffMeshNavigationManager::GetOffMeshNavigationForMesh(const NavigationMeshID& meshID) const
{
    if (meshID && m_offMeshMap.validate(meshID))
    {
        return m_offMeshMap[meshID];
    }
    else
    {
        assert(0);
        AIWarning("No off-mesh navigation available, returning empty one! Navigation might not be able to use external links when path-finding");
        return m_emptyOffMeshNavigation;
    }
}

MNM::OffMeshNavigation& OffMeshNavigationManager::GetOffMeshNavigationForMesh(const NavigationMeshID& meshID)
{
    if (meshID && m_offMeshMap.validate(meshID))
    {
        return m_offMeshMap[meshID];
    }
    else
    {
        assert(0);
        AIWarning("No off-mesh navigation available, returning empty one! Navigation might not be able to use external links when path-finding");
        return m_emptyOffMeshNavigation;
    }
}

void OffMeshNavigationManager::QueueCustomLinkAddition(const MNM::LinkAdditionRequest& request)
{
    m_operations.push_back(request);
}

void OffMeshNavigationManager::QueueCustomLinkRemoval(const MNM::LinkRemovalRequest& request)
{
    m_operations.push_back(request);
}

bool OffMeshNavigationManager::AddCustomLink(const NavigationMeshID& meshID, MNM::OffMeshLink& linkData, MNM::OffMeshLinkID& linkID, bool dataExists, bool trimExcess)
{
    // Grab the navigation mesh
    NavigationMesh& mesh = gAIEnv.pNavigationSystem->GetMesh(meshID);

    // Query the entry/exit positions
    const Vec3 startPoint = linkData.GetStartPosition();
    const Vec3 endPoint = linkData.GetEndPosition();

    MNM::vector3_t fixedStartPoint(startPoint);
    MNM::vector3_t fixedEndPoint(endPoint);
    // Calc mid-point
    MNM::vector3_t fixedMidPoint((startPoint + endPoint) / 2.f);

    MNM::TriangleID startTriangleID, endTriangleID;

    const MNM::real_t range = MNM::real_t(1.0f);
    MNM::vector3_t startEdgePoint, endEdgePoint;

    // Get entry triangle
    if (trimExcess)
    {
        // If trimming excess length, find the closest triangle/point at the edge of the navmesh
        startTriangleID = mesh.grid.GetTriangleEdgeAlongLine(fixedMidPoint, fixedStartPoint, range, range, startEdgePoint);
    }
    else
    {
        startTriangleID = mesh.grid.GetTriangleAt(fixedStartPoint, range, range);
    }

    if (!startTriangleID)
    {
        //Bail out; Entry position not connected to a triangle
        return false;
    }

    // Get entry triangle
    if (trimExcess)
    {
        // If trimming excess length, find the closest triangle/point at the edge of the navmesh
        endTriangleID = mesh.grid.GetTriangleEdgeAlongLine(fixedMidPoint, fixedEndPoint, range, range, endEdgePoint);
    }
    else
    {
        endTriangleID = mesh.grid.GetTriangleAt(fixedEndPoint, range, range);
    }

    if (!endTriangleID)
    {
        //Bail out; Exit position not connected to a triangle
        return false;
    }

    if (startTriangleID == endTriangleID)
    {
        // points are on the same triangle so don't bother
        return false;
    }

    // If trimming excess length, set the new entry/exit
    if (trimExcess)
    {
        linkData.SetStartPosition(startEdgePoint.GetVec3());
        linkData.SetEndPosition(endEdgePoint.GetVec3());
    }

    // Select the corresponding off-mesh navigation object
    assert(m_offMeshMap.validate(meshID));
    MNM::OffMeshNavigation& offMeshNavigation = m_offMeshMap[meshID];

    // Register the new link with the off-mesh navigation system
    MNM::OffMeshLinkPtr pClonedData(offMeshNavigation.AddLink(mesh, startTriangleID, endTriangleID, linkData, linkID, dataExists));
    assert(linkID > 0);

    // If not updating an existing link, cache it
    if (dataExists == false)
    {
        m_links[linkID] = SLinkInfo(meshID, startTriangleID, linkID, pClonedData);
    }
    else
    {
        m_links[linkID].triangleID = startTriangleID;
    }

    gAIEnv.pNavigationSystem->AddIslandConnectionsBetweenTriangles(meshID, startTriangleID, endTriangleID);

    return true;
}

void OffMeshNavigationManager::RemoveCustomLink(const MNM::OffMeshLinkID& linkID)
{
    TLinkInfoMap::iterator linkIt = m_links.find(linkID);
    if (linkIt != m_links.end())
    {
        SLinkInfo& linkInfo = linkIt->second;

        // Select the corresponding off-mesh navigation object
        assert(m_offMeshMap.validate(linkInfo.meshID));
        MNM::OffMeshNavigation& offMeshNavigation = m_offMeshMap[linkInfo.meshID];
        NavigationMesh& mesh = gAIEnv.pNavigationSystem->GetMesh(linkInfo.meshID);

        // Remove link
        // NOTE: There should only ever be one element in the link array
        offMeshNavigation.RemoveLink(mesh, linkInfo.triangleID, linkID);

        gAIEnv.pNavigationSystem->RemoveAllIslandConnectionsForObject(linkInfo.meshID, linkID);

        // Remove cached data
        m_links.erase(linkIt);
    }
}

void OffMeshNavigationManager::ProcessQueuedRequests()
{
    if (!m_operations.empty())
    {
        Operations::iterator it = m_operations.begin();
        Operations::iterator end = m_operations.end();
        for (; it != end; ++it)
        {
            switch (it->operationType)
            {
            case MNM::eOffMeshOperationType_Add:
                if (AddCustomLink(it->meshId, *(it->pLinkData), it->linkId, it->dataExists, it->trimExcess))
                {
                    if (it->callback)
                    {
                        it->callback(it->linkId);
                    }
                }
                break;
            case MNM::eOffMeshOperationType_Remove:
                NotifyAllListenerAboutLinkDeletion(it->linkId);
                RemoveCustomLink(it->linkId);
                break;
            default:
                assert(0);
            }
        }

        stl::free_container(m_operations);
    }
}

void OffMeshNavigationManager::NotifyAllListenerAboutLinkDeletion(const MNM::OffMeshLinkID& linkID)
{
    for (Listeners::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
    {
        notifier->OnOffMeshLinkGoingToBeRemoved(linkID);
    }
}

MNM::OffMeshLink* OffMeshNavigationManager::GetOffMeshLink(const MNM::OffMeshLinkID& linkID)
{
    TLinkInfoMap::iterator linkIt = m_links.find(linkID);
    if (linkIt != m_links.end())
    {
        SLinkInfo& linkInfo = linkIt->second;

        // Select the corresponding off-mesh navigation object
        assert(m_offMeshMap.validate(linkInfo.meshID));
        MNM::OffMeshNavigation& offMeshNavigation = m_offMeshMap[linkInfo.meshID];

        return offMeshNavigation.GetObjectLinkInfo(linkID);
    }

    return NULL;
}

void OffMeshNavigationManager::RegisterListener(IOffMeshNavigationListener* pListener, const char* listenerName)
{
    m_listeners.Add(pListener, listenerName);
}

void OffMeshNavigationManager::UnregisterListener(IOffMeshNavigationListener* pListener)
{
    m_listeners.Remove(pListener);
}

void OffMeshNavigationManager::RemoveAllQueuedAdditionRequestForEntity(const EntityId requestOwner)
{
    m_operations.erase(std::remove_if(m_operations.begin(), m_operations.end(), MNM::IsOffMeshAdditionOperationRequestRelatedToEntity(requestOwner)), m_operations.end());
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class CRegisterSOHelper
{
public:
    struct SHelperInfo
    {
        SHelperInfo(const MNM::TriangleID& _triangleID, const Vec3& _position, SmartObjectHelper* _pHelper)
            : triangleID(_triangleID)
            , position(_position)
            , pHelper(_pHelper)
        {
        }

        SmartObjectHelper*              pHelper;
        MNM::TriangleID                 triangleID;
        Vec3                            position;
    };
    typedef std::vector<SHelperInfo> THelpersInfo;

    struct SHelpersLink
    {
        SHelpersLink(int from, int to)
            : fromIndex(from)
            , toIndex(to)
        {
        }

        bool operator==(const SHelpersLink& other) const
        {
            return (fromIndex == other.fromIndex) && (toIndex == other.toIndex);
        }

        int fromIndex;
        int toIndex;
    };
    typedef std::vector<SHelpersLink>   TObjectLinks;

    struct SAgentTypeInfo
    {
        friend class CRegisterSOHelper;

        SAgentTypeInfo(NavigationAgentTypeID _agentTypeID)
            : agentTypeID(_agentTypeID)
        {
        }

        ILINE NavigationAgentTypeID GetAgentTypeID() const { return agentTypeID; }

        bool IsUserClassCompatible(const char* userClass)
        {
            for (size_t i = 0; i < userClasses.size(); ++i)
            {
                if (strcmp(userClasses[i].c_str(), userClass))
                {
                    continue;
                }

                return true;
            }

            return false;
        }

    private:

        ILINE void AddUserClass(const char* userClass) { stl::push_back_unique(userClasses, userClass); }

        NavigationAgentTypeID   agentTypeID;
        std::vector<string>     userClasses;
    };

    CRegisterSOHelper(CSmartObject& smartObject, CSmartObjectClass& smartObjectClass)
        : m_smartObject(smartObject)
        , m_smartObjectClass(smartObjectClass)
        , m_navigationSystem(*gAIEnv.pNavigationSystem)
    {
        GenerateCompatibleTypes();
    }

    bool GetAgentInfo(SAgentTypeInfo& agentInfo)
    {
        if (m_currentElement < m_supportedAgentTypes.size())
        {
            agentInfo = m_supportedAgentTypes[m_currentElement];
            m_currentElement++;
            return true;
        }

        return false;
    }

    bool CanRegister() const
    {
        return !(m_supportedAgentTypes.empty());
    }

private:

    void GenerateCompatibleTypes()
    {
        const CSmartObjectClass::THelperLinks::const_iterator itLinksBegin = m_smartObjectClass.m_vHelperLinks.begin();
        const CSmartObjectClass::THelperLinks::const_iterator itLinksEnd = m_smartObjectClass.m_vHelperLinks.end();

        size_t agentTypeCount = m_navigationSystem.GetAgentTypeCount();
        m_supportedAgentTypes.reserve(agentTypeCount);

        for (size_t agentIdx = 0; agentIdx < agentTypeCount; ++agentIdx)
        {
            const NavigationAgentTypeID agentTypeId = m_navigationSystem.GetAgentTypeID(agentIdx);
            bool addAgentTypeInfo = false;
            SAgentTypeInfo agentTypeInfo(agentTypeId);

            for (CSmartObjectClass::THelperLinks::const_iterator linkIt = itLinksBegin; linkIt != itLinksEnd; ++linkIt)
            {
                const CSmartObjectClass::HelperLink& helperLink = *linkIt;
                const char* userClassName = (helperLink.condition && helperLink.condition->pUserClass) ? helperLink.condition->pUserClass->GetName().c_str() : NULL;

                if (userClassName && m_navigationSystem.AgentTypeSupportSmartObjectUserClass(agentTypeId, userClassName))
                {
                    agentTypeInfo.AddUserClass(userClassName);
                    addAgentTypeInfo = true;
                }
            }

            if (addAgentTypeInfo)
            {
                m_supportedAgentTypes.push_back(agentTypeInfo);
            }
        }

        m_currentElement = 0;
    }

    typedef std::vector<SAgentTypeInfo> TSupportedAgentTypes;
    TSupportedAgentTypes    m_supportedAgentTypes;
    size_t                  m_currentElement;

    CSmartObject&       m_smartObject;
    CSmartObjectClass&  m_smartObjectClass;
    NavigationSystem&   m_navigationSystem;
};

void OffMeshNavigationManager::RegisterSmartObject(CSmartObject* pSmartObject, CSmartObjectClass* pSmartObjectClass)
{
    assert(pSmartObject && pSmartObjectClass);

    if (!CanRegisterObject() || pSmartObjectClass->IsSmartObjectUser())
    {
        return;
    }

    //Filter out non-compatible smart-objects first...
    CRegisterSOHelper registerSOHelper(*pSmartObject, *pSmartObjectClass);
    if (!registerSOHelper.CanRegister())
    {
        return;
    }

    //////////////////////////////////////////////////////////////////////////
    /// If this smart object class is already associated with this object, unregister
    /// all related links to ensure we don't duplicate.
    if (ObjectRegistered(pSmartObject->GetEntityId(), pSmartObjectClass->GetName()))
    {
        UnregisterSmartObject(pSmartObject, pSmartObjectClass->GetName());
    }

    //////////////////////////////////////////////////////////////////////////
    /// Register the object, no matter what, so we keep track of it
    TRegisteredObjects::iterator registeredObjectIt = m_registeredObjects.find(pSmartObject->GetEntityId());
    if (registeredObjectIt == m_registeredObjects.end())
    {
        std::pair<TRegisteredObjects::iterator, bool> newObjectIt = m_registeredObjects.insert(TRegisteredObjects::value_type(pSmartObject->GetEntityId(), TSOClassInfo()));
        assert(newObjectIt.second);

        registeredObjectIt = newObjectIt.first;
    }

    //////////////////////////////////////////////////////////////////////////
    /// Iterate through all compatible agent types for the smart object
    CRegisterSOHelper::SAgentTypeInfo agentTypeInfo(NavigationAgentTypeID(0));
    std::vector<SmartObjectHelper*> validHelpers;

    while (registerSOHelper.GetAgentInfo(agentTypeInfo))
    {
        NavigationAgentTypeID agentTypeID = agentTypeInfo.GetAgentTypeID();
        NavigationMeshID targetMeshID;

        validHelpers.clear();
        validHelpers.reserve(pSmartObjectClass->m_vHelperLinks.size());

        std::vector<SmartObjectHelper*> validHelpers;

        //////////////////////////////////////////////////////////////////////////
        /// Find out to which mesh the object should belong
        for (CSmartObjectClass::SetHelpers::iterator itHelpersEnd = pSmartObjectClass->m_setNavHelpers.end(),
             itHelpers = pSmartObjectClass->m_setNavHelpers.begin(); itHelpers != itHelpersEnd; ++itHelpers)
        {
            SmartObjectHelper* pHelper = *itHelpers;

            // Calculate position
            const Vec3 helperPosition = pSmartObject->GetHelperPos(pHelper);

            NavigationMeshID meshID = gAIEnv.pNavigationSystem->GetEnclosingMeshID(agentTypeID, helperPosition);

            if (targetMeshID && (targetMeshID != meshID))
            {
                //Bail out, not all helpers are connected to the same mesh
                return;
            }

            targetMeshID = meshID;
            validHelpers.push_back(pHelper);
        }

        //////////////////////////////////////////////////////////////////////////
        /// Object belongs to a mesh; Create a new link
        if (targetMeshID)
        {
            // Grab the begin and end links of the smart object class
            const CSmartObjectClass::THelperLinks::const_iterator itLinksBegin = pSmartObjectClass->m_vHelperLinks.begin();
            const CSmartObjectClass::THelperLinks::const_iterator itLinksEnd = pSmartObjectClass->m_vHelperLinks.end();

            // Cache of registered object links to prevent duplication during iteration
            CRegisterSOHelper::TObjectLinks registeredLinks;
            registeredLinks.reserve(pSmartObjectClass->m_vHelperLinks.size());

            // Grab the link ID list
            // This is the link ID's generated for this smart object class on this smart object and are used as keys into m_links
            uint32 smartObjectClassNameCrC = CCrc32::ComputeLowercase(pSmartObjectClass->GetName().c_str());
            OffMeshLinkIDList& linkIDList = registeredObjectIt->second[smartObjectClassNameCrC];

            // Iterate over each helper link, adding their start/end points as off-mesh links if compatible with the
            // chosen user class.
            for (CSmartObjectClass::THelperLinks::const_iterator linkIt = itLinksBegin; linkIt != itLinksEnd; ++linkIt)
            {
                const CSmartObjectClass::HelperLink& helperLink = *linkIt;
                const char* userClassName = (helperLink.condition && helperLink.condition->pUserClass) ? helperLink.condition->pUserClass->GetName().c_str() : NULL;

                if (!userClassName || !agentTypeInfo.IsUserClassCompatible(userClassName))
                {
                    continue;  // Invalid smart object class name or not compatible with user smart object class
                }
                // Determine to/from helper indexes of this helper link
                int helperIndex = 0;
                int fromIndex = -1;
                int toIndex = -1;
                CSmartObjectClass::SetHelpers& helpers = pSmartObjectClass->m_setNavHelpers;
                CSmartObjectClass::SetHelpers::iterator itHelpersEnd = helpers.end();
                for (CSmartObjectClass::SetHelpers::iterator itHelpers = helpers.begin(); itHelpers != itHelpersEnd; ++itHelpers, ++helperIndex)
                {
                    if (linkIt->from == *itHelpers)
                    {
                        fromIndex = helperIndex;
                    }
                    else if (linkIt->to == *itHelpers)
                    {
                        toIndex = helperIndex;
                    }
                }

                // If both helper indexes are valid, attempt to register link
                if ((fromIndex != -1) && (toIndex != -1))
                {
                    const bool alreadyRegistered = stl::find(registeredLinks, CRegisterSOHelper::SHelpersLink(fromIndex, toIndex));
                    if (!alreadyRegistered)
                    {
                        const EntityId smartObjectEntityId = pSmartObject->GetEntityId();
                        // Create a smart object off-mesh link data
                        MNM::OffMeshLinkPtr linkData(new OffMeshLink_SmartObject(smartObjectEntityId, pSmartObject, pSmartObjectClass, validHelpers[fromIndex], validHelpers[toIndex]));

                        // Register the new link with the off-mesh navigation system
                        MNM::OffMeshLinkID linkID = MNM::Constants::eOffMeshLinks_InvalidOffMeshLinkID;

                        MNM::LinkAdditionRequest request(smartObjectEntityId, targetMeshID, linkData, linkID);
                        request.SetCallback(functor(linkIDList, &OffMeshNavigationManager::OffMeshLinkIDList::OnLinkAddedSuccesfullyForSmartObject));
                        QueueCustomLinkAddition(request);

                        // Prevent duplicate registration
                        registeredLinks.push_back(CRegisterSOHelper::SHelpersLink(fromIndex, toIndex));
                    }
                }
            }
        }
    }
}

void OffMeshNavigationManager::UnregisterSmartObjectForAllClasses(CSmartObject* pSmartObject)
{
    CSmartObjectClasses& classes = pSmartObject->GetClasses();
    CSmartObjectClasses::iterator it, itEnd = classes.end();

    for (it = classes.begin(); it != itEnd; ++it)
    {
        UnregisterSmartObject(pSmartObject, (*it)->GetName());
    }
}

void OffMeshNavigationManager::UnregisterSmartObject(CSmartObject* pSmartObject, const string& smartObjectClassName)
{
    assert(pSmartObject);

    // Attempt to find the object in the registered list
    const EntityId objectId = pSmartObject->GetEntityId();
    TRegisteredObjects::iterator objectIt = m_registeredObjects.find(objectId);

    if (objectIt != m_registeredObjects.end())
    {
        uint32 smartObjectClassNameCrC = CCrc32::ComputeLowercase(smartObjectClassName.c_str());
        TSOClassInfo::iterator classIt = objectIt->second.find(smartObjectClassNameCrC);
        if (classIt != objectIt->second.end())
        {
            // Remove associated links
            const OffMeshLinkIDList::TLinkIDList& linkList = classIt->second.GetLinkIDList();
            for (int i = 0; i < linkList.size(); ++i)
            {
                MNM::LinkRemovalRequest request(objectId, linkList[i]);
                QueueCustomLinkRemoval(request);
            }

            // Remove object from registration
            objectIt->second.erase(classIt);
        }

        // If there aren't any classes remaining on this registered object, remove it.
        if (objectIt->second.size() == 0)
        {
            RemoveAllQueuedAdditionRequestForEntity(objectId);
            m_registeredObjects.erase(objectIt);
        }
    }
}

void OffMeshNavigationManager::RefreshConnections(const NavigationMeshID meshID, const MNM::TileID tileID)
{
    if (m_offMeshMap.validate(meshID))
    {
        MNM::OffMeshNavigation& offMeshNavigation = m_offMeshMap[meshID];

        //////////////////////////////////////////////////////////////////////////
        // Invalidate links for the tile.
        // Because the tile has been regenerated, all off-mesh links are invalid now
        offMeshNavigation.InvalidateLinks(tileID);

        //////////////////////////////////////////////////////////////////////////
        /// Refresh all links tied to this mesh and tile
        /// NOTE: This only re-adds them to the Tile with the cached data.
        TLinkInfoMap::iterator iter;
        for (iter = m_links.begin(); iter != m_links.end(); ++iter)
        {
            // Find out if object is bound to the modified mesh and tile
            SLinkInfo& linkInfo = iter->second;
            if (linkInfo.meshID != meshID)
            {
                continue;  // Mismatched mesh id
            }
            if (tileID != MNM::ComputeTileID(linkInfo.triangleID))
            {
                continue;  // Mismatched tile id
            }
            MNM::LinkAdditionRequest request(linkInfo.offMeshLink->GetEntityIdForOffMeshLink(), meshID, linkInfo.offMeshLink, linkInfo.linkID);
            QueueCustomLinkAddition(request);
        }

        std::vector<EntityId> tempObjectIds;
        tempObjectIds.reserve(m_registeredObjects.size());

        //////////////////////////////////////////////////////////////////////////
        /// Find object's smart classes which were unable to register before to be
        /// registered again because of the change
        for (TRegisteredObjects::const_iterator objectIt = m_registeredObjects.begin(); objectIt != m_registeredObjects.end(); ++objectIt)
        {
            const TSOClassInfo classInfos = objectIt->second;
            for (TSOClassInfo::const_iterator classIt = classInfos.begin(); classIt != classInfos.end(); ++classIt)
            {
                const OffMeshLinkIDList::TLinkIDList& linkList = classIt->second.GetLinkIDList();
                if (linkList.empty() == false)
                {
                    continue;
                }

                tempObjectIds.push_back(objectIt->first);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        /// Register again those objects which could have been affected by the change
        for (std::vector<EntityId>::const_iterator objectIt = tempObjectIds.begin(); objectIt != tempObjectIds.end(); ++objectIt)
        {
            CSmartObject* pSmartObject = CSmartObjectManager::GetSmartObject(*objectIt);
            assert(pSmartObject);
            if (pSmartObject)
            {
                CSmartObjectClasses& classes = pSmartObject->GetClasses();
                CSmartObjectClasses::iterator it, itEnd = classes.end();
                for (it = classes.begin(); it != itEnd; ++it)
                {
                    CSmartObjectClass* pSmartObjectClass = *it;

                    RegisterSmartObject(pSmartObject, pSmartObjectClass);
                }
            }
        }
    }
}

void OffMeshNavigationManager::Clear()
{
    m_offMeshMap.clear();
    m_registeredObjects.clear();
    m_links.clear();
    m_objectRegistrationEnabled = false;

    stl::free_container(m_operations);
}

void OffMeshNavigationManager::Enable()
{
    m_objectRegistrationEnabled = true;
}

void OffMeshNavigationManager::OnNavigationMeshCreated(const NavigationMeshID& meshID)
{
    assert(!m_offMeshMap.validate(meshID));

    m_offMeshMap.insert(meshID, MNM::OffMeshNavigation());
}

void OffMeshNavigationManager::OnNavigationMeshDestroyed(const NavigationMeshID& meshID)
{
    assert(m_offMeshMap.validate(meshID));

    m_offMeshMap.erase(meshID);

    //////////////////////////////////////////////////////////////////////////
    /// Remove all existing links for this mesh
    OffMeshLinkIDList::TLinkIDList removedLinkIDs;
    TLinkInfoMap::iterator iter;
    for (iter = m_links.begin(); iter != m_links.end(); )
    {
        // Find out if object is bound to the modified mesh and tile
        SLinkInfo& linkInfo = iter->second;
        if (linkInfo.meshID != meshID)
        {
            ++iter;
            continue;  // Mismatched mesh id
        }

        removedLinkIDs.push_back(linkInfo.linkID);

        m_links.erase(iter++);
    }

    //////////////////////////////////////////////////////////////////////////
    /// Remove smart object references to removed link IDs
    for (TRegisteredObjects::iterator objectIt = m_registeredObjects.begin(); objectIt != m_registeredObjects.end(); ++objectIt)
    {
        for (TSOClassInfo::iterator classInfoIt = objectIt->second.begin(); classInfoIt != objectIt->second.end(); ++classInfoIt)
        {
            OffMeshLinkIDList::TLinkIDList& linkList = classInfoIt->second.GetLinkIDList();
            for (int i = 0; i < removedLinkIDs.size(); ++i)
            {
                stl::find_and_erase(linkList, removedLinkIDs[i]);
            }
        }
    }
}

void OffMeshNavigationManager::OnNavigationLoadedComplete()
{
    //////////////////////////////////////////////////////////////////////////
    //Only after the navigation loaded process is completed off-mesh links can be created
    //Off-mesh links are created after the mesh is loaded, they are not stored within the mesh when exported
    Enable();
}

bool OffMeshNavigationManager::ObjectRegistered(const EntityId objectId, const string& smartObjectClassName) const
{
    uint32 smartObjectClassNameCrC = CCrc32::ComputeLowercase(smartObjectClassName.c_str());
    TRegisteredObjects::const_iterator itFoundObject = m_registeredObjects.find(objectId);
    return (itFoundObject != m_registeredObjects.end() && itFoundObject->second.find(smartObjectClassNameCrC) != itFoundObject->second.end());
}

bool OffMeshNavigationManager::IsObjectLinkedWithNavigationMesh(const EntityId objectId) const
{
    TRegisteredObjects::const_iterator objectIt = m_registeredObjects.find(objectId);

    return (objectIt != m_registeredObjects.end()) ? !objectIt->second.empty() : false;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#if DEBUG_MNM_ENABLED

void OffMeshNavigationManager::UpdateEditorDebugHelpers()
{
    if (!gEnv->IsEditing())
    {
        return;
    }

    static float time = 0.0f;

    time += (gEnv->pTimer->GetFrameTime() * 2.5f);

    IRenderAuxGeom* pRenderAux = gEnv->pRenderer->GetIRenderAuxGeom();

    SAuxGeomRenderFlags oldFlags = pRenderAux->GetRenderFlags();
    SAuxGeomRenderFlags renderFlags(oldFlags);

    renderFlags.SetAlphaBlendMode(e_AlphaBlended);
    renderFlags.SetDepthWriteFlag(e_DepthWriteOff);

    pRenderAux->SetRenderFlags(renderFlags);

    TRegisteredObjects::const_iterator objectIt = m_registeredObjects.begin();
    TRegisteredObjects::const_iterator endIt = m_registeredObjects.end();

    const float alpha = clamp_tpl((1.0f + sinf(time)) * 0.5f, 0.25f, 0.7f);
    const ColorB color (255, 0, 0, (uint8)(alpha * 255));

    for (; objectIt != endIt; ++objectIt)
    {
        if (!objectIt->second.empty())
        {
            continue;
        }

        IEntity* pObjectEntity = gEnv->pEntitySystem->GetEntity(objectIt->first);
        if (pObjectEntity)
        {
            uint32 numSOFound = 0;
            gAIEnv.pSmartObjectManager->GetSOClassTemplateIStatObj(pObjectEntity, NULL, numSOFound);

            ISmartObjectManager::IStatObjPtr* ppStatObjects = NULL;
            if (numSOFound)
            {
                ppStatObjects = new ISmartObjectManager::IStatObjPtr[numSOFound];
                if (gAIEnv.pSmartObjectManager->GetSOClassTemplateIStatObj(pObjectEntity, ppStatObjects, numSOFound) > 0)
                {
                    pRenderAux->DrawAABB(ppStatObjects[0]->GetAABB(), pObjectEntity->GetWorldTM(), true, color, eBBD_Faceted);
                }


                SAFE_DELETE_ARRAY(ppStatObjects);
            }
        }
    }

    pRenderAux->SetRenderFlags(oldFlags);
}

#endif

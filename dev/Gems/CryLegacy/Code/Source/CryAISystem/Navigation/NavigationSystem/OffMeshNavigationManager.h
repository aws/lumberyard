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

#ifndef __OFFMESH_NAVIGATION_MANAGER_H__
#define __OFFMESH_NAVIGATION_MANAGER_H__

#pragma once

#include <INavigationSystem.h>
#include <IOffMeshNavigationManager.h>
#include "../MNM/OffGridLinks.h"

class CSmartObject;
class CSmartObjectClass;

//////////////////////////////////////////////////////////////////////////
/// OffMesh Navigation Manager
/// Keeps track of off-mesh navigation objects associated to each navigation mesh
/// and all the smart-objects which can generate off-mesh connections
///
/// Interfaces also with the Navigation System, to update the right objects
/// when meshes are created, updated or deleted
//////////////////////////////////////////////////////////////////////////
class OffMeshNavigationManager
    : public IOffMeshNavigationManager
{
private:
    struct SLinkInfo
    {
        SLinkInfo() {}
        SLinkInfo(NavigationMeshID _meshId, MNM::TriangleID _triangleID, MNM::OffMeshLinkID _linkID, MNM::OffMeshLinkPtr _offMeshLink)
            : meshID(_meshId)
            , triangleID(_triangleID)
            , linkID(_linkID)
            , offMeshLink(_offMeshLink)
        {}

        bool operator==(const NavigationMeshID _meshID) const
        {
            return (meshID == _meshID);
        }

        NavigationMeshID meshID;
        MNM::TriangleID triangleID;
        MNM::OffMeshLinkID linkID;
        MNM::OffMeshLinkPtr offMeshLink;
    };

    struct SObjectMeshInfo
    {
        SObjectMeshInfo(NavigationMeshID _meshId)
            : meshID(_meshId)
        {
        }

        bool operator==(const NavigationMeshID _meshID)
        {
            return (meshID == _meshID);
        }

        typedef std::vector<MNM::TriangleID>                TTriangleList;
        TTriangleList       triangleList;
        NavigationMeshID    meshID;
    };
    typedef std::vector<SObjectMeshInfo>    TObjectInfo;

public:

    OffMeshNavigationManager(const int offMeshMapSize);

    const MNM::OffMeshNavigation& GetOffMeshNavigationForMesh(const NavigationMeshID& meshID) const;

    //Note: Try to kill this one, outside should never manipulate this
    // Needs to make a lot of code in SmartObjects const correct first...
    MNM::OffMeshNavigation& GetOffMeshNavigationForMesh(const NavigationMeshID& meshID);

    void RegisterSmartObject(CSmartObject* pSmartObject, CSmartObjectClass* pSmartObjectClass);
    void UnregisterSmartObjectForAllClasses(CSmartObject* pSmartObject);

    void RefreshConnections(const NavigationMeshID meshID, const MNM::TileID tileID);
    void Clear();
    void Enable();

    void OnNavigationMeshCreated(const NavigationMeshID& meshID);
    void OnNavigationMeshDestroyed(const NavigationMeshID& meshID);
    void OnNavigationLoadedComplete();

    bool IsObjectLinkedWithNavigationMesh(const EntityId objectId) const;

    // IOffMeshNavigationManager
    virtual void QueueCustomLinkAddition(const MNM::LinkAdditionRequest& request) override;
    virtual void QueueCustomLinkRemoval(const MNM::LinkRemovalRequest& request) override;
    virtual MNM::OffMeshLink* GetOffMeshLink(const MNM::OffMeshLinkID& linkID) override;
    virtual void RegisterListener(IOffMeshNavigationListener* pListener, const char* listenerName) override;
    virtual void UnregisterListener(IOffMeshNavigationListener* pListener) override;
    virtual void RemoveAllQueuedAdditionRequestForEntity(const EntityId requestOwner) override;
    // ~IOffMeshNavigationManager

    void ProcessQueuedRequests();

#if DEBUG_MNM_ENABLED
    void UpdateEditorDebugHelpers();
#else
    void UpdateEditorDebugHelpers() {};
#endif

private:
    bool AddCustomLink(const NavigationMeshID& meshID, MNM::OffMeshLink& linkData, MNM::OffMeshLinkID& linkID, bool dataExists = false, bool trimExcess = false);
    void RemoveCustomLink(const MNM::OffMeshLinkID& linkID);


    void UnregisterSmartObject(CSmartObject* pSmartObject, const string& smartObjectClassName);

    bool ObjectRegistered(const EntityId objectId, const string& smartObjectClassName) const;
    ILINE bool CanRegisterObject() const { return m_objectRegistrationEnabled; };

    void NotifyAllListenerAboutLinkAddition(const MNM::OffMeshLinkID& linkID);
    void NotifyAllListenerAboutLinkDeletion(const MNM::OffMeshLinkID& linkID);

    // For every navigation mesh, there will be always an off-mesh navigation object
    typedef id_map<uint32, MNM::OffMeshNavigation> TOffMeshMap;
    TOffMeshMap m_offMeshMap;

    MNM::OffMeshNavigation m_emptyOffMeshNavigation;

    // Tracking of objects registered
    // All registered objects are stored here, and some additional data
    // like to witch mesh they belong (only one), or bound triangles/tiles
    struct OffMeshLinkIDList
    {
        typedef std::vector<MNM::OffMeshLinkID> TLinkIDList;

        TLinkIDList& GetLinkIDList() { return offMeshLinkIDList; }
        const TLinkIDList& GetLinkIDList() const { return offMeshLinkIDList; }

        void OnLinkAddedSuccesfullyForSmartObject(MNM::OffMeshLinkID linkID)
        {
            assert(linkID != MNM::Constants::eOffMeshLinks_InvalidOffMeshLinkID);
            offMeshLinkIDList.push_back(linkID);
        }

    private:
        TLinkIDList offMeshLinkIDList;
    };

    typedef stl::STLPoolAllocator<std::pair<const uint32, OffMeshLinkIDList>, stl::PoolAllocatorSynchronizationSinglethreaded> TSOClassInfoAllocator;
    typedef std::map<uint32, OffMeshLinkIDList, std::less<uint32>, TSOClassInfoAllocator> TSOClassInfo;
    typedef std::map<EntityId, TSOClassInfo> TRegisteredObjects;
    TRegisteredObjects m_registeredObjects;

    // List of registered links
    typedef std::map<MNM::OffMeshLinkID, SLinkInfo> TLinkInfoMap;
    TLinkInfoMap m_links;

    bool m_objectRegistrationEnabled;

    typedef std::vector<MNM::OffMeshOperationRequestBase> Operations;
    Operations m_operations;

    typedef CListenerSet<IOffMeshNavigationListener*> Listeners;
    Listeners m_listeners;
};

#endif  //__OFFMESH_NAVIGATION_MANAGER_H__
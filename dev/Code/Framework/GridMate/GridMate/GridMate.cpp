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

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define GRIDMATE_CPP_SECTION_1 1
#define GRIDMATE_CPP_SECTION_2 2
#define GRIDMATE_CPP_SECTION_3 3
#define GRIDMATE_CPP_SECTION_4 4
#define GRIDMATE_CPP_SECTION_5 5
#define GRIDMATE_CPP_SECTION_6 6
#define GRIDMATE_CPP_SECTION_7 7
#endif

#ifndef AZ_UNITY_BUILD

#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/std/hash.h>

#include <GridMate/GridMate.h>
#include <GridMate/GridMateService.h>
#include <GridMate/GridMateEventsBus.h>
#include <GridMate/Version.h>

#include <GridMate/Achievements/AchievementMgr.h>
#include <GridMate/Leaderboard/LeaderboardService.h>
#include <GridMate/Storage/GridStorageService.h>

#ifndef GRIDMATE_FOR_TOOLS
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION GRIDMATE_CPP_SECTION_1
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/GridMate_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/GridMate_cpp_provo.inl"
    #endif
#   endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION GRIDMATE_CPP_SECTION_2
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/GridMate_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/GridMate_cpp_provo.inl"
    #endif
#   endif
#endif

namespace GridMate
{
    class GridMateImpl
        : public IGridMate
    {
    private:
        struct ServiceInfo
        {
            GridMateService* m_service;
            GridMateServiceId m_serviceId;
            bool m_isOwnService;
        };

        typedef AZStd::vector<ServiceInfo, GridMateStdAlloc> ServiceTable;

    public:
        AZ_CLASS_ALLOCATOR(GridMateImpl, GridMateAllocator, 0);

        GridMateImpl(const GridMateDesc& desc);
        virtual ~GridMateImpl();

        void Update() override;

        EndianType GetDefaultEndianType() const override { return m_endianType; }

        //////////////////////////////////////////////////////////////////////////
        // Requires on-line service to be started
        /*
        * Leaderboard service
        */
        bool StartLeaderboardService(ServiceType type) override;
        bool StartLeaderboardServiceCustom(LeaderboardService* userService) override;
        void StopLeaderboardService() override;
        bool IsLeaderboardServiceStarted() const override { return m_lbService != nullptr; }
        LeaderboardService* GetLeaderboardService() override { return m_lbService; }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Requires on-line service to be started
        bool StartAchievementService(ServiceType type, const AchievementServiceDesc& desc) override;
        bool StartAchievementServiceCustom(AchievementMgr* userService) override;
        void StopAchievementService() override;
        bool IsAchievementServiceStarted() const override { return m_achievementMgr != nullptr; }
        AchievementMgr* GetAchievementService() override { return m_achievementMgr; }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        bool StartStorageService(ServiceType type, const GridStorageServiceDesc& desc) override;
        bool StartStorageServiceCustom(GridStorageService* userService) override;
        void StopStorageService() override;
        bool IsStorageServiceStarted() const override { return m_storageService != nullptr; }
        GridStorageService* GetStorageService() override { return m_storageService; }
        //////////////////////////////////////////////////////////////////////////        

        void RegisterService(GridMateServiceId id, GridMateService* service, bool delegateOwnership = false) override;
        void UnregisterService(GridMateServiceId id) override;
        bool HasService(GridMateServiceId id) override;
        GridMateService* GetServiceById(GridMateServiceId id) override;

        EndianType m_endianType;

        LeaderboardService* m_lbService;
        bool m_isCustomLbService;

        AchievementMgr* m_achievementMgr;
        bool m_isCustomAchievementMgr;

        GridStorageService* m_storageService;
        bool m_isCustomStorageService;

        ServiceTable m_services;

        struct StaticInfo
        {
            StaticInfo()
                : m_numGridMates(0)
                , m_gridMateAllocatorRefCount(0)
            { }

            int m_numGridMates;
            int m_gridMateAllocatorRefCount;
        };
        static StaticInfo   s_info;
    };
}

GridMate::GridMateImpl::StaticInfo GridMate::GridMateImpl::s_info;

using namespace GridMate;

//=========================================================================
// GridMateCreate
//=========================================================================
IGridMate* GridMate::GridMateCreate(const GridMateDesc& desc)
{
    // Memory
    if (AZ::AllocatorInstance<GridMateAllocator>::IsReady())
    {
        AZ_TracePrintf("GridMate", "GridMate Allocator has already started! Ignoring current allocator descriptor!\n");
        if (GridMateImpl::s_info.m_numGridMates == 0) // add ref count if we did not start it at all
        {
            GridMateImpl::s_info.m_gridMateAllocatorRefCount = 1;
        }
    }
    else
    {
        AZ::AllocatorInstance<GridMateAllocator>::Create(desc.m_allocatorDesc);
    }

    GridMateImpl::s_info.m_numGridMates++;
    GridMateImpl::s_info.m_gridMateAllocatorRefCount++;

    GridMateImpl* impl = aznew GridMateImpl(desc);
    EBUS_EVENT_ID(impl, GridMateEventsBus, OnGridMateInitialized, impl);
    return impl;
}

//=========================================================================
// GridMateCreate
//=========================================================================
void GridMate::GridMateDestroy(IGridMate* gridMate)
{
    AZ_Assert(gridMate != nullptr, "Invalid GridMate interface pointer!");
    EBUS_EVENT_ID(gridMate, GridMateEventsBus, OnGridMateShutdown, gridMate);

    delete gridMate;
    GridMateImpl::s_info.m_numGridMates--;
    GridMateImpl::s_info.m_gridMateAllocatorRefCount--;

    if (GridMateImpl::s_info.m_gridMateAllocatorRefCount == 0)
    {
        AZ::AllocatorInstance<GridMateAllocator>::Destroy();
    }

    if (GridMateImpl::s_info.m_numGridMates == 0)
    {
        GridMateImpl::s_info.m_gridMateAllocatorRefCount = 0;
    }
}

//=========================================================================
// GridMateImpl
//=========================================================================
GridMateImpl::GridMateImpl(const GridMateDesc& desc)
{
    m_endianType = desc.m_endianType;

    m_lbService = nullptr;
    m_isCustomLbService = false;

    m_achievementMgr = nullptr;
    m_isCustomAchievementMgr = false;

    m_storageService = nullptr;
    m_isCustomStorageService = false;
}

//=========================================================================
// ~GridMateImpl
//=========================================================================
GridMateImpl::~GridMateImpl()
{
    if (m_lbService)
    {
        StopLeaderboardService();
    }

    if (m_achievementMgr)
    {
        StopAchievementService();
    }

    if (m_storageService)
    {
        StopStorageService();
    }

    while (!m_services.empty())
    {
        ServiceInfo registeredService = m_services.back();
        m_services.pop_back();
        registeredService.m_service->OnServiceUnregistered(this);
        if (registeredService.m_isOwnService)
        {
            delete registeredService.m_service;
        }
    }
}

void GridMateImpl::RegisterService(GridMateServiceId id, GridMateService* service, bool delegateOwnership)
{
    AZ_Assert(service, "Invalid service");

    GridMateService* duplicate = GetServiceById(id);
    AZ_Assert(!duplicate, "Trying to register the same GridMate service id twice.");
    if (!duplicate)
    {
        service->OnServiceRegistered(this);
        m_services.push_back();
        ServiceInfo& serviceInfo = m_services.back();        
        serviceInfo.m_isOwnService = delegateOwnership;
        serviceInfo.m_service = service;
        serviceInfo.m_serviceId = id;
        EBUS_EVENT_ID(this, GridMateEventsBus, OnGridMateServiceAdded, this, service);
    }
    else
    {        
        if (delegateOwnership)
        {
            delete service;
        }
    }
}

void GridMateImpl::UnregisterService(GridMateServiceId id)
{
    for (auto iter = m_services.begin(); iter != m_services.end(); ++iter)
    {
        if (iter->m_serviceId == id)
        {
            ServiceInfo serviceInfo = *iter;
            m_services.erase(iter);
            serviceInfo.m_service->OnServiceUnregistered(this);
            if (serviceInfo.m_isOwnService)
            {
                delete serviceInfo.m_service;
            }
            return;
        }
    }
    AZ_Error("GridMate", false, "Trying to stop an unregistered session service.");
}

bool GridMateImpl::HasService(GridMateServiceId id)
{
    GridMateService* service = GetServiceById(id);
    return service != nullptr;
}

GridMateService* GridMateImpl::GetServiceById(GridMateServiceId id)
{
    for (auto iter = m_services.begin(); iter != m_services.end(); ++iter)
    {
        if (id == iter->m_serviceId)
        {
            return iter->m_service;
        }
    }
    return nullptr;
}

//=========================================================================
// Update
//=========================================================================
void
GridMateImpl::Update()
{
    for (auto serviceIter = m_services.begin(); serviceIter != m_services.end(); ++serviceIter)
    {
        serviceIter->m_service->OnGridMateUpdate(this);
    }

    if (m_lbService)
    {
        m_lbService->Update();
    }
    if (m_achievementMgr)
    {
        m_achievementMgr->Update();
    }
    if (m_storageService)
    {
        m_storageService->Update();
    }

    EBUS_EVENT_ID(this, GridMateEventsBus, OnGridMateUpdate, this);

}

//=========================================================================
// StartLeaderboardService
//=========================================================================
bool GridMateImpl::StartLeaderboardService(ServiceType type)
{
    AZ_Assert(m_lbService == nullptr, "Leaderboard service already started!");
    if (m_lbService != nullptr)
    {
        return false;
    }

    m_isCustomLbService = false;

    const char* serviceName = "Unknown";
    (void)serviceName;
    switch (type)
    {
    case ST_LAN:
        AZ_Assert(false, "Leaderboard service is not available for ST_LAN!");
        break;

#ifndef GRIDMATE_FOR_TOOLS
    case ST_XLIVE: // ACCEPTED_USE
    {
        serviceName = "  XLive"; // ACCEPTED_USE
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION GRIDMATE_CPP_SECTION_3
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/GridMate_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/GridMate_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
        AZ_Assert(false, "Xbox Live leaderboard service is available on XBone platform only!"); // ACCEPTED_USE
#endif
        break;
    }

    case ST_PSN: // ACCEPTED_USE
        serviceName = "  PSN"; // ACCEPTED_USE
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION GRIDMATE_CPP_SECTION_4
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/GridMate_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/GridMate_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
        AZ_Assert(false, "PSN leaderboard service is available on PS4 platform only!"); // ACCEPTED_USE
#endif
        break;
#endif // GRIDMATE_FOR_TOOLS

    default:
        AZ_Assert(false, "ServiceType 0x%x is not supported!", static_cast<int>(type));
        break;
    }

    if (m_lbService == nullptr)
    {
        return false;
    }

    m_lbService->Init(this);

    AZ_TracePrintf("GridMate", "\n");
    AZ_TracePrintf("GridMate", "================================================\n");
    AZ_TracePrintf("GridMate", "= GridMate Leaderboard Service %s started!     =\n", serviceName);
    AZ_TracePrintf("GridMate", "================================================\n\n");
    return true;
}

//=========================================================================
// StartLeaderboardServiceCustom
//=========================================================================
bool GridMateImpl::StartLeaderboardServiceCustom(GridMate::LeaderboardService* userService)
{
    AZ_Assert(m_lbService == nullptr, "Leaderboard service already started!");
    if (m_lbService != nullptr)
    {
        return false;
    }

    m_isCustomLbService = true;
    m_lbService = userService;

    if (m_lbService == nullptr)
    {
        return false;
    }

    m_lbService->Init(this);

    AZ_TracePrintf("GridMate", "\n");
    AZ_TracePrintf("GridMate", "=================================================\n");
    AZ_TracePrintf("GridMate", "= GridMate Custom/User Leaderboard Service started! =\n");
    AZ_TracePrintf("GridMate", "=================================================\n\n");

    return true;
}

//=========================================================================
// StopLeaderboardService
//=========================================================================
void GridMateImpl::StopLeaderboardService()
{
    if (m_lbService)
    {
        m_lbService->Shutdown();
        if (!m_isCustomLbService)
        {
            delete m_lbService;
        }
        m_lbService = nullptr;
        m_isCustomLbService = false;
    }

    AZ_TracePrintf("GridMate", "\n");
    AZ_TracePrintf("GridMate", "================================================\n");
    AZ_TracePrintf("GridMate", "= GridMate Leaderboard Service stopped!        =\n");
    AZ_TracePrintf("GridMate", "================================================\n\n");
}

//=========================================================================
// StartAchievementService
//=========================================================================
bool GridMateImpl::StartAchievementService(ServiceType type, const AchievementServiceDesc& desc)
{
    (void)desc;
    AZ_Assert(m_achievementMgr == nullptr, "Achievement service already started!");
    if (m_achievementMgr != nullptr)
    {
        return false;
    }

    m_isCustomAchievementMgr = false;

    const char* serviceName = "Unknown";
    (void)serviceName;
    switch (type)
    {
    case ST_LAN:
        AZ_Assert(false, "Achievement service is not available for ST_LAN!");
        break;

#ifndef GRIDMATE_FOR_TOOLS
    case ST_XLIVE: // ACCEPTED_USE
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION GRIDMATE_CPP_SECTION_5
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/GridMate_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/GridMate_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
        AZ_Assert(false, "Xbox live achievement service is available on XBone platform only!"); // ACCEPTED_USE
#endif
        break;

    case ST_PSN: // ACCEPTED_USE
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION GRIDMATE_CPP_SECTION_6
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/GridMate_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/GridMate_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
        AZ_Assert(false, "PSN trophy service is available on PS4 platform only!"); // ACCEPTED_USE
#endif
        break;
#endif // GRIDMATE_FOR_TOOLS

    default:
        AZ_Assert(false, "ServiceType 0x%x is not supported!", static_cast<int>(type));
        break;
    }

    if (m_achievementMgr == nullptr)
    {
        return false;
    }

    m_achievementMgr->Init(this);

    AZ_TracePrintf("GridMate", "\n");
    AZ_TracePrintf("GridMate", "================================================\n");
    AZ_TracePrintf("GridMate", "= GridMate Achievement Service %s started!     =\n", serviceName);
    AZ_TracePrintf("GridMate", "================================================\n\n");
    return true;
}

//=========================================================================
// StartAchievementServiceCustom
//=========================================================================
bool GridMateImpl::StartAchievementServiceCustom(AchievementMgr* userService)
{
    AZ_Assert(m_achievementMgr == nullptr, "Achievement service already started!");
    if (m_achievementMgr != nullptr)
    {
        return false;
    }

    m_isCustomAchievementMgr = true;
    m_achievementMgr = userService;

    if (m_achievementMgr == nullptr)
    {
        return false;
    }
    m_achievementMgr->Init(this);

    AZ_TracePrintf("GridMate", "\n");
    AZ_TracePrintf("GridMate", "=================================================\n");
    AZ_TracePrintf("GridMate", "= GridMate Custom/User Achievement Service started! =\n");
    AZ_TracePrintf("GridMate", "=================================================\n\n");

    return true;
}

//=========================================================================
// StopAchievementService
//=========================================================================
void GridMateImpl::StopAchievementService()
{
    if (m_achievementMgr)
    {
        m_achievementMgr->Shutdown();
        if (!m_isCustomAchievementMgr)
        {
            delete m_achievementMgr;
        }
        m_achievementMgr = nullptr;
        m_isCustomAchievementMgr = false;
    }

    AZ_TracePrintf("GridMate", "\n");
    AZ_TracePrintf("GridMate", "================================================\n");
    AZ_TracePrintf("GridMate", "= GridMate Achievement Service stopped!        =\n");
    AZ_TracePrintf("GridMate", "================================================\n\n");
}

//=========================================================================
// StartStorageService
//=========================================================================
bool GridMateImpl::StartStorageService(ServiceType type, const GridStorageServiceDesc& desc)
{
    (void)desc;
    AZ_Assert(m_storageService == nullptr, "Storage service already started!");
    if (m_storageService != nullptr)
    {
        return false;
    }

    m_isCustomStorageService = false;

    const char* serviceName = "Unknown";
    (void)serviceName;
    switch (type)
    {
    case ST_LAN:
        AZ_Assert(false, "Storage service is not available for ST_LAN!");
        break;
#ifndef GRIDMATE_FOR_TOOLS
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION GRIDMATE_CPP_SECTION_7
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/GridMate_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/GridMate_cpp_provo.inl"
    #endif
#endif
#endif // GRIDMATE_FOR_TOOLS
    default:
        AZ_Assert(false, "ServiceType 0x%x is not supported!", static_cast<int>(type));
        break;
    }

    if (m_storageService == nullptr)
    {
        return false;
    }

    m_storageService->Init(this);

    AZ_TracePrintf("GridMate", "\n");
    AZ_TracePrintf("GridMate", "================================================\n");
    AZ_TracePrintf("GridMate", "= GridMate Storage Service %s started!         =\n", serviceName);
    AZ_TracePrintf("GridMate", "================================================\n\n");
    return true;
}

//=========================================================================
// StartStorageServiceCustom
//=========================================================================
bool GridMateImpl::StartStorageServiceCustom(GridStorageService* userService)
{
    AZ_Assert(m_storageService == nullptr, "Storage service already started!");
    if (m_storageService != nullptr)
    {
        return false;
    }

    m_isCustomStorageService = true;
    m_storageService = userService;

    if (m_storageService == nullptr)
    {
        return false;
    }
    m_storageService->Init(this);

    AZ_TracePrintf("GridMate", "\n");
    AZ_TracePrintf("GridMate", "=================================================\n");
    AZ_TracePrintf("GridMate", "= GridMate Custom/User Storage Service started! =\n");
    AZ_TracePrintf("GridMate", "=================================================\n\n");

    return true;
}

//=========================================================================
// StopStorageService
//=========================================================================
void GridMateImpl::StopStorageService()
{
    if (m_storageService)
    {
        m_storageService->Shutdown();
        if (!m_isCustomStorageService)
        {
            delete m_storageService;
        }
        m_storageService = nullptr;
        m_isCustomStorageService = false;
    }

    AZ_TracePrintf("GridMate", "\n");
    AZ_TracePrintf("GridMate", "================================================\n");
    AZ_TracePrintf("GridMate", "= GridMate Storage Service stopped!            =\n");
    AZ_TracePrintf("GridMate", "================================================\n\n");
}

/**
 * Windows platform-specific net modules
 */
#if AZ_TRAIT_OS_USE_WINDOWS_SOCKETS
#   pragma comment(lib,"WS2_32.lib")
#endif  // AZ_PLATFORM_WINDOWS

#endif // #ifndef AZ_UNITY_BUILD

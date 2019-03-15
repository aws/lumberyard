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
#ifndef GM_LEADERBOARD_H
#define GM_LEADERBOARD_H

#include <GridMate/EBus.h>
#include <GridMate/Online/UserServiceTypes.h>
#include <GridMate/Containers/unordered_map.h>
#include <GridMate/String/string.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/mutex.h>

#define GMLB_vector(T)              AZStd::vector < T, GridMate::GridMateStdAlloc >
#define GMLB_unordered_map(K, T)    AZStd::unordered_map < K, T, AZStd::hash<K>, AZStd::equal_to<K>, GridMate::GridMateStdAlloc >

namespace GridMate
{
    class IGridMate;
    class Leaderboard;
    class LeaderboardService;

    /*
     * LbTableId
     * On some plaforms, leaderboards are identified by a number, while on Steam they are identified by a string.
     * Care must be taken to ensure that the id matches the settings for each platform and their
     * platform definitions.
     */
    class LbTableId
    {
        int             m_id;
        gridmate_string m_idStr;

    public:
        LbTableId(int id);
        LbTableId(const char* idStr);

        int         ToInt() const;
        const char* ToString() const;
    };

    /*
     * LbFieldType
     */
    class LbFieldType
    {
    public:
        enum StorageType
        {
            LBSTOR_INVALID,
            LBSTOR_INT32,
            LBSTOR_INT64,   // Only supported on deprecated platforms, not on Steam.
            LBSTOR_FLOAT,
            LBSTOR_CONTEXT,
        };

        enum LbFieldColumnId
        {
            LBField_NoInfoType  = -1,
            LBField_NoColumnId  = -1,
        };

        LbFieldType(StorageType stor)
            : m_storageType(stor)
            , m_infoType(LBField_NoInfoType)
            , m_columnId(LBField_NoColumnId) {}
        LbFieldType(StorageType stor, int infoType, int columnId = LBField_NoColumnId)
            : m_storageType(stor)
            , m_infoType(infoType)
            , m_columnId(columnId) {}
        LbFieldType(StorageType stor, const char* dataSource)
            : m_storageType(stor)
            , m_infoType(LBField_NoInfoType)
            , m_columnId(LBField_NoColumnId)
            , m_dataSource(dataSource) {}

        bool operator==(const LbFieldType& rhs) const { return m_storageType == rhs.m_storageType && m_infoType == rhs.m_infoType && m_columnId == rhs.m_columnId; }
        bool operator!=(const LbFieldType& rhs) const { return !(*this == rhs); }

        StorageType     m_storageType;      // Specifies C++ data type for this field
        int             m_infoType;         // OBSOLETE PLATFORM ONLY?
        int             m_columnId;         // OBSOLETE PLATFORM ONLY?
#if defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/LeaderboardService_h_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/LeaderboardService_h_provo.inl"
    #endif
#endif
        gridmate_string m_dataSource;
    };

    /*
     * LbTableFormat
     * Passed-in during leaderboard registration.
     * The first field must be the field the leaderboard is ranked by.
     */
    struct LbTableFormat
        : private GMLB_vector(LbFieldType)
    {
        bool    sortAscending;  // if true, leaderboard will be sorted from lowest to highest

        LbTableFormat()
            : sortAscending(false)  {}

        using GMLB_vector(LbFieldType) ::push_back;
        using GMLB_vector(LbFieldType) ::operator[];
        using GMLB_vector(LbFieldType) ::size;
        bool operator==(const LbTableFormat& rhs) const
        {
            return static_cast<const GMLB_vector(LbFieldType)&>(*this) == static_cast<const GMLB_vector(LbFieldType)&>(rhs);
        }
        bool operator!=(const LbTableFormat& rhs) const
        {
            return !(*this == rhs);
        }
    };

    /*
     * LbData
     */
    class LbData
    {
    public:
        static LbData FromInt32(AZ::s32 v)      { LbData data(LbFieldType::LBSTOR_INT32); data.m_v.m_i32 = v; return data; }
        static LbData FromInt64(AZ::s64 v)      { LbData data(LbFieldType::LBSTOR_INT64); data.m_v.m_i64 = v; return data; }
        static LbData FromFloat(float v)        { LbData data(LbFieldType::LBSTOR_FLOAT); data.m_v.m_float = v; return data; }
        static LbData FromContext(AZ::s32 v)    { LbData data(LbFieldType::LBSTOR_CONTEXT); data.m_v.m_i32 = v; return data; }

        LbData(LbFieldType::StorageType valueType = LbFieldType::LBSTOR_INVALID)
            : m_storageType(static_cast<char>(valueType)) {}

        LbFieldType::StorageType GetType() const    { return static_cast<LbFieldType::StorageType>(m_storageType); }

        AZ::s32 ToInt32()   const;
        AZ::s64 ToInt64()   const;
        float   ToFloat()   const;
        AZ::s32 ToContext() const;

    protected:
        union
        {
            AZ::s64 m_i64;
            AZ::s32 m_i32;
            float   m_float;
        }       m_v;
        char    m_storageType;
    };

    /*
     * LbRow
     */
    class LbRow
        : private GMLB_vector(LbData) {
        int             m_rank;
        gridmate_string m_gamerTag;
        const PlayerId* m_playerId;

    public:
        LbRow()
            : m_playerId(nullptr)   {}

        using GMLB_vector(LbData) ::reserve;
        using GMLB_vector(LbData) ::resize;
        using GMLB_vector(LbData) ::push_back;
        using GMLB_vector(LbData) ::clear;
        using GMLB_vector(LbData) ::operator[];
        using GMLB_vector(LbData) ::size;

        void            SetRank(int i)                  { m_rank = i;   }
        int             GetRank() const                 { return m_rank; }
        void            SetGamerTag(const char* tag)    { m_gamerTag = tag; }
        const char*     GetGamerTag() const             { return m_gamerTag.c_str(); }
        void            SetPlayerId(const PlayerId* id) { m_playerId = id; }
        const PlayerId* GetPlayerId() const             { return m_playerId; }
    };

    /*
     * LbView
     */
    class LbView
        : public ReferenceCounted
    {
    public:
        AZ_CLASS_ALLOCATOR(LbView, GridMateAllocator, 0);

        LbView(Leaderboard* leaderboard)
            : m_leaderboard(leaderboard)
            , m_state(LBV_Pending) {}

        const LbRow&    GetRow(AZStd::size_t i) const   { return m_data[i]; }
        AZStd::size_t   GetRowCount() const             { return m_data.size(); }
        bool            IsPending() const               { return m_state == LBV_Pending; }
        bool            IsUnavailable() const           { return m_state == LBV_Unavailable; }
        bool            IsReady() const                 { return m_state == LBV_Ready; }

    protected:
        void            SetUnavailable();
        void            SetReady();

        GMLB_vector(LbRow)  m_data;
        Leaderboard*        m_leaderboard;

        enum LbViewState
        {
            LBV_Pending,
            LBV_Unavailable,
            LBV_Ready,
        };
        AZStd::atomic_int   m_state;
    };

    /*
     * LbViewHandle
     */
    //typedef LbView*   LbViewHandle;
    typedef AZStd::intrusive_ptr<LbView> LbViewHandle;

    /*
     * Leaderboard
     * Provides interfaces for all leaderboard-specific requests
     */
    class Leaderboard
    {
    public:
        Leaderboard(LeaderboardService* service, const LbTableId& id, const LbTableFormat& fmt)
            : m_id(id)
            , m_fmt(fmt)
            , m_service(service)
        {}
        virtual ~Leaderboard()                      {}

        const LbTableId&        GetId() const       { return m_id; }
        const LbTableFormat&    GetFormat() const   { return m_fmt; }

        virtual void            SetStats(ILocalMember* pRequestor, const PlayerId& player, const LbRow& row) = 0;
        virtual LbViewHandle    GetViewByRank(ILocalMember* pRequestor, int first, int nRows) = 0;
        virtual LbViewHandle    GetViewAroundPlayer(ILocalMember* pPlayer, int nRows) = 0;
        virtual LbViewHandle    GetViewOfFriends(ILocalMember* pPlayer) = 0;

        inline LeaderboardService*  GetService() const { return m_service; }

    protected:
        LbTableId       m_id;
        LbTableFormat   m_fmt;
        LeaderboardService* m_service;
    };

    /*
     * LeaderboardService
     * Provides general leaderboard service functionality and the
     * registration/unregistration of specific leaderboards
     */
    class LeaderboardService
    {
        friend class GridMateImpl;

    public:
        LeaderboardService()
            : m_gridMate(nullptr) {}
        virtual ~LeaderboardService();

        virtual void            Init(IGridMate* gridMate) { AZ_Assert(gridMate, "You must provide a valid gridmate instance!"); m_gridMate = gridMate; }
        virtual void            Update();
        virtual void            Shutdown();
        virtual void            FlushStats() = 0;
        virtual Leaderboard*    RegisterLB(const LbTableId& id, const LbTableFormat& fmt) = 0;
        virtual void            UnregisterLB(const LbTableId& id) = 0;
        virtual Leaderboard*    GetLB(const LbTableId& id) = 0;

        inline IGridMate*       GetGridMate() const             { return m_gridMate; }

    protected:
        IGridMate*  m_gridMate;
    };

    //-----------------------------------------------------------------------------
    // LeaderboardCallbacks
    //-----------------------------------------------------------------------------
    class LeaderboardCallbacks
        : public GridMateEBusTraits
    {
    public:
        static const bool EnableEventQueue = true;

        virtual         ~LeaderboardCallbacks() {}
        virtual void    OnViewFinished(const LbViewHandle& hLbView) = 0;
    };
    //-----------------------------------------------------------------------------
    typedef AZ::EBus<LeaderboardCallbacks> LeaderboardBus;
}   // namespace GridMate

#endif  // GM_LEADERBOARD_H

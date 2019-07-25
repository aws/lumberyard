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
/**
 * @file
 * Provides player achievements functionality.
 */
#ifndef GM_ACHIEVEMENTS_H
#define GM_ACHIEVEMENTS_H

#include <GridMate/GridMate.h>
#include <GridMate/Online/UserServiceTypes.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/string/conversions.h>

namespace GridMate
{
    struct AchievementInfo;
    struct AchievementRequestStatus;

    typedef AZStd::basic_string<char, AZStd::char_traits<char>, GridMateStdAlloc > ach_string;

    /*
     * AchievementId
     * On some platforms achievements are identified by a number, while on Steam they are identified by a string.
     * Care must be taken to ensure that the id matches the settings for each platform and their
     * platform definitions.
     */
    class AchievementId
    {
        int         m_id;
        ach_string  m_idStr;

    public:
        AchievementId(int id)
            : m_id(id)    {}
        AchievementId(const char* idStr)    { AZ_Assert(idStr && idStr[0], "Id strings cannot be NULL or empty!"); m_idStr = idStr; }

        int         ToInt() const           { AZ_Assert(!IsString(), "This id is not of number type"); return m_id; }
        bool        IsInt() const           { return !IsString(); }
        const char* ToString() const        { AZ_Assert(IsString(), "This id is not of string type"); return m_idStr.c_str(); }
        bool        IsString() const        { return !m_idStr.empty(); }
        ach_string  ToStringDesc() const    { return IsString() ? m_idStr : static_cast<ach_string>(AZStd::to_string(m_id)); }
    };

    /*
     * AchievementQueryResult
     */
    struct AchievementQueryResult
    {
        enum
        {
            QUERY_PENDING,
            QUERY_DONE,
            QUERY_ERROR
        };

        ach_string          m_name;
        ach_string          m_desc;

        AZStd::atomic_int   m_status;       // This is the status of the query
    };

    //-------------------------------------------------------------------------
    // Notifications (outbound)
    //-------------------------------------------------------------------------
    class AchievementBusTraits
        : public GridMateEBusTraits
    {
    public:
        virtual ~AchievementBusTraits() {}

        virtual void    OnQueryFailed(const ILocalMember* localUser, const AchievementId& id)       = 0;
        virtual void    OnQueryCompleted(const ILocalMember* localUser, const AchievementId& id)    = 0;
        virtual void    OnUnlockFailed(const ILocalMember* localUser, const AchievementId& id)      = 0;
        virtual void    OnUnlockCompleted(const ILocalMember* localUser, const AchievementId& id)   = 0;
    };
    //-------------------------------------------------------------------------
    typedef AZ::EBus<AchievementBusTraits> AchievementBus;
    //-------------------------------------------------------------------------

    /*
     * AchievementServiceDesc
     */
    struct AchievementServiceDesc
    {
        int m_threadCpuId;

        AchievementServiceDesc()
            : m_threadCpuId(0) {}
    };

    /*
     * AchievementMgr
     */
    class AchievementMgr
    {
    public:
        AchievementMgr()
            : m_gridMate(nullptr)  {}
        virtual ~AchievementMgr()   {}

        // Achievement query functions
        virtual int             GetCount(const ILocalMember* user) const = 0;
        virtual AchievementId   GetAchievementId(const ILocalMember* user, int index) const = 0;
        virtual void            GetDetails(const ILocalMember* user, const AchievementId& id, AchievementQueryResult& ret) = 0;
        virtual bool            IsUnlocked(const ILocalMember* user, const AchievementId& id) const = 0;
        virtual bool            IsHidden(const ILocalMember* user, const AchievementId& id) const = 0;
        virtual int             GetRewardValue(const ILocalMember* user, const AchievementId& id) const = 0;
        virtual int             GetUnlockedCount(const ILocalMember* user) const = 0;
        virtual int             GetHiddenCount(const ILocalMember* user) const = 0;

        // Achievement change functions
        virtual void    Unlock(const ILocalMember* user, const AchievementId& id, AZ::u32 percentage) = 0;

        // Mgr functions
        virtual void    Init(IGridMate* gridMate)           { AZ_Assert(gridMate, "You must provide a valid gridmate"); m_gridMate = gridMate; }
        virtual void    Shutdown() = 0;
        virtual void    Update() = 0;
        virtual bool    IsReady() const = 0;
        virtual size_t  GetDiskSpaceRequirements() const    { return 0; }
    protected:
        IGridMate*      m_gridMate;
    };
}   // namespace GridMate

#endif  // GM_ACHIEVEMENTS_H

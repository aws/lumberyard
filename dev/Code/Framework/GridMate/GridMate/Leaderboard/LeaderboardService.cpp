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

#include <GridMate/Leaderboard/LeaderboardService.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/string/conversions.h>

namespace GridMate {
    //-------------------------------------------------------------------------
    // LbTableId
    //-------------------------------------------------------------------------
    LbTableId::LbTableId(int id)
        : m_id(id)
    {
        AZStd::to_string(m_idStr, id);
    }
    //-------------------------------------------------------------------------
    LbTableId::LbTableId(const char* idStr)
    {
        AZ_Assert(idStr && idStr[0], "Id strings cannot be NULL or empty!");
        m_idStr = idStr;
        m_id = static_cast<int>(AZ::Crc32(idStr));
    }
    //-------------------------------------------------------------------------
    int LbTableId::ToInt() const
    {
        return m_id;
    }
    //-------------------------------------------------------------------------
    const char* LbTableId::ToString() const
    {
        return m_idStr.c_str();
    }
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // LbData
    //-------------------------------------------------------------------------
    AZ::s32 LbData::ToInt32() const
    {
        AZ_Assert(m_storageType == (char)LbFieldType::LBSTOR_INT32 || m_storageType == (char)LbFieldType::LBSTOR_CONTEXT, "This LbData is not an int32!");
        return m_v.m_i32;
    }
    //-------------------------------------------------------------------------
    AZ::s64 LbData::ToInt64() const
    {
        AZ_Assert(m_storageType == (char)LbFieldType::LBSTOR_INT64, "This LbData is not an int64!");
        return m_v.m_i64;
    }
    //-------------------------------------------------------------------------
    float LbData::ToFloat() const
    {
        AZ_Assert(m_storageType == (char)LbFieldType::LBSTOR_FLOAT, "This LbData is not a float!");
        return m_v.m_float;
    }
    //-------------------------------------------------------------------------
    AZ::s32 LbData::ToContext() const
    {
        AZ_Assert(m_storageType == (char)LbFieldType::LBSTOR_CONTEXT, "This LbData is not a context!");
        return m_v.m_i32;
    }
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // LbView
    //-------------------------------------------------------------------------
    void LbView::SetUnavailable()
    {
        m_state = LBV_Unavailable;
        EBUS_QUEUE_EVENT_ID(m_leaderboard->GetService()->GetGridMate(), LeaderboardBus, OnViewFinished, LbViewHandle(this));
    }
    //-------------------------------------------------------------------------
    void LbView::SetReady()
    {
        m_state = LBV_Ready;
        EBUS_QUEUE_EVENT_ID(m_leaderboard->GetService()->GetGridMate(), LeaderboardBus, OnViewFinished, LbViewHandle(this));
    }
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    void LeaderboardService::Update()
    {
        LeaderboardBus::ExecuteQueuedEvents();
    }
    //-------------------------------------------------------------------------
    void LeaderboardService::Shutdown()
    {
        LeaderboardBus::ClearQueuedEvents();
    }
    //-------------------------------------------------------------------------
    LeaderboardService::~LeaderboardService()
    {
    }
    //-------------------------------------------------------------------------
}   // namesapce GridMate

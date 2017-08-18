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

// Description : Routine for profiling disk IO


#ifndef CRYINCLUDE_CRYSYSTEM_DISKPROFILER_H
#define CRYINCLUDE_CRYSYSTEM_DISKPROFILER_H
#pragma once

#ifdef USE_DISK_PROFILER

#include <IDiskProfiler.h>
#include <IStreamEngine.h>
#include <CryThread.h>
#include <Cry_Color.h>

//////////////////////////////////////////////////////////////////////////
// Disk Profile main class
//////////////////////////////////////////////////////////////////////////
class CDiskProfiler
    : public IDiskProfiler
{
    friend class CDiskProfileTimer;
public:
    CDiskProfiler(ISystem* pSystem);
    ~CDiskProfiler();
    virtual void Update();                      // perframe update routine
    virtual DiskOperationInfo GetStatistics() const
    {
        return m_outStatistics;
    }
    virtual bool RegisterStatistics(SDiskProfileStatistics* pStatistics);       // main statistics registering function

    virtual void SetTaskType(const uint32 nThreadId, const uint32 nType = 0xffffffff);      // task type registering function

    virtual bool IsEnabled() const;

protected:

    // rendering routine
    void RenderBlock(const float timeStart, const float timeEnd, const ColorB threadColor, const ColorB IOTypeColor);
    void Render();

    typedef std::deque<SDiskProfileStatistics*> Statistics;
    typedef std::map<uint32, ColorB> ThreadColorMap;
    typedef std::map<uint32, EStreamTaskType, std::less<uint32>, stl::STLGlobalAllocator<std::pair<const uint32, EStreamTaskType> > > ThreadTaskTypeMap;

    volatile bool                       m_bEnabled;
    CryCriticalSection          m_csLock;               // MT-safe profiling
    Statistics                          m_statistics;       // main statistics collector
    ThreadColorMap                  m_threadsColorLegend;
    ThreadTaskTypeMap               m_currentThreadTaskType;
    ISystem*                                m_pSystem;

    int                                         m_nHeightOffset;    // offset from bottom of the screen, in pixels

    DiskOperationInfo               m_outStatistics;
public:
    static int profile_disk;
    static int profile_disk_max_items;
    static int profile_disk_max_draw_items;
    static float profile_disk_timeframe;        // max time for profiling timeframe
    static int profile_disk_type_filter;        // filter by task types for profiling
    static int profile_disk_budget;     // filter by task types for profiling
};

#endif

#endif // CRYINCLUDE_CRYSYSTEM_DISKPROFILER_H

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

// Description : Stores global AI statistics and provides display/output methods


#include "CryLegacy_precompiled.h"
#include "StatsManager.h"
#include "DebugDrawContext.h"

#pragma warning (disable: 6313)

// Useful formatting strings
// 30 characters total are allowed for this in the standard view!
// 20 (max) of those are usually used for the short description
const static char* sFmtDefault = "%s: %3.1f";


void CStatsManager::InitDescriptions()
{
    // The list of statistics metadata
    static const SStatMetadata sMetadata[] = {
        { "Enabled AI Actors", eSF_FrameReset | eSF_Integer, 0 },
        { "AI full updates", eSF_FrameReset, 0 },
        { "TPS queries (sync)", eSF_FrameReset, 0 },
        { "TPS queries (async)", eSF_FrameReset, 0 },
    };

    // Make sure that the number of descriptions never varies from the size of the enum
    // That should be enough to avoid descriptions and enums from getting out of step
    PREFAST_SUPPRESS_WARNING(6326) COMPILE_TIME_ASSERT(sizeof(sMetadata) / sizeof(SStatMetadata) == eStat_Last);

    m_sMetadata = sMetadata;
}


CStatsManager::CStatsManager(void)
{
    m_fValues = new float[eStat_Last];

    InitDescriptions();

    Reset(eStatReset_All);
}

CStatsManager::~CStatsManager(void)
{
    delete[] m_fValues;
}

void CStatsManager::Reset(EStatsReset eReset)
{
    // Reset some or all of the stats values
    for (uint32 i = 0; i < eStat_Last; i++)
    {
        switch (eReset)
        {
        case eStatReset_Frame:
            if (!(m_sMetadata[i].flags & eSF_FrameReset))
            {
                break;
            }
        // Otherwise, fall-through
        case eStatReset_All:
            m_fValues[i] = 0.0f;
        }
    }
}

void CStatsManager::Render()
{
    CDebugDrawContext dc;

    static float xPos = 78.0f;
    static float yPos = 45.0f;
    static float yStep = 2.0;

    if (gAIEnv.CVars.StatsDisplayMode == 0)
    {
        return;
    }

    float yOffset = 0.0f;

    const char sTitle[30] = "------- AI Statistics -------";
    dc->TextToScreen(xPos, yPos, sTitle);
    yOffset += yStep;

    for (uint32 i = 0; i < eStat_Last; i++, yOffset += yStep)
    {
        const SStatMetadata& metadata = m_sMetadata[i];
        const char* sFormSpec = metadata.format ? metadata.format : sFmtDefault;
        dc->TextToScreen(xPos, yPos + yOffset, sFormSpec, metadata.description, m_fValues[i]);
    }
}


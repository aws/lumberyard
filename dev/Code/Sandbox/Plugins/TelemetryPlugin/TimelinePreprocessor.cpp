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

#include "stdafx.h"
#include "TimelinePreprocessor.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct STimelineCrawler
{
    void setTimeline(CTelemetryTimeline* tl)
    {
        timeline = tl;
    }

    bool canAdvance() const
    {
        return (position + 1) < timeline->getEventCount();
    }

    int64 getTime()
    {
        if (position < 0 || position >= timeline->getEventCount())
        {
            return -1;
        }
        return timeline->getEvent(position).timeMs;
    }

    int64 getNextTime()
    {
        int pos = position + 1;
        if (pos < 0 || pos >= timeline->getEventCount())
        {
            return -1;
        }
        return timeline->getEvent(pos).timeMs;
    }

    bool advance(int64 time)
    {
        if (getNextTime() > time)
        {
            return false;
        }

        ++position;
        return true;
    }

    bool advance()
    {
        return advance(getNextTime());
    }

    STelemetryEvent& getEvent()
    {
        CRY_ASSERT(position >= 0 && position < timeline->getEventCount());
        return timeline->getEvent((size_t)position);
    }

    STelemetryEvent& getNextEvent()
    {
        CRY_ASSERT(position < timeline->getEventCount() - 1);
        return timeline->getEvent((size_t)position + 1);
    }

    STimelineCrawler()
        : timeline(0)
        , position(-1)
        , processing(true)
        , advanced(false)
    { }

public:
    CTelemetryTimeline* timeline;
    int position;
    bool advanced;
    bool processing;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CTimelinePreprocessor::DeduceEventPositions(CTelemetryTimeline** timelines, size_t numTimelines, size_t posTimeline)
{
    // Setup crawlers
    STimelineCrawler* crawlers = new STimelineCrawler[numTimelines];
    STimelineCrawler& positionCrawler = crawlers[posTimeline];

    for (int i = 0; i < numTimelines; ++i)
    {
        crawlers[i].setTimeline(timelines[i]);
    }


    int advanceable;
    int64 smallest_step;

    while (true)
    {
        advanceable = 0;
        smallest_step = std::numeric_limits<int64>::max();

        // Calculate minimal step
        for (size_t i = 0; i < numTimelines; ++i)
        {
            if (crawlers[i].processing && crawlers[i].canAdvance())
            {
                ++advanceable;
                int64 t = crawlers[i].getNextTime();
                if (t < smallest_step)
                {
                    smallest_step = t;
                }
            }
            else
            {
                crawlers[i].processing = false;
            }
        }


        if (!advanceable)
        {
            break;
        }


        // Advance crawlers
        for (int i = 0; i < numTimelines; ++i)
        {
            if (crawlers[i].processing)
            {
                crawlers[i].advanced = crawlers[i].advance(smallest_step);
            }
        }

        if (positionCrawler.position == -1)
        {
            positionCrawler.advance();
        }

        for (size_t i = 0; i < numTimelines; ++i)
        {
            if (crawlers[i].processing && crawlers[i].advanced && i != posTimeline)
            {
                if (positionCrawler.canAdvance())
                {
                    STelemetryEvent& pos1 = positionCrawler.getEvent();
                    STelemetryEvent& pos2 = positionCrawler.getNextEvent();
                    STelemetryEvent& e = crawlers[i].getEvent();

                    float a = (float)(e.timeMs - pos1.timeMs) / (float)(pos2.timeMs - pos1.timeMs);
                    e.position = LERP(pos1.position, pos2.position, a);
                }
                else
                {
                    crawlers[i].getEvent().position = positionCrawler.getEvent().position;
                }
            }
        }
    }

    delete[] crawlers;

    for (size_t i = 0; i < numTimelines; ++i)
    {
        timelines[i]->UpdateBBox();
    }
}

//////////////////////////////////////////////////////////////////////////

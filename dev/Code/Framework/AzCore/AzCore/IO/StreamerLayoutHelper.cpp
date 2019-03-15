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
#ifndef AZ_UNITY_BUILD

#include <AzCore/IO/StreamerLayoutHelper.h>
#include <AzCore/IO/Device.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/sort.h>

using namespace AZ::IO;

//=========================================================================
// StreamerLayoutHelper
// [12/14/2011]
//=========================================================================
StreamerLayoutHelper::StreamerLayoutHelper(const char* deviceName)
{
    IO::Streamer& streamer = IO::Streamer::Instance();
    m_targetDevice = streamer.FindDevice(deviceName);
    m_currentStreamId = 0;

    BusConnect();

    // we assume there is no inital satate in the streamer, make sure you start the helper before any IO operations.
}

//=========================================================================
// OnRegisterStream
// [12/14/2011]
//=========================================================================
void StreamerLayoutHelper::OnRegisterStream(AZ::IO::GenericStream* stream, OpenMode flags)
{
    IO::Device* device = GetStreamer()->FindDevice(stream->GetFilename());
    if (device != m_targetDevice)
    {
        return;
    }

    bool isWrite = (flags & (IO::OpenMode::ModeWrite | IO::OpenMode::ModeAppend | IO::OpenMode::ModeUpdate)) != IO::OpenMode::Invalid;

    if (isWrite)
    {
        return;           // we care about read streams only
    }
    StreamLayoutMap::pair_iter_bool iter = m_layoutData.insert_key(Uuid::CreateName(stream->GetFilename()));
    StatsLayoutEntry& se = iter.first->second;
    if (iter.second)
    {
        se.m_name = stream->GetFilename();
        se.m_bytesRead = 0;
        se.m_splitFactor = 0;
        se.m_layoutGroup = 0;
        se.m_isLayer0 = false;
    }
}

//=========================================================================
// OnReadComplete
// [12/14/2011]
//=========================================================================
void StreamerLayoutHelper::OnReadComplete(AZ::IO::GenericStream* stream, AZ::u64 bytesTransferred)
{
    IO::Device* device = GetStreamer()->FindDevice(stream->GetFilename());
    if (device != m_targetDevice)
    {
        return;
    }

    AZ_Assert(stream->CanRead(), "If we read/write to the same device, there is no point to optimize the layout!");

    StreamID streamID = Uuid::CreateName(stream->GetFilename());
    if (m_currentStreamId != 0 && m_currentStreamId != streamID)
    {
        StreamLayoutMap::iterator iter = m_layoutData.find(m_currentStreamId);
        AZ_Assert(iter != m_layoutData.end(), "We should have registered this stream!");
        StatsLayoutEntry& se = iter->second;
        TranstionTargetMap::iterator targetIter = se.m_transitions.insert(AZStd::make_pair(streamID, 0)).first;
        targetIter->second++;
    }

    StreamLayoutMap::iterator iter = m_layoutData.find(streamID);
    AZ_Assert(iter != m_layoutData.end(), "We should have registered this stream!");
    StatsLayoutEntry& se = iter->second;
    se.m_bytesRead += bytesTransferred;
    m_currentStreamId = streamID;
}

//=========================================================================
// GenerateOptimalDeviceReadLayout
// [11/15/2011]
//=========================================================================
void StreamerLayoutHelper::LinkUnattachedStream(StatsLayoutEntry* unattachedStream, StatsLayoutEntry* attachedStream)
{
    unattachedStream->m_layoutGroup = attachedStream->m_layoutGroup;
    StatsLayoutEntry* streamForward = attachedStream;
    StatsLayoutEntry* streamBackward = attachedStream;
    while (true)
    {
        if (streamForward->m_layoutNext == NULL)
        {
            unattachedStream->m_layoutPrev = streamForward;
            streamForward->m_layoutNext = unattachedStream;
            break;
        }
        streamForward = streamForward->m_layoutNext;
        if (streamBackward->m_layoutPrev == NULL)
        {
            unattachedStream->m_layoutNext = streamBackward;
            streamBackward->m_layoutPrev = unattachedStream;
            break;
        }
        streamBackward = streamBackward->m_layoutPrev;
    }
}

//=========================================================================
// GenerateOptimalSeekDeviceReadLayout
// [12/14/2011]
//=========================================================================
void StreamerLayoutHelper::GenerateOptimalSeekDeviceReadLayout(AZStd::vector<AZStd::pair<AZStd::string, unsigned int> >& layout)
{
    layout.clear();

    AZStd::lock_guard<StreamerDrillerBus::MutexType> lock(StreamerDrillerBus::GetOrCreateContext().m_contextMutex);

    typedef AZStd::unordered_map<StreamID, SeekLink> SeekLinksMap;
    SeekLinksMap seekLinks;
    // create seek links, the sum of transitions/seek from stream A -> B and B -> A. Which will determine how close files should be.
    for (StreamLayoutMap::iterator iter = m_layoutData.begin(); iter != m_layoutData.end(); ++iter)
    {
        StatsLayoutEntry& le = iter->second;
        le.m_layoutGroup = 0;
        le.m_splitFactor = 0;
        le.m_layoutPrev = le.m_layoutNext = NULL; // reset layout pointers
        StreamID strID1 = iter->first;
        for (TranstionTargetMap::iterator seekToIter = le.m_transitions.begin(); seekToIter != le.m_transitions.end(); ++seekToIter)
        {
            StreamID strID2 = seekToIter->first;
            StreamID linkID = strID1 + strID2;
            SeekLinksMap::pair_iter_bool insertIter = seekLinks.insert_key(linkID);
            if (insertIter.second)
            {
                insertIter.first->second.weight = seekToIter->second;
                insertIter.first->second.stream1 = &iter->second;
                insertIter.first->second.stream2 = &m_layoutData.find(strID2)->second;
            }
            else
            {
                insertIter.first->second.weight += seekToIter->second;
            }
        }
    }

    // sort links on number of transitions/seeks
    AZStd::vector<SeekLink*> sortedLinks;
    sortedLinks.reserve(seekLinks.size());
    for (SeekLinksMap::iterator linkIter = seekLinks.begin(); linkIter != seekLinks.end(); ++linkIter)
    {
        sortedLinks.push_back(&linkIter->second);
    }

    AZStd::sort(sortedLinks.begin(), sortedLinks.end(), &SeekLink::Sort);

    // generate layout
    {
        unsigned int currentSortGroup = 1;

        for (size_t i = 0; i < sortedLinks.size(); ++i)
        {
            SeekLink* link = sortedLinks[i];

            StatsLayoutEntry* stream1 = link->stream1;
            StatsLayoutEntry* stream2 = link->stream2;

            stream1->m_splitFactor += link->weight;
            stream2->m_splitFactor += link->weight;

            if (stream1->m_layoutGroup == 0 && stream2->m_layoutGroup == 0)
            {
                stream1->m_layoutGroup = currentSortGroup++;
                LinkUnattachedStream(stream2, stream1);
                continue;
            }
            if (stream1->m_layoutGroup == 0)
            {
                LinkUnattachedStream(stream1, stream2);
                continue;
            }
            if (stream2->m_layoutGroup == 0)
            {
                LinkUnattachedStream(stream2, stream1);
                continue;
            }
            if (stream1->m_layoutGroup != stream2->m_layoutGroup)   // if we belong to different groups
            {
                // find the shortest link A <-> B or B <-> A
                StatsLayoutEntry* stream1Forward = stream1;
                StatsLayoutEntry* stream1Backward = stream1;
                StatsLayoutEntry* stream2Forward = stream2;
                StatsLayoutEntry* stream2Backward = stream2;

                while (true)
                {
                    if (stream1Forward->m_layoutNext == NULL && stream2Backward->m_layoutPrev == NULL)
                    {
                        stream1Forward->m_layoutNext = stream2Backward;
                        stream2Backward->m_layoutPrev = stream1Forward;

                        StatsLayoutEntry* streamGroupToUpdate = stream2Backward;
                        while (streamGroupToUpdate != NULL)
                        {
                            streamGroupToUpdate->m_layoutGroup = stream1->m_layoutGroup;
                            streamGroupToUpdate = streamGroupToUpdate->m_layoutNext;
                        }
                        break;
                    }
                    if (stream1Backward->m_layoutPrev == NULL && stream2Forward->m_layoutNext == NULL)
                    {
                        stream1Backward->m_layoutPrev = stream2Forward;
                        stream2Forward->m_layoutNext = stream1Backward;

                        StatsLayoutEntry* streamGroupToUpdate = stream2Forward;
                        while (streamGroupToUpdate != NULL)
                        {
                            streamGroupToUpdate->m_layoutGroup = stream1->m_layoutGroup;
                            streamGroupToUpdate = streamGroupToUpdate->m_layoutPrev;
                        }
                        break;
                    }

                    if (stream1Forward->m_layoutNext != NULL)
                    {
                        stream1Forward = stream1Forward->m_layoutNext;
                    }
                    if (stream1Backward->m_layoutPrev != NULL)
                    {
                        stream1Backward = stream1Backward->m_layoutPrev;
                    }
                    if (stream2Forward->m_layoutNext != NULL)
                    {
                        stream2Forward = stream2Forward->m_layoutNext;
                    }
                    if (stream2Backward->m_layoutPrev != NULL)
                    {
                        stream2Backward = stream2Backward->m_layoutPrev;
                    }
                }
            }
        }

        // output to user
        layout.reserve(m_layoutData.size());
        for (StreamLayoutMap::iterator iter = m_layoutData.begin(); iter != m_layoutData.end(); ++iter)
        {
            StatsLayoutEntry& le = iter->second;
            if (le.m_layoutPrev == NULL)  // is head
            {
                StatsLayoutEntry* head = &le;
                while (head != NULL)
                {
                    layout.push_back(AZStd::make_pair(head->m_name, head->m_splitFactor));
                    head = head->m_layoutNext;
                }
            }
        }
        AZ_Assert(m_layoutData.size() == layout.size(), "Layout algorithm error, we should have all the files in the layout!");
    }
}

//=========================================================================
// GenerateOptimalTransferDeviceReadLayout
// [11/15/2011]
//=========================================================================
void
StreamerLayoutHelper::GenerateOptimalTransferDeviceReadLayout(AZStd::vector<AZStd::string>& layer0, AZStd::vector<AZStd::string>& layer1)
{
    layer0.clear();
    layer1.clear();

    AZStd::vector<StatsLayoutEntry*> sortedLayer0, sortedLayer1;
    sortedLayer0.reserve(m_layoutData.size());
    sortedLayer1.reserve(m_layoutData.size());
    for (StreamLayoutMap::iterator iter = m_layoutData.begin(); iter != m_layoutData.end(); ++iter)
    {
        if (iter->second.m_isLayer0)
        {
            sortedLayer0.push_back(&iter->second);
        }
        else
        {
            sortedLayer1.push_back(&iter->second);
        }
    }
    AZStd::sort(sortedLayer0.begin(), sortedLayer0.end(), &StatsLayoutEntry::SortOnReadSize);
    AZStd::sort(sortedLayer1.begin(), sortedLayer1.end(), &StatsLayoutEntry::SortOnReadSize);

    for (size_t i = 0; i < sortedLayer0.size(); ++i)
    {
        layer0.push_back(sortedLayer0[i]->m_name);
    }
    for (size_t i = 0; i < sortedLayer1.size(); ++i)
    {
        layer1.push_back(sortedLayer1[i]->m_name);
    }
}

#endif // #ifndef AZ_UNITY_BUILD

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

#pragma once

#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>

namespace AZ
{
    namespace IO
    {
        class ReadSplitter
            : public StreamStackEntry
        {
        public:
            explicit ReadSplitter(u64 maxReadSize);
            ~ReadSplitter() override = default;

            void PrepareRequest(FileRequest* request) override;

            void CollectStatistics(AZStd::vector<Statistic>& statistics) const override;

        private:
            AverageWindow<u64, double, s_statisticsWindowSize> m_averageNumSubReads;
            u64 m_maxReadSize;
        };
    } // namespace IO
} // namesapce AZ
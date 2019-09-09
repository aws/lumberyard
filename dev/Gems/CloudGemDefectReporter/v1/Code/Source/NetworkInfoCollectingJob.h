/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/
#pragma once

#include <AzCore/Jobs/Job.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/vector.h>

namespace CloudGemDefectReporter
{
    class NetworkInfoCollectingJob : public AZ::Job
    {
    public:
        AZ_CLASS_ALLOCATOR(NetworkInfoCollectingJob, AZ::SystemAllocator, 0);

        NetworkInfoCollectingJob(AZ::JobContext* jobContext, const AZStd::vector<AZStd::string>& domainNames, int reportId, int handlerId);

        void SetResultSeparator(const AZStd::string& separator) { m_resultSeparator = separator; };
        const AZStd::string GetResultSeparator() const { return m_resultSeparator; };

        void Process() override;

    private:
        AZStd::string CombineResults(const AZStd::vector<AZStd::string>& outputs) const;

    private:
        AZStd::vector<AZStd::string> m_domainNames; 
        int m_reporId;
        int m_handlerId;
        AZ::JobContext* m_jobContext;
        AZStd::string m_resultSeparator{"\n==================================\n"};
    };
}

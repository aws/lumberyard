
#include "CloudGemDefectReporter_precompiled.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>

#include <Traceroute.h>
#include <NSLookup.h>
#include <NetworkInfoCollectingJob.h>
#include <CloudGemDefectReporter/DefectReporterDataStructures.h>
#include <CloudGemDefectReporter/CloudGemDefectReporterBus.h>

namespace CloudGemDefectReporter
{
    NetworkInfoCollectingJob::NetworkInfoCollectingJob(AZ::JobContext* jobContext, const AZStd::vector<AZStd::string>& domainNames, int reportId, int handlerId) :
        AZ::Job(true, jobContext),
        m_jobContext(jobContext),
        m_domainNames(domainNames),
        m_reporId(reportId),
        m_handlerId(handlerId)
    {
    }

    void NetworkInfoCollectingJob::Process()
    {
        if (m_domainNames.size() == 0)
        {
            return;
        }

        AZStd::vector<AZStd::string> tracerouteResults;
        tracerouteResults.resize(m_domainNames.size());

        AZStd::vector<AZStd::string> nslookupResults;
        nslookupResults.resize(m_domainNames.size());

        for (int i = 0; i < m_domainNames.size(); i++)
        {            
            AZStd::string& domainName = m_domainNames[i];                        

            {
                AZStd::string& tracerouteResult = tracerouteResults[i];

                auto job = AZ::CreateJobFunction([this, domainName, &tracerouteResult]()
                {
                    Traceroute traceroute;
                    tracerouteResult = traceroute.GetResult(domainName);
                }, true, m_jobContext);

                StartAsChild(job);
            }

            {
                AZStd::string& nslookupResult = nslookupResults[i];

                auto job = AZ::CreateJobFunction([this, domainName, &nslookupResult]()
                {
                    NSLookup nslookup;
                    nslookupResult = nslookup.GetResult(domainName);
                }, true, m_jobContext);

                StartAsChild(job);
            }
        }

        WaitForChildren();

        AZStd::vector<MetricDesc> metrics;
        
        {
            MetricDesc metricDesc;
            metricDesc.m_key = "traceroute";
            metricDesc.m_data = CombineResults(tracerouteResults);

            metrics.emplace_back(metricDesc);
        }

        {
            MetricDesc metricDesc;
            metricDesc.m_key = "nslookup";
            metricDesc.m_data = CombineResults(nslookupResults);

            metrics.emplace_back(metricDesc);
        }

        CloudGemDefectReporterRequestBus::QueueBroadcast(&CloudGemDefectReporterRequestBus::Events::ReportData, m_reporId, m_handlerId, metrics, AZStd::vector<AttachmentDesc>());
    }

    AZStd::string NetworkInfoCollectingJob::CombineResults(const AZStd::vector<AZStd::string>& results) const
    {
        AZStd::string out;
        for (auto& result : results)
        {
            if (result.empty())
            {
                continue;
            }

            if (!out.empty())
            {
                out += m_resultSeparator;
            }

            out += result;
        }

        return out;
    }
}

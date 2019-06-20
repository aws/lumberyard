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

#include "CloudGemDefectReporter_precompiled.h"

#include <NetworkInfoCollectingComponent.h>
#include <NetworkInfoCollectingJob.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/std/string/regex.h>

#include <CloudCanvasCommon/CloudCanvasCommonBus.h>

#include <iterator>
#include <sstream>

#include <CrySystemBus.h>

namespace CloudGemDefectReporter
{

    NetworkInfoCollectingComponent::NetworkInfoCollectingComponent()
    {

    }

    void NetworkInfoCollectingComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<NetworkInfoCollectingComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<NetworkInfoCollectingComponent>("NetworkInfoCollectingComponent", "Provides traceroute and DNS information when OnCollectDefectReporterData is triggered")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void NetworkInfoCollectingComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CloudGemDefectReporterNetworkInfoCollectingService"));
    }


    void NetworkInfoCollectingComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void NetworkInfoCollectingComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void NetworkInfoCollectingComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("NetworkInfoCollectingComponent"));
    }

    void NetworkInfoCollectingComponent::Init()
    {

    }

    void NetworkInfoCollectingComponent::Activate()
    {
        CloudGemDefectReporterNotificationBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();
    }

    void NetworkInfoCollectingComponent::Deactivate()
    {
        CloudGemDefectReporterNotificationBus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
    }

    void NetworkInfoCollectingComponent::OnCrySystemInitialized(ISystem& system, const SSystemInitParams& params)
    {
        ReadServerDomainNames();
    }

    bool NetworkInfoCollectingComponent::ReadServerDomainNames()
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();

        if (!fileIO)
        {
            AZ_Error("CloudCanvas", false, "No FileIoBase Instance");
            return false;
        }
            
        AZ::IO::HandleType fileHandle;
        if (!fileIO->Open(GetServerDomainNamesConfigPath(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeText, fileHandle))
        {
            AZ_Warning("CloudCanvas", false, "Failed to open server name config file for defect reporter");
            return "";
        }

        AZ::u64 fileSize = 0;
        if (!fileIO->Size(fileHandle, fileSize))
        {
            AZ_Warning("CloudCanvas", false, "Failed to open server name config file for defect reporter");
            return "";
        }

        std::string output;
        output.resize(fileSize);

        fileIO->Read(fileHandle, &output[0], fileSize);
        fileIO->Close(fileHandle);

        std::istringstream iss(output);

        std::vector<std::string> tokens{ std::istream_iterator<std::string>{iss},
            std::istream_iterator<std::string>{} };

        AZStd::regex domainNameRegex("^([a-z0-9]+(-[a-z0-9]+)*\\.)+[a-z]{2,}$", AZStd::regex::extended);
        for (auto& domainName : tokens)
        {
            if (AZStd::regex_match(domainName.c_str(), domainNameRegex))
            {
                m_serverDomainNames.push_back(domainName.c_str());
            }
            else
            {
                AZ_Warning("CloudCanvas", false, "Invalid domain name encountered, skip..");
            }
        }        

        return true;
    }

    void NetworkInfoCollectingComponent::OnCollectDefectReporterData(int reportID)
    {   
        if (m_serverDomainNames.size() == 0)
        {
            return;
        }

        int handlerId = CloudGemDefectReporter::INVALID_ID;
        CloudGemDefectReporterRequestBus::BroadcastResult(handlerId, &CloudGemDefectReporterRequestBus::Events::GetHandlerID, reportID);

        AZ::JobContext* jobContext{ nullptr };
        EBUS_EVENT_RESULT(jobContext, CloudCanvasCommon::CloudCanvasCommonRequestBus, GetDefaultJobContext);

        NetworkInfoCollectingJob* job = aznew NetworkInfoCollectingJob(jobContext, m_serverDomainNames, reportID, handlerId);
        job->Start();        
    }

    const char* NetworkInfoCollectingComponent::GetServerDomainNamesConfigPath() const
    {
        return "CloudCanvas/defect_reporter_server_names.txt";
    }
}
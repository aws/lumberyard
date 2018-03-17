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

#include <CloudGemAWSScriptBehaviors_precompiled.h>

#include "AWSBehaviorURL.h"

#include <aws/core/utils/StringUtils.h>

namespace CloudGemAWSScriptBehaviors
{
    AWSBehaviorURL::AWSBehaviorURL() :
        m_inputURL()
    {

    }

    void AWSBehaviorURL::ReflectSerialization(AZ::SerializeContext* serializeContext)
    {
        if (serializeContext)
        {
            serializeContext->Class<AWSBehaviorURL>()
                ->Field("URL", &AWSBehaviorURL::m_inputURL)
                ->Version(1);
        }
    }

    void AWSBehaviorURL::ReflectBehaviors(AZ::BehaviorContext* behaviorContext)
    {
        behaviorContext->Class<AWSBehaviorURL>("AWSBehaviorURL")
            ->Method("Decode", &AWSBehaviorURL::Decode, nullptr, "Decodes the URL on AWS")
            ->Property("URL", nullptr, BehaviorValueSetter(&AWSBehaviorURL::m_inputURL));

        behaviorContext->EBus<AWSBehaviorURLNotificationsBus>("AWSBehaviorURLNotificationsBus")
            ->Handler<AWSBehaviorURLNotificationsBusHandler>();
    }

    void AWSBehaviorURL::ReflectEditParameters(AZ::EditContext* editContext)
    {
        editContext->Class<AWSBehaviorURL>("AWSBehaviorURL", "Wraps AWS URL functionality")
            ->DataElement(AZ::Edit::UIHandlers::Default, &AWSBehaviorURL::m_inputURL, "URL", "The URL to decode");
    }

    void AWSBehaviorURL::Decode()
    {
        if (m_inputURL.empty())
        {
            EBUS_EVENT(AWSBehaviorURLNotificationsBus, OnError, "AWS URL Decode: No URL provided");
            return;
        }

        AZStd::string decodedURL = Aws::Utils::StringUtils::URLDecode(m_inputURL.c_str()).c_str();
        EBUS_EVENT(AWSBehaviorURLNotificationsBus, OnSuccess, decodedURL.c_str());
    }
}
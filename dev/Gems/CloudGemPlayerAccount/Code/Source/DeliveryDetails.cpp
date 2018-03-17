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
#include "CloudGemPlayerAccount_precompiled.h"
#include "DeliveryDetails.h"

#include <AzCore/base.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace CloudGemPlayerAccount
{
    namespace 
    {
        const char* MEDIUM_NOT_SET = "NOT_SET";
        const char* MEDIUM_SMS = "SMS";
        const char* MEDIUM_EMAIL = "EMAIL";
    }

    void DeliveryDetails::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<DeliveryDetails>()
                ->Version(1);
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            // Scripts only need the getters, not setters.
            behaviorContext->Class<DeliveryDetails>()
                ->Method("GetAttributeName", &DeliveryDetails::GetAttributeName)
                ->Method("GetDeliveryMedium", &DeliveryDetails::GetDeliveryMedium)
                ->Method("GetDestination", &DeliveryDetails::GetDestination)
                ;
        }
    }

    DeliveryDetails::DeliveryDetails()
    {
    }

    void DeliveryDetails::Reset(const CognitoDeliveryDetailsType& details)
    {
         m_attributeName = details.GetAttributeName().c_str();
         m_destination = details.GetDestination().c_str();

        auto medium = details.GetDeliveryMedium();
        switch (medium)
        {
        case Aws::CognitoIdentityProvider::Model::DeliveryMediumType::NOT_SET:
            m_deliveryMedium = MEDIUM_NOT_SET;
            break;
        case Aws::CognitoIdentityProvider::Model::DeliveryMediumType::SMS:
            m_deliveryMedium = MEDIUM_SMS;
            break;
        case Aws::CognitoIdentityProvider::Model::DeliveryMediumType::EMAIL:
            m_deliveryMedium = MEDIUM_EMAIL;
            break;
        default:
            // There currently seems to be a bug in the SDK that makes the delivery medium set incorrectly.
            // The result should be either email or SMS, so just check if it's an email address.
            m_deliveryMedium = (m_destination.find('@') != Aws::String::npos) ? MEDIUM_EMAIL : MEDIUM_SMS;
            break;
        }
    }

    const AZStd::string& DeliveryDetails::GetAttributeName() const
    {
        return m_attributeName;
    }

    const AZStd::string& DeliveryDetails::GetDeliveryMedium() const
    {
        return m_deliveryMedium;
    }

    const AZStd::string& DeliveryDetails::GetDestination() const
    {
        return m_destination;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////

    void DeliveryDetailsArray::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<DeliveryDetailsArray>()
                ->Version(1);
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<DeliveryDetailsArray>()
                ->Method("GetSize", &DeliveryDetailsArray::GetSize)
                ->Method("At", &DeliveryDetailsArray::At)
                ;
        }
    }

    DeliveryDetailsArray::DeliveryDetailsArray(const Aws::Vector<CognitoDeliveryDetailsType>& data)
        : m_data(data.size())
    {
        for (size_t i = 0; i < data.size(); ++i)
        {
            m_data[i].Reset(data[i]);
        }
    }

    int DeliveryDetailsArray::GetSize() const
    {
        return m_data.size();
    }

    DeliveryDetails DeliveryDetailsArray::At(int index) const
    {
        if (index < 0 || index >= m_data.size())
        {
            return DeliveryDetails();
        }
        return m_data[index];
    }

    const DeliveryDetailsArrayData& DeliveryDetailsArray::GetData() const
    {
        return m_data;
    }

}   // namespace CloudGemPlayerAccount

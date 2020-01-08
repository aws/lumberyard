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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.

AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/cognito-idp/model/CodeDeliveryDetailsType.h>
#include <aws/core/utils/memory/stl/AWSVector.h>
AZ_POP_DISABLE_WARNING

namespace CloudGemPlayerAccount
{
    using CognitoDeliveryDetailsType = Aws::CognitoIdentityProvider::Model::CodeDeliveryDetailsType;

    class DeliveryDetails
    {
    public:
        AZ_TYPE_INFO(DeliveryDetails, "{E799A50D-6ACD-485B-8774-415DE2E44D91}")
        AZ_CLASS_ALLOCATOR(DeliveryDetails, AZ::SystemAllocator, 0)

        static void Reflect(AZ::ReflectContext* context);

        DeliveryDetails();

        void Reset(const CognitoDeliveryDetailsType& details);

        const AZStd::string& GetAttributeName() const;
        const AZStd::string& GetDeliveryMedium() const;
        const AZStd::string& GetDestination() const;

    protected:
        AZStd::string m_attributeName;
        AZStd::string m_deliveryMedium;
        AZStd::string m_destination;
    };

    using DeliveryDetailsArrayData = AZStd::vector<DeliveryDetails>;

    class DeliveryDetailsArray
    {
    public:
        AZ_TYPE_INFO(DeliveryDetailsArray, "{39924149-76D8-4BD7-B97A-D02184DBFFA1}")
        AZ_CLASS_ALLOCATOR(DeliveryDetailsArray, AZ::SystemAllocator, 0)

        static void Reflect(AZ::ReflectContext* context);

        DeliveryDetailsArray() {}
        DeliveryDetailsArray(const Aws::Vector<CognitoDeliveryDetailsType>& data);

        int GetSize() const;
        DeliveryDetails At(int index) const;

        const DeliveryDetailsArrayData& GetData() const;    // Not exposed to scripts

    protected:
        DeliveryDetailsArrayData m_data;
    };

}   // namespace CloudGemPlayerAccount
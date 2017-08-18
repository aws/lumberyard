/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#include <AzCore/EBus/EBus.h>

#include <LmbrAWS/ILmbrAWS.h>

#include <LmbrAWS/IAWSClientManager.h>

namespace CloudCanvasCommon
{

    class CloudCanvasCommonRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // TODO: this bus will support resource logical to physical mapping lookup

        //////////////////////////////////////////////////////////////////////////
        /// Returns an actual AWS resource name given a "ResourceGroup.Resource" 
        /// format "logical" resource name.
        virtual AZStd::string GetLogicalToPhysicalResourceMapping(const AZStd::string& logicalResourceName) = 0;

        // Create the S3 client for S3 Presign Node 
        virtual LmbrAWS::S3::BucketClient GetBucketClient(const AZStd::string& bucketName) = 0;

        // Some platforms (Android) need to point the http client at the certificate bundle to avoid SSL errors
        virtual LmbrAWS::RequestRootCAFileResult RequestRootCAFile(AZStd::string& resultPath) = 0;

        // Query the lambda manager for mapped function names (TODO when client manager just stores by type
        // make general queries like "GetLogicalResourceNamesByType"
        virtual AZStd::vector<AZStd::string> GetLogicalResourceNames(const AZStd::string& resourceType) = 0;
    };
    using CloudCanvasCommonRequestBus = AZ::EBus<CloudCanvasCommonRequests>;

} // namespace CloudCanvasCommon

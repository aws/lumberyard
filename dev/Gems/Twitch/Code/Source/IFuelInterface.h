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

#include <HttpRequestor/HttpRequestorBus.h>
#include <Twitch/TwitchTypes.h>
#include <Twitch/FuelTypes.h>

namespace Twitch
{
    class IFuelInterface;
    using IFuelInterfacePtr = std::shared_ptr<IFuelInterface>;

    class IFuelInterface
    {
    public:
        static IFuelInterfacePtr Alloc();

        virtual ~IFuelInterface() = default;

        virtual ResultCode GetClientID(AZStd::string& clientid) = 0;
        virtual ResultCode GetOAuthToken(AZStd::string& token) = 0;
        virtual ResultCode RequestEntitlement(AZStd::string& entitlement) = 0;
        virtual ResultCode RequestProductCatalog(ProductData& productData) = 0;
        virtual ResultCode PurchaseProduct(const Twitch::FuelSku& sku, PurchaseReceipt& purchaseReceipt) = 0;
        virtual ResultCode GetPurchaseUpdates(const AZStd::string& syncToken, PurchaseUpdate& purchaseUpdate) = 0;
        virtual bool SetApplicationID(const AZStd::string& twitchApplicaitonID) = 0;
        virtual AZStd::string GetApplicationID() const = 0;
        virtual AZStd::string GetSessionID() const = 0;
        virtual void AddHTTPRequest(const AZStd::string& URI, Aws::Http::HttpMethod method, const HttpRequestor::Headers & headers, const HttpRequestor::Callback & callback) = 0;
        virtual void AddHTTPRequest(const AZStd::string& URI, Aws::Http::HttpMethod method, const HttpRequestor::Headers & headers, const AZStd::string& body, const HttpRequestor::Callback& callback) = 0;
    };
}

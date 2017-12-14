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

#include <IFuelInterface.h>
#include <AzCore/std/parallel/mutex.h>


//
// Sadly, The FUEL SDK will include Microsoft's pplwin.h and ppltaskscheduler.h, both of
// which generate a C4355 warning, "warning C4355 : 'this' : used in base member initializer list"
// Also in fuel\include\fuel_impl\signature_validator.h(103): warning C4018: '<': signed/unsigned mismatch
//

#if defined(_WIN32)
#pragma warning (push) 
#pragma warning (disable:4355 4018)
#endif

// This must be undefined or the fuel client will compile with errors do to an
// actual member "GetMessage"
#ifdef GetMessage
#undef GetMessage
#endif

#include <fuel_impl/IapClient.h>

#if defined(_WIN32)
#pragma warning (pop)
#endif

namespace Twitch
{
    typedef std::shared_ptr<Fuel::Iap::IapClient> IapClientPtr;                             // 
    typedef std::map<Fuel::Iap::Model::Sku, Fuel::Iap::Model::Product> FuelProductMap;      // These are directly from the Fuel SDK, also the FUEL SDK uses std, not a AZStd, so it must be in the std namespace.
    typedef std::vector<Fuel::Iap::Model::Sku> FuelSkuArray;                                //

    class FuelInterface : public IFuelInterface
    {
    public:
        FuelInterface();
        virtual ~FuelInterface() AZ_DEFAULT_METHOD;
        
        IapClientPtr GetFuelClient() const;
        ResultCode GetClientID(AZStd::string& clientid) override;
        ResultCode GetOAuthToken(AZStd::string& token) override;
        ResultCode RequestEntitlement(AZStd::string& entitlement) override;
        ResultCode RequestProductCatalog(ProductData& productData) override;
        ResultCode PurchaseProduct(const Twitch::FuelSku& sku, PurchaseReceipt& purchaseReceipt) override;
        ResultCode GetPurchaseUpdates(const AZStd::string& syncToken, PurchaseUpdate& purchaseUpdate) override;
        bool SetApplicationID(const AZStd::string& twitchApplicaitonID) override;
        AZStd::string GetApplicationID() const  override;
        AZStd::string GetSessionID() const override;
        void AddHTTPRequest(const AZStd::string& URI, Aws::Http::HttpMethod method, const HttpRequestor::Headers & headers, const HttpRequestor::Callback & callback) override;
        void AddHTTPRequest(const AZStd::string& URI, Aws::Http::HttpMethod method, const HttpRequestor::Headers & headers, const AZStd::string& body, const HttpRequestor::Callback& callback) override;

    private:
        ResultCode RequestNewTwitchToken();
        void GetProductList(ProductInfoList& productList, const Fuel::Iap::IapClient::GetProductDataOutcome& outcome) const;
        void GetProductUnavailableSkus(FuelSkuList& skuList, const Fuel::Iap::IapClient::GetProductDataOutcome& outcome) const;
        void GetPurchaseReceipt(PurchaseReceipt& purchaseReceipt, const Fuel::Iap::Model::Receipt & receipt) const;
        bool IsValidTwitchAppID(const AZStd::string& twitchAppID) const;
        AZStd::string FuelStringToAZString(const Fuel::String& fuelString) const;
        Fuel::String AZStringToFuelString(const AZStd::string& azString) const;
        FuelProductType GetFuelProductType(Fuel::Iap::Model::ProductType pType) const;
        FulfillmentResult GetFulfillmentResult(Fuel::Iap::Model::FulfillmentResult result) const;

    private:
        static const AZ::u64 kRefreshTokenExperationTimeMS = 15 /*Minutes*/ * 60 /*Seconds*/ * 1000 /*Milliseconds in a second*/;   // 15 minutes

    private:
        AZStd::mutex            m_fuelMutex;
        AZStd::string           m_applicationID;
        AZStd::string           m_cachedClientID;
        AZStd::string           m_cachedOAuthToken;
        AZStd::string           m_cachedEntitlement;
        AZStd::string           m_cachedSessionID;
        AZ::u64                 m_refreshTokenExpireTime;
    };
}

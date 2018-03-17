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

#include "Twitch_precompiled.h"
#include <AzCore/std/time.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Math/Uuid.h>
#include <HttpRequestor/HttpRequestorBus.h>
#include "FuelInterface.h"

#ifdef GetMessage
#undef GetMessage
#endif

namespace Twitch
{
    IFuelInterfacePtr IFuelInterface::Alloc()
    {
        return std::make_shared<FuelInterface>();
    }
    
    FuelInterface::FuelInterface()
        : m_refreshTokenExpireTime(0)
    {
        // each time we create a interface we need a new session id, however this should not change during the life span of this object.
        AZ::Uuid sessionid( AZ::Uuid::Create() );
        char temp[128];

        sessionid.ToString(temp, 128, false, false);

        m_cachedSessionID = AZStd::string(temp);
    }

    bool FuelInterface::SetApplicationID(const AZStd::string& twitchApplicaitonID)
    {
        bool success = false;

        /*
        ** THIS CAN ONLY BE SET ONCE!!!!!!
        */

        if( m_applicationID.empty() )
        {
            if( IsValidTwitchAppID(twitchApplicaitonID) )
            {
                m_applicationID = twitchApplicaitonID;
                success = true;
            }
            else
            {
                AZ_Warning("FuelInterface::SetApplicationID", false, "Invalid Twitch Application ID! Request ignored!");
            }
        }
        else
        {
            AZ_Warning("FuelInterface::SetApplicationID", false, "Twitch Application ID is already set! Request ignored!");
        }
    
        return success;
    }
    
    Twitch::IapClientPtr FuelInterface::GetFuelClient() const
    {
        Twitch::IapClientPtr iapClient;

        // Create an IapClient instance
        Fuel::Iap::IapClient::CreateIapClientOutcome createResponse = Fuel::Iap::IapClient::Create("", "1.0", Fuel::RendererType::DIRECT_X_11);
        if (createResponse.IsSuccess())
        {
            // extract the IapClient pointer from the response
            iapClient = createResponse.GetResponseWithOwnership();

            if (iapClient == nullptr)
            {
                AZ_Warning("Fuel::GetFuelClient", false, "Failed to obtain a iapClient object!");
            }
        }
        else
        {
            AZ_Warning("Fuel::GetFuelClient", false, "Failed to create a Fuel client: %S", createResponse.GetError().GetMessage().c_str());
        }

        return iapClient;
    }

    ResultCode FuelInterface::GetClientID(AZStd::string& id)
    {
        ResultCode result = ResultCode::Unknown;
        // we only need to get a client id if we don't have one.

        if ( m_cachedClientID.empty() )
        {
            result = RequestNewTwitchToken();
        }
        
        if( !m_cachedClientID.empty() )
        {
            id = m_cachedClientID;
            result = ResultCode::Success;
        }

        return result;
    }

    ResultCode FuelInterface::GetOAuthToken(AZStd::string& token)
    {
        ResultCode result = ResultCode::Unknown;
        AZ::u64 currentUTCTimeMiliSecond = AZStd::GetTimeUTCMilliSecond();

        if (m_cachedOAuthToken.empty() || (m_refreshTokenExpireTime > currentUTCTimeMiliSecond) )
        {
            m_cachedOAuthToken.clear();
            result = RequestNewTwitchToken();
        }

        if( !m_cachedOAuthToken.empty() )
        {
            token = m_cachedOAuthToken;
            result = ResultCode::Success;
        }

        return result;
    }

    ResultCode FuelInterface::RequestEntitlement(AZStd::string& entitlement)
    {
        ResultCode result = ResultCode::Unknown;
        AZ::u64 currentUTCTimeMiliSecond = AZStd::GetTimeUTCMilliSecond();

        if ( m_cachedEntitlement.empty() )
        {
            result = RequestNewTwitchToken();
        }

        if (!m_cachedEntitlement.empty())
        {
            entitlement = m_cachedEntitlement;
            result = ResultCode::Success;
        }

        return result;
    }
    
    ResultCode FuelInterface::RequestProductCatalog(ProductData& productData)
    {
        ResultCode result = ResultCode::Unknown;

        Twitch::IapClientPtr clientPtr = GetFuelClient();

        if( clientPtr != nullptr)
        {
            const Fuel::Iap::Model::GetProductDataRequest request;  // Twitch sample suggest to make this const
            Fuel::Iap::IapClient::GetProductDataOutcome outcome( clientPtr->GetProductData(request) );

            if( outcome.IsSuccess() )
            {
                GetProductList(productData.ProductList, outcome);
                GetProductUnavailableSkus(productData.UnavailableSkus, outcome);

                result = ResultCode::Success;
            }
            else
            {
                result = ResultCode::FuelProductDataFail;
                AZ_Warning("FuelInterface::RequestProductCatalog", false, "Failed to get the FUEL session! Error: %S (%d)",
                    outcome.GetError().GetMessage().c_str(),
                    static_cast<std::underlying_type<Fuel::Iap::IapErrors>::type>(outcome.GetError().GetType()));
            }
        }
        else
        {
            result = ResultCode::FuelNoIAPClient;
            AZ_Warning("FuelInterface::RequestProductCatalog", false, "No client object");
        }

        return result;
    }

    ResultCode FuelInterface::PurchaseProduct(const Twitch::FuelSku& sku, PurchaseReceipt& purchaseReceipt)
    {
        ResultCode result = ResultCode::Unknown;

        Twitch::IapClientPtr clientPtr = GetFuelClient();

        if (clientPtr != nullptr)
        {
            Fuel::Iap::Model::PurchaseRequest request;
            request.SetSku( AZStringToFuelString(sku) );

            Fuel::Iap::IapClient::PurchaseOutcome outcome(clientPtr->Purchase(request));

            if (outcome.IsSuccess())
            {
                GetPurchaseReceipt(purchaseReceipt, outcome.GetResponse().GetReceipt());
                
                result = ResultCode::Success;
            }
            else
            {
                result = ResultCode::FuelPurchaseFail;
                AZ_Warning("FuelInterface::PurchaseProduct", false, "Failed to make purchase! Error: %S (%d)",
                    outcome.GetError().GetMessage().c_str(),
                    static_cast<std::underlying_type<Fuel::Iap::IapErrors>::type>(outcome.GetError().GetType()));
            }
        }
        else
        {
            result = ResultCode::FuelNoIAPClient;
            AZ_Warning("FuelInterface::PurchaseProduct", false, "No client object");
        }

        return result;
    }

    ResultCode FuelInterface::GetPurchaseUpdates(const AZStd::string& syncToken, PurchaseUpdate& purchaseUpdate)
    {
        ResultCode result = ResultCode::Unknown;

        Twitch::IapClientPtr clientPtr = GetFuelClient();

        if (clientPtr != nullptr)
        {
            Fuel::Iap::Model::SyncPointData syncPointData;
            syncPointData.SetSyncToken( AZStringToFuelString(syncToken) );
            syncPointData.SetTimestamp( Fuel::DateTime(std::chrono::seconds(0)) );

            Fuel::Iap::Model::GetPurchaseUpdatesRequest request;
            request.SetSyncPoint( syncPointData);

            Fuel::Iap::IapClient::GetPurchaseUpdatesOutcome outcome(clientPtr->GetPurchaseUpdates(request));

            if (outcome.IsSuccess())
            {
                //
                // get our receipts
                //
                const std::vector<Fuel::Iap::Model::Receipt> receipts( outcome.GetResponse().GetReceipts() );

                for(const auto & i: receipts)
                {
                    PurchaseReceipt purchaseReceipt;

                    GetPurchaseReceipt(purchaseReceipt, i);

                    purchaseUpdate.Products.push_back(purchaseReceipt);
                }

                //
                // get our sync token
                //

                const Fuel::Iap::Model::SyncPointData syncPointDataOutcome( outcome.GetResponse().GetSyncPoint());

                purchaseUpdate.SyncToken = FuelStringToAZString( syncPointDataOutcome.GetSyncToken() );

                result = ResultCode::Success;
            }
            else
            {
                result = ResultCode::FuelPurchaseFail;
                AZ_Warning("FuelInterface::GetPurchaseUpdates", false, "Failed to get purchase updates! Error: %S (%d)",
                    outcome.GetError().GetMessage().c_str(),
                    static_cast<std::underlying_type<Fuel::Iap::IapErrors>::type>(outcome.GetError().GetType()));
            }
        }
        else
        {
            result = ResultCode::FuelNoIAPClient;
            AZ_Warning("FuelInterface::GetPurchaseUpdates", false, "No client object");
        }

        return result;
    }

    AZStd::string FuelInterface::GetApplicationID() const
    {
        return m_applicationID;
    }

    AZStd::string FuelInterface::GetSessionID() const
    {
        return m_cachedSessionID;
    }

    void FuelInterface::AddHTTPRequest(const AZStd::string& URI, Aws::Http::HttpMethod method, const HttpRequestor::Headers & headers, const HttpRequestor::Callback & callback)
    {
        EBUS_EVENT(HttpRequestor::HttpRequestorRequestBus, AddRequestWithHeaders, URI, method, headers, callback);
    }

    void FuelInterface::AddHTTPRequest(const AZStd::string& URI, Aws::Http::HttpMethod method, const HttpRequestor::Headers & headers, const AZStd::string& body, const HttpRequestor::Callback & callback)
    {
        EBUS_EVENT(HttpRequestor::HttpRequestorRequestBus, AddRequestWithHeadersAndBody, URI, method, headers, body, callback);
    }
        
    ResultCode FuelInterface::RequestNewTwitchToken()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_fuelMutex);
        ResultCode result = ResultCode::Unknown;

        Twitch::IapClientPtr iapClientPtr(GetFuelClient());

        if (iapClientPtr != nullptr)
        {
            // get the application id
            AZStd::wstring appClientID;
            AZStd::to_wstring(appClientID, GetApplicationID());
            
            if( !appClientID.empty() )
            {
                Fuel::Iap::Model::GetSessionRequest sessionRequest;
                sessionRequest.SetClientId(appClientID.c_str());
                const auto authResponse = iapClientPtr->GetSession(sessionRequest);
                if (authResponse.IsSuccess())
                {
                    const auto& authResult = authResponse.GetResponse();
                    const auto& twitchUser = authResult.GetUser();
                    const auto& twitcEntitlement = authResult.GetEntitlement();

                    AZStd::string userOAuthToken;
                    AZStd::to_string(userOAuthToken, twitchUser.GetAccessToken().c_str());

                    AZStd::string userClientID;
                    AZStd::to_string(userClientID, twitchUser.GetTuid().c_str());

                    AZStd::string entitlement;
                    AZStd::to_string(entitlement, twitcEntitlement.c_str());

                    // if we don't have both then something is really wrong, this should never happen.

                    if (!userOAuthToken.empty() && !userClientID.empty())
                    {
                        result = ResultCode::Success;
                        m_cachedClientID = userClientID;
                        m_cachedOAuthToken = userOAuthToken;
                        m_cachedEntitlement = entitlement;

                        // reset time for the when the O-Auth token will expire.

                        m_refreshTokenExpireTime = AZStd::GetTimeUTCMilliSecond() + kRefreshTokenExperationTimeMS;
                    }
                    else
                    {
                        result = ResultCode::FuelMissingCredentials;
                        AZ_Warning("FuelInterface::RequestNewTwitchToken", false, "Missing OAuth or ClientID!");
                    }
                }
                else
                {
                    result = ResultCode::FuelNoSession;
                    AZ_Warning("FuelInterface::RequestNewTwitchToken", false, "Failed to get the FUEL session! Error: %S (%d)", 
                        authResponse.GetError().GetMessage().c_str(),
                        static_cast<std::underlying_type<Fuel::Iap::IapErrors>::type>(authResponse.GetError().GetType()) );
                }
            }
            else
            {
                result = ResultCode::FuelNoApplicationID;
                AZ_Warning("FuelInterface::RequestNewTwitchToken", false, "Empty or missing application id!");
            }
        }
        else
        {
            result = ResultCode::FuelNoIAPClient;
            AZ_Warning("FuelInterface::RequestNewTwitchToken", false, "No client object");
        }

        return result;
    }

    bool FuelInterface::IsValidTwitchAppID(const AZStd::string& twitchAppID) const
    {
        static const AZStd::size_t kMinIDLength = 24;   // min id length
        static const AZStd::size_t kMaxIDLength = 64;   // max id length
        bool isValid = false;

        if ((twitchAppID.size() >= kMinIDLength) && (twitchAppID.size() < kMaxIDLength))
        {
            AZStd::size_t found = twitchAppID.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyz");

            if (found == AZStd::string::npos)
            {
                isValid = true;
            }
        }

        return isValid;
    }
  
    void FuelInterface::GetProductList(ProductInfoList& productList, const Fuel::Iap::IapClient::GetProductDataOutcome & outcome) const
    {
        const FuelProductMap & fuelProducts( outcome.GetResponse().GetProducts() );
        
        for(const auto & i: fuelProducts)
        {
            ProductInfo info;

            info.Sku                = FuelStringToAZString(i.first);
            info.Description        = FuelStringToAZString(i.second.GetDescription());
            info.Price              = FuelStringToAZString(i.second.GetPrice());
            info.SmallIconUrl       = FuelStringToAZString(i.second.GetSmallIconUrl());
            info.Title              = FuelStringToAZString(i.second.GetTitle() );
            info.ProductType        = GetFuelProductType(i.second.GetProductType() );
            
            productList.push_back(info);
        }
    }

    void FuelInterface::GetProductUnavailableSkus(FuelSkuList& skuList, const Fuel::Iap::IapClient::GetProductDataOutcome & outcome) const
    {
        const FuelSkuArray & sdkMap(outcome.GetResponse().GetUnavailableSkus());

        for (const auto & i : sdkMap)
        {
            skuList.push_back( FuelStringToAZString(i) );
        }
    }

    void FuelInterface::GetPurchaseReceipt(PurchaseReceipt& purchaseReceipt, const Fuel::Iap::Model::Receipt & receipt) const
    {
        purchaseReceipt.Sku             = FuelStringToAZString(receipt.GetSku());
        purchaseReceipt.ReceiptId       = FuelStringToAZString(receipt.GetReceiptId());
        purchaseReceipt.PurchaseDate    = receipt.GetPurchaseDate().time_since_epoch().count();
        purchaseReceipt.CancelDate      = receipt.GetCancelDate().time_since_epoch().count();
        purchaseReceipt.Type            = GetFuelProductType(receipt.GetProductType());
    }


    AZStd::string FuelInterface::FuelStringToAZString(const Fuel::String& fuelString) const
    {
        AZStd::string utf8String;
        AZStd::to_string(utf8String, fuelString.c_str(), fuelString.length());

        return utf8String;
    }

    Fuel::String FuelInterface::AZStringToFuelString(const AZStd::string& utf8String) const
    {
        AZStd::wstring wString;
        AZStd::to_wstring(wString, utf8String.c_str(), utf8String.length());

        return Fuel::String(wString.c_str());
    }

    FuelProductType FuelInterface::GetFuelProductType(Fuel::Iap::Model::ProductType pType) const
    {
        FuelProductType fType = FuelProductType::Undefined;

        switch(pType)
        {
            case Fuel::Iap::Model::ProductType::Consumable:     fType = FuelProductType::Consumable; break;
            case Fuel::Iap::Model::ProductType::Entitlement:    fType = FuelProductType::Entitlement; break;
            case Fuel::Iap::Model::ProductType::Subscription:   fType = FuelProductType::Subscription; break;
            case Fuel::Iap::Model::ProductType::Unknown:        fType = FuelProductType::Unknown; break;
            default: fType = FuelProductType::Undefined;
        }

        return fType;
    }

    FulfillmentResult FuelInterface::GetFulfillmentResult(Fuel::Iap::Model::FulfillmentResult result) const
    {
        FulfillmentResult fType = FulfillmentResult::Undefined;

        switch (result)
        {
            case Fuel::Iap::Model::FulfillmentResult::Fulfilled:    fType = FulfillmentResult::Fulfilled; break;
            case Fuel::Iap::Model::FulfillmentResult::Unavailable:  fType = FulfillmentResult::Unavailable; break;
            default: fType = FulfillmentResult::Undefined;
        }

        return fType;
    }

}




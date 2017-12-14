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

namespace Twitch
{
    /*
    ** The Twitch Commerce API result from a call to GetProductData. All strings
    ** are UTF-8.
    */

    using FuelSku = AZStd::string;
    using FuelSkuList = AZStd::list<FuelSku>;

    enum class FuelProductType {Consumable, Entitlement, Subscription, Unknown, Undefined};
    enum class FulfillmentResult {Fulfilled, Unavailable, Undefined};
       
    struct ProductInfo
    {
        AZ_TYPE_INFO(ProductInfo, "{D9D91FDD-3F40-4EB5-8B76-DA0B3D6905BA}");

        ProductInfo()
            : ProductType(FuelProductType::Unknown)
        {
        }

        FuelSku             Sku;
        AZStd::string       Description;
        AZStd::string       Price;
        AZStd::string       SmallIconUrl;
        AZStd::string       Title;
        FuelProductType     ProductType;
    };

    using ProductInfoList = AZStd::list<ProductInfo>;

    struct ProductData
    {
        AZ_TYPE_INFO(ProductData, "{587C8136-D2AD-40DA-A7CF-AA0CFFB486B7}");

        ProductInfoList         ProductList;
        FuelSkuList             UnavailableSkus;
    };
    
    CreateReturnTypeClass(ProductDataReturnValue, ProductData, "{FDE124C0-DEEF-4339-A5A5-7EAF98274C03}");
        
    /*
    ** Represents the purchase of consumable content, entitlement content or
    ** subscription content, as well as the renewal of a subscription. All strings
    ** are UTF-8.
    */

    struct PurchaseReceipt
    {
        AZ_TYPE_INFO(PurchaseReceipt, "{33FD83AD-D3EC-4AB8-9DF7-A75A3FBE3DBD}");

        PurchaseReceipt()
            : PurchaseDate(0)
            , CancelDate(0)
            , Type(FuelProductType::Unknown)
        {
        }
        
        FuelSku             Sku;
        AZStd::string       ReceiptId;
        AZ::u64             PurchaseDate;
        AZ::u64             CancelDate;
        FuelProductType     Type;
    };

    using PurchaseReceiptList = AZStd::list<PurchaseReceipt>;

    CreateReturnTypeClass(PurchaseReceiptReturnValue, PurchaseReceipt, "{1EF817BC-1D1D-43BC-B4C4-26315DA6E995}");

    /*
    ** Initiates a request to retrieve updates about items the customer has purchased or canceled.
    ** The request includes a sync token, an opaque string that the backend services created and use to
    ** determine which purchase updates were synced. The first time GetPurchaseUpdates is called a empty 
    ** sync token should be used. The response will contain a new sync token which can be used in the
    ** request of the next call to GetPurchaseUpdates. In each subsequent call to GetPurchaseUpdates, 
    ** sync token returned can be used in the request for the next.
    **
    ** Once the developer has all the receipts, they can fulfill or revoke those orders, if they have
    ** not done so previously. Then the developer should mark the order as fulfilled or unavailable,
    ** using NotifyFulfillment. Once a consumable order is marked as fulfilled, it no longer is returned
    ** by GetPurchaseUpdates.
    **/

    struct PurchaseUpdate
    {
        AZ_TYPE_INFO(PurchaseUpdate, "{4A805E11-6023-4E1B-A0B7-F501F3549B29}");

        AZStd::string           SyncToken;
        PurchaseReceiptList     Products;
    };
    
    CreateReturnTypeClass(PurchaseUpdateReturnValue, PurchaseUpdate, "{93F90EEE-17E3-4055-BD63-DCC8D98D1BE7}");
}

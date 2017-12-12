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

#include "InAppPurchases/InAppPurchasesBus.h"
#include <AzCore/Module/Module.h>
#include <IGem.h>

namespace InAppPurchases
{
    class InAppPurchasesModule
        : public CryHooksModule
        , public InAppPurchasesRequestBus::Handler
    {
    public:
        AZ_RTTI(InAppPurchasesModule, "{19EC24B6-87E8-44AE-AC33-C280F67FD3F7}", AZ::Module);

        void Initialize() override;
        
        InAppPurchasesModule();
        ~InAppPurchasesModule() override;

        void QueryProductInfoById(const AZStd::string& productId) const override;
        void QueryProductInfoByIds(AZStd::vector<AZStd::string>& productIds) const override;
        void QueryProductInfo() const override;

        const AZStd::vector<AZStd::unique_ptr<ProductDetails const> >* GetCachedProductInfo() const override;
        const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >* GetCachedPurchasedProductInfo() const override;

        void PurchaseProductWithDeveloperPayload(const AZStd::string& productId, const AZStd::string& developerPayload) const override;
        void PurchaseProduct(const AZStd::string& productId) const override;
        
        void QueryPurchasedProducts() const override;
        
        void RestorePurchasedProducts() const override;
        
        void ConsumePurchase(const AZStd::string& purchaseToken) const override;
        
        void FinishTransaction(const AZStd::string& transactionId, bool downloadHostedContent) const override;

        void ClearCachedProductDetails() override;
        void ClearCachedPurchasedProductDetails() override;

        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
    };
}

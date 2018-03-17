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

#include "InAppPurchases_precompiled.h"

#include "InAppPurchasesModule.h"
#include <platform_impl.h>

#include <FlowSystem/Nodes/FlowBaseNode.h>

namespace InAppPurchases
{
    InAppPurchasesModule::InAppPurchasesModule() : CryHooksModule()
    {
        InAppPurchasesRequestBus::Handler::BusConnect();
    }

    InAppPurchasesModule::~InAppPurchasesModule()
    {
        InAppPurchasesInterface::DestroyInstance();
        InAppPurchasesRequestBus::Handler::BusDisconnect();
    }

    void InAppPurchasesModule::Initialize()
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->Initialize();
        }
    }

    void InAppPurchasesModule::QueryProductInfoById(const AZStd::string& productId) const
    {
        AZStd::vector<AZStd::string> productIds;
        productIds.push_back(productId);
        InAppPurchasesModule::QueryProductInfoByIds(productIds);
    }

    void InAppPurchasesModule::QueryProductInfoByIds(AZStd::vector<AZStd::string>& productIds) const
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->QueryProductInfo(productIds);
        }
    }

    void InAppPurchasesModule::QueryProductInfo() const
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->QueryProductInfo();
        }
    }

    const AZStd::vector<AZStd::unique_ptr<ProductDetails const> >* InAppPurchasesModule::GetCachedProductInfo() const
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            return &InAppPurchasesInterface::GetInstance()->GetCache()->GetCachedProductDetails();
        }
        return nullptr;
    }

    const AZStd::vector<AZStd::unique_ptr<PurchasedProductDetails const> >* InAppPurchasesModule::GetCachedPurchasedProductInfo() const
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            return &InAppPurchasesInterface::GetInstance()->GetCache()->GetCachedPurchasedProductDetails();
        }
        return nullptr;
    }

    void InAppPurchasesModule::PurchaseProductWithDeveloperPayload(const AZStd::string& productId, const AZStd::string& developerPayload) const
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->PurchaseProduct(productId, developerPayload);
        }
    }

    void InAppPurchasesModule::PurchaseProduct(const AZStd::string& productId) const
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->PurchaseProduct(productId);
        }
    }

    void InAppPurchasesModule::QueryPurchasedProducts() const
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->QueryPurchasedProducts();
        }
    }
    
    void InAppPurchasesModule::RestorePurchasedProducts() const
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->RestorePurchasedProducts();
        }
    }
    
    void InAppPurchasesModule::ConsumePurchase(const AZStd::string& purchaseToken) const
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->ConsumePurchase(purchaseToken);
        }
    }
    
    void InAppPurchasesModule::FinishTransaction(const AZStd::string& transactionId, bool downloadHostedContent) const
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->FinishTransaction(transactionId, downloadHostedContent);
        }
    }

    void InAppPurchasesModule::ClearCachedProductDetails()
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->GetCache()->ClearCachedProductDetails();
        }
    }

    void InAppPurchasesModule::ClearCachedPurchasedProductDetails()
    {
        if (InAppPurchasesInterface::GetInstance() != nullptr)
        {
            InAppPurchasesInterface::GetInstance()->GetCache()->ClearCachedPurchasedProductDetails();
        }
    }

    void InAppPurchasesModule::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
    {
        switch (event)
        {
        case ESYSTEM_EVENT_FLOW_SYSTEM_REGISTER_EXTERNAL_NODES:
        {
            RegisterExternalFlowNodes();
        }
        break;
        }
    }
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(InAppPurchases_92fe57eae7d3402a90761973678c079a, InAppPurchases::InAppPurchasesModule)
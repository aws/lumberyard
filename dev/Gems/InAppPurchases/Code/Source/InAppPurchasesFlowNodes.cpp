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
#include "StdAfx.h"

#include <FlowSystem/Nodes/FlowBaseNode.h>

#include <InAppPurchases/InAppPurchasesBus.h>

#include <AzCore/JSON/document.h>
#include <AzCore/JSON/error/en.h>


namespace InAppPurchases
{
    class QueryProductInfoFlowNode
        : public CFlowBaseNode<eNCT_Singleton>
    {
    public:

        QueryProductInfoFlowNode(SActivationInfo* activationInfo)
            : CFlowBaseNode()
        {
        }

        ~QueryProductInfoFlowNode() override
        {
        }

    protected:

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig<string>("ProductIds", _HELP("The Ids of the products for which you want to retrieve product info. The product ids must be in JSON format with a list of product ids inside an array named product_ids")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.sDescription = _HELP("Query for product details from Play Store/App Store");
            
            config.SetCategory(EFLN_ADVANCED);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
        {
            switch (event)
            {
            case eFE_Activate:
            {
                if (IsPortActive(activationInfo, 0))
                {
                    const char* productIds = GetPortString(activationInfo, 0).c_str();

                    rapidjson::Document parseProductIds;
                    parseProductIds.Parse<rapidjson::kParseNoFlags>(productIds);
                    if (parseProductIds.HasParseError())
                    {
                        const char* errorStr = rapidjson::GetParseError_En(parseProductIds.GetParseError());
                        AZ_Warning("LumberyardInAppBilling", false, "Failed to parse product_ids: %s\n", errorStr);
                        return;
                    }

                    AZStd::vector<AZStd::string> productIdsVec;
                    if (parseProductIds.HasMember("product_ids") && parseProductIds["product_ids"].GetType() == rapidjson::kArrayType)
                    {
                        rapidjson::Value& productIdsArray = parseProductIds["product_ids"];
                        for (rapidjson::Value::ConstValueIterator itr = productIdsArray.Begin(); itr != productIdsArray.End(); itr++)
                        {
                            if ((*itr).HasMember("id"))
                            {
                                productIdsVec.push_back((*itr)["id"].GetString());
                            }
                        }
                        EBUS_EVENT(InAppPurchases::InAppPurchasesRequestBus, QueryProductInfoByIds, productIdsVec);
                    }
                    else
                    {
                        AZ_Warning("LumberyardInAppPurchases", false, "The JSON string provided does not contain an array named ProductIds!");
                    }
                }
            }
            break;
            }
        }

        void GetMemoryUsage(ICrySizer* sizer) const override
        {
            sizer->Add(*this);
        }
    };

    REGISTER_FLOW_NODE("InAppPurchases:QueryProductInfo", QueryProductInfoFlowNode)


    class PurchaseProductFlowNode
        : public CFlowBaseNode<eNCT_Singleton>
    {
    public:

        PurchaseProductFlowNode(SActivationInfo* activationInfo)
            : CFlowBaseNode()
        {
            m_productId = "";
            m_developerPayload = "";
        }

        ~PurchaseProductFlowNode() override
        {
        }

    protected:

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Activate"),
                InputPortConfig<string>("ProductId", _HELP("The Id of the product you wish to purchase.")),
                InputPortConfig<string>("DeveloperPayload", _HELP("[OPTIONAL] Developer payload is usually a username or account name that uniquely identifies the user who is logged into the device and making the purchase.")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.sDescription = _HELP("Purchase a product using its product id.");

            config.SetCategory(EFLN_ADVANCED);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
        {
            switch (event)
            {
            case eFE_Activate:
            {
                if (IsPortActive(activationInfo, 1))
                {
                    m_productId = AZStd::string(GetPortString(activationInfo, 1).c_str());
                }
                if (IsPortActive(activationInfo, 2))
                {
                    m_developerPayload = AZStd::string(GetPortString(activationInfo, 2).c_str());
                }

                if (IsPortActive(activationInfo, 0))
                {
                    if (!m_productId.empty())
                    {
                        EBUS_EVENT(InAppPurchases::InAppPurchasesRequestBus, PurchaseProductWithDeveloperPayload, m_productId, m_developerPayload);
                        m_productId = "";
                        m_developerPayload = "";
                    }
                    else
                    {
                        AZ_Warning("LumberyardInAppPurchases", false, "Cannot purchase a product without a product id!");
                    }
                }
            }
            break;
            }
        }

        void GetMemoryUsage(ICrySizer* sizer) const override
        {
            sizer->Add(*this);
        }

    private:

        AZStd::string m_productId;
        AZStd::string m_developerPayload;
    };

    REGISTER_FLOW_NODE("InAppPurchases:PurchaseProduct", PurchaseProductFlowNode)


    class QueryPurchasedProductsFlowNode
        : public CFlowBaseNode<eNCT_Singleton>
    {
    public:

        QueryPurchasedProductsFlowNode(SActivationInfo* activationInfo)
            : CFlowBaseNode()
        {
        }

        ~QueryPurchasedProductsFlowNode() override
        {
        }

    protected:

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Activate"),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.sDescription = _HELP("Query the list of products that the user already owns.");

            config.SetCategory(EFLN_ADVANCED);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
        {
            switch (event)
            {
            case eFE_Activate:
            {
                if (IsPortActive(activationInfo, 0))
                {
                    EBUS_EVENT(InAppPurchases::InAppPurchasesRequestBus, QueryPurchasedProducts);
                }
            }
            break;
            }
        }

        void GetMemoryUsage(ICrySizer* sizer) const override
        {
            sizer->Add(*this);
        }
    };

    REGISTER_FLOW_NODE("InAppPurchases:QueryPurchasedProducts", QueryPurchasedProductsFlowNode)


    class ConsumePurchaseFlowNode
        : public CFlowBaseNode<eNCT_Singleton>
    {
    public:

        ConsumePurchaseFlowNode(SActivationInfo* activationInfo)
            : CFlowBaseNode()
        {
            m_purchaseToken = "";
        }

        ~ConsumePurchaseFlowNode() override
        {
        }

    protected:

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Activate"),
                InputPortConfig<string>("PurchaseToken", _HELP("The purchase token you received with the receipt. For platforms that don't explicitly require a purchase to be consumed, pass in an empty string.")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.sDescription = _HELP("Query the list of products that the user already owns.");

            config.SetCategory(EFLN_ADVANCED);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
        {
            switch (event)
            {
            case eFE_Activate:
            {
                if (IsPortActive(activationInfo, 1))
                {
                    m_purchaseToken = AZStd::string(GetPortString(activationInfo, 1).c_str());
                }

                if (IsPortActive(activationInfo, 0))
                {
                    if (!m_purchaseToken.empty())
                    {
                        EBUS_EVENT(InAppPurchases::InAppPurchasesRequestBus, ConsumePurchase, m_purchaseToken);
                        m_purchaseToken = "";
                    }
                    else
                    {
                        AZ_Warning("LumberyardInAppPurchases", false, "No purchase token provided!");
                    }
                }
            }
            break;
            }
        }

        void GetMemoryUsage(ICrySizer* sizer) const override
        {
            sizer->Add(*this);
        }

    private:

        AZStd::string m_purchaseToken;
    };

    REGISTER_FLOW_NODE("InAppPurchases:ConsumePurchase", ConsumePurchaseFlowNode)


    class FinishTransactionFlowNode
        : public CFlowBaseNode<eNCT_Singleton>
    {
    public:

        FinishTransactionFlowNode(SActivationInfo* activationInfo)
            : CFlowBaseNode()
        {
            m_orderId = "";
            m_downloadHostedContent = false;
        }

        ~FinishTransactionFlowNode() override
        {
        }

    protected:

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Activate"),
                InputPortConfig<string>("TransactionId", _HELP("The transaction id(order id) you received in the receipt after the purchase.")),
                InputPortConfig<string>("DownloadHostedContent", _HELP("If you have content hosted on Apple/Sony/Microsoft servers, setting this to true will download the content before finishing the transaction. Defaults to false.")),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.sDescription = _HELP("Query the list of products that the user already owns.");

            config.SetCategory(EFLN_ADVANCED);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
        {
            switch (event)
            {
            case eFE_Activate:
            {
                if (IsPortActive(activationInfo, 1))
                {
                    m_orderId = AZStd::string(GetPortString(activationInfo, 1).c_str());
                }
                if (IsPortActive(activationInfo, 2))
                {
                    m_downloadHostedContent = GetPortBool(activationInfo, 2);
                }

                if (IsPortActive(activationInfo, 0))
                {
                    if (!m_orderId.empty())
                    {
                        EBUS_EVENT(InAppPurchases::InAppPurchasesRequestBus, FinishTransaction, m_orderId, m_downloadHostedContent);
                        m_orderId = "";
                        m_downloadHostedContent = false;
                    }
                    else
                    {
                        AZ_Warning("LumberyardInAppPurchases", false, "No transaction id provided!");
                    }
                }
            }
            break;
            }
        }

        void GetMemoryUsage(ICrySizer* sizer) const override
        {
            sizer->Add(*this);
        }

    private:

        AZStd::string m_orderId;
        bool m_downloadHostedContent;
    };

    REGISTER_FLOW_NODE("InAppPurchases:FinishTransaction", FinishTransactionFlowNode)


    class InitializeFlowNode
        : public CFlowBaseNode<eNCT_Singleton>
    {
    public:

        InitializeFlowNode(SActivationInfo* activationInfo)
            : CFlowBaseNode()
        {
        }

        ~InitializeFlowNode() override
        {
        }

    protected:

        void GetConfiguration(SFlowNodeConfig& config) override
        {
            static const SInputPortConfig inputs[] =
            {
                InputPortConfig_Void("Activate"),
                { 0 }
            };

            config.pInputPorts = inputs;
            config.sDescription = _HELP("Initialize the in app purchases module.");

            config.SetCategory(EFLN_ADVANCED);
        }

        void ProcessEvent(EFlowEvent event, SActivationInfo* activationInfo) override
        {
            switch (event)
            {
            case eFE_Activate:
            {
                if (IsPortActive(activationInfo, 0))
                {
                    EBUS_EVENT(InAppPurchases::InAppPurchasesRequestBus, Initialize);
                }
            }
            break;
            }
        }

        void GetMemoryUsage(ICrySizer* sizer) const override
        {
            sizer->Add(*this);
        }
    };

    REGISTER_FLOW_NODE("InAppPurchases:Initialize", InitializeFlowNode)
}

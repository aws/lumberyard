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

#include "InAppPurchasesAndroid.h"

#include "InAppPurchasesModule.h"

#include <InAppPurchases/InAppPurchasesResponseBus.h>
#include <NativeUIRequests.h>

#include <AzCore/JSON/document.h>
#include <AzCore/JSON/error/en.h>
#include <AzCore/IO/FileIO.h>
#include <SDL_system.h>

#include <android/log.h>
#include <AzCore/EBus/BusImpl.h>

namespace InAppPurchases
{
    static PurchasedProductDetailsAndroid* ParseReceiptDetails(JNIEnv* env, jobjectArray jpurchasedProductDetails, jobjectArray jpurchaseSignatures, int index)
    {
        jstring jpurchasedProduct = static_cast<jstring>(env->GetObjectArrayElement(jpurchasedProductDetails, index));
        jstring jsignature = static_cast<jstring>(env->GetObjectArrayElement(jpurchaseSignatures, index));
        const char* purchasedProduct = env->GetStringUTFChars(jpurchasedProduct, nullptr);
        const char* signature = env->GetStringUTFChars(jsignature, nullptr);

        PurchasedProductDetailsAndroid* purchasedProductDetails = new PurchasedProductDetailsAndroid();

        rapidjson::Document parsePurchasedProduct;
        parsePurchasedProduct.Parse<rapidjson::kParseNoFlags>(purchasedProduct);
        purchasedProductDetails->SetProductId(parsePurchasedProduct["productId"].GetString());
        if (parsePurchasedProduct.HasMember("orderId"))
        {
            purchasedProductDetails->SetOrderId(parsePurchasedProduct["orderId"].GetString());
        }
        purchasedProductDetails->SetPackageName(parsePurchasedProduct["packageName"].GetString());
        purchasedProductDetails->SetPurchaseToken(parsePurchasedProduct["purchaseToken"].GetString());
        if (parsePurchasedProduct.HasMember("developerPayload"))
        {
            purchasedProductDetails->SetDeveloperPayload(parsePurchasedProduct["developerPayload"].GetString());
        }
        purchasedProductDetails->SetPurchaseTime(parsePurchasedProduct["purchaseTime"].GetUint64());

        int purchaseState = parsePurchasedProduct["purchaseState"].GetInt();
        if (purchaseState == 0)
        {
            purchasedProductDetails->SetPurchaseState(PurchaseState::PURCHASED);
        }
        else if (purchaseState == 1)
        {
            purchasedProductDetails->SetPurchaseState(PurchaseState::CANCELLED);
        }
        else if (purchaseState == 2)
        {
            purchasedProductDetails->SetPurchaseState(PurchaseState::REFUNDED);
        }

        if (parsePurchasedProduct.HasMember("autoRenewing"))
        {
            purchasedProductDetails->SetIsAutoRenewing(parsePurchasedProduct["autoRenewing"].GetBool());
        }
        purchasedProductDetails->SetPurchaseSignature(AZStd::string(signature, env->GetStringUTFLength(jsignature)));

        env->ReleaseStringUTFChars(jpurchasedProduct, purchasedProduct);
        env->ReleaseStringUTFChars(jsignature, signature);

        return purchasedProductDetails;
    }

    void ProductInfoRetrieved(JNIEnv* env, jobject obj, jobjectArray jproductDetails)
    {
        int numProducts = env->GetArrayLength(jproductDetails);

        InAppPurchasesInterface::GetInstance()->GetCache()->ClearCachedProductDetails();

        for (int i = 0; i < numProducts; i++)
        {
            jstring jproduct = static_cast<jstring>(env->GetObjectArrayElement(jproductDetails, i));
            const char* product = env->GetStringUTFChars(jproduct, nullptr);

            ProductDetailsAndroid* productDetails = new ProductDetailsAndroid();

            rapidjson::Document parseProduct;
            parseProduct.Parse<rapidjson::kParseNoFlags>(product);
            productDetails->SetProductId(parseProduct["productId"].GetString());
            productDetails->SetProductType(parseProduct["type"].GetString());
            productDetails->SetProductPrice(parseProduct["price"].GetString());
            productDetails->SetProductCurrencyCode(parseProduct["price_currency_code"].GetString());
            productDetails->SetProductTitle(parseProduct["title"].GetString());
            productDetails->SetProductDescription(parseProduct["description"].GetString());
            productDetails->SetProductPriceMicro(parseProduct["price_amount_micros"].GetUint64());
            InAppPurchasesInterface::GetInstance()->GetCache()->AddProductDetailsToCache(productDetails);

            env->ReleaseStringUTFChars(jproduct, product);
        }
        EBUS_EVENT(InAppPurchasesResponseBus, ProductInfoRetrieved, InAppPurchasesInterface::GetInstance()->GetCache()->GetCachedProductDetails());
    }

    void PurchasedProductsRetrieved(JNIEnv* env, jobject object, jobjectArray jpurchasedProductDetails, jobjectArray jpurchaseSignatures)
    {
        InAppPurchasesInterface::GetInstance()->GetCache()->ClearCachedPurchasedProductDetails();
        int numPurchasedProducts = env->GetArrayLength(jpurchasedProductDetails);
        for (int i = 0; i < numPurchasedProducts; i++)
        {
            PurchasedProductDetailsAndroid* purchasedProduct = ParseReceiptDetails(env, jpurchasedProductDetails, jpurchaseSignatures, i);
            InAppPurchasesInterface::GetInstance()->GetCache()->AddPurchasedProductDetailsToCache(purchasedProduct);
        }
        EBUS_EVENT(InAppPurchasesResponseBus, PurchasedProductsRetrieved, InAppPurchasesInterface::GetInstance()->GetCache()->GetCachedPurchasedProductDetails());
    }

    void NewProductPurchased(JNIEnv* env, jobject object, jobjectArray jpurchaseReceipt, jobjectArray jpurchaseSignature)
    {
        PurchasedProductDetailsAndroid* purchasedProduct = ParseReceiptDetails(env, jpurchaseReceipt, jpurchaseSignature, 0);

        if (purchasedProduct->GetPurchaseState() == PurchaseState::PURCHASED)
        {
            InAppPurchasesInterface::GetInstance()->GetCache()->AddPurchasedProductDetailsToCache(purchasedProduct);
            EBUS_EVENT(InAppPurchasesResponseBus, NewProductPurchased, purchasedProduct);
        }
        else if (purchasedProduct->GetPurchaseState() == PurchaseState::CANCELLED)
        {
            EBUS_EVENT(InAppPurchasesResponseBus, PurchaseCancelled, purchasedProduct);
            delete purchasedProduct;
        }
        else if (purchasedProduct->GetPurchaseState() == PurchaseState::REFUNDED)
        {
            EBUS_EVENT(InAppPurchasesResponseBus, PurchaseRefunded, purchasedProduct);
            delete purchasedProduct;
        }
    }

    static JNINativeMethod methods[] = {
        { "nativeProductInfoRetrieved", "([Ljava/lang/String;)V", (void*)ProductInfoRetrieved },
        { "nativePurchasedProductsRetrieved", "([Ljava/lang/String;[Ljava/lang/String;)V", (void*)PurchasedProductsRetrieved },
        { "nativeNewProductPurchased", "([Ljava/lang/String;[Ljava/lang/String;)V", (void*)NewProductPurchased },
    };


    void InAppPurchasesAndroid::Initialize()
    {
        JNIEnv* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
        jobject activityObject = static_cast<jobject>(SDL_AndroidGetActivity());

        jclass billingClass = env->FindClass("com/amazon/lumberyard/iap/LumberyardInAppBilling");
        jmethodID mid = env->GetMethodID(billingClass, "<init>", "(Landroid/app/Activity;)V");

        jobject billingInstance = env->NewObject(billingClass, mid, activityObject);
        m_billingInstance = env->NewGlobalRef(billingInstance);

        env->RegisterNatives(billingClass, methods, sizeof(methods) / sizeof(methods[0]));

        mid = env->GetMethodID(billingClass, "IsKindleDevice","()Z");
        jboolean result = env->CallBooleanMethod(m_billingInstance, mid);
        if (!result)
        {
        mid = env->GetMethodID(billingClass, "Initialize", "()V");
        env->CallVoidMethod(m_billingInstance, mid);
        }
        else
        {
            EBUS_EVENT(NativeUI::NativeUIRequestBus, DisplayOkDialog, "Kindle Device Detected", "IAP currently unsupported on Kindle devices", false);
        }

        env->DeleteLocalRef(billingClass);
        env->DeleteLocalRef(billingInstance);
        env->DeleteLocalRef(activityObject);
    }

    InAppPurchasesAndroid::~InAppPurchasesAndroid()
    {
        JNIEnv* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
        jclass billingClass = env->GetObjectClass(m_billingInstance);
        jmethodID mid = env->GetMethodID(billingClass, "UnbindService", "()V");
        env->CallVoidMethod(m_billingInstance, mid);

        env->DeleteLocalRef(billingClass);
        env->DeleteGlobalRef(m_billingInstance);
    }

    void InAppPurchasesAndroid::QueryProductInfo(AZStd::vector<AZStd::string>& productIds) const
    {
        JNIEnv* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());

        jobjectArray jproductIds = static_cast<jobjectArray>(env->NewObjectArray(productIds.size(), env->FindClass("java/lang/String"), env->NewStringUTF("")));
        for (int i = 0; i < productIds.size(); i++)
        {
            env->SetObjectArrayElement(jproductIds, i, env->NewStringUTF(productIds[i].c_str()));
        }

        jclass billingClass = env->GetObjectClass(m_billingInstance);
        jmethodID mid = env->GetMethodID(billingClass, "QueryProductInfo", "([Ljava/lang/String;)V");
        env->CallVoidMethod(m_billingInstance, mid, jproductIds);

        env->DeleteLocalRef(jproductIds);
        env->DeleteLocalRef(billingClass);
    }

    void InAppPurchasesAndroid::QueryProductInfo() const
    {
        AZ::IO::FileIOBase* fileReader = AZ::IO::FileIOBase::GetInstance();

        AZStd::string fileBuffer;

        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        AZ::u64 fileSize = 0;
        if (!fileReader->Open("@assets@/product_ids.json", AZ::IO::OpenMode::ModeRead, fileHandle))
        {
            AZ_TracePrintf("LumberyardInAppBilling", "Unable to open file product_ids.json\n");
            return;
        }

        if ((!fileReader->Size(fileHandle, fileSize)) || (fileSize == 0))
        {
            AZ_TracePrintf("LumberyardInAppBilling", "Unable to read file product_ids.json - file truncated\n");
            fileReader->Close(fileHandle);
            return;
        }
        fileBuffer.resize(fileSize);
        if (!fileReader->Read(fileHandle, fileBuffer.data(), fileSize, true))
        {
            fileBuffer.resize(0);
            fileReader->Close(fileHandle);
            AZ_TracePrintf("LumberyardInAppBilling", "Failed to read file product_ids.json\n");
            return;
        }
        fileReader->Close(fileHandle);

        rapidjson::Document document;
        document.Parse(fileBuffer.data());
        if (document.HasParseError())
        {
            const char* errorStr = rapidjson::GetParseError_En(document.GetParseError());
            AZ_TracePrintf("LumberyardInAppBilling", "Failed to parse product_ids.json: %s\n", errorStr);
            return;
        }

        const rapidjson::Value& productList = document["product_ids"];
        const auto& end = productList.End();
        AZStd::vector<AZStd::string> productIds;
        for (auto it = productList.Begin(); it != end; it++)
        {
            const auto& elem = *it;
            productIds.push_back(elem["id"].GetString());
        }

        QueryProductInfo(productIds);
    }

    void InAppPurchasesAndroid::PurchaseProduct(const AZStd::string& productId, const AZStd::string& developerPayload) const
    {
        JNIEnv* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());

        const AZStd::vector <AZStd::unique_ptr<ProductDetails const> >& cachedProductDetails = InAppPurchasesInterface::GetInstance()->GetCache()->GetCachedProductDetails();
        AZStd::string productType = "";
        for (int i = 0; i < cachedProductDetails.size(); i++)
        {
            const ProductDetailsAndroid* productDetails = azrtti_cast<const ProductDetailsAndroid*>(cachedProductDetails[i].get());
            const AZStd::string& cachedProductId = productDetails->GetProductId();
            if (cachedProductId.compare(productId) == 0)
            {
                productType = productDetails->GetProductType();
                break;
            }
        }

        if (productType.empty())
        {
            AZ_TracePrintf("LumberyardInAppBilling", "Failed to find product with id: %s", productId.c_str());
            return;
        }

        jstring jproductId = env->NewStringUTF(productId.c_str());
        jstring jdeveloperPayload = env->NewStringUTF(developerPayload.c_str());
        jstring jproductType = env->NewStringUTF(productType.c_str());
        jint jrequestCode = 0;

        jclass billingClass = env->GetObjectClass(m_billingInstance);
        jmethodID mid = env->GetMethodID(billingClass, "PurchaseProduct", "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
        env->CallVoidMethod(m_billingInstance, mid, jrequestCode, jproductId, jdeveloperPayload, jproductType);

        env->DeleteLocalRef(jproductId);
        env->DeleteLocalRef(jdeveloperPayload);
        env->DeleteLocalRef(jproductType);
        env->DeleteLocalRef(billingClass);
    }

    void InAppPurchasesAndroid::PurchaseProduct(const AZStd::string& productId) const
    {
        PurchaseProduct(productId, "");
    }

    void InAppPurchasesAndroid::QueryPurchasedProducts() const
    {
        JNIEnv* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());

        jclass billingClass = env->GetObjectClass(m_billingInstance);
        jmethodID mid = env->GetMethodID(billingClass, "QueryPurchasedProducts", "()V");
        env->CallVoidMethod(m_billingInstance, mid);

        env->DeleteLocalRef(billingClass);
    }
    
    void InAppPurchasesAndroid::RestorePurchasedProducts() const
    {
        
    }

    void InAppPurchasesAndroid::ConsumePurchase(const AZStd::string& purchaseToken) const
    {
        JNIEnv* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());

        jclass billingClass = env->GetObjectClass(m_billingInstance);
        jmethodID mid = env->GetMethodID(billingClass, "ConsumePurchase", "(Ljava/lang/String;)V");
        jstring jpurchaseToken = env->NewStringUTF(purchaseToken.c_str());
        env->CallVoidMethod(m_billingInstance, mid, jpurchaseToken);

        env->DeleteLocalRef(jpurchaseToken);
        env->DeleteLocalRef(billingClass);
    }

    void InAppPurchasesAndroid::FinishTransaction(const AZStd::string& transactionId, bool downloadHostedContent) const
    {

    }

    InAppPurchasesCache* InAppPurchasesAndroid::GetCache()
    {
        return &m_cache;
    }
}
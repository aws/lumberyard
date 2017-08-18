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

package com.amazon.lumberyard.iap;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.Context;
import android.content.ComponentName;
import android.content.Intent;
import android.content.IntentSender.SendIntentException;
import android.content.res.Resources;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.text.TextUtils;
import android.util.Log;

import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicBoolean;

import org.json.JSONException;
import org.json.JSONObject;

import com.amazon.lumberyard.ActivityResultsListener;

import com.android.vending.billing.IInAppBillingService;


public class LumberyardInAppBilling extends ActivityResultsListener
{
    public LumberyardInAppBilling(Activity activity)
    {
        super(activity);

        m_activity = activity;
        m_context = (Context)activity;

        m_packageName = m_activity.getPackageName();

        Resources resources = m_activity.getResources();
        int stringId = resources.getIdentifier("public_key", "string", m_activity.getPackageName());
        m_appPublicKey = resources.getString(stringId);

        m_setupDone = false;

        Log.d(s_tag, "Instance created");
    }


    public static native void nativeProductInfoRetrieved(String[] productDetails);
    public static native void nativePurchasedProductsRetrieved(String[] purchasedProductDetails, String[] signatures);
    public static native void nativeNewProductPurchased(String[] purchaseReceipt, String[] signature);


    public void Initialize()
    {
        if (m_setupDone)
        {
            Log.d(s_tag, "Already initialized!");
            return;
        }

        m_serviceConnection = new ServiceConnection()
        {
            @Override
            public void onServiceDisconnected(ComponentName name)
            {
                Log.d(s_tag, "Service disconnected");
                m_service = null;
            }

            @Override
            public void onServiceConnected(ComponentName name, IBinder service)
            {
                Log.d(s_tag, "Service connected");
                m_service = IInAppBillingService.Stub.asInterface(service);
                try
                {
                    for (int i = 0; i < productTypes.length; i++)
                    {
                        int isBillingSupported = m_service.isBillingSupported(BILLING_API_VERSION, m_packageName, productTypes[i]);
                        if (isBillingSupported != BILLING_RESPONSE_RESULT_OK)
                        {
                            Log.d(s_tag, m_packageName + " returned error code " + BILLING_RESPONSE_RESULT_STRINGS[isBillingSupported]);
                            return;
                        }
                    }

                    Log.d(s_tag, m_packageName + " supported");
                    m_setupDone = true;
                    m_asyncOperationInProgress = new AtomicBoolean();
                    m_asyncOperationInProgress.set(false);
                }
                catch (RemoteException e)
                {
                    Log.e(s_tag, "RemoteException encountered! " + e.getMessage());
                    e.printStackTrace();
                    return;
                }
            }
        };

        Intent serviceIntent = new Intent("com.android.vending.billing.InAppBillingService.BIND");
        serviceIntent.setPackage("com.android.vending");
        if (!m_context.getPackageManager().queryIntentServices(serviceIntent, 0).isEmpty())
        {
            m_context.bindService(serviceIntent, m_serviceConnection, Context.BIND_AUTO_CREATE);
            Log.d(s_tag, "Service bound to context");
        }
        else
        {
            Log.e(s_tag, "Unable to bind service to context");
            return;
        }

        m_requestQueue = new ArrayList<Request>();
    }


    public void UnbindService()
    {
        if (!m_setupDone)
        {
            Log.e(s_tag, "Not initialized!");
            return;
        }

        m_setupDone = false;
        m_asyncOperationInProgress.set(false);
        m_requestQueue.clear();

        if (m_context != null)
        {
            m_context.unbindService(m_serviceConnection);
            Log.d(s_tag, "Unbinding service from context");
        }
        m_context = null;
        m_serviceConnection = null;
        m_service = null;
    }


    public void QueryProductInfo(final String[] skuListArray)
    {
        if (!m_setupDone)
        {
            Log.e(s_tag, "Not initialized!");
            return;
        }

        if (m_asyncOperationInProgress.compareAndSet(false, true))
        {
            (new Thread(new Runnable()
            {
                public void run()
                {
                    try
                    {
                        ArrayList<String> skuList = new ArrayList<String> ();
                        for (int i = 0; i < skuListArray.length; i++)
                        {
                            skuList.add(skuListArray[i]);
                        }

                        Bundle queryProductDetails = new Bundle();
                        queryProductDetails.putStringArrayList("ITEM_ID_LIST", skuList);

                        ArrayList<String> responseList = new ArrayList<String>();

                        for (int i = 0; i < productTypes.length; i++)
                        {
                            Bundle productDetails = m_service.getSkuDetails(BILLING_API_VERSION, m_packageName, productTypes[i], queryProductDetails);

                            int response = productDetails.getInt("RESPONSE_CODE");
                            if (response != BILLING_RESPONSE_RESULT_OK)
                            {
                                Log.d(s_tag, "Unable to retrieve product details. Error code: " + BILLING_RESPONSE_RESULT_STRINGS[response]);
                                return;
                            }

                            responseList.addAll(productDetails.getStringArrayList("DETAILS_LIST"));
                        }

                        String productDetailsArray[] = responseList.toArray(new String[responseList.size()]);
                        nativeProductInfoRetrieved(productDetailsArray);
                    }
                    catch (RemoteException e)
                    {
                        Log.e(s_tag, "RemoteException encountered! " + e.getMessage());
                        e.printStackTrace();
                    }
                    finally
                    {
                        m_asyncOperationInProgress.set(false);
                        ProcessQueuedRequests();
                    }
                }
            })).start();
        }
        else
        {
            Request newRequest = new Request();
            newRequest.m_operation = "QueryProductInfo";
            newRequest.m_numParameters = skuListArray.length;
            newRequest.m_parameters = new ArrayList<String>();
            for (int i = 0; i < skuListArray.length; i++)
            {
                newRequest.m_parameters.add(skuListArray[i]);
            }
            m_requestQueue.add(newRequest);

            Log.d(s_tag, "Another async operation is in progress. This request has been queued.");
        }
    }


    public void PurchaseProduct(int requestCode, String productSku, String developerPayload, String productType)
    {
        if (!m_setupDone)
        {
            Log.e(s_tag, "Not initialized!");
            return;
        }

        try
        {
            Bundle buyIntentBundle = m_service.getBuyIntent(BILLING_API_VERSION, m_packageName, productSku, productType, developerPayload);
            int buyIntentResponse = buyIntentBundle.getInt("RESPONSE_CODE");
            if (buyIntentResponse != BILLING_RESPONSE_RESULT_OK)
            {
                Log.d(s_tag, "Unable to purchase product with id " + productSku + ". Error Code: " + BILLING_RESPONSE_RESULT_STRINGS[buyIntentResponse]);
                return;
            }

            PendingIntent pendingIntent = buyIntentBundle.getParcelable("BUY_INTENT");
            m_activity.startIntentSenderForResult(pendingIntent.getIntentSender(), requestCode, new Intent(), Integer.valueOf(0), Integer.valueOf(0), Integer.valueOf(0));
        }
        catch (SendIntentException e)
        {
            Log.e(s_tag, "SendIntentException encountered! " + e.getMessage());
            e.printStackTrace();
            return;
        }
        catch (RemoteException e)
        {
            Log.e(s_tag, "RemoteException encountered! " + e.getMessage());
            e.printStackTrace();
            return;
        }

        Log.d(s_tag, "Purchase intent sent successfully");
    }


    public boolean ProcessActivityResult(int requestCode, int resultCode, Intent purchaseData)
    {
        if (purchaseData == null)
        {
            Log.d(s_tag, "Got null intent!");
            return true;
        }

        int response = purchaseData.getIntExtra("RESPONSE_CODE", 0);
        if (resultCode == Activity.RESULT_OK && response == BILLING_RESPONSE_RESULT_OK)
        {
            String purchaseReceipt = purchaseData.getStringExtra("INAPP_PURCHASE_DATA");
            String dataSignature = purchaseData.getStringExtra("INAPP_DATA_SIGNATURE");
            if (purchaseReceipt == null || dataSignature == null)
            {
                Log.d(s_tag, "purchaseData or signature is null! " + purchaseData.getExtras().toString());
                return true;
            }

            String[] purchaseReceiptArray = new String[1];
            String[] purchaseSignatureArray = new String[1];
            purchaseReceiptArray[0] = purchaseReceipt;
            purchaseSignatureArray[0] = dataSignature;
            nativeNewProductPurchased(purchaseReceiptArray, purchaseSignatureArray);
        }
        else if (resultCode == Activity.RESULT_OK)
        {
            Log.d(s_tag, "Returned error code " + BILLING_RESPONSE_RESULT_STRINGS[response]);
            return true;
        }
        else if (resultCode == Activity.RESULT_CANCELED)
        {
            Log.d(s_tag, "Purchase was cancelled. Response: " + BILLING_RESPONSE_RESULT_STRINGS[response]);
            return true;
        }
        else
        {
            Log.d(s_tag, "Purchase failed! Response: " + BILLING_RESPONSE_RESULT_STRINGS[response] + " Result Code: " + Integer.toString(resultCode));
            return true;
        }
        return true;
    }


    public void QueryPurchasedProducts()
    {
        if (!m_setupDone)
        {
            Log.e(s_tag, "Not initialized!");
            return;
        }

        if (m_asyncOperationInProgress.compareAndSet(false, true))
        {
            (new Thread(new Runnable()
            {
                public void run()
                {
                    try
                    {
                        ArrayList<String> purchaseDetails = new ArrayList<String>();
                        ArrayList<String> signatureList = new ArrayList<String>();
                        String continuationToken = null;

                        for (int i = 0; i < productTypes.length; i++)
                        {
                            do
                            {
                                Bundle purchasedItems = m_service.getPurchases(BILLING_API_VERSION, m_packageName, productTypes[i], continuationToken);
                                int response = purchasedItems.getInt("RESPONSE_CODE");
                                if (response != BILLING_RESPONSE_RESULT_OK)
                                {
                                    Log.d(s_tag, "Returned error code " + BILLING_RESPONSE_RESULT_STRINGS[response]);
                                    break;
                                }
                                ArrayList<String> detailsList = purchasedItems.getStringArrayList("INAPP_PURCHASE_DATA_LIST");
                                ArrayList<String> signatures = purchasedItems.getStringArrayList("INAPP_DATA_SIGNATURE_LIST");
                                purchaseDetails.addAll(detailsList);
                                signatureList.addAll(signatures);

                                continuationToken = purchasedItems.getString("INAPP_CONTINUATION_TOKEN");
                            } while(!TextUtils.isEmpty(continuationToken));

                            if (continuationToken != null)
                            {
                                Log.d(s_tag, "Unable to retrieve purchased products!");
                                return;
                            }
                        }

                        String productPurchaseDetailsArray[] = purchaseDetails.toArray(new String[purchaseDetails.size()]);
                        String signatureListArray[] = signatureList.toArray(new String[signatureList.size()]);
                        nativePurchasedProductsRetrieved(productPurchaseDetailsArray, signatureListArray);
                    }
                    catch (RemoteException e)
                    {
                        Log.e(s_tag, "RemoteException encountered! " + e.getMessage());
                        e.printStackTrace();
                    }
                    finally
                    {
                        m_asyncOperationInProgress.set(false);
                        ProcessQueuedRequests();
                    }
                }
            })).start();
        }
        else
        {
            Request newRequest = new Request();
            newRequest.m_operation = "QueryPurchasedProducts";
            newRequest.m_numParameters = 0;
            m_requestQueue.add(newRequest);

            Log.d(s_tag, "Another async operation is in progress. This request has been queued.");
        }
    }


    public void ConsumePurchase(final String purchaseToken)
    {
        if (!m_setupDone)
        {
            Log.e(s_tag, "Not initialized!");
            return;
        }

        if (m_asyncOperationInProgress.compareAndSet(false, true))
        {
            (new Thread(new Runnable()
            {
                public void run()
                {
                    try
                    {
                        int response = m_service.consumePurchase(BILLING_API_VERSION, m_packageName, purchaseToken);
                        if (response != BILLING_RESPONSE_RESULT_OK)
                        {
                            Log.d(s_tag, "Returned error code " + BILLING_RESPONSE_RESULT_STRINGS[response]);
                        }
                    }
                    catch (RemoteException e)
                    {
                        Log.e(s_tag, "RemoteException encountered! " + e.getMessage());
                        e.printStackTrace();
                    }
                    finally
                    {
                        m_asyncOperationInProgress.set(false);
                        ProcessQueuedRequests();
                    }
                }
            })).start();
        }
        else
        {
            Request newRequest = new Request();
            newRequest.m_operation = "ConsumePurchase";
            newRequest.m_numParameters = 1;
            newRequest.m_parameters = new ArrayList<String>();
            newRequest.m_parameters.add(purchaseToken);
            m_requestQueue.add(newRequest);

            Log.d(s_tag, "Another async operation is in progress. This request has been queued.");
        }
    }

    private void ProcessQueuedRequests()
    {
        if (m_requestQueue.size() > 0)
        {
            Request request = m_requestQueue.get(0);
            if (request.m_operation.equals("QueryProductInfo"))
            {
                String skuList[] = request.m_parameters.toArray(new String[request.m_numParameters]);
                QueryProductInfo(skuList);
            }
            else if (request.m_operation.equals("QueryPurchasedProducts"))
            {
                QueryPurchasedProducts();
            }
            else if (request.m_operation.equals("ConsumePurchase"))
            {
                String purchaseToken = request.m_parameters.get(0);
                ConsumePurchase(purchaseToken);
            }
            m_requestQueue.remove(0);
        }
    }


    private class Request
    {
        public String m_operation;
        public int m_numParameters;
        public ArrayList<String> m_parameters;
    };

    private static final int BILLING_RESPONSE_RESULT_OK = 0;
    private static final int BILLING_RESPONSE_RESULT_USER_CANCELED = 1;
    private static final int BILLING_RESPONSE_RESULT_BILLING_UNAVAILABLE = 3;
    private static final int BILLING_RESPONSE_RESULT_ITEM_UNAVAILABLE = 4;
    private static final int BILLING_RESPONSE_RESULT_DEVELOPER_ERROR = 5;
    private static final int BILLING_RESPONSE_RESULT_ERROR = 6;
    private static final int BILLING_RESPONSE_RESULT_ITEM_ALREADY_OWNED = 7;
    private static final int BILLING_RESPONSE_RESULT_ITEM_NOT_OWNED = 8;

    private static final int BILLING_API_VERSION = 3;

    private static final String[] BILLING_RESPONSE_RESULT_STRINGS = {
        "Billing response result is ok",
        "User cancelled the request",
        "The requested service is unavailable",
        "Billing is currently unavailable",
        "The requested item is unavailable",
        "Developer error",
        "Error",
        "The item being purchased is already owned by the user",
        "The item is not owned by the user"
    };

    private static final String s_tag = "LumberyardInAppBilling";

    private static final String[] productTypes = { "inapp", "subs" };

    private Context m_context;

    private Activity m_activity;

    private IInAppBillingService m_service;

    private ServiceConnection m_serviceConnection;

    private String m_appPublicKey;

    private String m_packageName;

    private boolean m_setupDone;

    private AtomicBoolean m_asyncOperationInProgress;

    private ArrayList<Request> m_requestQueue;
}
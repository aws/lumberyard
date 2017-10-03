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
package com.amazon.lumberyard;


import android.app.Activity;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.os.Bundle;
import android.os.Environment;
import android.os.Looper;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;

import java.io.File;
import java.io.InputStream;
import java.io.IOException;
import java.lang.Exception;
import java.lang.InterruptedException;
import java.util.ArrayList;
import java.util.List;

import org.libsdl.app.SDLActivity;

import com.amazon.lumberyard.input.KeyboardHandler;
import com.amazon.lumberyard.input.MotionSensorManager;

import com.amazon.lumberyard.io.APKHandler;
import com.amazon.lumberyard.AndroidDeviceManager;
import com.amazon.lumberyard.io.obb.ObbDownloaderActivity;


////////////////////////////////////////////////////////////////
public class LumberyardActivity extends SDLActivity
{
    private static final int DOWNLOAD_OBB_REQUEST = 1337;

    ////////////////////////////////////////////////////////////////
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        if (m_keyboardHandler != null && keyCode == KeyEvent.KEYCODE_BACK)
        {
            // this is to prevent the native activity from exiting on back button press
            KeyboardHandler.SendKeyCode(keyCode, event.getAction());
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    ////////////////////////////////////////////////////////////////
    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event)
    {
        if (m_keyboardHandler != null && keyCode == KeyEvent.KEYCODE_BACK)
        {
            KeyboardHandler.SendKeyCode(keyCode, event.getAction());
        }
        return super.onKeyUp(keyCode, event);
    }

    ////////////////////////////////////////////////////////////////
    // called from the native to get the application package name
    // e.g. com.lumberyard.samples for SamplesProject
    public String GetPackageName()
    {
        return getApplicationContext().getPackageName();
    }

    ////////////////////////////////////////////////////////////////
    // called from the native to get the app version code
    // android:versionCode in the AndroidManifest.xml.
    public int GetAppVersionCode()
    {
        try
        {
            PackageInfo pInfo = getPackageManager().getPackageInfo(getPackageName(), 0);
            return pInfo.versionCode;
        }
        catch (NameNotFoundException e)
        {
            return 0;
        }
    }

    ////////////////////////////////////////////////////////////////
    // accessor for the native to create and get the KeyboardHandler
    public KeyboardHandler GetKeyboardHandler()
    {
        if (m_keyboardHandler == null)
        {
            m_keyboardHandler = new KeyboardHandler(this);
        }
        return m_keyboardHandler;
    }

    ////////////////////////////////////////////////////////////////
    // accessor for the native to create and get the MotionSensorManager
    public MotionSensorManager GetMotionSensorManager()
    {
        if (m_motionSensorManager == null)
        {
            m_motionSensorManager = new MotionSensorManager(this);
        }
        return m_motionSensorManager;
    }

    ////////////////////////////////////////////////////////////////
    // called from the native code, either right after the renderer is initialized with a splash
    // screen of it's own, or at the end of the system initialization when no native splash
    // is specified
    public void OnEngineRendererTakeover()
    {
        this.runOnUiThread(new Runnable() {
            @Override
            public void run()
            {
                mLayout.removeView(m_splashView);
                m_splashView = null;
            }
        });
    }

    ////////////////////////////////////////////////////////////////
    public void RegisterActivityResultsListener(ActivityResultsListener listener)
    {
        m_activityResultsListeners.add(listener);
    }

    ////////////////////////////////////////////////////////////////
    public void UnregisterActivityResultsListener(ActivityResultsListener listener)
    {
        m_activityResultsListeners.remove(listener);
    }

    ////////////////////////////////////////////////////////////////
    // Starts the download of the obb files and waits (block) until the activity finishes.
    // Return true in case of success, false otherwise.
    public boolean DownloadObb()
    {
        Intent downloadIntent = new Intent(this, ObbDownloaderActivity.class);
        ActivityResult result = new ActivityResult();
        if (launchActivity(downloadIntent, DOWNLOAD_OBB_REQUEST, true, result))
        {
            return result.m_result == Activity.RESULT_OK;
        }

        return false;
    }

    ////////////////////////////////////////////////////////////////
    // Returns the value of a boolean resource.
    public boolean GetBooleanResource(String resourceName)
    {
        Resources resources = this.getResources();
        int resourceId = resources.getIdentifier(resourceName, "bool", this.getPackageName());
        return resources.getBoolean(resourceId);
    }

    // ----

    ////////////////////////////////////////////////////////////////
    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        Resources resources = this.getResources();
        int layoutId = resources.getIdentifier("splash_screen", "layout", this.getPackageName());

        LayoutInflater factory = LayoutInflater.from(this);
        m_splashView = factory.inflate(layoutId, null);
        mLayout.addView(m_splashView);

        if (GetBooleanResource("enable_keep_screen_on"))
        {
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        }

        APKHandler.SetAssetManager(getAssets());

        AndroidDeviceManager.context = this;

        boolean useMainObb = GetBooleanResource("use_main_obb");
        boolean usePatchObb = GetBooleanResource("use_patch_obb");

        if (IsBootstrapInAPK() && (useMainObb || usePatchObb))
        {
            Log.d("LumberyardActivity", "Using OBB expansion files for game assets");

            File obbRootPath = getApplicationContext().getObbDir();

            String packageName = GetPackageName();
            int appVersionCode = GetAppVersionCode();

            String mainObbFilePath = String.format("%s/main.%d.%s.obb", obbRootPath, appVersionCode, packageName);
            String patchObbFilePath = String.format("%s/patch.%d.%s.obb", obbRootPath, appVersionCode, packageName);

            File mainObbFile = new File(mainObbFilePath);
            File patchObbFile = new File(patchObbFilePath);

            boolean needToDownload = (  (useMainObb && !mainObbFile.canRead())
                                     || (usePatchObb && !patchObbFile.canRead()));

            if (needToDownload)
            {
                Log.d("LumberyardActivity", "Attempting to download the OBB expansion files");
                boolean downloadResult = DownloadObb();
                if (!downloadResult)
                {
                    Log.e("LumberyardActivity", "Failed to download the OBB expansion file.  Exiting...");
                    finish();
                }
            }
        }
        else
        {
            Log.d("LumberyardActivity", "Assets already on the device, not using the OBB expansion files.");
        }
    }

    ////////////////////////////////////////////////////////////////
    @Override
    protected void onDestroy()
    {
        // Signal any thread that is waiting for the result of an activity
        for(ActivityResult result : m_waitingResultList)
        {
            synchronized(result)
            {
                result.m_isRunning = false;
                result.notifyAll();
            }
        }
        super.onDestroy();
    }

    ////////////////////////////////////////////////////////////////
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        for (ActivityResultsListener listener : m_activityResultsListeners)
        {
            listener.ProcessActivityResult(requestCode, resultCode, data);
        }
    }

    ////////////////////////////////////////////////////////////////
    private boolean launchActivity(Intent intent, final int activityRequestCode, boolean waitForResult, final ActivityResult result)
    {
        if (waitForResult)
        {
            if (Looper.myLooper() == Looper.getMainLooper())
            {
                // Can't block if we are on the UI Thread.
                return false;
            }

            ActivityResultsListener activityListener = new ActivityResultsListener(this)
            {
                @Override
                public boolean ProcessActivityResult(int requestCode, int resultCode, Intent data)
                {
                    if (requestCode == activityRequestCode)
                    {
                        synchronized(result)
                        {
                            result.m_result = resultCode;
                            result.m_isRunning = false;
                            result.notify();
                        }
                        return true;
                    }

                    return false;
                }
            };

            this.RegisterActivityResultsListener(activityListener);
            m_waitingResultList.add(result);
            result.m_isRunning = true;
            startActivityForResult(intent, activityRequestCode);
            synchronized(result)
            {
                // Wait until the downloader activity finishes.
                boolean ret = true;
                while (result.m_isRunning)
                {
                    try
                    {
                        result.wait();
                    }
                    catch(InterruptedException exception)
                    {
                        ret = false;
                    }
                }
                this.UnregisterActivityResultsListener(activityListener);
                m_waitingResultList.remove(result);
                return ret;
            }
        }
        else
        {
            startActivityForResult(intent, activityRequestCode);
            return true;
        }
    }

    ////////////////////////////////////////////////////////////////
    private boolean IsBootstrapInAPK()
    {
        try
        {
            InputStream bootstrap = getAssets().open("bootstrap.cfg", AssetManager.ACCESS_UNKNOWN);
            bootstrap.close();
            return true;
        }
        catch (IOException exception)
        {
            return false;
        }
    }

    ////////////////////////////////////////////////////////////////
    private class ActivityResult
    {
        public int m_result;
        public boolean m_isRunning;
    }

    // ----

    private View m_splashView = null;
    private KeyboardHandler m_keyboardHandler = null;
    private MotionSensorManager m_motionSensorManager = null;
    private List<ActivityResultsListener> m_activityResultsListeners = new ArrayList<ActivityResultsListener>();
    private List<ActivityResult> m_waitingResultList = new ArrayList<ActivityResult>();;
}

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
import android.app.NativeActivity;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.graphics.Point;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Looper;
import android.util.Log;
import android.view.Display;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.widget.LinearLayout;
import android.widget.PopupWindow;

import java.io.File;
import java.io.InputStream;
import java.io.IOException;
import java.lang.InterruptedException;
import java.util.ArrayList;
import java.util.List;

import com.amazon.lumberyard.io.APKHandler;
import com.amazon.lumberyard.io.obb.ObbDownloaderActivity;


////////////////////////////////////////////////////////////////
public class LumberyardActivity extends NativeActivity
{
    ////////////////////////////////////////////////////////////////
    @Override
    public void onBackPressed()
    {
        // by doing nothing here will prevent the activity from being exited (the default behaviour)
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
    // called from the native code to show the Java splash screen
    public void ShowSplashScreen()
    {
        Log.d(TAG, "ShowSplashScreen called");

        this.runOnUiThread(new Runnable() {
            @Override
            public void run()
            {
                if (!m_splashShowing)
                {
                    ShowSplashScreenImpl();
                }
                else
                {
                    Log.d(TAG, "The splash screen is already showing");
                }
            }
        });
    }

    ////////////////////////////////////////////////////////////////
    // called from the native code to dismiss the Java splash screen
    public void DismissSplashScreen()
    {
        Log.d(TAG, "DismissSplashScreen called");
        if (m_splashShowing)
        {
            this.runOnUiThread(new Runnable() {
                @Override
                public void run()
                {
                    if (m_slashWindow != null)
                    {
                        Log.d(TAG, "Dismissing the splash screen");
                        m_slashWindow.dismiss();
                        m_slashWindow = null;
                    }
                    else
                    {
                        Log.d(TAG, "There is no splash screen to dismiss");
                    }
                }
            });

            m_splashShowing = false;
        }
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

        if (GetBooleanResource("enable_keep_screen_on"))
        {
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        }

        ProcessImmersiveModeSetting();

        APKHandler.SetAssetManager(getAssets());

        AndroidDeviceManager.context = this;

        boolean useMainObb = GetBooleanResource("use_main_obb");
        boolean usePatchObb = GetBooleanResource("use_patch_obb");

        if (IsBootstrapInAPK() && (useMainObb || usePatchObb))
        {
            Log.d(TAG, "Using OBB expansion files for game assets");

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
                Log.d(TAG, "Attempting to download the OBB expansion files");
                boolean downloadResult = DownloadObb();
                if (!downloadResult)
                {
                    Log.e(TAG, "****************************************************************");
                    Log.e(TAG, "Failed to download the OBB expansion file.  Exiting...");
                    Log.e(TAG, "****************************************************************");
                    finish();
                }
            }
        }
        else
        {
            Log.d(TAG, "Assets already on the device, not using the OBB expansion files.");
        }

        // ensure we use the music media stream
        setVolumeControlStream(AudioManager.STREAM_MUSIC);
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

        // Ideally we should be calling super.onDestroy() here and going through the "graceful" shutdown process,
        // however some deadlock(s) happen in the static de-allocation preventing the process to naturally exit.
        // On phones, and most tablets, this doesn't happen because the process is terminated by the system but
        // while running in Samsung DEX mode it's kept alive until it seemingly exits naturally.  Manually killing
        // the process in the onDestroy is probably the best compromise until the graceful exit is fixed with LY-70527
        android.os.Process.killProcess(android.os.Process.myPid());
    }

    ////////////////////////////////////////////////////////////////
    @Override
    public void onWindowFocusChanged(boolean hasFocus)
    {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus)
        {
            ProcessImmersiveModeSetting();
        }
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

    // ----

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
    private void ShowSplashScreenImpl()
    {
        Log.d(TAG, "Showing the Splash Screen");

        // load the splash screen view
        Resources resources = getResources();
        int layoutId = resources.getIdentifier("splash_screen", "layout", getPackageName());

        LayoutInflater factory = LayoutInflater.from(this);
        View splashView = factory.inflate(layoutId, null);

        // get the resolution of the display
        Display display = getWindowManager().getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);

        // create the popup with the splash screen layout. this is because the standard
        // view hierarchy for Android apps doesn't exist when using the NativeActivity
        m_slashWindow = new PopupWindow(splashView, size.x, size.y);
        m_slashWindow.setClippingEnabled(false);

        // add a dummy layout to the main view for the splash popup window
        LinearLayout mainLayout = new LinearLayout(this);
        setContentView(mainLayout);

        // show the splash window
        m_slashWindow.showAtLocation(mainLayout, Gravity.CENTER, 0, 0);
        m_slashWindow.update();

        m_splashShowing = true;
    }

    ////////////////////////////////////////////////////////////////
    private void ProcessImmersiveModeSetting()
    {
        int systemUiFlags = (View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
        if (!GetBooleanResource("disable_immersive_mode"))
        {
            systemUiFlags |= (View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                    View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                    View.SYSTEM_UI_FLAG_FULLSCREEN |
                    View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        }
        getWindow().getDecorView().setSystemUiVisibility(systemUiFlags);
    }

    // ----

    ////////////////////////////////////////////////////////////////
    private class ActivityResult
    {
        public int m_result;
        public boolean m_isRunning;
    }

    // ----

    private static final int DOWNLOAD_OBB_REQUEST = 1337;
    private static final String TAG = "LMBR";

    private PopupWindow m_slashWindow = null;
    private boolean m_splashShowing = false;

    private List<ActivityResultsListener> m_activityResultsListeners = new ArrayList<ActivityResultsListener>();
    private List<ActivityResult> m_waitingResultList = new ArrayList<ActivityResult>();
}

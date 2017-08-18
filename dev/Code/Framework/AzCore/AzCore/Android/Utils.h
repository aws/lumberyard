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

#include <AzCore/std/string/string.h>

#include <jni.h>
#include <android/asset_manager.h>


namespace AZ
{
    namespace Android
    {
        namespace Utils
        {
            //! Request the global reference to the activity class
            jclass GetActivityClassRef();

            //! Request the global reference to the activity instance
            jobject GetActivityRef();

            //! Get the global pointer to the Android asset manager, which is used for APK file i/o.
            AAssetManager* GetAssetManager();

            //! Get the hidden internal storage, typically this is where the application is installed
            //! on the device.
            //! e.g. /data/data/<package_name/files
            const char* GetInternalStoragePath();

            //! Get the root directory for external (or public) storage.
            //! e.g. /storage/sdcard0/, /storage/self/primary/, etc.
            const char* GetExternalStorageRoot();

            //! Get the application specific directory for external (or public) storage.
            //! e.g. <public_storage>/Android/data/<package_name/files
            const char* GetExternalStoragePath();

            //! Get the application specific directory for obb files.
            //! e.g. <public_storage>/Android/obb/<package_name/files
            const char* GetObbStoragePath();

            //! Get the dot separated package name for the current application.
            //! e.g. com.lumberyard.samples for SamplesProject
            const char* GetPackageName();

            //! Get the game project name from the Java string resources.
            const char* GetGameProjectName();

            //! Get the app version code (android:versionCode in the manifest).
            int GetAppVersionCode();

            //! Get the filename of the obb. This doesn't include the path to the obb folder.
            AZStd::string GetObbFileName(bool mainFile);

            //! Get the value of a boolean resource.
            bool GetBooleanResource(const char* resourceName);

            //! Check to see if the path is prefixed with "/APK/"
            bool IsApkPath(const char* filePath);

            //! Will first check to verify the argument is an apk asset path and if so
            //! will strip the prefix from the path.
            //! \return The pointer position of the relative asset path
            const char* StripApkPrefix(const char* filePath);

            //! Search a set of predefined locations where the assets could be on the device
            const char* FindAssetsDirectory();
        }
    }
}

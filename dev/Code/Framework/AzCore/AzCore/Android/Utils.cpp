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
#include "Utils.h"
#include "AndroidEnv.h"

#include <AzCore/Debug/Trace.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/set.h>


namespace AZ
{
    namespace Android
    {
        namespace Utils
        {
            namespace
            {
                ////////////////////////////////////////////////////////////////
                const char* GetApkAssetsPrefix()
                {
                    return "/APK/";
                }
            }


            ////////////////////////////////////////////////////////////////
            jclass GetActivityClassRef()
            {
                return AndroidEnv::Get()->GetActivityClassRef();
            }

            ////////////////////////////////////////////////////////////////
            jobject GetActivityRef()
            {
                return AndroidEnv::Get()->GetActivityRef();
            }

            ////////////////////////////////////////////////////////////////
            AAssetManager* GetAssetManager()
            {
                return AndroidEnv::Get()->GetAssetManager();
            }

            ////////////////////////////////////////////////////////////////
            const char* GetAppPrivateStoragePath()
            {
                return AndroidEnv::Get()->GetAppPrivateStoragePath();
            }

            ////////////////////////////////////////////////////////////////
            const char* GetAppPublicStoragePath()
            {
                return AndroidEnv::Get()->GetAppPublicStoragePath();
            }

            ////////////////////////////////////////////////////////////////
            const char* GetObbStoragePath()
            {
                return AndroidEnv::Get()->GetObbStoragePath();
            }

            ////////////////////////////////////////////////////////////////
            const char* GetPackageName()
            {
                return AndroidEnv::Get()->GetPackageName();
            }

            ////////////////////////////////////////////////////////////////
            int GetAppVersionCode()
            {
                return AndroidEnv::Get()->GetAppVersionCode();
            }

            ////////////////////////////////////////////////////////////////
            const char* GetObbFileName(bool mainFile)
            {
                return AndroidEnv::Get()->GetObbFileName(mainFile);
            }

            ////////////////////////////////////////////////////////////////
            bool IsApkPath(const char* filePath)
            {
                return (strncmp(filePath, GetApkAssetsPrefix(), 4) == 0); // +3 for "APK", +1 for '/' starting slash
            }

            ////////////////////////////////////////////////////////////////
            const char* StripApkPrefix(const char* filePath)
            {
                const int prefixLength = 5; // +3 for "APK", +2 for '/' on either end
                if (!IsApkPath(filePath))
                {
                    return filePath;
                }

                return filePath + prefixLength;
            }

            ////////////////////////////////////////////////////////////////
            const char* FindAssetsDirectory()
            {
            #if !defined(_RELEASE)
                // first check to see if they are in public storage (application specific)
                const char* publicAppStorage = GetAppPublicStoragePath();

                OSString path = OSString::format("%s/bootstrap.cfg", publicAppStorage);
                AZ_TracePrintf("Android::Utils", "Searching for %s", path.c_str());

                FILE* f = fopen(path.c_str(), "r");
                if (f != nullptr)
                {
                    fclose(f);
                    return publicAppStorage;
                }
            #endif // !defined(_RELEASE)

                // if they aren't in public storage, they are in private storage (APK)
                AAssetManager* mgr = GetAssetManager();
                if (mgr)
                {
                    AAsset* asset = AAssetManager_open(mgr, "bootstrap.cfg", AASSET_MODE_UNKNOWN);
                    if (asset)
                    {
                        AAsset_close(asset);
                        return GetApkAssetsPrefix();
                    }
                }

                AZ_Assert(false, "Failed to locate the bootstrap.cfg path");
                return nullptr;
            }
        }
    }
}

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
                    return "APK";
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
            const char* GetInternalStoragePath()
            {
                return AndroidEnv::Get()->GetInternalStoragePath().c_str();
            }

            ////////////////////////////////////////////////////////////////
            const char* GetExternalStorageRoot()
            {
                return AndroidEnv::Get()->GetExternalStorageRoot().c_str();
            }

            ////////////////////////////////////////////////////////////////
            const char* GetExternalStoragePath()
            {
                return AndroidEnv::Get()->GetExternalStoragePath().c_str();
            }

            ////////////////////////////////////////////////////////////////
            const char* GetObbStoragePath()
            {
                return AndroidEnv::Get()->GetObbStoragePath().c_str();
            }

            ////////////////////////////////////////////////////////////////
            const char* GetPackageName()
            {
                return AndroidEnv::Get()->GetPackageName().c_str();
            }

            ////////////////////////////////////////////////////////////////
            const char* GetGameProjectName()
            {
                return AndroidEnv::Get()->GetGameProjectName().c_str();
            }

            ////////////////////////////////////////////////////////////////
            int GetAppVersionCode()
            {
                return AndroidEnv::Get()->GetAppVersionCode();
            }

            ////////////////////////////////////////////////////////////////
            AZStd::string GetObbFileName(bool mainFile)
            {
                return AndroidEnv::Get()->GetObbFileName(mainFile);
            }

            ////////////////////////////////////////////////////////////////
            bool GetBooleanResource(const char* resourceName)
            {
                return AndroidEnv::Get()->GetBooleanResource(resourceName);
            }

            //////////////////////////////////////////////////////////////// 
            bool IsApkPath(const char* filePath)
            {
                return (strstr(filePath, GetApkAssetsPrefix()) != nullptr);
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
                const char* assetsPath = nullptr;

                const char* internalStoragePath = GetInternalStoragePath();
                const char* externalStoragePath = GetExternalStoragePath();
                const char* externalStorageRoot = GetExternalStorageRoot();

                const char* gameProjectName = GetGameProjectName();

                AZ_TracePrintf("Android::Utils", "InternalStoragePath = %s", internalStoragePath);
                AZ_TracePrintf("Android::Utils", "ExternalStoragePath = %s", externalStoragePath);
                AZ_TracePrintf("Android::Utils", "ExternalStorageRoot = %s", externalStorageRoot);

                // Try to figure out where the PAK files are stored
                AZStd::vector<const char*> paths = {
                    internalStoragePath,
                    externalStoragePath,
                    externalStorageRoot,
                };

                AZStd::vector<const char*> validPaths;
                for (size_t i = 0; i < paths.size(); ++i)
                {
                    AZStd::string path = AZStd::string::format("%s/%s/bootstrap.cfg", paths[i], gameProjectName);

                    AZ_TracePrintf("Android::Utils", "Searching for %s", path.c_str());
                    FILE* f = fopen(path.c_str(), "r");
                    if (f != nullptr)
                    {
                        validPaths.push_back(paths[i]);
                        fclose(f);
                    #if defined(_RELEASE)
                        break;
                    #endif
                    }
                }

            #if defined(_RELEASE)
                // if we haven't found the cfg file, try looking inside the APK
                if (validPaths.empty())
            #endif
                {
                    // Try to search assets within apk package.
                    AAssetManager* mgr = GetAssetManager();
                    if (mgr)
                    {
                        AAsset* asset = AAssetManager_open(mgr, "bootstrap.cfg", AASSET_MODE_UNKNOWN);
                        if (asset)
                        {
                            validPaths.push_back(GetApkAssetsPrefix());
                            AAsset_close(asset);
                        }
                    }
                }

                if (validPaths.empty())
                {
                    AZ_Assert(false, "Failed to locate the asset path");
                }
                else
                {
                    assetsPath = validPaths.front();
                }

            #if !defined(_RELEASE)
                // Remove duplicated paths
                auto comp = [](const char* lhs, const char* rhs) { return azstricmp(lhs, rhs) < 0; };
                AZStd::set<const char *, decltype(comp)> validPathsSet(validPaths.begin(), validPaths.end(), comp);
                if (validPathsSet.size() > 1)
                {
                    AZStd::string msg = "Found valid assets in multiple locations: \n";
                    AZStd::for_each(validPathsSet.begin(), validPathsSet.end(), [&msg](const char* path){ msg.append(AZStd::string::format("%s \n", path)); });
                    msg.append(AZStd::string::format("Using %s as root folder.", assetsPath));
                    AZ_Warning("LMBR", false, "%s", msg.c_str());
                }
            #endif
                return assetsPath;
            }
        }
    }
}

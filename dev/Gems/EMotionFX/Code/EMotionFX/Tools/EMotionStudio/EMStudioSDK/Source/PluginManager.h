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

#ifndef __EMSTUDIO_PLUGINMANAGER_H
#define __EMSTUDIO_PLUGINMANAGER_H

// include MCore
#include <MCore/Source/Array.h>
#include <MCore/Source/UnicodeString.h>
#include "EMStudioConfig.h"
#include "EMStudioPlugin.h"

namespace EMStudio
{
    /**
     *
     *
     */
    class EMSTUDIO_API PluginManager
    {
        MCORE_MEMORYOBJECTCATEGORY(PluginManager, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        PluginManager();
        ~PluginManager();

        // overloaded
        bool LoadPlugins(const char* filename);
        void RegisterPlugin(EMStudioPlugin* plugin);
        void LoadPluginsFromDirectory(const char* directory);
        EMStudioPlugin* CreateWindowOfType(const char* pluginType, const char* objectName = nullptr);
        uint32 FindPluginByTypeString(const char* pluginType) const;

        EMStudioPlugin* FindActivePlugin(uint32 classID);   // find first active plugin, or nullptr when not found

        MCORE_INLINE uint32 GetNumPlugins() const                           { return mPlugins.GetLength(); }
        MCORE_INLINE EMStudioPlugin* GetPlugin(const uint32 index)          { return mPlugins[index]; }

        MCORE_INLINE uint32 GetNumActivePlugins() const                     { return mActivePlugins.GetLength(); }
        MCORE_INLINE EMStudioPlugin* GetActivePlugin(const uint32 index)    { return mActivePlugins[index]; }

        uint32 GetNumActivePluginsOfType(const char* pluginType) const;
        uint32 GetNumActivePluginsOfType(uint32 classID) const;
        void RemoveActivePlugin(EMStudioPlugin* plugin);

        QString GenerateObjectName() const;

        void SortActivePlugins()                                            { mActivePlugins.Sort(); }

    private:
        MCore::Array<EMStudioPlugin*>   mPlugins;

        #if defined(MCORE_PLATFORM_WINDOWS)
        MCore::Array<HMODULE>       mPluginLibs;
        #else
        MCore::Array<void*>         mPluginLibs;
        #endif

        MCore::Array<EMStudioPlugin*>   mActivePlugins;

        void UnloadPluginLibs();
    };
}   // namespace EMStudio

#endif

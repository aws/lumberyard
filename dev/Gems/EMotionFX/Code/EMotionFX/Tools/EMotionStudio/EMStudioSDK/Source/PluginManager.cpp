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

// include required headers
#include "EMStudioManager.h"
#include <MysticQt/Source/MysticQtConfig.h>
#include "PluginManager.h"
#include "EMStudioPlugin.h"
#include "DockWidgetPlugin.h"

// include Qt related
#include <QMainWindow>
#include <QDir>
#include <QTime>
#include <QVariant>
#include <QApplication>

// include MCore related
#include <MCore/Source/LogManager.h>

// include mac reladed
#ifdef MCORE_PLATFORM_POSIX
    #include <dlfcn.h>
#endif


namespace EMStudio
{
    // constructor
    PluginManager::PluginManager()
    {
        mPlugins.SetMemoryCategory(MEMCATEGORY_EMSTUDIOSDK);
        mPluginLibs.SetMemoryCategory(MEMCATEGORY_EMSTUDIOSDK);
        mActivePlugins.SetMemoryCategory(MEMCATEGORY_EMSTUDIOSDK);

        mActivePlugins.Reserve(50);
        mPluginLibs.Reserve(10);
        mPlugins.Reserve(50);
    }


    // destructor
    PluginManager::~PluginManager()
    {
        MCore::LogInfo("Unloading plugins");
        UnloadPluginLibs();
    }


    // remove a given active plugin
    void PluginManager::RemoveActivePlugin(EMStudioPlugin* plugin)
    {
        if (mActivePlugins.Find(plugin) == MCORE_INVALIDINDEX32)
        {
            MCore::LogWarning("Failed to remove plugin '%s'", plugin->GetName());
            return;
        }

        const uint32 numPlugins = mActivePlugins.GetLength();
        for (uint32 p = 0; p < numPlugins; ++p)
        {
            mActivePlugins[p]->OnBeforeRemovePlugin(plugin->GetClassID());
        }

        mActivePlugins.RemoveByValue(plugin);
        delete plugin;
    }


    // unload the plugin libraries
    void PluginManager::UnloadPluginLibs()
    {
        // process any remaining events
        QApplication::processEvents();

        // delete all plugins
        const uint32 numPlugins = mPlugins.GetLength();
        for (uint32 i = 0; i < numPlugins; ++i)
        {
            delete mPlugins[i];
        }
        mPlugins.Clear();

        // delete all active plugins
        const uint32 numActivePlugins = mActivePlugins.GetLength();
        if (numActivePlugins > 0)
        {
            // iterate from back to front, destructing the plugins and removing them directly from the array of active plugins
            for (int32 a = numActivePlugins - 1; a >= 0; a--)
            {
                EMStudioPlugin* plugin = mActivePlugins[a];

                const uint32 currentNumPlugins = mActivePlugins.GetLength();
                for (uint32 p = 0; p < currentNumPlugins; ++p)
                {
                    mActivePlugins[p]->OnBeforeRemovePlugin(plugin->GetClassID());
                }

                mActivePlugins.Remove(a);
                delete plugin;
            }

            MCORE_ASSERT(mActivePlugins.GetIsEmpty());
        }

    #if defined(MCORE_PLATFORM_WINDOWS)
        // unload all plugin libs
        const uint32 numLibs = mPluginLibs.GetLength();
        for (uint32 l = 0; l < numLibs; ++l)
        {
            FreeLibrary(mPluginLibs[l]);
        }
        mPluginLibs.Clear();
    #else
        // unload all plugin libs
        const uint32 numLibs = mPluginLibs.GetLength();
        for (uint l = 0; l < numLibs; ++l)
        {
            dlclose(mPluginLibs[l]);
        }
        mPluginLibs.Clear();
    #endif
    }


    // scan for plugins and load them
    void PluginManager::LoadPluginsFromDirectory(const char* directory)
    {
        // scan the directory for .plugin files
        QDir dir(directory);
        dir.setFilter(QDir::Files | QDir::NoSymLinks);
        dir.setSorting(QDir::Name);

        // iterate over all files
        MCore::String filename;
        MCore::String finalFile;
        QFileInfoList list = dir.entryInfoList();
        for (int i = 0; i < list.size(); ++i)
        {
            QFileInfo fileInfo = list.at(i);
            FromQtString(fileInfo.fileName(), &filename);

            // only add files with the .plugin extension
            if (filename.ExtractFileExtension().Lowered() == "plugin")
            {
            #ifdef MCORE_DEBUG
                if (filename.Lowered().Contains("_debug") == false) // skip non-debug versions of the plugins
                {
                    continue;
                }
            #else
                if (filename.Lowered().Contains("_debug"))  // skip debug versions of the plugins
                {
                    continue;
                }
            #endif

                finalFile = directory;
                finalFile += filename;
                LoadPlugins(finalFile.AsChar());
            }
        }
    }


    // load a given plugin library
    bool PluginManager::LoadPlugins(const char* filename)
    {
        MCore::LogInfo("Loading plugins from file '%s'", filename);

        //----------------------------------------
        // Windows
        //----------------------------------------
    #ifdef MCORE_PLATFORM_WINDOWS
        // load this DLL into our address space
        HMODULE dllHandle = ::LoadLibraryA(filename);
        if (dllHandle == nullptr)
        {
            MCore::LogError("Failed to load the plugin library file (code=%d)", GetLastError());
            return false;
        }

        // get the address to the RegisterPlugins function inside the plugin
        typedef void (__cdecl * RegisterFunction)();
        RegisterFunction registerFunction = (RegisterFunction)::GetProcAddress(dllHandle, "RegisterPlugins");
        if (registerFunction == nullptr)
        {
            MCore::LogError("Failed to find the RegisterPlugins function inside this plugin library");
            FreeLibrary(dllHandle);
            return false;
        }

        // execute the register plugins function
        (registerFunction)();

        // Find handle
        mPluginLibs.Add(dllHandle);
    #else
        // load dylib into address space
        void* dylibHandle = dlopen(filename, RTLD_LAZY);
        if (dylibHandle == nullptr)
        {
            MCore::LogError("Failed to load the plugin library file (error: %s)", dlerror());
            return false;
        }

        dlerror();  // clear the error currently set

        // get the address to the RegisterPlugins function inside the plugin
        typedef void (MCORE_CDECL * RegisterPlugins)();
        RegisterPlugins registerPlugins = (RegisterPlugins)dlsym(dylibHandle, "RegisterPlugins");
        //      if (!registerPlugins)
        const char* error = dlerror();
        if (error != 0)
        {
            MCore::LogError("Failed to find the RegisterPlugins function inside this plugin library (code=%s)", error);
            dlclose(dylibHandle);
            return false;
        }

        // execute the register plugins function
        (registerPlugins)();

        // Find handle
        mPluginLibs.Add(dylibHandle);
    #endif

        //----------------------------------------
        // TODO: add other platforms here
        //----------------------------------------

        return true;
    }


    // register the plugin
    void PluginManager::RegisterPlugin(EMStudioPlugin* plugin)
    {
        mPlugins.Add(plugin);
    }


    // create a new active plugin from a given type
    EMStudioPlugin* PluginManager::CreateWindowOfType(const char* pluginType, const char* objectName)
    {
        // try to locate the plugin type
        const uint32 pluginIndex = FindPluginByTypeString(pluginType);
        if (pluginIndex == MCORE_INVALIDINDEX32)
        {
            return nullptr;
        }

        // create the new plugin of this type
        EMStudioPlugin* newPlugin = mPlugins[ pluginIndex ]->Clone();

        // init the plugin
        newPlugin->CreateBaseInterface(objectName);
        newPlugin->Init();
        newPlugin->RegisterKeyboardShortcuts();

        // register as active plugin and return it
        mActivePlugins.Add(newPlugin);
        return newPlugin;
    }


    // find a given plugin by its name (type string)
    uint32 PluginManager::FindPluginByTypeString(const char* pluginType) const
    {
        MCore::String pluginName(pluginType);
        const uint32 numPlugins = mPlugins.GetLength();
        for (uint32 i = 0; i < numPlugins; ++i)
        {
            if (pluginName.CheckIfIsEqualNoCase(mPlugins[i]->GetName()))
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // generate a unique object name
    QString PluginManager::GenerateObjectName() const
    {
        MCore::String randomString;

        // random seed
        qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));

        // repeat until we found a free ID
        for (;; )
        {
            // generate a string from a set of random numbers
            randomString.Format("PLUGIN%d%d%d", qrand(), qrand(), qrand());

            // check if we have a conflict with a current plugin
            bool hasConflict = false;
            const uint32 numActivePlugins = mActivePlugins.GetLength();
            for (uint32 i = 0; i < numActivePlugins; ++i)
            {
                EMStudioPlugin* plugin = mActivePlugins[i];

                // if the object name of a current plugin is equal to the one
                if (plugin->GetHasWindowWithObjectName(randomString))
                {
                    hasConflict = true;
                    break;
                }
            }

            if (hasConflict == false)
            {
                return randomString.AsChar();
            }
        }

        //return QString("INVALID");
    }


    // find the number of active plugins of a given type
    uint32 PluginManager::GetNumActivePluginsOfType(const char* pluginType) const
    {
        uint32 total = 0;

        // build a string object for comparing
        MCore::String pluginTypeString = pluginType;

        // check all active plugins to see if they are from the given type
        const uint32 numActivePlugins = mActivePlugins.GetLength();
        for (uint32 i = 0; i < numActivePlugins; ++i)
        {
            if (pluginTypeString.CheckIfIsEqualNoCase(mActivePlugins[i]->GetName()))
            {
                total++;
            }
        }

        return total;
    }


    // find the first active plugin of a given type
    EMStudioPlugin* PluginManager::FindActivePlugin(uint32 classID)
    {
        const uint32 numActivePlugins = mActivePlugins.GetLength();
        for (uint32 i = 0; i < numActivePlugins; ++i)
        {
            if (mActivePlugins[i]->GetClassID() == classID)
            {
                return mActivePlugins[i];
            }
        }

        return nullptr;
    }



    // find the number of active plugins of a given type
    uint32 PluginManager::GetNumActivePluginsOfType(uint32 classID) const
    {
        uint32 total = 0;

        // check all active plugins to see if they are from the given type
        const uint32 numActivePlugins = mActivePlugins.GetLength();
        for (uint32 i = 0; i < numActivePlugins; ++i)
        {
            if (mActivePlugins[i]->GetClassID() == classID)
            {
                total++;
            }
        }

        return total;
    }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/PluginManager.moc>
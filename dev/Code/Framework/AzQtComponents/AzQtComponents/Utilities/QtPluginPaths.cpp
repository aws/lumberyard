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

#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <QApplication>
#include <QDir>
#include <QSettings>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QJsonValue>


#include <AzCore/base.h>
#include <AzCore/IO/SystemFile.h>

#if defined(WIN32) || defined(WIN64)
#include <Windows.h>
#endif

#if defined(__APPLE__)
// needed for _NSGetExecutablePath
#include <mach-o/dyld.h>
#include <libgen.h>
#include <unistd.h>
#endif

namespace AzQtComponents
{

    // the purpose of this function is to set up the QT globals so that it finds its platform libraries and that kind of thing.
    // these paths have to be set up BEFORE you create the Qt application itself, otherwise it won't know how to work on your current platform
    // since it will be missing the plugin for your current platform (windows/osx/etc)
    void PrepareQtPaths()
    {
#if defined(USE_DEFAULT_QT_LIBRARY_PATHS)
        // Nothing to do
#elif defined(WIN32) || defined(WIN64)
        wchar_t modulePath[_MAX_PATH] = { 0 };
        GetModuleFileNameW(NULL, modulePath, _MAX_PATH);
        wchar_t drive[_MAX_DRIVE] = { 0 };
        wchar_t dir[_MAX_DIR] = { 0 };
        _wsplitpath_s(modulePath, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);
        wcscat_s(dir, L"qtlibs\\plugins");
        wchar_t pluginPath[_MAX_PATH] = { 0 };
        _wmakepath_s(pluginPath, drive, dir, NULL, NULL);
        wchar_t pluginFullPath[_MAX_PATH] = { 0 };
        _wfullpath(pluginFullPath, pluginPath, _MAX_PATH);
        QApplication::addLibraryPath(QString::fromWCharArray(pluginFullPath)); // relative to the executable
#elif defined(__APPLE__)
        char appPath[AZ_MAX_PATH_LEN] = { 0 };
        unsigned int bufSize = AZ_MAX_PATH_LEN;
        _NSGetExecutablePath(appPath, &bufSize);
        const char* exedir = dirname(appPath);
        QString applicationDir = exedir;

        azstrcpy(appPath, AZ_MAX_PATH_LEN, applicationDir.toUtf8().data());
        azstrcat(appPath, AZ_MAX_PATH_LEN, "/../../../qtlibs/plugins");
        QCoreApplication::addLibraryPath(appPath);

        // make it so that it also checks in the real bin64 folder, in case we are in a separate folder.
        azstrcpy(appPath, AZ_MAX_PATH_LEN, applicationDir.toUtf8().data());
        azstrcat(appPath, AZ_MAX_PATH_LEN, "/../../../../BinMac64/qtlibs/plugins");
        QCoreApplication::addLibraryPath(appPath);

        // also add the qtlibs folder directly in case we're already in that folder running outside the app package
        azstrcpy(appPath, AZ_MAX_PATH_LEN, applicationDir.toUtf8().data());
        azstrcat(appPath, AZ_MAX_PATH_LEN, "/qtlibs/plugins");
        QCoreApplication::addLibraryPath(appPath);
#else
#error need to call QApplication::addLibraryPath with the path to where the platform-specific qt plugins folder is.
#endif
    }

    QString FindEngineRootDir(QApplication* app)
    {
        // The QApplication must be initialized before this method is called
        // so it must be passed in as a parameter, even if we don't use it.
        (void)app;

        // Attempt to locate the engine by looking for 'engineroot.txt' and walking up the folder path until it is found (or not)
        QDir appPath(QApplication::applicationDirPath());
        QString engineRootPath;
        while (!appPath.isRoot())
        {
            if (QFile::exists(appPath.filePath("engine.json")))
            {
                engineRootPath = appPath.absolutePath();
                break;
            }
            if (!appPath.cdUp())
            {
                break;
            }
        }

        // If we found the actual root, see if the root should represent the (external) engine root if its defined in engine.json 
        if (!engineRootPath.isEmpty())
        {

            while (true)
            {
                QString engineJsonFilePath(appPath.filePath("engine.json"));

                QFile engineJsonFile(engineJsonFilePath);
                if (!engineJsonFile.open(QIODevice::ReadOnly | QIODevice::Text))
                {
                    // Only proceed if we can read engine.json
                    break;
                }

                QByteArray engineJsonData = engineJsonFile.readAll();
                engineJsonFile.close();
                QJsonDocument engineJsonDoc(QJsonDocument::fromJson(engineJsonData));
                if (engineJsonDoc.isNull())
                {
                    // Only proceed if engine.json is a valid json file
                    break;
                }

                QJsonObject engineJsonRoot = engineJsonDoc.object();
                if (!engineJsonRoot.contains("ExternalEnginePath"))
                {
                    // Only proceed if engine.json has the "ExternalEnginePath" attribute
                    break;
                }

                QJsonValue externalEnginePathObject = engineJsonRoot["ExternalEnginePath"];
                if (!externalEnginePathObject.isString())
                {
                    // Only proceed if engine.json's "ExternalEnginePath" is a string
                    break;
                }

                QString externalEnginePathStr = externalEnginePathObject.toString();
                QDir    externalEnginePathDir = QDir(externalEnginePathStr);
                if (externalEnginePathDir.exists())
                {
                    engineRootPath = externalEnginePathStr;
                }
                break;
            }
        }

        return engineRootPath;

    }
} // namespace AzQtComponents


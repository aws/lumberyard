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
#include "assetUtils.h"

#include <AzCore/Math/Sha1.h>

#include "native/utilities/PlatformConfiguration.h"
#include "native/AssetManager/assetScanner.h"
#include "native/assetprocessor.h"
#include "native/AssetDatabase/AssetDatabase.h"
#include "native/resourcecompiler/rcjob.h"
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QTextStream>
#include <QThread>
#include <QSettings>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>

#if !defined(BATCH_MODE)
#include <QMessageBox>
#endif

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <libgen.h>
#include <unistd.h>
#endif

#include <AzCore/Debug/Trace.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/error/en.h>

#include <AzCore/base.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/Logging/LogLine.h>

#if defined(AZ_PLATFORM_WINDOWS)
#   include <windows.h>
#else
#   include <QFile>
#endif

#if defined(AZ_PLATFORM_APPLE)
#include <fcntl.h>
#endif

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>


namespace AssetUtilsInternal
{
    static const unsigned int g_RetryWaitInterval = 250; // The amount of time that we are waiting for retry.
    // This is because Qt has to init random number gen on each thread.
    AZ_THREAD_LOCAL bool g_hasInitializedRandomNumberGenerator = false;

    // so that even if we do init two seeds at exactly the same msec time, theres still this extra
    // changing number
    static AZStd::atomic_int g_randomNumberSequentialSeed;

    bool FileCopyMoveWithTimeout(QString sourceFile, QString outputFile, bool isCopy, unsigned int waitTimeInSeconds)
    {
        if (waitTimeInSeconds < 0)
        {
            AZ_Warning("Asset Processor", waitTimeInSeconds >= 0, "Invalid timeout specified by the user")
            waitTimeInSeconds = 0;
        }
        bool failureOccurredOnce = false; // used for logging.
        bool operationSucceeded = false;
        QFile outFile(outputFile);
        QElapsedTimer timer;
        timer.start();
        do
        {
            //Removing the old file if it exists
            if (outFile.exists())
            {
                if (!outFile.remove())
                {
                    if (!failureOccurredOnce)
                    {
                        AZ_Warning(AssetProcessor::ConsoleChannel, false, "Warning: Unable to remove file %s to copy source file %s in... (We may retry)\n", outputFile.toUtf8().constData(), sourceFile.toUtf8().constData());
                        failureOccurredOnce = true;
                    }
                    //not able to remove the file
                    if (waitTimeInSeconds != 0)
                    {
                        //Sleep only for non zero waitTime
                        QThread::msleep(AssetUtilsInternal::g_RetryWaitInterval);
                    }
                    continue;
                }
            }

            //ensure that the output dir is present
            QFileInfo outFileInfo(outputFile);
            if (!outFileInfo.absoluteDir().mkpath("."))
            {
                AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Failed to create directory (%s).\n", outFileInfo.absolutePath().toUtf8().data());
                return false;
            }

            if (isCopy && QFile::copy(sourceFile, outputFile))
            {
                //Success
                operationSucceeded = true;
                break;
            }
            else if (!isCopy && QFile::rename(sourceFile, outputFile))
            {
                //Success
                operationSucceeded = true;
                break;
            }
            else
            {
                failureOccurredOnce = true;
                if (waitTimeInSeconds != 0)
                {
                    //Sleep only for non zero waitTime
                    QThread::msleep(AssetUtilsInternal::g_RetryWaitInterval);
                }
            }
        } while (!timer.hasExpired(waitTimeInSeconds * 1000)); //We will keep retrying until the timer has expired the inputted timeout

        if (!operationSucceeded)
        {
            //operation failed for the given timeout
            AZ_Warning(AssetProcessor::ConsoleChannel, false, "WARNING: Could not copy/move source %s to %s, giving up\n", sourceFile.toUtf8().constData(), outputFile.toUtf8().constData());
            return false;
        }
        else if (failureOccurredOnce)
        {
            // if we failed once, write to log to indicate that we eventually succeeded.
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "SUCCESS:  after failure, we later succeeded to copy/move file %s\n", outputFile.toUtf8().constData());
        }

        return true;
    }
}

namespace AssetUtilities
{
    // do not place Qt objects in global scope, they allocate and refcount threaded data.
    char s_gameName[AZ_MAX_PATH_LEN] = { 0 };
    char s_assetRoot[AZ_MAX_PATH_LEN] = { 0 };
    char s_assetServerAddress[AZ_MAX_PATH_LEN] = { 0 };
    char s_cachedEngineRoot[AZ_MAX_PATH_LEN] = { 0 };

    void ResetAssetRoot()
    {
        s_assetRoot[0] = 0;
        s_cachedEngineRoot[0] = 0;
    }

    bool CopyDirectory(QDir source, QDir destination)
    {
        if (!destination.exists())
        {
            bool result = destination.mkpath(".");
            if (!result)
            {
                AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Failed to create directory (%s).\n", destination.absolutePath().toUtf8().data());
                return false;
            }
        }

        QFileInfoList entries = source.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);

        for (const QFileInfo& entry : entries)
        {
            if (entry.isDir())
            {
                //if the entry is a directory than recursively call the function
                if (!CopyDirectory(QDir(source.absolutePath() + "/" + entry.completeBaseName()), QDir(destination.absolutePath() + "/" + entry.completeBaseName())))
                {
                    return false;
                }
            }
            else
            {
                //if the entry is a file than copy it but before than ensure that the destination file is not present
                QString destinationFile = destination.absolutePath() + "/" + entry.fileName();

                if (QFile::exists(destinationFile))
                {
                    if (!QFile::remove(destinationFile))
                    {
                        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Unable to remove file (%s).\n", destinationFile.toUtf8().data());
                        return false;
                    }
                }

                QString sourceFile = source.absolutePath() + "/" + entry.fileName();

                if (!QFile::copy(sourceFile, destinationFile))
                {
                    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Unable to copy sourcefile (%s) to destination (%s).\n", sourceFile.toUtf8().data(), destinationFile.toUtf8().data());
                    return false;
                }
            }
        }

        return true;
    }

    bool ComputeAssetRoot(QDir& root, const QDir* startingRoot)
    {
        if (s_assetRoot[0])
        {
            root = QDir(s_assetRoot);
            return true;
        }

        QString appDir;
        QString appFileName;
        AssetUtilities::ComputeApplicationInformation(appDir, appFileName);
        QDir systemRoot(appDir);

        if (startingRoot)
        {
            // this is for specifying the actual root if you want to.
            systemRoot = *startingRoot;
        }

        while (!systemRoot.isRoot() && !systemRoot.exists("bootstrap.cfg"))
        {
            systemRoot.cdUp();
        }
        if (!systemRoot.exists("bootstrap.cfg"))
        {
#if defined(BATCH_MODE)
            AZ_Warning("AssetProcessor", false, "Failed to find bootstrap.cfg in any folder that is anywhere up the folder tree from where this executable is running from.");
#else
            QMessageBox msgBox;
            msgBox.setText(QCoreApplication::translate("errors", "The system root which contains bootstrap.cfg could not be found."));
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
#endif
            return false;
        }
        else
        {
            azstrcpy(s_assetRoot, AZ_MAX_PATH_LEN, systemRoot.absolutePath().toUtf8().constData());
            root = systemRoot;
            return true;
        }
    }
    //! Get the external engine root folder if the engine is external to the current root folder.
    //! If the current root folder is also the engine folder, then this behaves the same as ComputeEngineRoot
    bool ComputeEngineRoot(QDir& root, const QDir* optionalStartingRoot /*= nullptr*/)
    {
        if (s_cachedEngineRoot[0])
        {
            root = QDir(QString::fromUtf8(s_cachedEngineRoot));
            return true;
        }

        QDir    currentRoot;
        if (!ComputeAssetRoot(currentRoot, optionalStartingRoot))
        {
            return false;
        }

        root = currentRoot;

        const char* engineRoot = nullptr;
        EBUS_EVENT_RESULT(engineRoot, AzToolsFramework::ToolsApplicationRequestBus, GetEngineRootPath);
        if (engineRoot != nullptr)
        {
            root = QDir(QString(engineRoot));
            azstrcpy(s_cachedEngineRoot, AZ_MAX_PATH_LEN, root.absolutePath().toUtf8().constData());
        }

        return true;
    }


    void  ComputeApplicationInformation(QString& dir, QString& filename)
    {
#if defined(__APPLE__)
        char appPath[AZ_MAX_PATH_LEN] = { 0 };
        unsigned int bufSize = AZ_MAX_PATH_LEN;
        _NSGetExecutablePath(appPath, &bufSize);
        const char* exedir = dirname(appPath);
        dir = exedir;
        filename = basename(appPath);
#elif defined(AZ_PLATFORM_WINDOWS)
        // in windows, the qtlibs folder is always in the same folder as the exe.
        // objective: find EXE, add libs!
        // cannot use Qt because QT doesn't exist yet.
        wchar_t currentFileName[AZ_MAX_PATH_LEN] = { 0 };
        wchar_t driveName[AZ_MAX_PATH_LEN] = { 0 };
        wchar_t directoryName[AZ_MAX_PATH_LEN] = { 0 };
        wchar_t fileName[AZ_MAX_PATH_LEN] = { 0 };
        wchar_t fileExtension[AZ_MAX_PATH_LEN] = { 0 };

        GetModuleFileNameW(nullptr, currentFileName, AZ_MAX_PATH_LEN);
        _wsplitpath_s(currentFileName, driveName, AZ_MAX_PATH_LEN, directoryName, AZ_MAX_PATH_LEN, fileName, AZ_MAX_PATH_LEN, fileExtension, AZ_MAX_PATH_LEN);
        _wmakepath_s(currentFileName, driveName, directoryName, nullptr, nullptr);
        dir = QString::fromWCharArray(currentFileName);
        _wmakepath_s(currentFileName, nullptr, nullptr, fileName, fileExtension);
        filename = QString::fromWCharArray(currentFileName);

        // There is a known bug in GetModuleFileNameX which causes the drive letter to lowercase.
        // generally, in windows, drive letters are uppercase (in all presentations of them, such as UI, command line, etc).
        // however, its not actually case sensitive for drive letters.  But we want to remain consistent, so we uppercase it here:
        dir = dir.mid(0, 1).toUpper() + dir.mid(1);
#endif
    }

    void ComputeAppRootAndBinFolderFromApplication(QString& appRoot, QString& filename, QString& binFolder)
    {
        QString appDirStr;
        ComputeApplicationInformation(appDirStr, filename);

        static const char AssetProcessorBundleName[] = "/AssetProcessor.app/Contents/MacOS";
        if (appDirStr.endsWith(AssetProcessorBundleName))
        {
            // We are inside a bundle but the rest of the code expects the appRoot
            // to be outside of that. So go up a few directories to get out of the
            // App Bundle heirarchy...
            appDirStr.chop(static_cast<int>(strlen(AssetProcessorBundleName)));
        }

        QDir rootDir(appDirStr);
        QDir appDir(appDirStr);
        appDir.cdUp();
        binFolder = rootDir.dirName();
        appRoot = appDir.absolutePath();
    }

    bool MakeFileWritable(QString fileName)
    {
#if defined WIN32
        DWORD fileAttributes = GetFileAttributesA(fileName.toUtf8());
        if (fileAttributes == INVALID_FILE_ATTRIBUTES)
        {
            //file does not exist
            return false;
        }
        if ((fileAttributes & FILE_ATTRIBUTE_READONLY))
        {
            fileAttributes = fileAttributes & ~(FILE_ATTRIBUTE_READONLY);
            if (SetFileAttributesA(fileName.toUtf8(), fileAttributes))
            {
                return true;
            }

            return false;
        }
        else
        {
            //file is writeable
            return true;
        }
#else
        QFileInfo fileInfo(fileName);

        if (!fileInfo.exists())
        {
            return false;
        }
        if (fileInfo.permission(QFile::WriteUser))
        {
            // file already has the write permission
            return true;
        }
        else
        {
            QFile::Permissions filePermissions = fileInfo.permissions();
            if (QFile::setPermissions(fileName, filePermissions | QFile::WriteUser))
            {
                //write permission added
                return true;
            }

            return false;
        }
#endif
    }

    bool CheckCanLock(QString fileName)
    {
#if defined(AZ_PLATFORM_WINDOWS)
        AZStd::wstring usableFileName;
        usableFileName.resize(fileName.length(), 0);
        fileName.toWCharArray(usableFileName.data());

        // third parameter dwShareMode (0) prevents share access
        HANDLE fileHandle = CreateFile(usableFileName.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, 0);

        if (fileHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(fileHandle);
            return true;
        }

        return false;
#else

        int handle = open(fileName.toUtf8().constData(), O_RDONLY | O_EXLOCK | O_NONBLOCK);
        if (handle != -1)
        {
            close(handle);
            return true;
        }
        return false;
#endif
    }

    bool InitializeQtLibraries()
    {
        char appPath[AZ_MAX_PATH_LEN] = { 0 };
        char exeDir[AZ_MAX_PATH_LEN] = { 0 };
        QString applicationDir;
        QString applicationFileName;
        ComputeApplicationInformation(applicationDir, applicationFileName);
#if defined(__APPLE__)
#if defined(BATCH_MODE)
        // in batch mode, the executable is in the binaries folder
        // and the 'qtlibs/plugins' folder is reltive to that
        azstrcpy(appPath, AZ_MAX_PATH_LEN, applicationDir.toUtf8().data());
        azstrcat(appPath, AZ_MAX_PATH_LEN, "/qtlibs/plugins");
        QCoreApplication::addLibraryPath(appPath);
#else
        // in GUI mode, the executable could be in an app package, so for example Bin64Mac/AssetProcessor.app/Contents/MacOS/ folder, so we have
        // to go up three folders to get to the binaries folder:
        azstrcpy(appPath, AZ_MAX_PATH_LEN, applicationDir.toUtf8().data());
        azstrcat(appPath, AZ_MAX_PATH_LEN, "/../../../qtlibs/plugins");
        QCoreApplication::addLibraryPath(appPath);

        // make it so that it also checks in the real bin64 folder, in case we are in a seperate folder.
        azstrcpy(appPath, AZ_MAX_PATH_LEN, applicationDir.toUtf8().data());
        azstrcat(appPath, AZ_MAX_PATH_LEN, "/../../../../BinMac64/qtlibs/plugins");
        QCoreApplication::addLibraryPath(appPath);

        // also add the qtlibs folder directly in case we're already in that folder running outside the app package
        azstrcpy(appPath, AZ_MAX_PATH_LEN, applicationDir.toUtf8().data());
        azstrcat(appPath, AZ_MAX_PATH_LEN, "/qtlibs/plugins");
        QCoreApplication::addLibraryPath(appPath);
 #endif
        // finally, fallback for if we're in the bin folder but not bin64.
        azstrcpy(appPath, AZ_MAX_PATH_LEN, applicationDir.toUtf8().data());
        azstrcat(appPath, AZ_MAX_PATH_LEN, "/../BinMac64/qtlibs/plugins");
        QCoreApplication::addLibraryPath(appPath);
#elif defined(_WIN32)
        // in windows, the qtlibs folder is always in the same folder as the exe.
        // objective: find EXE, add libs!
        // cannot use Qt because QT doesn't exist yet.
        char finalName[AZ_MAX_PATH_LEN] = { 0 };
        azstrcpy(appPath, AZ_MAX_PATH_LEN, applicationDir.toUtf8().data());
        azstrcat(appPath, AZ_MAX_PATH_LEN, "qtlibs\\plugins");
        _fullpath(finalName, appPath, AZ_MAX_PATH_LEN);
        QCoreApplication::addLibraryPath(finalName);

        azstrcpy(appPath, AZ_MAX_PATH_LEN, applicationDir.toUtf8().data());
#if (_MSC_VER >= 1910)
        azstrcat(appPath, AZ_MAX_PATH_LEN, "..\\Bin64vc141\\qtlibs\\plugins");
#elif (_MSC_VER >= 1900)
        azstrcat(appPath, AZ_MAX_PATH_LEN, "..\\Bin64vc140\\qtlibs\\plugins");
#else
        azstrcat(appPath, AZ_MAX_PATH_LEN, "..\\Bin64vc120\\qtlibs\\plugins");
#endif
        _fullpath(finalName, appPath, AZ_MAX_PATH_LEN);
        QCoreApplication::addLibraryPath(finalName);

#endif
        return true;
    }

    QString ComputeGameName(QString initialFolder /*= QString()*/, bool force)
    {
        if (force || s_gameName[0] == 0)
        {
            // if its been specified on the command line, then ignore bootstrap:
            QStringList args = QCoreApplication::arguments();
            for (QString arg : args)
            {
                if (arg.contains("/gamefolder=", Qt::CaseInsensitive))
                {
                    QString rawValueString = arg.split("=")[1].trimmed();
                    if (!rawValueString.isEmpty())
                    {
                        azstrcpy(s_gameName, AZ_MAX_PATH_LEN, rawValueString.toUtf8().constData());
                        return rawValueString;
                    }
                }
            }

            if (initialFolder.isEmpty())
            {
                QDir engineRoot;
                if (!AssetUtilities::ComputeEngineRoot(engineRoot))
                {
                    return QString();
                }

                initialFolder = engineRoot.absolutePath();
            }
            azstrcpy(s_gameName, AZ_MAX_PATH_LEN, ReadGameNameFromBootstrap(initialFolder).toUtf8().constData());
        }

        return QString::fromUtf8(s_gameName);
    }

    bool InServerMode()
    {
        static bool s_serverMode = CheckServerMode();
        return s_serverMode;
    }

    bool CheckServerMode()
    {
        QStringList args = QCoreApplication::arguments();
        for (const QString& arg : args)
        {
            if (arg.contains("/server", Qt::CaseInsensitive) || arg.contains("--server", Qt::CaseInsensitive))
            {
                bool isValid = false;
                AssetProcessor::AssetServerBus::BroadcastResult(isValid, &AssetProcessor::AssetServerBusTraits::IsServerAddressValid);
                if (isValid)
                {
                    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Asset Processor is running in server mode.\n");
                    return true;
                }
                else
                {
                    AZ_Warning(AssetProcessor::ConsoleChannel, false, "Invalid server address, please check the AssetProcessorPlatformConfig.ini file \
to ensure that the address is correct. Asset Processor won't be running in server mode.");
                }

                break;
            }
        }

        return false;
    }


    QString ServerAddress()
    {
        if (s_assetServerAddress[0])
        {
            return QString::fromUtf8(s_assetServerAddress);
        }
        // QCoreApplication is not created during unit test mode and that can cause QtWarning to get emitted
        // since we need to retreive arguements from Qt
        if (QCoreApplication::instance())
        {
            // if its been specified on the command line, then ignore AssetProcessorPlatformConfig:
            QStringList args = QCoreApplication::arguments();
            for (QString arg : args)
            {
                if (arg.contains("/serverAddress=", Qt::CaseInsensitive) || arg.contains("--serverAddress=", Qt::CaseInsensitive))
                {
                    QString serverAddress = arg.split("=")[1].trimmed();
                    if (!serverAddress.isEmpty())
                    {
                        azstrcpy(s_assetServerAddress, AZ_MAX_PATH_LEN, serverAddress.toUtf8().constData());
                        return s_assetServerAddress;
                    }
                }
            }
        }

        QDir engineRoot;
        ComputeEngineRoot(engineRoot);
        QString rootConfigFile = engineRoot.absoluteFilePath("AssetProcessorPlatformConfig.ini");

        if (QFile::exists(rootConfigFile))
        {
            QString address;
            QSettings loader(rootConfigFile, QSettings::IniFormat);
            loader.beginGroup("Server");
            address = loader.value("cacheServerAddress", QString()).toString();
            loader.endGroup();
            azstrcpy(s_assetServerAddress, AZ_MAX_PATH_LEN, address.toUtf8().constData());
            return address;
        }

        return QString();
    }

    QString ReadGameNameFromBootstrap(QString initialFolder /*= QString()*/)
    {
        if (initialFolder.isEmpty())
        {
            QDir engineRoot;
            if (!AssetUtilities::ComputeEngineRoot(engineRoot))
            {
                return QString();
            }

            initialFolder = engineRoot.absolutePath();
        }
        // regexp that matches either the beginning of the file, some whitespace, and sys_game_folder, or,
        // matches a newline, then whitespace, then sys_game_folder
        // it will not match comments.
        QRegExp gameFolderPattern("(^|\\n)\\s*sys_game_folder\\s*=\\s*(\\w+)\\b", Qt::CaseInsensitive, QRegExp::RegExp);
        QString gameFolder = ReadPatternFromBootstrap(gameFolderPattern, initialFolder);
        AZ_Warning("AssetUtils", !gameFolder.isEmpty(), "ComputeGameName: sys_game_folder specification in bootstrap.cfg is malformed");
        return gameFolder;
    }

    QString ReadPatternFromBootstrap(QRegExp regExp, QString initialFolder)
    {
        unsigned tries = 0;
        do
        {
            QString prefix = initialFolder + "/" + QString("../").repeated(tries);
            QString bootstrapFilename = prefix + "bootstrap.cfg";
            QFile bootstrap(bootstrapFilename);
            if (bootstrap.open(QIODevice::ReadOnly))
            {
                QString contents(bootstrap.readAll());
                if (contents.length() > 1) // it may just be an EOF marker or CR.
                {
                    int matchIdx = regExp.indexIn(contents);
                    if (matchIdx != -1)
                    {
                        return regExp.cap(2);
                    }
                    else
                    {
                        // the file has data in it, we didn't find the key.
                        // there's no reason to delay further.
                        return QString();
                    }
                }
            }

            bootstrap.close();
            QThread::msleep(AssetUtilsInternal::g_RetryWaitInterval);
        } while (++tries < 3);

        return QString();
    }

    QString ReadWhitelistFromBootstrap(QString initialFolder /*= QString()*/)
    {
        if (initialFolder.isEmpty())
        {
            QDir engineRoot;
            if (!AssetUtilities::ComputeEngineRoot(engineRoot))
            {
                return QString();
            }

            initialFolder = engineRoot.absolutePath();
        }
        // regexp that matches either the beginning of the file, some whitespace, and sys_game_folder, or,
        // matches a newline, then whitespace, then sys_game_folder
        // it will not match comments.
        QRegExp whiteListPattern("(^|\\n)\\s*white_list\\s*=\\s*(.+)", Qt::CaseInsensitive, QRegExp::RegExp);
        QString lineValue;
        unsigned tries = 0;
        do
        {
            QString prefix = initialFolder + "/" + QString("../").repeated(tries);
            QString bootstrapFilename = prefix + "bootstrap.cfg";
            QFile bootstrap(bootstrapFilename);
            if (bootstrap.open(QIODevice::ReadOnly))
            {
                while (!bootstrap.atEnd())
                {
                    QString contents(bootstrap.readLine());
                    int matchIdx = whiteListPattern.indexIn(contents);
                    if (matchIdx != -1)
                    {
                        lineValue = whiteListPattern.cap(2);
                        break;
                    }
                }
            }

            bootstrap.close();
        } while (++tries < 3);

        return lineValue;
    }

    QString ReadRemoteIpFromBootstrap(QString initialFolder /*= QString()*/)
    {
        if (initialFolder.isEmpty())
        {
            QDir engineRoot;
            if (!AssetUtilities::ComputeEngineRoot(engineRoot))
            {
                return QString();
            }

            initialFolder = engineRoot.absolutePath();
        }
        // regexp that matches either the beginning of the file, some whitespace, and sys_game_folder, or,
        // matches a newline, then whitespace, then sys_game_folder
        // it will not match comments.
        QRegExp remoteIpPattern("(^|\\n)\\s*remote_ip\\s*=\\s*(.+)", Qt::CaseInsensitive, QRegExp::RegExp);
        QString lineValue;
        unsigned tries = 0;
        do
        {
            QString prefix = initialFolder + "/" + QString("../").repeated(tries);
            QString bootstrapFilename = prefix + "bootstrap.cfg";
            QFile bootstrap(bootstrapFilename);
            if (bootstrap.open(QIODevice::ReadOnly))
            {
                while (!bootstrap.atEnd())
                {
                    QString contents(bootstrap.readLine());
                    int matchIdx = remoteIpPattern.indexIn(contents);
                    if (matchIdx != -1)
                    {
                        lineValue = remoteIpPattern.cap(2);
                        break;
                    }
                }
            }

            bootstrap.close();
        } while (++tries < 3);

        return lineValue;
    }

    bool WriteWhitelistToBootstrap(QStringList newWhiteList)
    {
        QDir assetRoot;
        ComputeAssetRoot(assetRoot);
        QString bootstrapFilename = assetRoot.filePath("bootstrap.cfg");
        QFile bootstrapFile(bootstrapFilename);

        // do not alter the branch file unless we are able to obtain an exclusive lock.  Other apps (such as NPP) may actually write 0 bytes first, then slowly spool out the remainder)
        if (!CheckCanLock(bootstrapFilename))
        {
            return false;
        }

        if (!bootstrapFile.open(QIODevice::ReadOnly))
        {
            return false;
        }

        // regexp that matches either the beginning of the file, some whitespace, and white_list, or,
        // matches a newline, then whitespace, then white_list it will not match comments.
        QRegExp whiteListPattern("(^|\\n)\\s*white_list\\s*=\\s*(.+)", Qt::CaseInsensitive, QRegExp::RegExp);

        //read the file line by line and try to find the white_list line
        QString readWhiteList;
        QString whiteListline;
        while (!bootstrapFile.atEnd())
        {
            QString contents(bootstrapFile.readLine());
            int matchIdx = whiteListPattern.indexIn(contents);
            if (matchIdx != -1)
            {
                whiteListline = contents;
                readWhiteList = whiteListPattern.cap(2);
                break;
            }
        }

        //read the entire file into so we can do a buffer replacement
        bootstrapFile.seek(0);
        QString fileContents;
        fileContents = bootstrapFile.readAll();
        bootstrapFile.close();

        //format the new white list
        QString formattedNewWhiteList = newWhiteList.join(", ");

        //if we didn't find a white_list entry then append one
        if (whiteListline.isEmpty())
        {
            fileContents.append("\nwhite_list = " + formattedNewWhiteList + "\n");
        }
        else if (QString::compare(formattedNewWhiteList, readWhiteList, Qt::CaseInsensitive) == 0)
        {
            // no need to update, they match
            return true;
        }
        else
        {
            //Replace the found line with a new one
            fileContents.replace(whiteListline, "white_list = " + formattedNewWhiteList + "\n");
        }

        // Make the bootstrap file writable
        if (!MakeFileWritable(bootstrapFilename))
        {
            AZ_Warning(AssetProcessor::ConsoleChannel, false, "Failed to make the bootstrap file writable.")
            return false;
        }
        if (!bootstrapFile.open(QIODevice::WriteOnly))
        {
            return false;
        }

        QTextStream output(&bootstrapFile);
        output << fileContents;
        bootstrapFile.close();
        return true;
    }

    bool WriteRemoteIpToBootstrap(QString newRemoteIp)
    {
        QDir assetRoot;
        ComputeAssetRoot(assetRoot);
        QString bootstrapFilename = assetRoot.filePath("bootstrap.cfg");
        QFile bootstrapFile(bootstrapFilename);

        // do not alter the branch file unless we are able to obtain an exclusive lock.  Other apps (such as NPP) may actually write 0 bytes first, then slowly spool out the remainder)
        if (!CheckCanLock(bootstrapFilename))
        {
            return false;
        }

        if (!bootstrapFile.open(QIODevice::ReadOnly))
        {
            return false;
        }

        // regexp that matches either the beginning of the file, and remote_ip, or,
        // matches a newline, then whitespace, then remote_ip it will not match comments.
        QRegExp remoteIpPattern("(^|\\n)\\s*remote_ip\\s*=\\s*(.+)", Qt::CaseInsensitive, QRegExp::RegExp);

        //read the file line by line and try to find the remote_ip line
        QString readRemoteIp;
        QString remoteIpline;
        while (!bootstrapFile.atEnd())
        {
            QString contents(bootstrapFile.readLine());
            int matchIdx = remoteIpPattern.indexIn(contents);
            if (matchIdx != -1)
            {
                remoteIpline = contents;
                readRemoteIp = remoteIpPattern.cap(2);
                break;
            }
        }

        //read the entire file into so we can do a buffer replacement
        bootstrapFile.seek(0);
        QString fileContents;
        fileContents = bootstrapFile.readAll();
        bootstrapFile.close();

        //if we didn't find a remote_ip entry then append one
        if (remoteIpline.isEmpty())
        {
            fileContents.append("\nremote_ip = " + newRemoteIp + "\n");
        }
        else if (QString::compare(newRemoteIp, readRemoteIp, Qt::CaseInsensitive) == 0)
        {
            // no need to update, they match
            return true;
        }
        else
        {
            //Replace the found line with a new one
            fileContents.replace(remoteIpline, "remote_ip = " + newRemoteIp + "\n");
        }

        // Make the bootstrap file writable
        if (!MakeFileWritable(bootstrapFilename))
        {
            AZ_Warning(AssetProcessor::ConsoleChannel, false, "Failed to make the bootstrap file writable.")
                return false;
        }
        if (!bootstrapFile.open(QIODevice::WriteOnly))
        {
            return false;
        }

        QTextStream output(&bootstrapFile);
        output << fileContents;
        bootstrapFile.close();
        return true;
    }

    quint16 ReadListeningPortFromBootstrap(QString initialFolder /*= QString()*/)
    {
        if (initialFolder.isEmpty())
        {
            QDir engineRoot;
            if (!AssetUtilities::ComputeEngineRoot(engineRoot))
            {
                //return the default port
                return 45643;
            }
            initialFolder = engineRoot.absolutePath();
        }
        // regexp that matches either the beginning of the file, some whitespace, and remote_port, or,
        // matches a newline, then whitespace, then remote_port
        // it will not match comments.
        QRegExp remotePortPattern("(^|\\n)\\s*remote_port\\s*=\\s*(\\w+)\\b", Qt::CaseInsensitive, QRegExp::RegExp);
        QString remotePort = ReadPatternFromBootstrap(remotePortPattern, initialFolder);
        if (remotePort.isEmpty())
        {
            //return the default port
            return 45643;
        }
        else
        {
            return remotePort.toUShort();
        }
    }

    QStringList ReadPlatformsFromCommandLine()
    {
        QStringList args = QCoreApplication::arguments();
        for (QString arg : args)
        {
            if (arg.contains("/platforms=", Qt::CaseInsensitive))
            {
                QString rawPlatformString = arg.split("=")[1];
                return rawPlatformString.split(",");
            }
        }

        return QStringList();
    }

    bool CopyFileWithTimeout(QString sourceFile, QString outputFile, unsigned int waitTimeInSeconds)
    {
        return AssetUtilsInternal::FileCopyMoveWithTimeout(sourceFile, outputFile, true, waitTimeInSeconds);
    }

    bool MoveFileWithTimeout(QString sourceFile, QString outputFile, unsigned int waitTimeInSeconds)
    {
        return AssetUtilsInternal::FileCopyMoveWithTimeout(sourceFile, outputFile, false, waitTimeInSeconds);
    }

    QString NormalizeAndRemoveAlias(QString path)
    {
        QString normalizedPath = NormalizeFilePath(path);
        if (normalizedPath.startsWith("@"))
        {
            int aliasEndIndex = normalizedPath.indexOf("@/", Qt::CaseInsensitive);
            if (aliasEndIndex != -1)
            {
                normalizedPath.remove(0, aliasEndIndex + 2);// adding two to remove both the percentage sign and the native separator
            }
            else
            {
                //try finding the second % index than,maybe the path is like @SomeAlias@somefolder/somefile.ext
                aliasEndIndex = normalizedPath.indexOf("@", 1, Qt::CaseInsensitive);
                if (aliasEndIndex != -1)
                {
                    normalizedPath.remove(0, aliasEndIndex + 1); //adding one to remove the percentage sign only
                }
            }
        }
        return normalizedPath;
    }

    bool ComputeProjectCacheRoot(QDir& projectCacheRoot)
    {
        QDir assetRoot;
        if (!ComputeAssetRoot(assetRoot))
        {
            return false; // failed to detect engine root
        }

        QString gameDir = ComputeGameName(assetRoot.absolutePath());
        if (gameDir.isEmpty())
        {
            return false;
        }

        projectCacheRoot = QDir(assetRoot.filePath("Cache/" + gameDir));
        return true;
    }


    bool ComputeFenceDirectory(QDir& fenceDir)
    {
        QDir cacheRoot;
        if (!AssetUtilities::ComputeProjectCacheRoot(cacheRoot))
        {
            return false;
        }
        fenceDir = QDir(cacheRoot.filePath("fence"));
        return true;
    }

    QString NormalizeFilePath(const QString& filePath)
    {
        // do NOT convert to absolute paths here, we just want to manipulate the string itself.

        // note that according to the Qt Documentation, in QDir::toNativeSeparators,
        // "The returned string may be the same as the argument on some operating systems, for example on Unix.".
        // in other words, what we need here is a custom normalization - we want always the same
        // direction of slashes on all platforms.s

        QString returnString = filePath;
        returnString.replace(QChar('\\'), QChar('/'));
        returnString = QDir::cleanPath(returnString);

#if defined(AZ_PLATFORM_WINDOWS)
        // windows has an additional idiosyncrasy - it returns upper and lower case drive letters
        // from various APIs differently.  we will settle on upper case as the standard.
        if ((returnString.length() > 1) && (returnString[1] == ':'))
        {
            returnString[0] = returnString[0].toUpper();
        }
#endif

        return returnString;
    }

    QString NormalizeDirectoryPath(const QString& directoryPath)
    {
        QString dirPath(NormalizeFilePath(directoryPath));
        while ((dirPath.endsWith('/')))
        {
            dirPath.resize(dirPath.length() - 1);
        }
        return dirPath;
    }

    AZ::Uuid CreateSafeSourceUUIDFromName(const char* sourceName, bool caseInsensitive)
    {
        AZStd::string lowerVersion(sourceName);
        if (caseInsensitive)
        {
            AZStd::to_lower(lowerVersion.begin(), lowerVersion.end());
        }
        AzFramework::StringFunc::Replace(lowerVersion, '\\', '/');
        return AZ::Uuid::CreateName(lowerVersion.c_str());
    }

    void NormalizeFilePaths(QStringList& filePaths)
    {
        for (int pathIdx = 0; pathIdx < filePaths.size(); ++pathIdx)
        {
            filePaths[pathIdx] = NormalizeFilePath(filePaths[pathIdx]);
        }
    }

    unsigned int ComputeCRC32(const char* inString, unsigned int priorCRC)
    {
        AZ::Crc32 crc(priorCRC != -1 ? priorCRC : 0U);
        crc.Add(inString, ::strlen(inString), false);
        return crc;
    }

    unsigned int ComputeCRC32(const char* data, size_t dataSize, unsigned int priorCRC)
    {
        AZ::Crc32 crc(priorCRC != -1 ? priorCRC : 0U);
        crc.Add(data, dataSize, false);
        return crc;
    }

    unsigned int ComputeCRC32Lowercase(const char* inString, unsigned int priorCRC)
    {
        AZ::Crc32 crc(priorCRC != -1 ? priorCRC : 0U);
        crc.Add(inString); // note that the char* version of Add() sets lowercase to be true by default.
        return crc;
    }

    unsigned int ComputeCRC32Lowercase(const char* data, size_t dataSize, unsigned int priorCRC)
    {
        AZ::Crc32 crc(priorCRC != -1 ? priorCRC : 0U);
        crc.Add(data, dataSize, true);
        return crc;
    }

    bool UpdateBranchToken()
    {
        QDir assetRoot;
        ComputeAssetRoot(assetRoot);
        QString bootstrapFilename = assetRoot.filePath("bootstrap.cfg");
        QFile bootstrapFile(bootstrapFilename);
        QString fileContents;

        // do not alter the branch file unless we are able to obtain an exclusive lock.  Other apps (such as NPP) may actually write 0 bytes first, then slowly spool out the remainder)
        QElapsedTimer timer;
        timer.start();
        bool hasLock = false;
        do
        {
            if (CheckCanLock(bootstrapFilename))
            {
                hasLock = true;
                break;
            }

            QThread::msleep(AssetUtilsInternal::g_RetryWaitInterval);

        } while (!timer.hasExpired(10 * AssetUtilsInternal::g_RetryWaitInterval));
        if (!hasLock)
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Unable to lock bootstrap file at: %s\n", bootstrapFilename.toUtf8().constData());
            return false;
        }

        if (!bootstrapFile.open(QIODevice::ReadOnly))
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Unable to open bootstrap file at: %s\n", bootstrapFilename.toUtf8().constData());
            return false;
        }

        fileContents = bootstrapFile.readAll();
        bootstrapFile.close();

        AZStd::string appBranchToken;
        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::CalculateBranchTokenForAppRoot, appBranchToken);
        QString currentBranchToken(appBranchToken.c_str());
        QString readBranchToken;

        // regexp that matches either the beginning of the file, some whitespace, and assetProcessor_branch_token, or,
        // matches a newline, then whitespace, then assetProcessor_branch_token
        // it will not match comments.
        QRegExp branchTokenPattern("(^|\\n)\\s*assetProcessor_branch_token\\s*=\\s*(\\S+)\\b", Qt::CaseInsensitive, QRegExp::RegExp);

        int matchIdx = branchTokenPattern.indexIn(fileContents);
        if (matchIdx != -1)
        {
            readBranchToken = branchTokenPattern.cap(2);
        }

        if (readBranchToken.isEmpty())
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "adding branch token (%s) in (%s)\n", currentBranchToken.toUtf8().constData(), bootstrapFilename.toUtf8().constData());
            fileContents.append("\nassetProcessor_branch_token = " + currentBranchToken + "\n");
        }
        else if (QString::compare(currentBranchToken, readBranchToken, Qt::CaseInsensitive) == 0)
        {
            // no need to update, branch token match
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Branch token (%s) is already correct in (%s)\n", currentBranchToken.toUtf8().constData(), bootstrapFilename.toUtf8().constData());
            return true;
        }
        else
        {
            //Updating branch token
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Updating branch token (%s) in (%s)\n", currentBranchToken.toUtf8().constData(), bootstrapFilename.toUtf8().constData());
            fileContents.replace(branchTokenPattern.cap(0), "\nassetProcessor_branch_token = " + currentBranchToken + "\n");
        }

        // Make the bootstrap file writable
        if (!MakeFileWritable(bootstrapFilename))
        {
            AZ_Warning(AssetProcessor::ConsoleChannel, false, "Failed to make the bootstrap file writable.")
            return false;
        }
        if (!bootstrapFile.open(QIODevice::WriteOnly))
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Unable to open bootstrap file (%s)\n", bootstrapFilename.toUtf8().constData());
            return false;
        }

        QTextStream output(&bootstrapFile);
        output << fileContents;
        bootstrapFile.close();
        return true;
    }

    QString ComputeJobDescription(const AssetProcessor::AssetRecognizer* recognizer)
    {
        return recognizer->m_name.toLower();
    }

    AZStd::string ComputeJobLogFolder()
    {
        return AZStd::string::format("@log@/logs/JobLogs");
    }

    AZStd::string ComputeJobLogFileName(const AzToolsFramework::AssetSystem::JobInfo& jobInfo)
    {
        return AZStd::string::format("%s-%u-%u.log", jobInfo.m_sourceFile.c_str(), jobInfo.GetHash(), jobInfo.m_jobRunKey);
    }

    AZStd::string ComputeJobLogFileName(const AssetBuilderSDK::CreateJobsRequest& createJobsRequest)
    {
        return AZStd::string::format("%s-%s_createJobs.log", createJobsRequest.m_sourceFile.c_str(), createJobsRequest.m_builderid.ToString<AZStd::string>(false).c_str());
    }

    ReadJobLogResult ReadJobLog(AzToolsFramework::AssetSystem::JobInfo& jobInfo, AzToolsFramework::AssetSystem::AssetJobLogResponse& response)
    {
        AZStd::string logFile = AssetUtilities::ComputeJobLogFolder() + "/" + AssetUtilities::ComputeJobLogFileName(jobInfo);
        return ReadJobLog(logFile.c_str(), response);
    }

    ReadJobLogResult ReadJobLog(const char* absolutePath, AzToolsFramework::AssetSystem::AssetJobLogResponse& response)
    {
        response.m_isSuccess = false;
        AZ::IO::HandleType handle = AZ::IO::InvalidHandle;
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
        {
            AZ_TracePrintf("AssetProcessorManager", "Error: AssetProcessorManager: FileIO is unavailable\n", absolutePath);
            response.m_jobLog = "FileIO is unavailable";
            response.m_isSuccess = false;
            return ReadJobLogResult::MissingFileIO;
        }

        if (!fileIO->Open(absolutePath, AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, handle))
        {
            AZ_TracePrintf("AssetProcessorManager", "Error: AssetProcessorManager: Failed to find the log file %s for a request.\n", absolutePath);

            response.m_jobLog.append(AZStd::string::format("Error: No log file found for the given log (%s)", absolutePath).c_str());
            response.m_isSuccess = false;

            return ReadJobLogResult::MissingLogFile;
        }

        AZ::u64 actualSize = 0;
        fileIO->Size(handle, actualSize);

        if (actualSize == 0)
        {
            AZ_TracePrintf("AssetProcessorManager", "Error: AssetProcessorManager: Log File %s is empty.\n", absolutePath);
            response.m_jobLog.append(AZStd::string::format("Error: Log is empty (%s)", absolutePath).c_str());
            response.m_isSuccess = false;
            fileIO->Close(handle);
            return ReadJobLogResult::EmptyLogFile;
        }

        size_t currentResponseSize = response.m_jobLog.size();
        response.m_jobLog.resize(currentResponseSize + actualSize);

        fileIO->Read(handle, response.m_jobLog.data() + currentResponseSize, actualSize);
        fileIO->Close(handle);
        response.m_isSuccess = true;
        return ReadJobLogResult::Success;
    }

    unsigned int GenerateFingerprint(const AssetProcessor::JobDetails& jobDetail)
    {
        // it is assumed that m_fingerprintFilesList contains the original file and all dependencies, and is in a stable order without duplicates
        // CRC32 is not an effective hash for this purpose, so we will build a string and then use SHA1 on it.
        
        // to avoid resizing and copying repeatedly we will keep track of the largest reserved capacity ever needed for this function, and reserve that much data
        static size_t s_largestFingerprintCapacitySoFar = 1;
        AZStd::string fingerprintString;
        fingerprintString.reserve(s_largestFingerprintCapacitySoFar);

        // in general, we'll build a string which is:
        // (version):[Array of individual file fingerprints][Array of individual job fingerprints]
        // with each element of the arrays seperated by colons.

        fingerprintString.append(jobDetail.m_extraInformationForFingerprinting);

        for (const auto& fingerprintFile : jobDetail.m_fingerprintFiles)
        {
            fingerprintString.append(":");
            fingerprintString.append(GetFileFingerprint(fingerprintFile.first, fingerprintFile.second));
        }

        // now the other jobs, which this job depends on:
        for (const AssetProcessor::JobDependencyInternal& jobDependencyInternal : jobDetail.m_jobDependencyList)
        {
            AssetProcessor::JobDesc jobDesc(jobDependencyInternal.m_jobDependency.m_sourceFile.m_sourceFileDependencyPath,
                jobDependencyInternal.m_jobDependency.m_jobKey, jobDependencyInternal.m_jobDependency.m_platformIdentifier);

            for (auto builderIter = jobDependencyInternal.m_builderUuidList.begin(); builderIter != jobDependencyInternal.m_builderUuidList.end(); ++builderIter)
            {
                AZ::u32 dependentJobFingerprint;
                AssetProcessor::ProcessingJobInfoBus::BroadcastResult(dependentJobFingerprint, &AssetProcessor::ProcessingJobInfoBusTraits::GetJobFingerprint, AssetProcessor::JobIndentifier(jobDesc, *builderIter));
                if (dependentJobFingerprint != 0)
                {
                    fingerprintString.append(AZStd::string::format(":%u", dependentJobFingerprint));
                }
            }
        }
        s_largestFingerprintCapacitySoFar = AZStd::GetMax(fingerprintString.capacity(), s_largestFingerprintCapacitySoFar);

        if (fingerprintString.empty())
        {
            AZ_Assert(false, "GenerateFingerprint was called but no input files were requested for fingerprinting.");
            return 0;
        }

        AZ::Sha1 sha;
        sha.ProcessBytes(fingerprintString.data(), fingerprintString.size());
        AZ::u32 digest[5];
        sha.GetDigest(digest);

        return digest[0]; // we only currently use 32-bit hashes.  This could be extended if collisions still occur.
    }

    AZStd::string GetFileFingerprint(const AZStd::string& absolutePath, const AZStd::string& nameToUse)
    {
        QFileInfo fileInfo(QString::fromUtf8(absolutePath.c_str()));
        QDateTime lastModifiedTime = fileInfo.lastModified();
        if (!lastModifiedTime.isValid())
        {
            // we still use the name here so that when missing files change, it still counts as a change.
            // we also don't use '0' as the placeholder, so that there is a difference between files that do not exist
            // and files which have 0 bytes size.
            return AZStd::string::format("-:-:%s", nameToUse.c_str());
        }
        else
        {
            if (lastModifiedTime.isDaylightTime())
            {
                int offsetTimeinSecs = lastModifiedTime.timeZone().daylightTimeOffset(lastModifiedTime);
                lastModifiedTime = lastModifiedTime.addSecs(-1 * offsetTimeinSecs);
            }
            lastModifiedTime.setTimeSpec(Qt::UTC);
            // its possible that the dependency has moved to a different file with the same modtime
            // so we add the size of it too.
            // its also possible that it moved to a different file with the same modtime AND size,
            // but with a different name.  So we add that too.
            return AZStd::string::format("%llX:%llu:%s", lastModifiedTime.toMSecsSinceEpoch(), fileInfo.size(), nameToUse.c_str());
        }
    }

    AZStd::string ComputeJobLogFileName(const AssetProcessor::JobEntry& jobEntry)
    {
        return AZStd::string::format("%s-%u-%u.log", jobEntry.m_databaseSourceName.toUtf8().constData(), jobEntry.GetHash(), jobEntry.m_jobRunKey);
    }

    bool CreateTempRootFolder(QString startFolder, QDir& tempRoot)
    {
        tempRoot.setPath(startFolder);

        if (!tempRoot.exists("AssetProcessorTemp"))
        {
            if (!tempRoot.mkpath("AssetProcessorTemp"))
            {
                AZ_WarningOnce("Asset Utils", false, "Could not create a temp folder at %s", startFolder.toUtf8().constData());
                return false;
            }
        }

        if (!tempRoot.cd("AssetProcessorTemp"))
        {
            AZ_WarningOnce("Asset Utils", false, "Could not access temp folder at %s/AssetProcessorTemp", startFolder.toUtf8().constData());
            return false;
        }

        return true;
    }

    bool CreateTempWorkspace(QString startFolder, QString& result)
    {
        if (!AssetUtilsInternal::g_hasInitializedRandomNumberGenerator)
        {
            AssetUtilsInternal::g_hasInitializedRandomNumberGenerator = true;
            // seed the random number generator a different seed as the main thread.  random numbers are thread-specific.
            // note that 0 is an invalid random seed.
            qsrand(QTime::currentTime().msecsSinceStartOfDay() + AssetUtilsInternal::g_randomNumberSequentialSeed.fetch_add(1) + 1);
        }

        QDir tempRoot;
        
        if (!CreateTempRootFolder(startFolder, tempRoot))
        {
            result.clear();
            return false;
        }

        // try multiple times in the very low chance that its going to be a collision:

        for (int attempts = 0; attempts < 3; ++attempts)
        {
            QTemporaryDir tempDir(tempRoot.absoluteFilePath("JobTemp-XXXXXX"));
            tempDir.setAutoRemove(false);

            if ((tempDir.path().isEmpty()) || (!QDir(tempDir.path()).exists()))
            {
                QByteArray errorData = tempDir.errorString().toUtf8();
                AZ_WarningOnce("Asset Utils", false, "Could not create new temp folder in %s - error from OS is '%s'", tempRoot.absolutePath().toUtf8().constData(), errorData);
                result.clear();
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));
                continue;
            }

            result = tempDir.path();
            break;
        }
        return !result.isEmpty();
    }

    bool CreateTempWorkspace(QString& result)
    {
        // Use the engine root as a temp workspace folder
        // this works better for numerous reasons
        // * Its on the same drive as the /Cache/ so we will be moving files instead of copying from drive to drive
        // * It is discoverable by the user and thus deletable and we can also tell people to send us that folder without them having to go digging for it
        // * If you can't write to it you have much bigger problems

        QDir rootDir;
        if (ComputeAssetRoot(rootDir))
        {
            QString tempPath = rootDir.absolutePath();
            return CreateTempWorkspace(tempPath, result);
        }
        result.clear();
        return false;
    }

    QString GuessProductNameInDatabase(QString path, QString platform, AssetProcessor::AssetDatabaseConnection* databaseConnection)
    {
        QString productName;
        QString inputName;
        QString platformName;
        QString jobDescription;
        QString gameName = AssetUtilities::ComputeGameName();
        AZ::u32 scanFolderID = 0;
        AZ::Uuid guid = AZ::Uuid::CreateNull();

        using namespace AzToolsFramework::AssetDatabase;

        productName = AssetUtilities::NormalizeAndRemoveAlias(path);

        // most of the time, the incoming request will be for an actual product name, so optimize this by assuming that is the case
        // and do an optimized query for it
        if (platform.isEmpty())
        {
            platform = AzToolsFramework::AssetSystem::GetHostAssetPlatform();
        }

        QString platformPrepend = QString("%1/%2/").arg(platform, gameName);
        QString productNameWithPlatformAndGameName = productName;

        if (!productName.startsWith(platformPrepend, Qt::CaseInsensitive))
        {
            productNameWithPlatformAndGameName = productName = QString("%1/%2/%3").arg(platform, gameName, productName);
        }

        ProductDatabaseEntryContainer products;
        if (databaseConnection->GetProductsByProductName(productNameWithPlatformAndGameName, products))
        {
            // if we find stuff, then return immediately, productName is already a productName.
            return productName;
        }

        // if that fails, see at least if it starts with the given product name.

        if (databaseConnection->GetProductsLikeProductName(productName, AssetDatabaseConnection::LikeType::StartsWith, products))
        {
            return productName;
        }

        if (!databaseConnection->GetProductsLikeProductName(productNameWithPlatformAndGameName, AssetDatabaseConnection::LikeType::StartsWith, products))
        {
            //if we are here it means that the asset database does not know about this product,
            //we will now remove the gameName and try again ,so now the path will only have $PLATFORM/ in front of it
            int gameNameIndex = productName.indexOf(gameName, 0, Qt::CaseInsensitive);

            if (gameNameIndex != -1)
            {
                //we will now remove the gameName and the separator
                productName.remove(gameNameIndex, gameName.length() + 1);// adding one for the native separator
            }

            //Search the database for this product
            if (!databaseConnection->GetProductsLikeProductName(productName, AssetDatabaseConnection::LikeType::StartsWith, products))
            {
                //return empty string if the database still does not have any idea about the product
                productName = QString();
            }
        }
        return productName.toLower();
    }

    bool UpdateToCorrectCase(const QString& rootPath, QString& relativePathFromRoot)
    {
        // normalize the input string:
        relativePathFromRoot = NormalizeFilePath(relativePathFromRoot);

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_APPLE_OSX)
        // these operating systems use File Systems which are generally not case sensitive, and so we can make this function "early out" a lot faster.
        // by quickly determining the case where it does not exist at all.  Without this assumption, it can be hundreds of times slower.
        if (!QFile::exists(QDir(rootPath).absoluteFilePath(relativePathFromRoot)))
        {
            return false;
        }
#endif
        
        QStringList tokenized = relativePathFromRoot.split(QChar('/'), QString::SkipEmptyParts);
        QString validatedPath(rootPath);

        bool success = true;

        for (QString& element : tokenized)
        {
            // validate the element:
            QStringList searchPattern;

            QString searchTerm = element;

            // its a wildcard globbing search, and we cannot include terms that have square brackets.
            // and unfortunately, Qt's wildcard globbing does not support escaping characters like [, ], and *
            searchTerm = searchTerm.replace("[", "?");
            searchTerm = searchTerm.replace("]", "?");
            searchTerm = searchTerm.replace("*", "?");

            searchPattern << searchTerm;

            QDir checkDir(validatedPath);

            // note that this specifically does not emit the case sensitive option - so it will find it caselessly.

            bool foundAMatch = false;
            QStringList actualCasing = checkDir.entryList(searchPattern, QDir::Files | QDir::Dirs);
            for (const QString& found : actualCasing)
            {
                if (QString::compare(found, element, Qt::CaseInsensitive) == 0)
                {
                    element = found;
                    foundAMatch = true;
                    break;
                }
            }
            if (!foundAMatch)
            {
                success = false;
                break;
            }
            validatedPath = checkDir.absoluteFilePath(element); // go one step deeper.
        }

        relativePathFromRoot = tokenized.join(QChar('/'));

        return success;
    }

    BuilderFilePatternMatcher::BuilderFilePatternMatcher(const AssetBuilderSDK::AssetBuilderPattern& pattern, const AZ::Uuid& builderDescID)
        : AssetBuilderSDK::FilePatternMatcher(pattern)
        , m_builderDescID(builderDescID)
    {
    }

    BuilderFilePatternMatcher::BuilderFilePatternMatcher(const BuilderFilePatternMatcher& copy)
        : AssetBuilderSDK::FilePatternMatcher(copy)
        , m_builderDescID(copy.m_builderDescID)
    {
    }

    const AZ::Uuid& BuilderFilePatternMatcher::GetBuilderDescID() const
    {
        return this->m_builderDescID;
    };

    QuitListener::QuitListener()
        : m_requestedQuit(false)
    {
    }

    QuitListener::~QuitListener()
    {
        BusDisconnect();
    }

    void QuitListener::ApplicationShutdownRequested()
    {
        m_requestedQuit = true;
    }

    bool QuitListener::WasQuitRequested() const
    {
        return m_requestedQuit;
    }

    JobLogTraceListener::JobLogTraceListener(const AZStd::string& logFileName, AZ::s64 jobKey, bool overwriteLogFile /* = false */)
    {
        m_logFileName = AssetUtilities::ComputeJobLogFolder() + "/" + logFileName;
        m_runKey = jobKey;
        m_forceOverwriteLog = overwriteLogFile;
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }

    JobLogTraceListener::JobLogTraceListener(const AzToolsFramework::AssetSystem::JobInfo& jobInfo, bool overwriteLogFile /* = false */)
    {
        m_logFileName = AssetUtilities::ComputeJobLogFolder() + "/" + AssetUtilities::ComputeJobLogFileName(jobInfo);
        m_runKey = jobInfo.m_jobRunKey;
        m_forceOverwriteLog = overwriteLogFile;
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }

    JobLogTraceListener::JobLogTraceListener(const AssetProcessor::JobEntry& jobEntry, bool overwriteLogFile /* = false */)
    {
        m_logFileName = AssetUtilities::ComputeJobLogFolder() + "/" + AssetUtilities::ComputeJobLogFileName(jobEntry);
        m_runKey = jobEntry.m_jobRunKey;
        m_forceOverwriteLog = overwriteLogFile;
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }

    JobLogTraceListener::~JobLogTraceListener()
    {
        BusDisconnect();
    }

    bool JobLogTraceListener::OnAssert(const char* message)
    {
        if (AssetProcessor::GetThreadLocalJobId() == m_runKey)
        {
            AppendLog(AzFramework::LogFile::SEV_ASSERT, "ASSERT", message);
            return true;
        }

        return false;
    }

    bool JobLogTraceListener::OnException(const char* message)
    {
        if (AssetProcessor::GetThreadLocalJobId() == m_runKey)
        {
            m_inException = true;
            AppendLog(AzFramework::LogFile::SEV_EXCEPTION, "EXCEPTION", message);
            // we return false here so that the main app can also trace it, exceptions are bad enough
            // that we want them to show up in all the logs.
        }

        return false;
    }

    // we want no trace of errors to show up from jobs, inside the console app
    // only in explicit usages, so we return true for pre-error here too
    bool JobLogTraceListener::OnPreError(const char* window, const char* /*file*/, int /*line*/, const char* /*func*/, const char* message)
    {
        if (AssetProcessor::GetThreadLocalJobId() == m_runKey)
        {
            AppendLog(m_inException ? AzFramework::LogFile::SEV_EXCEPTION : AzFramework::LogFile::SEV_ERROR, window, message);
            return true;
        }

        return false;
    }

    bool JobLogTraceListener::OnWarning(const char* window, const char* message)
    {
        if (AssetProcessor::GetThreadLocalJobId() == m_runKey)
        {
            AppendLog(m_inException ? AzFramework::LogFile::SEV_EXCEPTION : AzFramework::LogFile::SEV_WARNING, window, message);
            return true;
        }

        return false;
    }

    bool JobLogTraceListener::OnPrintf(const char* window, const char* message)
    {
        if (AssetProcessor::GetThreadLocalJobId() == m_runKey)
        {
            if (azstrnicmp(window, "debug", 5) == 0)
            {
                AppendLog(AzFramework::LogFile::SEV_DEBUG, window, message);
            }
            else
            {
                AppendLog(m_inException ? AzFramework::LogFile::SEV_EXCEPTION : AzFramework::LogFile::SEV_NORMAL, window, message);
            }

            return true;
        }

        return false;
    }

    void JobLogTraceListener::AppendLog(AzFramework::LogFile::SeverityLevel severity, const char* window, const char* message)
    {
        if (m_isLogging)
        {
            return;
        }

        m_isLogging = true;

        if (!m_logFile)
        {
            m_logFile.reset(new AzFramework::LogFile(m_logFileName.c_str(), m_forceOverwriteLog));
        }
        m_logFile->AppendLog(severity, window, message);
        m_isLogging = false;
    }

    void JobLogTraceListener::AppendLog(AzToolsFramework::Logging::LogLine& logLine)
    {
        using namespace AzToolsFramework;
        using namespace AzFramework;

        if (m_isLogging)
        {
            return;
        }

        m_isLogging = true;

        if (!m_logFile)
        {
            m_logFile.reset(new LogFile(m_logFileName.c_str(), m_forceOverwriteLog));
        }

        LogFile::SeverityLevel severity;

        switch (logLine.GetLogType())
        {
        case Logging::LogLine::TYPE_MESSAGE:
            severity = LogFile::SEV_NORMAL;
            break;
        case Logging::LogLine::TYPE_WARNING:
            severity = LogFile::SEV_WARNING;
            break;
        case Logging::LogLine::TYPE_ERROR:
            severity = LogFile::SEV_ERROR;
            break;
        default:
            severity = LogFile::SEV_DEBUG;
        }

        m_logFile->AppendLog(severity, logLine.GetLogMessage().c_str(), (int)logLine.GetLogMessage().length(),
            logLine.GetLogWindow().c_str(), (int)logLine.GetLogWindow().length(), logLine.GetLogThreadId(), logLine.GetLogTime());
        m_isLogging = false;
    }

} // namespace AssetUtilities

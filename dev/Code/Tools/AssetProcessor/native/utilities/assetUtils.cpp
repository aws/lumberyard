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
#include "AssetUtils.h"

#include "native/utilities/PlatformConfiguration.h"
#include "native/AssetManager/assetScanner.h"
#include "native/assetprocessor.h"
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
#include <AzToolsFramework/Engine/EngineUtilities.h>

#if defined(AZ_PLATFORM_WINDOWS)
#   include <windows.h>
#else
#   include <QFile>
#endif

#if defined(AZ_PLATFORM_APPLE)
#include <fcntl.h>
#endif

#include <AzCore/std/string/wildcard.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>


namespace AssetUtilsInternal
{
    static const unsigned int g_RetryWaitInterval = 250; // The amount of time that we are waiting for retry.
    // This is because Qt has to init random number gen on each thread.
    AZ_THREAD_LOCAL bool g_hasInitializedRandomNumberGenerator = false;

    // so that even if we do init two seeds at exactly the same msec time, theres still this extra
    // changing number
    static AZStd::atomic_int g_randomNumberSequentialSeed; 
    // the Assert Absorber here is used to absorb asserts during regex creation.
    // it only absorbs asserts spawned by this thread;
    class AssertAbsorber
        : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        AssertAbsorber()
        {
            // only absorb asserts when this object is on scope in the thread that this object is on scope in.
            s_onAbsorbThread = true;
            BusConnect();
        }

        bool OnAssert(const char* message)
        {
            if (s_onAbsorbThread)
            {
                m_assertMessage = message;
                return true; // I handled this, do not forward it
            }
            return false;
        }

        ~AssertAbsorber()
        {
            s_onAbsorbThread = false;
        }

        AZStd::string m_assertMessage;
        // only absorb messages for your thread!
        static AZ_THREAD_LOCAL bool s_onAbsorbThread;
    };
    AZ_THREAD_LOCAL bool AssertAbsorber::s_onAbsorbThread = false;

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
            AZ_Warning(AssetProcessor::ConsoleChannel, false, "WARNING: Could not copy/move source %s to %s, giving up\n", outputFile.toUtf8().constData(), sourceFile.toUtf8().constData());
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

    QString GetBranchToken()
    {
        QDir engineRoot;
        ComputeAssetRoot(engineRoot);
        QString token = engineRoot.absolutePath().toLower().replace('/', '_').replace("\\", "_");
        const AZStd::string tokenStr(token.toUtf8().data());
        const AZ::Crc32 branchTokenCrc(tokenStr.c_str(), tokenStr.size(), true);
        char branchToken[12];
        azsnprintf(branchToken, 12, "0x%08X", static_cast<AZ::u32>(branchTokenCrc));
        return QString(branchToken);
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

		AZStd::string assetRoot(currentRoot.absolutePath().toUtf8().data());
		AZ::Outcome<AZStd::string, AZStd::string> outcome = AzToolsFramework::EngineUtilities::ReadEngineConfigurationValue(assetRoot, AZStd::string("ExternalEnginePath"));

		if (outcome.IsSuccess())
		{
			root = QDir(QString(outcome.GetValue().c_str()));
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
#elif defined(_WIN32)
        // in windows, the qtlibs folder is always in the same folder as the exe.
        // objective: find EXE, add libs!
        // cannot use Qt because QT doesn't exist yet.
        char currentFileName[AZ_MAX_PATH_LEN] = { 0 };
        char driveName[AZ_MAX_PATH_LEN] = { 0 };
        char directoryName[AZ_MAX_PATH_LEN] = { 0 };
        char fileName[AZ_MAX_PATH_LEN] = { 0 };
        char fileExtension[AZ_MAX_PATH_LEN] = { 0 };
        GetModuleFileNameA(nullptr, currentFileName, AZ_MAX_PATH_LEN);
        _splitpath_s(currentFileName, driveName, AZ_MAX_PATH_LEN, directoryName, AZ_MAX_PATH_LEN, fileName, AZ_MAX_PATH_LEN, fileExtension, AZ_MAX_PATH_LEN);
        _makepath_s(currentFileName, driveName, directoryName, nullptr, nullptr);
        dir = QString(currentFileName);
        _makepath_s(currentFileName, nullptr, nullptr, fileName, fileExtension);
        filename = QString(currentFileName);
#endif
    }

    void ComputeAppRootAndBinFolderFromApplication(QString& appRoot, QString& filename, QString& binFolder)
    {
        QString appDirStr;
        ComputeApplicationInformation(appDirStr, filename);
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
#if (_MSC_VER == 1800)
        azstrcat(appPath, AZ_MAX_PATH_LEN, "..\\Bin64vc120\\qtlibs\\plugins");
#else
        azstrcat(appPath, AZ_MAX_PATH_LEN, "..\\Bin64vc140\\qtlibs\\plugins");
#endif
        _fullpath(finalName, appPath, AZ_MAX_PATH_LEN);
        QCoreApplication::addLibraryPath(finalName);

#endif
        return true;
    }

    QString ComputeGameName(QString initialFolder  /*= QString()*/, bool force)
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

    QString ComputePlatformName(int platform)
    {
        if (platform & AssetBuilderSDK::Platform_PC)
        {
            return "pc";
        }
        else if (platform & AssetBuilderSDK::Platform_ES3)
        {
            return "es3";
        }
        else if (platform & AssetBuilderSDK::Platform_IOS)
        {
            return "ios";
        }
        else if (platform & AssetBuilderSDK::Platform_OSX)
        {
            return "osx_gl";
        }



        return QString();
    }

    int ComputePlatformFlag(QString platform)
    {
        int platformFlag = AssetBuilderSDK::Platform_NONE;

        if (QString::compare(platform, "pc", Qt::CaseInsensitive) == 0)
        {
            platformFlag = AssetBuilderSDK::Platform_PC;
        }
        else if (QString::compare(platform, "es3", Qt::CaseInsensitive) == 0)
        {
            platformFlag = AssetBuilderSDK::Platform_ES3;
        }
        else if (QString::compare(platform, "ios", Qt::CaseInsensitive) == 0)
        {
            platformFlag = AssetBuilderSDK::Platform_IOS;
        }
        else if (QString::compare(platform, "osx_gl", Qt::CaseInsensitive) == 0)
        {
            platformFlag = AssetBuilderSDK::Platform_OSX;
        }



        return platformFlag;
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
                int matchIdx = regExp.indexIn(contents);
                if (matchIdx != -1)
                {
                    return regExp.cap(2);
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
        // do NOT convert to absolute paths here.
        QString copyPath = filePath;
        copyPath.replace('\\', '/');
        return copyPath;
    }

    QString NormalizeDirectoryPath(const QString& directoryPath)
    {
        QString dirPath(directoryPath);
        dirPath.replace('\\', '/');
        while ((dirPath.endsWith('/')) && (dirPath.length() > 0))
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
        crc.Add(inString);
        return crc;
    }

    unsigned int ComputeCRC32Lowercase(const char* data, size_t dataSize, unsigned int priorCRC)
    {
        AZ::Crc32 crc(priorCRC != -1 ? priorCRC : 0U);
        crc.Add(data, dataSize);
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
        if (!CheckCanLock(bootstrapFilename))
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

        QString currentBranchToken = GetBranchToken();
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

    QString GenerateKeyForSourceFile(QString path, AssetProcessor::PlatformConfiguration* platformConfig)
    {
        //This functions takes an input path like @assets@/Editor/Texture/xxx.dds
        //or an input path like Editor/Texture/xxx.dds
        //or an input path like this D:/x/y/z/Editor/Texture/xxx.tif
        //where Editor is the source folder AP is monitoring
        //and returns a relative file path without file extension like Editor/Texture/xxx

        QString normalizedFilePath = NormalizeFilePath(path);
        QFileInfo fileInfo(normalizedFilePath);
        if (fileInfo.isRelative())
        {
            if (normalizedFilePath.contains("@assets@/", Qt::CaseInsensitive))
            {
                normalizedFilePath.remove("@assets@/", Qt::CaseInsensitive);
            }

            //if the game or editor is asking for a *cm_diff.dds than we need to remove _diff from the name
            if (normalizedFilePath.contains("cm_diff"))
            {
                normalizedFilePath.remove("_diff");
            }
        }
        else
        {
            //if we are here than we need to find relative path
            //from the monitored folder
            QString scanFolderName;
            QString relativePathToFile;
            if (!platformConfig->ConvertToRelativePath(normalizedFilePath, relativePathToFile, scanFolderName))
            {
                AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Unable to find relative path to %s.\n", normalizedFilePath.toUtf8().data());

                return QString();
            }

            normalizedFilePath = relativePathToFile;
        }

        fileInfo.setFile(normalizedFilePath);

        QString fileName = fileInfo.baseName();//if path is /xyz/aaa.b.c filename is aaa

        QDir fileDir(fileInfo.path());
        QString outputString = fileDir.filePath(fileName);

        return outputString.toLower();
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

    unsigned int GenerateFingerprint(const AssetProcessor::JobDetails& jobDetail)
    {
        unsigned int fingerprint = GenerateBaseFingerprint(jobDetail.m_jobEntry.m_absolutePathToFile, jobDetail.m_extraInformationForFingerprinting);

        // If fingerprint is zero, then the file does not exist so abort
        if (fingerprint == 0)
        {
            return 0;
        }

        for (AZStd::string fingerprintFile : jobDetail.m_fingerprintFilesList)
        {
            unsigned int metaDataFingerprint = GenerateBaseFingerprint(fingerprintFile.c_str());

            if (metaDataFingerprint != 0)
            {
                fingerprint = AssetUtilities::ComputeCRC32Lowercase(reinterpret_cast<const char*>(&metaDataFingerprint), sizeof(unsigned int), fingerprint);
            }
        }

        AZ_TracePrintf(AssetProcessor::DebugChannel, "FP:'%s':'%u'\n", jobDetail.m_extraInformationForFingerprinting.toUtf8().data(), fingerprint);

        return fingerprint;
    }

    unsigned int GenerateBaseFingerprint(QString fullPathToFile, QString extraInfo /*= QString()*/)
    {
        QFileInfo fi(fullPathToFile);
        if (!fi.exists())
        {
            return 0;
        }
        unsigned int fingerprint = 0;
        QDateTime mod = fi.lastModified();

        fingerprint = AssetUtilities::ComputeCRC32(extraInfo.toStdString().data(), extraInfo.toStdString().length(), static_cast<unsigned int>(extraInfo.size()));
        qint64 highreztimer = mod.toMSecsSinceEpoch();
        // commented to reduce spam in logs.
        //AZ_TracePrintf(AssetProcessor::DebugChannel, "ms since epoch: %llu", highreztimer);
        fingerprint = AssetUtilities::ComputeCRC32(&highreztimer, sizeof(highreztimer), fingerprint);

        return fingerprint;
    }

    AZStd::string ComputeJobLogFileName(const AssetProcessor::JobEntry& jobEntry)
    {
        return AZStd::string::format("%s-%u-%u.log", jobEntry.m_relativePathToFile.toUtf8().constData(), jobEntry.GetHash(), jobEntry.m_jobRunKey);
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
        QDir tempRoot(startFolder);

        if (!tempRoot.exists("AssetProcessorTemp"))
        {
            if (!tempRoot.mkpath("AssetProcessorTemp"))
            {
                AZ_WarningOnce("Asset Utils", false, "Could not create a temp folder at %s", startFolder.toUtf8().constData());
                result.clear();
                return false;
            }
        }

        if (!tempRoot.cd("AssetProcessorTemp"))
        {
            AZ_WarningOnce("Asset Utils", false, "Could not access temp folder at %s/AssetProcessorTemp", startFolder.toUtf8().constData());
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

    FilePatternMatcher::FilePatternMatcher(const AssetBuilderSDK::AssetBuilderPattern& pattern)
        : m_pattern(pattern)
    {
        if (pattern.m_type == AssetBuilderSDK::AssetBuilderPattern::Regex)
        {
            m_isRegex = true;
            m_isValid = FilePatternMatcher::ValidatePatternRegex(pattern.m_pattern, m_errorString);
            if (m_isValid)
            {
                this->m_regex = RegexType(pattern.m_pattern.c_str(), RegexType::flag_type::icase | RegexType::flag_type::ECMAScript);
            }
        }
        else
        {
            m_isValid = true;
            m_isRegex = false;
        }
    }

    FilePatternMatcher::FilePatternMatcher(const AZStd::string& pattern, AssetBuilderSDK::AssetBuilderPattern::PatternType type)
        : FilePatternMatcher(AssetBuilderSDK::AssetBuilderPattern(pattern, type))
    {
    }

    FilePatternMatcher::FilePatternMatcher(const FilePatternMatcher& copy)
        : m_pattern(copy.m_pattern)
        , m_regex(copy.m_regex)
        , m_isRegex(copy.m_isRegex)
        , m_isValid(copy.m_isValid)
        , m_errorString(copy.m_errorString)
    {
    }

    FilePatternMatcher& AssetUtilities::FilePatternMatcher::operator=(const AssetUtilities::FilePatternMatcher& copy)
    {
        this->m_pattern = copy.m_pattern;
        this->m_regex = copy.m_regex;
        this->m_isRegex = copy.m_isRegex;
        this->m_isValid = copy.m_isValid;
        this->m_errorString = copy.m_errorString;
        return *this;
    }

    bool FilePatternMatcher::MatchesPath(const AZStd::string& assetPath) const
    {
        bool matches = false;
        if (this->m_isRegex)
        {
            matches = AZStd::regex_match(assetPath.c_str(), this->m_regex);
        }
        else
        {
            matches = AZStd::wildcard_match(this->m_pattern.m_pattern, assetPath);
        }
        return matches;
    }

    bool FilePatternMatcher::MatchesPath(const QString& assetPath) const
    {
        return MatchesPath(AZStd::string(assetPath.toUtf8().data()));
    }

    bool FilePatternMatcher::IsValid() const
    {
        return this->m_isValid;
    }

    AZStd::string FilePatternMatcher::GetErrorString() const
    {
        return m_errorString;
    }

    const AssetBuilderSDK::AssetBuilderPattern& FilePatternMatcher::GetBuilderPattern() const
    {
        return this->m_pattern;
    }

    bool FilePatternMatcher::ValidatePatternRegex(const AZStd::string& pattern, AZStd::string& errorString)
    {
        AssetUtilsInternal::AssertAbsorber absorber;
        AZStd::regex validate_regex(pattern.c_str(),
            AZStd::regex::flag_type::icase |
            AZStd::regex::flag_type::ECMAScript);

        return absorber.m_assertMessage.empty();
    }


    BuilderFilePatternMatcher::BuilderFilePatternMatcher(const AssetBuilderSDK::AssetBuilderPattern& pattern, const AZ::Uuid& builderDescID)
        : FilePatternMatcher(pattern)
        , m_builderDescID(builderDescID)
    {
    }

    BuilderFilePatternMatcher::BuilderFilePatternMatcher(const BuilderFilePatternMatcher& copy)
        : FilePatternMatcher(copy)
        , m_builderDescID(copy.m_builderDescID)
    {
    }

    const AZ::Uuid& BuilderFilePatternMatcher::GetBuilderDescID() const
    {
        return this->m_builderDescID;
    };

    QuitListener::QuitListener()
        :m_requestedQuit(false)
    {
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
            AppendLog(AzFramework::LogFile::SEV_EXCEPTION, "EXCEPTION", message);
            return true;
        }

        return false;
    }

    bool JobLogTraceListener::OnError(const char* window, const char* message)
    {
        if (AssetProcessor::GetThreadLocalJobId() == m_runKey)
        {
            AppendLog(AzFramework::LogFile::SEV_ERROR, window, message);
            return true;
        }

        return false;
    }

    bool JobLogTraceListener::OnWarning(const char* window, const char* message)
    {
        if (AssetProcessor::GetThreadLocalJobId() == m_runKey)
        {
            AppendLog(AzFramework::LogFile::SEV_WARNING, window, message);
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
                AppendLog(AzFramework::LogFile::SEV_NORMAL, window, message);
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
} // namespace AssetUtilities

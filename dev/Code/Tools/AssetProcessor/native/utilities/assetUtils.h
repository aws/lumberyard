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

#include <cstdlib> // for size_t
#include <QString>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzFramework/Logging/LogFile.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include "native/assetprocessor.h"
#include "native/utilities/AssetUtilEBusHelper.h"
#include "native/utilities/ApplicationManagerAPI.h"
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>

namespace AzToolsFramework
{
    namespace AssetSystem
    {
        struct JobInfo;
    }
}

class QStringList;
class QDir;

namespace AssetProcessor
{
    class PlatformConfiguration;
    struct AssetRecognizer;
    class JobEntry;
    class AssetDatabaseConnection;
}

namespace AssetUtilities
{
    //! Compute the root asset folder by scanning for marker files such as root.ini
    //! By Default, this searches the applications root and walks upwards, but you are allowed to instead
    //! supply a different starting root.  in that case, it will start from there instead, and walk upwards.
    bool ComputeAssetRoot(QDir& root, const QDir* optionalStartingRoot = nullptr);

    //! Get the engine root folder if the engine is external to the current root folder.  
    //! If the current root folder is also the engine folder, then this behaves the same as ComputeAssetRoot
    bool ComputeEngineRoot(QDir& root, const QDir* optionalStartingRoot = nullptr);

    //! Reset the asset root to not be cached anymore.  Generally only useful for tests
    void ResetAssetRoot();

    //! Copy all files from  the source directory to the destination directory, returns true if successfull, else return false
    bool CopyDirectory(QDir source, QDir destination);

    //! Computes and returns the application directory and filename
    void ComputeApplicationInformation(QString& dir, QString& filename);

    //! Computes and returns the root directory, binfolder, and filename from an application running from a BinXX folder
    void ComputeAppRootAndBinFolderFromApplication(QString& appRoot, QString& filename, QString& binFolder);

    //! makes the file writable
    //! return true if operation is successful, otherwise return false
    bool MakeFileWritable(QString filename);

    //! Check to see if we can Lock the file
    bool CheckCanLock(QString filename);

    bool InitializeQtLibraries();

    //! Updates the branch token in the bootstrap file
    bool UpdateBranchToken();

    //! Determine the name of the current game - for example, SamplesProject
    QString ComputeGameName(QString initialFolder = QString(), bool force = false);

    //! Reads the white list directly from the bootstrap file
    QString ReadWhitelistFromBootstrap(QString initialFolder = QString());

    //! Writes the white list directly to the bootstrap file
    bool WriteWhitelistToBootstrap(QStringList whiteList);

    //! Reads the game name directly from the bootstrap file
    QString ReadGameNameFromBootstrap(QString initialFolder = QString());

    //! Reads a pattern from the bootstrap file
    QString ReadPatternFromBootstrap(QRegExp regExp, QString initialFolder);

    //! Reads the listening port from the bootstrap file
    //! By default the listening port is 45643
    quint16 ReadListeningPortFromBootstrap(QString initialFolder = QString());

    //! Reads platforms from command line
    QStringList ReadPlatformsFromCommandLine();

    //! Copies the sourceFile to the outputFile,returns true if the copy operation succeeds otherwise return false
    //! This function will try deleting the outputFile first,if it exists, before doing the copy operation
    bool CopyFileWithTimeout(QString sourceFile, QString outputFile, unsigned int waitTimeinSeconds = 0);
    //! Moves the sourceFile to the outputFile,returns true if the move operation succeeds otherwise return false
    //! This function will try deleting the outputFile first,if it exists, before doing the move operation
    bool MoveFileWithTimeout(QString sourceFile, QString outputFile, unsigned int waitTimeinSeconds = 0);

    //! Normalize and removes any alias from the path
    QString NormalizeAndRemoveAlias(QString path);

    //! Determine the Job Description for a job, for now it is the name of the recognizer
    QString ComputeJobDescription(const AssetProcessor::AssetRecognizer* recognizer);

    //! Compute the root of the cache for the current project.
    //! This is generally the "cache" folder, subfolder gamedir.
    bool ComputeProjectCacheRoot(QDir& projectCacheRoot);

    //! Compute the folder that will be used for fence files.
    bool ComputeFenceDirectory(QDir& fenceDir);

    //! Converts all slashes to forward slashes, removes double slashes, 
    //! replaces all indirections such as '.' or '..' as appropriate.
    //! On windows, the drive letter (if present) is converted to uppercase.
    //! Besides that, all case is preserved.
    QString NormalizeFilePath(const QString& filePath);
    void NormalizeFilePaths(QStringList& filePaths);

    //! given a directory name, normalize it the same way as the above file path normalizer
    //! does not convert into absolute path - do that yourself before calling this if you want that
    QString NormalizeDirectoryPath(const QString& directoryPath);

    // UUID generation defaults to lowercase SHA1 of the source name, this does normalization and such
    AZ::Uuid CreateSafeSourceUUIDFromName(const char* sourceName, bool caseInsensitive = true);

    //! Compute a CRC given a null-terminated string
    //! @param[in] priorCRC     If supplied, continues an existing CRC by feeding it more data
    unsigned int ComputeCRC32(const char* inString, unsigned int priorCRC = 0xFFFFFFFF);

    //! Compute a CRC given data and a size
    //! @param[in] priorCRC     If supplied, continues an existing CRC by feeding it more data
    unsigned int ComputeCRC32(const char* data, size_t dataSize, unsigned int priorCRC = 0xFFFFFFFF);

    //! Compute a CRC given data and a size
    //! @param[in] priorCRC     If supplied, continues an existing CRC by feeding it more data
    template <typename T>
    unsigned int ComputeCRC32(const T* data, size_t dataSize, unsigned int priorCRC = 0xFFFFFFFF)
    {
        return ComputeCRC32(reinterpret_cast<const char*>(data), dataSize, priorCRC);
    }

    //! Compute a CRC given a null-terminated string
    //! @param[in] priorCRC     If supplied, continues an existing CRC by feeding it more data
    unsigned int ComputeCRC32Lowercase(const char* inString, unsigned int priorCRC = 0xFFFFFFFF);

    //! Compute a CRC given data and a size
    //! @param[in] priorCRC     If supplied, continues an existing CRC by feeding it more data
    unsigned int ComputeCRC32Lowercase(const char* data, size_t dataSize, unsigned int priorCRC = 0xFFFFFFFF);

    //! Compute a CRC given data and a size
    //! @param[in] priorCRC     If supplied, continues an existing CRC by feeding it more data
    template <typename T>
    unsigned int ComputeCRC32Lowercase(const T* data, size_t dataSize, unsigned int priorCRC = 0xFFFFFFFF)
    {
        return ComputeCRC32Lowercase(reinterpret_cast<const char*>(data), dataSize, priorCRC);
    }

    //! attempt to create a workspace for yourself to use as scratch-space, at that starting root folder.
    //! If it succeeds, it will return true and set the result to the final absolute folder name.
    //! this includes creation of temp folder with numbered/lettered temp characters in it.
    //! Note that its up to you to clean this temp workspace up.  It will not automatically be deleted!
    //! If you fail to delete the temp workspace, it will eventually fill the folder up and cause problems.
    bool CreateTempWorkspace(QString startFolder, QString& result);

    //! Create a temp workspace in a default location
    //! If it succeeds, it will return true and set the result to the final absolute folder name.
    //! If it fails, it will return false and result will be an empty string
    //! Note that its up to you to clean this temp workspace up.  It will not automatically be deleted!
    //! If you fail to delete the temp workspace, it will eventually fill the folder up and cause problems.
    bool CreateTempWorkspace(QString& result);

    bool CreateTempRootFolder(QString startFolder, QDir& tempRoot);

    AZStd::string ComputeJobLogFolder();
    AZStd::string ComputeJobLogFileName(const AzToolsFramework::AssetSystem::JobInfo& jobInfo);
    AZStd::string ComputeJobLogFileName(const AssetProcessor::JobEntry& jobEntry);
    AZStd::string ComputeJobLogFileName(const AssetBuilderSDK::CreateJobsRequest& createJobsRequest);

    enum class ReadJobLogResult
    {
        Success,
        MissingFileIO,
        MissingLogFile,
        EmptyLogFile,
    };

    ReadJobLogResult ReadJobLog(AzToolsFramework::AssetSystem::JobInfo& jobInfo, AzToolsFramework::AssetSystem::AssetJobLogResponse& response);
    ReadJobLogResult ReadJobLog(const char* absolutePath, AzToolsFramework::AssetSystem::AssetJobLogResponse& response);

    //! interrogate a given file, which is specified as a full path name, and generate a fingerprint for it.
    unsigned int GenerateFingerprint(const AssetProcessor::JobDetails& jobDetail);
    
    // Generates a fingerprint string based on details of the file, will return the string "0" if the file does not exist.
    // note that the 'name to use' can be blank, but it used to disambiguate between files that have the same
    // modtime and size.
    AZStd::string GetFileFingerprint(const AZStd::string& absolutePath, const AZStd::string& nameToUse);

    QString GuessProductNameInDatabase(QString path, QString platform, AssetProcessor::AssetDatabaseConnection* databaseConnection);

    /** A utility function which checks the given path starting at the root and updates the relative path to be the actual case correct path.
    * For example, if you pass it "c:\lumberyard\dev" as the root and "editor\icons\whatever.ico" as the relative path.
    * It may update relativePathFromRoot to be "Editor\Icons\Whatever.ico" if such a casing is the actual physical case on disk already.
    * @param rootPath a trusted already-case-correct path (will not be case corrected).
    * @param relativePathFromRoot a non-trusted (may be incorrect case) path relative to rootPath, 
    *        which will be normalized and updated to be correct casing.
    * @return if such a file does NOT exist, it returns FALSE, else returns TRUE.
    * @note A very expensive function!  Call sparingly.
    */
    bool UpdateToCorrectCase(const QString& rootPath, QString& relativePathFromRoot);

    class BuilderFilePatternMatcher
        : public AssetBuilderSDK::FilePatternMatcher
    {
    public:
        BuilderFilePatternMatcher() = default;
        BuilderFilePatternMatcher(const BuilderFilePatternMatcher& copy);
        BuilderFilePatternMatcher(const AssetBuilderSDK::AssetBuilderPattern& pattern, const AZ::Uuid& builderDescID);

        const AZ::Uuid& GetBuilderDescID() const;
    protected:
        AZ::Uuid    m_builderDescID;
    };

    //! QuitListener is an utility class that can be used to listen for application quit notification
    class QuitListener
        : public AssetProcessor::ApplicationManagerNotifications::Bus::Handler
    {
    public:

        QuitListener();
        ~QuitListener();
        /// ApplicationManagerNotifications::Bus::Handler
        void ApplicationShutdownRequested() override;

        bool WasQuitRequested() const;

    private:
        AZStd::atomic<bool> m_requestedQuit;
    };

    //! JobLogTraceListener listens for job messages
    class JobLogTraceListener
        : public AZ::Debug::TraceMessageBus::Handler
    {
    public:

        JobLogTraceListener(const AZStd::string& logFileName, AZ::s64 jobKey, bool overwriteLogFile = false);

        JobLogTraceListener(const AzToolsFramework::AssetSystem::JobInfo& jobInfo, bool overwriteLogFile = false);

        JobLogTraceListener(const AssetProcessor::JobEntry& jobEntry, bool overwriteLogFile = false);

        ~JobLogTraceListener();

        //////////////////////////////////////////////////////////////////////////
        // AZ::Debug::TraceMessagesBus - we actually ignore all outputs except those for our ID.

        bool OnAssert(const char* message) override;
        bool OnException(const char* message) override;
        bool OnPreError(const char* window, const char* file, int line, const char* func, const char* message) override;
        bool OnWarning(const char* window, const char* message) override;
        //////////////////////////////////////////////////////////////////////////

        bool OnPrintf(const char* window, const char* message) override;
        //////////////////////////////////////////////////////////////////////////

    private:
        AZStd::unique_ptr<AzFramework::LogFile> m_logFile;
        AZStd::string m_logFileName;
        AZ::s64 m_runKey = 0;
        // using m_isLogging bool to prevent an infinite loop which can happen if an error/warning happens when trying to create an invalid logFile,
        // because it will cause the appendLog function to be called again, which will again try to create that log file.
        bool m_isLogging = false;
        bool m_inException = false;
        //! If true, log file will be overwritten instead of appended
        bool m_forceOverwriteLog = false;

        void AppendLog(AzFramework::LogFile::SeverityLevel severity, const char* window, const char* message);
    };
} // namespace AssetUtilities

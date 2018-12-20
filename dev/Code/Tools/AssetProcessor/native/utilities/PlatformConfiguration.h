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
#ifndef PLATFORMCONFIGURATION_H
#define PLATFORMCONFIGURATION_H

#include <QList>
#include <QString>
#include <QObject>
#include <QHash>
#include <QRegExp>
#include <QPair>
#include <QVector>
#include <QSet>

#include <AzCore/std/string/string.h>
#include <native/utilities/assetUtils.h>
#include <native/AssetManager/assetScanFolderInfo.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

class QSettings;

namespace AssetProcessor
{
    class PlatformConfiguration;
    class ScanFolderInfo;

    //! Information for a given recognizer, on a specific platform
    //! essentially a plain data holder, but with helper funcs
    class AssetPlatformSpec
    {
    public:
        QString m_extraRCParams;
    };

    //! The information about a gem that we need, extracted from the gem system.
    struct GemInformation
    {
        QString m_identifier; ///< The UUID of the gem.
        QString m_absolutePath;     ///< Where the gem's folder is (as an absolute path)
        QString m_relativePath;    ///< Where the gem's folder is (relative to the gems search path(s))
        QString m_displayName;     ///< A friendly display name, not to be used for any pathing stuff.
        bool m_isGameGem = false; ///< True if its a 'game project' gem.  Only one such gem can exist for any game project.
        bool m_assetOnly = false; ///< True if it is an asset only gems.

        GemInformation() = default;
        GemInformation(const char* identifier, const char* absolutePath, const char* relativePath, const char* displayName, bool isGameGem, bool assetOnlyGem);

        static QString GetGemAssetFolder() { return QString("Assets"); }
        static AZStd::string GemDirectoryFolderName() { return AZStd::string("Gems"); }
    };

    //! The data about a particular recognizer, including all platform specs.
    //! essentially a plain data holder, but with helper funcs
    struct AssetRecognizer
    {
        AssetRecognizer() = default;

        AssetRecognizer(const QString& name, bool testLockSource, int priority, bool critical, bool supportsCreateJobs, AssetBuilderSDK::FilePatternMatcher patternMatcher, const QString& version, const AZ::Data::AssetType& productAssetType)
            : m_name(name)
            , m_testLockSource(testLockSource)
            , m_priority(priority)
            , m_isCritical(critical)
            , m_supportsCreateJobs(supportsCreateJobs)
            , m_patternMatcher(patternMatcher)
            , m_version(version)
            , m_productAssetType(productAssetType) // if specified, it allows you to assign a UUID for the type of products directly.
        {}

        QString m_name;
        AssetBuilderSDK::FilePatternMatcher  m_patternMatcher;
        QString m_version = QString();

        // the QString is the Platform Identifier ("pc")
        // the AssetPlatformSpec is the details for processing that asset on that platform.
        QHash<QString, AssetPlatformSpec> m_platformSpecs;

        // an optional parameter which is a UUID of types to assign to the output asset(s)
        // if you don't specify one, then a heuristic will be used
        AZ::Uuid m_productAssetType = AZ::Uuid::CreateNull();

        int m_priority = 0; // used in order to sort these jobs vs other jobs when no other priority is applied (such as platform connected)
        bool m_testLockSource = false;
        bool m_isCritical = false;
        bool m_supportsCreateJobs = false; // used to indicate a recognizer that can respond to a createJobs request
    };
    //! Dictionary of Asset Recognizers based on name
    typedef QHash<QString, AssetRecognizer> RecognizerContainer;
    typedef QList<const AssetRecognizer*> RecognizerPointerContainer;

    //! The structure holds information about a particular exclude recognizer
    struct ExcludeAssetRecognizer
    {
        QString                             m_name;
        AssetBuilderSDK::FilePatternMatcher  m_patternMatcher;
    };
    typedef QHash<QString, ExcludeAssetRecognizer> ExcludeRecognizerContainer;

    //! Interface to get constant references to asset and exclude recognizers
    struct RecognizerConfiguration
    {
        virtual const RecognizerContainer& GetAssetRecognizerContainer() const = 0;
        virtual const ExcludeRecognizerContainer& GetExcludeAssetRecognizerContainer() const = 0;
    };

    /** Reads the platform ini configuration file to determine
    * platforms for which assets needs to be build
    */
    class PlatformConfiguration
        : public QObject
        , public RecognizerConfiguration
    {
        Q_OBJECT
    public:
        typedef QPair<QRegExp, QString> RCSpec;
        typedef QVector<RCSpec> RCSpecList;

    public:
        explicit PlatformConfiguration(QObject* pParent = nullptr);
        virtual ~PlatformConfiguration() = default;

        /** Use this function to parse the set of config files and the gem file to set up the platform config.
        * This should be about the only function that is required to be called in order to end up with
        * a full configuration.
        * Note that order of the config files is relevant - later files override settings in
        * files that are earlier.
        * @param rootConfigFile the root config file that is the default, baseline config.  This has the lowest priority
        * @param projectConfigFile (optional) the project configuration file.  This has the absolutely highest priority and will be read last
        * @param gemsFolderPath (optional) the absolute path to a folder containing the 'gem.json' file for this project.  If specified, gems will be queried and they will be added in the middle between the above two
        * Also note that if the project has any "game project gems", then those will also be inserted last, and thus have a higher priority than the root or non-project gems.
        */
        bool InitializeFromConfigFiles(QString rootConfigFile, QString projectConfigFile = QString());

        QString PlatformName(unsigned int platformCrc) const;
        QString RendererName(unsigned int rendererCrc) const;

        const AZStd::vector<AssetBuilderSDK::PlatformInfo>& GetEnabledPlatforms() const;
        const AssetBuilderSDK::PlatformInfo* const GetPlatformByIdentifier(const char* identifier) const;

        int MetaDataFileTypesCount() const { return m_metaDataFileTypes.count(); }
        // Metadata file types are (meta file extension, original file extension - or blank if its tacked on the end instead of replacing).
        // so for example if its
        // blah.tif + blah.tif.metadata, then its ("metadata", "")
        // but if its blah.tif + blah.metadata (rplacing tif, data is lost) then its ("metadata", "tif")
        QPair<QString, QString> GetMetaDataFileTypeAt(int pos) const;

        // Metadata extensions can also be a real file, to create a dependency on file types if a specific file changes
        // so for example, a Metadata file type pair ("Animations/SkeletonList.xml", "i_caf")
        // would cause all i_caf files to be re-evaluated when Animations/SkeletonList.xml is modified.
        bool IsMetaDataTypeRealFile(QString relativeName) const;

        void EnablePlatform(const AssetBuilderSDK::PlatformInfo& platform, bool enable = true);

        //! Gets the minumum jobs specified in the configuration file
        int GetMinJobs() const;
        int GetMaxJobs() const;

        //! Return how many scan folders there are
        int GetScanFolderCount() const;

        //! Return the gems info list
        QList<GemInformation> GetGemsInformation() const;

        //! Retrieve the scan folder at a given index.
        AssetProcessor::ScanFolderInfo& GetScanFolderAt(int index);

        //!  Manually add a scan folder.  Also used for testing.
        void AddScanFolder(const AssetProcessor::ScanFolderInfo& source, bool isUnitTesting = false);

        //!  Manually add a recognizer.  Used for testing.
        void AddRecognizer(const AssetRecognizer& source);

        //!  Manually remove a recognizer.  Used for testing.
        void RemoveRecognizer(QString name);

        //!  Manually add an exclude recognizer.  Used for testing.
        void AddExcludeRecognizer(const ExcludeAssetRecognizer& recogniser);

        //!  Manually remove an exclude recognizer.  Used for testing.
        void RemoveExcludeRecognizer(QString name);

        //!  Manually add a metadata type.  Used for testing.
        //!  The originalextension, if specified, means this metafile type REPLACES the given extension
        //!  If not specified (blank) it means that the metafile extension is added onto the end instead and does
        //!  not remove the original file extension
        void AddMetaDataType(const QString& type, const QString& originalExtension);

        // ------------------- utility functions --------------------
        ///! Checks to see whether the input file is an excluded file
        bool IsFileExcluded(QString fileName) const;

        //! Given a file name, return a container that contains all matching recognizers
        //!
        //! Returns false if there were no matches, otherwise returns true
        bool GetMatchingRecognizers(QString fileName, RecognizerPointerContainer& output) const;

        //! given a fileName (as a relative and which scan folder it was found in)
        //! Return either an empty string, or the canonical path to a file which overrides it
        //! becuase of folder priority.
        //! Note that scanFolderName is only used to exit quickly
        //! If its found in any scan folder before it arrives at scanFolderName it will be considered a hit
        QString GetOverridingFile(QString relativeName, QString scanFolderName) const;

        //! given a relative name, loop over folders and resolve it to a full path with the first existing match.
        QString FindFirstMatchingFile(QString relativeName) const;

        //! given a fileName (as a full path), return the relative path to it and the scan folder name it was found under.
        //!
        //! for example
        //! c:/dev/mygame/textures/texture1.tga
        //! ----> [textures/texture1.tga] found under [c:/dev/mygame]
        //! c:/dev/engine/models/box01.mdl
        //! ----> [models/box01.mdl] found under[c:/dev/engine]
        bool ConvertToRelativePath(QString fullFileName, QString& relativeName, QString& scanFolderName) const;

        //! given a full file name (assumed already fed through the normalization funciton), return the first matching scan folder
        const AssetProcessor::ScanFolderInfo* GetScanFolderForFile(const QString& fullFileName) const;

        //! Given a scan folder path, get its complete info
        const AssetProcessor::ScanFolderInfo* GetScanFolderByPath(const QString& scanFolderPath) const;

        const RecognizerContainer& GetAssetRecognizerContainer() const override;

        const ExcludeRecognizerContainer& GetExcludeAssetRecognizerContainer() const override;

        /** returns true if the config is valid.
        * configs are considered invalid if critical information is missing.
        * for example, if no recognizers are given, or no platforms are enabled.
        * They can also be considered invalid if a critical parse error occurred during load.
        */
        bool IsValid() const;

        /** If IsValid is false, this will contain the full error string to show to the user.
        * Note that IsValid will automatically write this error string to stderror as part of checking
        * So this function is there for those wishing to use a GUI.
        */
        const AZStd::string& GetError() const;

        void PopulatePlatformsForScanFolder(AZStd::vector<AssetBuilderSDK::PlatformInfo>& platformsList, QStringList includeTagsList = QStringList(), QStringList excludeTagsList = QStringList());

        //! Read all the gems for the project and return a list of
        //! the folders containing gems that are active for this project.
        virtual bool ReadGems(QList<GemInformation>& gemInfoList);  //virtual, so that unit tests can override to avoid loading extra dlls during testing
    protected:

        // call this first, to populate the list of platform informations
        void ReadPlatformInfosFromConfigFile(QString fileSource);
        // call this next, in order to find out what platforms are enabled
        void PopulateEnabledPlatforms(QString fileSource);
        // finaly, call this, in order to delete the platforminfos for non-enabled platforms
        void FinalizeEnabledPlatforms();

        // iterate over all the gems and add their folders to the "scan folders" list as appropriate.
        void AddGemScanFolders(const QList<GemInformation>& gemInfoList);

        // iterate over all the gems and add their asset processor config snippets to the configFilesOut array
        void AddGemConfigFiles(const QList<GemInformation>& gemInfoList, QStringList& configFilesOut, bool& foundGameGem);

        void ReadEnabledPlatformsFromConfigFile(QString fileSource);
        bool ReadRecognizersFromConfigFile(QString fileSource);
        void ReadMetaDataFromConfigFile(QString fileSource);

    private:
        AZStd::vector<AssetBuilderSDK::PlatformInfo> m_enabledPlatforms;
        RecognizerContainer m_assetRecognizers;
        ExcludeRecognizerContainer m_excludeAssetRecognizers;
        QVector<AssetProcessor::ScanFolderInfo> m_scanFolders;
        QList<QPair<QString, QString> > m_metaDataFileTypes;
        QSet<QString> m_metaDataRealFiles;
        QList<GemInformation> m_gemInfoList;

        int m_minJobs = 1;
        int m_maxJobs = 3;

        // used only during file read, keeps the total running list of all the enabled platforms from all config files and command lines
        QStringList m_tempEnabledPlatforms;

        bool ReadRecognizerFromConfig(AssetRecognizer& target, QSettings& loader); // assumes the group is already selected

        ///! if non-empty, fatalError contains the error that occurred during read.
        ///! it will be printed out to the log when
        mutable AZStd::string m_fatalError;
    };
} // end namespace AssetProcessor

#endif // PLATFORMCONFIGURATION_H

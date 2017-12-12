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

#include "native/utilities/assetUtils.h"
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

    //! The data about a particular recognizer, including all platform specs.
    //! essentially a plain data holder, but with helper funcs
    struct AssetRecognizer
    {
        AssetRecognizer() = default;

        AssetRecognizer(const QString& name, bool testLockSource, int priority, bool critical, bool supportsCreateJobs, AssetUtilities::FilePatternMatcher patternMatcher, const QString& version, const AZ::Data::AssetType& productAssetType)
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
        AssetUtilities::FilePatternMatcher  m_patternMatcher;
        QString m_version = QString();
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
        AssetUtilities::FilePatternMatcher  m_patternMatcher;
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
        virtual ~PlatformConfiguration();
        void PopulateEnabledPlatforms(QString fileSource);
        void ReadPlatformsFromConfigFile(QString fileSource);
        bool ReadRecognizersFromConfigFile(QString fileSource);
        void ReadMetaDataFromConfigFile(QString fileSource);
        void ReadGemsConfigFile(QString gemsFile, QStringList& gemConfigFiles);

        QString PlatformName(unsigned int platformCrc) const;
        QString RendererName(unsigned int rendererCrc) const;

        int PlatformCount() const { return m_platforms.count(); }
        QString PlatformAt(int pos) const;

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

        //! used for tests
        void EnablePlatform(QString platform, bool enable = true);

        //! Gets the minumum jobs specified in the configuration file
        int GetMinJobs() const;
        int GetMaxJobs() const;

        //! Return how many scan folders there are
        int GetScanFolderCount() const;

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

        // returns the number of gems successfully read.
        int ReadGems(QString gemsConfigName, QStringList& gemConfigFiles);

        const RecognizerContainer& GetAssetRecognizerContainer() const override;

        const ExcludeRecognizerContainer& GetExcludeAssetRecognizerContainer() const override;
    private:
        QVector<QString> m_platforms;
        QHash<unsigned int, QString> m_platformsByCrc;
        QHash<unsigned int, QString> m_renderersByCrc;
        RecognizerContainer m_assetRecognizers;
        ExcludeRecognizerContainer m_excludeAssetRecognizers;
        QVector<AssetProcessor::ScanFolderInfo> m_scanFolders;
        QList<QPair<QString, QString> > m_metaDataFileTypes;
        QSet<QString> m_metaDataRealFiles;

        int m_minJobs = 1;
        int m_maxJobs = 3;

        bool ReadRecognizerFromConfig(AssetRecognizer& target, QSettings& loader); // assumes the group is already selected
    };
} // end namespace AssetProcessor

#endif // PLATFORMCONFIGURATION_H

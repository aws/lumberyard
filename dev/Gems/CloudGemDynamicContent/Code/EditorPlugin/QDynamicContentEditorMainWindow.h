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

#include <ui_dynamic_content.h>
#include <QSortFilterProxyModel>
#include "ManifestDataStructures.h"
#include <ui_create_package.h>
#include <ui_incompatible_files.h>
#include <ui_change_target_platform.h>
#include <ui_create_new_manifest.h>
#include <ui_platform_warning.h>
#include <ui_upload_packages.h>

#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

namespace DynamicContent
{
    class FileWatcherModel;
    class PackagesModel;

    class QDynamicContentEditorMainWindow
        : public QMainWindow
        , public Ui_QDynamicContentEditorMainWindow
        , private PythonWorkerEvents::Bus::Handler
    {
        Q_OBJECT

    public:
        const double FileNameColumnWidth = 0.6;
        const double FilePlatformColumnWidth = 0.25;
        const double FileStatusColumnWidth = 0.15;

        static const double PackagesNameColumnWidth()
        {
            return 0.5;
        }

        static const double PackagesPlatformColumnWidth()
        {
            return 0.2;
        }

        static const double PackagesStatusColumnWidth()
        {
            return 0.15;
        }

        static const double PackagesS3StatusColumnWidth()
        {
            return 0.15;
        }

        static const GUID& GetClassID()
        {
            // {52475BFF-D302-4C8A-9ED6-B4804E8F9D62}
            static const GUID guid = {
                0x52475bff, 0xd302, 0x4c8a, { 0x9e, 0xd6, 0xb4, 0x80, 0x4e, 0x8f, 0x9d, 0x62 }
            };
            return guid;
        }

        explicit QDynamicContentEditorMainWindow(QWidget* parent = nullptr);
        ~QDynamicContentEditorMainWindow();

        void AddFilesToPackageFromFileInfoList(const QStringList& fileInfoList, const QModelIndex& pakIndex);

        bool CheckPlatformType(const QStringList& fileInfoList, const QModelIndex& pakIndex);

        struct PlatformInfo
        {
            bool LicenseExists = true;
            QString LargeSizeIconLink;
            QString SmallSizeIconLink;
            QString DisplayName;
        };

        static QMap<QString, QString> PlatformDisplayNames()
        {
            QMap<QString, QString> platformDisplayMap;
            platformDisplayMap["osx_gl"] = "macOS";
            platformDisplayMap["es3"] = "Android";
            platformDisplayMap["ios"] = "iOS";
            platformDisplayMap["pc"] = "Windows";
            platformDisplayMap["xenia"] = "Xenia";
            platformDisplayMap["provo"] = "Provo";
            platformDisplayMap["shared"] = "Shared";
            return platformDisplayMap;
        }

        static QMap<QString, PlatformInfo> PlatformMap()
        {
            QMap<QString, PlatformInfo> platformMap;
            QList<QString> platformList = PlatformDisplayNames().keys();
            QMap<QString, QString> platformDisplayNames = PlatformDisplayNames();
            for (auto platform : platformList)
            {
                platformMap[platform].LicenseExists = true;
                platformMap[platform].LargeSizeIconLink = QString("Editor/Icons/CloudCanvas/Platform/" + platform + "/" + platform + "_16px.png");
                platformMap[platform].SmallSizeIconLink = QString("Editor/Icons/CloudCanvas/Platform/" + platform + "/" + platform + "_14px.png");
                platformMap[platform].DisplayName = platformDisplayNames[platform];     
            }
            return platformMap;
        }

        static QString FolderName(QString displayName)
        {
            QMap<QString, PlatformInfo> platformMap = PlatformMap();
            QStringList folderNames = platformMap.keys();
            for (auto folderName : folderNames)
            {
                if (platformMap[folderName].DisplayName == displayName)
                {
                    return folderName;
                }
            }
            return "";
        }

        struct AddFilesStatus 
        {
            static const int Match = 1;
            static const int Unsupport = 2;
            static const int NoPlatformInfo = 3;
            static const int Unmatch = 4;
        };

    signals:
        void KeyExists(QVariant keyInfo);
        void GenerateKeyCompleted();
        void ReceivedPythonOutput(PythonWorkerRequestId requestId, const QString key, const QVariant value);

    private:
        //////////////////////////////////////////////////////////////////////////
        // PythonWorkerEvents
        bool OnPythonWorkerOutput(PythonWorkerRequestId requestId, const QString& key, const QVariant& value) override;
        //////////////////////////////////////////////////////////////////////////

        void SetupUI();
        void PythonExecute(const char* command, const QVariantMap& args = QVariantMap{});

        void SetPlatformLabelIcons();
        void SetIconOpacity(QString platformName);
        void CheckPlatformLicense(QString platform);
        
        void UpdateManifestList(const QVariant& vManifestList);
        void OpenManifest(const QString& filePath);
        void UpdateManifest(const QVariant& vManifestInfo);
        void UpdateBucketDiff(const QVariant& vBucketDiffInfo);
        void UpdateLocalDiff(const QVariant& vDiffInfo);

        ManifestFiles GetWatchedFilesFromFileInfoListAndPlatform(const QStringList& fileInfoList, const QString& platformType);
        void AddFileFromCache(QString path, QString platform);
        bool FileProcessedByAssetProcessor(QString filePath);
        bool CheckAddedFilesPlatformType(QVariantList vFileNames, QString targetPlatform, bool fileProcessedByAssetProcessor);
        int AddedFilesPlatformStatus(QVariantList vFileNames, QString targetPlatform, bool filesInCache);
        void AddFolderFromCache(QString path, QString platform);
        void AddFilesToPackage(const ManifestFiles& manifestFiles, const QModelIndex& pakIndex);

        void UpdateManifestSourceControlState();
        void RequestEditManifest();
        bool PrepareManifestForWork();

        void SelectCurrentManifest();

        void ParseNameAndPlatform(const QString& fileName, QString& localName, QString& platformName) const;

        QMenu* addPlatformMenuActions(QMenu* menu);
        void GetSelectedPlatforms(QList<QCheckBox*> checkBoxList, QVariantList* selectedPlatformTypes);
        void SetFilterRegExp(QVariantList* platforms);

    private slots:
        void OnNewManifest();
        void OnDeleteManifest();
        void OnManifestLoadComplete();
        void OnChangeTargetPlatforms();
        void OnUploadChangedPackages();
        void OnGenerateNewKeys();
        void OnOpenDocumentation();
        void OnAddFile(QAction *action);
        void OnAddFolder(QAction *action);
        void OnRemoveFile();
        void OnRemoveFromPackage();
        void OnAddPackage();
        void OnRemovePackage();
        void OnSelectManifest(int index);
        void OnGenerateKeys();
        void OnFileWatcherSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected);
        void OnPakTreeSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected);
        void OnSignalUploadComplete();
        void OnLeftPaneContextMenuRequested(QPoint pos);
        void OnRightPaneContextMenuRequested(QPoint pos);
        void OnSortingLeftPaneByColumn(int column);
        void OnSortingRightPaneByColumn(int column);
        void SetPakStatusOutdated(QString pakName);
        void OnCheckExistingKeys();
        bool ProcessPythonWorkerOutput(PythonWorkerRequestId requestId, const QString& key, const QVariant& value);

        void UpdateUI();
    private:
        PythonWorkerRequestId m_requestId;
        QScopedPointer<FileWatcherModel> m_fileWatcherModel;
        QScopedPointer<QSortFilterProxyModel> m_packagesProxyModel;
        QScopedPointer<PackagesModel> m_packagesModel;
        QScopedPointer<QSortFilterProxyModel> m_fileWatcherProxyModel;
        QQueue<QVariantList*> m_dataRetainer;  
        bool m_noStackState { false };

        ManifestFiles m_watchedFiles;
        ManifestFiles m_pakFiles;

        QMenu* m_contentMenu;
        QMenu* m_addFilesButtonMenu;
        QMenu* m_addFolderButtonMenu;
        QMenu* m_settingsButtonMenu;
        QMap<QString, PlatformInfo> m_platformMap;
        bool m_isLoading{ false };
        bool m_isUploading{ false };
        QStringList m_manifestTargetPlatforms;

        struct ManifestInfo
        {
            AzToolsFramework::SourceControlFileInfo m_manifestSourceFileInfo;
            QString m_currentlySelectedManifestName;
            QString m_fullPathToManifest;
        };
        QSharedPointer<ManifestInfo> m_manifestStatus;

        void SetNoStackState(bool state) { m_noStackState = state; }
        bool GetNoStackState() { return m_noStackState; }

        void ReadAllFilesInFolder(const QString& folderName, QVariantList* vFileNames);
        void AddFileInfoMap(const QString& fileName, QVariantList* vFileNames);
    };

    class QCreatePackageDialog : public QDialog,
        public Ui_CreatePackageDialog
    {
        Q_OBJECT
    public:
 
        explicit QCreatePackageDialog(QWidget* parent = nullptr);
        ~QCreatePackageDialog() = default;

        void SetPlatformButtons();

    private slots:
        void OnPlatformSelected(QAbstractButton * button);
    };

    class QIncompatibleFilesDialog : public QDialog,
        public Ui_IncompatibleFilesDialog
    {
        Q_OBJECT
    public:

        explicit QIncompatibleFilesDialog(QWidget* parent = nullptr);
        ~QIncompatibleFilesDialog() = default;

        void AddIncompatibleFile(const QString& imcompatibleFileName);
        void SetIncompatibleFilesDialogText(const QString& pakName, const QString& platformType);
    };

    class QChangeTargetPlatformDialog : public QDialog,
        public Ui_ChangeTargetPlatformDialog
    {
        Q_OBJECT
    public:

        explicit QChangeTargetPlatformDialog(QWidget* parent = nullptr);
        ~QChangeTargetPlatformDialog() = default;
    };

    class QCreateNewManifestDialog : public QDialog,
        public Ui_CreateNewManifestDialog
    {
        Q_OBJECT
    public:

        explicit QCreateNewManifestDialog(QWidget* parent = nullptr);
        ~QCreateNewManifestDialog() = default;

        void SetManifestNameRegExp();
        QString ManifestName();
    };

    class QPlatformWarningDialog : public QDialog,
        public Ui_PlatformWarningDialog
    {
        Q_OBJECT
    public:

        explicit QPlatformWarningDialog(QWidget* parent = nullptr);
        ~QPlatformWarningDialog() = default;

        void setAddFileWarningMessage(int status);
        void setCreatePakWarningMessage();
    };

    class QUploadPackagesDialog : public QDialog,
        public Ui_UploadPackagesDialog
    {
        Q_OBJECT
    public:

        explicit QUploadPackagesDialog(QWidget* parent = nullptr);
        ~QUploadPackagesDialog() = default;

        struct UploadButtonStatus
        {
            static const int Upload = 1;
            static const int Next = 2;
            static const int Continue = 3;
        };

        struct CancelButtonStatus
        {
            static const int Back = 1;
            static const int Cancel = 2;
        };

        bool PackagesSigned();
        void UpdateUploadPackagesDialogUI();
        void SetUploadButtonCurrentStatus(int status);
        void SetCancelButtonCurrentStatus(int status);

    signals:
        void CheckExistingKeys();
        void GenerateKeys();

    public slots:
        void OnKeyExists(QVariant keyInfo);
        void OnResetUploadPackagesDialogUI();

    private slots:
        void OnCheckBoxStateChanged(int state);
        void OnUploadButtonReleased();
        void OnCancelButtonReleased();

    private:
        static const QMap<int, QString> UploadButtonText()
        {
            QMap<int, QString> uploadButtonTextMap;
            uploadButtonTextMap[UploadButtonStatus::Upload] = tr("Upload");
            uploadButtonTextMap[UploadButtonStatus::Next] = tr("Next");
            uploadButtonTextMap[UploadButtonStatus::Continue] = tr("Continue");
            return uploadButtonTextMap;
        }

        static const QMap<int, QString> CancelButtonText()
        {
            QMap<int, QString> cancelButtonTextMap;
            cancelButtonTextMap[CancelButtonStatus::Back] = tr("Back");
            cancelButtonTextMap[CancelButtonStatus::Cancel] = tr("Cancel");
            return cancelButtonTextMap;
        }

        int m_uploadButtonStatus;
        int m_cancelButtonStatus;

        int UploadButtonCurrentStatus() { return m_uploadButtonStatus; };
        int CancelButtonCurrentStatus() { return m_cancelButtonStatus; };
    };
}

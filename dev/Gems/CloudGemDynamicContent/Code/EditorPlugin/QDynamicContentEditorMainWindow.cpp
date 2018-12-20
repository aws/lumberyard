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
#include "CloudGemDynamicContent_precompiled.h"

#include "QDynamicContentEditorMainWindow.h"

#include "FileWatcherModel.h"
#include "PackagesModel.h"
#include "ManifestTableKeys.h"
#include "fileWatcherView.h"

#include <QPainter>
#include <QRadioButton>
#include <QDesktopServices>

#include <LyMetricsProducer/LyMetricsAPI.h>

#include <QDynamicContentEditorMainWindow.moc>


namespace DynamicContent
{
    const int QUploadPackagesDialog::UploadButtonStatus::Upload;
    const int QUploadPackagesDialog::UploadButtonStatus::Next;
    const int QUploadPackagesDialog::UploadButtonStatus::Continue;

    const int QUploadPackagesDialog::CancelButtonStatus::Back;
    const int QUploadPackagesDialog::CancelButtonStatus::Cancel;

    // note that these match the list of command handlers found in AWS/cli-plugin-code/resource_commands.py
    static const char* COMMAND_LIST_MANIFESTS = "list-manifests";
    static const char* COMMAND_SHOW_MANIFESTS = "show-manifest";
    static const char* COMMAND_NEW_MANIFEST = "new-manifest";
    static const char* COMMAND_DELETE_MANIFEST = "delete-manifest";
    static const char* COMMAND_ADD_FILES_TO_MANIFEST = "add-files-to-manifest";
    static const char* COMMAND_DELETE_FILES_FROM_MANIFEST = "delete-files-from-manifest";
    static const char* COMMAND_ADD_PAK_TO_MANIFEST = "add-pak-to-manifest";
    static const char* COMMAND_DELETE_PAK_FROM_MANIFEST = "delete-pak-from-manifest";
    static const char* COMMAND_ADD_FILES_TO_PAK = "add-files-to-pak";
    static const char* COMMAND_DELETE_FILES_FROM_PAK = "delete-files-from-pak";
    static const char* COMMAND_PAK_AND_UPLOAD = "pak-and-upload";
    static const char* COMMAND_GET_BUCKET_STATUS = "get-bucket-status";
    static const char* COMMAND_GET_LOCAL_FILE_STATUS = "get-local-file-status";
    static const char* COMMAND_GENERATE_KEYS = "generate-keys";
    static const char* COMMAND_SIGNAL_UPLOAD_COMPLETE = "signal-upload-complete";
    static const char* COMMAND_NO_STACK = "no-stack";
    static const char* COMMAND_PERCENT_COMPLETE = "percent-complete";
    static const char* COMMAND_CHANGE_TARGET_PLATFORMS = "change-target-platforms";
    static const char* COMMAND_GET_FULL_PLATFORM_CACHE_GAME_PATH = "get-full-platform-cache-game-path";
    static const char* COMMAND_SIGNAL_FOUND_UPDATE_ITEM = "found-update-item";
    static const char* COMMAND_SIGNAL_DONE_UPLOADING = "done-uploading";
    static const char* COMMAND_CHECK_EXISTING_KEYS = "check-existing-keys";

    static const char* BEGIN_BUTTON_ANCHOR = "<a style='color:#4285F4; text-decoration:none' href=\"clickme\">";
    static const char* END_BUTTON_ANCHOR = "</a>";

    static const char* DCM_METRIC_EVENT_NAME = "DynamicContentManager";
    static const char* DCM_OPERATION_ATTRIBUTE_NAME = "Operation";
    
    QDynamicContentEditorMainWindow::QDynamicContentEditorMainWindow(QWidget* parent) :
        m_fileWatcherModel(new FileWatcherModel(this)),
        m_packagesModel(new PackagesModel(this)),
        m_fileWatcherProxyModel(new QSortFilterProxyModel(this)),
        m_packagesProxyModel(new QSortFilterProxyModel(this))
    {

        EBUS_EVENT_RESULT(m_requestId, PythonWorkerRequests::Bus, AllocateRequestId);

        m_manifestStatus.reset(new ManifestInfo);

        setupUi(this);
        PythonWorkerEvents::Bus::Handler::BusConnect();
        SetupUI();

        m_platformMap = PlatformMap();
        QList<QString> platformCheckList = QList<QString>() << "xboxone" << "ps4"; // ACCEPTED_USE
        for (auto platform : platformCheckList)
        {
            CheckPlatformLicense(platform);
        }

        auto metricId = LyMetrics_CreateEvent(DCM_METRIC_EVENT_NAME);
        LyMetrics_AddAttribute(metricId, DCM_OPERATION_ATTRIBUTE_NAME, "open");
        LyMetrics_SubmitEvent(metricId);
    }

    QDynamicContentEditorMainWindow::~QDynamicContentEditorMainWindow()
    {
        PythonWorkerEvents::Bus::Handler::BusDisconnect();
        auto metricId = LyMetrics_CreateEvent(DCM_METRIC_EVENT_NAME);
        LyMetrics_AddAttribute(metricId, DCM_OPERATION_ATTRIBUTE_NAME, "close");
        LyMetrics_SubmitEvent(metricId);
    }

    void QDynamicContentEditorMainWindow::CheckPlatformLicense(QString platform)
    {
        AZStd::string fullPathToAssets = gEnv->pFileIO->GetAlias("@assets@");
        QString fullPath = fullPathToAssets.c_str();
        fullPath.replace("\\", "/");
        QStringList pathList = fullPath.split("/", QString::SkipEmptyParts);

        //Check whether the folder for the platform exists
        if (pathList.size() >= 2)
        {
            pathList[pathList.size() - 2] = platform;
            QString ps4FullPath = pathList.join("/"); // ACCEPTED_USE
            if (!QDir(ps4FullPath).exists()) // ACCEPTED_USE
            {
                m_platformMap[platform].LicenseExists = false;
                QLabel* platformLabel = findChild<QLabel *>(platform);
                platformLabel->hide();
            }
        }
    }

    void QDynamicContentEditorMainWindow::SetPlatformLabelIcons()
    {
        QStringList platformNames = m_platformMap.keys();
        for (auto platformName : platformNames)
        {
            QLabel* platform = findChild<QLabel *>(platformName);
            if (platform)
            {
                platform->setPixmap(QPixmap(m_platformMap[platformName].LargeSizeIconLink));
            }
        }
    }

    void QDynamicContentEditorMainWindow::SetIconOpacity(QString platformName)
    {
        QLabel* unselectedPlatform = findChild<QLabel *>(platformName);
        QPixmap pixmap = QPixmap(m_platformMap[platformName].LargeSizeIconLink);
        QPixmap transparentpixmap(pixmap.size());
        transparentpixmap.fill(Qt::transparent);
        QPainter painter;
        painter.begin(&transparentpixmap);
        painter.setOpacity(0.1);
        painter.drawPixmap(0, 0, pixmap);
        painter.end();
        unselectedPlatform->setPixmap(transparentpixmap);
    }

    // This is not guaranteed to occur on the UI thread (Which is our main thread currently)
    // We need to process our results there
    bool QDynamicContentEditorMainWindow::OnPythonWorkerOutput(PythonWorkerRequestId requestId, const QString& key, const QVariant& value)
    {
        // Use QT Signal to marshal back
        ReceivedPythonOutput(requestId, key, value);
        return (requestId == m_requestId);
    }

    bool QDynamicContentEditorMainWindow::ProcessPythonWorkerOutput(PythonWorkerRequestId requestId, const QString& key, const QVariant& value)
    {
        // This is a poor solution to a problem with Qt containers
        // Technically the containers implement copy on write with the contained objects
        // However as the objects being contained are QVariants the copy on write
        // doesn't occur as the underlying type that needs a deep copy isn't available.
        // This prevents storing stack variables in containers that are passed to the Python worker
        // This solution creates the variables on the heap and then assumes the responses
        // are local, linear and quick. This would probably blow up on button spam.
        // This will need to be reworked for public consumption but will work for the demo.
        // Correct way to deal with this is to track each item sent to python worker along
        // with identifying info that gets passed back in to update so they can be deleted
        // when we are sure they are no longer used. Execution ID may be the way to deal with this
        // but this function currently get called before the python worker attempts to remove 
        // events from the retry pool, so if they get deleted here right after the call succeeds
        // the code will still crash.
        while (m_dataRetainer.size() > 1)
        {
            delete m_dataRetainer.dequeue();
        }

        if (key == "message")
        {
            QString message = value.toString();
            QRegExp profileRegExp("\nDefault Profile: .*|\nRemoved Profile: .*\nWarning: this was your default profile.");
            QRegExp deploymentRegExp("\nUser default:    .*\nProject default: .*\n|\n.* deployment stack .* and access stack .* have been deleted.");
            //Refresh the GUI when the default profile or deployment is changed
            if (message.contains(profileRegExp) || message.contains(deploymentRegExp))
            {
                PythonExecute(COMMAND_LIST_MANIFESTS);
                return true;
            }
        }

        if (requestId == m_requestId)
        {
            if (m_isUploading || m_isLoading)
            {
                if (key == COMMAND_SIGNAL_FOUND_UPDATE_ITEM || key == COMMAND_SIGNAL_DONE_UPLOADING)
                {
                    QMap<QString, QVariant> outputMap = value.toMap();
                    status->showMessage(outputMap[KEY_MESSAGE].toString());
                }
                else
                {
                    status->showMessage(value.toString());
                }
            }
            if (key == COMMAND_LIST_MANIFESTS)
            {
                SetNoStackState(false);
                UpdateManifestList(value);
                return true;
            } 
            else if (key == COMMAND_SHOW_MANIFESTS)
            {
                status->showMessage("");
                SetNoStackState(false);
                OnManifestLoadComplete();
                UpdateManifest(value);
                return true;
            }
            else if (key == COMMAND_GET_BUCKET_STATUS)
            {
                SetNoStackState(false);
                UpdateBucketDiff(value);
                return true;
            }
            else if (key == COMMAND_GET_LOCAL_FILE_STATUS)
            {
                SetNoStackState(false);
                UpdateLocalDiff(value);
                return true;
            }
            else if(key == COMMAND_SIGNAL_UPLOAD_COMPLETE)
            {
                status->showMessage("");
                SetNoStackState(false);
                OnSignalUploadComplete();
                return true;
            }
            else if (key == COMMAND_NO_STACK)
            {
                SetNoStackState(true);
                UpdateUI();
                return true;
            }
            else if (key == COMMAND_GET_FULL_PLATFORM_CACHE_GAME_PATH)
            {
                SetNoStackState(false);

                auto output = value.toMap();
                QString type = output["type"].toString();
                QString path = output["path"].toString();
                QString platform = output["platform"].toString();
                if (type == "file")
                {
                    AddFileFromCache(path, platform);
                }
                else
                {
                    AddFolderFromCache(path, platform);
                }

                return true;
            }
            else if (key == COMMAND_SIGNAL_FOUND_UPDATE_ITEM)
            {
                QMap<QString, QVariant> outputMap = value.toMap();
                m_packagesModel->startS3StatusAnimation(outputMap[KEY_KEY_NAME].toString());
                return true;
            }
            else if (key == COMMAND_SIGNAL_DONE_UPLOADING)
            {
                m_packagesModel->stopS3StatusAnimation(value.toString()); // ACCEPTED_USE
                return true;
            }
            else if (key == COMMAND_CHECK_EXISTING_KEYS)
            {
                KeyExists(value);
                return true;
            }
            else if (key == COMMAND_GENERATE_KEYS)
            {
                QString message = value.toString();
                gEnv->pLog->Log("(Cloud Canvas) %s", message.toStdString().c_str());
                GenerateKeyCompleted();
                return true;
            }
        }
        return false;
    }

    void QDynamicContentEditorMainWindow::SetupUI()
    {
        setFixedSize(width(), height());

        connect(this, &QDynamicContentEditorMainWindow::ReceivedPythonOutput, this, &QDynamicContentEditorMainWindow::ProcessPythonWorkerOutput);

        // Buttons
        connect(btnUpload, &QPushButton::released, this, &QDynamicContentEditorMainWindow::OnUploadChangedPackages);
        connect(btnRemoveFile, &QPushButton::released, this, &QDynamicContentEditorMainWindow::OnRemoveFile);
        connect(btnRemoveFromPack, &QPushButton::released, this, &QDynamicContentEditorMainWindow::OnRemoveFromPackage);
        connect(btnAddPackage, &QPushButton::released, this, &QDynamicContentEditorMainWindow::OnAddPackage);
        connect(btnRemovePackage, &QPushButton::released, this, &QDynamicContentEditorMainWindow::OnRemovePackage);

        //File menu actions
        connect(actionCreateNewManifest, &QAction::triggered, this, &QDynamicContentEditorMainWindow::OnNewManifest);
        connect(actionUploadAllFiles, &QAction::triggered, this, &QDynamicContentEditorMainWindow::OnUploadChangedPackages);
        connect(actionGenerateNewKeys, &QAction::triggered, this, &QDynamicContentEditorMainWindow::OnGenerateNewKeys);
        connect(actionDynamicContentDocumentation, &QAction::triggered, this, &QDynamicContentEditorMainWindow::OnOpenDocumentation);

        m_addFilesButtonMenu = new QMenu(this);
        connect(m_addFilesButtonMenu, &QMenu::triggered, this, &QDynamicContentEditorMainWindow::OnAddFile);
        btnAddFile->setMenu(m_addFilesButtonMenu);

        m_addFolderButtonMenu = new QMenu(this);
        connect(m_addFolderButtonMenu, &QMenu::triggered, this, &QDynamicContentEditorMainWindow::OnAddFolder);
        btnAddFolder->setMenu(m_addFolderButtonMenu);

        m_settingsButtonMenu = new QMenu(this);
        auto changeTargetPlatforms = m_settingsButtonMenu->addAction("Change Target Platforms");
        connect(changeTargetPlatforms, &QAction::triggered, this, &QDynamicContentEditorMainWindow::OnChangeTargetPlatforms);
        auto deleteManifest = m_settingsButtonMenu->addAction("Delete Manifest");
        connect(deleteManifest, &QAction::triggered, this, &QDynamicContentEditorMainWindow::OnDeleteManifest);
        settings->setMenu(m_settingsButtonMenu);
        settings->setIcon(QIcon("Editor/Icons/CloudCanvas/gear.png"));


        connect(cbManifestSelection, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &QDynamicContentEditorMainWindow::OnSelectManifest);

        m_contentMenu = new QMenu(this);

        auto manifestNameFont = manifestName->font();
        manifestNameFont.setPointSize(20);
        manifestName->setFont(manifestNameFont);
        manifestName->setText("");

        connect(m_fileWatcherModel.data(), &FileWatcherModel::fileInPakChanged, this, &QDynamicContentEditorMainWindow::SetPakStatusOutdated);

        m_fileWatcherProxyModel->setSourceModel(m_fileWatcherModel.data());

        QRect geometry = tableFileWatcher->geometry();
        bool verticalHeaderVisible = tableFileWatcher->verticalHeader()->isVisible();
        tableFileWatcher->deleteLater();

        tableFileWatcher = new FileWatcherView{tableFileWatcher->parentWidget()};
        tableFileWatcher->setModel(m_fileWatcherProxyModel.data());
        tableFileWatcher->setGeometry(geometry);
        tableFileWatcher->verticalHeader()->setVisible(verticalHeaderVisible);
        tableFileWatcher->setColumnWidth(0, tableFileWatcher->width() * FileNameColumnWidth);
        tableFileWatcher->setColumnWidth(1, tableFileWatcher->width() * FilePlatformColumnWidth);
        tableFileWatcher->setColumnWidth(2, tableFileWatcher->width() * FileStatusColumnWidth);
        tableFileWatcher->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
        tableFileWatcher->setAlternatingRowColors(true);
        tableFileWatcher->setDragEnabled(true);
        tableFileWatcher->setSortingEnabled(true);
        tableFileWatcher->setContextMenuPolicy(Qt::CustomContextMenu);

        connect(tableFileWatcher, &QTableView::customContextMenuRequested, this, &QDynamicContentEditorMainWindow::OnLeftPaneContextMenuRequested);
        connect(tableFileWatcher->horizontalHeader(), &QHeaderView::sectionClicked, this, &QDynamicContentEditorMainWindow::OnSortingLeftPaneByColumn);

        m_packagesProxyModel->setSourceModel(m_packagesModel.data());
        treePackages->setModel(m_packagesProxyModel.data());
        treePackages->setColumnWidth(0, treePackages->width() * PackagesNameColumnWidth());
        treePackages->setColumnWidth(1, treePackages->width() * PackagesPlatformColumnWidth());
        treePackages->setColumnWidth(2, treePackages->width() * PackagesStatusColumnWidth());
        treePackages->setColumnWidth(3, treePackages->width() * PackagesS3StatusColumnWidth());

        treePackages->setAcceptDrops(true);
        treePackages->setDropIndicatorShown(true);
        treePackages->setSortingEnabled(true);
        treePackages->setContextMenuPolicy(Qt::CustomContextMenu);

        connect(treePackages, &QTreeView::customContextMenuRequested, this, &QDynamicContentEditorMainWindow::OnRightPaneContextMenuRequested);
        connect(treePackages->header(), &QHeaderView::sectionClicked, this, &QDynamicContentEditorMainWindow::OnSortingRightPaneByColumn);

        connect(tableFileWatcher->selectionModel(), &QItemSelectionModel::selectionChanged, this, &QDynamicContentEditorMainWindow::OnFileWatcherSelectionChanged);
        connect(treePackages->selectionModel(), &QItemSelectionModel::selectionChanged, this, &QDynamicContentEditorMainWindow::OnPakTreeSelectionChanged);     

        btnUpload->setProperty("class", "Primary");

        cbManifestSelection->setEditText("Choose / create a manifest");

        PythonExecute(COMMAND_LIST_MANIFESTS);
        
        UpdateUI();
    }

    void QDynamicContentEditorMainWindow::PythonExecute(const char* command, const QVariantMap& args)
    {
        EBUS_EVENT(PythonWorkerRequests::Bus, ExecuteAsync, m_requestId, command, args);
    }


    QCreateNewManifestDialog::QCreateNewManifestDialog(QWidget* parent) : QDialog(parent)
    {
        setupUi(this);

        buttonBox->button(QDialogButtonBox::Ok)->setProperty("class", "Primary");

        QMap<QString, QDynamicContentEditorMainWindow::PlatformInfo> platformMap = QDynamicContentEditorMainWindow::PlatformMap();
        QList<QCheckBox*> checkBoxList = findChildren<QCheckBox*>();
        for (auto checkBox : checkBoxList)
        {
            checkBox->setIcon(QIcon(platformMap[checkBox->objectName()].LargeSizeIconLink));
        }
    }

    void QCreateNewManifestDialog::SetManifestNameRegExp()
    {
        manifestNameEdit->setValidator(new QRegExpValidator(QRegExp("^[-0-9a-zA-Z_][-0-9a-zA-Z!_.]*$")));
    }

    QString QCreateNewManifestDialog::ManifestName()
    {
        return manifestNameEdit->text();
    }

    void QDynamicContentEditorMainWindow::OnNewManifest()
    {
        QScopedPointer<QCreateNewManifestDialog> createNewManifestDialog{new QCreateNewManifestDialog(this)};

        QStringList platformNames = m_platformMap.keys();
        for (auto platform : platformNames)
        {
            if (!m_platformMap[platform].LicenseExists)
            {
                QCheckBox* platformCheckBox = createNewManifestDialog->findChild<QCheckBox *>(platform);
                platformCheckBox->hide();
            }

        }

        createNewManifestDialog->SetManifestNameRegExp();
        int execCode = createNewManifestDialog->exec();

        QString newName;

        while (execCode > 0)
        {
            newName = createNewManifestDialog->ManifestName();
            if (newName.length()==0)
            {
                auto nameMissing = QString(tr("Please enter a name for the manifest."));
                auto reply = QMessageBox::information(this,
                    tr("Attention"),
                    nameMissing);
                execCode = createNewManifestDialog->exec();
                continue;
            }
            QVariantList* selectedPlatformTypes = new QVariantList();
            m_dataRetainer.enqueue(selectedPlatformTypes);
            QList<QCheckBox*> checkBoxList = createNewManifestDialog->findChildren<QCheckBox*>();
            GetSelectedPlatforms(checkBoxList, selectedPlatformTypes);

            QVariantMap args;
            args[ARGS_NEW_FILE_NAME] = newName;
            args[ARGS_MANIFEST_PATH] = cbManifestSelection->currentData().toString();
            args[ARGS_PLATFORM_LIST] = *selectedPlatformTypes;
            m_manifestStatus->m_currentlySelectedManifestName = newName + ".json";
            m_manifestStatus->m_fullPathToManifest = cbManifestSelection->currentData().toString();
            PythonExecute(COMMAND_NEW_MANIFEST, args);
            break;
        }
        if(execCode <= 0)
        {
            SelectCurrentManifest();
        }
        
        UpdateUI();
    }

    void QDynamicContentEditorMainWindow::GetSelectedPlatforms(QList<QCheckBox*> checkBoxList, QVariantList* selectedPlatformTypes)
    {
        m_addFilesButtonMenu->clear();
        m_addFolderButtonMenu->clear();
        m_manifestTargetPlatforms.clear();

        bool platformsSelected = false;
        SetPlatformLabelIcons();

        for (auto checkBox : checkBoxList)
        {
            if (checkBox->isChecked())
            {
                selectedPlatformTypes->append(checkBox->objectName());
                m_addFilesButtonMenu->addAction(m_platformMap[checkBox->objectName()].DisplayName);
                m_addFolderButtonMenu->addAction(m_platformMap[checkBox->objectName()].DisplayName);
                m_manifestTargetPlatforms.append(checkBox->objectName());
                platformsSelected = true;
            }
            else
            {
                SetIconOpacity(checkBox->objectName());
            }
        }

        if (!platformsSelected)
        {
            m_addFilesButtonMenu = addPlatformMenuActions(m_addFilesButtonMenu);
            m_addFolderButtonMenu = addPlatformMenuActions(m_addFolderButtonMenu);
            SetPlatformLabelIcons();
        }
    }

    QMenu* QDynamicContentEditorMainWindow::addPlatformMenuActions(QMenu* menu)
    {
        QStringList platformNames = m_platformMap.keys();
        platformNames.removeOne("shared");
        for (auto platform : platformNames)
        {
            if (m_platformMap[platform].LicenseExists)
            {
                menu->addAction(m_platformMap[platform].DisplayName);
                m_manifestTargetPlatforms.append(platform);
            }
        }
        return menu;
    }

    void QDynamicContentEditorMainWindow::OnDeleteManifest()
    {
        auto fileName = cbManifestSelection->currentText();
        auto question = QString(tr("Are you sure you want to delete the manifest %1?")).arg(fileName);
        auto reply = QMessageBox::question(this,
            tr("Attention"),
            question,
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes)
        {
            auto fullPath = cbManifestSelection->currentData().toString();
            QVariantMap args;
            args[ARGS_MANIFEST_PATH] = fullPath;
            PythonExecute(COMMAND_DELETE_MANIFEST, args);
        }

        UpdateUI();
    }

    QChangeTargetPlatformDialog::QChangeTargetPlatformDialog(QWidget* parent) : QDialog(parent)
    {
        setupUi(this);

        buttonBox->button(QDialogButtonBox::Ok)->setProperty("class", "Primary");

        QMap<QString, QDynamicContentEditorMainWindow::PlatformInfo> platformMap = QDynamicContentEditorMainWindow::PlatformMap();
        QList<QCheckBox*> checkBoxList = findChildren<QCheckBox*>();
        for (auto checkBox : checkBoxList)
        {
            checkBox->setIcon(QIcon(platformMap[checkBox->objectName()].LargeSizeIconLink));
        }
    }

    void QDynamicContentEditorMainWindow::OnChangeTargetPlatforms()
    {
        QScopedPointer<QChangeTargetPlatformDialog> changeTargetPlatformDialog{new QChangeTargetPlatformDialog(this)};

        QStringList platformNames = m_platformMap.keys();
        for (auto platform : platformNames)
        {
            if (!m_platformMap[platform].LicenseExists)
            {
                QCheckBox* platformCheckBox = changeTargetPlatformDialog->findChild<QCheckBox *>(platform);
                platformCheckBox->hide();
            }

        }

        if (changeTargetPlatformDialog->exec() == QDialog::Accepted)
        {
            QVariantList* selectedPlatformTypes = new QVariantList();
            m_dataRetainer.enqueue(selectedPlatformTypes);

            QList<QCheckBox*> checkBoxList = changeTargetPlatformDialog->findChildren<QCheckBox*>();
            GetSelectedPlatforms(checkBoxList, selectedPlatformTypes);

            SetFilterRegExp(selectedPlatformTypes);

            QVariantMap args;
            args[ARGS_PLATFORM_LIST] = *selectedPlatformTypes;
            args[ARGS_MANIFEST_PATH] = cbManifestSelection->currentData().toString();
            PythonExecute(COMMAND_CHANGE_TARGET_PLATFORMS, args);
        }
    }

    void QDynamicContentEditorMainWindow::SetFilterRegExp(QVariantList* platforms)
    {
        m_fileWatcherProxyModel->setFilterRole(FileWatcherModel::PlatformRole);
        m_fileWatcherProxyModel->setFilterKeyColumn(FileWatcherModel::platformColumn);
        m_packagesProxyModel->setFilterRole(PackagesModel::PlatformRole);
        m_packagesProxyModel->setFilterKeyColumn(PackagesModel::ColumnName::Platform);

        if (platforms->length() > 0)
        {
            QString regExp = "shared";
            for (auto platform : *platforms)
            {
                regExp += "|";
                regExp += platform.toString();
            }

            m_fileWatcherProxyModel->setFilterRegExp(QRegExp(regExp));
            m_packagesProxyModel->setFilterRegExp(QRegExp(regExp));
        }
        else
        {
            m_fileWatcherProxyModel->setFilterRegExp(QRegExp(".*"));
            m_packagesProxyModel->setFilterRegExp(QRegExp(".*"));
        }
    }

    void QDynamicContentEditorMainWindow::OnManifestLoadComplete()
    {
        if (m_isLoading)
        {
            m_isLoading = false;
            UpdateUI();
        }
    }

    QUploadPackagesDialog::QUploadPackagesDialog(QWidget* parent) : QDialog(parent)
    {
        setupUi(this);
        uploadButton->setProperty("class", "Primary");
        SetUploadButtonCurrentStatus(UploadButtonStatus::Upload);
        SetCancelButtonCurrentStatus(CancelButtonStatus::Cancel);
        generateKeyConfirmation->hide();

        connect(checkSignPackages, &QCheckBox::stateChanged, this, &QUploadPackagesDialog::OnCheckBoxStateChanged);
        connect(uploadButton, &QPushButton::released, this, &QUploadPackagesDialog::OnUploadButtonReleased);
        connect(cancelButton, &QPushButton::released, this, &QUploadPackagesDialog::OnCancelButtonReleased);
    }

    void QUploadPackagesDialog::OnUploadButtonReleased()
    {
        if (UploadButtonCurrentStatus() == UploadButtonStatus::Next)
        {
            UpdateUploadPackagesDialogUI();
        }
        else if (UploadButtonCurrentStatus() == UploadButtonStatus::Continue)
        {
            GenerateKeys();
        }
        else if (UploadButtonCurrentStatus() == UploadButtonStatus::Upload)
        {
            accept();
        }
    }

    void QUploadPackagesDialog::OnCancelButtonReleased()
    {
        if (CancelButtonCurrentStatus() == CancelButtonStatus::Back)
        {
            OnResetUploadPackagesDialogUI();
        }
        else if (CancelButtonCurrentStatus() == CancelButtonStatus::Cancel)
        {
            reject();
        }
    }

    void QUploadPackagesDialog::OnCheckBoxStateChanged(int state)
    {
        if (state == Qt::Unchecked)
        {
            keyStatus->hide();

            SetUploadButtonCurrentStatus(UploadButtonStatus::Upload);
            uploadButton->setEnabled(true);
        }
        else if (state == Qt::Checked)
        {
            uploadButton->setEnabled(false);

            CheckExistingKeys();
        }
    }

    bool QUploadPackagesDialog::PackagesSigned()
    {
        return checkSignPackages->isChecked();
    }

    void  QUploadPackagesDialog::OnKeyExists(QVariant keyInfo)
    {
        QMap<QString, QVariant> keyInfoMap = keyInfo.toMap();
        if (keyInfoMap[KEY_KEY_EXISTS].toBool())
        {
            QFileInfo info(keyInfoMap[KEY_PUBLIC_KEY_FILE].toString());
            QDateTime lastModifiedTime = info.lastModified().toLocalTime();

            QString message = QString(tr("Generated %1")).arg(lastModifiedTime.toString());
            keyStatus->setText("<font color='green'>" + message + "</font>");
            keyStatus->show();

        }
        else
        {
            QString message = QString(tr("Security key does not exist. Click \"Next\" to generate a new key."));
            keyStatus->setText("<font color='red'>" + message + "</font>");
            keyStatus->show();

            SetUploadButtonCurrentStatus(UploadButtonStatus::Next);
        }
        uploadButton->setEnabled(true);
    }

    void QUploadPackagesDialog::OnResetUploadPackagesDialogUI()
    {
        setWindowTitle("Upload packages");

        SetUploadButtonCurrentStatus(UploadButtonStatus::Upload);
        SetCancelButtonCurrentStatus(CancelButtonStatus::Cancel);

        GeneralDescription->setText(tr("All packages with this manifest will be uploaded to Amazon S3. Please do not close the Dynamic Content Manager until the upload is completed."));
        securityKeyDescription->show();
        generateKeyConfirmation->hide();

        checkSignPackages->show();
        checkSignPackages->setCheckState(Qt::Checked);
        checkSignPackages->stateChanged(Qt::Checked);   
    }

    void QUploadPackagesDialog::UpdateUploadPackagesDialogUI()
    {
        setWindowTitle("Generate new keys");

        SetUploadButtonCurrentStatus(UploadButtonStatus::Continue);
        SetCancelButtonCurrentStatus(CancelButtonStatus::Back);

        GeneralDescription->setText(tr("The security key can be used to sign all manifests and packages uploaded to S3."));
        securityKeyDescription->hide();  
        generateKeyConfirmation->show();

        keyStatus->hide();
        checkSignPackages->hide();
    }

    void QUploadPackagesDialog::SetUploadButtonCurrentStatus(int status)
    {
        QMap<int, QString> uploadButtonText = UploadButtonText();
        uploadButton->setText(uploadButtonText[status]);
        m_uploadButtonStatus = status;
    }

    void QUploadPackagesDialog::SetCancelButtonCurrentStatus(int status)
    {
        QMap<int, QString> cancelButtonText = CancelButtonText();
        cancelButton->setText(cancelButtonText[status]);
        m_cancelButtonStatus = status;
    }

    void QDynamicContentEditorMainWindow::OnUploadChangedPackages()
    {
        if (!PrepareManifestForWork())
        {
            AZ_Warning("Dynamic Content Editor", false, "Source Control down or operation failed");
        }

        QScopedPointer<QUploadPackagesDialog> uploadPackagesDialog{ new QUploadPackagesDialog(this) };

        connect(uploadPackagesDialog.data(), &QUploadPackagesDialog::CheckExistingKeys, this, &QDynamicContentEditorMainWindow::OnCheckExistingKeys);
        connect(uploadPackagesDialog.data(), &QUploadPackagesDialog::GenerateKeys, this, &QDynamicContentEditorMainWindow::OnGenerateKeys);

        connect(this, &QDynamicContentEditorMainWindow::KeyExists, uploadPackagesDialog.data(), &QUploadPackagesDialog::OnKeyExists);
        connect(this, &QDynamicContentEditorMainWindow::GenerateKeyCompleted, uploadPackagesDialog.data(), &QUploadPackagesDialog::OnResetUploadPackagesDialogUI);

        if (uploadPackagesDialog->exec() == QDialog::Accepted)
        {
            QVariantMap args;
            args[ARGS_MANIFEST_PATH] = cbManifestSelection->currentData().toString();
            args[ARGS_SIGNING] = uploadPackagesDialog->PackagesSigned();
            PythonExecute(COMMAND_PAK_AND_UPLOAD, args);
            m_isUploading = true;

            const char* signStatus = uploadPackagesDialog->PackagesSigned() ? "true" : "false";
            auto metricId = LyMetrics_CreateEvent(DCM_METRIC_EVENT_NAME);
            LyMetrics_AddAttribute(metricId, DCM_OPERATION_ATTRIBUTE_NAME, "upload all");
            LyMetrics_AddAttribute(metricId, "SignPackage", signStatus);
            LyMetrics_SubmitEvent(metricId);

            UpdateUI();
        }
    }

    void QDynamicContentEditorMainWindow::OnGenerateNewKeys()
    {
        QScopedPointer<QUploadPackagesDialog> uploadPackagesDialog{ new QUploadPackagesDialog(this) };
        uploadPackagesDialog->UpdateUploadPackagesDialogUI();
        uploadPackagesDialog->SetCancelButtonCurrentStatus(QUploadPackagesDialog::CancelButtonStatus::Cancel);

        disconnect(this, &QDynamicContentEditorMainWindow::GenerateKeyCompleted, uploadPackagesDialog.data(), &QUploadPackagesDialog::OnResetUploadPackagesDialogUI);
        connect(this, &QDynamicContentEditorMainWindow::GenerateKeyCompleted, uploadPackagesDialog.data(), &QUploadPackagesDialog::accept);
        connect(uploadPackagesDialog.data(), &QUploadPackagesDialog::GenerateKeys, this, &QDynamicContentEditorMainWindow::OnGenerateKeys);

        uploadPackagesDialog->exec();
    }

    void QDynamicContentEditorMainWindow::OnCheckExistingKeys()
    {
        QVariantMap args;
        PythonExecute(COMMAND_CHECK_EXISTING_KEYS, args);
    }

    void QDynamicContentEditorMainWindow::OnOpenDocumentation()
    {
        QString link = "http://docs.aws.amazon.com/lumberyard/latest/developerguide/cloud-canvas-cloud-gem-dc-manager.html";
        QDesktopServices::openUrl(QUrl(link));
    }

    void QDynamicContentEditorMainWindow::OnAddFile(QAction *action)
    {
        auto platform = action->text();

        QVariantMap args;
        args["platform"] = FolderName(platform);
        args["type"] = "file";
        PythonExecute(COMMAND_GET_FULL_PLATFORM_CACHE_GAME_PATH, args);

        auto metricId = LyMetrics_CreateEvent(DCM_METRIC_EVENT_NAME);
        LyMetrics_AddAttribute(metricId, DCM_OPERATION_ATTRIBUTE_NAME, "add file");
        LyMetrics_AddAttribute(metricId, "Platform", platform.toStdString().c_str());
        LyMetrics_SubmitEvent(metricId);
    }

    void QDynamicContentEditorMainWindow::OnAddFolder(QAction *action)
    {
        auto platform = action->text();

        QVariantMap args;
        args["platform"] = FolderName(platform);
        args["type"] = "folder";
        PythonExecute(COMMAND_GET_FULL_PLATFORM_CACHE_GAME_PATH, args);

        auto metricId = LyMetrics_CreateEvent(DCM_METRIC_EVENT_NAME);
        LyMetrics_AddAttribute(metricId, DCM_OPERATION_ATTRIBUTE_NAME, "add folder");
        LyMetrics_AddAttribute(metricId, "Platform", platform.toStdString().c_str());
        LyMetrics_SubmitEvent(metricId);
    }

    void QDynamicContentEditorMainWindow::AddFileFromCache(QString path, QString platform)
    {
        auto manifestName = cbManifestSelection->currentText();

        if (!PrepareManifestForWork())
        {
            AZ_Warning("Dynamic Content Editor", false, "Source Control down or operation failed");
        }

        auto caption = QString(tr("Add files to the manifest - %1")).arg(manifestName);

        auto fileNames = QFileDialog::getOpenFileNames(this,
            caption,
            path);

        if (fileNames.size() > 0)
        {
            // the map will take ownership of this but doesn't copy it, just takes a pointer
            // we need to create on heap otherwise it will get destroyed and cause an exception when
            // other code attempts to delete the map (in this case the PythonWorker retry code)
            QVariantList* vFileNames = new QVariantList();
            m_dataRetainer.enqueue(vFileNames);

            bool fileProcessedByAssetProcessor = true;
            for (auto fileName : fileNames)
            {
                AddFileInfoMap(fileName, vFileNames);

                if (!FileProcessedByAssetProcessor(fileName))
                {
                    fileProcessedByAssetProcessor = false;
                }
            }

            if (!CheckAddedFilesPlatformType(*vFileNames, platform, fileProcessedByAssetProcessor))
            {
                return;
            }

            QVariantMap args;
            args[ARGS_FILE_LIST] = *vFileNames;
            args[ARGS_MANIFEST_PATH] = cbManifestSelection->currentData().toString();
            PythonExecute(COMMAND_ADD_FILES_TO_MANIFEST, args);
        }

        UpdateUI();
    }

    void QDynamicContentEditorMainWindow::AddFolderFromCache(QString path, QString platform)
    {
        auto manifestName = cbManifestSelection->currentText();

        if (!PrepareManifestForWork())
        {
            AZ_Warning("Dynamic Content Editor", false, "Source Control down or operation failed");
        }

        auto caption = QString(tr("Add folders to the manifest - %1")).arg(manifestName);

        auto folderName = QFileDialog::getExistingDirectory(this,
            caption,
            path);

        if (!folderName.isEmpty())
        {
            // the map will take ownership of this but doesn't copy it, just takes a pointer
            // we need to create on heap otherwise it will get destroyed and cause an exception when
            // other code attempts to delete the map (in this case the PythonWorker retry code)
            QVariantList* vFileNames = new QVariantList();
            m_dataRetainer.enqueue(vFileNames);
            ReadAllFilesInFolder(folderName, vFileNames);

            bool fileProcessedByAssetProcessor = FileProcessedByAssetProcessor(folderName);
            if (!CheckAddedFilesPlatformType(*vFileNames, platform, fileProcessedByAssetProcessor))
            {
                return;
            }

            QVariantMap args;
            args[ARGS_FILE_LIST] = *vFileNames;
            args[ARGS_MANIFEST_PATH] = cbManifestSelection->currentData().toString();
            PythonExecute(COMMAND_ADD_FILES_TO_MANIFEST, args);
        }

        UpdateUI();
    }

    bool QDynamicContentEditorMainWindow::FileProcessedByAssetProcessor(QString filePath)
    {
        AZStd::string fullPathToAssets = gEnv->pFileIO->GetAlias("@assets@");
        QString fullPath = fullPathToAssets.c_str();
        fullPath.replace("\\", "/");
        QStringList pathList = fullPath.split("/");

        if (pathList.size() >= 2)
        {
            //Get the directory of the cache folder
            int indexOfCache = pathList.indexOf("cache");
            int pathListLength = pathList.length();
            for (int i = 0; i < pathListLength - indexOfCache - 1; i++)
            {
                pathList.removeLast();
            }
            QString cachePath = pathList.join("/");

            return filePath.contains(cachePath, Qt::CaseInsensitive);
        }

        return false;
    }

    QPlatformWarningDialog::QPlatformWarningDialog(QWidget* parent) : QDialog(parent)
    {
        setupUi(this);
        buttonBox->button(QDialogButtonBox::Ok)->setProperty("class", "Primary");
    }

    void QPlatformWarningDialog::setAddFileWarningMessage(int status)
    {
        if (status == QDynamicContentEditorMainWindow::AddFilesStatus::Unsupport)
        {
            warningLabel->setText(tr("The platform type of the currently selected files is not supported by the manifest. The files will be added to the manifest, but hidden, unless the target platforms for the manifest are updated. Would you like to add these files?"));
        }
        else if (status == QDynamicContentEditorMainWindow::AddFilesStatus::Unmatch)
        {
            warningLabel->setText(tr("The platform type of the current files is not the platform you selected. Would you like to add these files?"));
        }
        else if (status == QDynamicContentEditorMainWindow::AddFilesStatus::NoPlatformInfo)
        {
            setWindowTitle("Error");
            warningLabel->setText(tr("The current files have not been processed by the Asset Processor and are not supported by the Dynamic Content Manager."));
        }
    }

    void QPlatformWarningDialog::setCreatePakWarningMessage()
    {
        warningLabel->setText(tr("The platform type of the new package is not unsupported by the manifest. The package will be created, but hidden, unless the target platforms for this manifest are updated. Would you like to create this package?"));
    }

    bool QDynamicContentEditorMainWindow::CheckAddedFilesPlatformType(QVariantList vFileNames, QString targetPlatform, bool fileProcessedByAssetProcessor)
    {
        int status = AddedFilesPlatformStatus(vFileNames, targetPlatform, fileProcessedByAssetProcessor);
        if (status != AddFilesStatus::Match)
        {
            QScopedPointer<QPlatformWarningDialog> addFilePlatformWarningDialog{ new QPlatformWarningDialog(this) };
            addFilePlatformWarningDialog->setAddFileWarningMessage(status);
            int exec = addFilePlatformWarningDialog->exec();
            if (status == AddFilesStatus::NoPlatformInfo || exec != QDialog::Accepted)
            {
                return false;
            }
        }

        return true;
    }

    int QDynamicContentEditorMainWindow::AddedFilesPlatformStatus(QVariantList vFileNames, QString targetPlatform, bool fileProcessedByAssetProcessor)
    {
        if (!fileProcessedByAssetProcessor)
        {
            return AddFilesStatus::NoPlatformInfo;
        }

        for (auto vFileName : vFileNames)
        {
            auto fileInfo = vFileName.toMap();
            if (fileInfo[KEY_PLATFORM_TYPE].toString() == "")
            {
                return AddFilesStatus::NoPlatformInfo;
            }
            else if (fileInfo[KEY_PLATFORM_TYPE].toString() != targetPlatform)
            {
                if (!m_manifestTargetPlatforms.contains(fileInfo[KEY_PLATFORM_TYPE].toString()))
                {
                    return AddFilesStatus::Unsupport;
                }
                else
                {
                    return AddFilesStatus::Unmatch;
                }
            }
        }
        return AddFilesStatus::Match;
    }

    void QDynamicContentEditorMainWindow::ParseNameAndPlatform(const QString& fileName, QString& localName, QString& platformName) const
    {
        localName = Path::FullPathToGamePath(fileName);

        QString trimmedName = fileName.left(fileName.length() - localName.length());
        trimmedName.replace("\\", "/");
        QStringList pathList = trimmedName.split("/", QString::SkipEmptyParts);

        // Our final directory is our game name, our second to last is our platform path
        if (pathList.size() >= 2)
        {
            platformName = pathList[pathList.size() - 2];
        }
    }

    void QDynamicContentEditorMainWindow::ReadAllFilesInFolder(const QString& folderName, QVariantList* vFileNames)
    {
        QDir dir(folderName);
        if (dir.exists())
        {
            const QFileInfoList fileList = dir.entryInfoList();
            for (auto file: fileList)
            {
                if (file.isDir() && file.fileName() != "." && file.fileName() != "..")
                {
                    ReadAllFilesInFolder(file.absoluteFilePath(), vFileNames);
                }
                else if (file.isFile())
                {
                    AddFileInfoMap(file.filePath(), vFileNames);
                }

            }
        }
    }

    void QDynamicContentEditorMainWindow::AddFileInfoMap(const QString& fileName, QVariantList* vFileNames)
    {
        QVariantMap fileInfoMap;
        QString localName;
        QString platformName;
        ParseNameAndPlatform(fileName, localName, platformName);

        fileInfoMap[KEY_FILE_NAME] = localName;
        fileInfoMap[KEY_PLATFORM_TYPE] = platformName;
        vFileNames->append(fileInfoMap);
    }

    void QDynamicContentEditorMainWindow::OnRemoveFile()
    {
        auto indicies = tableFileWatcher->selectionModel()->selectedIndexes();
        QVariantList* fileInfoList = new QVariantList();
        m_dataRetainer.enqueue(fileInfoList);
        
        for (auto index : indicies)
        {
            auto sourceIndex = m_fileWatcherProxyModel->mapToSource(index);
            auto fileEntry = m_watchedFiles[sourceIndex.row()];
            auto filePath = QDir(fileEntry.localFolder).filePath(fileEntry.keyName);
            auto filePlatform = fileEntry.platformType;

            QVariantMap fileInfo;
            fileInfo[KEY_FILE_NAME] = filePath;
            fileInfo[KEY_PLATFORM_TYPE] = filePlatform;
            fileInfoList->append(fileInfo);
        }

        if (fileInfoList->size() > 0)
        {
            auto reply = QMessageBox::question(this,
                tr("Attention"),
                tr("Are you sure you want to delete the selected files?"),
                QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes)
            {
                QVariantMap args;
                args[ARGS_FILE_LIST] = *fileInfoList;
                args[ARGS_MANIFEST_PATH] = cbManifestSelection->currentData().toString();
                args[ARGS_SECTION] = KEY_FILES_SECTION;
                PythonExecute(COMMAND_DELETE_FILES_FROM_MANIFEST, args);

                auto metricId = LyMetrics_CreateEvent(DCM_METRIC_EVENT_NAME);
                LyMetrics_AddAttribute(metricId, DCM_OPERATION_ATTRIBUTE_NAME, "remove file");
                LyMetrics_SubmitEvent(metricId);
            }
        }

        UpdateUI();
    }

    void QDynamicContentEditorMainWindow::OnRemoveFromPackage()
    {
        auto proxyPakIndex = treePackages->currentIndex();
        auto pakIndex = m_packagesProxyModel->mapToSource(proxyPakIndex);
        // make sure this is actually a file, not the pak
        if (pakIndex.parent().isValid())
        {
            auto pakItem = m_packagesModel->itemFromIndex(pakIndex);
            auto fileName = pakItem->text();
            auto platformIndex = m_packagesModel->index(pakIndex.row(), PackagesModel::ColumnName::Platform, pakIndex.parent());
            auto platformItem = m_packagesModel->itemFromIndex(platformIndex);
            auto filePlatform = platformItem->data(PackagesModel::PlatformRole);

            QVariantMap fileEntry;
            fileEntry[KEY_KEY_NAME] = fileName;
            fileEntry[KEY_PLATFORM_TYPE] = filePlatform.toString();

            QVariantMap args;
            args[ARGS_FILE_DATA] = fileEntry;
            args[ARGS_MANIFEST_PATH] = cbManifestSelection->currentData().toString();
            PythonExecute(COMMAND_DELETE_FILES_FROM_PAK, args);
        }

        UpdateUI();
    }

    QCreatePackageDialog::QCreatePackageDialog(QWidget* parent) : QDialog(parent)
    {
        setupUi(this);

        buttonBox->button(QDialogButtonBox::Ok)->setProperty("class", "Primary");
        packageName->setToolTip("Valid characters are 0-9a-zA-Z!_. and the maximum length is 100");
        SetPlatformButtons();
    }

    void QCreatePackageDialog::SetPlatformButtons()
    {
        platformGroup->setExclusive(true);
        connect(platformGroup, static_cast<void(QButtonGroup::*)(QAbstractButton *)>(&QButtonGroup::buttonClicked), this, &QCreatePackageDialog::OnPlatformSelected);

        QMap<QString, QDynamicContentEditorMainWindow::PlatformInfo> platformMap = QDynamicContentEditorMainWindow::PlatformMap();
        QList<QRadioButton*> radioButtonList = findChildren<QRadioButton*>();
        for (auto radioButton : radioButtonList)
        {
            radioButton->setIcon(QIcon(platformMap[radioButton->objectName()].LargeSizeIconLink));
            if (radioButton->objectName() == "shared")
            {
                radioButton->setChecked(true);
                OnPlatformSelected(radioButton);
            }
        }
    }

    void QCreatePackageDialog::OnPlatformSelected(QAbstractButton * button)
    {
        QString platform = button->objectName();
        packageName->setValidator(new QRegExpValidator(QRegExp("^[-0-9a-zA-Z!_.]{1," + QString::number(94 - platform.length()) + "}$")));
    }

    void QDynamicContentEditorMainWindow::OnAddPackage()
    {
        if (!PrepareManifestForWork())
        {
            AZ_Warning("Dynamic Content Editor", false, "Source Control down or operation failed");
        }

        QScopedPointer<QCreatePackageDialog> createDialog{new QCreatePackageDialog(this)};

        QStringList platformNames = m_platformMap.keys();
        for (auto platform : platformNames)
        {
            if (!m_platformMap[platform].LicenseExists)
            {
                QRadioButton* platformRadioButton = createDialog->findChild<QRadioButton *>(platform);
                platformRadioButton->hide();
            }

        }

        int execCode = createDialog->exec();
        auto newName = createDialog->packageName->text();

        if (execCode > 0)
        {
            auto platformType = createDialog->platformGroup->checkedButton()->objectName();

            if (!m_manifestTargetPlatforms.contains(platformType) && platformType != "shared")
            {
                QScopedPointer<QPlatformWarningDialog> createPakPlatformWarningDialog{ new QPlatformWarningDialog(this) };
                createPakPlatformWarningDialog->setCreatePakWarningMessage();
                if (createPakPlatformWarningDialog->exec() != QDialog::Accepted)
                {
                    return;
                }
            }
            
            QVariantMap args;
            args[ARGS_NEW_FILE_NAME] = newName + "." + platformType;
            args[ARGS_PLATFORM_TYPE] = platformType;
            args[ARGS_MANIFEST_PATH] = cbManifestSelection->currentData().toString();
            PythonExecute(COMMAND_ADD_PAK_TO_MANIFEST, args);

            auto metricId = LyMetrics_CreateEvent(DCM_METRIC_EVENT_NAME);
            LyMetrics_AddAttribute(metricId, DCM_OPERATION_ATTRIBUTE_NAME, "add package");
            LyMetrics_AddAttribute(metricId, "Platform", platformType.toStdString().c_str());
            LyMetrics_SubmitEvent(metricId);
        }

        UpdateUI();
    }

    void QDynamicContentEditorMainWindow::OnRemovePackage()
    {
        auto proxyPakIndex = treePackages->currentIndex();
        auto pakIndex = m_packagesProxyModel->mapToSource(proxyPakIndex);
        // only operate on paks which are top level and therefore parent isn't valid
        if (!pakIndex.parent().isValid())
        {
            auto reply = QMessageBox::question(this,
                tr("Attention"),
                tr("Are you sure you want to delete the selected package?"),
                QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes)
            {
                auto pakItem = m_packagesModel->itemFromIndex(pakIndex);
                if (!pakItem)
                {
                    AZ_Warning("Dynamic Content Editor", false, "Item already deleted");
                    return;
                }
                auto fileEntry = pakItem->text();
                auto platformIndex = m_packagesModel->index(pakIndex.row(), PackagesModel::ColumnName::Platform, pakIndex.parent());
                auto platformItem = m_packagesModel->itemFromIndex(platformIndex);
                auto platform = platformItem->data(PackagesModel::PlatformRole);

                QVariantMap pakInfo;
                pakInfo[KEY_KEY_NAME] = fileEntry;
                pakInfo[KEY_PLATFORM_TYPE] = platform;

                QVariantMap args;
                args[ARGS_FILE_DATA] = pakInfo;
                args[ARGS_MANIFEST_PATH] = cbManifestSelection->currentData().toString();
                PythonExecute(COMMAND_DELETE_PAK_FROM_MANIFEST, args);

                auto metricId = LyMetrics_CreateEvent(DCM_METRIC_EVENT_NAME);
                LyMetrics_AddAttribute(metricId, DCM_OPERATION_ATTRIBUTE_NAME, "delete package");
                LyMetrics_SubmitEvent(metricId);
            }
        }

        UpdateUI();
    }

    void QDynamicContentEditorMainWindow::OnGenerateKeys()
    {
        QVariantMap args;
        args[ARGS_GUI] = true;
        PythonExecute(COMMAND_GENERATE_KEYS, args);
        UpdateUI();
    }


    void QDynamicContentEditorMainWindow::UpdateManifestList(const QVariant& vManifestList)
    {
        cbManifestSelection->clear();

        // add help text
        cbManifestSelection->addItem(tr("Choose/create a manifest"));
        qobject_cast<QStandardItemModel*>(cbManifestSelection->model())->item(0)->setEnabled(false);
        cbManifestSelection->insertSeparator(1);

        // add any manifests found
        auto list = vManifestList.toStringList();
        for (auto item : list)
        {
            auto fileInfo = QFileInfo(item);
            cbManifestSelection->addItem(fileInfo.fileName(), item);
        }

        // add create entry
        cbManifestSelection->insertSeparator(cbManifestSelection->count());
        cbManifestSelection->addItem(tr("Create a new manifest..."));

        SelectCurrentManifest();
    }

    void QDynamicContentEditorMainWindow::SelectCurrentManifest()
    {
        // determine if our current selection is available, if so make sure we remain set to it
        if (m_manifestStatus->m_currentlySelectedManifestName.isEmpty())
        {
            cbManifestSelection->setCurrentIndex(0);
        }
        else
        {
            int currentItem = cbManifestSelection->findText(m_manifestStatus->m_currentlySelectedManifestName);
            currentItem = currentItem == -1 ? 0 : currentItem;
            cbManifestSelection->setCurrentIndex(currentItem);
        }
        UpdateUI();
    }

    void QDynamicContentEditorMainWindow::OnSelectManifest(int index)
    {
        // index 0 always contains the help text
        if (index > 0)
        {
            // last item is always the create
            if (index == cbManifestSelection->count() - 1)
            {
                OnNewManifest();
            }
            else
            {
                auto fullPath = cbManifestSelection->itemData(index).toString();
                auto fileName = cbManifestSelection->itemText(index);

                QFont font = manifestName->font();
                font.setPointSize(18);
                manifestName->setFont(font);

                QFontMetrics fm(font);
                QString text = QFileInfo(fileName).completeBaseName();
                if (fm.width(text) > manifestName->maximumWidth())
                {
                    text = fm.elidedText(text, Qt::ElideRight, manifestName->maximumWidth() - 2);
                }
                manifestName->setText(text + "  "); // spaces leave a bit of a gap between this and the platform icons
                manifestName->adjustSize();

                m_manifestStatus->m_currentlySelectedManifestName = fileName;
                m_manifestStatus->m_fullPathToManifest = fullPath;
                // I clear the model on anticipation of new data coming in to the model from python
                // Trying to clear the model and add new data in one step in the model caused crashes
                // due to the async nature of the update
                m_packagesModel->clear();
                UpdateManifestSourceControlState();
                if(fileName.length())
                {
                    OpenManifest(fullPath);
                }

                UpdateUI();
            }
        }
    }

    void QDynamicContentEditorMainWindow::OpenManifest(const QString& filePath)
    {
        QVariantMap args;
        args[ARGS_MANIFEST_PATH] = filePath;
        args[ARGS_SECTION] = KEY_FILES_SECTION;

        m_isLoading = true;
        status->showMessage("Retriving file info from " + filePath);

        PythonExecute(COMMAND_SHOW_MANIFESTS, args);
        PythonExecute(COMMAND_GET_BUCKET_STATUS, args);
        PythonExecute(COMMAND_GET_LOCAL_FILE_STATUS, args);
    
        UpdateUI();
    }

    void QDynamicContentEditorMainWindow::UpdateManifestSourceControlState()
    {
        QSharedPointer<ManifestInfo> sourcePtr = m_manifestStatus;
        sourcePtr->m_manifestSourceFileInfo.m_status = AzToolsFramework::SCS_ProviderIsDown;
        sourcePtr->m_manifestSourceFileInfo.m_flags = 0;

        QString requestedName{ m_manifestStatus->m_currentlySelectedManifestName };
        if (!requestedName.length())
        {
            return;
        }
        using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
        SCCommandBus::Broadcast(&SCCommandBus::Events::GetFileInfo, sourcePtr->m_fullPathToManifest.toStdString().c_str(),
            [requestedName, sourcePtr](bool wasSuccess, const AzToolsFramework::SourceControlFileInfo& fileInfo)
        {
            if (sourcePtr->m_currentlySelectedManifestName == requestedName)
            {
                sourcePtr->m_manifestSourceFileInfo = fileInfo;
            }
        });
    }

    bool QDynamicContentEditorMainWindow::PrepareManifestForWork()
    {
        // ensure we're connected to source control, free of connection errors.
        if (!m_manifestStatus->m_manifestSourceFileInfo.CompareStatus(AzToolsFramework::SCS_OpSuccess))
        {
            return false;
        }

        if (m_manifestStatus->m_manifestSourceFileInfo.HasFlag(AzToolsFramework::SCF_OpenByUser))
        {
            return true;
        }

        QString checkoutString = m_manifestStatus->m_currentlySelectedManifestName;
        checkoutString += " needs checkout.\n";

        checkoutString += "\nCheck out now?";

        auto reply = QMessageBox::question(
            this,
            "File needs checkout",
            checkoutString,
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes)
        {
            RequestEditManifest();
        }

        return m_manifestStatus->m_manifestSourceFileInfo.HasFlag(AzToolsFramework::SCF_OpenByUser);
    }

    void QDynamicContentEditorMainWindow::RequestEditManifest()
    {
        QSharedPointer<ManifestInfo> sourcePtr = m_manifestStatus;        
        QString requestedName{ m_manifestStatus->m_currentlySelectedManifestName };
        bool opComplete = false;

        using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
        AzToolsFramework::SourceControlResponseCallback scCallback = 
            [&opComplete, &sourcePtr, requestedName](bool, AzToolsFramework::SourceControlFileInfo fileInfo)
        {
            if (sourcePtr->m_currentlySelectedManifestName == requestedName)
            {
                sourcePtr->m_manifestSourceFileInfo = fileInfo;
                opComplete = true;
            }
        };

        SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit, sourcePtr->m_fullPathToManifest.toStdString().c_str(), true, scCallback);
        while (!opComplete)
        {
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 16);
        }
    }

    void QDynamicContentEditorMainWindow::UpdateManifest(const QVariant& vManifestInfo)
    {
        m_watchedFiles.clear();
        m_pakFiles.clear();
        SetPlatformLabelIcons();

        auto sections = vManifestInfo.value<QVariantMap>();

        auto vWatchedFiles = sections[KEY_FILES_SECTION].value<QVariantList>();
        for (auto&& vFile : vWatchedFiles)
        {
            auto newFile = vFile.value<QVariantMap>();
            m_watchedFiles.append(ManifestFile(newFile));
        }

        auto vPakFiles = sections[KEY_PAKS_SECTION].value<QVariantList>();
        for (auto&& vFile : vPakFiles)
        {
            auto newFile = vFile.value<QVariantMap>();
            m_pakFiles.append(ManifestFile(newFile));
        }

        auto vPlatforms = sections[KEY_PLATFORMS_SECTION].value<QVariantList>();
        SetFilterRegExp(&vPlatforms);

        m_addFilesButtonMenu->clear();
        m_addFolderButtonMenu->clear();
        m_manifestTargetPlatforms.clear();

        if (vPlatforms.length() > 0)
        {
            QStringList selectedPlatforms;
            for (auto&& vPlatform : vPlatforms)
            {
                QLabel* selectedPlatform = findChild<QLabel *>(vPlatform.toString());
                selectedPlatforms.append(vPlatform.toString());

                QString platformName = vPlatform.toString();
                m_addFilesButtonMenu->addAction(m_platformMap[platformName].DisplayName);
                m_addFolderButtonMenu->addAction(m_platformMap[platformName].DisplayName);
                m_manifestTargetPlatforms.append(platformName);
            }

            QList<QString> platformList = m_platformMap.keys();
            platformList.removeOne("shared");
            for (auto platform : platformList) {
                if (!selectedPlatforms.contains(platform))
                {
                    SetIconOpacity(platform);
                }
            }
        }
        else
        {
            m_addFilesButtonMenu = addPlatformMenuActions(m_addFilesButtonMenu);
            m_addFolderButtonMenu = addPlatformMenuActions(m_addFolderButtonMenu);
        }

        m_fileWatcherModel->Update(m_watchedFiles);
        m_packagesModel->Update(m_pakFiles, m_watchedFiles, sections[KEY_PAKSTATUS_SECTION]);

        UpdateUI();
    }

    void QDynamicContentEditorMainWindow::UpdateBucketDiff(const QVariant& vBucketDiffInfo)
    {
        auto bucketDiffInfo = vBucketDiffInfo.value<QVariantMap>();
        m_packagesModel->UpdateBucketDiffInfo(bucketDiffInfo);

        UpdateUI();
    }

    ManifestFiles QDynamicContentEditorMainWindow::GetWatchedFilesFromFileInfoListAndPlatform(const QStringList& fileInfoList, const QString& platformType)
    {
        ManifestFiles indexList;

        const bool checkPlatform = platformType.length() && platformType != "shared";
        for (auto&& file : m_watchedFiles)
        {
            if (checkPlatform && file.platformType.length() && file.platformType!= platformType)
            {
                continue;
            }

            auto fileName = file.localFolder;
            fileName = Path::FullPathToGamePath(fileName) + '/';
            fileName += file.keyName;

            auto fileInfo = FileWatcherModel::fileInfoConstructedByNameAndPlatform(fileName, file.platformType);
            int foundAt = fileInfoList.indexOf(fileInfo);
            if (foundAt >= 0)
            {
                indexList.append(file);
            }
        }

        return indexList;
    }

    void QDynamicContentEditorMainWindow::AddFilesToPackage(const ManifestFiles& manifestFiles, const QModelIndex& pakIndex)
    {
        if (!PrepareManifestForWork())
        {
            AZ_Warning("Dynamic Content Editor", false, "Source Control down or operation failed");
        }

        if (pakIndex.isValid())
        {
            auto pakItem = m_packagesModel->itemFromIndex(pakIndex);
            auto selectedPakFile = pakItem->text();

            QVariantList* files = new QVariantList();
            m_dataRetainer.enqueue(files);
            for (auto fileEntry : manifestFiles)
            {
                files->append(fileEntry.GetVariantMap());
            }

            QVariantMap args;
            args[ARGS_PAK_FILE] = selectedPakFile;
            args[ARGS_FILE_LIST] = *files;
            args[ARGS_MANIFEST_PATH] = cbManifestSelection->currentData().toString();
            PythonExecute(COMMAND_ADD_FILES_TO_PAK, args);
        }

        UpdateUI();
    }

    QIncompatibleFilesDialog::QIncompatibleFilesDialog(QWidget* parent) : QDialog(parent)
    {
        setupUi(this);

        buttonBox->button(QDialogButtonBox::Ok)->setProperty("class", "Primary");
    }

    void QIncompatibleFilesDialog::AddIncompatibleFile(const QString& imcompatibleFileName)
    {
        incompatibleFilesList->addItem(imcompatibleFileName);
    }

    void QIncompatibleFilesDialog::SetIncompatibleFilesDialogText(const QString& pakName, const QString& platformType)
    {
        QString message = QString(tr("Some files cannot be added to the package %1 because they are incompatible with the package's target platform %2.")).arg(pakName, platformType);
        alert->setText(message);

        message = QString(tr("These files will not be added. Continue with adding the remaining %1 compatible files?")).arg(platformType);
        question->setText(message);
    }

    void QDynamicContentEditorMainWindow::AddFilesToPackageFromFileInfoList(const QStringList& fileInfoList, const QModelIndex& pakIndex)
    {
        QString platformType;
        QString pakName;
        if (pakIndex.isValid() && pakIndex.row() < m_pakFiles.size())
        {
            ManifestFile thisEntry = m_pakFiles[pakIndex.row()];
            platformType = thisEntry.platformType;
            pakName = thisEntry.keyName;
        }
        auto files = GetWatchedFilesFromFileInfoListAndPlatform(fileInfoList,platformType);

        if (files.length() != fileInfoList.length())
        {
            QScopedPointer<QIncompatibleFilesDialog> incompatibleFilesDialog{ new QIncompatibleFilesDialog(this) };
            incompatibleFilesDialog->SetIncompatibleFilesDialogText(pakName, m_platformMap[platformType].DisplayName);

            QStringList compatibleFileInfos;
            for (auto file : files)
            {
                auto fileName = file.localFolder;
                fileName = Path::FullPathToGamePath(fileName) + '/';
                fileName += file.keyName;
                compatibleFileInfos.append(FileWatcherModel::fileInfoConstructedByNameAndPlatform(fileName, file.platformType));
            }

            for (auto fileInfo : fileInfoList)
            {
                QString fileName = FileWatcherModel::fileNameDeconstructedfromfileInfo(fileInfo);
                if (!compatibleFileInfos.contains(fileInfo))
                {
                    incompatibleFilesDialog->AddIncompatibleFile(fileName);
                }
            }

            if (!incompatibleFilesDialog->exec() == QDialog::Accepted)
            {
                return;
            }
        }
        AddFilesToPackage(files, pakIndex);
    }

    bool QDynamicContentEditorMainWindow::CheckPlatformType(const QStringList& fileInfoList, const QModelIndex& pakIndex)
    {
        QString platformType;
        if (pakIndex.isValid() && pakIndex.row() < m_pakFiles.size())
        {
            ManifestFile thisEntry = m_pakFiles[pakIndex.row()];
            platformType = thisEntry.platformType;
        }

        auto files = GetWatchedFilesFromFileInfoListAndPlatform(fileInfoList, platformType);
        if (files.length() == 0)
        {
            return false;
        }

        return true;
    }

    void QDynamicContentEditorMainWindow::UpdateLocalDiff(const QVariant& vDiffInfo)
    {
        m_packagesModel->UpdateLocalDiffInfo(vDiffInfo);

        UpdateUI();
    }

    void QDynamicContentEditorMainWindow::UpdateUI()
    {
        // assume everything is enabled and visible unless there is a reason not to
        bool removeFilesFromWatcherEnabled = true;
        bool deletePackageEnabled = true;
        bool removeFilesFromPackageEnabled = true;
        bool uploadVisible = true;
        bool deleteWidgetVisible = true;
        bool fileWatcherVisible = true;
        bool packagesVisible = true;
        bool manifestSelectionVisible = true;
        bool noStackErrorVisible = false;

        uploadVisible = !m_isUploading;
        // if no file selected disable add to pack, remove file from watcher
        if (!tableFileWatcher->selectionModel()->hasSelection())
        {
            removeFilesFromWatcherEnabled = false;
        }

        // if no pak selected disable add to pack, delete pak
        // if no file selected in pak disable remove files from package
        auto proxyPakIndex = treePackages->currentIndex();
        auto pakIndex = m_packagesProxyModel->mapToSource(proxyPakIndex);
        // pak files have no parent, contents of pak files to
        if (pakIndex.isValid())
        {
            if (pakIndex.parent().isValid())
            {
                deletePackageEnabled = false;
            }
            else
            {
                removeFilesFromPackageEnabled = false;
            }
        }
        else
        {
            deletePackageEnabled = false;
            removeFilesFromPackageEnabled = false;
        }

        // if no watched files disable appropriate buttons that operate on them
        if (m_watchedFiles.count() == 0)
        {
            removeFilesFromWatcherEnabled = false;
        }

        // if no pak files, disable appropriate buttons that operate on them
        if (m_pakFiles.count() == 0)
        {
            uploadVisible = false;
            deletePackageEnabled = false;
            removeFilesFromPackageEnabled = false;
        }

        // nothing else matters if there are no manifests loaded, override all other checks here
        if (cbManifestSelection->count() == 0 || cbManifestSelection->currentIndex() <= 1 || GetNoStackState())
        {
            removeFilesFromWatcherEnabled = false;
            deletePackageEnabled = false;
            removeFilesFromPackageEnabled = false;
            uploadVisible = false;
            deleteWidgetVisible = false;
            fileWatcherVisible = false;
            packagesVisible = false;
        }

        if (GetNoStackState())
        {
            manifestSelectionVisible = false;
            noStackErrorVisible = true;
        }

        btnRemoveFile->setEnabled(removeFilesFromWatcherEnabled);

        btnRemovePackage->setEnabled(deletePackageEnabled);

        btnRemoveFromPack->setEnabled(removeFilesFromPackageEnabled);

        btnUpload->setVisible(uploadVisible);

        widgetDeleteManifest->setVisible(deleteWidgetVisible);

        grpFileWatcher->setVisible(fileWatcherVisible);

        grpPackages->setVisible(packagesVisible);

        cbManifestSelection->setVisible(manifestSelectionVisible);

        lblNoStackError->setVisible(noStackErrorVisible);
    }

    void QDynamicContentEditorMainWindow::OnFileWatcherSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
    {
        Q_UNUSED(selected);
        Q_UNUSED(deselected);
        UpdateUI();
    }

    void QDynamicContentEditorMainWindow::OnPakTreeSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
    {
        Q_UNUSED(selected);
        Q_UNUSED(deselected);
        UpdateUI();
    }

    void QDynamicContentEditorMainWindow::OnSignalUploadComplete()
    {
        if (m_isUploading)
        {
            m_isUploading = false;
            UpdateUI();
        }
    }

    void QDynamicContentEditorMainWindow::OnLeftPaneContextMenuRequested(QPoint pos)
    {
        int selectedRow = tableFileWatcher->indexAt(pos).row();
        m_contentMenu->clear();

        if (selectedRow != -1)
        {
            auto menuAction = m_contentMenu->addAction("Remove File");
            connect(menuAction, &QAction::triggered, this, &QDynamicContentEditorMainWindow::OnRemoveFile);
        }

        m_contentMenu->popup(tableFileWatcher->viewport()->mapToGlobal(pos));
    }

    void QDynamicContentEditorMainWindow::OnRightPaneContextMenuRequested(QPoint pos)
    {
        int selectedRow = treePackages->indexAt(pos).row();
        m_contentMenu->clear();

        if (selectedRow != -1)
        {
            if (btnRemoveFromPack->isEnabled())
            {
                auto menuAction = m_contentMenu->addAction("Remove File from Package");
                connect(menuAction, &QAction::triggered, this, &QDynamicContentEditorMainWindow::OnRemoveFromPackage);
            }
            if (btnRemovePackage->isEnabled())
            {
                auto menuAction = m_contentMenu->addAction("Delete Package");
                connect(menuAction, &QAction::triggered, this, &QDynamicContentEditorMainWindow::OnRemovePackage);
            }
        }
        else
        {
            auto menuAction = m_contentMenu->addAction("New Package");
            connect(menuAction, &QAction::triggered, this, &QDynamicContentEditorMainWindow::OnAddPackage);
        }

        m_contentMenu->popup(treePackages->viewport()->mapToGlobal(pos));
    }

    void QDynamicContentEditorMainWindow::OnSortingLeftPaneByColumn(int column)
    {
        if (column == FileWatcherModel::InPakColumn)
        {
            m_fileWatcherProxyModel->setSortRole(Qt::DecorationRole);
        }
        else if (column == FileWatcherModel::platformColumn)
        {
            m_fileWatcherProxyModel->setSortRole(FileWatcherModel::PlatformRole);
        }
        else
        {
            m_fileWatcherProxyModel->setSortRole(Qt::DisplayRole);
        }

        tableFileWatcher->sortByColumn(column);
    }

    void QDynamicContentEditorMainWindow::OnSortingRightPaneByColumn(int column)
    {
        if (column == PackagesModel::ColumnName::Platform)
        {
            m_packagesProxyModel->setSortRole(PackagesModel::PlatformRole);
        }
        else if (column == PackagesModel::ColumnName::S3Status)
        {
            m_packagesProxyModel->setSortRole(PackagesModel::S3StatusRole);
        }
        else
        {
            m_packagesProxyModel->setSortRole(Qt::DisplayRole);
        }

        treePackages->sortByColumn(column);
    }

    void QDynamicContentEditorMainWindow::SetPakStatusOutdated(QString pakName)
    {
        m_packagesModel->SetPakStatus(pakName, PackagesModel::PakStatus::Outdated);
    }
}

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
#include "StdAfx.h"
#include "WelcomeScreenDialog.h"
#include <AzFramework/API/ApplicationAPI.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <WelcomeScreen/ui_WelcomeScreenDialog.h>
#include "LevelFileDialog.h"

#include <Core/QtEditorApplication.h>
#include <Editor/MainWindow.h>

#include <QStringListModel>
#include <QListView>
#include <QToolTip>
#include <QMenu>
#include <QDesktopServices>
#include <QFileDialog>
#include <QUrl>
#include <QTimer>
#include <QMessageBox>
#include <QApplication>
#include <QDesktopWidget>
#include <QSignalBlocker>

#include "NewsShared/ResourceManagement/ResourceManifest.h"
#include "NewsShared/Qt/ArticleViewContainer.h"

#include <LyMetricsProducer/LyMetricsAPI.h>

using namespace AzQtComponents;

#define WMSEVENTNAME "WMSEvent"
#define WMSEVENTOPERATION "operation"

static int GetSmallestScreenHeight()
{
    QDesktopWidget* desktopWidget = QApplication::desktop();

    int smallestHeight = -1;
    int screenCount = desktopWidget->screenCount();
    for (int i = 0; i < screenCount; i++)
    {
        int screenHeight = desktopWidget->availableGeometry(i).height();
        if ((smallestHeight < 0) || (smallestHeight > screenHeight))
        {
            smallestHeight = screenHeight;
        }
    }

    return smallestHeight;
}

WelcomeScreenDialog::WelcomeScreenDialog(QWidget* pParent)
    : QDialog(new WindowDecorationWrapper(WindowDecorationWrapper::OptionAutoAttach | WindowDecorationWrapper::OptionAutoTitleBarButtons, pParent), Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
    , ui(new Ui::WelcomeScreenDialog)
    , m_pRecentListModel(new QStringListModel(this))
    , m_pRecentList(nullptr)
{
    ui->setupUi(this);

    // Make our welcome screen checkboxes appear as toggle switches
    AzQtComponents::CheckBox::applyToggleSwitchStyle(ui->enableUI20);
    AzQtComponents::CheckBox::applyToggleSwitchStyle(ui->autoLoadLevel);
    AzQtComponents::CheckBox::applyToggleSwitchStyle(ui->showOnStartup);

    ui->autoLoadLevel->setChecked(gSettings.bAutoloadLastLevelAtStartup);
    ui->enableUI20->setChecked(gSettings.bEnableUI2);

    ui->recentLevelList->setModel(m_pRecentListModel);
    ui->recentLevelList->setMouseTracking(true);
    ui->recentLevelList->setContextMenuPolicy(Qt::CustomContextMenu);

    auto currentProjectButtonMenu = new QMenu();

    // If the current project is external to the engine folder, then do not add options to launch AP or Project Configurator
    // because this feature is in preview mode.
    bool isProjectExternal = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(isProjectExternal, &AzFramework::ApplicationRequests::IsEngineExternal);

    if (!isProjectExternal)
    {

        auto switchProjAction = currentProjectButtonMenu->addAction("Switch project...");
        auto openSAAction = currentProjectButtonMenu->addAction("Setup Assistant...");

        QObject::connect(switchProjAction, &QAction::triggered, this, [this] {
            // close this dialog first before attempting to close the editor
            CCryEditApp* cryEdit = CCryEditApp::instance();
            if (cryEdit->OpenProjectConfiguratorSwitchProject())
            {
                // close the dialog box before closing the editor
                accept();

                SendMetricsEvent("SwitchProjectButtonClicked");
            }
        });

        QObject::connect(openSAAction, &QAction::triggered, this, [this] {
            // close this dialog first before attempting to close the editor
            CCryEditApp* cryEdit = CCryEditApp::instance();

            const QString closeMsg = tr(
                "You must close the Editor before opening Setup Assistant.\nDo you want to close the editor before continuing to the Setup Assistant?");
            if (cryEdit->ToExternalToolPrompt(closeMsg, "Editor"))
            {
                // close the dialog box before closing the editor
                accept();
                if (cryEdit->OpenSetupAssistant())
                {
                    // close the window at the end of the qt event loop
                    QTimer::singleShot(0, []() {MainWindow::instance()->close(); });
                }

                SendMetricsEvent("SetupAssistantButtonClicked");
            }
        });
    }

    ui->currentProjectButton->setMenu(currentProjectButtonMenu);
    ui->currentProjectButton->setText(gEnv->pConsole->GetCVar("sys_game_folder")->GetString());
    ui->currentProjectButton->adjustSize();
    ui->currentProjectButton->setMinimumWidth(ui->currentProjectButton->width() + 40);

    ui->documentationLink->setCursor(Qt::PointingHandCursor);
    ui->documentationLink->installEventFilter(this);

    connect(ui->recentLevelList, &QWidget::customContextMenuRequested, this, &WelcomeScreenDialog::OnShowContextMenu);

    connect(ui->recentLevelList, &QListView::entered, this, &WelcomeScreenDialog::OnShowToolTip);
    connect(ui->recentLevelList, &QListView::clicked, this, &WelcomeScreenDialog::OnRecentLevelListItemClicked);

    connect(ui->newLevelButton, &QPushButton::clicked, this, &WelcomeScreenDialog::OnNewLevelBtnClicked);
    connect(ui->openLevelButton, &QPushButton::clicked, this, &WelcomeScreenDialog::OnOpenLevelBtnClicked);

    connect(ui->newSliceButton, &QPushButton::clicked, this, &WelcomeScreenDialog::OnNewSliceBtnClicked);
    connect(ui->openSliceButton, &QPushButton::clicked, this, &WelcomeScreenDialog::OnOpenSliceBtnClicked);

    connect(ui->documentationButton, &QPushButton::clicked, this, &WelcomeScreenDialog::OnDocumentationBtnClicked);
    connect(ui->showOnStartup, &QCheckBox::clicked, this, &WelcomeScreenDialog::OnShowOnStartupBtnClicked);
    connect(ui->autoLoadLevel, &QCheckBox::clicked, this, &WelcomeScreenDialog::OnAutoLoadLevelBtnClicked);
    connect(ui->enableUI20, &QCheckBox::toggled, this, &WelcomeScreenDialog::OnEnableUI20Toggled);

    m_manifest = new News::ResourceManifest(
            std::bind(&WelcomeScreenDialog::SyncSuccess, this),
            std::bind(&WelcomeScreenDialog::SyncFail, this, std::placeholders::_1),
            std::bind(&WelcomeScreenDialog::SyncUpdate, this, std::placeholders::_1, std::placeholders::_2));
    
    m_articleViewContainer = new News::ArticleViewContainer(this, *m_manifest);
    connect(m_articleViewContainer, &News::ArticleViewContainer::scrolled,
        this, &WelcomeScreenDialog::previewAreaScrolled);
    connect(m_articleViewContainer, &News::ArticleViewContainer::linkActivatedSignal,
        this, &WelcomeScreenDialog::linkActivated);
    ui->articleViewContainerRoot->layout()->addWidget(m_articleViewContainer);

    m_manifest->Sync();

#ifndef ENABLE_SLICE_EDITOR
    ui->newSliceButton->hide();
    ui->openSliceButton->hide();
#endif

    // Adjust the height, if need be
    // Do it in the constructor so that the WindowDecoratorWrapper handles it correctly
    int smallestHeight = GetSmallestScreenHeight();
    if (smallestHeight < geometry().height())
    {
        const int SOME_PADDING_IN_PIXELS = 90;
        int difference = geometry().height() - (smallestHeight - SOME_PADDING_IN_PIXELS);

        QRect newGeometry = geometry().adjusted(0, difference / 2, 0, -difference / 2);
        setMinimumSize(minimumSize().width(), newGeometry.height());
        resize(newGeometry.size());
    }
}


WelcomeScreenDialog::~WelcomeScreenDialog()
{
    delete ui;
    delete m_manifest;
}

void WelcomeScreenDialog::done(int result)
{
    if (m_waitingOnAsync)
    {
        m_manifest->Abort();
    }

    QDialog::done(result);
}

const QString& WelcomeScreenDialog::GetLevelPath()
{
    return m_levelPath;
}

bool WelcomeScreenDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->documentationLink)
    {
        if (event->type() == QEvent::MouseButtonRelease)
        {
            OnDocumentationBtnClicked(false);
            return true;
        }
    }

    return QDialog::eventFilter(watched, event);
}

void WelcomeScreenDialog::SetRecentFileList(RecentFileList* pList)
{
    if (!pList)
    {
        return;
    }

    m_pRecentList = pList;

    QString gamePath = Path::GetExecutableParentDirectory() + QDir::separator() + gEnv->pConsole->GetCVar("sys_game_folder")->GetString();
    Path::ConvertSlashToBackSlash(gamePath);
    gamePath = Path::ToUnixPath(gamePath.toLower());
    gamePath = Path::AddSlash(gamePath);

    QString sCurDir = (Path::GetEditingGameDataFolder() + QDir::separator().toLatin1()).c_str();
    int nCurDir = sCurDir.length();

    int recentListSize = pList->GetSize();
    for (int i = 0; i < recentListSize; ++i)
    {
        if (CFileUtil::Exists(pList->m_arrNames[i], false))
        {
            QString sCurEntryDir = pList->m_arrNames[i].left(nCurDir);

            if (sCurEntryDir.compare(sCurDir, Qt::CaseInsensitive) != 0)
            {
                //unavailable entry (wrong directory)
                continue;
            }
        }
        else
        {
            //invalid entry (not existing)
            continue;
        }

        QString fullPath = pList->m_arrNames[i];
        QString name = Path::GetFileName(fullPath);

        Path::ConvertSlashToBackSlash(fullPath);
        fullPath = Path::ToUnixPath(fullPath.toLower());
        fullPath = Path::AddSlash(fullPath);

        if (fullPath.contains(gamePath))
        {
            m_pRecentListModel->setStringList(m_pRecentListModel->stringList() << QString(name));
            m_levels.push_back(std::make_pair(name, pList->m_arrNames[i]));
        }
    }

    ui->recentLevelList->setCurrentIndex(QModelIndex());
    int rowSize = ui->recentLevelList->sizeHintForRow(0) + ui->recentLevelList->spacing() * 2;
    ui->recentLevelList->setMinimumHeight(m_pRecentListModel->rowCount() * rowSize);
    ui->recentLevelList->setMaximumHeight(m_pRecentListModel->rowCount() * rowSize);
}


void WelcomeScreenDialog::RemoveLevelEntry(int index)
{
    TNamePathPair levelPath = m_levels[index];

    m_pRecentListModel->removeRow(index);
    m_levels.erase(m_levels.begin() + index);


    if (!m_pRecentList)
    {
        return;
    }

    for (int i = 0; i < m_pRecentList->GetSize(); ++i)
    {
        QString fullPath = m_pRecentList->m_arrNames[i];
        QString fullPath2 = levelPath.second;

        // path from recent list
        Path::ConvertSlashToBackSlash(fullPath);
        fullPath = Path::ToUnixPath(fullPath.toLower());
        fullPath = Path::AddPathSlash(fullPath);

        // path from our dashboard list
        Path::ConvertSlashToBackSlash(fullPath2);
        fullPath2 = Path::ToUnixPath(fullPath2.toLower());
        fullPath2 = Path::AddPathSlash(fullPath2);

        if (fullPath == fullPath2)
        {
            m_pRecentList->Remove(index);
            break;
        }
    }

    m_pRecentList->WriteList();
}


void WelcomeScreenDialog::OnShowToolTip(const QModelIndex& index)
{
    const QString& fullPath = m_levels[index.row()].second;

    //TEMPORARY:Begin This can be put back once the main window is in Qt
    //QRect itemRect = ui->recentLevelList->visualRect(index);
    QToolTip::showText(QCursor::pos(), QString("Open level: %1").arg(fullPath) /*, ui->recentLevelList, itemRect*/);
    //TEMPORARY:END
}


void WelcomeScreenDialog::OnShowContextMenu(const QPoint& pos)
{
    QModelIndex index = ui->recentLevelList->indexAt(pos);
    if (index.isValid())
    {
        QString level = m_pRecentListModel->data(index, 0).toString();

        QPoint globalPos = ui->recentLevelList->viewport()->mapToGlobal(pos);

        QMenu contextMenu;
        contextMenu.addAction(QString("Remove " + level + " from recent list"));
        QAction* selectedItem = contextMenu.exec(globalPos);
        if (selectedItem)
        {
            RemoveLevelEntry(index.row());

            SendMetricsEvent("RemovedLevelFromRecentLevelList");
        }
    }
}


void WelcomeScreenDialog::OnNewLevelBtnClicked(bool checked)
{
    m_levelPath = "new";
    accept();

    SendMetricsEvent("NewLevelButtonClicked");
}


void WelcomeScreenDialog::OnOpenLevelBtnClicked(bool checked)
{
    CLevelFileDialog dlg(true, this);

    if (dlg.exec() == QDialog::Accepted)
    {
        m_levelPath = dlg.GetFileName();
        accept();
    }

    SendMetricsEvent("OpenLevelButtonClicked");
}

void WelcomeScreenDialog::OnNewSliceBtnClicked(bool checked)
{
    m_levelPath = "new slice";
    accept();

    SendMetricsEvent("NewSliceButtonClicked");
}

void WelcomeScreenDialog::OnOpenSliceBtnClicked(bool)
{
    QString fileName = QFileDialog::getOpenFileName(MainWindow::instance(),
        tr("Open Slice"),
        Path::GetEditingGameDataFolder().c_str(),
        tr("Slice (*.slice)"));

    if (!fileName.isEmpty())
    {
        m_levelPath = fileName;
        accept();
    }

    SendMetricsEvent("OpenSliceButtonClicked");
}

void WelcomeScreenDialog::OnRecentLevelListItemClicked(const QModelIndex& modelIndex)
{
    int index = modelIndex.row();

    if (index >= 0 && index < m_levels.size())
    {
        m_levelPath = m_levels[index].second;
        accept();
    }

    SendMetricsEvent("LoadedLevelFromRecentLevelList");
}


void WelcomeScreenDialog::OnCloseBtnClicked(bool checked)
{
    accept();
}

void WelcomeScreenDialog::OnEnableUI20Toggled(bool checked)
{
    QMessageBox restartMessage(AzToolsFramework::GetActiveWindow());
    restartMessage.setWindowTitle(QObject::tr("Restart"));
    restartMessage.setText(QObject::tr("You must restart the Lumberyard Editor to finish enabling or disabling UI 2.0. Select 'Shutdown' to stop the Editor, and then relaunch it to complete the changes."));
    QPushButton* shutdownButton = restartMessage.addButton(QObject::tr("Shutdown"), QMessageBox::AcceptRole);
    restartMessage.addButton(QObject::tr("Cancel"), QMessageBox::RejectRole);
    restartMessage.exec();

    // Close the Editor on behalf of the user.
    if (restartMessage.clickedButton() == shutdownButton)
    {
        // Save our toggled setting
        gSettings.bEnableUI2 = checked;
        gSettings.Save();

        // Send a Metrics event since we changed the UI 2.0 setting
        if (gSettings.bEnableUI2)
        {
            SendMetricsEvent("EditorUI10Off");
        }
        else
        {
            SendMetricsEvent("EditorUI10On");
        }

        // Queue the MainWindow close event to be executed on the next event loop pass
        QTimer::singleShot(0, MainWindow::instance(), &MainWindow::close);

        // Then we need to dismiss ourself (the Welcome Screen modal dialog), or else the MainWindow close event will be ignored
        reject();
    }
    // Otherwise, we cancelled, so reset the checkbox to the previous value
    else
    {
        QSignalBlocker blocker(ui->enableUI20);
        ui->enableUI20->setChecked(!checked);
    }
}

void WelcomeScreenDialog::OnAutoLoadLevelBtnClicked(bool checked)
{
    gSettings.bAutoloadLastLevelAtStartup = checked;
    gSettings.Save();

    SendMetricsEvent("AutoLoadLevelButtonClicked", (checked) ? "1" : "0");
}


void WelcomeScreenDialog::OnShowOnStartupBtnClicked(bool checked)
{
    gSettings.bShowDashboardAtStartup = !checked;
    gSettings.Save();

    if (gSettings.bShowDashboardAtStartup == false)
    {
        QMessageBox msgBox(AzToolsFramework::GetActiveWindow());
        msgBox.setWindowTitle(QObject::tr("Skip the Welcome dialog on startup"));
        msgBox.setText(QObject::tr("You may re-enable the Welcome dialog at any time by going to Edit > Editor Settings > Global Preferences in the menu bar."));
        msgBox.exec();
    }

    SendMetricsEvent("ShowOnStartupButtonClicked", (checked) ? "1" : "0");
}

void WelcomeScreenDialog::OnDocumentationBtnClicked(bool checked)
{
    QString webLink = tr("https://aws.amazon.com/lumberyard/support/");
    QDesktopServices::openUrl(QUrl(webLink));

    SendMetricsEvent("DocumentationButtonClicked");

    linkActivated(webLink);
}

void WelcomeScreenDialog::SyncFail(News::ErrorCode error)
{
    m_articleViewContainer->AddErrorMessage();
    m_waitingOnAsync = false;
}

void WelcomeScreenDialog::SyncSuccess()
{
    m_articleViewContainer->PopulateArticles();
    m_waitingOnAsync = false;
}

void WelcomeScreenDialog::previewAreaScrolled()
{
    //this should only be reported once per session
    if (m_messageScrollReported)
    {
        return;
    }
    m_messageScrollReported = true;

    SendMetricsEvent("WelcomeMessageScrolled");
}

void WelcomeScreenDialog::linkActivated(const QString& link)
{
    SendMetricsEvent("LinkActivated", "", link.toUtf8());
}

void WelcomeScreenDialog::SendMetricsEvent(const char* eventType, const char* checked, const char* link)
{
    LyMetrics_SendEvent(WMSEVENTNAME, {
        { WMSEVENTOPERATION, eventType },
        { "checked", checked },
        { "link", link },
    });
}

#include <WelcomeScreen/WelcomeScreenDialog.moc>

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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include <AudioControlsEditorWindow.h>

#include <AzCore/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <ATLControlsModel.h>
#include <ATLControlsPanel.h>
#include <AudioControlsEditorPlugin.h>
#include <AudioControlsEditorUndo.h>
#include <AudioSystemPanel.h>
#include <CryFile.h>
#include <CryPath.h>
#include <DockTitleBarWidget.h>
#include <IAudioSystem.h>
#include <ImplementationManager.h>
#include <InspectorPanel.h>
#include <ISystem.h>
#include <QAudioControlEditorIcons.h>
#include <Util/PathUtil.h>

#include <QPaintEvent>
#include <QPushButton>
#include <QApplication>
#include <QPainter>
#include <QMessageBox>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    // static
    bool CAudioControlsEditorWindow::m_wasClosed = false;

    //-------------------------------------------------------------------------------------------//
    CAudioControlsEditorWindow::CAudioControlsEditorWindow(QWidget* parent)
        : QMainWindow(parent)
    {
        setupUi(this);

        m_pATLModel = CAudioControlsEditorPlugin::GetATLModel();
        IAudioSystemEditor* pAudioSystemImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
        if (pAudioSystemImpl)
        {
            m_pATLControlsPanel = new CATLControlsPanel(m_pATLModel, CAudioControlsEditorPlugin::GetControlsTree());
            m_pInspectorPanel = new CInspectorPanel(m_pATLModel);
            m_pAudioSystemPanel = new CAudioSystemPanel();

            CDockTitleBarWidget* pTitleBar = new CDockTitleBarWidget(m_pATLControlsDockWidget);
            m_pATLControlsDockWidget->setTitleBarWidget(pTitleBar);

            pTitleBar = new CDockTitleBarWidget(m_pInspectorDockWidget);
            m_pInspectorDockWidget->setTitleBarWidget(pTitleBar);

            pTitleBar = new CDockTitleBarWidget(m_pMiddlewareDockWidget);
            m_pMiddlewareDockWidget->setTitleBarWidget(pTitleBar);

            // Custom title based on Middleware name
            m_pMiddlewareDockWidget->setWindowTitle(QString(pAudioSystemImpl->GetName().c_str()) + " Controls");

            m_pATLControlsDockLayout->addWidget(m_pATLControlsPanel);
            m_pInspectorDockLayout->addWidget(m_pInspectorPanel);
            m_pMiddlewareDockLayout->addWidget(m_pAudioSystemPanel);

            Update();

            connect(m_pATLControlsPanel, SIGNAL(SelectedControlChanged()), this, SLOT(UpdateInspector()));
            connect(m_pATLControlsPanel, SIGNAL(SelectedControlChanged()), this, SLOT(UpdateFilterFromSelection()));
            connect(m_pATLControlsPanel, SIGNAL(ControlTypeFiltered(EACEControlType, bool)), this, SLOT(FilterControlType(EACEControlType, bool)));

            connect(CAudioControlsEditorPlugin::GetImplementationManager(), SIGNAL(ImplementationChanged()), this, SLOT(Update()));

            connect(&m_fileSystemWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(ReloadMiddlewareData()));

            GetIEditor()->RegisterNotifyListener(this);

            // LY-11309: this call to reload middleware data will force refresh of the data when
            // making changes to the middleware project while the AudioControlsEditor window is closed.
            if (m_wasClosed)
            {
                ReloadMiddlewareData();
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    CAudioControlsEditorWindow::~CAudioControlsEditorWindow()
    {
        GetIEditor()->UnregisterNotifyListener(this);
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::StartWatchingFolder(const AZStd::string_view folder)
    {
        m_fileSystemWatcher.addPath(folder.data());

        AZStd::string search;
        AZ::StringFunc::Path::Join(folder.data(), "*.*", search, false, true, false);
        _finddata_t fd;
        ICryPak* pCryPak = gEnv->pCryPak;
        intptr_t handle = pCryPak->FindFirst(search.c_str(), &fd);
        if (handle != -1)
        {
            do
            {
                AZStd::string sName = fd.name;
                if (!sName.empty() && sName[0] != '.')
                {
                    if (fd.attrib & _A_SUBDIR)
                    {
                        AZ::StringFunc::Path::Join(folder.data(), sName.c_str(), sName);
                        StartWatchingFolder(sName);
                    }
                }
            }
            while (pCryPak->FindNext(handle, &fd) >= 0);
            pCryPak->FindClose(handle);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::keyPressEvent(QKeyEvent* pEvent)
    {
        uint16 mod = pEvent->modifiers();
        if (pEvent->key() == Qt::Key_S && pEvent->modifiers() == Qt::ControlModifier)
        {
            Save();
        }
        else if (pEvent->key() == Qt::Key_Z && (pEvent->modifiers() & Qt::ControlModifier))
        {
            if (pEvent->modifiers() & Qt::ShiftModifier)
            {
                GetIEditor()->Redo();
            }
            else
            {
                GetIEditor()->Undo();
            }
        }
        QMainWindow::keyPressEvent(pEvent);
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::closeEvent(QCloseEvent* pEvent)
    {
        if (m_pATLModel && m_pATLModel->IsDirty())
        {
            QMessageBox messageBox(this);
            messageBox.setText(tr("There are unsaved changes."));
            messageBox.setInformativeText(tr("Do you want to save your changes?"));
            messageBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            messageBox.setDefaultButton(QMessageBox::Save);
            messageBox.setWindowTitle("Audio Controls Editor");
            switch (messageBox.exec())
            {
            case QMessageBox::Save:
                QApplication::setOverrideCursor(Qt::WaitCursor);
                Save();
                QApplication::restoreOverrideCursor();
                pEvent->accept();
                break;
            case QMessageBox::Discard:
                pEvent->accept();
                break;
            default:
                pEvent->ignore();
                return;
            }
        }
        else
        {
            pEvent->accept();
        }

        // If the close event was accepted, set this flag so next time the window opens it will refresh the middleware data.
        m_wasClosed = true;
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::Reload()
    {
        bool bReload = true;
        if (m_pATLModel && m_pATLModel->IsDirty())
        {
            QMessageBox messageBox(this);
            messageBox.setText(tr("If you reload you will lose all your unsaved changes."));
            messageBox.setInformativeText(tr("Are you sure you want to reload?"));
            messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            messageBox.setDefaultButton(QMessageBox::No);
            messageBox.setWindowTitle("Audio Controls Editor");
            bReload = (messageBox.exec() == QMessageBox::Yes);
        }

        if (bReload)
        {
            CAudioControlsEditorPlugin::ReloadModels();
            Update();
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::Update()
    {
        if (!m_pATLControlsPanel)
        {
            return;
        }

        m_pATLControlsPanel->Reload();
        m_pAudioSystemPanel->Reload();
        UpdateInspector();
        IAudioSystemEditor* pAudioSystemImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
        if (pAudioSystemImpl)
        {
            StartWatchingFolder(pAudioSystemImpl->GetDataPath());
            m_pMiddlewareDockWidget->setWindowTitle(QString(pAudioSystemImpl->GetName().c_str()) + " Controls");
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::Save()
    {
        bool bPreloadsChanged = m_pATLModel->IsTypeDirty(eACET_PRELOAD);
        CAudioControlsEditorPlugin::SaveModels();
        UpdateAudioSystemData();

        // When preloads have been modified, ask the user to refresh the audio system (reload banks & controls)
        if (bPreloadsChanged)
        {
            QMessageBox messageBox(this);
            messageBox.setText(tr("Preload requests have been modified.\n\n"
                "For the new data to be loaded the audio system needs to be refreshed, this will stop all currently playing audio.\n"
                "Do you want to do this now?\n\n"
                "You can always refresh manually at a later time through the Game->Audio menu."));
            messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            messageBox.setDefaultButton(QMessageBox::Yes);
            messageBox.setWindowTitle("Audio Controls Editor");
            if (messageBox.exec() == QMessageBox::Yes)
            {
                AZStd::string levelName;
                AzToolsFramework::EditorRequestBus::BroadcastResult(levelName, &AzToolsFramework::EditorRequests::GetLevelName);
                if (AZ::StringFunc::Equal(levelName.c_str(), "Untitled"))
                {
                    // Reset to empty string to indicate that no level is loaded
                    levelName.clear();
                }

                Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::RefreshAudioSystem, levelName.c_str());
            }
        }
        m_pATLModel->ClearDirtyFlags();
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::UpdateInspector()
    {
        m_pInspectorPanel->SetSelectedControls(m_pATLControlsPanel->GetSelectedControls());
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::UpdateFilterFromSelection()
    {
        bool bAllSameType = true;
        EACEControlType selectedType = eACET_NUM_TYPES;
        ControlList ids = m_pATLControlsPanel->GetSelectedControls();
        size_t size = ids.size();
        for (size_t i = 0; i < size; ++i)
        {
            CATLControl* pControl = m_pATLModel->GetControlByID(ids[i]);
            if (pControl)
            {
                if (selectedType == eACET_NUM_TYPES)
                {
                    selectedType = pControl->GetType();
                }
                else if (selectedType != pControl->GetType())
                {
                    bAllSameType = false;
                }
            }
        }

        bool bSelectedFolder = (selectedType == eACET_NUM_TYPES);
        for (int i = 0; i < eACET_NUM_TYPES; ++i)
        {
            EACEControlType type = (EACEControlType)i;
            bool bAllowed = bSelectedFolder || (bAllSameType && selectedType == type);
            m_pAudioSystemPanel->SetAllowedControls((EACEControlType)i, bAllowed);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::UpdateAudioSystemData()
    {
        Audio::SAudioRequest oConfigDataRequest;
        oConfigDataRequest.nFlags = Audio::eARF_PRIORITY_HIGH;

        // clear the AudioSystem controls data (all)
        Audio::SAudioManagerRequestData<Audio::eAMRT_CLEAR_CONTROLS_DATA> oClearRequestData(Audio::eADS_ALL);
        oConfigDataRequest.pData = &oClearRequestData;
        Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, oConfigDataRequest);

        // parse the AudioSystem global controls data
        const char* controlsPath = nullptr;
        Audio::AudioSystemRequestBus::BroadcastResult(controlsPath, &Audio::AudioSystemRequestBus::Events::GetControlsPath);
        Audio::SAudioManagerRequestData<Audio::eAMRT_PARSE_CONTROLS_DATA> oParseGlobalRequestData(controlsPath, Audio::eADS_GLOBAL);
        oConfigDataRequest.pData = &oParseGlobalRequestData;
        Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, oConfigDataRequest);

        // parse the AudioSystem level-specific controls data
        AZStd::string levelName;
        AzToolsFramework::EditorRequestBus::BroadcastResult(levelName, &AzToolsFramework::EditorRequests::GetLevelName);
        if (!levelName.empty() && !AZ::StringFunc::Equal(levelName.c_str(), "Untitled"))
        {
            AZStd::string levelControlsPath;
            AZ::StringFunc::Path::Join(controlsPath, "levels", levelControlsPath);
            AZ::StringFunc::Path::Join(levelControlsPath.c_str(), levelName.c_str(), levelControlsPath);

            Audio::SAudioManagerRequestData<Audio::eAMRT_PARSE_CONTROLS_DATA> oParseLevelRequestData(levelControlsPath.c_str(), Audio::eADS_LEVEL_SPECIFIC);
            oConfigDataRequest.pData = &oParseLevelRequestData;
            Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequest, oConfigDataRequest);
        }

        // inform the middleware specific plugin that the data has been saved
        // to disk (in case it needs to update something)
        CAudioControlsEditorPlugin::GetAudioSystemEditorImpl()->DataSaved();
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::OnEditorNotifyEvent(EEditorNotifyEvent event)
    {
        if (event == eNotify_OnEndSceneSave)
        {
            CAudioControlsEditorPlugin::ReloadScopes();
            m_pInspectorPanel->Reload();
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::FilterControlType(EACEControlType type, bool bShow)
    {
        m_pAudioSystemPanel->SetAllowedControls(type, bShow);
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::ReloadMiddlewareData()
    {
        IAudioSystemEditor* pAudioSystemImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
        if (pAudioSystemImpl)
        {
            pAudioSystemImpl->Reload();
        }
        m_pAudioSystemPanel->Reload();
        m_pInspectorPanel->Reload();
    }
} // namespace AudioControls

#include <Source/Editor/AudioControlsEditorWindow.moc>

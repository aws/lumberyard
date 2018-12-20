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

#include "EMStudioManager.h"
#include "LayoutManager.h"
#include "MainWindow.h"
#include <EMotionStudio/EMStudioSDK/Source/DockWidgetPlugin.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MetricsEventSender.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/Array.h>
#include <MCore/Source/MemoryFile.h>

#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QRegExpValidator>

namespace EMStudio
{
    LayoutManagerSaveAsWindow::LayoutManagerSaveAsWindow(const char* defaultName, QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle("Save Layout As");
        setMinimumWidth(300);

        QVBoxLayout* layout = new QVBoxLayout();
        layout->addWidget(new QLabel("Please enter the layout name:"));

        mLineEdit = new QLineEdit(defaultName);
        connect(mLineEdit, SIGNAL(textChanged(QString)), this, SLOT(NameEditChanged(QString)));
        mLineEdit->selectAll();
        mLineEdit->setValidator(new QRegExpValidator(QRegExp("^[a-zA-Z0-9_\\-./]*$"), mLineEdit));

        QHBoxLayout* buttonLayout   = new QHBoxLayout();
        mOKButton                   = new QPushButton("OK");
        QPushButton* cancelButton   = new QPushButton("Cancel");
        buttonLayout->addWidget(mOKButton);
        buttonLayout->addWidget(cancelButton);

        if (mLineEdit->text().isEmpty())
        {
            mOKButton->setEnabled(false);
            GetManager()->SetWidgetAsInvalidInput(mLineEdit);
        }

        layout->addWidget(mLineEdit);
        layout->addLayout(buttonLayout);
        setLayout(layout);

        connect(mOKButton, SIGNAL(clicked()), this, SLOT(accept()));
        connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    }


    void LayoutManagerSaveAsWindow::NameEditChanged(const QString& text)
    {
        if (text.isEmpty())
        {
            mOKButton->setEnabled(false);
            GetManager()->SetWidgetAsInvalidInput(mLineEdit);
        }
        else
        {
            mOKButton->setEnabled(true);
            mLineEdit->setStyleSheet("");
        }
    }


    LayoutManager::LayoutManager()
    {
        mIsSwitching = false;
    }


    LayoutManager::~LayoutManager()
    {
    }


    void LayoutManager::SaveLayoutAs()
    {
        LayoutManagerSaveAsWindow saveAsWindow(GetMainWindow()->GetCurrentLayoutName(), GetMainWindow());
        if (saveAsWindow.exec() == QDialog::Rejected)
        {
            return;
        }

        const AZStd::string filename = AZStd::string::format("%sLayouts/%s.layout", MysticQt::GetDataDir().c_str(), saveAsWindow.GetName().toUtf8().data());

        // If the file already exists, ask to overwrite or not.
        if (QFile::exists(filename.c_str()))
        {
            QMessageBox msgBox(QMessageBox::Warning, "Overwrite Existing Layout?", "A layout with the same name already exists.<br>Would you like to overwrite it?<br><br>Click <b>yes</b> to <b>overwrite</b> the existing layout.<br>Click <b>no</b> to <b>cancel saving</b> this layout.", QMessageBox::Yes | QMessageBox::No, (QWidget*)EMStudio::GetMainWindow());
            msgBox.setTextFormat(Qt::RichText);
            if (msgBox.exec() != QMessageBox::Yes)
            {
                return;
            }
        }

        // Try to save the layout to a file.
        if (SaveLayout(filename.c_str()))
        {
            GetMainWindow()->GetOptions().SetApplicationMode(saveAsWindow.GetName().toUtf8().data());
            GetMainWindow()->SavePreferences();
            GetMainWindow()->UpdateLayoutsMenu();

            MCore::LogInfo("Successfully saved layout to file '%s'", filename.c_str());
            GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_SUCCESS, "Layout <font color=green>successfully</font> saved");
        }
        else
        {
            MCore::LogError("Failed to save layout to file '%s'", filename.c_str());

            const AZStd::string errorMessage = AZStd::string::format("Failed to save layout to file '<b>%s</b>', is it maybe read only? Maybe it is not checked out?", filename.c_str());
            GetCommandManager()->AddError(errorMessage.c_str());
            GetCommandManager()->ShowErrorReport();

            GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_ERROR, "Layout <font color=red>failed</font> to save");
        }
    }


    bool LayoutManager::SaveLayout(const char* filename)
    {
        QFile file(filename);
        if (file.open(QIODevice::WriteOnly) == false)
        {
            MCore::LogWarning("Failed to open layout file '%s' for writing, file might be in use?", filename);
            return false;
        }

        LayoutHeader header;
        header.mFileTypeCode[0] = 'E';
        header.mFileTypeCode[1] = 'M';
        header.mFileTypeCode[2] = 'S';
        header.mFileTypeCode[3] = 'L';
        header.mFileTypeCode[4] = 'A';
        header.mFileTypeCode[5] = 'Y';
        header.mFileTypeCode[6] = 'O';
        header.mFileTypeCode[7] = 'U';
        header.mFileTypeCode[8] = 'T';
        header.mEMFXVersionHigh = EMotionFX::GetEMotionFX().GetHighVersion();
        header.mEMFXVersionLow  = EMotionFX::GetEMotionFX().GetLowVersion();

        azstrcpy(header.mEMFXCompileDate, 64, EMotionFX::GetEMotionFX().GetCompilationDate());
        azstrcpy(header.mCompileDate, 64, MCORE_DATE);
        azstrcpy(header.mDescription, 256, "");

        header.mLayoutVersionHigh   = 0;
        header.mLayoutVersionLow    = 1;
        header.mNumPlugins = GetPluginManager()->GetNumActivePlugins();
        if (file.write((char*)&header, sizeof(LayoutHeader)) == -1)
        {
            MCore::LogWarning("Failed to write layout header to layout file '%s'", filename);
            return false;
        }

        // For each plugin (window) save the object name.
        for (uint32 i = 0; i < header.mNumPlugins; ++i)
        {
            EMStudioPlugin* plugin = GetPluginManager()->GetActivePlugin(i);

            // Write the plugin data into memory.
            MCore::MemoryFile memFile;
            plugin->WriteLayoutData(memFile);

            // Save the plugin header.
            LayoutPluginHeader pluginHeader;
            pluginHeader.mDataSize      = static_cast<uint32>(memFile.GetFileSize());
            pluginHeader.mDataVersion   = plugin->GetLayoutDataVersion();

            azstrcpy(pluginHeader.mObjectName, 128, FromQtString(plugin->GetObjectName()).c_str());
            azstrcpy(pluginHeader.mPluginName, 128, plugin->GetName());

            file.write((char*)&pluginHeader, sizeof(LayoutPluginHeader));

            //MCore::LogDetailedInfo("pluginHeader.mDataSize = %d bytes (version=%d) (name=%s)", pluginHeader.mDataSize, pluginHeader.mDataVersion, pluginHeader.mPluginName);

            if (memFile.GetMemoryStart())
            {
                if (file.write((char*)memFile.GetMemoryStart(), memFile.GetFileSize()) == -1)
                {
                    MCore::LogWarning("Failed to write plugin data for plugin '%s' to layout file '%s'", plugin->GetName(), filename);
                    return false;
                }
            }
        }

        const QByteArray windowLayout = GetMainWindow()->saveState();

        // Write the state data length.
        const uint32 stateLength = windowLayout.size();
        if (file.write((char*)&stateLength, sizeof(uint32)) == -1)
        {
            MCore::LogWarning("Failed to write main window state length to layout file '%s'", filename);
            return false;
        }

        // Write the state data.
        if (file.write(windowLayout) == -1)
        {
            MCore::LogWarning("Failed to write main window state data to layout file '%s'", filename);
            return false;
        }

        // Make sure the file is saved.
        file.flush();
        return true;
    }


    bool LayoutManager::LoadLayout(const char* filename)
    {
        // If we are already switching, skip directly.
        if (mIsSwitching)
        {
            return true;
        }

        mIsSwitching = true;

        QFile file(filename);
        if (file.open(QIODevice::ReadOnly) == false)
        {
            mIsSwitching = false;
            return false;
        }

        // Build an array of active plugins.
        PluginManager* pluginManager = GetPluginManager();
        PluginManager::PluginVector activePlugins = pluginManager->GetActivePlugins();

        // Read the layout file header.
        LayoutHeader header;
        if (file.read((char*)&header, sizeof(LayoutHeader)) == -1)
        {
            MCore::LogWarning("Error reading header from layout file '%s'", filename);
            mIsSwitching = false;
            return false;
        }

        // Check if this is a valid layout file.
        if (header.mFileTypeCode[0] != 'E' || header.mFileTypeCode[1] != 'M' || header.mFileTypeCode[2] != 'S' ||
            header.mFileTypeCode[3] != 'L' || header.mFileTypeCode[4] != 'A' || header.mFileTypeCode[5] != 'Y' || header.mFileTypeCode[6] != 'O' || header.mFileTypeCode[7] != 'U' || header.mFileTypeCode[8] != 'T')
        {
            MCore::LogWarning("Failed to load file '%s' as it is not a valid EMotion Studio layout file.", filename);
            mIsSwitching = false;
            return false;
        }

        //MCore::LogDetailedInfo("EMotion FX version          = v%d.%d", header.mEMFXVersionHigh, header.mEMFXVersionLow / 100);
        //MCore::LogDetailedInfo("EMotion FX compile date     = %s",     header.mEMFXCompileDate);
        //MCore::LogDetailedInfo("EMStudio compile date = %s",     header.mCompileDate);
        //MCore::LogDetailedInfo("Layout description    = %s",     header.mDescription);
        //MCore::LogDetailedInfo("Layout version        = v%d.%d", header.mLayoutVersionHigh, header.mLayoutVersionLow);
        //MCore::LogDetailedInfo("Num active plugins    = %d",   header.mNumPlugins);

        // Iterate through the plugins and try to reuse them.
        for (uint32 i = 0; i < header.mNumPlugins; ++i)
        {
            // load the plugin header
            LayoutPluginHeader pluginHeader;
            if (file.read((char*)&pluginHeader, sizeof(LayoutPluginHeader)) == -1)
            {
                MCore::LogWarning("Error reading plugin header from layout file '%s'", filename);
                mIsSwitching = false;
                return false;
            }

            //MCore::LogDetailedInfo("Loading plugin settings for plugin '%s'...", pluginHeader.mPluginName);
            //MCore::LogDetailedInfo("   + Data size    = %d bytes", pluginHeader.mDataSize);
            //MCore::LogDetailedInfo("   + Data version = %d", pluginHeader.mDataVersion);

            EMStudioPlugin* plugin = nullptr;

            // Check if we already have a window using a similar plugin.
            // If so, we can reuse this window with already initialized plugin
            // all we need to do is then change the object name used when restoring the state.
            {
                PluginManager::PluginVector::const_iterator itActivePlugin = activePlugins.begin();
                while (itActivePlugin != activePlugins.end())
                {
                    // Is the plugin name the same as we need to create?
                    if (AzFramework::StringFunc::Equal((*itActivePlugin)->GetName(), pluginHeader.mPluginName))
                    {
                        plugin = *itActivePlugin;
                        plugin->SetObjectName(pluginHeader.mObjectName);
                        if (plugin->GetPluginType() == EMStudioPlugin::PLUGINTYPE_DOCKWIDGET)
                        {
                            DockWidgetPlugin* dockPlugin = static_cast<DockWidgetPlugin*>(plugin);
                            // Dock widgets, when maximized, sometimes fail to
                            // get a mouse release event when they are moved.
                            // Calling setFloating(false) ensures they are not
                            // in the middle of a drag operation while their
                            // geometry is being restored from the saved
                            // layout.
                            dockPlugin->GetDockWidget()->setFloating(false);
                        }
                        activePlugins.erase(itActivePlugin);
                        break;
                    }
                    ++itActivePlugin;
                }
            }

            // Try to create the plugin of this type.
            if (!plugin)
            {
                plugin = GetPluginManager()->CreateWindowOfType(pluginHeader.mPluginName, pluginHeader.mObjectName);

                if (!plugin)
                {
                    MCore::LogError("Failed to create plugin window of type '%s', with data size %d bytes", pluginHeader.mPluginName, pluginHeader.mDataSize);

                    // Skip the data.
                    file.seek(file.pos() + pluginHeader.mDataSize);

                    continue;
                }
            }

            if (plugin->ReadLayoutSettings(file, pluginHeader.mDataSize, pluginHeader.mDataVersion) == false)
            {
                MCore::LogWarning("Error reading plugin settings from layout file '%s'", filename);
                mIsSwitching = false;
                return false;
            }
        }

        // Delete all active plugins that haven't been reused.
        for (EMStudioPlugin* remainingActivePlugin : activePlugins)
        {
            pluginManager->RemoveActivePlugin(remainingActivePlugin);
        }

        // Read the main window state data length.
        uint32 stateLength;
        if (file.read((char*)&stateLength, sizeof(uint32)) == -1)
        {
            MCore::LogWarning("Error reading main window state length from layout file '%s'", filename);
            mIsSwitching = false;
            return false;
        }

        // Read the state data.
        const QByteArray layout = file.read(stateLength);
        if (layout.size() == 0)
        {
            MCore::LogWarning("Error reading main window state data from layout file '%s'", filename);
            mIsSwitching = false;
            return false;
        }

        // Restore the state data.
        GetMainWindow()->restoreState(layout);
        GetMainWindow()->UpdateCreateWindowMenu(); // update Window->Create menu

        // Trigger the OnAfterLoadLayout callbacks.
        const uint32 numActivePlugins = pluginManager->GetNumActivePlugins();
        for (uint32 p = 0; p < numActivePlugins; ++p)
        {
            GetPluginManager()->GetActivePlugin(p)->OnAfterLoadLayout();
        }

        // Send LyMetrics event.
        //AZStd::string layoutName;
        //AzFramework::StringFunc::Path::GetFileName(filename, layoutName);
        //MetricsEventSender::SendSwitchLayoutEvent(layoutName.c_str());

        mIsSwitching = false;
        return true;
    }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/LayoutManager.moc>

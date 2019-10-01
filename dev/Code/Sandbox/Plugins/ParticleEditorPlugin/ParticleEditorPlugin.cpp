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
#include "stdafx.h"
#include "ParticleEditorPlugin.h"
#include "platform_impl.h"
#include "Include/ICommandManager.h"
#include <Include/IEditorParticleManager.h>
#include "Include/IEditorClassFactory.h"
#include <Include/IDataBaseLibrary.h>

#include <AzCore/std/string/wildcard.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/XML/rapidxml.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>

#include <LyViewPaneNames.h>
#include <QT/MainWindow.h>

#include <EditorUI_QTDLLBus.h>



static bool const c_EnableParticleEditorMenuEntry = true;
const char* CParticleEditorPlugin::m_RegisteredQtViewPaneName = "Particle Editor";

namespace PluginInfo
{
    const char* kName = "Particle Editor plug-in";
    const char* kGUID = "{98C1DC36-5D0E-4CF6-90CE-AFA1107CE80F}";
    const int kVersion = 1;
}

CParticleEditorPlugin::CParticleEditorPlugin(IEditor* editor)
{
    if (c_EnableParticleEditorMenuEntry)
    {
        AzToolsFramework::ViewPaneOptions options;
        options.canHaveMultipleInstances = true;
        options.isPreview = true;
        /*
         * We disabled the deletion on close for following reason:
         * 1. Optimize the editor work-flow. There is no need to create a brand new editor
         * every time user open it.
         * 2. Prevent the crash on race condition. If we delete the editor on close, while other
         * operations are not finished, it could crash the editor.
        */
        options.isDeletable = false;
        options.sendViewPaneNameBackToAmazonAnalyticsServers = true;

        AzToolsFramework::RegisterViewPane<CMainWindow>(m_RegisteredQtViewPaneName, LyViewPane::CategoryTools, options);
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
    }
}

void CParticleEditorPlugin::Release()
{
    AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
    if (c_EnableParticleEditorMenuEntry)
    {
        AzToolsFramework::UnregisterViewPane(m_RegisteredQtViewPaneName);
    }
    delete this;
}

void CParticleEditorPlugin::ShowAbout()
{
}

const char* CParticleEditorPlugin::GetPluginGUID()
{
    return PluginInfo::kGUID;
}

DWORD CParticleEditorPlugin::GetPluginVersion()
{
    return PluginInfo::kVersion;
}

const char* CParticleEditorPlugin::GetPluginName()
{
    return PluginInfo::kName;
}

bool CParticleEditorPlugin::CanExitNow()
{
    return true;
}

void CParticleEditorPlugin::OnEditorNotify(EEditorNotifyEvent aEventId)
{
}

bool CParticleEditorPlugin::IsParticleXMLFile(const char* fileSourceFilePath) const
{
    if ((!fileSourceFilePath) || (!AZStd::wildcard_match("*.xml", fileSourceFilePath)))
    {
        return false;
    }

    using namespace AZ::IO;

    bool isParticleLibrary = false;

    // we are forced to read the asset to discover its type since "xml" was the chosen extension.
    SystemFile::SizeType fileSize = SystemFile::Length(fileSourceFilePath);
    if (fileSize > 0)
    {
        AZStd::vector<char> buffer(fileSize + 1);
        buffer[fileSize] = 0;
        if (!AZ::IO::SystemFile::Read(fileSourceFilePath, buffer.data(), fileSize))
        {
            return false;
        }

        AZ::rapidxml::xml_document<char>* xmlDoc = azcreate(AZ::rapidxml::xml_document<char>, (), AZ::SystemAllocator, "ParticleEditorPlugin Temp XML Reader");
        if (xmlDoc->parse<AZ::rapidxml::parse_no_data_nodes>(buffer.data()))
        {
            AZ::rapidxml::xml_node<char>* xmlRootNode = xmlDoc->first_node();
            if (xmlRootNode)
            {
                if (azstricmp(xmlRootNode->name(), "ParticleLibrary") == 0)
                {
                    isParticleLibrary = true;
                }
            }
        }

        azdestroy(xmlDoc, AZ::SystemAllocator, AZ::rapidxml::xml_document<char>);
    }

    return isParticleLibrary;
}

void CParticleEditorPlugin::AddSourceFileOpeners(const char* fullSourceFileName, const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers)
{
    // this is on the plugin class becuase it needs to be on an object that exists permanently,
    // even when the main window is not open.
    using namespace AzToolsFramework;
    using namespace AzToolsFramework::AssetBrowser;

    if (IsParticleXMLFile(fullSourceFileName))
    {
        auto onChooseMyOpener = [](const char* fullSourceFileNameInCallback, const AZ::Uuid& sourceUUIDInCallback)
        {
            // assuming this is an async call, we don't use the outer sourceUUID, we use the callback one.
            if (const SourceAssetBrowserEntry* fullDetails = SourceAssetBrowserEntry::GetSourceByUuid(sourceUUIDInCallback))
            {
                // find out the path that the asset in the library uses.  It has to be in /libs/particles.
                AZStd::string fullPathName = fullDetails->GetFullPath();
                AZStd::to_lower(fullPathName.begin(), fullPathName.end());
                size_t startpos = fullPathName.find("/libs/particles/");
                if (startpos == AZStd::string::npos)
                {
                    AZ_Error("Particle Editor", false, "Cannot load particle library %s - it needs to be in a /libs/particles/ folder.", fullDetails->GetFullPath().c_str());
                    return; // not in the appropriate folder.
                }
                // the library system expects the name of the
                fullPathName = fullDetails->GetFullPath().substr(startpos + strlen("/libs/particles/"));

                IDataBaseLibrary* loaded = GetIEditor()->GetParticleManager()->LoadLibrary(QString::fromUtf8(fullPathName.c_str()), false);
                if (loaded)
                {
                    OpenViewPane(m_RegisteredQtViewPaneName);

                    using namespace EditorUIPlugin;
                    AZStd::string libraryName = loaded->GetName().toUtf8().constData();
                    EditorLibraryUndoRequests::Bus::Broadcast(&EditorLibraryUndoRequests::AddLibraryCreateUndo, libraryName);
                    // now select the first item in the library, so that the user gets to immediately see the items in the viewport.
                    if (loaded->GetItemCount() > 0)
                    {
                        GetIEditor()->GetParticleManager()->SetSelectedItem(loaded->GetItem(0));
                    }

                    // make sure the selection is in sync:
                    QString loadedFileName = loaded->GetFilename();
                    LibraryChangeEvents::Bus::Broadcast(&LibraryChangeEvents::LibraryChangedInManager, loadedFileName.toUtf8().data());
                }
            }
        };
        openers.push_back({ "Lumberyard_ParticleEditor", "Open in Particle Editor", QIcon(), onChooseMyOpener });
    }
}

AZ::s32 CParticleEditorPlugin::GetPriority() const
{
    return 1; // override default XML behavior.
}


AzToolsFramework::AssetBrowser::SourceFileDetails CParticleEditorPlugin::GetSourceFileDetails(const char* sourceFileName)
{
    if (IsParticleXMLFile(sourceFileName))
    {
        return AzToolsFramework::AssetBrowser::SourceFileDetails("Editor/Icons/AssetBrowser/Particle_16.png");
    }
    return AzToolsFramework::AssetBrowser::SourceFileDetails();
}

#include "CloudGemDynamicContent_precompiled.h"
#include <DynamicContentEditorPlugin.h>
#include "IGemManager.h"
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include "CryExtension/ICryFactoryRegistry.h"
#include "QDynamicContentEditorMainWindow.h"
#include "QtViewPaneManager.h"

static const char* ViewPaneName = "Dynamic Content Manager";

DynamicContentEditorPlugin::DynamicContentEditorPlugin(IEditor* editor)
    : m_registered(false)
{
    AzToolsFramework::ViewPaneOptions opt;
    opt.isDeletable = true;
    opt.canHaveMultipleInstances = false;
    AzToolsFramework::RegisterViewPane<DynamicContent::QDynamicContentEditorMainWindow>(ViewPaneName, "Cloud Canvas", opt);
    m_registered = true;
}

void DynamicContentEditorPlugin::Release()
{
    if (m_registered)
    {
        AzToolsFramework::UnregisterViewPane(ViewPaneName);
    }
}

void DynamicContentEditorPlugin::OnEditorNotify(EEditorNotifyEvent aEventId)
{
    switch (aEventId)
    {
    case eNotify_OnInit:
    {
    }
    break;
    case eNotify_OnQuit:
    {

    }
    break;
    default:
    {

    }
    }

}

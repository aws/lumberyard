#include "stdafx.h"
#include <DynamicContentEditorPlugin.h>
#include "IGemManager.h"
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include "CryExtension/ICryFactoryRegistry.h"
#include "QDynamicContentEditorMainWindow.h"
#include "QtViewPaneManager.h"

DynamicContentEditorPlugin::DynamicContentEditorPlugin(IEditor* editor)
    : m_registered(false)
{
    QtViewOptions opt;
    opt.isDeletable = true;
    opt.canHaveMultipleInstances = false;
    m_registered = RegisterQtViewPane<DynamicContent::QDynamicContentEditorMainWindow>(editor, "Dynamic Content Manager", "Cloud Canvas", opt);
}

void DynamicContentEditorPlugin::Release()
{
    if (m_registered)
    {
        UnregisterQtViewPane<DynamicContent::QDynamicContentEditorMainWindow>();
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

#pragma once

#include <IEditor.h>
#include <Include/IPlugin.h>
#include <AzCore/std/string/string.h>


//------------------------------------------------------------------
class DynamicContentEditorPlugin : public IPlugin
{
public:
    DynamicContentEditorPlugin(IEditor* editor);

    void Release() override;
    void ShowAbout() override {}
    const char* GetPluginGUID() override { return "{7E7BCAC7-B72B-45EA-9C98-B48F95D84AA7}"; }
    DWORD GetPluginVersion() override { return 1; }
    const char* GetPluginName() override { return "DynamicContentEditor"; }
    bool CanExitNow() override { return true; }
    void OnEditorNotify(EEditorNotifyEvent aEventId) override;

 private:
    bool m_registered;
};

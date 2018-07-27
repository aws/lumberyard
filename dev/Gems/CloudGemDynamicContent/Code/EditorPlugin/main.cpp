/** @file dllmain.cpp
	@brief Dll entry point
	@author Josh Coyne - Allegorithmic (josh.coyne@allegorithmic.com)
	@date 09-14-2015
	@copyright Allegorithmic. All rights reserved.
*/
#include "CloudGemDynamicContent_precompiled.h"

#include <platform.h>
#include <platform_impl.h>
#include <IEditor.h>
#include <Include/IPlugin.h>
#include <Include/IEditorClassFactory.h>
#include "DynamicContentEditorPlugin.h"

//------------------------------------------------------------------
PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
    ISystem *pSystem = pInitParam->pIEditorInterface->GetSystem();
    ModuleInitISystem(pSystem, "DynamicContentEditorPlugin");
    return new DynamicContentEditorPlugin(GetIEditor());
}


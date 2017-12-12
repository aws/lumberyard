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

// Description : Script Bind for CActionMapManager


#ifndef CRYINCLUDE_CRYACTION_SCRIPTBIND_ACTIONMAPMANAGER_H
#define CRYINCLUDE_CRYACTION_SCRIPTBIND_ACTIONMAPMANAGER_H
#pragma once


#include <IScriptSystem.h>
#include <ScriptHelpers.h>


class CActionMapManager;


class CScriptBind_ActionMapManager
    : public CScriptableBase
{
public:
    CScriptBind_ActionMapManager(ISystem* pSystem, CActionMapManager* pActionMapManager);
    virtual ~CScriptBind_ActionMapManager();

    void Release() { delete this; };

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

    //! <code>ActionMapManager.EnableActionFilter( name, enable )</code>
    //!     <param name="name">Filter name.</param>
    //!     <param name="enable">True to enable the filter, false otherwise.</param>
    //! <description>Enables a filter for the actions.</description>
    int EnableActionFilter(IFunctionHandler* pH, const char* name, bool enable);

    //! <code>ActionMapManager.EnableActionMap( name, enable )</code>
    //!     <param name="name">Action map name.</param>
    //!     <param name="enable">True to enable the filter, false otherwise.</param>
    //! <description>Enables an action map.</description>
    int EnableActionMap(IFunctionHandler* pH, const char* name, bool enable);

    //! <code>ActionMapManager.LoadFromXML( name )</code>
    //!     <param name="name">XML file name.</param>
    //! <description>Loads information from an XML file.</description>
    int LoadFromXML(IFunctionHandler* pH, const char* name);

private:
    void RegisterGlobals();
    void RegisterMethods();

    ISystem* m_pSystem;
    CActionMapManager* m_pManager;
};


#endif // CRYINCLUDE_CRYACTION_SCRIPTBIND_ACTIONMAPMANAGER_H

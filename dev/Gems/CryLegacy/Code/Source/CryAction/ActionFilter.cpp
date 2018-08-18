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

#include "CryLegacy_precompiled.h"
#include "ActionFilter.h"
#include "ActionMapManager.h"


//------------------------------------------------------------------------
CActionFilter::CActionFilter(CActionMapManager* pActionMapManager, IInput* pInput, const char* name, EActionFilterType type)
    : m_pActionMapManager(pActionMapManager)
    , m_pInput(pInput)
    , m_enabled(false)
    , m_type(type)
    , m_name(name)
{
}

//------------------------------------------------------------------------
CActionFilter::~CActionFilter()
{
}

//------------------------------------------------------------------------
void CActionFilter::Filter(const ActionId& action)
{
    m_filterActions.insert(action);
}

//------------------------------------------------------------------------
void CActionFilter::Enable(bool enable)
{
    m_enabled = enable;

    // auto-release all actions which
    if (m_enabled && m_type == eAFT_ActionFail)
    {
        TFilterActions::const_iterator it = m_filterActions.begin();
        TFilterActions::const_iterator end = m_filterActions.end();
        for (; it != end; ++it)
        {
            m_pActionMapManager->ReleaseActionIfActive(*it);
        }
    }
    else if (!m_enabled)
    {
        // Reset analog key states
        if (!gEnv->IsDedicated())
        {
            CRY_ASSERT(m_pInput);   // don't assert on dedicated servers OR clients
        }
        if (m_pInput)
        {
            m_pInput->ClearAnalogKeyState();
        }

        if (m_pActionMapManager)
        {
            m_pActionMapManager->RemoveAllRefireData();
        }
    }
};
//------------------------------------------------------------------------
bool CActionFilter::ActionFiltered(const ActionId& action)
{
    // never filter anything out when disabled
    if (!m_enabled)
    {
        return false;
    }

    TFilterActions::const_iterator it = m_filterActions.find(action);

    if ((m_type == eAFT_ActionPass) == (it == m_filterActions.end()))
    {
        static ICVar* pDebugVar = gEnv->pConsole->GetCVar("i_debug");
        if (pDebugVar && pDebugVar->GetIVal() != 0)
        {
            CryLog("Action %s is filtered by %s", action.c_str(), m_name.c_str());
        }
        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------
bool CActionFilter::SerializeXML(const XmlNodeRef& root, bool bLoading)
{
    if (bLoading)
    {
        // loading
        const XmlNodeRef& child = root;
        if (strcmp(child->getTag(), "actionfilter") != 0)
        {
            return false;
        }

        EActionFilterType actionFilterType = eAFT_ActionFail;
        if (!strcmp(child->getAttr("type"), "actionFail"))
        {
            actionFilterType = eAFT_ActionFail;
        }
        if (!strcmp(child->getAttr("type"), "actionPass"))
        {
            actionFilterType = eAFT_ActionPass;
        }

        m_type = actionFilterType;

        int nFilters = child->getChildCount();
        for (int f = 0; f < nFilters; ++f)
        {
            XmlNodeRef filter = child->getChild(f);
            Filter(CCryName(filter->getAttr("name")));
        }
    }
    else
    {
        // saving
        XmlNodeRef filterRoot = root->newChild("actionfilter");
        filterRoot->setAttr("name", m_name);
        filterRoot->setAttr("type", m_type == eAFT_ActionPass ? "actionPass" : "actionFail");
        filterRoot->setAttr("version", m_pActionMapManager->GetVersion());
        TFilterActions::const_iterator iter = m_filterActions.begin();
        while (iter != m_filterActions.end())
        {
            XmlNodeRef filterChild = filterRoot->newChild("filter");
            filterChild->setAttr("name", iter->c_str());
            ++iter;
        }
    }
    return true;
}

void CActionFilter::GetMemoryUsage(ICrySizer* s) const
{
    s->AddObject(m_filterActions);
}



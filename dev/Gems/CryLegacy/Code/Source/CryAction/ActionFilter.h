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

#ifndef CRYINCLUDE_CRYACTION_ACTIONFILTER_H
#define CRYINCLUDE_CRYACTION_ACTIONFILTER_H
#pragma once


#include "IActionMapManager.h"


typedef std::set<ActionId> TFilterActions;


class CActionMapManager;

class CActionFilter
    : public IActionFilter
{
public:
    CActionFilter(CActionMapManager* pActionMapManager, IInput* pInput, const char* name, EActionFilterType type = eAFT_ActionFail);
    virtual ~CActionFilter();

    // IActionFilter
    virtual void Release() { delete this; };
    virtual void Filter(const ActionId& action);
    virtual bool SerializeXML(const XmlNodeRef& root, bool bLoading);
    virtual const char* GetName() { return m_name.c_str(); }
    virtual void Enable(bool enable);
    virtual bool Enabled() { return m_enabled; };
    // ~IActionFilter

    bool ActionFiltered(const ActionId& action);

    virtual void GetMemoryUsage(ICrySizer* pSizer) const;

private:
    bool                            m_enabled;
    CActionMapManager* m_pActionMapManager;
    IInput*             m_pInput;
    TFilterActions      m_filterActions;
    EActionFilterType m_type;
    string            m_name;
};


#endif // CRYINCLUDE_CRYACTION_ACTIONFILTER_H

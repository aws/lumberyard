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

#ifndef CRYINCLUDE_CRYACTION_GAMETOKENS_GAMETOKEN_H
#define CRYINCLUDE_CRYACTION_GAMETOKENS_GAMETOKEN_H
#pragma once

#include "IGameTokens.h"

class CGameTokenSystem;

//////////////////////////////////////////////////////////////////////////
class CGameToken
    : public IGameToken
{
public:
    CGameToken();
    ~CGameToken();

    //////////////////////////////////////////////////////////////////////////
    // IGameToken implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void SetName(const char* sName);
    virtual const char* GetName() const { return m_name; }
    virtual void SetFlags(uint32 flags) { m_nFlags = flags; }
    virtual uint32 GetFlags() const { return m_nFlags; }
    virtual EFlowDataTypes GetType() const { return (EFlowDataTypes)m_value.GetType(); };
    virtual void SetType(EFlowDataTypes dataType);
    virtual void SetValue(const TFlowInputData& val);
    virtual bool GetValue(TFlowInputData& val) const;
    virtual void SetValueAsString(const char* sValue, bool bDefault = false);
    virtual const char* GetValueAsString() const;
    //////////////////////////////////////////////////////////////////////////

    void AddListener(IGameTokenEventListener* pListener) { stl::push_back_unique(m_listeners, pListener); };
    void RemoveListener(IGameTokenEventListener* pListener) { stl::find_and_erase(m_listeners, pListener); };
    void Notify(EGameTokenEvent event);

    CTimeValue GetLastChangeTime() const { return m_changed; };

    void GetMemoryStatistics(ICrySizer* s);

private:
    friend class CGameTokenSystem; // Need access to m_name
    static CGameTokenSystem* g_pGameTokenSystem;

    uint32 m_nFlags;
    string m_name;
    TFlowInputData m_value;

    CTimeValue m_changed;

    typedef std::list<IGameTokenEventListener*> Listeneres;
    Listeneres m_listeners;
};


#endif // CRYINCLUDE_CRYACTION_GAMETOKENS_GAMETOKEN_H

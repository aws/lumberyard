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

// Description : Action Map implementation. Maps Actions to Keys.


#ifndef CRYINCLUDE_CRYACTION_ACTIONMAP_H
#define CRYINCLUDE_CRYACTION_ACTIONMAP_H
#pragma once

#include <CryListenerSet.h>

#include "IActionMapManager.h"

class CActionMapManager;
class CActionMap;

typedef std::vector<SActionInput*> TActionInputs;

class CActionMapAction
    : public IActionMapAction
{
public:
    CActionMapAction();
    virtual ~CActionMapAction();

    // IActionMapAction
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual void Release() { delete this; };
    virtual int GetNumActionInputs() const { return m_actionInputs.size(); };
    virtual const SActionInput* FindActionInput(const char* szInput) const;
    virtual const SActionInput* GetActionInput(const int iIndex) const;
    virtual const SActionInput* GetActionInput(const EActionInputDevice device, const int iIndexByDevice) const;
    virtual const ActionId& GetActionId() const { return m_actionId; }
    virtual const char* GetTriggeredActionInput() const { return m_triggeredInput; };
    // IActionMapAction

    bool AddActionInput(const SActionInput& actionInput, const int iByDeviceIndex = -1);
    bool RemoveActionInput(uint32 inputCRC);
    void RemoveAllActionInputs();
    SActionInput* AddAndGetActionInput(const SActionInput& actionInput);
    void SetParentActionMap(CActionMap* pParentActionMap) { m_pParentActionMap = pParentActionMap; }
    SActionInput* FindActionInput(uint32 inputCRC);
    SActionInput* GetActionInput(const int iIndex);
    SActionInput* GetActionInput(const EActionInputDevice device, const int iIndexByDevice);
    void SetActionId(const ActionId& actionId) { m_actionId = actionId; }
    void SetNumRebindedInputs(const int iNumRebindedInputs) { m_iNumRebindedInputs = iNumRebindedInputs; }
    int GetNumRebindedInputs() const { return m_iNumRebindedInputs; }

private:
    TActionInputString  m_triggeredInput;
    ActionId                        m_actionId;
    TActionInputs               m_actionInputs;
    CActionMap*                 m_pParentActionMap;
    int                                 m_iNumRebindedInputs;
};

class CActionMap
    : public IActionMap
{
public:
    CActionMap(CActionMapManager* pActionMapManager, const char* name);
    virtual ~CActionMap();

    // IActionMap
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual void Release();
    virtual void Clear();
    virtual const IActionMapAction* GetAction(const ActionId& actionId) const;
    virtual IActionMapAction* GetAction(const ActionId& actionId);
    virtual bool CreateAction(const ActionId& actionId);
    virtual bool RemoveAction(const ActionId& actionId);
    virtual int  GetActionsCount() const { return m_actions.size(); };
    virtual bool AddActionInput(const ActionId& actionId, const SActionInput& actionInput, const int iByDeviceIndex = -1);
    virtual bool AddAndBindActionInput(const ActionId& actionId, const SActionInput& actionInput) override;
    virtual bool RemoveActionInput(const ActionId& actionId, const char* szInput);
    virtual bool ReBindActionInput(const ActionId& actionId, const char* szCurrentInput, const char* szNewInput);
    virtual bool ReBindActionInput(const ActionId& actionId,
        const char* szNewInput,
        const EActionInputDevice device,
        const int iByDeviceIndex);
    virtual int GetNumRebindedInputs() { return m_iNumRebindedInputs; }
    virtual bool Reset();
    virtual bool LoadFromXML(const XmlNodeRef& actionMapNode);
    virtual bool LoadRebindingDataFromXML(const XmlNodeRef& actionMapNode);
    virtual bool SaveRebindingDataToXML(XmlNodeRef& actionMapNode) const;
    virtual IActionMapActionIteratorPtr CreateActionIterator();
    virtual void SetActionListener(EntityId id);
    virtual EntityId GetActionListener() const;
    virtual const char* GetName() { return m_name.c_str(); }
    virtual void Enable(bool enable);
    virtual bool Enabled() const { return m_enabled; };
    // ~IActionMap

    void EnumerateActions(IActionMapPopulateCallBack* pCallBack) const;
    bool CanProcessInput(const SInputEvent& inputEvent, CActionMap* pActionMap, CActionMapAction* pAction, SActionInput* pActionInput);
    bool IsActionInputTriggered(const SInputEvent& inputEvent, CActionMap* pActionMap, CActionMapAction* pAction, SActionInput* pActionInput) const;
    void InputProcessed();
    void ReleaseActionsIfActive();
    void ReleaseActionIfActive(const ActionId& actionId);
    void ReleaseFilteredActions();
    void AddExtraActionListener(IActionListener* pExtraActionListener);
    void RemoveExtraActionListener(IActionListener* pExtraActionListener);
    void NotifyExtraActionListeners(const ActionId& action, int currentState, float value);

private:
    CActionMapAction* CreateAndGetAction(const ActionId& actionId);
    bool AddAndBindActionInput(CActionMapAction* pAction, const SActionInput& actionInput);
    bool ReBindActionInput(CActionMapAction* pAction, const char* szCurrentInput, const char* szNewInput);
    bool ReBindActionInput(CActionMapAction* pAction, SActionInput& actionInput, const char* szNewInput);
    bool ReBindActionInput(CActionMapAction* pAction,
        const char* szNewInput,
        const EActionInputDevice device,
        const int iByDeviceIndex);
    void ReleaseActionIfActiveInternal(CActionMapAction& action);
    EActionAnalogCompareOperation GetAnalogCompareOpTypeFromStr(const char* szTypeStr);
    const char* GetAnalogCompareOpStr(EActionAnalogCompareOperation compareOpType) const;
    void SetNumRebindedInputs(const int iNumRebindedInputs) { m_iNumRebindedInputs = iNumRebindedInputs; }
    bool LoadActionInputAttributesFromXML(const XmlNodeRef& actionInputNode, SActionInput& actionInput);
    bool SaveActionInputAttributesToXML(XmlNodeRef& actionInputNode, const SActionInput& actionInput) const;
    void LoadActivationModeBitAttributeFromXML(const XmlNodeRef& attributeNode, int& activationFlags, const char* szActivationModeName, EActionActivationMode activationMode);
    void SaveActivationModeBitAttributeToXML(XmlNodeRef& attributeNode, const int activationFlags, const char* szActivationModeName, EActionActivationMode activationMode) const;

    typedef std::map<ActionId, CActionMapAction>    TActionMap;
    typedef CListenerSet<IActionListener*>              TActionMapListeners;

    bool                                m_enabled;
    CActionMapManager*  m_pActionMapManager;
    TActionMap                  m_actions;
    EntityId                        m_listenerId;
    TActionMapListeners m_actionMapListeners;
    string                          m_name;
    int                                 m_iNumRebindedInputs;
};


#endif // CRYINCLUDE_CRYACTION_ACTIONMAP_H

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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_OBJECTLISTCTRL_H
#define CRYINCLUDE_EDITOR_CONTROLS_OBJECTLISTCTRL_H
#pragma once

#include "SubeditListCtrl.h"

class CObjectListCtrl;

class IListCtrlObject
{
public:
    virtual ~IListCtrlObject() = 0;

    virtual CString GetText(int nSubItem) = 0;
    virtual bool IsSelected() = 0;
    virtual CSubeditListCtrl::EditStyle GetEditStyle(int subitem) = 0;
    virtual void GetOptions(int subitem, std::vector<string>& options, string& currentOption) = 0;
    virtual void SetIndex(int nIndex) = 0;
};

inline IListCtrlObject::~IListCtrlObject() {}

class IObjectListCtrlListener
{
public:
    virtual ~IObjectListCtrlListener() = 0;

    virtual void OnObjectListCtrlSelectionRangeChanged(CObjectListCtrl* pCtrl, int nFirstSelectedItem, int nLastSelectedItem, bool bSelected) = 0;
    virtual void OnObjectListCtrlLabelChanged(CObjectListCtrl* pCtrl, int nItem, int nSubItem, const char* szNewText) = 0;
    virtual void OnObjectListCtrlItemActivated(CObjectListCtrl* pCtrl, int nItem) = 0;
};

inline IObjectListCtrlListener::~IObjectListCtrlListener() {}

// CObjectListCtrl

class CObjectListCtrl
    : public CSubeditListCtrl
{
    DECLARE_DYNAMIC(CObjectListCtrl)

public:
    CObjectListCtrl();
    virtual ~CObjectListCtrl();

    void AddListener(IObjectListCtrlListener* pListener);
    void RemoveListener(IObjectListCtrlListener* pListener);
    int AddObject(IListCtrlObject* pObject);
    void DeleteAllItems();

private:
    afx_msg void OnLVNGetDispInfo(NMHDR* pInfo, LRESULT* result);
    afx_msg void OnLVNSetDispInfo(NMHDR* pInfo, LRESULT* result);
    afx_msg void OnLVNODStateChanged(NMHDR* pInfo, LRESULT* result);
    afx_msg void OnLVNItemChanged(NMHDR* pInfo, LRESULT* result);

    std::vector<IListCtrlObject*> objects;
    std::set<IObjectListCtrlListener*> listeners;

protected:
    DECLARE_MESSAGE_MAP()

    virtual void TextChanged(int item, int subitem, const char* szText);
    virtual void GetOptions(int item, int subitem, std::vector<string>& options, string& currentOption);
    virtual EditStyle GetEditStyle(int item, int subitem);
};



#endif // CRYINCLUDE_EDITOR_CONTROLS_OBJECTLISTCTRL_H

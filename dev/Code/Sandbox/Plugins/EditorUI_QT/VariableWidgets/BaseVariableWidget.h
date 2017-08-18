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
#ifndef CRYINCLUDE_EDITORUI_QT_VARIABLE_WIDGETS_IVARIABLEWIDGET_H
#define CRYINCLUDE_EDITORUI_QT_VARIABLE_WIDGETS_IVARIABLEWIDGET_H
#pragma once

struct IVariable;
class CAttributeItem;

#include <functional>
#include <vector>

class CBaseVariableWidget
{
public:
    CBaseVariableWidget(CAttributeItem* parent);
    virtual ~CBaseVariableWidget();

    template <class T>
    void addOnDestructionEvent(const T& ptr);
    virtual void setVar(IVariable* var) {m_var = var; }
    virtual void onVarChanged(IVariable* var) = 0;
    IVariable* getVar(){return m_var; }
    CAttributeItem* getParent(){ return m_parent; }
protected:
    std::vector<std::function<void(CBaseVariableWidget*)> > m_onDestructionEvents;
    bool        m_ignoreSetCallback;
    IVariable*  m_var;
    CAttributeItem* m_parent;
};

template <class T>
void CBaseVariableWidget::addOnDestructionEvent(const T& ptr)
{
    m_onDestructionEvents.push_back(ptr);
}

class __ScoppedBoolToggle
{
public:
    __ScoppedBoolToggle(bool& val)
        : mVal(val)
    {
        mVal = true;
    }

    ~__ScoppedBoolToggle()
    {
        mVal = !mVal;
    }

private:
    bool& mVal;
};
#define SelfCallFence(boolHandle) \
    if (boolHandle) {return; } __ScoppedBoolToggle __scopedBoolToggle(boolHandle)

#endif // CRYINCLUDE_EDITORUI_QT_VARIABLE_WIDGETS_IVARIABLEWIDGET_H

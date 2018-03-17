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

// Description : Subclassed CPropertyCtrl for extensions


#ifndef CRYINCLUDE_EDITOR_VEHICLES_PROPERTYCTRLEXT_H
#define CRYINCLUDE_EDITOR_VEHICLES_PROPERTYCTRLEXT_H
#pragma once


#include <Controls/ReflectedPropertyControl/ReflectedPropertyCtrl.h>


/**
*/
class CPropertyCtrlExt
    : public ReflectedPropertyControl
{
    Q_OBJECT
public:
    typedef Functor1<ReflectedPropertyItem*> PreSelChangeCallback;

    CPropertyCtrlExt(QWidget* widget = nullptr);

    void SetPreSelChangeCallback(PreSelChangeCallback callback) { m_preSelChangeFunc = callback; }
    void SelectItem(ReflectedPropertyItem* item) override;
    void OnItemChange(ReflectedPropertyItem* item, bool deferCallbacks = true) override;

    void SetVehicleVar(IVariable* pVar) { m_pVehicleVar = pVar; }

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;

    void OnAddChild();
    void OnDeleteChild(ReflectedPropertyItem* pItem);
    void OnGetEffect(ReflectedPropertyItem* pItem);

    void ReloadItem(ReflectedPropertyItem* pItem);

    PreSelChangeCallback m_preSelChangeFunc;

    IVariable* m_pVehicleVar;
};


#endif // CRYINCLUDE_EDITOR_VEHICLES_PROPERTYCTRLEXT_H

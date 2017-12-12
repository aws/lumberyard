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

#pragma once
//////////////////////////////////////////////////////////////////////////
//  CryENGINE Source File
//  Copyright (C) 2000-2012, Crytek GmbH, All rights reserved
//////////////////////////////////////////////////////////////////////////
#ifndef CRYINCLUDE_EDITOR_OBJECTPANEL_H
#define CRYINCLUDE_EDITOR_OBJECTPANEL_H

#include <QWidget>

class CBaseObject;

namespace Ui
{
    class ObjectPanel;
}

class CObjectPanel
    : public QWidget
{
    Q_OBJECT

    // Construction
public:
    struct SParams
    {
        SParams(CBaseObject* pObject = NULL);
        QString name;
        QColor color;
        float area;
        QString layer;
        uint32 minSpec;
    };
    CObjectPanel(QWidget* parent = nullptr);   // standard constructor
    ~CObjectPanel();

    void SetMultiSelect(bool bEnable);
    bool IsMultiSelect() const { return m_multiSelect; };
    void SetParams(CBaseObject* obj, const SParams& params);
    void GetParams(SParams& params);

    CBaseObject* GetObject() const { return m_obj; }

    virtual void OnUpdate();

    //! Callback called when layer is updated.
    void OnLayerUpdate(int event, CObjectLayer* pLayer);


    // Implementation
protected:
    void OnObjectColor(const QColor& color);
    void OnInitDialog();
    void OnUpdateArea(double value);
    void OnBnClickedLayer();
    void OnChangeName();
    void OnBnClickedMaterial();
    void OnMinSpecChange();

    QScopedPointer<Ui::ObjectPanel> m_ui;

    CBaseObject* m_obj;
    bool m_multiSelect;
    bool m_currentLayerModified;
};
#endif // CRYINCLUDE_EDITOR_OBJECTPANEL_H

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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_PICKOBJECTBUTTON_H
#define CRYINCLUDE_EDITOR_CONTROLS_PICKOBJECTBUTTON_H
#pragma once

#include <QPushButton>

class QPickObjectButton
    : public QPushButton
    , public IPickObjectCallback
{
    Q_OBJECT

    Q_PROPERTY(bool Selected READ IsSelected WRITE SetSelected)
    // Construction
public:
    QPickObjectButton(QWidget* parent = nullptr);
    virtual ~QPickObjectButton();

    void SetPickCallback(IPickObjectCallback* callback, const QString& statusText, const QMetaObject* targetClass = 0, bool bMultiPick = false);

    // Implementation
public:

    //! Called when object picked.
    virtual void OnPick(CBaseObject* picked);
    //! Called when pick mode cancelled.
    virtual void OnCancelPick();
    virtual bool OnPickFilter(CBaseObject* filterObject);

    bool IsSelected() const { return m_selected; }
    void SetSelected(bool selected);

    // Generated message map functions
protected:
    void OnClicked();

    const QString m_styleSheet;
    bool m_selected = false;

    IPickObjectCallback* m_pickCallback = nullptr;
    QString m_statusText;
    const QMetaObject* m_targetClass = nullptr;
    bool m_bMultipick = false;
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_PICKOBJECTBUTTON_H

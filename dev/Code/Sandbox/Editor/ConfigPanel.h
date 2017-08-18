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
#ifndef CRYINCLUDE_EDITOR_CONFIGPANEL_H
#define CRYINCLUDE_EDITOR_CONFIGPANEL_H

#include <QWidget>
#include <QScopedPointer>

namespace Config
{
    struct IConfigVar;
    class CConfigGroup;
}

class QGroupBox;

class CConfigPanel
    : public QWidget
    , public IEditorNotifyListener
{
public:
    CConfigPanel(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CConfigPanel();

    // Create edit controls for given configuration group
    void DisplayGroup(Config::CConfigGroup* pGroup, const char* szGroupName);

    // Called after value of given config variable was changed
    virtual void OnConfigValueChanged(Config::IConfigVar* pVar) {};

private slots:
    void OnOptionChanged(bool checked);
    void OnTextChanged(const QString& text);

protected:
    // IEditorNotifyListener interface implementation
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

    struct CItem
    {
        QWidget* m_widget;
        Config::IConfigVar* m_pVar;

        CItem(Config::IConfigVar* pVar, QWidget* widget);
        ~CItem();
    };

    typedef std::map<QObject*, QScopedPointer<CItem> > TItemMap;
    TItemMap m_items;

    uint m_currentId;
};

#endif // CRYINCLUDE_EDITOR_CONFIGPANEL_H

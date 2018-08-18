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
#pragma once
#include "api.h"
// QT
#include <QDockWidget>
#include <QMap>
#include <QShortcutEvent>

#include <AzCore/std/functional.h>

class EDITOR_QT_UI_API FloatableDockPanel
    : public QDockWidget
{
    Q_OBJECT
public:
    explicit FloatableDockPanel(const QString& title, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    void SetHotkeyHandler(QObject* obj, AZStd::function<void(QKeyEvent* e)> hotkeyHandler);
    void SetShortcutHandler(QObject* obj, AZStd::function<bool(QShortcutEvent* e)> shortcutHandler);
private Q_SLOTS:
    void onFloatingStatusChanged(bool floating);
protected:
    virtual bool eventFilter(QObject* obj, QEvent* e) override;
    QMap<QObject*, AZStd::function<void(QKeyEvent* e)> > m_hotkeyHandlers;
    QMap<QObject*, AZStd::function<bool(QShortcutEvent* e)> > m_shortcutHandlers;
};
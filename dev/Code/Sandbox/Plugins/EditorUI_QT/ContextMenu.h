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

#ifndef CONTEXTMENU_H
#define CONTEXTMENU_H

#include "api.h"
#include <QMenu>

class EDITOR_QT_UI_API ContextMenu
    : public QMenu
{
    Q_OBJECT

public:
    ContextMenu(QWidget* p = nullptr);
    ContextMenu(const QString& title, QWidget* parent = nullptr);
    ~ContextMenu()
    {
    }
private:
    void Initialize();
};

#endif
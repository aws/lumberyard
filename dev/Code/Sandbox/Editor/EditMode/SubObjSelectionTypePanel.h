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

#ifndef CRYINCLUDE_EDITOR_EDITMODE_SUBOBJSELECTIONTYPEPANEL_H
#define CRYINCLUDE_EDITOR_EDITMODE_SUBOBJSELECTIONTYPEPANEL_H
#pragma once

#include "Objects/SubObjSelection.h"
#include <QWidget>
#include <QScopedPointer>

class QListWidgetItem;

namespace Ui {
    class CSubObjSelectionTypePanel;
}

// CSubObjSelectionTypePanel dialog

class CSubObjSelectionTypePanel
    : public QWidget
{
    Q_OBJECT
public:
    CSubObjSelectionTypePanel(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CSubObjSelectionTypePanel();

    void SelectElemtType(int nIndex);

protected slots:
    void ChangeSelectionType(QListWidgetItem* current, QListWidgetItem* previous);
private:
    ESubObjElementType ListItemToElementType(QListWidgetItem* current);
    QListWidgetItem* ElementTypeToListItem(ESubObjElementType elementType);

    bool m_initialized;
    QScopedPointer<Ui::CSubObjSelectionTypePanel> ui;
};

#endif // CRYINCLUDE_EDITOR_EDITMODE_SUBOBJSELECTIONTYPEPANEL_H

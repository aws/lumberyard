/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/
#ifndef CRYINCLUDE_EDITORUI_QT_ATTRIBUTELISTVIEW_H
#define CRYINCLUDE_EDITORUI_QT_ATTRIBUTELISTVIEW_H
#pragma once

#include "api.h"

#include <qdockwidget.h>

enum InsertPosition
{
    INSERT_ABOVE,
    INSERT_BELOW
};

class EDITOR_QT_UI_API CAttributeListView
    : public QDockWidget
{
    Q_OBJECT
public:
    CAttributeListView(bool isCustom = false, QString groupvisibility = "Both");
    CAttributeListView(QWidget* parent, bool isCustom = false, QString groupvisibility = "Both");

    // QDragDrop Related Events
    void dragMoveEvent(QDragMoveEvent* event);
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);

    void setCustomPanel(bool isCustom);
    bool isCustomPanel();
    bool isShowInGroup();
    bool isHideInEmitter();
    QString getGroupVisibility();

private:
    void SetDragStyle(QWidget* widget, QString style);

    bool m_isCustom;
    QWidget* m_previousGroup;
    InsertPosition positionIndex;
    QString m_GroupVisibility;
};

#endif // CRYINCLUDE_EDITORUI_QT_ATTRIBUTEITEM_H

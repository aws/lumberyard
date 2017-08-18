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
#ifndef CRYINCLUDE_EDITORUI_QT_AttributeGroup_H
#define CRYINCLUDE_EDITORUI_QT_AttributeGroup_H
#pragma once

#include "AttributeItem.h"
#include "AttributeViewConfig.h"
#include "AttributeItemLogicCallbacks.h"

class EDITOR_QT_UI_API CAttributeGroup
    : public QWidget
{
    Q_OBJECT
public:

    CAttributeGroup(QWidget* parent);
    CAttributeGroup(QWidget* parent, CAttributeItem* content);
    ~CAttributeGroup();
    void Init();

    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void paintEvent(QPaintEvent* event);
    void SetConfigGroup(const CAttributeViewConfig::config::group* config);
    void SetConfigItem(const CAttributeViewConfig::config::item* config);
    CAttributeItem* GetContent(){ return m_content; }
    void SetContent(CAttributeItem* content);
    bool DropAGroup(CAttributeItem* dropAtAttributeItem, QWidget* insertAtGroup, int positionIndex);
    const CAttributeViewConfig::config::group* GetConfigGroup(){ return m_attributeConfigGroup; }
    const CAttributeViewConfig::config::item* GetConfigItem(){ return m_attributeConfigItem; }
    void SetDragStyle(QString DragStatus);

private:
    QVBoxLayout* m_layout;
    CAttributeItem* m_content;
    const CAttributeViewConfig::config::group* m_attributeConfigGroup;
    const CAttributeViewConfig::config::item* m_attributeConfigItem;

    QPoint m_dragStartPoint;
};
#endif // CRYINCLUDE_EDITORUI_QT_AttributeGroup_H

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

#include "UICommon.h"

class QSlider;
class QTreeWidget;
class QTreeWidgetItem;
class SubdivisionTool;

class SubdivisionPanel
    : public QWidget
    , public IBasePanel
{
public:
    SubdivisionPanel(SubdivisionTool* pTool);

    void Done() override;
    void Update() override;

protected:
    void OnFreeze();
    void OnDelete();
    void OnDeleteUnused();
    void OnClear();
    void OnItemDoubleClicked(QTreeWidgetItem* item, int column);
    void OnItemChanged(QTreeWidgetItem* item, int column);
    void OnCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);

    void AddSemiSharpGroup();
    void RefreshEdgeGroupList();

    QSlider* m_pSubdivisionLevelSlider;
    QTreeWidget* m_pSemiSharpCreaseList;
    SubdivisionTool* m_pSubdivisionTool;
    QString m_ItemNameBeforeEdit;
};
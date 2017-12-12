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


#ifndef OBJECT_SELECTOR_TABLE_VIEW_H
#define OBJECT_SELECTOR_TABLE_VIEW_H
#pragma once

#include "StdAfx.h"

#include <QTableView>
#include <QSettings>

class QMenu;
class QAction;
class QMouseEvent;
class ObjectSelectorModel;

class ObjectSelectorTableView
    : public QTableView
{
    Q_OBJECT
public:
    explicit ObjectSelectorTableView(QWidget* parent = nullptr);
    ~ObjectSelectorTableView();
    void setModel(QAbstractItemModel* model) override;
    void EnableTreeMode(bool);
    bool IsTreeMode() const;

private Q_SLOTS:
    void ToggleColumnVisibility(QAction*);

protected:
    bool eventFilter(QObject* watched, QEvent* ev) override;
private:
    void InitializeColumnWidths();
    void CreateHeaderPopup();
    void SaveColumnsVisibility();

    QMenu* m_headerPopup;
    QSettings m_settings;
    ObjectSelectorModel* m_model;
    bool m_bAutoselect = false;
    bool m_treeMode = false;
};

#endif

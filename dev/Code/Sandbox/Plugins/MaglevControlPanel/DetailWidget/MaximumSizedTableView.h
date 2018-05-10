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

#include <QFrame>
#include <QTableView>
#include <QScrollBar>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>

class MaximumSizeTableViewObject
    : public QTableView
{
    Q_OBJECT
public:
    MaximumSizeTableViewObject(QWidget* parent);

    void mouseReleaseEvent(QMouseEvent* event);

signals:
    void OnRightClick(QMouseEvent*);
};

class MaximumSizedTableView
    : public QFrame
{
public:

    MaximumSizedTableView(QWidget* parent);

    void showEvent(QShowEvent* event) override;

    MaximumSizeTableViewObject* TableView();


    void Resize();

private:

    MaximumSizeTableViewObject* m_tableView;

    int m_horizontalScrollBarHeight = 0;
    int m_verticalScrollBarWidth = 0;
};


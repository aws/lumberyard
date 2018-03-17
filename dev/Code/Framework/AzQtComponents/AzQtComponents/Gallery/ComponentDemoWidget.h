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

#include <QMainWindow>
#include <QScopedPointer>

namespace Ui {
    class ComponentDemoWidget;
}

class ComponentDemoWidget : public QMainWindow
{
    Q_OBJECT

public:
    explicit ComponentDemoWidget(QWidget* parent = nullptr);
    ~ComponentDemoWidget() override;

private:
    void addPage(QWidget* widget, const QString& title);

    QScopedPointer<Ui::ComponentDemoWidget> ui;
};



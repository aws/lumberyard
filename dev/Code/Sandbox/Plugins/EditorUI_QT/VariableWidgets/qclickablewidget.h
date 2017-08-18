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
#ifndef QCLICKABLEWIDGET_H
#define QCLICKABLEWIDGET_H

#include <QWidget>

// Forward declaration
class QMouseEvent;

namespace Ui {
    class QClickableWidget;
}

class QClickableWidget
    : public QWidget
{
    Q_OBJECT

public:
    explicit QClickableWidget(QWidget* parent = 0);
    ~QClickableWidget();

signals:
    void clicked();

private slots:
    void on_QClickableWidget_destroyed();

protected:
    void mousePressEvent(QMouseEvent* e);

private:
    Ui::QClickableWidget* ui;
};

#endif // QCLICKABLEWIDGET_H

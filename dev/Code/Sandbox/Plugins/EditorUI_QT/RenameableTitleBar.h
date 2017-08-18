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
#include <QWidget>

class QLabel;
class QLineEdit;
class QHBoxLayout;
class QMouseEvent;

class RenameableTitleBar
    : public QWidget
{
    Q_OBJECT
public:
    RenameableTitleBar(QWidget* parent);

    void SetEditorsPlaceholderText(const QString& text);
    void SetTitle(const QString& text);
    void BeginRename();
    void UpdateSelectedLibStyle(bool LibIsSelected);
    void UpdateExpandedLibStyle(bool LibIsExpanded);

signals:
    void SignalTitleValidationCheck(const QString& text, bool& validName);
    void SignalTitleChanged(const QString& text);
    void SignalLeftClicked(const QPoint& pos);
    void SignalRightClicked(const QPoint& pos);
private:
    void EndRename();
    void mouseReleaseEvent(QMouseEvent* mouseEvent);

    QWidget* m_collapseIcon;
    QWidget* m_icon;
    QLabel* m_label;
    QLineEdit* m_editor;
    QHBoxLayout* m_layout;
    bool m_ignoreEndRename;
};
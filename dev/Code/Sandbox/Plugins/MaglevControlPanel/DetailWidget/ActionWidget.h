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

class QVBoxLayout;
class QLabel;
class QPushButton;
class QDesktopServices;

class HeadingWidget;
class ButtonBarWidget;

class ActionWidget
    : public QFrame
{
    Q_OBJECT

public:

    ActionWidget(QWidget* parent);

    void SetTitleText(const QString& text);
    void SetMessageText(const QString& text);
    void SetActionText(const QString& text);
    void SetActionToolTip(const QString& toolTip);
    void AddButton(QPushButton* button);
    void SetLearnMoreMessageText(const QString& text);
    void AddLearnMoreLink(const QString& text, const QString& url);
    void AddCloudFormationDocumentationLink();

signals:

    void ActionClicked();

protected:

    QPushButton* GetActionButton();

private:

    HeadingWidget* m_headingWidget;
    ButtonBarWidget* m_buttonBarWidget;
    QPushButton* m_actionButton;
    QLabel* m_learnMoreMessageLabel;
    QVBoxLayout* m_learnMoreLayout;

    void CreateUI();

    void OnLinkActivated(const QString& link);

    // always included
    void AddCloudCanvasDocumentationLink();
    void AddCloudCanvasTutorialsLink();
};

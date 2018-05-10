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
#ifndef ACTIONWIDGETCOLOREDCHECKBOX_H
#define ACTIONWIDGETCOLOREDCHECKBOX_H

#include <QWidget>
#include <QWidgetAction>
#include <QMenu>

namespace Ui {
    class ActionWidgetColoredCheckbox;
}

class ActionWidgetColoredCheckboxAction;
class ActionWidgetColoredCheckbox
    : public QWidget
{
    Q_OBJECT

public:
    explicit ActionWidgetColoredCheckbox(QWidget* parent, ActionWidgetColoredCheckboxAction* action);
    ~ActionWidgetColoredCheckbox();
    virtual void focusInEvent(QFocusEvent* event) Q_DECL_OVERRIDE;
    virtual void focusOutEvent(QFocusEvent* event) Q_DECL_OVERRIDE;
    virtual bool event(QEvent* event) Q_DECL_OVERRIDE;
    void receiveFocusFromMenu();

private:
    Ui::ActionWidgetColoredCheckbox* ui;
    bool m_colored;
};

class ActionWidgetColoredCheckboxAction
    : public QWidgetAction
{
public:
    ActionWidgetColoredCheckboxAction(QWidget* parent);
    QWidget* createWidget(QWidget* parent);
    void deleteWidget(QWidget* widget);

    void setCaption(QString caption){m_caption = caption; }
    void setColored(bool colored){m_colored = colored; }
    void setChecked(bool checked){m_checked = checked; }

    //Note: these values are not kept in sync. They are only used to pass on the initial values to ActionWidgetColoredCheckbox
    QString getCaption(){return m_caption; }
    bool getColored(){return m_colored; }
    bool getChecked(){return m_checked; }
private:
    QString m_caption;
    bool m_colored;
    bool m_checked;
};

#endif // ACTIONWIDGETCOLOREDCHECKBOX_H

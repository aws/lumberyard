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
#ifndef QPresetSwatchWidget_h__
#define QPresetSwatchWidget_h__

#include <QWidget>
#include <functional>
#include <Controls/QToolTipWidget.h>

class QLabel;
class QPushButton;
class QGridLayout;

class QPresetWidget
    : public QWidget
{
    Q_OBJECT
public:
    QPresetWidget(QString _name, QWidget* _value, QWidget* parent);
    virtual ~QPresetWidget();

    QString GetName() const;
    void SetName(QString value);

    virtual void setToolTip(QString tip);

    void setPresetSize(int w, int h);
    void setLabelWidth(int w);

    void ShowGrid();
    void ShowList();

    QWidget* GetWidget();

    void SetOnClickCallback (std::function<void(QString)> callback);
    void SetOnRightClickCallback(std::function<void(QString)> callback);

    virtual void SetForegroundStyle(QString ss);
    void SetLabelStyle(QString ss);
    void SetBackgroundStyle(QString ss);

protected:
    virtual bool eventFilter(QObject* obj, QEvent* e);
private:
    QLabel*         name;
    QWidget*        background;
    QWidget*        value;
    QGridLayout*    layout;
    std::function<void(QString)> callback_on_click;
    std::function<void(QString)> callback_on_right_click;
    QToolTipWidget* m_tooltip;
};

#endif // QPresetSwatchWidget_h__

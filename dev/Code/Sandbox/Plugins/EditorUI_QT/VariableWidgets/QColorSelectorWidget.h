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
#ifndef QColorSelectorWidget_h__
#define QColorSelectorWidget_h__

#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QBrush>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QButtonGroup>
#include <QScrollArea>
#include <QScrollBar>
#include <QGroupBox>
#include <QMenu>

#include <functional>
#include "QPresetSelectorWidget.h"


class QColorSelectorWidget
    : public QPresetSelectorWidget
{
    Q_OBJECT
public:
    QColorSelectorWidget(QWidget* parent = 0);
    virtual ~QColorSelectorWidget(){}

    QColor GetSelectedColor();

    void SetColor(QColor color);

    void AddColor(QColor color, QString name);

    void SetValueChangedCallback(std::function<void(QColor)> callback);
protected:
    virtual QWidget* CreateSelectedWidget() override;
    virtual void onValueChanged(unsigned int preset);
    virtual void StylePreset(QPresetWidget* widget, bool selected = false);
    virtual bool AddPresetToLibrary(QWidget* value, unsigned int lib, QString name = QString(), bool allowDefault = false, bool InsertAtTop = true);
    virtual int PresetAlreadyExistsInLibrary(QWidget* value, int lib);
    virtual bool LoadPreset(QString filepath);
    virtual bool SavePreset(QString filepath);
    virtual void BuildDefaultLibrary();

    virtual bool LoadSessionPresetFromName(QSettings* settings, const QString& libName) override;

    virtual QString SettingsBaseGroup() const override;

    virtual void SavePresetToSession(QSettings* settings, const int& lib, const int& preset) override;

    struct Current
    {
        QString name;
        QColor color;
    };
    Current m_current;
    std::function<void(QColor)> callback_value_changed;
};
#endif // QColorSelectorWidget_h__

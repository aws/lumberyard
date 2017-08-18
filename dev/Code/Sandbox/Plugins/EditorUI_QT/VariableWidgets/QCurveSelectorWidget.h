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
#ifndef QCurveSelectorWidget_h__
#define QCurveSelectorWidget_h__


#include "QPresetSelectorWidget.h"
#include "QCurveSwatchWidget.h"

class QCurveSelectorWidget
    : public QPresetSelectorWidget
{
    Q_OBJECT
public:
    QCurveSelectorWidget(QWidget* parent = 0);
    virtual ~QCurveSelectorWidget(){ /*StoreSessionPresets();*/ }

    SCurveEditorCurve& GetSelectedCurve();

    void SetContent(SCurveEditorContent content);
    void SetCurve(SCurveEditorCurve curve);

    void SetCallbackCurveChanged(std::function<void(struct SCurveEditorCurve)> callback)
    {
        callback_curve_changed = callback;
    }
protected:
    virtual void onValueChanged(unsigned int preset) override;
    virtual void StylePreset(QPresetWidget* widget, bool selected = false) override;
    virtual int PresetAlreadyExistsInLibrary(QWidget* value, int lib) override;
    virtual bool LoadPreset(QString filepath) override;
    virtual bool SavePreset(QString filepath) override;
    virtual void BuildDefaultLibrary() override;
    virtual bool AddPresetToLibrary(QWidget* value, unsigned int lib, QString name = QString(), bool allowDefault = false, bool InsertAtTop = true) override;

    virtual QWidget* CreateSelectedWidget() override;

    virtual bool LoadSessionPresetFromName(QSettings* settings, const QString& libName) override;

    virtual void SavePresetToSession(QSettings* settings, const int& lib, const int& preset) override;

    virtual QString SettingsBaseGroup() const override;

    QString m_curveFileName;
protected:
    struct Current
    {
        SCurveEditorContent content;
    };
    Current m_current;
    std::function<void(struct SCurveEditorCurve)> callback_curve_changed;
};

#endif // QCurveSelectorWidget_h__

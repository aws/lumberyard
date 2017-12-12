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
#ifndef QGradientSelectorWidget_h__
#define QGradientSelectorWidget_h__

#include "QPresetSelectorWidget.h"
#include "QGradientSwatchWidget.h"
#include "CurveEditorContent.h"

class QGradientSelectorWidget
    : public QPresetSelectorWidget
{
    Q_OBJECT
public:
    QGradientSelectorWidget(QWidget* parent = 0);
    virtual ~QGradientSelectorWidget(){}

    SCurveEditorCurve& GetSelectedCurve();
    QGradientStops& GetSelectedStops();
    float GetSelectedAlphaMin();
    float GetSelectedAlphaMax();

    void SetContent(SCurveEditorContent content);
    void SetGradientStops(QGradientStops stops);

    void SetCallbackCurveChanged(std::function<void(const struct SCurveEditorCurve&)> callback);
    void SetCallbackGradientChanged(std::function<void(const QGradientStops&)> callback);
    void SetPresetName(QString name);
    virtual bool LoadPreset(QString filepath) override;
    virtual bool SavePreset(QString filepath) override;
protected:
    virtual void onValueChanged(unsigned int preset) override;
    virtual void StylePreset(QPresetWidget* widget, bool selected = false) override;
    virtual int PresetAlreadyExistsInLibrary(QWidget* value, int lib) override;
    virtual void BuildDefaultLibrary() override;
    virtual bool AddPresetToLibrary(QWidget* value, unsigned int lib, QString name = QString(), bool allowDefault = false, bool InsertAtTop = true) override;

    virtual QWidget* CreateSelectedWidget() override;
    virtual bool LoadSessionPresetFromName(QSettings* settings, const QString& libName) override;
    virtual void SavePresetToSession(QSettings* settings, const int& lib, const int& preset) override;
    virtual QString SettingsBaseGroup() const override;

protected:
    std::function<void(struct SCurveEditorCurve)> m_callbackCurveChanged;
    std::function<void(QGradientStops)> m_callbackGradientChanged;

private:
    struct Current
    {
        QGradientStops stops;
        SCurveEditorContent content;
    };
    Current m_current;
    SCurveEditorCurve m_DefaultCurve; //used in QGradientSelectorWidget::GetSelectedCurve when lastPresetIndexSelected is not in range.
};
#endif // QGradientSelectorWidget_h__

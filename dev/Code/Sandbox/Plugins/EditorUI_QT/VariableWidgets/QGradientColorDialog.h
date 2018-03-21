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
#ifndef QGradientColorDialog_h__
#define QGradientColorDialog_h__

#include "../api.h"
#include <QDialog>
#include <QGridLayout>
#include <QPushButton>
#include <QMap>
#include "CurveEditorContent.h"
#include <QBrush>
#include <QDockWidget>
#include <QWidgetAction>
#include <qcoreevent.h>
#include <QLineEdit>
#include <QPushButton>
#include <QMenu>
#include <Controls/QToolTipWidget.h>

struct ISplineInterpolator;
struct SCurveEditorContent;
class QGradientSelectorWidget;
class QCustomGradientWidget;
class QCurveSelectorWidget;
struct SCurveEditorCurve;
class QGradientColorPickerWidget;
class QAmazonLineEdit;

class EDITOR_QT_UI_API QGradientColorDialog
    : public QDialog
{
    Q_OBJECT
private:
    friend class UIFactory;
    QGradientColorDialog(QWidget* parent = 0,
        SCurveEditorContent content = SCurveEditorContent(),
        QGradientStops hueStops = { QGradientStop(0.0, QColor(255, 255, 255, 255)), QGradientStop(1.0, QColor(255, 255, 255, 255)) });
    virtual ~QGradientColorDialog()
    {
        m_gradientmenu->clear();
    }
public:
    QGradientStops GetGradient();
    SCurveEditorCurve GetCurve();
    SCurveEditorContent GetContent();
    void SetCallbackCurve(std::function<void(SCurveEditorContent)> callback);
    void SetCallbackGradient(std::function<void(QGradientStops)> callback);
    void AddCurveToLibrary();
    void AddGradientToLibrary(QString);
    void RemoveCallbacks();
    void UpdateColors(const QMap<QString, QColor>& colorMap);
    void ExportLibrary(QString filepath);

    //////////////////////////////////////////////////////////////////////////
    //callbacks
    void OnCurveChanged(SCurveEditorCurve curve);
    void OnGradientChanged(QGradientStops gradient);
    void OnSelectedGradientChanged(QGradientStops gradient);
    void OnChangeUpdateDisplayedGradient();
    //////////////////////////////////////////////////////////////////////////
protected:
    virtual void showEvent(QShowEvent* e) override;
    virtual void closeEvent(QCloseEvent*) override;
    virtual void hideEvent(QHideEvent*) override;
    virtual bool eventFilter(QObject* obj, QEvent* event) override;

    void SetSelectedCurveKeyPosition();

    void StoreSessionState();
    void LoadSessionState();

    void ShowGradientKeySettings(float uValue = 0.f, float vValue = 0.f);
    void HideGradientKeySettings();
    void ShowHueKeySettings(int location = 100);
    void HideHueKeySettings();

    void AlphaKeyEditingFinished(QAmazonLineEdit* alphaKeyLabel);

protected slots:
    void OnAccepted();
    void OnRejected();
    void keyPressEvent(QKeyEvent* event);

protected:
    QMenu* m_gradientmenu;

    //menu items used in GradientPickerWidget - should move these
    QAmazonLineEdit* m_gradientLine;
    QPushButton* m_gradientAddBtn;
    QWidgetAction* m_gradientLineAction;
    QWidgetAction* m_gradientAddAction;
    //!menu items used in GradientPickerWidget

    //moved some of the hue bar items in here so i could access them for tooltip purposes
    QPushButton* colorPickerButton;
    QAmazonLineEdit* colorSetLocation;
    QAmazonLineEdit* alphaKeySetU;
    QAmazonLineEdit* alphaKeySetV;
    QLabel* localtionLabel;
    QLabel* alphaKeyULabel;
    QLabel* alphaKeyVLabel;
    QLabel* percentLabel;

    QColor selectedColor;
    QGridLayout layout;
    QPushButton okButton, cancelButton;
    std::function<void(SCurveEditorContent)> callback_content_changed;
    std::function<void(QGradientStops)> callback_gradient_changed;
    QGradientSelectorWidget* gradientSelectorWidget;
    QCurveSelectorWidget* curveSelectorWidget;
    QGradientColorPickerWidget* gradientPickerWidget;
    QCustomGradientWidget* m_selectedGradient;
    QToolTipWidget* m_tooltip;
    bool sessionStateLoaded;
};
#endif // QGradientColorDialog_h__

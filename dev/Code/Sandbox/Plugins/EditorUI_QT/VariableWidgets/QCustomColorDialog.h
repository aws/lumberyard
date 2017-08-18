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
#ifndef QCustomColorDialog_h__
#define QCustomColorDialog_h__

#include "../api.h"

#include <QDialog>

#include "QColorDescriptorWidget.h"
#include "QColorSelectorWidget.h"
#include "QGradientSwatchWidget.h"


class QColorEyeDropper;

class EDITOR_QT_UI_API QCustomColorDialog
    : public QDialog
{
    Q_OBJECT
private:
    friend class UIFactory;
    QCustomColorDialog(QColor defaultColor, QColorEyeDropper* m_eyeDropper = 0, QWidget* parent = 0);
    ~QCustomColorDialog(){}
public:
    void OnSelectedColorChanged(QColor _color, bool onStart = false);
    void OnAddColorFromPicker(QColor _color, QString _name = "");
    QColor GetColor() const;
    void setAlphaEnabled(bool allowAlpha);
    void SetStartingHue(int hue);
signals:
    void SignalSelectedColorChanged();
    void SignalEndColorEditing(bool accept);
public slots:
    void OpenColorPickerDocs();

protected:
    void StoreSessionState();
    void LoadSessionState();
    virtual void keyPressEvent(QKeyEvent*) override;
    virtual void closeEvent(QCloseEvent* e) override;
    virtual void showEvent(QShowEvent* e) override;
    virtual void hideEvent(QHideEvent*) override;

    void UnregisterColorPicker();
    void RegisterColorPicker();

    QColor color;
    QVBoxLayout layout;
    QHBoxLayout btnLayout;
    QPushButton* okButton, *cancelButton;
    QPushButton* helpButton;
    bool sessionStateLoaded;

    class QColorDescriptorWidget* descriptor;
    class QColorSelectorWidget* selector;
    class QColorPickerWidget* picker;
    class QColorEyeDropper* dropper;
protected slots:
    void OnAccepted();
    void OnRejected();
};

#endif // QCustomColorDialog_h__

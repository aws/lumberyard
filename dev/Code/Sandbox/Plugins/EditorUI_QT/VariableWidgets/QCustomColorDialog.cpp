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
#include "stdafx.h"
#include "QCustomColorDialog.h"

#ifdef EDITOR_QT_UI_EXPORTS
#include <VariableWidgets/QCustomColorDialog.moc>
#endif
#include "QColorPickerWidget.h"
#include "QColorEyeDropper.h"
#include <QEvent>
#include <QSettings>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QApplication>

#define EDITOR_QTUI_COLORPICKER_HELP_DOC_LINK "https://docs.aws.amazon.com/lumberyard/userguide/color-picker"

QCustomColorDialog::QCustomColorDialog(QColor defaultColor, QColorEyeDropper* eyeDropper, QWidget* parent)
    : layout(this)
    , okButton(new QPushButton(this))
    , helpButton(new QPushButton(this))
    , cancelButton(new QPushButton(this))
    , sessionStateLoaded(false)
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setWindowTitle(tr("Color Picker"));



    descriptor = new QColorDescriptorWidget(this);
    selector = new QColorSelectorWidget(this);
    picker = new QColorPickerWidget(this);
    setContentsMargins(10, 10, 10, 0);
    okButton->setText(tr("Ok"));
    cancelButton->setText(tr("Cancel"));
    okButton->setObjectName("ColorPickerOkButton");
    helpButton->setText("?");
    helpButton->setObjectName("ColorPickerHelpButton");
    connect(okButton, SIGNAL(clicked()), this, SLOT(OnAccepted()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(OnRejected()));
    connect(helpButton, SIGNAL(clicked()), this, SLOT(OpenColorPickerDocs()));
    if (eyeDropper)
    {
        connect(picker, &QColorPickerWidget::SignalEnableEyeDropper, this, [=]()
            {
                RegisterColorPicker();
                eyeDropper->StartEyeDropperMode();
            });
        connect(eyeDropper, &QColorEyeDropper::SignalEyeDropperColorPicked, this, [=](const QColor& color)
            {
                OnSelectedColorChanged(color, false);
            });
    }
    dropper = eyeDropper;
    btnLayout.addWidget(helpButton);
    btnLayout.addStretch(5);
    btnLayout.addWidget(okButton);
    btnLayout.addWidget(cancelButton);

    layout.addWidget(picker, Qt::AlignJustify);
    layout.addWidget(descriptor, Qt::AlignJustify);
    layout.addWidget(selector, Qt::AlignJustify);


    descriptor->SetColorChangeCallBack([=](QColor c){this->OnSelectedColorChanged(c); });
    selector->SetValueChangedCallback([=](QColor c){this->OnSelectedColorChanged(c);    descriptor->SetHue(color.hue()); });
    descriptor->SetHueChangedCallback([=](int hue)
        {
            qreal h, s, l, a;
            color.getHslF(&h, &s, &l, &a);
            h = hue / 360.0f;
            color.setHslF(h, s, l, a);
            picker->SetHue(hue);
        });
    picker->SetColorChangeCallBack([=](QColor c){this->OnSelectedColorChanged(c); });
    picker->SetAddColorCallback([=](QColor c, QString n){this->OnAddColorFromPicker(c, n); });
    picker->SetColorChoiceAsFinalCallback([=](QColor c){ this->OnSelectedColorChanged(c); this->OnAccepted(); });
    connect(picker, &QColorPickerWidget::SignalHueChanged, descriptor, &QColorDescriptorWidget::OnHueEditFinished);
    setLayout(&layout);

    layout.addLayout(&btnLayout);
    setFixedSize(320, 691);
    resize(320, 691);
    okButton->setAutoDefault(true);
    okButton->setFixedSize(54, 24);
    cancelButton->setFixedSize(54, 24);
    helpButton->setFixedSize(24, 24);
}

void QCustomColorDialog::OnSelectedColorChanged(QColor _color, bool onStart /* = false*/)
{
    emit SignalSelectedColorChanged();
    color = _color;
    selector->SetColor(color);
    picker->SetColor(color, onStart);
    // SetColor for descriptor will update the hue value for picker and selector as well.
    // Then selector and picker would think they already be set to updated color and prevent
    // setting the same color again.
    descriptor->SetColor(color);
}

void QCustomColorDialog::OnAddColorFromPicker(QColor _color, QString _name /*= ""*/)
{
    selector->AddColor(_color, _name);
}

QColor QCustomColorDialog::GetColor() const
{
    if (color.isValid())
    {
        return color;
    }
    return QColor();
}

void QCustomColorDialog::setAlphaEnabled(bool allowAlpha)
{
    if (!allowAlpha)
    {
        color.setAlpha(255);
    }
    descriptor->setAlphaEnabled(allowAlpha);
    OnSelectedColorChanged(color);
}

void QCustomColorDialog::closeEvent(QCloseEvent* event)
{
    StoreSessionState();
    selector->SaveOnClose();
    UnregisterColorPicker();
    reject();
    SignalEndColorEditing(false);
}

void QCustomColorDialog::OnAccepted()
{
    selector->SaveOnClose();
    UnregisterColorPicker();
    emit SignalEndColorEditing(true);
    accept();
}

void QCustomColorDialog::OnRejected()
{
    selector->SaveOnClose();
    UnregisterColorPicker();
    emit SignalEndColorEditing(false);
    reject();
}

void QCustomColorDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
    {
        return event->ignore();
    }
    if (event->key() == Qt::Key_Escape)
    {
        return OnRejected();
    }
    QDialog::keyPressEvent(event);
}

void QCustomColorDialog::SetStartingHue(int hue)
{
    descriptor->SetHue(hue);
}

void QCustomColorDialog::showEvent(QShowEvent* e)
{
    QDialog::showEvent(e);
    LoadSessionState();
}

void QCustomColorDialog::StoreSessionState()
{
    if (!sessionStateLoaded)
    {
        // If StoreSessionState() happens to be called before LoadSessionState(), it would clobber saved data.
        return;
    }

    QSettings settings("Amazon", "Lumberyard");
    QString group = "Color Editor/";
    const QPoint _pos = window() ? window()->frameGeometry().topLeft() : QPoint();

    settings.beginGroup(group);
    settings.remove("");
    settings.sync();

    settings.setValue("pos", _pos);

    settings.endGroup();
    settings.sync();
}

void QCustomColorDialog::LoadSessionState()
{
    QSettings settings("Amazon", "Lumberyard");
    QString group = "Color Editor/";
    settings.beginGroup(group);
    QRect desktop = QApplication::desktop()->availableGeometry();


    if (QWidget* topLevel = window())
    {
        const int w = topLevel->width();
        const int h = topLevel->height();
        //start in a central location, or last saved pos
        QPoint pos = settings.value("pos", QPoint((desktop.width() - w) / 2, (desktop.height() - h) / 2)).toPoint();
        topLevel->move(pos);
    }

    settings.endGroup();
    settings.sync();

    sessionStateLoaded = true;
}

void QCustomColorDialog::OpenColorPickerDocs()
{
    QDesktopServices::openUrl(QUrl(EDITOR_QTUI_COLORPICKER_HELP_DOC_LINK));
}


void QCustomColorDialog::hideEvent(QHideEvent* e)
{
    StoreSessionState();
    QDialog::hideEvent(e);
}

void QCustomColorDialog::UnregisterColorPicker()
{
    dropper->UnregisterExceptionWidget(okButton);
    dropper->UnregisterExceptionWidget(cancelButton);
    if (dropper && dropper->EyeDropperIsActive())
    {
        dropper->EndEyeDropperMode();
    }

}

void QCustomColorDialog::RegisterColorPicker()
{
    dropper->RegisterExceptionWidget(okButton);
    dropper->RegisterExceptionWidget(cancelButton);
}

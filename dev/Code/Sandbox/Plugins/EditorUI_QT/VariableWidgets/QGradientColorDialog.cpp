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
#include "QGradientColorDialog.h"

#include "ISplines.h"
#include "CurveEditorContent.h"
#include "QGradientSelectorWidget.h"
#include "QCustomGradientWidget.h"
#include "QCurveSelectorWidget.h"
#include "QGradientColorPickerWidget.h"
#include "QCustomColorDialog.h"
#include "../ContextMenu.h"
#include "UIFactory.h"
#include <QSettings>
#include <QApplication>
#include <QDesktopWidget>
#include "QAmazonLineEdit.h"
#include "IEditorParticleUtils.h"

#define QGRADIENTCOLORDIALOG_PRECISION 4
#define QGRADIENTCOLORDIALOG_HEADER_HEIGHT 24

QGradientColorDialog::QGradientColorDialog(QWidget* parent,
    SCurveEditorContent content,
    QGradientStops hueStops)
    : QDialog(parent)
    , layout(this)
    , m_gradientmenu(new ContextMenu(this))
    , m_gradientLine(new QAmazonLineEdit(this))
    , m_gradientAddBtn(new QPushButton(this))
    , okButton(this)
    , cancelButton(this)
    , sessionStateLoaded(false)
{
    setWindowTitle("Gradient Editor");
    //changed minimum width to account for clipping of text on buttons when resolution was lower
    setMinimumWidth(475);
    gradientSelectorWidget = new QGradientSelectorWidget(this);
    gradientSelectorWidget->SetContent(content);
    gradientSelectorWidget->SetGradientStops(hueStops);
    curveSelectorWidget = new QCurveSelectorWidget(this);
    curveSelectorWidget->SetContent(content);
    gradientPickerWidget = new QGradientColorPickerWidget(content, hueStops, this);

    gradientSelectorWidget->SetCallbackCurveChanged([=](SCurveEditorCurve _curve){this->OnCurveChanged(_curve); });
    curveSelectorWidget->SetCallbackCurveChanged([=](SCurveEditorCurve _curve){this->OnCurveChanged(_curve); });
    gradientSelectorWidget->SetCallbackGradientChanged([=](QGradientStops _stops){this->OnSelectedGradientChanged(_stops); });
    gradientPickerWidget->SetCallbackCurveChanged([=](SCurveEditorCurve _curve){this->OnCurveChanged(_curve); });
    gradientPickerWidget->SetCallbackGradientChanged([=](QGradientStops _stops){this->OnGradientChanged(_stops); });
    gradientPickerWidget->SetCallbackCurrentColorRequired([=]() -> QColor{ return selectedColor; });

    m_gradientLineAction = new QWidgetAction(m_gradientmenu);
    m_gradientLineAction->setDefaultWidget(m_gradientLine);
    m_gradientAddBtn->setText("Add to Library");
    m_gradientAddAction = new QWidgetAction(m_gradientmenu);
    m_gradientAddAction->setDefaultWidget(m_gradientAddBtn);

    m_gradientmenu->addAction(m_gradientLineAction);
    m_gradientmenu->addAction(m_gradientAddAction);
    connect(m_gradientAddBtn, &QPushButton::clicked, this, [&]()
        {
            gradientSelectorWidget->SetPresetName(m_gradientLine->text());
            AddGradientToLibrary(QString("Default"));
            m_gradientmenu->hide();
            m_gradientLine->setText("");
        });

    m_selectedGradient = new QCustomGradientWidget();
    m_selectedGradient->AddGradient(new QLinearGradient(0, 0, 1, 0), QPainter::CompositionMode::CompositionMode_Source);
    m_selectedGradient->SetGradientStops(0, gradientSelectorWidget->GetSelectedStops());
    m_selectedGradient->SetBackgroundImage(":/particleQT/icons/color_btn_bkgn.png");
    m_selectedGradient->setMinimumWidth(130);
    // set height of the gradient swatch matches the UI on attribute panel
    m_selectedGradient->setMaximumHeight(QGRADIENTCOLORDIALOG_HEADER_HEIGHT);
    m_selectedGradient->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);

    if (content.m_curves.size() > 0)
    {
        OnCurveChanged(content.m_curves.back());
    }
    OnGradientChanged(hueStops);

    //@eric - conffx

    localtionLabel = new QLabel(this);
    localtionLabel->setText("Location");
    localtionLabel->setMinimumWidth(30);
    localtionLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);

    alphaKeyULabel = new QLabel(this);
    alphaKeyULabel->setText("u");
    alphaKeyULabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);

    alphaKeyVLabel = new QLabel(this);
    alphaKeyVLabel->setText("v");
    alphaKeyVLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);

    colorSetLocation = new QAmazonLineEdit(this);
    colorSetLocation->setText("100");
    colorSetLocation->setMinimumWidth(45);
    colorSetLocation->setMaximumHeight(QGRADIENTCOLORDIALOG_HEADER_HEIGHT);
    colorSetLocation->installEventFilter(this);
    colorSetLocation->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);

    alphaKeySetU = new QAmazonLineEdit(this);
    alphaKeySetU->setText("0");
    alphaKeySetU->setMinimumWidth(45);
    alphaKeySetU->setMaximumHeight(QGRADIENTCOLORDIALOG_HEADER_HEIGHT);
    alphaKeySetU->installEventFilter(this);
    alphaKeySetU->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);

    alphaKeySetV = new QAmazonLineEdit(this);
    alphaKeySetV->setText("0");
    alphaKeySetV->setMinimumWidth(45);
    alphaKeySetV->setMaximumHeight(QGRADIENTCOLORDIALOG_HEADER_HEIGHT);
    alphaKeySetV->installEventFilter(this);
    alphaKeySetV->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);


    percentLabel = new QLabel(this);
    percentLabel->setText("%");
    percentLabel->setMinimumSize(10, QGRADIENTCOLORDIALOG_HEADER_HEIGHT);
    percentLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);


    QLabel* ColorLabel = new QLabel(this);
    ColorLabel->setText("Color");
    ColorLabel->setMinimumSize(30, QGRADIENTCOLORDIALOG_HEADER_HEIGHT);
    ColorLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);

    colorPickerButton = new QPushButton(this);
    colorPickerButton->setMaximumWidth(30);
    colorPickerButton->setStyleSheet(QString().sprintf("QPushButton{background: rgba(%d,%d,%d);}", selectedColor.red(), selectedColor.green(), selectedColor.blue()));
    colorPickerButton->setMaximumSize(30, QGRADIENTCOLORDIALOG_HEADER_HEIGHT);
    colorPickerButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
    colorPickerButton->installEventFilter(this);

    int row = 0, column = 0, maxColumns = 16;
    layout.addWidget(localtionLabel, row, column, 1, 1, Qt::AlignLeft);
    layout.addWidget(alphaKeyULabel, row, column, 1, 1, Qt::AlignLeft);

    column++;
    layout.addWidget(colorSetLocation, row, column, 1, 1, Qt::AlignLeft);
    layout.addWidget(alphaKeySetU, row, column, 1, 1, Qt::AlignLeft);

    column++;
    layout.addWidget(percentLabel, row, column, 1, 1, Qt::AlignLeft);
    layout.addWidget(alphaKeyVLabel, row, column, 1, 1, Qt::AlignLeft);

    column++;
    layout.addWidget(alphaKeySetV, row, column, 1, 1, Qt::AlignLeft);

    column++;
    layout.addWidget(m_selectedGradient, row, column, 1, 4, Qt::AlignRight);
    column += 4;
    layout.addWidget(ColorLabel, row, column, 1, 1, Qt::AlignRight);
    column++;
    layout.addWidget(colorPickerButton, row, column, 1, 1, Qt::AlignRight);

    int leftColumnSize = ++column; // Record how many colum already have
    layout.addWidget(curveSelectorWidget, row, column, 2, maxColumns - leftColumnSize, Qt::AlignTop);
    row++;
    column = 0;
    layout.addWidget(gradientPickerWidget, row, column, 1, leftColumnSize);
    row++;
    column = 0;
    layout.addWidget(gradientSelectorWidget, row, column, 1, maxColumns);
    row++;
    //set the colum for two buttons
    column = maxColumns - 2;
    okButton.setText(tr("Ok"));
    cancelButton.setText(tr("Cancel"));
    layout.addWidget(&okButton, row, column, 1, 1);
    column++;
    layout.addWidget(&cancelButton, row, column, 1, 1);


    connect(colorPickerButton, &QPushButton::clicked, this, [=]()
        {
            QCustomColorDialog* dlg = UIFactory::GetColorPicker(selectedColor, false);
            if (dlg->exec())
            {
                selectedColor = dlg->GetColor();
                QString colStr;
                int r, g, b;
                selectedColor.getRgb(&r, &g, &b);
                colStr = colStr.sprintf("QPushButton{background: rgba(%d,%d,%d, 255);}", r, g, b);
                colorPickerButton->setStyleSheet(colStr);
            }
        });
    connect(colorSetLocation, &QAmazonLineEdit::editingFinished, this, [=]()
        {
            //only update position if text is not empty
            if (colorSetLocation->text().isEmpty())
            {
                while (colorSetLocation->isUndoAvailable())
                {
                    colorSetLocation->undo();
                }
            }
            //clamp the value and update location text
            int val = qMax(0, qMin(100, colorSetLocation->text().toInt()));
            colorSetLocation->setText(QString("%1").arg(val));
            gradientPickerWidget->SetSelectedGradientPosition(val);
            colorSetLocation->clearFocus();
            m_tooltip->hide();
        });

    connect(alphaKeySetU, &QAmazonLineEdit::editingFinished, this, [=]()
        {
            AlphaKeyEditingFinished(alphaKeySetU);
        });

    connect(alphaKeySetV, &QAmazonLineEdit::editingFinished, this, [=]()
        {
            AlphaKeyEditingFinished(alphaKeySetV);
        });

    connect(gradientPickerWidget, &QGradientColorPickerWidget::SignalSelectCurveKey, this, [=](float u, float v)
        {
            ShowGradientKeySettings(u, v);
            HideHueKeySettings();
        });
    gradientPickerWidget->SetCallbackColorChanged([=](QColor color)
        {
            selectedColor = color;
        });
    gradientPickerWidget->SetCallbackLocationChanged([=](short location)
        {
            HideGradientKeySettings();
            ShowHueKeySettings(location);
            OnChangeUpdateDisplayedGradient();
        });
    gradientPickerWidget->SetCallbackAddCurveToLibary([&]()
        {
            AddCurveToLibrary();
        });
    gradientPickerWidget->SetCallbackAddGradientToLibary([&](QString name)
        {
            AddGradientToLibrary(name);
        });
    connect(&okButton, &QPushButton::clicked, this, [=](){OnAccepted(); });
    connect(&cancelButton, &QPushButton::clicked, this, [=](){OnRejected(); });
    m_tooltip = new QToolTipWidget(this);
    connect(m_gradientLine, &QAmazonLineEdit::textChanged, this, [=](){m_tooltip->hide(); });

    ShowHueKeySettings();
    HideGradientKeySettings();
}

void QGradientColorDialog::OnAccepted()
{
    curveSelectorWidget->SaveOnClose();
    gradientSelectorWidget->SaveOnClose();
    accept();
}

void QGradientColorDialog::OnRejected()
{
    curveSelectorWidget->SaveOnClose();
    gradientSelectorWidget->SaveOnClose();
    reject();
}

void QGradientColorDialog::keyPressEvent(QKeyEvent* event)
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

void QGradientColorDialog::OnCurveChanged(SCurveEditorCurve curve)
{
    SCurveEditorContent content = SCurveEditorContent();
    content.m_curves.clear();
    content.m_curves.push_back(curve);
    content.m_curves.back().m_customInterpolator = gradientPickerWidget->GetContent().m_curves.back().m_customInterpolator;
    gradientPickerWidget->SetCurve(curve);
    curveSelectorWidget->SetCurve(curve);
    gradientPickerWidget->update();
    gradientSelectorWidget->SetContent(content);
    gradientSelectorWidget->update();

    if ((bool)callback_content_changed)
    {
        callback_content_changed(content);
    }
    OnChangeUpdateDisplayedGradient();
}

void QGradientColorDialog::OnGradientChanged(QGradientStops gradient)
{
    gradientSelectorWidget->SetGradientStops(gradient);
    gradientSelectorWidget->update();
    if ((bool)callback_gradient_changed)
    {
        callback_gradient_changed(gradient);
    }
    OnChangeUpdateDisplayedGradient();
}

void QGradientColorDialog::closeEvent(QCloseEvent* closeEvent)
{
    StoreSessionState();
    curveSelectorWidget->SaveOnClose();
    gradientSelectorWidget->SaveOnClose();
    reject();
}

QGradientStops QGradientColorDialog::GetGradient()
{
    return gradientPickerWidget->GetStops();
}

SCurveEditorCurve QGradientColorDialog::GetCurve()
{
    return gradientPickerWidget->GetCurve();
}

SCurveEditorContent QGradientColorDialog::GetContent()
{
    return gradientPickerWidget->GetContent();
}

void QGradientColorDialog::AddCurveToLibrary()
{
    curveSelectorWidget->OnAddPresetClicked();
}

void QGradientColorDialog::AddGradientToLibrary(QString name)
{
    gradientSelectorWidget->setObjectName(name);
    gradientSelectorWidget->SetPresetName(name);
    gradientSelectorWidget->OnAddPresetClicked();
}

void QGradientColorDialog::ExportLibrary(QString filepath)
{
    gradientSelectorWidget->SavePreset(filepath);
}

void QGradientColorDialog::OnChangeUpdateDisplayedGradient()
{
    QGradientStops stops;
    for (unsigned int i = 0; i < m_selectedGradient->width(); i++)
    {
        float time = i / float(m_selectedGradient->width());
        QColor color = gradientPickerWidget->GetHueAt(time);
        color.setAlpha(MIN(255, MAX(0, (int)(gradientPickerWidget->GetAlphaAt(time) * 255))));
        stops.push_back(QGradientStop(time, color));
    }
    m_selectedGradient->SetGradientStops(0, stops);
    gradientPickerWidget->update();
}

bool QGradientColorDialog::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == (QObject*)colorPickerButton)
    {
        if (event->type() == QEvent::ToolTip)
        {
            QHelpEvent* e = (QHelpEvent*)event;
            QString R, G, B;
            R.setNum(selectedColor.red());
            G.setNum(selectedColor.green());
            B.setNum(selectedColor.blue());

            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Gradient Editor", "Color", "R: " + R + "\tG: " + G + "\tB: " + B);

            m_tooltip->TryDisplay(e->globalPos(), colorPickerButton, QToolTipWidget::ArrowDirection::ARROW_RIGHT);

            return true;
        }
        if (event->type() == QEvent::Leave)
        {
            if (m_tooltip->isVisible())
            {
                m_tooltip->hide();
            }
        }
    }
    if (obj == (QObject*)colorSetLocation)
    {
        if (event->type() == QEvent::ToolTip)
        {
            QHelpEvent* e = (QHelpEvent*)event;

            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Gradient Editor", "Location");

            m_tooltip->TryDisplay(e->globalPos(), colorSetLocation, QToolTipWidget::ArrowDirection::ARROW_LEFT);

            return true;
        }
        if (event->type() == QEvent::Leave)
        {
            if (m_tooltip->isVisible())
            {
                m_tooltip->hide();
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}

void QGradientColorDialog::showEvent(QShowEvent* e)
{
    QDialog::showEvent(e);
    LoadSessionState();
}

void QGradientColorDialog::StoreSessionState()
{
    if (!sessionStateLoaded)
    {
        // If StoreSessionState() happens to be called before LoadSessionState(), it would clobber saved data.
        return;
    }

    QSettings settings("Amazon", "Lumberyard");
    QString group = "Gradient Editor/";
    QPoint _pos = window() ? window()->frameGeometry().topLeft() : QPoint();
    QSize _size = window() ? window()->size() : size();

    settings.beginGroup(group);
    settings.remove("");
    settings.sync();

    settings.setValue("pos", _pos);
    settings.setValue("size", _size);

    settings.endGroup();
    settings.sync();
}

void QGradientColorDialog::LoadSessionState()
{
    QSettings settings("Amazon", "Lumberyard");
    QString group = "Gradient Editor/";
    settings.beginGroup(group);
    QRect desktop = QApplication::desktop()->availableGeometry();

    if (QWidget* topLevel = window())
    {
        //decent start size = 475,450
        //start with a decent size, or last saved size
        topLevel->resize(settings.value("size", QSize(475, 450)).toSize());
        //start in a central location, or last saved pos
        QPoint pos = settings.value("pos", QPoint((desktop.width() - width()) / 2, (desktop.height() - height()) / 2)).toPoint();
        topLevel->move(pos);
    }

    settings.endGroup();
    settings.sync();

    sessionStateLoaded = true;
}

void QGradientColorDialog::hideEvent(QHideEvent* e)
{
    StoreSessionState();
    QDialog::hideEvent(e);
}

void QGradientColorDialog::OnSelectedGradientChanged(QGradientStops gradient)
{
    gradientPickerWidget->SetGradientStops(gradient);
    gradientPickerWidget->update();
    gradientSelectorWidget->SetGradientStops(gradient);
    gradientSelectorWidget->update();
    if ((bool)callback_gradient_changed)
    {
        callback_gradient_changed(gradient);
    }
    OnChangeUpdateDisplayedGradient();
}

void QGradientColorDialog::RemoveCallbacks()
{
    callback_content_changed = nullptr;
    callback_gradient_changed = nullptr;
}

void QGradientColorDialog::SetCallbackGradient(std::function<void(QGradientStops)> callback)
{
    callback_gradient_changed = callback;
}

void QGradientColorDialog::SetCallbackCurve(std::function<void(SCurveEditorContent)> callback)
{
    callback_content_changed = callback;
}

void QGradientColorDialog::UpdateColors(const QMap<QString, QColor>& colorMap)
{
    gradientPickerWidget->UpdateColors(colorMap);
}


void QGradientColorDialog::ShowGradientKeySettings(float uValue /*= 0*/, float vValue /*= 0*/)
{
    alphaKeyULabel->show();
    alphaKeyVLabel->show();
    alphaKeySetU->show();
    alphaKeySetV->show();
    alphaKeySetU->blockSignals(true);
    alphaKeySetU->setText(QString::number(uValue, 'f', QGRADIENTCOLORDIALOG_PRECISION));
    alphaKeySetU->blockSignals(false);
    alphaKeySetV->blockSignals(true);
    alphaKeySetV->setText(QString::number(vValue, 'f', QGRADIENTCOLORDIALOG_PRECISION));
    alphaKeySetV->blockSignals(false);
}

void QGradientColorDialog::HideGradientKeySettings()
{
    alphaKeyULabel->hide();
    alphaKeyVLabel->hide();
    alphaKeySetU->hide();
    alphaKeySetV->hide();
}


void QGradientColorDialog::ShowHueKeySettings(int location /*= 100*/)
{
    localtionLabel->show();
    colorSetLocation->show();
    colorSetLocation->blockSignals(true);
    colorSetLocation->setText(QString("%1").arg(location));
    colorSetLocation->blockSignals(false);
    percentLabel->show();
}

void QGradientColorDialog::HideHueKeySettings()
{
    localtionLabel->hide();
    colorSetLocation->hide();
    percentLabel->hide();
}

void QGradientColorDialog::AlphaKeyEditingFinished(QAmazonLineEdit* alphaKeyLabel)
{
    //only update position if text is not empty
    if (alphaKeyLabel->text().isEmpty())
    {
        while (alphaKeyLabel->isUndoAvailable())
        {
            alphaKeyLabel->undo();
        }
    }
    //clamp the value and update location text
    SetSelectedCurveKeyPosition();
    colorSetLocation->clearFocus();
    m_tooltip->hide();
}


void QGradientColorDialog::SetSelectedCurveKeyPosition()
{
    //clamp the value and update location text
    float uVal = qMax(0.f, qMin(1.f, alphaKeySetU->text().toFloat()));
    float vVal = qMax(0.f, qMin(1.f, alphaKeySetV->text().toFloat()));
    alphaKeySetU->blockSignals(true);
    alphaKeySetU->setText(QString::number(uVal, 'f', QGRADIENTCOLORDIALOG_PRECISION));
    alphaKeySetU->blockSignals(false);
    alphaKeySetV->blockSignals(true);
    alphaKeySetV->setText(QString::number(vVal, 'f', QGRADIENTCOLORDIALOG_PRECISION));
    alphaKeySetV->blockSignals(false);

    gradientPickerWidget->SetSelectedCurveKeyPosition(uVal, vVal);
}


#include "VariableWidgets/QGradientColorDialog.moc"
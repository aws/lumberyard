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
#include "EditorUI_QT_Precompiled.h"
#include "QColorDescriptorWidget.h"

#ifdef EDITOR_QT_UI_EXPORTS
#include <VariableWidgets/QColorDescriptorWidget.moc>
#endif
#include "qcoreevent.h"
#include "../AttributeView.h"
#include "IEditorParticleUtils.h"
#include "../Utils.h"

void QColorDescriptorWidget::InitHueRanges()
{
    if (!hueRanges.isEmpty())
    {
        hueRanges.clear();
    }
    hueRanges.push_back(HueRange(0, 359, tr("Channels")));
    hueRanges.push_back(HueRange(321, 40, tr("Reds")));
    hueRanges.push_back(HueRange(41, 80, tr("Yellows")));
    hueRanges.push_back(HueRange(61, 169, tr("Greens")));
    hueRanges.push_back(HueRange(141, 220, tr("Cyans")));
    hueRanges.push_back(HueRange(201, 280, tr("Blues")));
    hueRanges.push_back(HueRange(241, 330, tr("Magentas")));

    hueRangeComboBox.clear();
    for (unsigned int i = 0; i < 7; i++)
    {
        hueRangeComboBox.addItem(hueRanges[i].name);
    }
}

QColorDescriptorWidget::QColorDescriptorWidget(QWidget* parent)
    : QWidget(parent)
    , hueRangeComboBox(this)
    , hueSlider(this)
    , satSlider(this)
    , lightSlider(this)
    , alphaSlider(this)
    , hueLabel(this)
    , satLabel(this)
    , lightLabel(this)
    , alphaLabel(this)
    , satEdit(this)
    , lightEdit(this)
    , alphaEdit(this)
    , layout(this)
{
    InitHueRanges();
    setContentsMargins(0, 0, 0, 0);
    SetLabelIcon(hueLabel, tr("hue_ico.png"));
    SetLabelIcon(satLabel, tr("saturation_ico.png"));
    SetLabelIcon(lightLabel, tr("lighting_ico.png"));
    SetLabelIcon(alphaLabel, tr("alpha_ico.png"));

    SetHueSliderBackgroundGradient(0);
    SetSatSliderBackgroundGradient(satSlider, 0, 255);
    SetLumSliderBackgroundGradient(lightSlider, 0, 255);
    SetAlphaSliderBackgroundGradient(alphaSlider, 0, 255);

    hueSlider.setValue(180);
    satSlider.setValue(127);
    lightSlider.setValue(127);
    alphaSlider.setValue(0);
    currentColor.setHsl(hueSlider.value(), satSlider.value(), lightSlider.value(), alphaSlider.value());
    OnColorChanged();

    layout.addWidget(&hueRangeComboBox, 0, 0, 1, 5);

    hueSlider.setOrientation(Qt::Orientation::Horizontal);
    satSlider.setOrientation(Qt::Orientation::Horizontal);
    lightSlider.setOrientation(Qt::Orientation::Horizontal);
    alphaSlider.setOrientation(Qt::Orientation::Horizontal);

    hueSlider.setFixedHeight(GetSliderHeight());
    satSlider.setFixedHeight(GetSliderHeight());
    lightSlider.setFixedHeight(GetSliderHeight());
    alphaSlider.setFixedHeight(GetSliderHeight());

    layout.addWidget(&hueLabel, 1, 0, 1, 1);
    layout.addWidget(&hueSlider, 1, 1, 1, 4);
    layout.setColumnStretch(1, 5);

    layout.addWidget(&satLabel, 2, 0, 1, 1);
    layout.addWidget(&satSlider, 2, 1, 1, 2);
    layout.addWidget(&satEdit, 2, 3, 1, 2);

    layout.addWidget(&lightLabel, 3, 0, 1, 1);
    layout.addWidget(&lightSlider, 3, 1, 1, 2);
    layout.addWidget(&lightEdit, 3, 3, 1, 2);

    layout.addWidget(&alphaLabel, 4, 0, 1, 1);
    layout.addWidget(&alphaSlider, 4, 1, 1, 2);
    layout.addWidget(&alphaEdit, 4, 3, 1, 2);

    connect(&hueSlider, SIGNAL(valueChanged(int)), this, SLOT(OnHueSliderChange()));
    connect(&satSlider, SIGNAL(valueChanged(int)), this, SLOT(OnSatSliderChange()));
    connect(&lightSlider, SIGNAL(valueChanged(int)), this, SLOT(OnLightSliderChange()));
    connect(&alphaSlider, SIGNAL(valueChanged(int)), this, SLOT(OnAlphaSliderChange()));


    connect(&satEdit, SIGNAL(textEdited(QString)), this, SLOT(OnSatEditChange(QString)));
    connect(&lightEdit, SIGNAL(textEdited(QString)), this, SLOT(OnLightEditChange(QString)));
    connect(&alphaEdit, SIGNAL(textEdited(QString)), this, SLOT(OnAlphaEditChange(QString)));

    connect(&hueRangeComboBox, SIGNAL(activated(int)), this, SLOT(OnHueRangeChanged(int)));

    hueRangeComboBox.installEventFilter(this);
    hueLabel.installEventFilter(this);
    hueSlider.installEventFilter(this);

    satLabel.installEventFilter(this);
    satEdit.installEventFilter(this);
    satSlider.installEventFilter(this);

    lightLabel.installEventFilter(this);
    lightEdit.installEventFilter(this);
    lightSlider.installEventFilter(this);

    alphaLabel.installEventFilter(this);
    alphaEdit.installEventFilter(this);
    alphaSlider.installEventFilter(this);

    satEdit.setAlignment(Qt::AlignRight);
    alphaEdit.setAlignment(Qt::AlignRight);
    lightEdit.setAlignment(Qt::AlignRight);

    setLayout(&layout);
    m_tooltip = new QToolTipWidget(this);

    hueSlider.setProperty("ColorSlider", true);
    satSlider.setProperty("ColorSlider", true);
    lightSlider.setProperty("ColorSlider", true);
    alphaSlider.setProperty("ColorSlider", true);

    connect(&satEdit, &QAmazonLineEdit::textChanged, this, [=](){m_tooltip->hide(); });
    connect(&lightEdit, &QAmazonLineEdit::textChanged, this, [=](){m_tooltip->hide(); });
    connect(&alphaEdit, &QAmazonLineEdit::textChanged, this, [=](){m_tooltip->hide(); });
}

void QColorDescriptorWidget::SetLabelIcon(QLabel& label, QString alias)
{
    QString labelStyleSheet = " background: url(:/particleQT/icons/";
    labelStyleSheet.append(alias);
    labelStyleSheet.append(") center no-repeat;");
    label.setStyleSheet(labelStyleSheet);
    label.setScaledContents(true);
    label.setFixedSize(20, 20);
    label.setMinimumSize(20, 20);
    label.setMaximumSize(20, 20);
}

void QColorDescriptorWidget::SetLabelSelected(QLabel& label, QString alias, bool selected)
{
    QString labelStyleSheet;
    if (selected)
    {
        labelStyleSheet = "background: rgb(23,23,23, 255)  url(:/particleQT/icons/";
    }
    else
    {
        labelStyleSheet = "background: url(:/particleQT/icons/";
    }
    labelStyleSheet.append(alias);
    labelStyleSheet.append(") center no-repeat;");
    label.setStyleSheet(labelStyleSheet);
    label.setScaledContents(true);
}

void QColorDescriptorWidget::SetHueSliderBackgroundGradient(int hueIndex)
{
    if (hueIndex < 0 || hueIndex > 7)
    {
        return;
    }

    int min = hueRanges[hueIndex].min;
    int max = hueRanges[hueIndex].max;

    if (max > 360 || min > 360)
    {
        return;
    }

    int w = max - min;
    if (max < min)
    {
        w = (max + 360) - min;
    }
    QString W;
    W.setNum(w);
    int nextStop = 0;

    hueSlider.blockSignals(true);
    hueSlider.setValue(0);
    hueSlider.setMinimum(0);
    hueSlider.setMaximum(w);
    hueSlider.blockSignals(false);

    //set the background color
    QString sliderStyle = "QSlider::groove:horizontal{background: qlineargradient(x1:0, y1:0, x2: 1, y2: 0";
    while (nextStop <= w)
    {
        QColor color;
        int currentHue = (min + nextStop) % 360;
        color.setHsl(currentHue, 255, 125, 255);
        QString ns;
        float fraction = (float)nextStop / (float)w;
        ns.setNum(fraction);
        sliderStyle.append(", stop: ");
        sliderStyle.append(ns);
        sliderStyle.append(" ");
        sliderStyle.append(color.name(QColor::HexArgb));
        nextStop += 1;
        if (nextStop > 360)
        {
            nextStop = 0;
        }
    }
    sliderStyle.append(");}\n");

    //set the handle
    sliderStyle.append("QSlider::handle:horizontal{image: url(:/particleQT/icons/slider_handle_ico.png);}");

    hueSlider.setStyleSheet(sliderStyle);
}

void QColorDescriptorWidget::SetSatSliderBackgroundGradient(QSlider& slider, int start, int end)
{
    int min = start;
    int max = end;
    int w = max - min;
    int nextStop = min;

    slider.setMinimum(min);
    slider.setMaximum(max);

    QString sliderStyle = "QSlider::groove:horizontal{background: qlineargradient(x1:0, y1:0, x2: 1, y2: 0";
    while (nextStop <= max)
    {
        QColor color;
        int h, s, l, a;
        currentColor.getHsl(&h, &s, &l, &a);

        color.setHsl(h, nextStop, l, 255);
        QString ns;
        float fraction = (float)nextStop / (float)max;
        ns.setNum(fraction);
        sliderStyle.append(", stop: ");
        sliderStyle.append(ns);
        sliderStyle.append(" ");
        sliderStyle.append(color.name(QColor::HexArgb));
        nextStop += w;
    }
    sliderStyle.append(");}\n");
    //set the handle
    sliderStyle.append("QSlider::handle:horizontal{image: url(:/particleQT/icons/slider_handle_ico.png);}");
    slider.setStyleSheet(sliderStyle);
}

void QColorDescriptorWidget::SetLumSliderBackgroundGradient(QSlider& slider, int start, int end)
{
    int min = start;
    int max = end;
    int w = max - min;
    int nextStop = min;

    slider.setMinimum(min);
    slider.setMaximum(max);

    QString sliderStyle = "QSlider::groove:horizontal{background: qlineargradient(x1:0, y1:0, x2: 1, y2: 0";
    while (nextStop <= max)
    {
        QColor color;
        int h, s, l, a;
        currentColor.getHsl(&h, &s, &l, &a);

        color.setHsl(h, s, nextStop, 255);
        QString ns;
        float fraction = (float)nextStop / (float)max;
        ns.setNum(fraction);
        sliderStyle.append(", stop: ");
        sliderStyle.append(ns);
        sliderStyle.append(" ");
        sliderStyle.append(color.name(QColor::HexArgb));
        nextStop += w;
    }
    sliderStyle.append(");}\n");
    //set the handle
    sliderStyle.append("QSlider::handle:horizontal{image: url(:/particleQT/icons/slider_handle_ico.png);}");
    slider.setStyleSheet(sliderStyle);
}

void QColorDescriptorWidget::SetAlphaSliderBackgroundGradient(QSlider& slider, int start, int end)
{
    int min = start;
    int max = end;
    int w = max - min;
    int nextStop = min;

    slider.setMinimum(min);
    slider.setMaximum(max);

    QString sliderStyle = "QSlider::groove:horizontal{border-image: url(:/particleQT/icons/alpha_bknd.png);}\n";
    //set the handle
    sliderStyle.append("QSlider::handle:horizontal{image: url(:/particleQT/icons/slider_handle_ico.png);}");
    slider.setStyleSheet(sliderStyle);
}

void QColorDescriptorWidget::OnColorChanged()
{
    qreal h, s, l, a, r, g, b;
    currentColor.getHslF(&h, &s, &l, &a);
    currentColor.getRgbF(&r, &g, &b, &a);
    int offset = hueRanges[hueRangeComboBox.currentIndex()].min;
    int as, al, aa;
    //converts 255 based colors to 0 to 100 based for ui display
    as = round(s * 100);
    al = round(l * 100);
    aa = round(a * 100);

    QString R, G, B, A, S, L;
    R.setNum(round(r * 255));
    G.setNum(round(g * 255));
    B.setNum(round(b * 255));
    A.setNum(aa);
    S.setNum(as);
    L.setNum(al);

    satEdit.blockSignals(true);
    lightEdit.blockSignals(true);
    alphaEdit.blockSignals(true);
    satSlider.blockSignals(true);
    lightSlider.blockSignals(true);
    alphaSlider.blockSignals(true);


    satEdit.setText(S);
    lightEdit.setText(L);
    alphaEdit.setText(A);

    satSlider.setValue(round(s * 255));
    lightSlider.setValue(round(l * 255));
    alphaSlider.setValue(round(a * 255));

    satEdit.blockSignals(false);
    lightEdit.blockSignals(false);
    alphaEdit.blockSignals(false);
    satSlider.blockSignals(false);
    lightSlider.blockSignals(false);
    alphaSlider.blockSignals(false);

    SetSatSliderBackgroundGradient(satSlider, 0, 255);
    SetLumSliderBackgroundGradient(lightSlider, 0, 255);
    SetAlphaSliderBackgroundGradient(alphaSlider, 0, 255);
    if ((bool)m_colorChangedCallback)
    {
        m_colorChangedCallback(currentColor);
    }
}

void QColorDescriptorWidget::OnHueSliderChange()
{
    const int offset = hueRanges[hueRangeComboBox.currentIndex()].min;
    const qreal currentHue = (offset + hueSlider.value()) % 360;
    qreal h, s, l, a;
    currentColor.getHslF(&h, &s, &l, &a);
    currentColor.setHslF(currentHue, s, l, a);
    if ((bool)m_hueChangedCallback)
    {
        m_hueChangedCallback(currentHue);
    }
    SetSatSliderBackgroundGradient(satSlider, 0, 255);
}

void QColorDescriptorWidget::OnSatSliderChange()
{
    int h, s, l, a;
    currentColor.getHsl(&h, &s, &l, &a);
    currentColor.setHsl(h, satSlider.value(), l, a);
    OnColorChanged();
}

void QColorDescriptorWidget::OnLightSliderChange()
{
    int h, s, l, a;
    currentColor.getHsl(&h, &s, &l, &a);
    currentColor.setHsl(h, s, lightSlider.value(), a);
    OnColorChanged();
}

void QColorDescriptorWidget::OnAlphaSliderChange()
{
    int h, s, l, a;
    currentColor.getHsl(&h, &s, &l, &a);
    currentColor.setHsl(h, s, l, alphaSlider.value());
    OnColorChanged();
}

void QColorDescriptorWidget::OnSatEditChange(const QString& text)
{
    int h, s, l, a;
    float val = satEdit.text().toInt();
    val *= 2.55f;
    val = round(val);
    if (val >= 0 && val <= 255)
    {
        currentColor.getHsl(&h, &s, &l, &a);
        currentColor.setHsl(h, val, l, a);
        OnColorChanged();
    }
}

void QColorDescriptorWidget::OnLightEditChange(const QString& text)
{
    int h, s, l, a;
    float val = lightEdit.text().toInt();
    val *= 2.55f;
    val = round(val);
    if (val >= 0 && val <= 255)
    {
        currentColor.getHsl(&h, &s, &l, &a);
        currentColor.setHsl(h, s, val, a);
        OnColorChanged();
    }
}

void QColorDescriptorWidget::OnAlphaEditChange(const QString& text)
{
    int h, s, l, a;
    float val = alphaEdit.text().toInt();
    val *= 2.55f;
    val = round(val);
    if (val >= 0 && val <= 255)
    {
        currentColor.getHsl(&h, &s, &l, &a);
        currentColor.setHsl(h, s, l, val);
        OnColorChanged();
    }
}

void QColorDescriptorWidget::SetColor(QColor color)
{
    if (currentColor != color)
    {
        currentColor = color;
        OnHueEditFinished(color);
        OnColorChanged();
    }
}

QColor QColorDescriptorWidget::GetColor()
{
    return currentColor;
}

QColorDescriptorWidget::~QColorDescriptorWidget()
{
}

void QColorDescriptorWidget::OnHueRangeChanged(int index)
{
    SetHueSliderBackgroundGradient(index);
    hueSlider.setValue(hueSlider.minimum());
    hueSlider.setSliderPosition(hueSlider.minimum());
    OnHueSliderChange();
}

void QColorDescriptorWidget::OnHueEditFinished(QColor color)
{
    if (color.hue() < hueRanges[hueRangeComboBox.currentIndex()].min
        || color.hue() > hueRanges[hueRangeComboBox.currentIndex()].max)// if hue is not in current hue range
    {
        hueRangeComboBox.setCurrentIndex(0);
    }
    int offset = hueRanges[hueRangeComboBox.currentIndex()].min;
    int currentHue = (color.hue() - offset) % 360;
    hueSlider.setValue(currentHue);
    OnHueSliderChange();
}

//This function is called when the color is set by other widget.
//We simply set the value but not trigger any signal to send the 
//color change event back.
//This will 1. eliminate the changes 2. avoid rounding error which 
//caused when we send the value back
void QColorDescriptorWidget::OnColorUpdatedNotified(QColor color)
{
    qreal h, s, l, a, r, g, b;
    
    currentColor.getHslF(&h, &s, &l, &a);
    currentColor.getRgbF(&r, &g, &b, &a);
    int ah, as, al, aa;
    //converts 255 based colors to 0 to 100 based for ui display
    ah = round(h * 360);
    as = round(s * 100);
    al = round(l * 100);
    aa = round(a * 100);


    if (ah < hueRanges[hueRangeComboBox.currentIndex()].min
        || ah > hueRanges[hueRangeComboBox.currentIndex()].max)// if hue is not in current hue range
    {
        hueRangeComboBox.setCurrentIndex(0);
    }
    int offset = hueRanges[hueRangeComboBox.currentIndex()].min;
    int currentHue = (ah - offset) % 360;
    
    QString R, G, B, A, S, L;
    R.setNum(round(r * 255));
    G.setNum(round(g * 255));
    B.setNum(round(b * 255));
    A.setNum(aa);
    S.setNum(as);
    L.setNum(al);

    QSignalBlocker blockerSatE(satEdit);
    QSignalBlocker blockerLightE(lightEdit);
    QSignalBlocker blockerAlphaE(alphaEdit);
    QSignalBlocker blockerSatS(satSlider);
    QSignalBlocker blockerLightS(lightSlider);
    QSignalBlocker blockerAlphaS(alphaSlider);
    QSignalBlocker blocker1(hueSlider);
    
    
    hueSlider.setValue(currentHue);

    satEdit.setText(S);
    lightEdit.setText(L);
    alphaEdit.setText(A);

    satSlider.setValue(round(s * 255));
    lightSlider.setValue(round(l * 255));
    alphaSlider.setValue(round(a * 255));

    SetSatSliderBackgroundGradient(satSlider, 0, 255);
    SetLumSliderBackgroundGradient(lightSlider, 0, 255);
    SetAlphaSliderBackgroundGradient(alphaSlider, 0, 255);
}

bool QColorDescriptorWidget::eventFilter(QObject* obj, QEvent* e)
{
    QRect widgetRect = QRect(QPoint(0, 0), QSize(0, 0));
    QToolTipWidget::ArrowDirection arrowDir = QToolTipWidget::ArrowDirection::ARROW_RIGHT;
    if (e->type() == QEvent::ToolTip)
    {
        QHelpEvent* event = (QHelpEvent*)e;
        QPoint ep = event->globalPos();
        #pragma region Hue tooltips
        if (obj == static_cast<QObject*>(&hueLabel))
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "ColorDescriptorWidget.Hue", "Label");
            widgetRect.setTopLeft(mapToGlobal(hueLabel.pos()));
            widgetRect.setSize(hueLabel.size());
            arrowDir = QToolTipWidget::ArrowDirection::ARROW_LEFT;
        }
        else if (obj == static_cast<QObject*>(&hueSlider))
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "ColorDescriptorWidget.Hue", "Slider");
            widgetRect.setTopLeft(mapToGlobal(hueSlider.pos()));
            arrowDir = QToolTipWidget::ArrowDirection::ARROW_LEFT;
            widgetRect.setSize(hueSlider.size());
        }
        else if (obj == static_cast<QObject*>(&hueRangeComboBox))
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "ColorDescriptorWidget.Hue", "ComboBox");
            widgetRect.setTopLeft(mapToGlobal(hueRangeComboBox.pos()));
            arrowDir = QToolTipWidget::ArrowDirection::ARROW_UP;
            widgetRect.setSize(hueRangeComboBox.size());
        }
        #pragma endregion Hue tooltips
        #pragma region saturation tooltips
        else if (obj == static_cast<QObject*>(&satLabel))
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "ColorDescriptorWidget.Sat", "Label");
            widgetRect.setTopLeft(mapToGlobal(satLabel.pos()));
            arrowDir = QToolTipWidget::ArrowDirection::ARROW_LEFT;
            widgetRect.setSize(satLabel.size());
        }
        if (obj == static_cast<QObject*>(&satEdit))
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "ColorDescriptorWidget.Sat", "Edit");
            widgetRect.setTopLeft(mapToGlobal(satEdit.pos()));
            widgetRect.setSize(satEdit.size());
        }
        if (obj == static_cast<QObject*>(&satSlider))
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "ColorDescriptorWidget.Sat", "Slider");
            widgetRect.setTopLeft(mapToGlobal(satSlider.pos()));
            arrowDir = QToolTipWidget::ArrowDirection::ARROW_LEFT;
            widgetRect.setSize(satSlider.size());
        }
        #pragma endregion saturation tooltips
        #pragma region lightness tooltips
        if (obj == static_cast<QObject*>(&lightLabel))
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "ColorDescriptorWidget.Light", "Label");
            widgetRect.setTopLeft(mapToGlobal(lightLabel.pos()));
            arrowDir = QToolTipWidget::ArrowDirection::ARROW_LEFT;
            widgetRect.setSize(lightLabel.size());
        }
        if (obj == static_cast<QObject*>(&lightEdit))
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "ColorDescriptorWidget.Light", "Edit");
            widgetRect.setTopLeft(mapToGlobal(lightEdit.pos()));
            widgetRect.setSize(lightEdit.size());
        }
        if (obj == static_cast<QObject*>(&lightSlider))
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "ColorDescriptorWidget.Light", "Slider");
            widgetRect.setTopLeft(mapToGlobal(lightSlider.pos()));
            arrowDir = QToolTipWidget::ArrowDirection::ARROW_LEFT;
            widgetRect.setSize(lightSlider.size());
        }
        #pragma endregion lightness tooltips
        #pragma region alpha tooltips
        if (obj == static_cast<QObject*>(&alphaLabel))
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "ColorDescriptorWidget.Alpha", "Label");
            widgetRect.setTopLeft(mapToGlobal(alphaLabel.pos()));
            arrowDir = QToolTipWidget::ArrowDirection::ARROW_LEFT;
            widgetRect.setSize(alphaLabel.size());
        }
        if (obj == static_cast<QObject*>(&alphaEdit))
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "ColorDescriptorWidget.Alpha", "Edit");
            widgetRect.setTopLeft(mapToGlobal(alphaEdit.pos()));
            widgetRect.setSize(alphaEdit.size());
        }
        if (obj == static_cast<QObject*>(&alphaSlider))
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "ColorDescriptorWidget.Alpha", "Slider");
            widgetRect.setTopLeft(mapToGlobal(alphaSlider.pos()));
            arrowDir = QToolTipWidget::ArrowDirection::ARROW_LEFT;
            widgetRect.setSize(alphaSlider.size());
        }
        #pragma endregion alpha tooltips

        m_tooltip->TryDisplay(ep, widgetRect, arrowDir);

        return true;
    }
    if (e->type() == QEvent::Leave)
    {
        if (m_tooltip)
        {
            m_tooltip->hide();
        }
    }

    if (e->type() == QEvent::FocusIn)
    {
        if (obj == (QObject*)&hueLabel || obj == (QObject*)&hueSlider)
        {
            SetLabelSelected(hueLabel, tr("hue_ico.png"), true);
            SetLabelSelected(satLabel, tr("saturation_ico.png"), false);
            SetLabelSelected(lightLabel, tr("lighting_ico.png"), false);
            SetLabelSelected(alphaLabel, tr("alpha_ico.png"), false);
        }
        if (obj == (QObject*)&satLabel || obj == (QObject*)&satSlider || obj == (QObject*)&satEdit)
        {
            SetLabelSelected(hueLabel, tr("hue_ico.png"), false);
            SetLabelSelected(satLabel, tr("saturation_ico.png"), true);
            SetLabelSelected(lightLabel, tr("lighting_ico.png"), false);
            SetLabelSelected(alphaLabel, tr("alpha_ico.png"), false);
        }
        if (obj == (QObject*)&lightLabel || obj == (QObject*)&lightSlider || obj == (QObject*)&lightEdit)
        {
            SetLabelSelected(hueLabel, tr("hue_ico.png"), false);
            SetLabelSelected(satLabel, tr("saturation_ico.png"), false);
            SetLabelSelected(lightLabel, tr("lighting_ico.png"), true);
            SetLabelSelected(alphaLabel, tr("alpha_ico.png"), false);
        }
        if (obj == (QObject*)&alphaLabel || obj == (QObject*)&alphaSlider || obj == (QObject*)&alphaEdit)
        {
            SetLabelSelected(hueLabel, tr("hue_ico.png"), false);
            SetLabelSelected(satLabel, tr("saturation_ico.png"), false);
            SetLabelSelected(lightLabel, tr("lighting_ico.png"), false);
            SetLabelSelected(alphaLabel, tr("alpha_ico.png"), true);
        }
    }


    return QWidget::eventFilter(obj, e);
}

void QColorDescriptorWidget::setAlphaEnabled(bool allowAlpha)
{
    if (!allowAlpha)
    {
        alphaEdit.text() = 100;
        alphaSlider.setValue(alphaSlider.maximum());
        alphaEdit.hide();
        alphaSlider.hide();
        alphaLabel.hide();
    }
    else
    {
        alphaSlider.show();
        alphaEdit.show();
        alphaLabel.show();
    }
    alphaEdit.setEnabled(allowAlpha);
    alphaSlider.setEnabled(allowAlpha);
    m_allowAlpha = allowAlpha;
}

int QColorDescriptorWidget::GetSliderHeight() const
{
    return 20;
}

void QColorDescriptorWidget::SetHue(int hue)
{
    hueRangeComboBox.setCurrentIndex(0);
    SetHueSliderBackgroundGradient(0);
    int offset = hueRanges[hueRangeComboBox.currentIndex()].min;
    int currentHue = (hue - offset) % 360;
    hueSlider.setValue(currentHue);
    OnHueSliderChange();
}


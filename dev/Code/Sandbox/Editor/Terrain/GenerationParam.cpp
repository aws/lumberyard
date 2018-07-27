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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"
#include "GenerationParam.h"
#include "Noise.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <Terrain/ui_GenerationParam.h>

/////////////////////////////////////////////////////////////////////////////
// CGenerationParam dialog


CGenerationParam::CGenerationParam(QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_previewDelegate(nullptr)
    , m_previewMiniX(0)
    , m_previewMiniY(0)
    , m_previewMiniZoom(0)
    , ui(new Ui::CGenerationParam)
{
    ui->setupUi(this);

    connect(ui->m_sldPasses, &QSlider::valueChanged, this, [this](int value)
        {
            // Passes
            ui->m_labelPasses->setText(QString::number(value));
            UpdatePreview();
        });

    connect(ui->m_sldFrequency, &QSlider::valueChanged, this, [this](int value)
        {
            // Frequency
            ui->m_labelFrequency->setText(QString::number(static_cast<double>(value) / 10.0f, 'f'));
            UpdatePreview();
        });

    connect(ui->m_sldFrequencyStep, &QSlider::valueChanged, this, [this](int value)
        {
            // Frequency step
            ui->m_labelFrequencyStep->setText(QString::number(static_cast<double>(value) / 10.0f, 'f'));
            UpdatePreview();
        });

    connect(ui->m_sldFade, &QSlider::valueChanged, this, [this](int value)
        {
            // Fade
            ui->m_labelFade->setText(QString::number(static_cast<double>(value) / 10.0f, 'f'));
            UpdatePreview();
        });

    connect(ui->m_sldRandomBase, &QSlider::valueChanged, this, [this](int value)
        {
            // Random base
            ui->m_labelRandomBase->setText(QString::number(value));
            UpdatePreview();
        });

    connect(ui->m_sldSharpness, &QSlider::valueChanged, this, [this](int value)
        {
            // Sharpness
            ui->m_labelSharpness->setText(QString::number(static_cast<double>(value) / 1000.0f, 'f'));
            UpdatePreview();
        });

    connect(ui->m_sldBlur, &QSlider::valueChanged, this, [this](int value)
        {
            // Blurring
            ui->m_labelBlur->setText(QString::number(value));
            UpdatePreview();
        });

    connect(ui->m_chkPreview, &QCheckBox::stateChanged, this, &CGenerationParam::OnShowPreviewChanged);
    OnShowPreviewChanged();

    connect(ui->m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    ui->m_previewTexture->installEventFilter(this);
    ui->m_previewLODs->installEventFilter(this);
}

CGenerationParam::~CGenerationParam()
{
}

bool CGenerationParam::eventFilter(QObject* object, QEvent* event)
{
    if (object == ui->m_previewTexture || object == ui->m_previewLODs)
    {
        switch (event->type())
        {
        case QEvent::Paint:
        {
            QWidget* panel = static_cast<QWidget*>(object);

            QPainter painter(panel);

            const QRect targetExtents = panel->rect();

            if (m_previewImage.isNull() || !ui->m_chkPreview->isChecked())
            {
                painter.fillRect(targetExtents, Qt::black);
                return true;
            }

            const uint32 rawWidth = m_previewImage.width();
            const uint32 rawHeight = m_previewImage.height();

            QRect sourceExtents;

            if (object == ui->m_previewTexture)
            {
                // Full image view...
                sourceExtents = QRect(0, 0, rawWidth, rawHeight);
            }
            else
            {
                // Mini view...
                const QSize panelExtents = panel->size();
                const float zoom = (m_previewMiniZoom > 0) ? (1.0f / m_previewMiniZoom) : (1 << -m_previewMiniZoom);
                const int offsetX = min<int>(panelExtents.width() / 2 * zoom, rawWidth / 2);
                const int offsetY = min<int>(panelExtents.height() / 2 * zoom, rawHeight / 2);
            const int centerX = clamp_tpl<int>(static_cast<int>(m_previewMiniX*rawWidth), offsetX, rawWidth - offsetX);
            const int centerY = clamp_tpl<int>(static_cast<int>(m_previewMiniY*rawHeight), offsetY, rawHeight - offsetY);

                sourceExtents = QRect(centerX - offsetX, centerY - offsetY, offsetX * 2, offsetY * 2);
            }

            painter.drawImage(targetExtents, m_previewImage, sourceExtents);

            return true;
        }

        case QEvent::MouseMove:
        {
            if (object == ui->m_previewTexture)
            {
                QPoint pos = static_cast<QMouseEvent*>(event)->pos();
                QSize extents = ui->m_previewTexture->size();

                m_previewMiniX = pos.x() / static_cast<float>(extents.width());
                m_previewMiniY = pos.y() / static_cast<float>(extents.height());

                ui->m_previewLODs->update();

                return true;
            }
        }
        }
    }

    return false;
}

void CGenerationParam::UpdatePreview()
{
    // We only need to update the preview if it is checked, since it will be
    // hidden otherwise
    if (m_previewDelegate && ui->m_chkPreview->isChecked())
    {
        SNoiseParams noise;
        FillParam(noise);
        m_previewDelegate->SetNoise(noise);

        m_previewImage = m_previewDelegate->GetImage();

        ui->m_previewTexture->update();
        ui->m_previewLODs->update();
    }
}

void CGenerationParam::OnShowPreviewChanged()
{
    bool checked = ui->m_chkPreview->isChecked();
    ui->m_previewTexture->setVisible(checked);
    ui->m_previewLODs->setVisible(checked);
    UpdatePreview();
}

void CGenerationParam::FillParam(SNoiseParams& pParam) const
{
    pParam.iPasses = ui->m_sldPasses->value();
    pParam.fFrequencyStep = ui->m_sldFrequencyStep->value() / 10.0f;
    pParam.fFrequency = ui->m_sldFrequency->value() / 10.0f;
    pParam.iSmoothness = ui->m_sldBlur->value();
    pParam.iRandom = ui->m_sldRandomBase->value();
    pParam.fFade = ui->m_sldFade->value() / 10.0f;
    pParam.iSharpness = ui->m_sldSharpness->value() / 1000.0f;
}

void CGenerationParam::LoadParam(const SNoiseParams& pParam)
{
    ui->m_sldFade->setValue(static_cast<int>(pParam.fFade * 10.0f));
    ui->m_sldFrequency->setValue(static_cast<int>(pParam.fFrequency * 10.0f));
    ui->m_sldFrequencyStep->setValue(static_cast<int>(pParam.fFrequencyStep * 10.0f));
    ui->m_sldPasses->setValue(pParam.iPasses);
    ui->m_sldRandomBase->setValue(pParam.iRandom);
    ui->m_sldSharpness->setValue(static_cast<int>(pParam.iSharpness * 1000.0f));
    ui->m_sldBlur->setValue(pParam.iSmoothness);
}

void CGenerationParam::wheelEvent(QWheelEvent* event)
{
    m_previewMiniZoom = clamp_tpl(m_previewMiniZoom + (event->angleDelta().y() / WHEEL_DELTA), -3, 3);
    ui->m_previewLODs->update();

    QDialog::wheelEvent(event);
}

void CGenerationParam::SetPreviewDelegate(NoiseGenerator::PreviewDelegate* previewDelegate)
{
    m_previewDelegate = previewDelegate;
}

#include <Terrain/GenerationParam.moc>

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

#ifndef CRYINCLUDE_EDITOR_TERRAIN_GENERATIONPARAM_H
#define CRYINCLUDE_EDITOR_TERRAIN_GENERATIONPARAM_H
#pragma once

#include <QDialog>

namespace Ui {
    class CGenerationParam;
}

class QImage;

struct SNoiseParams;

namespace NoiseGenerator
{
    //! This is a delegate interface that allows the CGenerationParam dialog to
    //! render a preview of the current parameters.
    class PreviewDelegate
    {
    public:
        //! Called by the dialog when new noise parameters have been set.
        //! \param noise Current noise parameters.
        //! Note: iWidth, iHeight, bBlueSky and bValid will not be set.
        virtual void SetNoise(const SNoiseParams& noise) = 0;

        //! Called by the dialog to get the preview image to be rendered.
        //! \returns Image data created by the delegate.
        virtual QImage GetImage() = 0;
    };
}


/////////////////////////////////////////////////////////////////////////////
// CGenerationParam dialog

class CGenerationParam
    : public QDialog
{
    Q_OBJECT

public:
    CGenerationParam(QWidget* pParent = nullptr);
    ~CGenerationParam();

    //! Set the preview delegate
    //! \param previewDelegate An instance of a PreviewDelegate or nullptr.
    //! Note: Should be set before DoModal() and the delegate must live at
    //! least until the dialog closes.
    void SetPreviewDelegate(NoiseGenerator::PreviewDelegate* previewDelegate);

    //! Set the current positions of the sliders in the dialog
    //! \param pParam New noise parameters.
    //! Note: Ignores iWidth, iHeight, bBlueSky and bValid
    void LoadParam(const SNoiseParams& pParam);

    //! Retrieve the current slider positions as a noise structure.
    //! Note: Does not set iWidth, iHeight, bBlueSky or bValid
    void FillParam(SNoiseParams& pParam) const;

    // Implementation
protected:
    NoiseGenerator::PreviewDelegate* m_previewDelegate;
    QImage m_previewImage;

    // Normalized X,Y for the mini-preview
    float m_previewMiniX, m_previewMiniY;

    // Zoom for the mini-preview (-ve = out, +ve = in)
    int m_previewMiniZoom;

    // Updates the preview image
    void UpdatePreview();
    void OnShowPreviewChanged();

    // Update the static number controls with the values from the sliders
    void UpdateStaticNum();

    void OnInitDialog();

    bool eventFilter(QObject* object, QEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

    QScopedPointer<Ui::CGenerationParam> ui;
};

#endif // CRYINCLUDE_EDITOR_TERRAIN_GENERATIONPARAM_H

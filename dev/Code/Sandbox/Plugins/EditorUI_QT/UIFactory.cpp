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

#include "UIFactory.h"

//Controls
#include "AttributeView.h"
#include "VariableWidgets/QGradientColorDialog.h"
#include "VariableWidgets/QKeySequenceEditorDialog.h"
#include "CurveEditorContent.h"
#include "qbrush.h"
#include "VariableWidgets/QCustomColorDialog.h"
#include "VariableWidgets/QColorEyeDropper.h"
#include "PreviewWindowView.h"

QGradientColorDialog* UIFactory::s_GradientColorDialog = nullptr;
QCustomColorDialog* UIFactory::s_CustomColorDialog = nullptr;
QKeySequenceEditorDialog* UIFactory::s_KeySequenceEditorDialog = nullptr;
QColorEyeDropper* UIFactory::s_ColorEyeDropper = nullptr;
QTUIEditorSettings* UIFactory::s_UISettings = nullptr;

void UIFactory::Initialize()
{
    SetupGradientEditor();
    SetupHotkeyEditor();
    SetupColorPicker();
    SetupQTUISettings();
}

void UIFactory::Deinitialize()
{
    delete s_GradientColorDialog;
    delete s_CustomColorDialog;
    delete s_KeySequenceEditorDialog;
    delete s_UISettings;

    s_GradientColorDialog = nullptr;
    s_CustomColorDialog = nullptr;
    s_KeySequenceEditorDialog = nullptr;
    s_UISettings = nullptr;
}

//Creation
CAttributeView* UIFactory::createAttributeView(QWidget* parent)
{
    return new CAttributeView(parent);
}

CPreviewWindowView* UIFactory::createPreviewWindowView(QWidget* parent)
{
    return new CPreviewWindowView(parent);
}

QGradientColorDialog* UIFactory::GetGradientEditor(SCurveEditorContent content, QGradientStops stops)
{
    CRY_ASSERT(s_GradientColorDialog);
    s_GradientColorDialog->OnSelectedGradientChanged(stops);
    if (content.m_curves.size() > 0)
    {
        s_GradientColorDialog->OnCurveChanged(content.m_curves.back());
    }
    return s_GradientColorDialog;
}
QGradientColorDialog* UIFactory::GetGradientEditor()
{
    CRY_ASSERT(s_GradientColorDialog);
    return s_GradientColorDialog;
}

QCustomColorDialog* UIFactory::GetColorPicker(QColor startColor, bool allowAlpha /* = true*/)
{
    CRY_ASSERT(s_CustomColorDialog);
    s_CustomColorDialog->SignalEndColorEditing(true);
    s_CustomColorDialog->OnSelectedColorChanged(startColor, true);
    s_CustomColorDialog->setAlphaEnabled(allowAlpha);
    s_CustomColorDialog->SetStartingHue(startColor.hue());
    return s_CustomColorDialog;
}

QColorEyeDropper* UIFactory::GetColorEyeDropper()
{
    CRY_ASSERT(s_ColorEyeDropper);
    return s_ColorEyeDropper;
}

QKeySequenceEditorDialog* UIFactory::GetHotkeyEditor()
{
    CRY_ASSERT(s_KeySequenceEditorDialog);
    s_KeySequenceEditorDialog->resetHotkeys();
    return s_KeySequenceEditorDialog;
}

QTUIEditorSettings* UIFactory::GetQTUISettings()
{
    CRY_ASSERT(s_UISettings);
    return s_UISettings;
}

void UIFactory::SetupQTUISettings()
{
    CRY_ASSERT(s_UISettings == nullptr);
    s_UISettings = new QTUIEditorSettings();
}


void UIFactory::SetupGradientEditor()
{
    CRY_ASSERT(s_GradientColorDialog == nullptr);
    s_GradientColorDialog = new QGradientColorDialog(nullptr);
}

void UIFactory::SetupHotkeyEditor()
{
    CRY_ASSERT(s_KeySequenceEditorDialog == nullptr);
    s_KeySequenceEditorDialog = new QKeySequenceEditorDialog(nullptr);
}

void UIFactory::SetupColorPicker()
{
    CRY_ASSERT(s_ColorEyeDropper == nullptr);
    s_ColorEyeDropper = new QColorEyeDropper(nullptr);
    CRY_ASSERT(s_CustomColorDialog == nullptr);
    s_CustomColorDialog = new QCustomColorDialog(QColor(255, 255, 255), s_ColorEyeDropper, nullptr);
}

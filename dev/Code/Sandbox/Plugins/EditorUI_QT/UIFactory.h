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
#pragma once
#ifndef CRYINCLUDE_EDITORUI_QT_UIFACTORY_H
#define CRYINCLUDE_EDITORUI_QT_UIFACTORY_H

#include "api.h"
#include <QPair>
#include <QVector>
#include <QColor>
#include "Utils.h"

class QWidget;
class CAttributeView;
class QCustomColorDialog;
class CPreviewWindowView;
class QGradientColorDialog;
class QColorEyeDropper;
struct SCurveEditorContent;
typedef QPair<qreal, QColor> QGradientStop;
typedef QVector<QGradientStop> QGradientStops;
class QKeySequenceEditorDialog;

class EDITOR_QT_UI_API UIFactory
{
public:
    static void Initialize(); //This will setup dialogs that can be shared between multiple plugins
    static void Deinitialize();

    static CAttributeView* createAttributeView(QWidget* parent);
    static CPreviewWindowView* createPreviewWindowView(QWidget* parent);
    static QGradientColorDialog* GetGradientEditor(SCurveEditorContent content, QGradientStops stops);
    static QGradientColorDialog* GetGradientEditor();
    static QCustomColorDialog* GetColorPicker(QColor startColor = Qt::white, bool allowAlpha = true);
    static QColorEyeDropper* GetColorEyeDropper();
    static QKeySequenceEditorDialog* GetHotkeyEditor();
    static QTUIEditorSettings* GetQTUISettings();

private:
    static void SetupGradientEditor();
    static void SetupColorPicker();
    static void SetupHotkeyEditor();
    static void SetupQTUISettings();

    static QGradientColorDialog* s_GradientColorDialog;
    static QCustomColorDialog* s_CustomColorDialog;
    static QKeySequenceEditorDialog* s_KeySequenceEditorDialog;
    static QColorEyeDropper* s_ColorEyeDropper;
    static QTUIEditorSettings* s_UISettings;
};

#endif // CRYINCLUDE_EDITORUI_QT_UIFACTORY_H
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

#ifndef CRYINCLUDE_PLUGINS_EDITORUI_QT_UTILS_H
#define CRYINCLUDE_PLUGINS_EDITORUI_QT_UTILS_H
#pragma once

#include "api.h"
struct IVariable;
class CAttributeItem;
class DockableLibraryPanel;
struct SCurveEditorKey;
class CBaseLibraryManager;

#include <QString>
#include <QVector>
#include <QRect>
#include <QPair>
#include <QSettings>
#include "Particles/ParticleItem.h"

#define QTUI_UTILS_NAME_SEPARATOR "&&"
#define PARTICLE_EDITOR_LAYOUT_IDENTIFIER 0x1227
// Updated the version number will need to updated the related save/load functions in AttributeView, PreviewWindowView, and QTUIEditorSettings
#define PARTICLE_EDITOR_LAYOUT_VERSION 0x3
#define UI_VAR_UNDO_TAG "UI Variable change"
#define PARTICLE_EDITOR_LAYOUT_SETTINGS_LOD "LayoutParticleEditorLod"
#define PARTICLE_EDITOR_LAYOUT_SETTINGS_ATTRIBUTES_OLD "LayoutParticleEditorProperties"
#define PARTICLE_EDITOR_LAYOUT_SETTINGS_ATTRIBUTES "LayoutParticleEditorAttributes"
#define DEFAULT_ATTRIBUTE_VIEW_LAYOUT_PATH "Editor/EditorParticlePanelsDefault.xml"
#define PARTICLE_EDITOR_PREVIEWER_SETTINGS "ParticleEditorPreviewer"

struct QTUIEditorSettings
{
    bool m_deletionPrompt;

    QTUIEditorSettings()
    {
        ResetToDefault();
    }

    void ResetToDefault()
    {
        m_deletionPrompt = true;
    }


    void LoadSetting(QByteArray data)
    {
        QDataStream stream(&data, QIODevice::ReadOnly);
        stream.setByteOrder(QDataStream::BigEndian);
        bool boolValue = false;

        quint32 fileVersion;
        stream >> fileVersion;
        switch (fileVersion)
        {
        case 0x1:
        case 0x2:
        case 0x3:
        {
            stream >> boolValue;
            m_deletionPrompt = boolValue;
            break;
        }
        default:
            break;
        }
    }

    void SaveSetting(QByteArray& dataout)
    {
        QDataStream stream(&dataout, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::BigEndian);
        quint32 fileVersion = PARTICLE_EDITOR_LAYOUT_VERSION;
        stream << fileVersion;
        stream << m_deletionPrompt;
    }
};

class EDITOR_QT_UI_API Utils
{
public:

    static void invoke(std::function<void()> func);
    static void runInvokeQueue();
};

template<typename T>
class FunctionCallExit
{
public:
    FunctionCallExit(const T& handler)
        : m_handler(handler)
    {

    }
    ~FunctionCallExit()
    {
        m_handler();
    }
private:
    T m_handler;
};


// Utility functions for qt recursion
template<typename T, typename T2>
void QtRecurseAll(T* item, const T2& handler)
{
    if (item == nullptr)
    {
        return;
    }

    handler(item);

    for (int i = 0; i < item->childCount(); i++)
    {
        QtRecurseAll(item->child(i), handler);
    }
}

template<typename T, typename T2>
void QtRecurseLeavesOnly(T* item, const T2& handler)
{
    if (item == nullptr)
    {
        return;
    }

    if (item->childCount() == 0)
    {
        handler(item);
    }

    for (int i = 0; i < item->childCount(); i++)
    {
        QtRecurseLeavesOnly(item->child(i), handler);
    }
}

template<typename T>
bool QtRecurseAnyMatch(T item, std::function<bool(T)> condition)
{
    if (item == nullptr)
    {
        return false;
    }

    if (condition(item))
    {
        return true;
    }

    for (int i = 0; i < item->childCount(); i++)
    {
        if (QtRecurseAnyMatch(item->child(i), condition))
        {
            return true;
        }
    }
    return false;
}

bool IsPointInRect(const QPoint& p, const QRect& r);

void EditorSetTangent(ISplineInterpolator* spline, const int keyId, const SCurveEditorKey* key, bool isInTangent);

#endif // CRYINCLUDE_PLUGINS_EDITORUI_QT_UTILS_H

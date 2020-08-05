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

#ifndef CRYINCLUDE_EDITOR_VISUALLOGVIEWER_VISUALLOGWND_H
#define CRYINCLUDE_EDITOR_VISUALLOGVIEWER_VISUALLOGWND_H
#pragma once

#include <QWidget>

#include <QScopedPointer>

#include <qwindowdefs.h>

class CVisualLogDialog;
class CVisualLogWnd;
struct SVLogCommonData;

// The view within the Visual Log Viewer window (CVisualLogWnd)
class CVLogImageView
    : public QWidget
{
    friend class CVisualLogWnd;
public: // Construction & destruction
    CVLogImageView(QWidget* parent = nullptr);
    virtual ~CVLogImageView();

public: // Operations
    void SetCommonData(SVLogCommonData* pCD) { m_pCD = pCD; }

public: // REIMPL
    QSize sizeHint() const override;

protected:
    void InitDestRect();
    void clearPreviousFrame() { m_nPrevFrame = -1; }
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:        // Member data
    QRect                       m_rcClient;
    QRect                       m_rcDest;
    int                         m_nPrevFrame;

    SVLogCommonData* m_pCD;
};



// The view within the CVLogTextPreview docking frame
class CVLogTextView
    : public QWidget
{
    friend class CVisualLogWnd;
public: // Construction & destruction
    CVLogTextView(QWidget* parent = nullptr);
    virtual ~CVLogTextView();

public: // Operations
    void SetCommonData(SVLogCommonData* pCD) { m_pCD = pCD; }

public: // REIMPL
    QSize sizeHint() const override;

protected:
    void ResetColumns();
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:        // Member data
    QRect                       m_rcClient;
    int                         m_nColumnWidth;
    QVector<int>                m_arrColHeights;

    SVLogCommonData* m_pCD;
};



// CVisualLogWnd frame
class CVisualLogWnd
    : public QWidget
{
    Q_OBJECT

public: // Construction & destruction
    explicit CVisualLogWnd(QWidget* parent = nullptr);
    virtual ~CVisualLogWnd();

private:
    bool ReadLogFiles();

protected slots:
    void OnFolderChanged();
    void OnFrameChanged();
    void RepaintIfNeeded();
    void RepaintTextView();
    void RepaintImageView();

public:     // Static members
    static const GUID& GetClassID();
    static void RegisterViewClass();

private:    // Member data
    QScopedPointer<CVLogImageView>   m_pView;               // The view within (captured images)
    QScopedPointer<CVLogTextView>    m_pTextView;           // The view within the text preview pane
    QScopedPointer<CVisualLogDialog> m_pControlsDlg;            // The dialog within the controls pane

private:    // Other data
    QScopedPointer<SVLogCommonData> m_pCD;
};

#endif // CRYINCLUDE_EDITOR_VISUALLOGVIEWER_VISUALLOGWND_H

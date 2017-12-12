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
#ifndef QKeySequenceCaptureWidget_h__
#define QKeySequenceCaptureWidget_h__

#include <QWidget>
#include <QApplication>
#include <QKeyEvent>
#include <QLabel>
#include "../ContextMenu.h"

class QKeySequenceCaptureWidget
    : public QLabel
{
    Q_OBJECT
    unsigned int m_flags;
    int keyInt;
    QKeySequence m_sequence;
    QKeySequence m_defaultSequence;
    QKeySequence m_lastKeySequence;
    QString m_path;
public:
    enum QKeySequenceCaptureFlags
    {
        CAPTURE_MODIFIERS = 1 << 1,
        DISPLAY_CAPTURE = 1 << 3
    };
    QKeySequenceCaptureWidget(QWidget* parent, QString path);
    QKeySequenceCaptureWidget(QWidget* parent, QString path, QString sequence);

    void SetFlags(unsigned int flags);

    QKeySequence GetSequence();
    QString GetPath();

    void SetSequence(QKeySequence sequence, bool asDefault);

    bool IsSameField(QString path);

    bool IsSameCategory(QString path);
    void SetCallbackOnFieldChanged(std::function<bool(QKeySequenceCaptureWidget*)>callback);
    bool IsKeyTaken(QKeySequence sequence);
    bool wasEdited() const;

    void Reset();

protected:
    void updateStyle();
    std::function<bool(QKeySequenceCaptureWidget*)> m_callback_on_field_changed;

    virtual void mousePressEvent(QMouseEvent* ev) override;

    virtual void keyPressEvent(QKeyEvent* event);

    virtual void focusInEvent(QFocusEvent*) override;

    virtual void focusOutEvent(QFocusEvent*) override;
    ContextMenu* m_menu;
};
#endif // QKeySequenceCaptureWidget_h__

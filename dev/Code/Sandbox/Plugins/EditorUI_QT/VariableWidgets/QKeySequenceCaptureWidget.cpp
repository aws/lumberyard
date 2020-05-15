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
#include "QKeySequenceCaptureWidget.h"

#include <QStyle>

void QKeySequenceCaptureWidget::focusInEvent(QFocusEvent* event)
{
    keyInt = 0;
    setProperty("HotkeyLabel", "Selected");
    updateStyle();
    m_lastKeySequence = m_sequence;
}

void QKeySequenceCaptureWidget::focusOutEvent(QFocusEvent* event)
{
    keyInt = 0;

    if (m_sequence != m_lastKeySequence)
    {
        if (m_callback_on_field_changed)
        {
            if (!m_callback_on_field_changed(this))
            {
                m_sequence = m_lastKeySequence;
                QString sequence = m_sequence.toString(QKeySequence::NativeText);
                setText((sequence.length() > 0) ? sequence : "Click to assign");
            }
        }
    }
    if (m_sequence == m_defaultSequence)
    {
        setProperty("HotkeyLabel", "Standard");
        updateStyle();
        return;
    }
    setProperty("HotkeyLabel", "Modified");
    updateStyle();
}

void QKeySequenceCaptureWidget::keyPressEvent(QKeyEvent* event)
{
    keyInt = event->key();

    Qt::Key key = static_cast<Qt::Key>(keyInt);
    if (key == Qt::Key_unknown)
    {
        return;
    }

    // the user have clicked just and only the special keys Ctrl, Shift, Alt, Meta.
    if (key == Qt::Key_Control ||
        key == Qt::Key_Shift ||
        key == Qt::Key_Alt ||
        key == Qt::Key_Meta)
    {
        //Use the modifiers here as the key int as that give the correct text.
        keyInt = 0;
    }

    // check for a combination of user clicks
    if (m_flags & CAPTURE_MODIFIERS)
    {
        Qt::KeyboardModifiers modifiers = event->modifiers();
        if (modifiers & Qt::ShiftModifier)
        {
            keyInt += Qt::SHIFT;
        }
        if (modifiers & Qt::ControlModifier)
        {
            keyInt += Qt::CTRL;
        }
        if (modifiers & Qt::AltModifier)
        {
            keyInt += Qt::ALT;
        }
        if (modifiers & Qt::MetaModifier)
        {
            keyInt += Qt::META;
        }
    }

    m_sequence = QKeySequence(keyInt);

    if (m_flags & DISPLAY_CAPTURE)
    {
        QString sequence = m_sequence.toString(QKeySequence::NativeText);
        setText(sequence);
    }
    return QWidget::keyPressEvent(event);
}

void QKeySequenceCaptureWidget::SetFlags(unsigned int flags)
{
    m_flags = flags;
}
QKeySequenceCaptureWidget::QKeySequenceCaptureWidget(QWidget* parent, QString path, QString sequence)
    : QLabel(parent)
{
    m_path = path;
    m_sequence = QKeySequence::fromString(sequence);
    m_lastKeySequence = m_defaultSequence = m_sequence;

    if (m_sequence.isEmpty())
    {
        setText("Click to assign");
    }
    else
    {
        setText(m_sequence.toString(QKeySequence::NativeText));
    }
    SetFlags(CAPTURE_MODIFIERS | DISPLAY_CAPTURE);
    setProperty("HotkeyLabel", "Standard");
    updateStyle();
    setFocusPolicy(Qt::FocusPolicy::StrongFocus);

    setFixedWidth(160);
    setAlignment(Qt::AlignRight);
    m_menu = new ContextMenu(this);
    QAction* action = m_menu->addAction("Clear");
    connect(action, &QAction::triggered, this, [=]() { SetSequence(QKeySequence(), false);  });
}

QKeySequenceCaptureWidget::QKeySequenceCaptureWidget(QWidget* parent, QString path)
    : QLabel(parent)
    , m_path(path)
{
    m_defaultSequence = m_sequence = QKeySequence();
    setText("Click to assign");
    SetFlags(CAPTURE_MODIFIERS | DISPLAY_CAPTURE);
    setProperty("HotkeyLabel", "Standard");
    updateStyle();
    setFocusPolicy(Qt::FocusPolicy::StrongFocus);

    setFixedWidth(160);
    setAlignment(Qt::AlignRight);
}

void QKeySequenceCaptureWidget::SetSequence(QKeySequence sequence, bool asDefault)
{
    m_sequence = sequence;
    if (asDefault)
    {
        m_lastKeySequence = m_defaultSequence = m_sequence;
    }
    if (m_flags & DISPLAY_CAPTURE)
    {
        if (!m_sequence.isEmpty())
        {
            QString _sequence = m_sequence.toString(QKeySequence::NativeText);
            setText(_sequence);
        }
        else
        {
            setText(tr("Click to assign"));
        }
    }
    if (asDefault || m_sequence == m_defaultSequence)
    {
        setProperty("HotkeyLabel", "Standard");
    }
    else
    {
        setProperty("HotkeyLabel", "Modified");
    }
    updateStyle();
}

void QKeySequenceCaptureWidget::Reset()
{
    m_sequence = m_defaultSequence;
}

bool QKeySequenceCaptureWidget::wasEdited() const
{
    return m_defaultSequence != m_sequence;
}

bool QKeySequenceCaptureWidget::IsKeyTaken(QKeySequence sequence)
{
    return sequence.toString(QKeySequence::PortableText).compare(m_sequence.toString(QKeySequence::PortableText)) == 0;
}

void QKeySequenceCaptureWidget::SetCallbackOnFieldChanged(std::function<bool(QKeySequenceCaptureWidget*)>callback)
{
    m_callback_on_field_changed = callback;
}

bool QKeySequenceCaptureWidget::IsSameCategory(QString path)
{
    QString m_cat = m_path.split('.').first();
    QString cat = path.split('.').first();
    return m_cat.compare(cat, Qt::CaseInsensitive) == 0;
}

bool QKeySequenceCaptureWidget::IsSameField(QString path)
{
    return m_path.compare(path, Qt::CaseInsensitive) == 0;
}

QKeySequence QKeySequenceCaptureWidget::GetSequence()
{
    return m_sequence;
}

QString QKeySequenceCaptureWidget::GetPath()
{
    return m_path;
}

void QKeySequenceCaptureWidget::mousePressEvent(QMouseEvent* ev)
{
    if (ev->button() == Qt::RightButton)
    {
        m_menu->exec(ev->globalPos());
    }
}

void QKeySequenceCaptureWidget::updateStyle()
{
    style()->unpolish(this);
    style()->polish(this);
    update();
}

#include <VariableWidgets/QKeySequenceCaptureWidget.moc>

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
#include "QCopyModalDialog.h"

//All needed for Clipboard.h
#include <ITexture.h>
#include <Util/Image.h>
#include <Clipboard.h>

#ifdef EDITOR_QT_UI_EXPORTS
#include <VariableWidgets/QCopyModalDialog.moc>
#endif

//For handling window moving
#include <qevent.h>

QCopyModalDialog::QCopyModalDialog(QWidget* parent /*= 0*/, QString paramName)
    : QDialog(parent)
    , layout(this)
    , cancelBtn(this)
    , replaceAllBtn(this)
    , closeBtn(this)
    , replaceParamOnlyBtn(this)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    cancelBtn.setText(tr("Cancel"));
    cancelBtn.setStyleSheet("border: none;");
    replaceAllBtn.setText(tr("Emitter Chain"));
    replaceParamOnlyBtn.setText(tr("Selected Emitter Only"));
    closeBtn.setText(tr(""));
    closeBtn.setStyleSheet(tr("QPushButton{background-image: url(:/particleQT/buttons/close_btn.png); background-position: center; background-repeat: no-repeat; border: none;}"));
    //Force button to be squre and fit the title bar area
    closeBtn.setFixedSize(QSize(BorderThickness, BorderThickness));
    parameterName = paramName;
    msg.setText(tr("Do you want to paste the selected emitter only or selected emitter chain?"));
    msg.setAlignment(Qt::AlignCenter);
    //Force first column to act as border by setting width (Using width with compensation as the cancel button has no visible border).
    layout.setColumnMinimumWidth(0, BorderWithPaddingCompensation);
    //Force last column to act as border by setting width
    layout.setColumnMinimumWidth(4, BorderThickness);
    //Force last row to act as border by setting height (Using size with compensation because of how qt pads button areas vertically).
    layout.setRowMinimumHeight(3, BorderWithPaddingCompensation);
    layout.addWidget(&closeBtn, 0, 4, 1, 1, Qt::AlignRight);
    layout.addWidget(&msg, 1, 1, 1, 3, Qt::AlignCenter);
    layout.addWidget(&replaceParamOnlyBtn, 2, 1, 1, 1, Qt::AlignLeft);
    layout.addWidget(&replaceAllBtn, 2, 2, 1, 1, Qt::AlignLeft);
    layout.addWidget(&cancelBtn, 2, 3, 1, 1, Qt::AlignLeft);
    replaceAll = false;
    connect(&closeBtn, &QPushButton::clicked, this, [=](){this->replaceAll = false; this->reject(); });
    connect(&cancelBtn, &QPushButton::clicked, this, [=](){this->replaceAll = false; this->reject(); });
    connect(&replaceAllBtn, &QPushButton::clicked, this, [=](){this->replaceAll = true; this->accept(); });
    connect(&replaceParamOnlyBtn, &QPushButton::clicked, this, [=](){this->replaceAll = false; this->accept(); });

	CClipboard clip(this);
	replaceAllBtn.setDisabled(clip.IsEmpty());
}

bool QCopyModalDialog::exec(bool& outReplaceAll)
{
    bool result = (QDialog::exec() == QDialog::Accepted);
    outReplaceAll = replaceAll;
    return result;
}

void QCopyModalDialog::mousePressEvent(QMouseEvent* mouse_event)
{
    m_pos = mouse_event->pos();
    m_moveWindow = true;
}

void QCopyModalDialog::mouseMoveEvent(QMouseEvent* mouse_event)
{
    if (!m_moveWindow)
    {
        return;
    }

    QPoint displacement = mouse_event->globalPos();
    move(displacement.x() - m_pos.x(), displacement.y() - m_pos.y());
}

void QCopyModalDialog::mouseReleaseEvent(QMouseEvent*)
{
    m_moveWindow = false;
}

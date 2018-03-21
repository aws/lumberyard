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

#ifndef QCopyModalDialog_h__
#define QCopyModalDialog_h__

#include "../api.h"

#include <QWidget>
#include <QDialog>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>

 class EDITOR_QT_UI_API QCopyModalDialog
    : public QDialog
{
    Q_OBJECT
public:
    QCopyModalDialog(QWidget* parent = 0, QString paramName = QString());
    //returns true if copy is happening, sets outReplaceAll true if replace all is pressed.
    bool exec(bool& outReplaceAll);
    using QDialog::exec;
    bool ShouldReplaceAll() { return replaceAll; }
protected:
    QGridLayout layout;
    QLabel      msg;
    QPushButton cancelBtn;
    QPushButton replaceAllBtn;
    QPushButton closeBtn;
    QPushButton replaceParamOnlyBtn;
    QString parameterName;
    bool exit;
    bool replaceAll;

    void mousePressEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);

private:
    static const int BorderThickness = 17;
    static const int BorderWithPaddingCompensation = 20;
    bool m_moveWindow;
    QPoint m_pos;
};
#endif // QCopyModalDialog_h__

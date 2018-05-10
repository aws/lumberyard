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

#ifndef QCopyableWidget_h__
#define QCopyableWidget_h__

#include <QWidget>

#include <functional>

#include "../api.h"

class EDITOR_QT_UI_API QCopyableWidget
    : public QWidget
{
    Q_OBJECT
public:
    enum COPY_MENU_FLAGS
    {
        COPY_MENU_DISABLED = 1 << 0,
        ENABLE_COPY = 1 << 1,
        ENABLE_PASTE = 1 << 2,
        ENABLE_DUPLICATE = 1 << 3,
        ENABLE_RESET = 1 << 4
    };
    QCopyableWidget(QWidget* parent = 0);

    //Does not check for existing options in menu, call in order you want them. Also doesn't check for similar items, please clear and rebuild before changing menu.
    void SetCopyMenuFlags(COPY_MENU_FLAGS flags);
    COPY_MENU_FLAGS GetCopyMenuFlags() { return m_flags; }

    void SetCustomMenuCreationCallback(std::function<void(QMenu*)> custom);
    std::function<void(QMenu*)> GetCustomMenuCreationCallback();

    void SetCopyCallback(std::function<void()> callback);
    void SetPasteCallback(std::function<void()> callback);
    void SetCheckPasteCallback(std::function<bool()> callback);
    void SetDuplicateCallback(std::function<void()> callback);
    void SetResetCallback(std::function<void()> callback);
    void SetCheckResetCallback(std::function<bool()> callback);

    virtual void LaunchMenu(QPoint pos);

    //Does not check for existing options in menu, call in order you want them.
    virtual void BuildMenu(QMenu *menu);
protected:
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void keyPressEvent(QKeyEvent* event) override;

private slots:

protected:
    COPY_MENU_FLAGS m_flags;

    //callbacks for each menu event, these need to be set outside of class
    std::function<void()> onCopyCallback;
    std::function<void()> onPasteCallback;
    std::function<bool()> onCheckPasteCallback;
    std::function<void()> onDuplicateCallback;
    std::function<void()> onResetCallback;
    std::function<bool()> onCheckResetCallback;

    //callback for custom menu additions
    std::function<void(QMenu*)> onCustomMenuCreationCallback;
};
#endif // QCopyableWidget_h__

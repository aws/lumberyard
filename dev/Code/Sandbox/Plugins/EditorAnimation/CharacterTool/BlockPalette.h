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

#pragma once
#include <memory>
#include <QWidget>

namespace Serialization {
    class IArchive;
}

struct BlockPaletteContent;
struct BlockPaletteLayout;
struct BlockPaletteItem;

struct BlockPaletteSelectedId
{
    union
    {
        long long userId;
        void* userPointer;
    };

    BlockPaletteSelectedId()
        : userId(0)
    {
    }
};
typedef std::vector<BlockPaletteSelectedId> BlockPaletteSelectedIds;

class BlockPalette
    : public QWidget
{
    Q_OBJECT
public:
    BlockPalette(QWidget* parent);
    ~BlockPalette();

    void SetContent(const BlockPaletteContent& content);
    void SetAddEventWithSingleClick(bool addWithSingleClick);
    const BlockPaletteContent& Content() const{ return *m_content.get(); }

    void GetSelectedIds(BlockPaletteSelectedIds* ids) const;
    const BlockPaletteItem* GetItemByHotkey(int hotkey) const;
    void Serialize(Serialization::IArchive& ar);
signals:
    void SignalChanged();
    void SignalSelectionChanged();
    void SignalItemClicked(const BlockPaletteItem& item);
protected slots:
    void OnMenuAdd();
    void OnMenuDelete();
    void OnMenuAddWithSingleClick();
    void OnMenuAssignHotkey();
private:
    struct MouseHandler;
    struct SelectionHandler;
    struct DragHandler;

    void UpdateLayout();
    void ItemClicked(int mouseButton);
    void AssignHotkey(int hotkey);

    void paintEvent(QPaintEvent* ev) override;
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;
    void mouseReleaseEvent(QMouseEvent* ev) override;
    void mouseDoubleClickEvent(QMouseEvent* ev) override;
    void keyPressEvent(QKeyEvent* ev) override;
    void resizeEvent(QResizeEvent* ev) override;
    void wheelEvent(QWheelEvent* ev) override;

    std::unique_ptr<BlockPaletteContent> m_content;
    std::unique_ptr<BlockPaletteLayout> m_layout;
    std::unique_ptr<MouseHandler> m_mouseHandler;

    std::vector<int> m_selectedItems;
    std::vector<int> m_draggedItems;
    std::vector<int> m_hotkeys;
    bool m_addWithSingleClick;
};


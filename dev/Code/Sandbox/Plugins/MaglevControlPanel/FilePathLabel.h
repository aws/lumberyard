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

#include <QLabel>
#include <QDesktopServices>
#include <QPainter>
#include <QFontMetrics>
#include <QUrl>
#include <QMenu>
#include <QFileInfo>
#include <QApplication>
#include <QClipboard>

#include <IEditor.h>
#include <IFileUtil.h>

class FilePathLabel
    : public QLabel
{
public:

    FilePathLabel()
    {
        // Let the HBoxLayout set the label's size based on the space
        // available in the window instead of the width of the label's
        // text.
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &QLabel::customContextMenuRequested, this, &FilePathLabel::OnContextMenuRequested);
    }

    void setText(const QString& text)
    {
        QLabel::setText(text);

        if (text.isEmpty())
        {
            setToolTip("");
        }
        else
        {
            setToolTip("Double click to open in script editor or Explorer.");
        }

        // QLabel isn't repainting correctly. This forces the issue.
        // The root cause is size policy set in the constructor. It
        // seems that QLabel assumes it will be resized to fit its
        // text, so it doesn't bother erasing anything outside the
        // area the text occupies. Hence it leaves behind scraps from
        // longer strings.
        repaint();
    }

    void paintEvent(QPaintEvent* event) override
    {
        // If the text is too wide to fit in the space available,
        // truncate and prefix with "...".
        QPainter p(this);
        QFontMetrics fm(font());
        if (fm.horizontalAdvance(text()) > contentsRect().width())
        {
            QString elidedText {
                this->fontMetrics().elidedText(text(), Qt::ElideLeft, rect().width())
            };
            p.drawText(rect(), elidedText);
        }
        else
        {
            QLabel::paintEvent(event);
        }
    }

    void mouseDoubleClickEvent(QMouseEvent* event) override
    {
        QFileInfo fileInfo {
            text()
        };

        if (fileInfo.isDir())
        {
            OnOpenPathInExplorer();
        }
        else
        {
            OnOpenInScriptEditor();
        }
    }

    void OnCopyPathToClipboard()
    {
        auto clipboard = QApplication::clipboard();
        clipboard->setText(text());
    }

    void OnOpenPathInExplorer()
    {
        // GetIEditor()->GetFileUtil()->ShowInExplorer doesn't handle full paths. Using openUrl instead.
        QDesktopServices::openUrl(QUrl::fromLocalFile(text()));
    }

    void OnOpenLocationInExplorer()
    {
        // GetIEditor()->GetFileUtil()->ShowInExplorer doesn't handle full paths. Using openUrl instead.
        QFileInfo fileInfo {
            text()
        };
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));
    }

    void OnOpenInScriptEditor()
    {
        GetIEditor()->GetFileUtil()->EditTextFile(text().toStdString().c_str(), 0, IFileUtil::FILE_TYPE_SCRIPT);
    }

    QMenu* GetContextMenu()
    {
        if (text().isEmpty())
        {
            return nullptr;
        }

        QFileInfo fileInfo {
            text()
        };

        auto menu = new QMenu {};

        if (fileInfo.isDir())
        {
            auto openPathInExplorer = menu->addAction("View in Explorer");
            openPathInExplorer->setToolTip(tr("View the directory in Windows Explorer."));
            connect(openPathInExplorer, &QAction::triggered, this, &FilePathLabel::OnOpenPathInExplorer);

            auto copyPathToClipboard = menu->addAction("Copy path to clipboard");
            copyPathToClipboard->setToolTip(tr("Copy the directory path to the clipboard."));
            connect(copyPathToClipboard, &QAction::triggered, this, &FilePathLabel::OnCopyPathToClipboard);
        }
        else
        {
            auto openFile = menu->addAction("Open in script editor");
            openFile->setToolTip(tr("Open file in the scripted editor configured in Lumberyard's global preferences."));
            connect(openFile, &QAction::triggered, this, &FilePathLabel::OnOpenInScriptEditor);

            auto openPathInExplorer = menu->addAction("View in Explorer");
            openPathInExplorer->setToolTip(tr("View the file in Windows Explorer."));
            connect(openPathInExplorer, &QAction::triggered, this, &FilePathLabel::OnOpenLocationInExplorer);

            menu->addSeparator();

            auto copyPathToClipboard = menu->addAction("Copy path to clipboard");
            copyPathToClipboard->setToolTip(tr("Copy the file path to the clipboard."));
            connect(copyPathToClipboard, &QAction::triggered, this, &FilePathLabel::OnCopyPathToClipboard);
        }

        return menu;
    }

    void OnContextMenuRequested(QPoint pos)
    {
        auto menu = GetContextMenu();

        if (menu)
        {
            menu->popup(mapToGlobal(pos));
        }
    }
};


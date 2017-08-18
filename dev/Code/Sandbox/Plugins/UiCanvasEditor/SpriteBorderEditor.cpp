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
#include "stdafx.h"
#include "SpriteBorderEditorCommon.h"

//-------------------------------------------------------------------------------

#define UICANVASEDITOR_SPRITEBORDEREDITOR_SCENE_WIDTH   (200)
#define UICANVASEDITOR_SPRITEBORDEREDITOR_SCENE_HEIGHT  (200)

//-------------------------------------------------------------------------------

SpriteBorderEditor::SpriteBorderEditor(const char* path, QWidget* parent)
    : QDialog(parent)
    , m_hasBeenInitializedProperly(true)
{
    // Remove the ability to resize this window.
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);

    ISprite* sprite = gEnv->pLyShine->LoadSprite(path);
    if (!sprite)
    {
        m_hasBeenInitializedProperly = false;
        return;
    }

    // The layout.
    QGridLayout* outerGrid = new QGridLayout(this);

    QGridLayout* innerGrid = new QGridLayout();
    outerGrid->addLayout(innerGrid, 0, 0, 1, 2);

    // The scene.
    QGraphicsScene* scene(new QGraphicsScene(0.0f, 0.0f, UICANVASEDITOR_SPRITEBORDEREDITOR_SCENE_WIDTH, UICANVASEDITOR_SPRITEBORDEREDITOR_SCENE_HEIGHT, this));

    // The view.
    innerGrid->addWidget(new SlicerView(scene, this), 0, 0, 6, 1);

    // The image.
    QGraphicsPixmapItem* pixmapItem = nullptr;
    QSize unscaledPixmapSize;
    QSize scaledPixmapSize;
    {
        QString fullPath = Path::GamePathToFullPath(sprite->GetTexturePathname().c_str());
        QPixmap unscaledPixmap(fullPath);
        if ((unscaledPixmap.size().height() <= 0) &&
            (unscaledPixmap.size().width() <= 0))
        {
            m_hasBeenInitializedProperly = false;
            return;
        }

        bool isVertical = (unscaledPixmap.size().height() > unscaledPixmap.size().width());

        // Scale-to-fit, while preserving aspect ratio.
        pixmapItem = scene->addPixmap(isVertical ? unscaledPixmap.scaledToHeight(UICANVASEDITOR_SPRITEBORDEREDITOR_SCENE_HEIGHT) : unscaledPixmap.scaledToWidth(UICANVASEDITOR_SPRITEBORDEREDITOR_SCENE_WIDTH));

        unscaledPixmapSize = unscaledPixmap.size();
        scaledPixmapSize = pixmapItem->pixmap().size();
    }

    // Add text fields and manipulators.
    {
        int row = 0;

        innerGrid->addWidget(new QLabel(QString("Texture is %1 x %2").arg(QString::number(unscaledPixmapSize.width()),
                    QString::number(unscaledPixmapSize.height())),
                this),
            row++,
            1);

        for (const auto& b : SpriteBorder())
        {
            SlicerEdit* edit = new SlicerEdit(b,
                    unscaledPixmapSize,
                    sprite);

            SlicerManipulator* manipulator = new SlicerManipulator(b,
                    unscaledPixmapSize,
                    scaledPixmapSize,
                    sprite,
                    scene);

            edit->SetManipulator(manipulator);
            manipulator->SetEdit(edit);

            innerGrid->addWidget(new QLabel(SpriteBorderToString(b), this), row, 1);
            innerGrid->addWidget(edit, row, 2);
            innerGrid->addWidget(new QLabel("pixels", this), row, 3);
            ++row;
        }
    }

    // Add buttons.
    {
        // Save button.
        QPushButton* saveButton = new QPushButton("Save", this);
        QObject::connect(saveButton,
            &QPushButton::clicked,
            [ this, sprite ](bool checked)
            {
                // Sanitize values.
                //
                // This is the simplest way to sanitize the
                // border values. Otherwise, we need to prevent
                // flipping the manipulators in the UI.
                {
                    ISprite::Borders b = sprite->GetBorders();

                    if (b.m_top > b.m_bottom)
                    {
                        std::swap(b.m_top, b.m_bottom);
                    }

                    if (b.m_left > b.m_right)
                    {
                        std::swap(b.m_left, b.m_right);
                    }

                    sprite->SetBorders(b);
                }

                // the sprite file may not exist yet. If it does not then GamePathToFullPath
                // will give a path in the project even if the texture is in a gem.
                // The texture is guaranteed to exist so use that to get the full path.
                QString fullTexturePath = Path::GamePathToFullPath(sprite->GetTexturePathname().c_str());
                const char* const spriteExtension = "sprite";
                string fullSpritePath = PathUtil::ReplaceExtension(fullTexturePath.toLatin1().data(), spriteExtension);

                FileHelpers::SourceControlAddOrEdit(fullSpritePath.c_str(), QApplication::activeWindow());

                bool saveSuccessful = sprite->SaveToXml(fullSpritePath.c_str());

                if (saveSuccessful)
                {
                    accept();
                    return;
                }

                QMessageBox(QMessageBox::Critical,
                    "Error",
                    "Unable to save file. Is the file read-only?",
                    QMessageBox::Ok, QApplication::activeWindow()).exec();
            });
        outerGrid->addWidget(saveButton, 1, 0);

        // Cancel button.
        QPushButton* cancelButton = new QPushButton("Cancel", this);
        QObject::connect(cancelButton,
            &QPushButton::clicked,
            [ this ](bool checked)
            {
                reject();
            });
        outerGrid->addWidget(cancelButton, 1, 1);
    }

    // Signal handlers.
    {
        ISprite::Borders originalBorders = sprite->GetBorders();
        QObject::connect(this,
            &QDialog::rejected,
            [ sprite, originalBorders ]()
            {
                // Restore original borders.
                sprite->SetBorders(originalBorders);
            });
    }

    setWindowTitle("Border Editor");
    setModal(true);
    setWindowModality(Qt::ApplicationModal);

    layout()->setSizeConstraint(QLayout::SetFixedSize);

    // Position the widget centered around cursor.
    {
        QSize halfSize = (layout()->sizeHint() / 2);
        move(QCursor::pos() - QPoint(halfSize.width(), halfSize.height()));
    }
}

bool SpriteBorderEditor::GetHasBeenInitializedProperly()
{
    return m_hasBeenInitializedProperly;
}

#include <SpriteBorderEditor.moc>

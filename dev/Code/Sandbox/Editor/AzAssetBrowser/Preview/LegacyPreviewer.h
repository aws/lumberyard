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

#include <Editor/Util/FileUtil.h>
#include <Editor/Util/Image.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QWidget>
#include <QScopedPointer>

namespace Ui
{
    class PreviewWidgetClass;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class ProductAssetBrowserEntry;
        class SourceAssetBrowserEntry;
        class AssetBrowserEntry;
    }
}

class QResizeEvent;

class PreviewWidget
    : public QWidget
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(PreviewWidget, AZ::SystemAllocator, 0);

    explicit PreviewWidget(QWidget* parent = nullptr);
    ~PreviewWidget();

    void Clear() const;
    void Display(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry);

protected:
    void resizeEvent(QResizeEvent * event) override;

private:
    struct FileInfo
    {
        QString filename;
        unsigned attrib;
        time_t time_create;    /* -1 for FAT file systems */
        time_t time_access;    /* -1 for FAT file systems */
        time_t time_write;
        _fsize_t size;
    };

    enum class TextureType
    {
        RGB,
        RGBA,
        Alpha
    };

    QScopedPointer<Ui::PreviewWidgetClass> m_ui;
    CImageEx m_previewImageSource;
    CImageEx m_previewImageUpdated;
    TextureType m_textureType;
    QString m_fileinfo;
    QString m_fileinfoAlphaTexture;

    bool DisplayProduct(const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product);
    void DisplaySource(const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* source);

    QString GetFileSize(const char* path);

    bool DisplayTextureLegacy(const char* fullImagePath);
    bool DisplayTextureProductModern(const char* fullProductImagePath);

    void UpdateTextureType();

    static bool FileInfoCompare(const FileInfo& f1, const FileInfo& f2);
    //! QLabel word wrap does not break long words such as filenames, so manual word wrap needed
    static QString WordWrap(const QString& string, int maxLength);
};

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
#include "StdAfx.h"

#include <Editor/AzAssetBrowser/Preview/PreviewWidget.h>
#include <Editor/Controls/PreviewModelCtrl.h>

#include <AzAssetBrowser/Preview/ui_PreviewWidget.h>

#include <AzCore/Asset/AssetTypeInfoBus.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>

#if defined(AZ_PLATFORM_WINDOWS)
#include <io.h>
#endif

#include <ICryPak.h>
#include <Util/PathUtil.h>

#include <QString>
#include <QStringList>

static const int CHAR_WIDTH = 6;

PreviewWidget::PreviewWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::PreviewWidgetClass())
    , m_textureType(TextureType::RGB)
{
    m_ui->setupUi(this);
    m_ui->m_comboBoxRGB->addItems(QStringList() << "RGB" << "RGBA" << "Alpha");
    m_ui->m_previewCtrl->SetAspectRatio(4.0f / 3.0f);
    connect(m_ui->m_comboBoxRGB, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this,
        [=](int index)
        {
            m_textureType = static_cast<TextureType>(index);
            UpdateTextureType();
        });
    Clear();
}

PreviewWidget::~PreviewWidget()
{
}

void PreviewWidget::Clear() const
{
    m_ui->m_previewCtrl->ReleaseObject();
    m_ui->m_modelPreviewWidget->hide();
    m_ui->m_texturePreviewWidget->hide();
    m_ui->m_fileInfoCtrl->hide();
    m_ui->m_noPreviewWidget->show();
}

void PreviewWidget::Display(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry)
{
    using namespace AzToolsFramework::AssetBrowser;

    if (!entry)
    {
        Clear();
        return;
    }

    switch (entry->GetEntryType())
    {
    case AssetBrowserEntry::AssetEntryType::Source:
    {
        AZStd::vector<const ProductAssetBrowserEntry*> products;
        entry->GetChildrenRecursively<ProductAssetBrowserEntry>(products);
        if (products.empty())
        {
            Clear();
        }
        else
        {
            for (auto* product : products)
            {
                if (DisplayProduct(product))
                {
                    break;
                }
            }
        }
        break;
    }
    case AssetBrowserEntry::AssetEntryType::Product:
        DisplayProduct(static_cast<const ProductAssetBrowserEntry*>(entry));
        break;
    default:
        Clear();
    }
}

void PreviewWidget::resizeEvent(QResizeEvent* /*event*/)
{
    m_ui->m_fileInfoCtrl->setText(WordWrap(m_fileinfo, m_ui->m_fileInfoCtrl->width() / CHAR_WIDTH));
}

bool PreviewWidget::DisplayProduct(const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product)
{
    m_ui->m_fileInfoCtrl->show();

    m_fileinfo = QString::fromUtf8(product->GetName().c_str());

    AZ::u64 fileSizeResult = 0;
    if (AZ::IO::FileIOBase::GetInstance()->Size(product->GetRelativePath().c_str(), fileSizeResult))
    {
#if defined(AZ_PLATFORM_APPLE)
        static const double kb = 1000.0;
        static const double mb = kb * 1000.0;
        static const double gb = mb * 1000.0;

        if (fileSizeResult < kb)
        {
            m_fileinfo += tr("\r\nFile Size: %1B").arg(fileSizeResult);
        }
        else if (fileSizeResult < mb)
        {
            double size = fileSizeResult / kb;
            m_fileinfo += tr("\r\nFile Size: %1kB").arg(size, 0, 'f', 2);
        }
        else if (fileSizeResult < gb)
        {
            double size = fileSizeResult / mb;
            m_fileinfo += tr("\r\nFile Size: %1mB").arg(size, 0, 'f', 2);
        }
        else
        {
            double size = fileSizeResult / gb;
            m_fileinfo += tr("\r\nFile Size: %1gB").arg(size, 0, 'f', 2);
        }
#else
        static const double kb = 1024.0;
        static const double mb = kb * 1024.0;
        static const double gb = mb * 1024.0;

        if (fileSizeResult < kb)
        {
            m_fileinfo += tr("\r\nFile Size: %1B").arg(fileSizeResult);
        }
        else if (fileSizeResult < mb)
        {
            double size = fileSizeResult / kb;
            m_fileinfo += tr("\r\nFile Size: %1KB").arg(size, 0, 'f', 2);
        }
        else if (fileSizeResult < gb)
        {
            double size = fileSizeResult / mb;
            m_fileinfo += tr("\r\nFile Size: %1MB").arg(size, 0, 'f', 2);
        }
        else
        {
            double size = fileSizeResult / gb;
            m_fileinfo += tr("\r\nFile Size: %1GB").arg(size, 0, 'f', 2);
        }
#endif //AZ_PLATFORM_APPLE
    }

    EBusFindAssetTypeByName meshAssetTypeResult("Static Mesh");
    AZ::AssetTypeInfoBus::BroadcastResult(meshAssetTypeResult, &AZ::AssetTypeInfo::GetAssetType);

    QString filename(product->GetRelativePath().c_str());

    // Find item.
    if (product->GetAssetType() == meshAssetTypeResult.GetAssetType())
    {
        m_ui->m_modelPreviewWidget->show();
        m_ui->m_texturePreviewWidget->hide();
        m_ui->m_noPreviewWidget->hide();
        m_ui->m_previewCtrl->LoadFile(filename);

        int nVertexCount = m_ui->m_previewCtrl->GetVertexCount();
        int nFaceCount = m_ui->m_previewCtrl->GetFaceCount();
        int nMaxLod = m_ui->m_previewCtrl->GetMaxLod();
        int nMtls = m_ui->m_previewCtrl->GetMtlCount();
        if (nFaceCount > 0)
        {
            m_fileinfo += tr("\r\n%1 Faces\r\n%2 Verts\r\n%3 MaxLod\r\n%4 Materials").arg(nFaceCount).arg(nVertexCount).arg(nMaxLod).arg(nMtls);
        }
        m_ui->m_fileInfoCtrl->setText(WordWrap(m_fileinfo, m_ui->m_fileInfoCtrl->width() / CHAR_WIDTH));
        updateGeometry();
        return true;
    }

    EBusFindAssetTypeByName textureAssetTypeResult("Texture");
    AZ::AssetTypeInfoBus::BroadcastResult(textureAssetTypeResult, &AZ::AssetTypeInfo::GetAssetType);

    if (product->GetAssetType() == textureAssetTypeResult.GetAssetType())
    {
        m_ui->m_modelPreviewWidget->hide();
        m_ui->m_texturePreviewWidget->show();
        m_ui->m_noPreviewWidget->hide();

        bool foundPixmap = false;
        if (!AZ::IO::FileIOBase::GetInstance()->IsDirectory(filename.toUtf8().data()))
        {
            QString strLoadFilename = Path::GamePathToFullPath(filename);
            {                // scoped to control how long the file is open
                CCryFile file;
                if (!file.Open(strLoadFilename.toUtf8().data(), "rb"))
                {
                    // try the relative path instead as a backup
                    strLoadFilename = filename;
                }
            }

            if (CImageUtil::LoadImage(strLoadFilename, m_previewImageSource))
            {
                m_fileinfo += QStringLiteral("\r\n%1x%2\r\n%3")
                        .arg(m_previewImageSource.GetWidth())
                        .arg(m_previewImageSource.GetHeight())
                        .arg(m_previewImageSource.GetFormatDescription());

                UpdateTextureType();
                foundPixmap = true;
            }
        }
        if (!foundPixmap)
        {
            m_ui->m_previewImageCtrl->setPixmap(QPixmap());
        }
        m_ui->m_fileInfoCtrl->setText(WordWrap(m_fileinfo, m_ui->m_fileInfoCtrl->width() / CHAR_WIDTH));
        updateGeometry();
        return true;
    }

    Clear();
    return false;
}

void PreviewWidget::UpdateTextureType()
{
    m_previewImageUpdated.Copy(m_previewImageSource);

    switch (m_textureType)
    {
    case TextureType::RGB:
    {
        m_previewImageUpdated.SwapRedAndBlue();
        m_previewImageUpdated.FillAlpha();
        break;
    }
    case TextureType::RGBA:
    {
        m_previewImageUpdated.SwapRedAndBlue();
        break;
    }
    case TextureType::Alpha:
    {
        for (int h = 0; h < m_previewImageUpdated.GetHeight(); h++)
        {
            for (int w = 0; w < m_previewImageUpdated.GetWidth(); w++)
            {
                int a = m_previewImageUpdated.ValueAt(w, h) >> 24;
                m_previewImageUpdated.ValueAt(w, h) = RGB(a, a, a) | 0xFF000000;
            }
        }
        break;
    }
    }
    // note that Qt will not deep copy the data, so WE MUST KEEP THE IMAGE DATA AROUND!
    QPixmap qtPixmap = QPixmap::fromImage(
            QImage(reinterpret_cast<uchar*>(m_previewImageUpdated.GetData()), m_previewImageUpdated.GetWidth(), m_previewImageUpdated.GetHeight(), QImage::Format_ARGB32));
    m_ui->m_previewImageCtrl->setPixmap(qtPixmap);
    m_ui->m_previewImageCtrl->updateGeometry();
}

bool PreviewWidget::FileInfoCompare(const FileInfo& f1, const FileInfo& f2)
{
    if ((f1.attrib & _A_SUBDIR) && !(f2.attrib & _A_SUBDIR))
    {
        return true;
    }
    if (!(f1.attrib & _A_SUBDIR) && (f2.attrib & _A_SUBDIR))
    {
        return false;
    }

    return QString::compare(f1.filename, f2.filename, Qt::CaseInsensitive) < 0;
}

QString PreviewWidget::WordWrap(const QString& string, int maxLength)
{
    QString result;
    int length = 0;

    for (auto c : string)
    {
        if (c == '\n')
        {
            length = 0;
        }
        else if (length > maxLength)
        {
            result.append('\n');
            length = 0;
        }
        else
        {
            length++;
        }
        result.append(c);
    }
    return result;
}

#include <AzAssetBrowser/Preview/PreviewWidget.moc>

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

#include "StdAfx.h"
#include "Clipboard.h"

#include <QClipboard>
#include <QMessageBox>
#include <QTimer>
#include <QVariant>

XmlNodeRef CClipboard::m_node;
QString CClipboard::m_title;
QVariant CClipboard::s_pendingPut;

//////////////////////////////////////////////////////////////////////////
// Clipboard implementation.
//////////////////////////////////////////////////////////////////////////
CClipboard::CClipboard(QWidget* parent)
    : m_parent(parent != nullptr ? parent : QApplication::activeWindow())
{
    m_putDebounce.setSingleShot(true);
    m_putDebounce.setInterval(0);
    // Wait one frame before setting clipboard contents, in case we're updated frequently
    QObject::connect(&m_putDebounce, &QTimer::timeout, [this](){SendPendingPut();});
}

void CClipboard::Put(XmlNodeRef& node, const QString& title)
{
    m_title = title;
    if (m_title.isEmpty())
    {
        m_title = node->getTag();
    }
    m_node = node;

    PutString(m_node->getXML().c_str(), title);
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CClipboard::Get() const
{
    QString str = GetString();
    return XmlHelpers::LoadXmlFromBuffer(str.toUtf8().data(), str.toUtf8().length(), true);
}

//////////////////////////////////////////////////////////////////////////
void CClipboard::PutString(const QString& text, [[maybe_unused]]const QString& title /* = ""  */)
{
    s_pendingPut = text;
    m_putDebounce.start();
}

//////////////////////////////////////////////////////////////////////////
QString CClipboard::GetString() const
{
    if (s_pendingPut.type() == QVariant::String)
    {
        return s_pendingPut.toString();
    }
    return QApplication::clipboard()->text();
}

//////////////////////////////////////////////////////////////////////////
bool CClipboard::IsEmpty() const
{
    return GetString().isEmpty();
}

//////////////////////////////////////////////////////////////////////////
void CClipboard::PutImage(const CImageEx& img)
{
    QImage image(img.GetWidth(), img.GetHeight(), QImage::Format_RGBA8888);
    s_pendingPut = image;
    m_putDebounce.start();
}

//////////////////////////////////////////////////////////////////////////
bool CClipboard::GetImage(CImageEx& img)
{
    QImage image;
    if (s_pendingPut.type() == QVariant::Image)
    {
        image = s_pendingPut.value<QImage>();
    }
    else
    {
        image = QApplication::clipboard()->image();
    }

    img.Allocate(image.width(), image.height());

    unsigned char* pSrc = (unsigned char*)image.scanLine(0);
    unsigned char* pDst = (unsigned char*)img.GetData();

    int stSrc = (image.depth() == 24) ? 3 : 4;

    for (int y = 0; y < image.height(); y++)
    {
        for (int x = 0; x < image.width(); x++)
        {
            pDst[x * 4 + (image.height() - y - 1) * image.width() * 4] = pSrc[x * stSrc + y * image.bytesPerLine()];
            pDst[x * 4 + (image.height() - y - 1) * image.width() * 4 + 1] = pSrc[x * stSrc + y * image.bytesPerLine() + 1];
            pDst[x * 4 + (image.height() - y - 1) * image.width() * 4 + 2] = pSrc[x * stSrc + y * image.bytesPerLine() + 2];
            pDst[x * 4 + (image.height() - y - 1) * image.width() * 4 + 3] = 0;
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CClipboard::SendPendingPut()
{
    if (s_pendingPut.type() == QVariant::String)
    {
        QString text = s_pendingPut.toString();
        QApplication::clipboard()->setText(text);
    }
    else if (s_pendingPut.type() == QVariant::Image)
    {
        QImage image = s_pendingPut.value<QImage>();
        QApplication::clipboard()->setImage(image);
    }
    s_pendingPut = {};
}

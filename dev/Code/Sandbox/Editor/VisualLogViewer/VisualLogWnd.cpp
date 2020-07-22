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
#include <cctype>
#include "VisualLogWnd.h"
#include <AzQtComponents/Components/StyledDockWidget.h>

#if defined(Q_OS_WIN)
#include <QtWinExtras/qwinfunctions.h>
#endif
#include <QDockWidget>
#include <QFontInfo>
#include <QLabel>
#include <QResizeEvent>
#include <QSplitter>
#include <QPainter>
#include <QVBoxLayout>

#include <QtViewPaneManager.h>
#include "VisualLogCommon.h"
#include "VisualLogControls.h"
#include <QtUtil.h>
#include <QtUtilWin.h>

#define VISUALLOGWND_CLASSNAME "VisualLogViewerWindow"

#ifndef _countof
#define _countof(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

char* _fgetts(char* data, qint64 maxSize, QIODevice& dev)
{
    auto read = dev.readLine(data, maxSize);
    if (read < 0)
        return nullptr;
    return data;
}

int _tcscmp(const QByteArray& lhs, const QByteArray& rhs)
{
    return QString::compare(lhs, rhs);
}

int _tcslen(const QByteArray& lhs)
{
    return lhs.indexOf('\0');
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVisualLogWnd class

// CVisualLogWnd static functions & data
const GUID& CVisualLogWnd::GetClassID()
{
    // {585596D7-0DC8-4770-A365-18CF44E35F4D}
    static const GUID guid =
    {
        0x585596D7, 0x0DC8, 0x4770, {0xA3, 0x65, 0x18, 0xCF, 0x44, 0xE3, 0x5F, 0x4D}
    };
    return guid;
}

void CVisualLogWnd::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.canHaveMultipleInstances = true;
    options.sendViewPaneNameBackToAmazonAnalyticsServers = true;
    options.isLegacy = true;
    AzToolsFramework::RegisterViewPane<CVisualLogWnd>("Visual Log Viewer", LyViewPane::CategoryOther, options);
}



// CVisualLogWnd construction & destruction
CVisualLogWnd::CVisualLogWnd(QWidget* parent)
    : QWidget(parent)
    , m_pControlsDlg(new CVisualLogDialog)
    , m_pView(new CVLogImageView)
    , m_pTextView(new CVLogTextView)
    , m_pCD(new SVLogCommonData)
{
    QVBoxLayout* layout;

    QDockWidget* textViewPane = new AzQtComponents::StyledDockWidget(this);
    textViewPane->setFeatures(QDockWidget::NoDockWidgetFeatures);
    textViewPane->setWidget(m_pTextView.data());
    textViewPane->setWindowTitle(QStringLiteral("Text Output"));

    QSplitter* splitter = new QSplitter;
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(m_pView.data());
    splitter->addWidget(textViewPane);
    splitter->setSizes(QList<int>() << 200 << 200);

    layout = new QVBoxLayout;
    layout->addWidget(m_pControlsDlg.data());
    layout->addWidget(splitter, 1);
    setLayout(layout);

    // Set the common data in the view (m_pView) , the dialog (m_pControlsDlg) and the text output view
    m_pControlsDlg->SetCommonData(m_pCD.data());
    m_pView->SetCommonData(m_pCD.data());
    m_pTextView->SetCommonData(m_pCD.data());

    connect(m_pControlsDlg.data(), &CVisualLogDialog::BackgroundChanged,
        this, &CVisualLogWnd::RepaintImageView);
    connect(m_pControlsDlg.data(), &CVisualLogDialog::FolderChanged,
        this, &CVisualLogWnd::OnFolderChanged);
    connect(m_pControlsDlg.data(), &CVisualLogDialog::FrameChanged,
        this, &CVisualLogWnd::OnFrameChanged);
    connect(m_pControlsDlg.data(), &CVisualLogDialog::InvalidateViews,
        this, &CVisualLogWnd::RepaintIfNeeded);
    connect(m_pControlsDlg.data(), &CVisualLogDialog::UpdateTextChanged,
        this, &CVisualLogWnd::RepaintTextView);
    connect(m_pControlsDlg.data(), &CVisualLogDialog::UpdateImagesChanged,
        this, &CVisualLogWnd::RepaintImageView);
    connect(m_pControlsDlg.data(), &CVisualLogDialog::KeepAspectChanged,
        this, &CVisualLogWnd::RepaintImageView);
}

CVisualLogWnd::~CVisualLogWnd()
{
}



// CVisualLogWnd operations

bool CVisualLogWnd::ReadLogFiles()
{
    QString strDir(QFileInfo(m_pCD->strFolder).fileName());
    QString strBase = "", strNum = "";
    strDir = strDir.left(strDir.length() - 1);   // Remove the last character ('\' or '/')

    // Clear other data
    m_pCD->DestroyFonts();
    m_pCD->arrFrameTxt.clear();
    m_pCD->nMaxCol  = 0;

    // Check if the string ends with 4 digits
    if (strDir.length() < 6)
    {
        return false;
    }
    for (int i = 4; i > 0; --i)
    {
        QChar chTmp = strDir[strDir.length() - i];
        if (!chTmp.isDigit())
        {
            return false;
        }
        strNum += chTmp;
    }

    strBase = strDir.left(strDir.length() - 4);

    // Save the names of the log files
    m_pCD->strLog               = strBase + strNum + ".log";
    m_pCD->strLogParams = strBase + strNum + "_params.log";

    // Open the log files
    QFile pfLog(m_pCD->strFolder + m_pCD->strLog);
    if (!pfLog.open(QFile::ReadOnly))
    {
        return false;
    }
    QFile pfLogParams(m_pCD->strFolder + m_pCD->strLogParams);
    if (!pfLogParams.open(QFile::ReadOnly))
    {
        pfLog.close();
        return false;
    }

    // File reading...
    TCHAR szBuffLog[0x400], szBuffParams[0x400], * pRet = 0, * pRet1 = 0;
    bool bRetVal = false;

    auto nestedParsingFunction = [&]()
    {
        // Read the first 2 lines of each file
        pRet = _fgetts(szBuffLog, _countof(szBuffLog), pfLog);
        pRet1 = _fgetts(szBuffParams, _countof(szBuffParams), pfLogParams);
        if (!pRet || !pRet1 || (0 != _tcscmp(szBuffLog, szBuffParams)))
        {
            return;
        }

        pRet = _fgetts(szBuffLog, _countof(szBuffLog), pfLog);
        pRet1 = _fgetts(szBuffParams, _countof(szBuffParams), pfLogParams);
        if (!pRet || !pRet1 || (0 != _tcscmp(szBuffLog, szBuffParams)))
        {
            return;
        }

        // Read the next 4 lines of m_pCD->strLogParams (the default values)

        // Read the default color
        pRet = _fgetts(szBuffParams, _countof(szBuffParams), pfLogParams);
        if (!pRet)
        {
            return;
        }
        while (*pRet != '(')
        {
            pRet++;
        }
        pRet++;
        {   // Read the default color
            QString r, g, b, a;
            while (*pRet != ',')
            {
                r += *pRet;
                pRet++;
            }
            pRet++;
            while (*pRet != ',')
            {
                g += *pRet;
                pRet++;
            }
            pRet++;
            while (*pRet != ',')
            {
                b += *pRet;
                pRet++;
            }
            pRet++;
            while (*pRet != ')')
            {
                a += *pRet;
                pRet++;
            }
            pRet++;
            float fr = r.toFloat();
            float fg = g.toFloat();
            float fb = b.toFloat();
            float fa = a.toFloat();
            fr *= fa;
            fg *= fa;
            fb *= fa;
            m_pCD->defVals.clr = QColor(int(255.f * fr), int(255.f * fg), int(255.f * fb));
        }

        // Read the default fontsize
        pRet = _fgetts(szBuffParams, _countof(szBuffParams), pfLogParams);
        if (!pRet)
        {
            return;
        }
        while (*pRet != '(')
        {
            pRet++;
        }
        pRet++;
        { // Read the default fontsize
            QString fs;
            while (*pRet != ')')
            {
                fs += *pRet;
                pRet++;
            }
            pRet++;
            float fsize = fs.toFloat();
            m_pCD->defVals.nSize = int (fsize * SVLogCommonData::fFONT_MUL);

            QFont font(SVLogCommonData::strFONT, m_pCD->defVals.nSize);
            QFontMetrics fmetrics(font);
            m_pCD->mapFonts.insert(m_pCD->defVals.nSize, font);
            m_pCD->mapHeights[m_pCD->defVals.nSize] = fmetrics.height();
        }

        // Read the default column (columns are numbered 1, 2, ... N)
        pRet = _fgetts(szBuffParams, _countof(szBuffParams), pfLogParams);
        if (!pRet)
        {
            return;
        }
        while (*pRet != '(')
        {
            pRet++;
        }
        pRet++;
        { // Read the default column
            QString col;
            while (*pRet != ')')
            {
                col += *pRet;
                pRet++;
            }
            pRet++;
            m_pCD->defVals.nCol = col.toInt();
        }

        // The maximum column number
        if (m_pCD->nMaxCol < m_pCD->defVals.nCol)
        {
            m_pCD->nMaxCol = m_pCD->defVals.nCol;
        }

        // Ignore one line
        pRet = _fgetts(szBuffParams, _countof(szBuffParams), pfLogParams);
        if (!pRet)
        {
            return;
        }

        // Initialize the size of m_pCD->arrFrameTxt
        //  m_pCD->arrFrameTxt.SetSize(m_pCD->nLastFrame + 1);

        int nFrameCounter = 0;
        while (true)
        {
            TCHAR* pRet = _fgetts(szBuffLog, _countof(szBuffLog), pfLog);
            // Check if eof has been reached in pfLog, finish reading if so
            if ((0 == pRet) && (pfLog.atEnd()))
            {
                bRetVal = true;
                break;
            }

            // If eof has been reached in pfLogParams and not in pfLog --> error
            if (0 == _fgetts(szBuffParams, _countof(szBuffParams), pfLogParams))
            {
                break;
            }

            if (szBuffParams[0] == 'F') // Reading frame info
            {
                if (0 != _tcscmp(szBuffLog, szBuffParams))
                {
                    break;  // Error (frame info not equal)
                }
                // Read the frame number
                while (!std::isdigit(*pRet))
                {
                    pRet++;
                }
                QString strFrameNum;
                while (std::isdigit(*pRet))
                {
                    strFrameNum += *pRet;
                    pRet++;
                }
                int nFrameNum = strFrameNum.toInt();

                // Read the frame image string
                QString strFrameName;
                while (*pRet != '[')
                {
                    pRet++;
                }
                pRet++;
                while (*pRet != ']')
                {
                    strFrameName += *pRet;
                    pRet++;
                }

                // Read the frame time
                QString strFrameTime;
                while (*pRet != '[')
                {
                    pRet++;
                }
                pRet++;
                while (*pRet != ']')
                {
                    strFrameTime += *pRet;
                    pRet++;
                }
                float fFrameTime = strFrameTime.toFloat();

                // Exit if the frame number doesn't match
                if (nFrameNum != nFrameCounter)
                {
                    CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "Duplicate frame in log files [frame = %d]", nFrameNum);
                    return;
                }
                nFrameCounter++;

                m_pCD->arrFrameTxt.push_back(SVLogFrameTxtDataEx());
                m_pCD->arrFrameTxt[nFrameNum].Set(strFrameName, fFrameTime, new QList<SVLogFrameTxtData>());    // Insert element
            }
            else    // Reading frame data
            {
                szBuffLog[_tcslen(szBuffLog) - 1] = '\0'; // Remove the '\n' character

                SVLogFrameTxtData ftd(szBuffLog, m_pCD->defVals.nCol, m_pCD->defVals.clr, m_pCD->defVals.nSize, m_pCD->defVals.bSync);

                // Parse line data
                pRet = szBuffParams;
                if (*pRet != '[')
                {
                    break;
                }
                for (pRet++;; )
                {
                    if (*pRet == ']')
                    {
                        break;
                    }

                    QString strParam;
                    while (std::isalpha(*pRet) || (*pRet == '_'))
                    {
                        strParam += *pRet;
                        pRet++;
                    }
                    pRet++;

                    if (strParam == "color")
                    {   // Read the color
                        QString r, g, b, a;
                        while (*pRet != ',')
                        {
                            r += *pRet;
                            pRet++;
                        }
                        pRet++;
                        while (*pRet != ',')
                        {
                            g += *pRet;
                            pRet++;
                        }
                        pRet++;
                        while (*pRet != ',')
                        {
                            b += *pRet;
                            pRet++;
                        }
                        pRet++;
                        while (*pRet != ')')
                        {
                            a += *pRet;
                            pRet++;
                        }
                        pRet++;
                        if (*pRet == ',')
                        {
                            pRet++;
                        }
                        float fr = r.toFloat();
                        float fg = g.toFloat();
                        float fb = b.toFloat();
                        float fa = a.toFloat();
                        fr *= fa;
                        fg *= fa;
                        fb *= fa;
                        ftd.clr = QColor(int(255.f * fr), int(255.f * fg), int(255.f * fb));
                    }
                    else if (strParam == "size")
                    {   // Read the font size
                        QString fs;
                        while (*pRet != ')')
                        {
                            fs += *pRet;
                            pRet++;
                        }
                        pRet++;
                        if (*pRet == ',')
                        {
                            pRet++;
                        }
                        float fsize = fs.toFloat();
                        ftd.nSize = int (fsize * SVLogCommonData::fFONT_MUL);

                        if (!m_pCD->mapFonts.contains(ftd.nSize))
                        {
                            QFont font(SVLogCommonData::strFONT, ftd.nSize);
                            QFontMetrics fmetrics(font);
                            m_pCD->mapFonts.insert(ftd.nSize, font);
                            m_pCD->mapHeights[ftd.nSize] = fmetrics.height();
                        }
                    }
                    else if (strParam == "column")
                    {   // Read the column number
                        QString col;
                        while (*pRet != ')')
                        {
                            col += *pRet;
                            pRet++;
                        }
                        pRet++;
                        if (*pRet == ',')
                        {
                            pRet++;
                        }
                        ftd.nCol = col.toInt();

                        if (ftd.nCol > m_pCD->nMaxCol)
                        {
                            m_pCD->nMaxCol = ftd.nCol;
                        }
                    }
                    else if (strParam == "align_columns_to_this")
                    {
                        while (*pRet != ')')
                        {
                            pRet++;
                        }
                        pRet++;
                        if (*pRet == ',')
                        {
                            pRet++;
                        }

                        ftd.bSync = true;
                    }
                    else
                    {
                        return;
                    }
                }

                // Add a line for the frame
                m_pCD->arrFrameTxt[nFrameCounter - 1].lst->push_back(ftd);
            }
        }
    };

    nestedParsingFunction();

    // Close the log files
    pfLog.close();
    pfLogParams.close();

    if (!bRetVal) // Clear data on failure
    {
        m_pCD->DestroyFonts();
        m_pCD->arrFrameTxt.clear();
        m_pCD->nMaxCol  = 0;
    }
    else
    {
        m_pTextView->ResetColumns();
        m_pCD->bHasText = true;
    }

    return bRetVal;
}

void CVisualLogWnd::OnFolderChanged()
{
    bool bNoImages, bNoTextFiles;

    // Enumerate the image files
    CFileEnum::ScanDirectory(m_pCD->strFolder, "Frame*.*", m_pCD->vecFiles, false);
    bNoImages = m_pCD->vecFiles.isEmpty();

    if (!bNoImages)
    {
        m_pCD->img = QImage(m_pCD->strFolder + m_pCD->vecFiles[0]);
        if (!m_pCD->img.isNull())
        {
            m_pView->InitDestRect();
        }
        else
        {
            bNoImages = true;
        }
    }

    bNoTextFiles = !ReadLogFiles();

    if (!bNoTextFiles && !bNoImages)
    {   // Restructure image vector
        if (m_pCD->arrFrameTxt.count() != m_pCD->vecFiles.size())
        {
            QStringList vecNew = QVector<QString>(m_pCD->arrFrameTxt.count(), QString()).toList();

            QStringList::iterator itOrig = m_pCD->vecFiles.begin();
            QStringList::iterator itNew = vecNew.begin();
            int nTextIdx = 0;
            for (; itOrig != m_pCD->vecFiles.end(); ++itOrig)
            {
                while ((nTextIdx < m_pCD->arrFrameTxt.count()) && (m_pCD->arrFrameTxt[nTextIdx].strFileName != *itOrig))
                {
                    nTextIdx++;
                }
                if (nTextIdx >= m_pCD->arrFrameTxt.count())
                {
                    bNoImages = true;
                    break;
                }
                vecNew[nTextIdx] = m_pCD->arrFrameTxt[nTextIdx].strFileName;
            }

            m_pCD->vecFiles.swap(vecNew);
        }
    }

    if (bNoImages)
    {
        m_pCD->vecFiles.clear();
    }

    // Set the common data
    m_pCD->eState           = eVLPS_Active;
    m_pCD->nCurFrame    = 0;
    m_pCD->nLastFrame   = max(int(m_pCD->vecFiles.size()), int(m_pCD->arrFrameTxt.count())) - 1;
    m_pCD->bHasImages   = !bNoImages;

    if (bNoImages && bNoTextFiles)
    {
        m_pControlsDlg->FolderValid(false);
        return;
    }
    if (bNoImages)
    {
        CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "Error reading the image files!\r\nNo image preview will be available.");
        m_pCD->vecFiles.clear();
    }
    if (bNoTextFiles)
    {
        CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "Error reading the text (*.log) files!\r\nNo text output will be available.");
    }
    m_pControlsDlg->FolderValid(true);

    // Always invalidate the view even if the folder is invalid
    m_pView->repaint();
    m_pTextView->repaint();
}

void CVisualLogWnd::OnFrameChanged()
{
    m_pView->clearPreviousFrame();
    RepaintIfNeeded();
}

void CVisualLogWnd::RepaintIfNeeded()
{
    if (m_pCD->bUpdateImages && m_pCD->bHasImages)
    {
        m_pView->repaint();
    }
    if (m_pCD->bUpdateTxt && m_pCD->bHasText)
    {
        m_pTextView->repaint();
    }
}

void CVisualLogWnd::RepaintTextView()
{
    m_pTextView->repaint();
}

void CVisualLogWnd::RepaintImageView()
{
    m_pView->repaint();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVLogImageView class

// CVLogImageView construction & destruction
CVLogImageView::CVLogImageView(QWidget* parent)
    : QWidget(parent)
    , m_nPrevFrame(-1)
    , m_pCD(nullptr)
{
}

CVLogImageView::~CVLogImageView()
{
}



// CVLogImageView operations

void CVLogImageView::InitDestRect()
{
    if (m_pCD && !m_pCD->img.isNull() && (m_rcClient.right() > 4) && (m_rcClient.bottom() > 4))
    {
        //if ((m_pCD->img.GetWidth() <= m_rcClient.right) && (m_pCD->img.GetHeight() <= m_rcClient.bottom))
        //{
        //  int nLeft   = (m_rcClient.right - m_pCD->img.GetWidth()) / 2;
        //  int nTop    = (m_rcClient.bottom - m_pCD->img.GetHeight()) / 2;
        //  m_rcDest.SetRect(nLeft, nTop, nLeft + m_pCD->img.GetWidth(), nTop + m_pCD->img.GetHeight());
        //}
        //else
        {
            int nWidth = m_rcClient.right() - 4;
            int nHeight = (nWidth * m_pCD->img.height()) / m_pCD->img.width();

            if (nHeight < (m_rcClient.bottom() - 4))
            {
                m_rcDest.setRect(m_rcClient.left() + 2,
                    m_rcClient.top() + (m_rcClient.bottom() - nHeight) / 2,
                    m_rcClient.width() - 4,
                    nHeight);
            }
            else
            {
                nHeight = m_rcClient.bottom() - 4;
                nWidth = (nHeight * m_pCD->img.width()) / m_pCD->img.height();

                m_rcDest.setRect(m_rcClient.left() + (m_rcClient.right() - nWidth) / 2,
                    m_rcClient.top() + 2,
                    nWidth,
                    m_rcClient.height() - 4);
            }
        }
    }
}

QSize CVLogImageView::sizeHint() const
{
    return m_rcClient.size();
}


void CVLogImageView::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    QPainter p(this);

    QPen pen = p.pen();
    pen.setColor(Qt::white);
    p.setPen(pen);

    p.fillRect(m_rcClient, Qt::black);

    if ((m_pCD->eState == eVLPS_Uninitialized) || !m_pCD->bHasImages || (m_rcClient.right() < 5) || (m_rcClient.bottom() < 5) ||
        !m_pCD || (m_rcDest.width() < 1) || (m_rcDest.height() < 1))
    {
        p.fillRect(m_rcClient, m_pCD->clrBack);
    }
    else
    {
        if (!m_pCD->bUpdateImages || (m_pCD->vecFiles[m_pCD->nCurFrame] == ""))
        {
            p.fillRect(m_pCD->bKeepAspect  ?  m_rcDest : m_rcClient, QColor(127, 127, 127));
            if (m_pCD->bKeepAspect)
            {
                p.fillRect(0, 0, m_rcClient.right(), m_rcDest.top(), m_pCD->clrBack);
                p.fillRect(0, m_rcDest.top(), m_rcDest.left(), m_rcClient.bottom() - m_rcDest.top(), m_pCD->clrBack);
                p.fillRect(m_rcDest.right(), m_rcDest.top(), m_rcClient.right() - m_rcDest.right(),  m_rcClient.bottom() - m_rcDest.top(), m_pCD->clrBack);
                p.fillRect(m_rcDest.left(), m_rcDest.bottom(), m_rcDest.width(), m_rcClient.bottom() - m_rcDest.bottom(), m_pCD->clrBack);
            }
        }
        else
        {
            if (m_nPrevFrame != m_pCD->nCurFrame)
            {
                m_pCD->img = QImage(m_pCD->strFolder + m_pCD->vecFiles[m_pCD->nCurFrame]);
                m_nPrevFrame = m_pCD->nCurFrame;
            }

            if (m_pCD->bKeepAspect)
            {
                if (!m_pCD->img.isNull())
                {
                    p.drawImage(m_rcDest, m_pCD->img);
                }
                else
                {
                    p.fillRect(m_rcDest, QColor(127, 127, 127));
                }

                p.fillRect(0, 0, m_rcClient.right(), m_rcDest.top(), m_pCD->clrBack);
                p.fillRect(0, m_rcDest.top(), m_rcDest.left(), m_rcClient.bottom() - m_rcDest.top(), m_pCD->clrBack);
                p.fillRect(m_rcDest.right(), m_rcDest.top(), m_rcClient.right() - m_rcDest.right(),  m_rcClient.bottom() - m_rcDest.top(), m_pCD->clrBack);
                p.fillRect(m_rcDest.left(), m_rcDest.bottom(), m_rcDest.width(), m_rcClient.bottom() - m_rcDest.bottom(), m_pCD->clrBack);
            }
            else
            {
                p.drawImage(QRect(0, 0, m_rcClient.width(), m_rcClient.height()), m_pCD->img);
            }
        }
    }
}

void CVLogImageView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    m_rcClient.setSize(event->size());
    InitDestRect();
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVLogTextView class

// CVLogTextView construction & destruction
CVLogTextView::CVLogTextView(QWidget* parent)
    : QWidget(parent)
    , m_pCD(nullptr)
{
    QFont font;
    font.setFamily("Courier New");
    font.setPointSize(10);
    setFont(font);
}

CVLogTextView::~CVLogTextView()
{
}



// CVLogTextView operations

void CVLogTextView::ResetColumns()
{
    if (m_pCD && m_pCD->nMaxCol)
    {
        m_nColumnWidth = m_rcClient.width() / m_pCD->nMaxCol;

        m_arrColHeights.resize(m_pCD->nMaxCol);
        for (int i = 0; i < m_pCD->nMaxCol; ++i)
        {
            m_arrColHeights[i] = 20;
        }
    }
}

QSize CVLogTextView::sizeHint() const
{
    return m_rcClient.size();
}

void CVLogTextView::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    QPainter p(this);

    QPen pen = p.pen();
    pen.setColor(Qt::white);
    p.setPen(pen);

    p.fillRect(m_rcClient, Qt::black);

    // Drawing code goes here
    if ((!m_pCD->bUpdateTxt) || (m_pCD->eState == eVLPS_Uninitialized) || !m_pCD->bHasText)
    {
        p.drawText(10, 10, QStringLiteral("No Data / No Update")); // XXX: _T() was used, do we use tr()?
    }
    else
    {
        p.fillRect(0, 0, m_rcClient.right(), 20, QColor(25, 25, 40));
        SVLogFrameTxtDataEx& refFTX = m_pCD->arrFrameTxt[m_pCD->nCurFrame];
        int nMaxColHeight = 20;

        // Draw frame info first
        static QRect rc1(15, 0, 45, 20), rc2(48, 0, 198, 20), rc3(250, 0, 290, 20);
        QString strTime;

        pen.setColor(QColor(100, 255, 100));
        p.setPen(pen);

        p.drawText(rc1, Qt::AlignRight | Qt::AlignVCenter | Qt::TextSingleLine | Qt::TextDontClip, QStringLiteral("File:"));
        p.drawText(rc2, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine | Qt::TextDontClip, refFTX.strFileName);

        pen.setColor(QColor(140, 140, 255));
        p.setPen(pen);

        strTime.sprintf("Time: [%8.3f]", refFTX.fFrameTime);
        p.drawText(rc3, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine | Qt::TextDontClip, strTime);

        // Draw frame data
            for (auto refFT : *refFTX.lst)
        {
            pen.setColor(refFT.clr);

            p.setFont(m_pCD->mapFonts[refFT.nSize]);
            if (refFT.bSync)
            {
                for (int i = 0; i < m_pCD->nMaxCol; ++i)
                {
                    m_arrColHeights[i] = nMaxColHeight;
                }
            }
            int x = m_arrColHeights[refFT.nCol - 1];
            p.drawText(1 + m_nColumnWidth * (refFT.nCol - 1), m_arrColHeights[refFT.nCol - 1], refFT.str);
            m_arrColHeights[refFT.nCol - 1] += m_pCD->mapHeights[refFT.nSize];
            if (nMaxColHeight < m_arrColHeights[refFT.nCol - 1])
            {
                nMaxColHeight = m_arrColHeights[refFT.nCol - 1];
            }
        }
    }
}

void CVLogTextView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    m_rcClient.setSize(event->size());
    ResetColumns();
}

#include <VisualLogViewer/VisualLogWnd.moc>
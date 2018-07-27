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

// Description : Implementaion of the class CClouds.


#include "StdAfx.h"
#include "Clouds.h"
#include "Util/DynamicArray2D.h"

#include <QLabel>
#include <QPainter>

//////////////////////////////////////////////////////////////////////
// Construction / destruction
//////////////////////////////////////////////////////////////////////

CClouds::CClouds()
{
    // Init member variables
    m_iWidth = 0;
    m_iHeight = 0;
    m_sLastParam.bValid = false;
}

CClouds::~CClouds()
{
}

bool CClouds::GenerateClouds(SNoiseParams* sParam, QLabel* pWndStatus)
{
    //////////////////////////////////////////////////////////////////////
    // Generate the clouds
    //////////////////////////////////////////////////////////////////////

    assert(sParam);

    // Save the width & height
    m_iWidth = sParam->iWidth;
    m_iHeight = sParam->iHeight;

    // Create a new bitmap
    m_bmpClouds = QImage(512, 512, QImage::Format_RGB32);

    // Hold the last used parameters
    memcpy(&m_sLastParam, sParam, sizeof(SNoiseParams));
    m_sLastParam.bValid = true;

    // Call the generation function function with the supplied parameters
    CalcCloudsPerlin(sParam, pWndStatus);

    return true;
}

void CClouds::DrawClouds(QPainter& pDC, const QRect& prcPosition)
{
    //////////////////////////////////////////////////////////////////////
    // Draw the generated cloud texture
    //////////////////////////////////////////////////////////////////////

    if (!m_bmpClouds.isNull())
    {
        // Blit it to the destination rectangle
        pDC.drawImage(prcPosition, m_bmpClouds, QRect(0, 0, m_iWidth, m_iHeight));
    }
    else
    {
        // Just draw a black rectangle
        pDC.fillRect(prcPosition, Qt::black);
    }
}

void CClouds::CalcCloudsPerlin(SNoiseParams* sParam, QLabel* pWndStatus)
{
    //////////////////////////////////////////////////////////////////////
    // Generate perlin noise based clouds
    //////////////////////////////////////////////////////////////////////

    unsigned int i, j, h;
    uint clrfColor;
    float fValueRange;
    CDynamicArray2D cCloud(sParam->iWidth, sParam->iHeight);
    float fLowestPoint = 256000.0f, fHighestPoint = -256000.0f;
    float fScaledCol;
    CNoise cNoise;
    float fYScale = 255;

    assert(!IsBadReadPtr(sParam, sizeof(SNoiseParams)));
    assert(sParam->fFrequency);
    assert(sParam->iWidth && sParam->iHeight);
    assert(pWndStatus);
    assert(!m_bmpClouds.isNull());

    //////////////////////////////////////////////////////////////////////
    // Generate the noise array
    //////////////////////////////////////////////////////////////////////

    // Set the random value
    srand(sParam->iRandom);

    // Process layers
    for (i = 0; i < sParam->iPasses; i++)
    {
        // Show status
        QString sStatus = QObject::tr("Calculating pass %1 of %2...").arg(i + 1).arg(sParam->iPasses);
        pWndStatus->setText(sStatus);

        // Apply the fractal noise function to the array
        cNoise.FracSynthPass(&cCloud, sParam->fFrequency, fYScale,
            sParam->iWidth, sParam->iHeight, FALSE);

        // Modify noise generation parameters
        sParam->fFrequency *= sParam->fFrequencyStep;
        fYScale *= sParam->fFade;
    }

    //////////////////////////////////////////////////////////////////////
    // Paint the clouds into the bitmap
    //////////////////////////////////////////////////////////////////////

    // Apply the exponential function and find the value range
    for (i = 0; i < sParam->iWidth; i++)
    {
        for (j = 0; j < sParam->iHeight; j++)
        {
            // Apply an exponential function to get separated clouds
            cCloud.m_Array[i][j] = CloudExpCurve(cCloud.m_Array[i][j],
                    sParam->iCover, sParam->iSharpness);

            fHighestPoint = std::max(fHighestPoint, cCloud.m_Array[i][j]);
            fLowestPoint = std::min(fLowestPoint, cCloud.m_Array[i][j]);
        }
    }

    // Prepare some renormalisation values
    fValueRange = fHighestPoint - fLowestPoint;

    // Smooth the clouds
    for (h = 0; h < sParam->iSmoothness; h++)
    {
        for (i = 1; i < sParam->iWidth - 1; i++)
        {
            for (j = 1; j < sParam->iHeight - 1; j++)
            {
                cCloud.m_Array[i][j] =
                    (cCloud.m_Array[i][j]     + cCloud.m_Array[i + 1][j]   + cCloud.m_Array[i][j + 1] +
                     cCloud.m_Array[i + 1][j + 1] + cCloud.m_Array[i - 1][j]   + cCloud.m_Array[i][j - 1] +
                     cCloud.m_Array[i - 1][j - 1] + cCloud.m_Array[i + 1][j - 1] + cCloud.m_Array[i - 1][j + 1])
                    / 9.0f;
            }
        }
    }

    for (i = 0; i < sParam->iWidth; i++)
    {
        for (j = 0; j < sParam->iHeight; j++)
        {
            // Normalize it first
            cCloud.m_Array[i][j] += fLowestPoint;
            cCloud.m_Array[i][j] = cCloud.m_Array[i][j] / fValueRange * 255.0f;
            if (cCloud.m_Array[i][j] > 255)
            {
                cCloud.m_Array[i][j] = 255;
            }

            // Get the 0-255 index from the array, convert it into 0x00RRGGBB format, (add the
            // blue sky) and draw the pixel it into the bitmap
            if (sParam->bBlueSky)
            {
                fScaledCol = cCloud.m_Array[i][j] / 255.0f;
                clrfColor = ((int) (fScaledCol * 255 + (1 - fScaledCol) * 40)) << 16 |
                    ((int) (fScaledCol * 255 + (1 - fScaledCol) * 90)) << 8 |
                    ((int) (fScaledCol * 255 + (1 - fScaledCol) * 180));
            }
            else
            {
                fScaledCol = cCloud.m_Array[i][j] / 255.0f;
                clrfColor = ((int) (fScaledCol * 255 + (1 - fScaledCol))) << 16 |
                    ((int) (fScaledCol * 255 + (1 - fScaledCol))) << 8 |
                    ((int) (fScaledCol * 255 + (1 - fScaledCol)));
            }

            // Set the pixel
            m_bmpClouds.setPixel(i, j, clrfColor);
        }
    }
}

float CClouds::CloudExpCurve(float v, unsigned int iCover, float fSharpness)
{
    //////////////////////////////////////////////////////////////////////
    // Exponential function to generate separated clouds
    //////////////////////////////////////////////////////////////////////

    float CloudDensity;
    float c;

    c = v - iCover;

    if (c < 0)
    {
        c = 0;
    }

    CloudDensity = 255.f - (float) ((pow(fSharpness, c)) * 255.0f);

    return CloudDensity;
}

void CClouds::Serialize(CArchive& ar)
{
    ////////////////////////////////////////////////////////////////////////
    // Save or restore the class
    ////////////////////////////////////////////////////////////////////////

    if (ar.IsStoring())
    {
        // Storing
        ar << m_iWidth << m_iHeight;
        ar.Write(&m_sLastParam, sizeof(m_sLastParam));
    }
    else
    {
        // Loading
        ar >> m_iWidth >> m_iHeight;
        ar.Read(&m_sLastParam, sizeof(m_sLastParam));
    }
}

void CClouds::Serialize(CXmlArchive& xmlAr)
{
    if (xmlAr.bLoading)
    {
        //Load
        XmlNodeRef clouds = xmlAr.root->findChild("Clouds");
        if (clouds)
        {
            clouds->getAttr("Width", m_iWidth);
            clouds->getAttr("Height", m_iHeight);
        }
    }
    else
    {
        //Save
        XmlNodeRef clouds = xmlAr.root->newChild("Clouds");
        clouds->setAttr("Width", (int)m_iWidth);
        clouds->setAttr("Height", (int)m_iHeight);
    }
}

/*
void CClouds::CalcCloudsBezierFault(CLOUDPARAM *sParam, CWnd *pWndStatus)
{
    //////////////////////////////////////////////////////////////////////
    // Generate new clouds. We are using quadric Bezier curves
    // instead of simple lines to smooth out the image. The point in front
    // of line test is done by using a simple Bresenham algorithm to find
    // the intersection points with each scanline. Use the titel of
    // the supplied window to provide status informations
    //////////////////////////////////////////////////////////////////////

    unsigned int i, j, h, iScanline, iScanlineCurX;
    uint8 iColor;
    COLORREF clrfColor;
    POINT p1, p2, p3;
    float fValueRange;
    CDynamicArray2D cCloud(sParam->iWidth, sParam->iHeight);
    unsigned int *pScanlineIntersection = new unsigned int [m_iHeight];
    float fLowestPoint = 65536.0f, fHighestPoint = -65536.0f;
    float fIncreasement = 1.0f;
    KeyPoint sKeyPoints[MAX_KEY_POINTS];
    unsigned int iKeyPointCount;
    int iDeltaX, iDeltaY;
    int iUDeltaX, iUDeltaY;
    char szWindowTitel[32];
    unsigned int iStatus, iLastStatus = 0;

    assert(sParam);
    assert(sParam->iIterations);

    // Paint it black first
    m_dcClouds.BitBlt(0, 0, m_iWidth, m_iHeight, &m_dcClouds, 0, 0, BLACKNESS);

    // Do the iterations
    for (i=0; i<sParam->iIterations; i++)
    {
        // Update status (if necessary)
        iStatus = (int) (i / (float) sParam->iIterations * 100);
        if (iStatus != iLastStatus)
        {
            sprintf_s(szWindowTitel, "%i%% Done", iStatus);
            pWndStatus->SetWindowText(szWindowTitel);
            iLastStatus = iStatus;
        }

        //////////////////////////////////////////////////////////////////////
        // Generate the curve
        //////////////////////////////////////////////////////////////////////

        // Pick two random points on the border
        RandomPoints(&p1, &p2);

        iDeltaX = p2.x - p1.x; // Work out X delta
        iDeltaY = p2.y - p1.y; // Work out Y delta
        iUDeltaX = abs(iDeltaX); // iUDeltaX is the unsigned X delta
        iUDeltaY = abs(iDeltaY); // iUDeltaY is the unsigned Y delta

        // Subdivide the line, use a random point in the heightmap as third control point
        p3.x = (long) (rand() / (float) RAND_MAX * m_iWidth);
        p3.y = (long) (rand() / (float) RAND_MAX * m_iHeight);

        // Get the keypoints of the Bezier curve
        iKeyPointCount = QuadricSubdivide(&p1, &p3, &p2,
            BEZIER_SUBDIVISON_LEVEL, sKeyPoints, true);

        //////////////////////////////////////////////////////////////////////
        // Trace each curve segment into the scanline intersection array
        //////////////////////////////////////////////////////////////////////

        for (j=0; j<iKeyPointCount - 1; j++)
        {
            // Get the start / endpoints of the segment
            p1.x = (long) sKeyPoints[j].fX;
            p1.y = (long) sKeyPoints[j].fY;
            p2.x = (long) sKeyPoints[j + 1].fX;
            p2.y = (long) sKeyPoints[j + 1].fY;

            TraceCurveSegment(&p1, &p2, iUDeltaX, iUDeltaY, pScanlineIntersection);
        }

        //////////////////////////////////////////////////////////////////////
        // Process all scanlines and raise / lower the terrain
        //////////////////////////////////////////////////////////////////////

        // This ensures a good distribution of vallies and mountains
        if (rand() % 2)
        {
            if (iDeltaX > 0)
                iDeltaX = -iDeltaX;
            else
                iDeltaX = abs(iDeltaX);
        }

        // We have to approach the two delta cases different to make sure we are
        // processing all pixels
        if (iUDeltaX < iUDeltaY)
        {
            // This is necessary because the direction of the line is important for the
            // raising / lowering. Doing it here safes us from using a branch in the
            // inner loop
            if (iDeltaX > 0)
                fIncreasement = (float) fabs(fIncreasement);
            else
                fIncreasement = (float) -fabs(fIncreasement);

            for (iScanline=0; iScanline<m_iHeight; iScanline++)
            {
                // Loop trough each pixel in the scanline
                for (iScanlineCurX=0; iScanlineCurX<m_iWidth; iScanlineCurX++)
                {
                    // Raise / lower the terrain on each side of the line
                    if (iScanlineCurX > pScanlineIntersection[iScanline])
                        cCloud.m_Array[iScanlineCurX][iScanline] += fIncreasement;
                    else
                        cCloud.m_Array[iScanlineCurX][iScanline] -= fIncreasement;

                    // Do we have a new highest / lowest value ?
                    fLowestPoint = std::min(fLowestPoint, cCloud.m_Array[iScanlineCurX][iScanline]);
                    fHighestPoint = std::max(fHighestPoint, cCloud.m_Array[iScanlineCurX][iScanline]);
                }
            }
        }
        else
        {
            // This is necessary because the direction of the line is important for the
            // raising / lowering. Doing it here safes us from using a branch in the
            // inner loop
            if (iDeltaY > 0)
                fIncreasement = (float) fabs(fIncreasement);
            else
                fIncreasement = (float) -fabs(fIncreasement);

            // Loop trough each pixel in the scanline
            for (iScanlineCurX=0; iScanlineCurX<m_iWidth; iScanlineCurX++)
            {
                for (iScanline=0; iScanline<m_iHeight; iScanline++)
                {
                    // Raise / lower the terrain on each side of the line
                    if (iScanline > pScanlineIntersection[iScanlineCurX])
                        cCloud.m_Array[iScanlineCurX][iScanline] += fIncreasement;
                    else
                        cCloud.m_Array[iScanlineCurX][iScanline] -= fIncreasement;

                    // Do we have a new highest / lowest value ?
                    fLowestPoint = std::min(fLowestPoint, cCloud.m_Array[iScanlineCurX][iScanline]);
                    fHighestPoint = std::max(fHighestPoint, cCloud.m_Array[iScanlineCurX][iScanline]);
                }
            }
        }
    }

    //////////////////////////////////////////////////////////////////////
    // Paint the clouds into the bitmap
    //////////////////////////////////////////////////////////////////////

    // Prepare some renormalisation values. Shorten the value range to get
    // higher contrast clouds
    fValueRange = (fHighestPoint - fLowestPoint) / 1.3f;

    if (fLowestPoint < 0)
    {
        fLowestPoint = (float) fabs(fLowestPoint);
        fHighestPoint += (float) fabs(fLowestPoint);
    }

    // Smooth the clouds
    for (h=0; h<sParam->iSmoothness; h++)
        for (i=1; i<m_iWidth-1; i++)
            for (j=1; j<m_iHeight-1; j++)
            {
                cCloud.m_Array[i][j] =
                    (cCloud.m_Array[i][j]     + cCloud.m_Array[i+1][j]   + cCloud.m_Array[i][j+1] +
                     cCloud.m_Array[i+1][j+1] + cCloud.m_Array[i-1][j]   + cCloud.m_Array[i][j-1] +
                     cCloud.m_Array[i-1][j-1] + cCloud.m_Array[i+1][j-1] + cCloud.m_Array[i-1][j+1])
                     / 9.0f;
            }

    for (i=0; i<m_iWidth; i++)
        for (j=0; j<m_iHeight; j++)
        {
            // Normalize it first
            cCloud.m_Array[i][j] += fLowestPoint;
            cCloud.m_Array[i][j] = cCloud.m_Array[i][j] / fValueRange * 255.0f;
            if (cCloud.m_Array[i][j] > 255)
                cCloud.m_Array[i][j] = 255;

            // Apply an exponetial function to get separated clouds
            cCloud.m_Array[i][j] = CloudExpCurve(cCloud.m_Array[i][j]);

            // Get the 0-255 index from the array, convert it into 0x00BBGGRR grayscale
            // and draw it into the bitmap
            iColor = (int8) cCloud.m_Array[i][j];
            clrfColor = iColor | iColor << 8 | iColor << 16;
            m_dcClouds.SetPixelV(i, j, clrfColor);
        }

    // Free the scanline intersection data
    delete [] pScanlineIntersection;
    pScanlineIntersection = 0;
}

unsigned int CClouds::QuadricSubdivide(LPPOINT p1, LPPOINT p2, LPPOINT p3, unsigned int iLevel,
                                          KeyPoint *pKeyPointArray, bool bFirstSubdivision)
{
    //////////////////////////////////////////////////////////////////////
    // Add a line (one key point). If level is 0 use the end points. If
    // this is the first subdivision, you have to pass true for
    // bFirstSubdivision. The number of keypoints is returned
    //////////////////////////////////////////////////////////////////////

    POINT pGl[3];
    POINT pGr[3];
    static int iCurKeyPoint = 0;

    // Start with the first keypoint when doing the first subdivision
    if (bFirstSubdivision)
        iCurKeyPoint = 0;

    if (iLevel == 0)
    {
        // Add it to the list
        pKeyPointArray[iCurKeyPoint].fX = (float) p1->x;
        pKeyPointArray[iCurKeyPoint].fY = (float) p1->y;

        // Advance to the next
        iCurKeyPoint++;

        return iCurKeyPoint;
    }

    // New geometry vectors for the left and right halves of the curve

    // Subdivide

    memcpy(&pGl[0], p1, sizeof(POINT));

    pGl[1].x = (p1->x + p2->x) >> 1;
    pGl[1].y = (p1->y + p2->y) >> 1;

    pGr[1].x = (p2->x + p3->x) >> 1;
    pGr[1].y = (p2->y + p3->y) >> 1;

    pGl[2].x = pGr[0].x = (pGl[1].x + pGr[1].x) >> 1;
    pGl[2].y = pGr[0].y = (pGl[1].y + pGr[1].y) >> 1;

    memcpy(&pGr[2], p3, sizeof(POINT));

    // Call recursively for left and right halves
    QuadricSubdivide(&pGl[0], &pGl[1], &pGl[2], --iLevel, pKeyPointArray, false);
    QuadricSubdivide(&pGr[0], &pGr[1], &pGr[2], iLevel, pKeyPointArray, false);

    // Add the end of the curve to the keypoint list if this is the first
    // recursion level
    if (bFirstSubdivision)
    {
        pKeyPointArray[iCurKeyPoint].fX = (float) p3->x;
        pKeyPointArray[iCurKeyPoint].fY = (float) p3->y;

        iCurKeyPoint++;
    }

    return iCurKeyPoint;
}

void CClouds::TraceCurveSegment(LPPOINT p1, LPPOINT p2, int iUCurveDeltaX,
                                   int iUCurveDeltaY, unsigned int *pScanlineIntersection)
{
    //////////////////////////////////////////////////////////////////////
    // Use the Bresenham line drawing algorithm to find the scanline
    // intersection points
    //////////////////////////////////////////////////////////////////////

    int iDeltaX, iDeltaY;
    int iUDeltaX, iUDeltaY;
    int iXAdd, iYAdd;
    int iError, iLoop;

    iDeltaX = p2->x - p1->x; // Work out X delta
    iDeltaY = p2->y - p1->y; // Work out Y delta

    iUDeltaX = abs(iDeltaX); // iUDeltaX is the unsigned X delta
    iUDeltaY = abs(iDeltaY); // iUDeltaY is the unsigned Y delta

    // Work out direction to step in the Y direction
    if (iDeltaX < 0)
        iXAdd = -1;
    else
        iXAdd = 1;

    // Work out direction to step in the Y direction
    if (iDeltaY < 0)
        iYAdd = -1;
    else
        iYAdd = 1;

    iError = 0;
    iLoop = 0;

    if (iUDeltaX > iUDeltaY)
    {
        // Delta X > Delta Y
        do
        {
            iError += iUDeltaY;

            // Time to move up / down ?
            if (iError >= iUDeltaX)
            {
                iError -= iUDeltaX;
                p1->y += iYAdd;
            }

            iLoop++;

            // Save scanline intersection
            if (iUCurveDeltaX < iUCurveDeltaY)
                pScanlineIntersection[p1->y] = p1->x;
            else
                pScanlineIntersection[p1->x] = p1->y;

            // Move horizontally
            p1->x += iXAdd;
        }
        while (iLoop < iUDeltaX); // Repeat for x length of line
    }
    else
    {
        // Delta Y > Delta X
        do
        {
            iError += iUDeltaX;

            // Time to move left / right?
            if (iError >= iUDeltaY)
            {
                iError -= iUDeltaY;
                // Move across
                p1->x += iXAdd;
            }

            iLoop++;

            // Save scanline intersection
            if (iUCurveDeltaX < iUCurveDeltaY)
                pScanlineIntersection[p1->y] = p1->x;
            else
                pScanlineIntersection[p1->x] = p1->y;

            // Move up / down a row
            p1->y += iYAdd;
        }
        while (iLoop < iUDeltaY); // Repeat for y length of line
    }
}

void CClouds::RandomPoints(LPPOINT p1, LPPOINT p2)
{
    //////////////////////////////////////////////////////////////////////
    // Make two random points that lay on the bitmap's boundaries
    //////////////////////////////////////////////////////////////////////

    bool bSwapSides;

    assert(p1 && p2);

    bSwapSides = (rand() % 2) ? true : false;

    if (rand() % 2)
    {
        p1->x = (long) (rand() / (float) RAND_MAX * m_iWidth);
        p1->y = bSwapSides ? 0 : m_iHeight - 1;
        p2->x = (long) (rand() / (float) RAND_MAX * m_iWidth);
        p2->y = bSwapSides ? m_iHeight - 1 : 0;
    }
    else
    {
        p1->x = bSwapSides ? m_iWidth - 1 : 0;
        p1->y = (long) (rand() / (float) RAND_MAX * m_iHeight);
        p2->x = bSwapSides ? 0 : m_iWidth - 1;
        p2->y = (long) (rand() / (float) RAND_MAX * m_iHeight);
    }
}
*/

#include <Terrain/Clouds.moc>
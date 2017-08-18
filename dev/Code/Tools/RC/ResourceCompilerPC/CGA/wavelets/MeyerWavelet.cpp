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

#include "stdafx.h"

#include "MeyerWavelet.h"

static TWavereal hm[62] =
{
    -0.00060115023435f, -0.00270467212464f, 0.00220253410091f, 0.00604581409732f, -0.00638771831850f,
    -0.01106149639251f, 0.01527001513093f, 0.01742343410373f, -0.03213079399021f, -0.02434874590608f,
    0.06373902432280f, 0.03065509196082f, -0.13284520043623f, -0.03508755565626f, 0.44459300275758f,
    0.74458559231881f, 0.44459300275758f, -0.03508755565626f, -0.13284520043623f, 0.03065509196082f,
    0.06373902432280f, -0.02434874590608f, -0.03213079399021f, 0.01742343410373f, 0.01527001513093f,
    -0.01106149639251f, -0.00638771831850f, 0.00604581409732f, 0.00220253410091f, -0.00270467212464f,
    -0.00060115023435f, 0.00006554305931f
};

//--------------------------------------------------------------------
//WaveletM class is the class implementing Discrette Wavelet Transform with Meyer wavelet.

void CMeyerDWT::Decompose(int level, int layer)
{
    // Does one step of the Mayer decomposition transform.
    //

    unsigned int  jc = (unsigned int)m_WS.GetCount() >> ((unsigned int)level + 1);
    unsigned int nh2 = m_nCoeffs >> 1;
    unsigned int jc2 = jc << 1;
    unsigned int i, j;
    int j1, l1, i2, i21;
    int sign;

    TWavereal* pDataa;
    int ida = GetWaveletLayerNumber(level, layer);
    pDataa = m_WS.GetData(ida);

    TWaveData approx(jc);
    TWaveData detail(jc);

    for (i = 0; i < jc; i++)
    {
        i2 = (i << 1) + nh2;
        i21 = (i << 1) - nh2 + 1;
        approx[i] = 0;
        detail[i] = 0;
        for (j = 0; j < m_nCoeffs; j++)
        {
            j1 = i21 + j;
            l1 = i2 - j;
            if (j1 < 0)
            {
                j1 += jc2;
            }

            if (j1 >= (int)jc2)
            {
                j1 -= jc2;
            }

            if (l1 < 0)
            {
                l1 += jc2;
            }

            if (l1 >= (int)jc2)
            {
                l1 -= jc2;
            }

            sign = (j & 1) == 0 ? -1 : 1;
            approx[i] += m_Filter[j] * pDataa[j1 << level];
            detail[i] += sign * m_Filter[j] * pDataa[l1 << level];
        }
    }

    int level1 = level + 1;
    int idaa = GetWaveletLayerNumber(level1, layer << 1);
    int idad = GetWaveletLayerNumber(level1, (layer << 1) + 1);
    pDataa = m_WS.GetData(idaa);// pWDC->data+idaa;
    TWavereal* pDatad = m_WS.GetData(idad);// pWDC->data+idad;

    for (i = 0; i < jc; i++)
    {
        pDataa[i << level1] = approx[i];
        pDatad[i << level1] = detail[i];
    }
}

void CMeyerDWT::Reconstruct(int level, int layer)
{
    // Does one step of the Meyer reconstruction transform.

    int jc = (int)m_WS.GetCount() >> (level + 1);
    int nh2 = m_nCoeffs >> 1;
    int jc2 = jc << 1;
    int level1 = level + 1;
    int ja = !(nh2 & 1);
    int i, j, j1, i2, sign;

    int idaa = GetWaveletLayerNumber(level1, layer << 1);
    int idad = GetWaveletLayerNumber(level1, (layer << 1) + 1);

    TWavereal* pDataa = m_WS.GetData(idaa);
    TWavereal* pDatad = m_WS.GetData(idad);


    TWaveData s(jc2);

    // approximate on this level

    for (i = 0; i < jc2; i++)
    {
        s[i] = 0;
        i2 = (i + nh2 - 1) >> 1;
        for (j = 0; j < nh2; j++)
        {
            j1 = i2 - j;
            if (j1 < 0)
            {
                j1 += jc;
            }

            if (j1 >= (jc))
            {
                j1 -= jc;
            }

            s[i] += m_Filter[(j << 1) + ja] * pDataa[j1 << level1];
        }

        ja = !ja;
    }

    // details on this level

    ja = (nh2 & 1);
    sign = (nh2 & 1) == 0 ? -1 : 1;

    for (i = 0; i < jc2; i++)
    {
        i2 = (i - nh2 + 1) >> 1;
        for (j = 0; j < nh2; j++)
        {
            j1 = i2 + j;

            if (j1 < 0)
            {
                j1 += jc;
            }

            if (j1 >= (jc))
            {
                j1 -= jc;
            }

            s[i] += sign * m_Filter[(j << 1) + ja] * pDatad[j1 << level1];
        }
        sign = -(sign);
        ja = !ja;
    }

    int ida = GetWaveletLayerNumber(level, layer);
    pDataa = m_WS.GetData(ida);

    for (i = 0; i < jc2; i++)
    {
        pDataa[i << level] = s[i];
    }
}



CMeyerDWT::CMeyerDWT(bool tree)
    : CDWT(1, tree)
{
    m_Filter = hm;
    m_nCoeffs = 61;
}


CMeyerDWT::CMeyerDWT(int num, bool tree)
    : CDWT(num, tree)
{
    m_Filter = hm;
    m_nCoeffs = 61;
}


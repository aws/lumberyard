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

#include "wavelet.h"


void CBaseDT::Denoise(TWavereal cut)
{
    // Simple denoising procedure. Sets to zero all elements of
    // wavelet spectrum which absolute value less then "cut".

    if (cut < 0.0f)
    {
        cut = -cut;
    }

    for (TWaveData::iterator it = m_WS.m_Data.begin(), end = m_WS.m_Data.end(); it != end; ++it)
    {
        if (fabs(*it) < cut)
        {
            *it = 0.0f;
        }
    }
}

int CBaseDT::LastMaximal(TWavereal val)
{
    for (TWaveData::iterator it = m_WS.m_Data.end() - 1, end = m_WS.m_Data.begin(); it != end; --it)
    {
        if (fabs(*it) > val)
        {
            return (int)std::distance(m_WS.m_Data.begin(), it);
        }
    }

    return 0;
}



void CDWT::DWT(const CWaveletData& data, int level)
{
    m_WS.m_Data = data.m_Data;
    m_WS.m_Rate = data.m_Rate;
    if (level != 0)
    {
        DWT(level);
    }
}

void CDWT::IDWT(CWaveletData& data, int level)
{
    //  m_WS = data;

    if (level != 0)
    {
        IDWT(level);
    }

    data.m_Data = m_WS.m_Data;
    data.m_Rate = m_WS.m_Rate;
}


void CDWT::DWT(int level)
{
    int maxLevel = GetMaxLevel();

    int levs = m_nCurLevel;

    int levf = m_nCurLevel + level;
    if ((level == -1) || (levf > maxLevel))
    {
        levf = maxLevel;
    }

    int layf;

    for (int i = levs; i < levf; i++)
    {
        layf = m_bTree ? 1 << i : 1;
        for (int layer = 0; layer < layf; layer++)
        {
            Decompose(i, layer);
        }

        m_nCurLevel = i + 1;
    }

    m_nCurLevel = levf;
}

void CDWT::IDWT(int level)
{
    int levs = m_nCurLevel;
    int levf = m_nCurLevel - level;
    if ((level == -1) || (levf < 0))
    {
        levf = 0;
    }

    int layf;

    for (int i = levs - 1; i >= levf; i--)
    {
        layf = m_bTree ? 1 << i : 1;
        for (int layer = 0; layer < (layf); layer++)
        {
            Reconstruct(i, layer);
        }

        m_nCurLevel = i;
    }
    m_nCurLevel = levf;
}

int CDWT::GetMaxLevel() const
{
    int res = 0;
    for (std::size_t i = m_WS.GetCount(); i >= m_nCoeffs; i = i >> 1, res++)
    {
        ;
    }
    return res;
}

// Get wavelet layer number for given level and layer.
int CDWT::GetWaveletLayerNumber(int level, int layer)
{
    int IdA = 0;

    for (int i = 0; i < level; i++)
    {
        if ((layer >> i) & 1)
        {
            IdA += 1 << (level - 1 - i);
        }
    }
    return IdA;
}

// Get frequency ID number for given level and layer.
int CDWT::GetFrequencyLayerNumber(int level, int IdL)
{
    //
    int IdF = IdL;

    for (int i = 1; i < level; i++)
    {
        int j = ((1 << i) & (IdF));
        if (j)
        {
            IdF = ((1 << i) - 1) ^ (IdF);
        }
    }

    return IdF;
}

// Get the layer number in binary tree for given level and frequency ID number.
int CDWT::GetLayerNumber(int level, int IdF)
{
    int IdL = IdF;
    for (int i = level - 1; i >= 1; i--)
    {
        int j = ((1 << i) & (IdL));
        if (j)
        {
            IdL = ((1 << i) - 1) ^ (IdL);
        }
    }

    return IdL;
}



void CDWT::GetFreqLayer(CWaveletData& td, int k)
{
    int k1 = GetLayerNumber(m_nCurLevel, k);
    GetLayer(td, k1);
}

void CDWT::GetLayer(CWaveletData& td, int k)
{
    int start, step;

    if (k < 0)
    {
        k = 0;
    }

    if (m_bTree)
    {
        int max_layers = 1 << m_nCurLevel;

        if (k > max_layers - 1)
        {
            k = max_layers - 1;
        }

        int m = (int)(m_WS.GetCount()) >> m_nCurLevel;

        if (td.GetCount() != m)
        {
            td.m_Data.resize(m);
        }

        int k1 = GetWaveletLayerNumber(m_nCurLevel, k);
        for (int i = 0; i < m; i++)
        {
            td.m_Data[i] = m_WS.m_Data[(i << m_nCurLevel) + k1];// pWDC->data[(i<<Level)+k1];
        }
    }
    else
    {
        bool is_top = false;
        int maxLevel = GetMaxLevel();

        if (k > maxLevel)
        {
            k = maxLevel;
        }

        if (k == 0)
        {
            is_top = true;
            k = m_nCurLevel;
        }

        int m = (int)(m_WS.GetCount()) >> k;

        //      if (td.GetCount() != m)
        td.m_Data.resize(m);

        start = (is_top) ? 0 : 1 << (k - 1);
        step = 1 << k;

        TWavereal* out, * d;
        out = &td.m_Data[0];
        d = m_WS.GetData(start);// pWDC->data+start;

        for (int i = 0; i < m; i++, out++, d += step)
        {
            *out = *d;
        }
    }
}

// replace layer with given number
void CDWT::PutLayer(CWaveletData& td, int k)
{
    // Replace content of the layer number k with data from array td.
    //
    int start, step;

    if (k < 0)
    {
        k = 0;
    }

    if (m_bTree)
    {
        int max_layers = 1 << m_nCurLevel;

        if (k > max_layers - 1)
        {
            k = max_layers - 1;
        }

        std::size_t m = (m_WS.GetCount()) >> m_nCurLevel;

        if (td.GetCount() != m)
        {
            return;
        }

        int k1 = GetWaveletLayerNumber(m_nCurLevel, k);

        for (std::size_t i = 0; i < m; i++)
        {
            m_WS.m_Data[(i << m_nCurLevel) + k1] = td.m_Data[i];
        }
    }
    else
    {
        bool is_top(false);

        int maxLevel = GetMaxLevel();
        if (k > maxLevel)
        {
            k = maxLevel;
        }

        if (k == 0)
        {
            is_top = true;
            k = m_nCurLevel;
        }

        std::size_t m = m_WS.GetCount() >> k;

        if (td.GetCount() != m)
        {
            return;
        }

        start = (is_top) ? 0 : 1 << (k - 1);
        step = 1 << k;

        TWavereal* in, * d;
        in = &td.m_Data[0];
        d = m_WS.GetData(start);// pWDC->data+start;

        for (std::size_t i = 0; i < m; i++, in++, d += step)
        {
            *d = *in;
        }
    }
}


void CDWT::PutFreqLayer(CWaveletData& td, int k)
{
    // The same as putLayer, but put layers in frequency order.

    PutLayer(td, GetFrequencyLayerNumber(m_nCurLevel, k));
}










// Forward DCT
void CDCT::FDCT(const CWaveletData& data)
{
    std::size_t count = data.GetCount();

    m_WS.m_Data.resize(count);
    TWavereal pi = PI / (TWavereal) count;
    for (std::size_t i = 0; i < count; ++i)
    {
        TWavereal currval (0.0f);

        for (std::size_t j = 0; j < count; ++j)
        {
            TWavereal Xn = data.m_Data[j];

            TWavereal cosine = cos(pi * ((TWavereal)j + 0.5f) * (TWavereal)i);
            currval += cosine * Xn;
        }

        m_WS.m_Data[i] = currval;
    }

    // cosine table

    m_cosine.resize(count * count);

    for (std::size_t i = 0; i < count; ++i)
    {
        for (std::size_t j = 0; j < count; ++j)
        {
            TWavereal cosine = cos(pi * ((TWavereal)i + 0.5f) * (TWavereal)j);
            m_cosine[i * count + j] = cosine;
        }
    }
}

// Inverse DCT
void CDCT::IDCT(CWaveletData& data, int lastEffective)
{
    std::size_t count = m_WS.GetCount();

    data.m_Data.resize(count);
    TWavereal pi = PI / (TWavereal) count;

    if (lastEffective == -1)
    {
        lastEffective = (int)count;
    }

    for (std::size_t i = 0; i < count; ++i)
    {
        TWavereal currval (0.5f * m_WS.m_Data[0]);

        for (std::size_t j = 1; j < (unsigned int)lastEffective; ++j)
        {
            TWavereal Xn = m_WS.m_Data[j];

            TWavereal cosine =  cos(pi * ((TWavereal)i + 0.5f) * (TWavereal)j); /* m_cosine[i * count + j];// * /*/
            currval += cosine * Xn;
        }
        data.m_Data[i] = currval * 2 / (TWavereal)(count);
    }
}









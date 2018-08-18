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

#include "CompressedData.h"

CWaveRDC::CWaveRDC()
{
    // Default constructor

    m_nLSW = 4;//
    m_Options = 0;
}

CWaveRDC::~CWaveRDC()
{
}



void CWaveRDC::Dir(int v)
{
    // not implemented yet
}



void CWaveRDC::ResizeZ(int n)
{
    m_DataZ.resize(n);
}

int CWaveRDC::DumpZ(const char* fname, int app)
{
    const char* mode = "wb";
    if (app == 1)
    {
        mode = "ab";
    }

    FILE* fp = nullptr;
    azfopen(&fp, fname, mode);
    if (!fp)
    {
        return -1;
    }

    int n = (int)(m_DataZ.size() * sizeof(m_DataZ[0]));
    fwrite(&m_DataZ[0], n, 1, fp);
    fclose(fp);

    return n;
}

int CWaveRDC::Compress(const CWaveletData& wd, bool fast)
{
    // Compress packs data into layer which is series of blocks each
    // consisting of bricks of two different length 'kl' and 'ks'. Each
    // brick represents one element of original unpacked data
    // (16-bit integer) which is shortened to exclude redundant bits.
    // Original data are taken from object wd. Packed data are
    // placed into array 'dataz'. Returns number of bytes
    // occupied by compressed data.

    int n = (int) wd.GetCount();
    int i, x;
    std::vector<int> dt(n);// = new int[n];
    TWavereal a, s = 0., ss = 0.;

    m_iShift = 0;
    m_nFreeBits = 8 * sizeof(unsigned int);

    // copy data to integer array
    for (i = 0; i < n; i++)
    {
        a = wd.m_Data[i];
        dt[i] = a > 0 ? int(a + 0.5f) : int(a - 0.5f);
    }

    // differentiate data if 2^7 bit in optz is set
    if ((m_Options & 128) != 0)
    {
        for (int i = n - 1; i > 0; i--)
        {
            dt[i] = short(dt[i] - dt[i - 1]);
        }
    }

    // calculate the DC shift and RMS
    for (i = 0; i < n; i++)
    {
        x = dt[i];
        s += x;
        ss += TWavereal(x * x);
    }

    s /= TWavereal(n);
    ss /= TWavereal(n);
    ss -= s * s;
    ss = 2.0f * sqrt(ss);
    m_iShift = short(s);

    // recalculate DC shift excluding such x that abs(x) > 2*RMS
    s = 0;
    for (i = 0; i < n; i++)
    {
        x = dt[i] - m_iShift;
        if (wabs(x) < ss)
        {
            s += x;
        }
    }
    s = (n > 0) ? s / TWavereal(n) : 0;

    m_iShift += short(s);

    if (m_iShift != 0)
    {
        for (i = 0; i < n; i++)
        {
            dt[i] = short(dt[i] - m_iShift);
        }
    }


    int h[17], g[17];
    int k, m;

    for (i = 0; i < 17; i++)
    {
        h[i] = 0;
        g[i] = 0;
    }

    for (i = 0; i < n; i++)
    {
        x = wabs(dt[i]);

        for (k = 0; (1 << k) < x; k++)
        {
            ;
        }

        m = 1 << k;                          // number of "short zeroes"

        if (x > 0)
        {
            k++;                     // add bit for sign
        }
        h[k] += 1;                         // histogram for number of bits

        if (m == dt[i])
        {
            g[k] += 1;          // histogram for "short zeroes"
        }
    }

    m_nBSW = m_nLargeInts = m_nSmallInts = 16;  // ks/kl - number of bits to pack short/long data samples
    int Lmin = 2 * n + 2;     // "compressed" length in bytes if ks = kl = 16
    int m0, L, ksw;

    m = 0;
    for (i = 16; i > 0; i--)
    {
        m += h[i];                         // # of long bricks
        if (m == 0)
        {
            m_nLargeInts = i - 1;              // # of bits for long brick
        }
        m0 = g[i - 1] < h[0] ? g[i - 1] : h[0];  // # of zeroes

        ksw = 16;
        if (m + m0 > 0)
        {
            for (ksw = 0; (1 << ksw) < (n / (m + m0) + 2); ksw++)
            {
                ;
            }
        }

        ksw = ksw < 15 ? ksw + 2 : 16;         // # of bits in BSW

        L = ((m + m0) * ksw + (n - m - m0) * (i - 1) + m * m_nLargeInts) / 8 + 1;
        if (L <= Lmin)
        {
            Lmin = L;
            m_nSmallInts = i - 1;
            m_nBSW = ksw;
            m_iZero = (m_nSmallInts == 0 || g[m_nSmallInts] > h[0]) ? 0 : 1 << (m_nSmallInts - 1); // zero in int domain
        }
    }
    int lcd = m_nLSW + (1 << (m_nBSW - 1)) + n;  // length of the cd array
    std::vector<unsigned int> cd(lcd, 0);// = new unsigned int[lcd];

    int ncd = m_nLSW;              // ncd counts elements of compressed array "cd"

    int j, jj = 0;
    unsigned int bsw = 0;            // Block Service Word
    int ns, maxns;                    // ns - # of short words in block
    maxns = (1 << (m_nBSW - 1)) - 1;        // maxns - max # of short bricks in block

    if (m_nSmallInts > 0)
    {
        m = 1 << (m_nSmallInts - 1);         // m - max # encoded with ks bits
    }

    int jmax;           // current limit on array index

    while (jj < n)
    {
        j = jj;
        jmax = ((j + maxns) < n) ? j + maxns : n;

        if (m_nSmallInts == 0)                    // count # of real 0
        {
            while (j < jmax && dt[j] == 0)
            {
                j++;
            }
        }
        else                           // count ns for |td|<=m, excluding zero
        {
            while (j < jmax && wabs(dt[j]) <= m && dt[j] != m_iZero)
            {
                j++;
            }
        }

        ns = j - jj;
        if (ns > maxns)
        {
            ns = maxns;    // maxns - max number of short words
        }

        j = jj + ns;

        //  fill BSW
        bsw = ns << 1;
        if (j < n)
        {
            if (m_nSmallInts == 0 && dt[j] != 0)
            {
                bsw += 1;
            }
            if (m_nSmallInts  > 0 && dt[j] != m_iZero)
            {
                bsw += 1;
            }
        }

        Push(bsw, &cd[0], ncd, m_nBSW);                          // push in BSW

        if (m_nSmallInts != 0)
        {
            Push(&dt[0], jj, &cd[0], ncd, m_nSmallInts, ns);         // push in a short words
        }

        jj += ns;

        if (j < n && dt[j] != m_iZero)
        {
            Push(&dt[0], j, &cd[0], ncd, m_nLargeInts, 1);   // push in a long word
        }

        jj++;
    }


    ncd++;
    cd[0] = ncd;                    // # of 32b words occupied by compressed data
    cd[1] = n;                      // # of 16b words occupied by original data

    unsigned short us = m_iZero;         // convert zero to unsigned short

    cd[2] = us;
    cd[2] = cd[2] << (8 * sizeof(us));

    us = m_iShift;                     // convert Shift to unsigned short
    bsw = 0;
    bsw = us;
    cd[2] |= bsw;                   // add Shift encoded as unsigned short

    bsw = 0;
    bsw |= ((m_Options & 0xFF) << 24);   // compression option
    bsw |= ((m_nSmallInts & 0x1F) << 19);     // length of short word
    bsw |= ((m_nLargeInts & 0x1F) << 14);     // length of long word
    bsw |= ((m_nBSW & 0x1F) << 9);    // length of the block service word
    cd[m_nLSW - 1] = bsw;

    // store compressed layer in new object then concatenate it with *this object

    if (ncd > 0)
    {
        for (int i = 0; i < ncd; i++)
        {
            m_DataZ.push_back(cd[i]);
        }
        m_nLayers += 1;
    }

    return (int)(m_DataZ.size() * sizeof(m_DataZ[0]));
}

int CWaveRDC::Push(int* dt, int j, unsigned int* cd, int& ncd,
    int k, int n)
{
    // Push takes "n" elements of data from "dt" and packs it as a series
    // of "n" words of k-bit length to the compressed data array "cd".
    // lcd counts the # of unsigned int words filled with compressed data.
    // This 'Push' makes data conversion:
    // x<0: x -> 2*(|x|-1)
    // x>0: x -> 2*(|x|-1) + 1
    // x=0: x -> 2*(zero-1) + 1

    int x, jj;
    unsigned int u;

    if (n == 0 || k == 0)
    {
        return 0;
    }
    jj = j;

    for (int i = 0; i < n; i++)
    {
        x = dt[jj++];
        u = x == 0 ? m_iZero - 1 : wabs(x) - 1;     // unsigned number
        u = x >= 0 ? (u << 1) + 1 : u << 1;        // add sign (first bit); 0/1 = -/+
        Push(u, cd, ncd, k);
    }

    return jj - j;
}

void CWaveRDC::Push(unsigned int& u, unsigned int* cd, int& ncd, int k)
{
    // Push takes unsigned int u  and packs it as a word k-bit leng
    // into the compressed data array "cd".
    // ncd counts the # of unsigned int words filled with compressed data.

    if (k == 0)
    {
        return;
    }

    if (k <= m_nFreeBits)
    {
        m_nFreeBits -= k;
        cd[ncd] |= u << m_nFreeBits;          // add whole word to cd[k]
    }
    else
    {
        cd[ncd++] |= u >> (k - m_nFreeBits);  // add upper bits to cd[k]
        m_nFreeBits += 8 * sizeof(u) - k;       // expected free space in cd[k+1]
        cd[ncd] = u << m_nFreeBits;           // save lower bits in cd[k+1]
    }
    return;
}

int CWaveRDC::UnCompress(CWaveletData& w, int m)
{
    // Unpacks data from the layer 'm' which is series of blocks each
    // consisting from bricks of two different length 'zl' and 'zs'.
    // Each brick represents one element of original unpacked data
    // (16-bit integer) which is shortened to exclude redundant bits.
    // Returns number of bytes occupied by unpacked data.

    if (m <= 0 || m > m_nLayers)
    {
        return 0;
    }

    // find layer number 'm'
    int ncd = 0;
    int mm = 1;

    while (mm < m)
    {
        if ((ncd + m_nLSW) > (int)m_DataZ.size())
        {
            //          cout <<" unCompress() error: layer "<< mm <<" is inconsistent.\n";
            return 0;
        }

        if (int(m_DataZ[ncd]) < m_nLSW)
        {
            //          cout <<" unCompress() error: layer "<< mm+1 <<" is inconsistent.\n";
            return 0;
        }
        ncd += m_DataZ[ncd];    // find beginning of the next layer
        mm++;
    }

    int ns, j = 0;

    int n  = m_DataZ[ncd + 1];  // number of uncompressed data elements
    unsigned int bsw;

    m_nFreeBits = 8 * sizeof(unsigned int);

    unsigned short us = (m_DataZ[ncd + 2] & 0xFFFF);      // unsigned shift
    m_iShift = us;                            // signed shift
    us = m_DataZ[ncd + 2] >> (8 * sizeof(us));   // unsigned zero
    m_iZero = us;

    m_Options = (m_DataZ[ncd + m_nLSW - 1] >> 24) & 0xFF;
    m_nSmallInts = (m_DataZ[ncd + m_nLSW - 1] >> 19) & 0x1F;
    m_nLargeInts = (m_DataZ[ncd + m_nLSW - 1] >> 14) & 0x1F;
    m_nBSW = (m_DataZ[ncd + m_nLSW - 1] >>  9) & 0x1F;

    std::vector<int> dt(n);


    ncd += m_nLSW;  // ncd is counter of elements of compressed array "dataz"

    while (j < n)
    {
        Pop(bsw, ncd, m_nBSW);          // read block service word
        ns = (bsw >> 1);

        if (ns < 0)
        {
            return -1;
        }


        if (m_nSmallInts > 0)
        {
            Pop(&dt[0], j, ncd, m_nSmallInts, ns);
        }
        else
        {
            for (int i = 0; i < ns; i++)
            {
                dt[j + i] = 0;
            }
        }

        j += ns;

        if (bsw & 1)
        {
            Pop(&dt[0], j, ncd, m_nLargeInts, 1);
        }
        else
        if (j < n)
        {
            dt[j] = m_iZero;
        }

        j++;
    }
    if ((m_Options & 128) != 0)
    {
        for (int i = 1; i < n; i++)
        {
            dt[i] += dt[i - 1];
        }
    }

    if (m_iShift != 0)
    {
        for (int i = 0; i < n; i++)
        {
            dt[i] += m_iShift;
        }
    }

    /*if (w.N != n)*/
    w.m_Data.resize(n);

    for (int i = 0; i < n; i++)
    {
        w.m_Data[i] = short(dt[i]);
    }

    return (int)w.GetCount() * 2;
}

void CWaveRDC::Pop(unsigned int& u, int& ncd, int k)
{
    // Pop unpacks one element of dataz. ncd - current position in the array
    // freebits - number of unpacked bits in dataz[ncd]

    int lul = 8 * sizeof(u);
    unsigned int mask = (1 << k) - 1;

    if (k == 0)
    {
        return;
    }

    if (k <= m_nFreeBits)
    {
        m_nFreeBits -= k;
        u = m_DataZ[ncd] >> m_nFreeBits;          // shift & save the word in u
    }
    else
    {
        u = m_DataZ[ncd++] << (k - m_nFreeBits);  // shift upper bits of the word
        m_nFreeBits += lul - k;                 // expected unpacked space in cd[k+1]
        u |= m_DataZ[ncd] >> m_nFreeBits;         // shift & add lower bits in u
    }
    u = mask & u;
    return;
}

int CWaveRDC::Pop(int* dt, int j, int& ncd, int k, int n)
{
    // Pop unpacks "n" elements of data which are stored in compressed
    // array "dataz" as a series of "n" words of k-bit length. Puts
    // unpacked data to integer array "dt" and returns number of words decoded

    int jj, x;
    unsigned int u;

    if (n == 0 || k == 0)
    {
        return 0;
    }
    jj = j;

    for (int i = 0; i < n; i++)
    {
        Pop(u, ncd, k);
        x = (u >> 1) + 1;
        if (!(u & 1))
        {
            x = -x;
        }

        if (x == m_iZero)
        {
            x = 0;
        }

        dt[jj++] = x;
    }
    return jj - j;
}

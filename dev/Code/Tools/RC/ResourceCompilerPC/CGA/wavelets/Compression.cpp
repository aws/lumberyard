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
#include "Compression.h"
#include "DaubechiWavelet.h"

namespace Wavelet
{
    int Compress(CWaveletData& in, TCompressetWaveletData& out, int Lwt, int Lbt,
        const TWavereal g1, const TWavereal g2, int np1, int np2)
    {
        // in - input data
        // Lw - decomposition depth for wavelet Tree
        // Lb - decomposition depth for wavelet binary Tree
        // g1, g2 - are value of losses in percents
        // np1, np2 - are orders of wavelets

        TWavereal m, r;
        TWavereal sf1, sf2;            // scaling factors
        TWavereal rf0 = 2.0f;     // layer 0 loss reduction factor
        TWaveData sl1, sl2;

        TWavereal f1 = sqrt(0.12f * fabs(g1));
        TWavereal f2 = sqrt(0.12f * fabs(g2));


        CWaveRDC z;
        z.m_Options = 0;
        CWaveletData wl;            // intermediate layer storage

        if (np1 <= 0)
        {
            np1 = 10;
        }
        if (np2 <= 0)
        {
            np2 = 10;
        }


        CDaubechiDWT wbt(np2, true);             // Daubechies binary tree wavelet

        TWavereal d, dmax;
        std::size_t nl1 = 0, nl2 = 0;

        if (Lwt >= 0)
        {
            nl1 = Lbt > 0 ? Lwt : Lwt + 1;
        }

        if (Lbt > 0)
        {
            nl2 = (size_t)1 << Lbt;
        }

        if (nl1 > 0)
        {
            sl1.resize(nl1);// = new float[nl1];
        }

        if (nl2 > 0)
        {
            sl2.resize(nl2);//=new float[nl2];
        }

        TWavereal skew = TWavereal(in.m_Data[in.GetCount() - 1] - in.m_Data[0]);
        TWavereal iskew = skew / TWavereal(in.GetCount() - 1);

        // eliminate difference between data edges by subtracting
        // linear function from data
        if (skew != 0.0f && (Lwt != 0 || Lbt != 0))
        {
            for (std::size_t i = 0; i < in.GetCount(); i++)
            {
                in.m_Data[i] -= i * iskew;
            }
        }

        if (Lbt > 0)
        {                           // binary tree processing
            wbt.DWT(in, Lbt);            // or from input

            for (std::size_t i = 1; i <= nl2; i++)
            {
                wbt.GetFreqLayer(wl, (int)(nl2 - i));
                if (f2 != 0.0f)
                {
                    wl.GetStatistics(m, r);
                    sf2 = (i != nl2) ? r * f2 : r * f2 / rf0;
                }
                else
                {
                    sf2 = 1.;
                }

                // find maximum data value and adjust
                // scaling coefficient to prevent data overflow
                if (sf2 != 0.0f)
                {
                    dmax = 0.0f;

                    for (std::size_t ii = 0; ii < wl.GetCount(); ii++)
                    {
                        d = fabs(wl.m_Data[ii]);
                        if (d > dmax)
                        {
                            dmax = d;
                        }
                    }
                    if (dmax / sf2 > 32767.0f)
                    {
                        sf2 = dmax / 32767.0f;
                    }
                }

                if ((sf2 != 0.0f) && (sf2 != 1.0f))
                {
                    wl *= 1.0f / sf2;
                }
                else
                {
                    sf2 = 1.0f;
                }

                if (i != nl2)
                {
                    z.Compress(wl, true);
                }
                else
                {
                    z.Compress(wl, false);
                }

                sl2[i - 1] = sf2;
            }
        }

        if ((Lwt == 0) && (Lbt == 0))
        {       // no wavelets
            if ((f1 > 0.0f) || (f2 > 0.0f))
            {           // scaling requested
                in.GetStatistics(m, r);
                sf1 = ((f1 < f2) && (f1 != 0.0f)) ? r * f1 : r * f2;
            }
            else
            {
                sf1 = 1.0f;           // no scaling requested
            }
            // adjust scaling if data RMS != 0
            if (sf1 != 0.0f)
            {
                dmax = 0.0f;
                for (std::size_t ii = 0; ii < wl.GetCount(); ii++)
                {
                    d = fabs(in.m_Data[ii]);
                    if (d > dmax)
                    {
                        dmax = d;
                    }
                }
                if (dmax / sf1 > 32767.0f)
                {
                    sf1 = dmax / 32767.0f;
                }
            }

            if ((sf1 != 0.0f) && (sf1 != 1.0f))
            {
                in *= 1.0f / sf1;
            }
            else
            {
                sf1 = 1.0f;
            }

            //    z.optz=128;
            z.Compress(in);
            sl1[0] = sf1;
            nl1 = 1;
        }


        int NN = (int)(3 + nl1 + nl2 + z.m_DataZ.size());
        if (Lwt != 0 || Lbt != 0)
        {
            NN += 1; // due to skew introduced starting from v.1.6
        }
        int* p;

        out.resize(NN);


        int i = 0;

        //      out[0] = ('W'<<24) + ('Z'<<16) + ('1'<<8) + ('6');
        out[i] = NN;        // output data length in 32-bit words
        out[++i] = (int)in.GetCount();//z.m_Data.size();        // uncompressed data length in 16-bit words
        out[++i] = (Lwt & 0xFF) << 24;
        out[i] |= (Lbt  & 0xFF) << 16;
        out[i] |= (np1 & 0xFF) << 8;
        out[i] |= (np2 & 0xFF) << 0;

        if (Lwt != 0 || Lbt != 0)
        {
            p = (int*) &skew;
            out[++i] = *p;
        }

        if (nl1 > 0)
        {
            p = (int*)&sl1[0];
            for (std::size_t j = 0; j < nl1; j++)
            {
                out[++i] = p[j];
            }
        }

        if (nl2 > 0)
        {
            p = (int*)&sl2[0];
            for (std::size_t j = 0; j < nl2; j++)
            {
                out[++i] = p[j];
            }
        }

        std::size_t jmax = z.m_DataZ.size();

        for (std::size_t j = 0; j < jmax; j++)
        {
            out[++i] = z.m_DataZ[j];
        }


        //return (int)((char*)&out[jmax] - (char*)&out[0]);//NN;
        return NN;
    }


    int UnCompress(TCompressetWaveletData& in, CWaveletData& out)
    {
        int* p;
        TWavereal* sl1 = NULL, * sl2 = NULL;
        int n;

        //      short aid, aiv;         // actual id and ver.# taken from input
        short swp = 0x1234;
        char* pswp = (char*) &swp;

        //aid = short(in[0] >> 16);
        //aiv = short(in[0] & 0xFFFF);

        int i = 0;
        int NN = in[i]; // length of input data in 32-bit words
        if (NN < 4)
        {
            return -1;
        }

        int nu = in[++i];             // length of uncompressed data in 16-bit words
        int Lwt = (in[++i] >> 24) & 0xFF; // number of Wavelet Tree decomposition levels
        int Lbt = (in[i] >> 16) & 0xFF;     // number of Binary Tree decomposition levels
        int np1 = (in[i] >> 8) & 0xFF;
        int np2 = in[i] & 0xFF;

        int nl1 = 0, nl2 = 0;       // number of elements in arrays sl1 and sl2
        if (Lwt >= 0)
        {
            nl1 = Lbt > 0 ? Lwt : Lwt + 1;
        }
        if (Lbt > 0)
        {
            nl2 = 1 << Lbt;
        }

        int nl = nl1 + nl2;     // number of layers in compressed data
        int nz = NN - 3 - nl;     // length of compressed data in 32-bit words
        if (Lwt != 0 || Lbt != 0)
        {
            nz--;
        }
        if (nz <= 0)
        {
            return -1;
        }

        TWavereal skew = 0.0f;
        TWavereal iskew = 0.0f;

        if (Lwt != 0 || Lbt != 0)
        {
            p = (int*) &skew;
            p[0] = in[++i];
            iskew = nu > 1 ? TWavereal(skew) / TWavereal(nu - 1) : 0.0f;
        }

        if (nl1 > 0)
        {
            sl1 = new TWavereal[nl1];
            p = (int*)sl1;
            for (int j = 0; j < nl1; j++)
            {
                p[j] = in[++i];
            }
        }

        if (nl2 > 0)
        {
            sl2 = new TWavereal[nl2];
            p = (int*)sl2;
            for (int j = 0; j < nl2; j++)
            {
                p[j] = in[++i];
            }
        }

        CWaveRDC z;
        z.ResizeZ(nz);
        //  z. N=nu;
        z.m_nLayers = nl;
        z.m_Options = 0;

        for (int j = 0; j < nz; j++)
        {
            z.m_DataZ[j] = in[++i];
        }

        if ((Lwt == 0) && (Lbt == 0) && (nl == 1))
        {
            n = z.UnCompress(out, 1);
            //      if (sl2[0] != 1.0f)
            out *= sl1[0];

            return nu;
        }

        CWaveletData wl;//

        int nt = nu / (1 << Lwt);          // binary tree data length

        CDaubechiDWT wbt(np2, 1);

        if (Lbt > 0)
        {
            wbt.m_WS.m_Data.resize(nt);
            wbt.m_nCurLevel = Lbt;
            wbt.m_bTree = true;
            wbt.GetMaxLevel();
        }

        if (Lbt > 0)
        {                       // binary tree processing
            for (int i = 1; i <= nl2; i++)
            {
                n = z.UnCompress(wl, Lwt + i);


                if (sl2[i - 1] != 1.0f)
                {
                    wl *= sl2[i - 1];
                }
                wbt.PutFreqLayer(wl, nl2 - i);
            }

            delete [] sl2;
        }


        if (Lbt > 0)
        {
            wbt.IDWT(wl, Lbt);       // reconstruct layer 0 from binary tree wavelet
        }
        else
        {
            n = z.UnCompress(wl, Lwt + 1);   // or take layer 0 from compressed data
            if (sl1[Lwt] != 1.)
            {
                wl *= sl1[Lwt];
            }
        }

        out = wl;           // or copy data from binary tree stage


        if (Lwt != 0 || Lbt != 0)
        {
            if (skew != 0.0f)
            {
                for (int j = 0; j < nu; j++)
                {
                    out.m_Data[j] += j * iskew;
                }
            }
        }

        return nu;
    }



    int CompressDCT(CWaveletData& in, TCompressetWaveletData& out, const TWavereal g)
    {
        CWaveRDC z;
        CDCT dct;

        TWavereal skew = TWavereal(in.m_Data[in.GetCount() - 1] - in.m_Data[0]);
        TWavereal iskew = skew / TWavereal(in.GetCount() - 1);

        // eliminate difference between data edges by subtracting
        // linear function from data
        if (skew != 0.0f)
        {
            for (std::size_t i = 0; i < in.GetCount(); i++)
            {
                in.m_Data[i] -= i * iskew;
            }
        }

        dct.FDCT(in);

        TWavereal f = sqrt(0.12f * fabs(g));
        TWavereal m, sf, r, dmax, d;
        TWavereal rf0 = 10.0f;        // layer 0 loss reduction factor


        if (f != 0.0f)
        {
            dct.m_WS.GetStatistics(m, r);
            //sf = (i !=nl2) ? r*f2 : r*f2/rf0;
            sf = r * f / rf0;
        }
        else
        {
            sf = 1.0f;
        }

        // find maximum data value and adjust
        // scaling coefficient to prevent data overflow
        if (sf != 0.0)
        {
            dmax = 0.0f;

            for (std::size_t ii = 0; ii < dct.m_WS.GetCount(); ii++)
            {
                d = fabs(dct.m_WS.m_Data[ii]);
                if (d > dmax)
                {
                    dmax = d;
                }
            }

            if (dmax / sf > 32767.0f)
            {
                sf = dmax / 32767.0f;
            }
        }

        if ((sf != 0.0f) && (sf != 1.0f))
        {
            dct.m_WS *= 1.0f / sf;
        }
        else
        {
            sf = 1.0f;
        }

        z.Compress(dct.m_WS, false);

        int newsize = 5 + (int)z.m_DataZ.size();
        int* p;

        out.resize(newsize);

        int i = 0;

        out[0] = ('D' << 24) + ('Z' << 16) + ('1' << 8) + ('6');
        out[++i] = newsize;     // output data length in 32-bit words
        out[++i] = (int)in.GetCount();//z.m_Data.size();        // uncompressed data length in 16-bit words

        p = (int*)&skew;
        out[++i] = *p;

        p = (int*)&sf;
        out[++i] = *p;

        std::size_t jmax = z.m_DataZ.size();

        for (std::size_t j = 0; j < jmax; j++)
        {
            out[++i] = z.m_DataZ[j];
        }


        //      return (int)((char*)out.end() - (char*)out.begin());//NN;
        return newsize;
    }



    int UnCompressDCT(TCompressetWaveletData& in, CWaveletData& out)
    {
        TWavereal skew, sf, iskew;
        int* p;

        int i = 0;
        int oldsize = in[++i];  // length of input data in 32-bit words
        if (oldsize < 4)
        {
            return -1;
        }

        int uncompressed = in[++i];           // length of uncompressed data in 16-bit words

        p = (int*) &skew;
        p[0] = in[++i];
        iskew = uncompressed > 1 ? TWavereal(skew) / TWavereal(uncompressed - 1) : 0.0f;

        p = (int*) &sf;
        p[0] = in[++i];

        int nz = oldsize - 5;

        CWaveRDC z;
        z.ResizeZ(nz);
        //  z. N=nu;
        z.m_nLayers = 1;
        z.m_Options = 0;

        for (int j = 0; j < nz; j++)
        {
            z.m_DataZ[j] = in[++i];
        }

        int n = z.UnCompress(out, 1);
        //      if (sl2[0] != 1.0f)
        //assert(n / sizeof(TWavereal) == uncompressed);
        out *= sf;


        CDCT dct;

        dct.m_WS.m_Data = out.m_Data;

        dct.IDCT(out);


        if (skew != 0.0f)
        {
            for (int j = 0; j < uncompressed; j++)
            {
                out.m_Data[j] += j * iskew;
            }
        }

        return uncompressed;
    }
}
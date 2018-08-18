/*
Based in part on Perlin noise
http ://mrl.nyu.edu/~perlin/doc/oscar.html#noise
Copyright(c) Ken Perlin

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"),
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and / or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
*/


// Noise.cpp: implementation of the CNoise class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Noise.h"
#include "Util/DynamicArray2D.h"

#pragma warning (push)
#pragma warning (disable : 4244)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CNoise::CNoise()
{
}

CNoise::~CNoise()
{
}

__forceinline float CNoise::Spline(float x, float* knot)
{
    /*
    * spline(...)
    *
    *   interpolate a 1d spline
    *   taken from _Texturing & Modeling: A Procedural Approach_
    *       chapter 2 - by Darwyn Peachey
    */

    // Const is a bit faster, nknots does not need to be of different size
    const int nknots = 4;
    const int nspans = nknots - 3;

    int span;
    float c0, c1, c2, c3;   // coefficients of the cubic

    /*
    if( nspans < 1)
    {
        // illegal
        return 0;
    }
    */

    // find the appropriate 4-point span of the spline
    x = (x < 0 ? 0 : (x > 1 ? 1 : x)) * nspans;
    span = ftoi(x); // Slow cast is already fast because of the overloaded C float/int conversion
    if (span >= nknots -3)
    {
        span = nknots - 3;
    }
    x -= span;
    knot += span;

    // evaluate the span cubic at x using horner's rule
    c3 = CR00*knot[0] + CR01*knot[1]
        + CR02*knot[2] + CR03*knot[3];
    c2 = CR10*knot[0] + CR11*knot[1]
        + CR12*knot[2] + CR13*knot[3];
    c1 = CR20*knot[0] + CR21*knot[1]
        + CR22*knot[2] + CR23*knot[3];
    c0 = CR30*knot[0] + CR31*knot[1]
        + CR32*knot[2] + CR33*knot[3];

    return ((c3*x + c2)*x + c1)*x + c0;
}

void CNoise::FracSynthPass(CDynamicArray2D* hBuf, float freq, float zscale, int xres, int zres, BOOL bLoop)
{
    // WARNING: Function is messy, and lots of dirty optimisation is used...

    /*
    * FracSynthPass(...)
    *
    *   generate basic points
    *   interpolate along spline & scale
    *   add to existing hbuffer
    */

    int i;
    int x, z;
    float* val = 0;

    float dfx, dfz;
    float* zknots = 0, * knots = 0;
    float xk, zk;
    float* hlist = 0;
    float* buf = 0;

    // how many to generate (need 4 extra for smooth 2d spline interpolation)
    const int max = ceil(freq) + 2;
    // delta x and z - pixels per spline segment
    dfx = xres / (freq-1);
    dfz = zres / (freq-1);
    
    auto cleanup = [=]
    {
        if(val)
        {
            free(val);
        }
        if(zknots)
        {
            free(zknots);
        }
    };

    // the generated values - to be equally spread across buf
    val = (float*)calloc(sizeof(float)*max*max, 1);
    if (!val)
    {
        CLogFile::WriteLine("Critical calloc error during noise calculation !");
        cleanup();
        return;
    }

    // intermediately calculated spline knots (for 2d)
    zknots = (float*)calloc(sizeof(float)*max, 1);
    if (!zknots)
    {
        CLogFile::WriteLine("Critical calloc error during noise calculation !");
        cleanup();
        return;
    }

    // horizontal lines through knots
    hlist = (float*)calloc(sizeof(float)*max*xres, 1);
    if (!hlist)
    {
        CLogFile::WriteLine("Critical calloc error during noise calculation !");
        cleanup();
        return;
    }

    // local buffer - to be added to hbuf
    buf = (float*)calloc(sizeof(float)*xres*zres, 1);
    if (!buf)
    {
        CLogFile::WriteLine("Critical calloc error during noise calculation !");
        cleanup();
        return;
    }

    // start at -dfx, -dfz - generate knots
    for (z = 0; z<max; z++)
    {
        int zm = z*max;

        for (x = 0; x<max; x++)
        {
            val[zm+x] = cry_random(-1.0f, 1.0f);
        }
    }


    if (bLoop)
    {
        for (z = 0; z<max; z++)
        {
            val[z*max+0] = val[z*max+max-1];
            val[z*max+1] = val[z*max+max-2];
        }

        for (x = 0; x<max; x++)
        {
            val[0*max+x] = val[(max-1)*max+x];
            val[1*max+x] = val[(max-2)*max+x];
        }
    }

    // interpolate horizontal lines through knots
    float* pOldHList = hlist;
    for (i = 0; i<max; i++)
    {
        knots = &val[i*max];
        xk = 0;
        for (x = 0; x<xres; x++)
        {
            // Incremental pointer is faster
            *hlist++ = Spline(xk/dfx, knots);
            xk += 1.0f;
            if (xk >= dfx)
            {
                xk -= dfx;
                knots++;
            }
        }
    }
    hlist = pOldHList;

    // interpolate all vertical lines
    float* pOldBuf = buf;
    for (x = 0; x<xres; x++)
    {
        zk = 0;
        knots = zknots;
        // build knot list
        for (i = 0; i<max; i++)
        {
            knots[i] = hlist[i*xres+x];
        }

        for (z = 0; z<zres; z++)
        {
            if (zk >= dfz)
            {
                zk -= dfz;
                knots++;
                // Shift zknots by knots + 4 indicies (used in Spline function)
                // shouldn't be bigger than number of elements in zknots (which is 'max')
                assert(knots - zknots + 4 <= max);
            }

            // Incremental pointer is *much* faster
            *buf++ = Spline(zk/dfz, /*4,*/ knots) * zscale;
            zk += 1.0f;
        }
    }
    buf = pOldBuf;

    // update hBuf (Some hacks are used to get around the pointer overhead)
    float** pArray = hBuf->m_Array;
    float* pArray2 = NULL;
    for (z = 0; z<zres; z++)
    {
        pArray2 = pArray[z];
        for (x = 0; x<xres; x++)
        {
            //hBuf->m_Array[z][x] += buf[z*xres+x];
            pArray2 [x] += *buf++;
            pArray2 [x] = MAX(MIN(pArray2 [x], 512000.0f), -512000.0f);
        }
    }

    cleanup();
    if (pOldBuf)
    {
        free(pOldBuf);
    }
    if (pOldHList)
    {
        free(pOldHList);
    }
}

#pragma warning (pop)
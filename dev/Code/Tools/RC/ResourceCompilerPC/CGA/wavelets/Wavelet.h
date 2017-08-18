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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_WAVELETS_WAVELET_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_WAVELETS_WAVELET_H
#pragma once



#define PI 3.141592653589793f

typedef float TWavereal;
//typedef std::complex<TWavereal> TWavecomplex;
typedef std::vector<TWavereal> TWaveData;

// Class with wavelet data.
class CWaveletData
{
public:
    // data

    TWaveData m_Data;
    //sampling rate
    TWavereal   m_Rate;

    // ctors
    CWaveletData()
        : m_Rate(.0) { }
    CWaveletData(int num)
        : m_Data(num)
        , m_Rate(.0) {}
    CWaveletData(int num, TWavereal rate)
        : m_Data(num)
        , m_Rate(rate) {}


    virtual ~CWaveletData() {}


    //  Calculates and returns mean and root mean square of the signal.
    void GetStatistics(TWavereal& mean, TWavereal& rms)
    {
        mean = 0.;
        rms = 0.;

        for (TWaveData::iterator it = m_Data.begin(), end = m_Data.end(); it != end; ++it)
        {
            mean += *it;
            rms += (*it) * (*it);
        }

        mean /= (TWavereal) m_Data.size();
        rms /= (TWavereal) m_Data.size();
        //rms -= mean*mean;

        if (rms != 0.0f)
        {
            rms = sqrt(rms);
        }
    }

    std::size_t GetCount() const
    {
        return m_Data.size();
    }

    TWavereal* GetData(size_t offset)
    {
        assert(m_Data.begin() + offset <= m_Data.end());
        return (TWavereal*)(&m_Data[0] + offset);
    }

    CWaveletData& operator+=(const TWavereal c)
    {
        for (TWaveData::iterator it = m_Data.begin(), end = m_Data.end(); it != end; ++it)
        {
            *it += c;
        }
        return *this;
    }

    /*********************************************************************
    * subtract constant shift from all elements of array
    *********************************************************************/
    CWaveletData& operator-=(const TWavereal c)
    {
        for (TWaveData::iterator it = m_Data.begin(), end = m_Data.end(); it != end; ++it)
        {
            *it -= c;
        }
        return *this;
    }

    /*********************************************************************
    * multiply all elements of data array by constant
    *********************************************************************/
    CWaveletData& operator*=(const TWavereal c)
    {
        for (TWaveData::iterator it = m_Data.begin(), end = m_Data.end(); it != end; ++it)
        {
            (*it) *= c;
        }
        return *this;
    }
};


// Class with transformed data
class CWaveletSeries
    : public CWaveletData
{
public:
    CWaveletSeries()
        : CWaveletData() { }
    CWaveletSeries(int num)
        : CWaveletData(num){}
    CWaveletSeries(int num, TWavereal rate)
        : CWaveletData(num, rate) {}
};


enum EBorderBehavior
// constants which rule data boundary processing, for variable 'border'
{
    eB_PAD_ZERO = 0,
    eB_RECYCLE = 1,
    eB_MIRROR = 2,
    eB_PAD_CONSTANT = 3,  // padding constants is equal to data values on edges
    eB_EXTRAPOLATION = 4
};

class CBaseDT
{
public:
    // Spectrum data
    CWaveletSeries  m_WS;

    CBaseDT() {};
    CBaseDT(unsigned int num)
        : m_WS(num) {};

    void Denoise(TWavereal cut);
    int LastMaximal(TWavereal val);
};
// Base class for discrete wavelet transform
class CDWT
    : public CBaseDT
{
    //  CDWT() : m_pWS(0) {;};

public:
    //ctors
    CDWT(int num = 1, bool tree = false, EBorderBehavior border = eB_RECYCLE)
        : m_bTree(tree)
        , CBaseDT(num)
        , m_Border(border)
    {
        m_nCurLevel = 0;
    }

    // virtual dtor
    virtual ~CDWT() {}
    // Direct discrete wavelet transform
    virtual void DWT(const CWaveletData&, int level = -1);
    // Inverse wavelet transform
    virtual void IDWT(CWaveletData&, int level = -1);
    // get layer with given number
    void GetLayer(CWaveletData&, int);
    // replace layer with given number
    void PutLayer(CWaveletData&, int);
    // Get frequency layer
    void GetFreqLayer(CWaveletData& td, int k);
    // Put frequency layer
    void PutFreqLayer(CWaveletData& td, int k);
    // Simple denoise algorithm

    //protected:
    // Direct discrete wavelet transform
    virtual void DWT(int level = 1);
    // Direct discrete wavelet transform
    virtual void IDWT(int level = 1);
    // return maximum level of composition
    int GetMaxLevel() const;
    // One step of decomposition
    virtual void Decompose(int, int) = 0;
    // One step of reconstruction
    virtual void Reconstruct(int, int) = 0;
    // Get wavelet layer number for given level and layer.
    int GetWaveletLayerNumber(int level, int layer);
    // Get frequency ID number for given level and layer.
    int GetFrequencyLayerNumber(int level, int IdL);
    // Get the layer number in binary tree for given level and frequency ID number.
    int GetLayerNumber(int level, int IdF);





    //protected:
    // Number of filter coefficients
    unsigned int    m_nCoeffs;
    // Current level of decomposition
    int m_nCurLevel;
    // Is it tree decomposition?
    bool m_bTree;
    // Border behavior
    EBorderBehavior m_Border;
};



// Discrete cosine transform
class CDCT
    : public CBaseDT
{
private:
    std::vector<TWavereal> m_cosine;

public:
    //Forward discrete cosine transform
    void FDCT(const CWaveletData& data);
    //Inverse discrete cosine transform
    void IDCT(CWaveletData& data, int lastEffective = -1);

    //  CWaveletSeries m_WS;
};




#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_WAVELETS_WAVELET_H

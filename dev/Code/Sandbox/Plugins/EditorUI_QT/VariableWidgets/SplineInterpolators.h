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
#ifndef SplineInterpolators_h__
#define SplineInterpolators_h__

#include "ISplines.h"
#include "FinalizingSpline.h"

//Helper class for splines used in the particle editor
template <typename T>
class FinalizingOptSpline
    : public spline::FinalizingSpline<typename spline::OptSpline<T>::source_spline, spline::OptSpline<T> >
{
public:
    typedef spline::FinalizingSpline<typename spline::OptSpline<T>::source_spline, spline::OptSpline<T> > Base;
    FinalizingOptSpline()
    {
        Base::SetFinal(new spline::OptSpline<float>);
    }
    ~FinalizingOptSpline()
    {
        delete Base::m_pFinal;
    }
};

/*Added to avoid referencing cry3dengine in a color editor...Steven @conffx*/
/*original can be found at "dev\Code\CryEngine\Cry3DEngine\TimeOfDay.cpp"*/
class CBezierSplineFloat
    : public spline::CBaseSplineInterpolator < float, spline::BezierSpline<float> >
{
public:
    float m_fMinValue;
    float m_fMaxValue;

    virtual int GetNumDimensions() { return 1; };

    virtual void Interpolate(float time, ValueType& value)
    {
        value_type v;
        if (interpolate(time, v))
        {
            ToValueType(v, value);
        }
        // Clamp values
        //value[0] = clamp_tpl(value[0],m_fMinValue,m_fMaxValue);
    }

    //////////////////////////////////////////////////////////////////////////
    void SerializeSpline(XmlNodeRef& node, bool bLoading)
    {
        if (bLoading)
        {
            string keystr = node->getAttr("Keys");

            resize(0);
            int curPos = 0;
            uint32 nKeys = 0;
            string key = keystr.Tokenize(",", curPos);
            while (!key.empty())
            {
                ++nKeys;
                key = keystr.Tokenize(",", curPos);
            }
            ;
            reserve_keys(nKeys);

            curPos = 0;
            key = keystr.Tokenize(",", curPos);
            while (!key.empty())
            {
                float time, v;
                int flags = 0;

                int res = azsscanf(key, "%g:%g:%d", &time, &v, &flags);
                if (res != 3)
                {
                    res = azsscanf(key, "%g:%g", &time, &v);
                    if (res != 2)
                    {
                        continue;
                    }
                }
                ValueType val;
                val[0] = v;
                int keyIndex = InsertKey(time, val);
                SetKeyFlags(keyIndex, flags);
                key = keystr.Tokenize(",", curPos);
            }
            ;
        }
        else
        {
            string keystr;
            string skey;
            for (int i = 0; i < num_keys(); i++)
            {
                skey.Format("%g:%g:%d,", key(i).time, key(i).value, key(i).flags);
                keystr += skey;
            }
            node->setAttr("Keys", keystr);
        }
    }
};

#endif // SplineInterpolators_h__

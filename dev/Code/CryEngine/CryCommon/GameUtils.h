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

// Description : Implements some utilities.


#ifndef CRYINCLUDE_CRYCOMMON_GAMEUTILS_H
#define CRYINCLUDE_CRYCOMMON_GAMEUTILS_H
#pragma once


//! normalize the val to 0-360 range
ILINE f32 Snap_s360(f32 val)
{
    if (val < 0.0f)
    {
        val = f32(360.0f + fmod_tpl(val, 360.0f));
    }
    else
    if (val >= 360.0f)
    {
        val = f32(fmod_tpl(val, 360.0f));
    }
    return val;
}

//! normalize the value to -PI, +PI range
ILINE f32 AngleWrap_PI(f32 val)
{
    return (float)fsel(-(val - gf_PI), (float)fsel(val + gf_PI, val, val + (2.f * gf_PI)), val - (2.f * gf_PI));
}

//! normalize the val to -180, 180 range
ILINE f32 Snap_s180(f32 val)
{
    if (val > -180.0f && val < 180.0f)
    {
        return val;
    }
    val = Snap_s360(val);
    if (val > 180.0f)
    {
        return -(360.0f - val);
    }
    return val;
}

//////////////////////////////////////////////////////////////////////////
// GetClampedFraction() - returns a decimal percentage (0.0 to 1.0) depending on where the fValue argument
// passed is within the [fStart, fEnd] interval. If fValue is outside the interval, it's clamped to be the
// closest endpoint
//////////////////////////////////////////////////////////////////////////
ILINE float GetClampedFraction(float fValue, float fStart, float fEnd)
{
    CRY_ASSERT((fStart - fEnd) != 0.0f);

    float fFraction = (fValue - fStart) / (fEnd - fStart);
    Limit(fFraction, 0.0f, 1.0f);
    return fFraction;
}

//interpolate angle
inline void Interpolate(Ang3& actual, const Ang3& goal, float speed, float frameTime, float limit = 0)
{
    Ang3 delta(goal - actual);
    delta.x = Snap_s180(delta.x);
    delta.y = Snap_s180(delta.y);
    delta.z = Snap_s180(delta.z);

    if (limit > 0.001f)
    {
        delta.x = max(min(delta.x, limit), -limit);
        delta.y = max(min(delta.y, limit), -limit);
        delta.z = max(min(delta.z, limit), -limit);
    }

    actual += delta * min(frameTime * speed, 1.0f);
}

//interpolate vector
inline void Interpolate(Vec3& actual, const Vec3& goal, float speed, float frameTime, float limit = 0)
{
    Vec3 delta(goal - actual);

    if (limit > 0.001f)
    {
        float len(delta.len());

        if (len > limit)
        {
            delta /= len;
            delta *= limit;
        }
    }

    actual += delta * min(frameTime * speed, 1.0f);
}

//interpolate vector
inline void Interpolate(Vec4& actual, const Vec4& goal, float speed, float frameTime, float limit = 0)
{
    Vec4 delta(goal - actual);

    if (limit > 0.001f)
    {
        float len(delta.GetLength());

        if (len > limit)
        {
            delta /= len;
            delta *= limit;
        }
    }

    actual += delta * min(frameTime * speed, 1.0f);
}

//interpolate float
ILINE void Interpolate(float& actual, float goal, float speed, float frameTime, float limit = 0)
{
    float delta(goal - actual);

    if (limit > 0.001f)
    {
        delta = max(min(delta, limit), -limit);
    }

    actual += delta * min(frameTime * speed, 1.0f);
}

//interpolate float - wraps at minValue/maxValue
inline void InterpolateWrapped(float& actual, float goal, float minValue, float maxValue, float speed, float frameTime, float limit = 0)
{
    assert(minValue < maxValue);
    assert(minValue <= goal);
    assert(goal <= maxValue);
    assert(minValue <= actual);
    assert(actual <= maxValue);

    float range = maxValue - minValue;
    float movement = 0.0f;
    if (goal < actual)
    {
        if (actual - goal > range * 0.5f)
        {
            movement = goal + range - actual;
        }
        else
        {
            movement = goal - actual;
        }
    }
    else
    {
        if (goal - actual > range * 0.5f)
        {
            movement = goal - (actual - range);
        }
        else
        {
            movement = goal - actual;
        }
    }

    if (limit > 0.001f)
    {
        movement = max(min(movement, limit), -limit);
    }

    actual += movement * min(frameTime * speed, 1.0f);
}


// Gives the (shortest) angle difference between two signed angles.
//
// a and b are signed angles, in radians.
// Result is in radians, and always in [0, PI].
inline float AngleDifference(float a, float b)
{
    float diff = fmod_tpl(fabs_tpl(a - b), gf_PI2);
    return (float)fsel(diff - gf_PI, gf_PI2 - diff, diff);
}

// not the best place for this
namespace GameUtils
{
    inline void timeToString(time_t value, string& outString)
    {
        struct tm dateTime;

        // convert the time to a string
        gEnv->pTimer->SecondsToDateUTC(value, dateTime);

        // use ISO 8601 UTC
        outString.Format("%d-%02d-%02dT%02d:%02d:%02dZ",
            dateTime.tm_year + 1900, dateTime.tm_mon + 1, dateTime.tm_mday,
            dateTime.tm_hour, dateTime.tm_min, dateTime.tm_sec);
    }

    inline time_t stringToTime(const char* str)
    {
        if (str)
        {
            struct tm timePtr;
            // convert the time to from a string to a time struct
            int n = azsscanf(str, "%i-%02d-%02dT%02d:%02d:%02d",
                    &timePtr.tm_year, &timePtr.tm_mon, &timePtr.tm_mday,
                    &timePtr.tm_hour, &timePtr.tm_min, &timePtr.tm_sec);
            if (n != 6)
            {
                return (time_t) 0;
            }

            timePtr.tm_year -= 1900;
            timePtr.tm_mon -= 1;
            timePtr.tm_isdst = -1;
            return gEnv->pTimer->DateToSecondsUTC(timePtr);
        }
        return (time_t) 0;
    }

    inline const char* GetPrettyAsyncTime()
    {
        const CTimeValue& t = gEnv->pTimer->GetAsyncTime();
        int64 milliseconds = t.GetMilliSecondsAsInt64();
        int64 seconds = milliseconds / 1000;
        int64 minutes = seconds / 60;
        int64 hours = minutes / 60;
        milliseconds -= seconds * 1000;
        seconds -= minutes * 60;
        minutes -= hours * 60;
        static CryFixedStringT<16> sTimeString;
        sTimeString.Format("%02d:%02d:%02d.%03d", (int)hours, (int)minutes, (int)seconds, (int)milliseconds);
        return sTimeString.c_str();
    }
};

template<class T, class U>
struct TKeyValuePair
{
    T key;
    U value;
};

template<class T, class U, class V>
T GetKeyByValue(const V& value, TKeyValuePair<T, U> list[], size_t list_size)
{
    for (size_t i = 0; i < list_size; ++i)
    {
        if (value == list[i].value)
        {
            return list[i].key;
        }
    }

    return list[0].key;
}

template<class T, class U>
T GetKeyByValue(const char* value, TKeyValuePair<T, U> list[], size_t list_size)
{
    return GetKeyByValue(CONST_TEMP_STRING(value), list, list_size);
}

template<class T, class U, class V>
U GetValueByKey(const V& key, TKeyValuePair<T, U> list[], size_t list_size)
{
    for (size_t i = 0; i < list_size; ++i)
    {
        if (key == list[i].key)
        {
            return list[i].value;
        }
    }
    return list[0].value;
}

template<class T, class U>
U GetValueByKey(const char* key, TKeyValuePair<T, U> list[], size_t list_size)
{
    return GetValueByKey(CONST_TEMP_STRING(key), list, list_size);
}

//first element of array is default value
#define KEY_BY_VALUE(v, g) GetKeyByValue(v, g, sizeof(g) / sizeof(g[0]))
#define VALUE_BY_KEY(v, g) GetValueByKey(v, g, sizeof(g) / sizeof(g[0]))

template<class T, class U, class V>
bool HasValue(const V& value, TKeyValuePair<T, U> list[], size_t list_size)
{
    for (int i = 0; i < list_size; ++i)
    {
        if (value == list[i].value)
        {
            return true;
        }
    }
    return false;
}

template<class T, class U, class V>
bool HasKey(const V& key, TKeyValuePair<T, U> list[], size_t list_size)
{
    for (int i = 0; i < list_size; ++i)
    {
        if (key == list[i].key)
        {
            return true;
        }
    }
    return false;
}

template<class T, class U>
bool HasValue(const char* value, TKeyValuePair<T, U> list[], size_t list_size)
{
    return HasValue(CONST_TEMP_STRING(value), list, list_size);
}

template<class T, class U>
bool HasKey(const char* key, TKeyValuePair<T, U> list[], size_t list_size)
{
    return HasKey(CONST_TEMP_STRING(key), list, list_size);
}

#define HAS_VALUE(v, g)  HasValue(v, g, sizeof(g) / sizeof(g[0]))
#define HAS_KEY(v, g)        HasKey  (v, g, sizeof(g) / sizeof(g[0]))

inline bool TimePassedCheck(CTimeValue& t, const CTimeValue& cur_time, const CTimeValue& step)
{
    if (fabs((cur_time - t).GetSeconds()) > step.GetSeconds())
    {
        t = cur_time;
        return true;
    }
    return false;
}

struct SChangeTrack
{
    SChangeTrack()
        : hasChange(false){}

    template<class T>
    inline bool CompareAndSet(T& old_value, const T& new_value)
    {
        if (old_value != new_value)
        {
            old_value = new_value;
            hasChange = true;
            return true;
        }
        return false;
    }

    inline bool CompareAndSet(string& old_value, const char* new_value)
    {
        if (old_value != new_value)
        {
            old_value = new_value;
            hasChange = true;
            return true;
        }
        return false;
    }

    void Changed(){hasChange = true; }
    bool IsChanged() const{return hasChange; }

    bool hasChange;
};

struct SStringPool
{
    inline const char* AddString(const char* str)
    {
        std::set<string>::const_iterator it = m_pool.find(CONST_TEMP_STRING(str));

        if (it == m_pool.end())
        {
            return m_pool.insert(str).first->c_str();
        }
        else
        {
            return it->c_str();
        }
    }

    const char* operator()(const char* str)
    {
        return AddString(str);
    }
    void Reset()
    {
        m_pool.clear();
    }
    std::set<string> m_pool;
};

#endif // CRYINCLUDE_CRYCOMMON_GAMEUTILS_H

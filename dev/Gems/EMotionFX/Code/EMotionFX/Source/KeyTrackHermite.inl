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

// default constructor
template <class ReturnType, class StorageType>
KeyTrackHermite<ReturnType, StorageType>::KeyTrackHermite()
{
    mTangents = nullptr;
}


// extended constructor
template <class ReturnType, class StorageType>
KeyTrackHermite<ReturnType, StorageType>::KeyTrackHermite(uint32 nrKeys)
{
    SetNumKeys(nrKeys);
}


// destructor
template <class ReturnType, class StorageType>
KeyTrackHermite<ReturnType, StorageType>::~KeyTrackHermite()
{
    ClearKeys();
    MCore::Free(mTangents);
}


// clear all keys
template <class ReturnType, class StorageType>
void KeyTrackHermite<ReturnType, StorageType>::ClearKeys()
{
    mKeys.Clear();
    MCore::Free(mTangents);
    mTangents = nullptr;
}


// init the keytrack
template <class ReturnType, class StorageType>
void KeyTrackHermite<ReturnType, StorageType>::Init()
{
    const uint32 numKeys = mKeys.GetLength();

    // check all key time values, so we are sure the first key start at time 0
    if (numKeys == 0)
    {
        MCore::Free(mTangents);
        mTangents = nullptr;
        return;
    }

    // get the time value of the first key, which is our minimum time
    const float minTime = mKeys[0].GetTime();

    // if it's not equal to zero, we have to correct it (and all other keys as well)
    if (minTime > 0.0f)
    {
        for (uint32 i = 0; i < numKeys; ++i)
        {
            mKeys[i].SetTime(mKeys[i].GetTime() - minTime);
        }
    }

    //-----------------
    // allocate the new tangents
    mTangents = (ReturnType*)MCore::Realloc(mTangents, sizeof(ReturnType) * numKeys, EMFX_MEMCATEGORY_MOTIONS_KEYTRACKS);

    // calculate all tangents
    for (uint32 i = 0; i < numKeys; ++i)
    {
        // if there is no previous or next key
        if (i == 0 || i == numKeys - 1)
        {
            ReturnType tangent;
            MCore::MemSet(&tangent, 0, sizeof(ReturnType));
            mTangents[i] = tangent;
        }
        else    // if there is a previous or next key
        {
            const ReturnType& prevValue = mKeys[i - 1].GetValue();
            const ReturnType& value     = mKeys[i  ].GetValue();
            const ReturnType& nextValue = mKeys[i + 1].GetValue();
            mTangents[i] = (0.5f * (value - prevValue)) + (0.5f * (nextValue - value));
        }
    }
}


template <class ReturnType, class StorageType>
MCORE_INLINE KeyFrame<ReturnType, StorageType>* KeyTrackHermite<ReturnType, StorageType>::GetKey(uint32 nr)
{
    MCORE_ASSERT(nr < mKeys.GetLength());
    return &mKeys[nr];
}


template <class ReturnType, class StorageType>
MCORE_INLINE KeyFrame<ReturnType, StorageType>* KeyTrackHermite<ReturnType, StorageType>::GetFirstKey()
{
    return (mKeys.GetLength() > 0) ? &mKeys[0] : nullptr;
}


template <class ReturnType, class StorageType>
MCORE_INLINE KeyFrame<ReturnType, StorageType>* KeyTrackHermite<ReturnType, StorageType>::GetLastKey()
{
    return (mKeys.GetLength() > 0) ? &mKeys.GetLast() : nullptr;
}



template <class ReturnType, class StorageType>
MCORE_INLINE const KeyFrame<ReturnType, StorageType>* KeyTrackHermite<ReturnType, StorageType>::GetKey(uint32 nr) const
{
    MCORE_ASSERT(nr < mKeys.GetLength());
    return &mKeys[nr];
}


template <class ReturnType, class StorageType>
MCORE_INLINE const KeyFrame<ReturnType, StorageType>* KeyTrackHermite<ReturnType, StorageType>::GetFirstKey() const
{
    return (mKeys.GetLength() > 0) ? &mKeys[0] : nullptr;
}


template <class ReturnType, class StorageType>
MCORE_INLINE const KeyFrame<ReturnType, StorageType>* KeyTrackHermite<ReturnType, StorageType>::GetLastKey() const
{
    return (mKeys.GetLength() > 0) ? &mKeys.GetLast() : nullptr;
}


template <class ReturnType, class StorageType>
MCORE_INLINE float KeyTrackHermite<ReturnType, StorageType>::GetFirstTime() const
{
    const KeyFrame<ReturnType, StorageType>* key = GetFirstKey();
    return (key) ? key->GetTime() : 0.0f;
}


template <class ReturnType, class StorageType>
MCORE_INLINE float KeyTrackHermite<ReturnType, StorageType>::GetLastTime() const
{
    const KeyFrame<ReturnType, StorageType>* key = GetLastKey();
    return (key) ? key->GetTime() : 0.0f;
}


template <class ReturnType, class StorageType>
MCORE_INLINE uint32 KeyTrackHermite<ReturnType, StorageType>::GetNumKeys() const
{
    return mKeys.GetLength();
}


template <class ReturnType, class StorageType>
MCORE_INLINE void KeyTrackHermite<ReturnType, StorageType>::AddKey(float time, const ReturnType& value)
{
    #ifdef MCORE_DEBUG
    if (mKeys.GetLength() > 0)
    {
        MCORE_ASSERT(time >= mKeys.GetLast().GetTime());
    }
    #endif

    // not the first key, so add on the end
    mKeys.Add(KeyFrame<ReturnType, StorageType>(time, value));
}


// find a key at a given time
template <class ReturnType, class StorageType>
MCORE_INLINE uint32 KeyTrackHermite<ReturnType, StorageType>::FindKeyNumber(float curTime) const
{
    return KeyFrameFinder<ReturnType, StorageType>::FindKey(curTime, mKeys.GetReadPtr(), mKeys.GetLength());
}


// find a key at a given time
template <class ReturnType, class StorageType>
MCORE_INLINE KeyFrame<ReturnType, StorageType>* KeyTrackHermite<ReturnType, StorageType>::FindKey(float curTime)  const
{
    // find the key number
    const uint32 keyNumber = KeyFrameFinder<ReturnType, StorageType>::FindKey(curTime, mKeys.GetReadPtr(), mKeys.GetLength());

    // if no key was found
    return (keyNumber != MCORE_INVALIDINDEX32) ? &mKeys[keyNumber] : nullptr;
}


// returns the interpolated value at a given time
template <class ReturnType, class StorageType>
ReturnType KeyTrackHermite<ReturnType, StorageType>::GetValueAtTime(float currentTime, uint32* cachedKey, uint8* outWasCacheHit) const
{
    MCORE_ASSERT(currentTime >= 0.0);
    MCORE_ASSERT(mKeys.GetLength() > 0);

    // make a local copy of the cached key value
    uint32 localCachedKey = (cachedKey) ? *cachedKey : MCORE_INVALIDINDEX32;

    // find the first key to start interpolating from (between this one and the next)
    uint32 keyNumber = MCORE_INVALIDINDEX32;

    // prevent searching in the set of keyframes when a cached key is available
    // of course we need to check first if the cached key is actually still valid or not
    if (localCachedKey == MCORE_INVALIDINDEX32) // no cached key has been set, so simply perform a search
    {
        if (outWasCacheHit)
        {
            *outWasCacheHit = 0;
        }

        keyNumber = KeyFrameFinder<ReturnType, StorageType>::FindKey(currentTime, mKeys.GetReadPtr(), mKeys.GetLength());

        if (cachedKey)
        {
            *cachedKey = keyNumber;
        }
    }
    else
    {
        // make sure we dont go out of bounds when checking
        if (localCachedKey >= mKeys.GetLength() - 2)
        {
            if (mKeys.GetLength() > 2)
            {
                localCachedKey = mKeys.GetLength() - 3;
            }
            else
            {
                localCachedKey = 0;
            }
        }

        // check if the cached key is still valid (cache hit)
        if ((mKeys[localCachedKey].GetTime() <= currentTime) && (mKeys[localCachedKey + 1].GetTime() >= currentTime))
        {
            keyNumber = localCachedKey;
            if (outWasCacheHit)
            {
                *outWasCacheHit = 1;
            }
        }
        else
        {
            if (localCachedKey < mKeys.GetLength() - 2 && (mKeys[localCachedKey + 1].GetTime() <= currentTime) && (mKeys[localCachedKey + 2].GetTime() >= currentTime))
            {
                if (outWasCacheHit)
                {
                    *outWasCacheHit = 1;
                }

                keyNumber = localCachedKey + 1;
            }
            else    // the cached key is invalid, so perform a real search (cache miss)
            {
                if (outWasCacheHit)
                {
                    *outWasCacheHit = 0;
                }

                keyNumber = KeyFrameFinder<ReturnType, StorageType>::FindKey(currentTime, mKeys.GetReadPtr(), mKeys.GetLength());

                if (cachedKey)
                {
                    *cachedKey = keyNumber;
                }
            }
        }
    }

    // if no key could be found
    if (keyNumber == MCORE_INVALIDINDEX32)
    {
        // if there are no keys at all, simply return an empty object
        if (mKeys.GetLength() == 0)
        {
            // return an empty object
            return ReturnType();
        }

        // return the last key
        return mKeys.GetLast().GetValue();
    }

    // check if we didn't reach the end of the track
    if ((keyNumber + 1) > (mKeys.GetLength() - 1))
    {
        return mKeys.GetLast().GetValue();
    }

    // perform interpolation
    return Interpolate(keyNumber, currentTime);
}


// perform interpolation
template <class ReturnType, class StorageType>
MCORE_INLINE ReturnType KeyTrackHermite<ReturnType, StorageType>::Interpolate(uint32 startKey, float currentTime) const
{
    // get the keys to interpolate between
    const KeyFrame<ReturnType, StorageType>& firstKey = mKeys[startKey];
    const KeyFrame<ReturnType, StorageType>& nextKey  = mKeys[startKey + 1];

    // calculate the time value in range of [0..1]
    const float t = (currentTime - firstKey.GetTime()) / (nextKey.GetTime() - firstKey.GetTime());

    //              [ 2 -2  1  1]   [a]
    //[u3 u2 u 1]   [-3  3 -2 -1]   [b]
    //              [ 0  0  1  0]   [ta]
    //              [ 1  0  0  0]   [tb]
    const float t2 = t * t;
    const float t3 = t2  * t;

    return (2 * t3 + -3 * t2 + 1) * firstKey.GetValue() +
           (-2 * t3 +  3 * t2)     * nextKey.GetValue() +
           (t3 + -2 * t2 + t) * mTangents[ startKey ] +
           (t3 + -t2)       * mTangents[ startKey + 1 ];
}



template <>
MCORE_INLINE MCore::Quaternion KeyTrackHermite<MCore::Quaternion, MCore::Compressed16BitQuaternion>::Interpolate(uint32 startKey, float currentTime) const
{
    // get the keys to interpolate between
    const KeyFrame<MCore::Quaternion, MCore::Compressed16BitQuaternion>& firstKey = mKeys[startKey];
    const KeyFrame<MCore::Quaternion, MCore::Compressed16BitQuaternion>& nextKey  = mKeys[startKey + 1];

    // get the two quaternions
    MCore::Quaternion a = firstKey.GetValue();
    MCore::Quaternion b = nextKey.GetValue();

    // check if both quaternions are on the same hypersphere or not, if not, invert one
    if (a.Dot(b) < 0.0f)
    {
        a = -a;
    }

    //              [ 2 -2  1  1]   [a]
    //[u3 u2 u 1]   [-3  3 -2 -1]   [b]
    //              [ 0  0  1  0]   [ta]
    //              [ 1  0  0  0]   [tb]

    // interpolate, and take the exponent of that, which is the interpolated quaternion
    // calculate the time value in range of [0..1]
    const float t  = (currentTime - firstKey.GetTime()) / (nextKey.GetTime() - firstKey.GetTime());
    const float t2 = t * t;
    const float t3 = t2  * t;

    return ((2 * t3 + -3 * t2 + 1) * a.LogN()  +
            (-2 * t3 +  3 * t2)     * b.LogN()  +
            (t3 + -2 * t2 + t) * mTangents[startKey] +
            (t3 + -t2)       * mTangents[startKey + 1]).Exp().Normalize();
}


// adds a keyframe with automatic sorting support
template <class ReturnType, class StorageType>
void KeyTrackHermite<ReturnType, StorageType>::AddKeySorted(float time, const ReturnType& value)
{
    // if there are no keys yet, add it
    if (mKeys.GetLength() == 0)
    {
        mKeys.Add(KeyFrame<ReturnType, StorageType>(time, value));
        return;
    }

    // get the keyframe time
    const float keyTime = time;

    // if we must add it at the end
    if (keyTime >= mKeys.GetLast().GetTime())
    {
        mKeys.Add(KeyFrame<ReturnType, StorageType>(time, value));
        return;
    }

    // if we have to add it in the front
    if (keyTime < mKeys.GetFirst().GetTime())
    {
        mKeys.Insert(0, KeyFrame<ReturnType, StorageType>(time, value));
        return;
    }

    // quickly find the location to insert, and insert it
    const uint32 place = KeyFrameFinder<ReturnType, StorageType>::FindKey(keyTime, mKeys.GetReadPtr(), mKeys.GetLength());
    mKeys.Insert(place + 1, KeyFrame<ReturnType, StorageType>(time, value));
}


template <class ReturnType, class StorageType>
MCORE_INLINE void KeyTrackHermite<ReturnType, StorageType>::RemoveKey(uint32 keyNr)
{
    mKeys.Remove(keyNr);
}


template <class ReturnType, class StorageType>
void KeyTrackHermite<ReturnType, StorageType>::MakeLoopable(float fadeTime)
{
    MCORE_ASSERT(fadeTime > 0);

    if (mKeys.GetLength() == 0)
    {
        return;
    }

    const float lastTime = GetLastKey()->GetTime();
    ReturnType firstValue = GetFirstKey()->GetValue();
    AddKey(lastTime + fadeTime, firstValue);

    // re-init the track
    Init();
}


// optimize the keytrack
template <class ReturnType, class StorageType>
uint32 KeyTrackHermite<ReturnType, StorageType>::Optimize(float maxError)
{
    // if there aren't at least two keys, return, because we never remove the first and last key frames
    // and we'd need at least two keyframes to interpolate between
    if (mKeys.GetLength() <= 2)
    {
        return 0;
    }

    // create a temparory copy of the keytrack data we're going to optimize
    KeyTrackHermite<ReturnType, StorageType> keyTrackCopy;
    keyTrackCopy.mKeys  = mKeys;
    keyTrackCopy.Init();

    // while we want to continue optimizing
    uint32 i = 1;
    uint32 numRemoved = 0;  // the number of removed keys
    do
    {
        // get the time of the current keyframe (starting from the second towards the last one)
        const float time = mKeys[i].GetTime();

        // remove the keyframe and reinit the keytrack (and interpolator's tangents etc)
        keyTrackCopy.RemoveKey(i);
        keyTrackCopy.Init();

        // get the interpolated value at the keyframe's time of the keytrack BEFORE we removed the keyframe
        ReturnType v1 = GetValueAtTime(time);

        // and get the interpolated value of the keytrack AFTER we removed the key
        ReturnType v2 = keyTrackCopy.GetValueAtTime(time);

        // if the points are close, so if the "visual" difference of removing the key is within our threshold
        // we can remove the key for real
        if (MCore::Compare<ReturnType>::CheckIfIsClose(v1, v2, maxError))
        {
            RemoveKey(i);   // remove the key for real
            Init();         // reinit the keytrack
            numRemoved++;
        }
        else    // if the "visual" difference is too high and we do not want ot remove the key, copy over the original keys again to restore it
        {
            keyTrackCopy.mKeys = mKeys;     // copy the keyframe array
            keyTrackCopy.Init();            // reinit the keytrack
            i++;                            // go to the next keyframe, and try ot remove that one
        }
    } while (i < mKeys.GetLength() - 1);    // while we haven't reached the last keyframe (minus one)

    //mKeys.Shrink();
    return numRemoved;
}


// pre-alloc keys
template <class ReturnType, class StorageType>
void KeyTrackHermite<ReturnType, StorageType>::SetNumKeys(uint32 numKeys)
{
    // resize the array of keys
    mKeys.Resize(numKeys);
}


// set a given key
template <class ReturnType, class StorageType>
MCORE_INLINE void KeyTrackHermite<ReturnType, StorageType>::SetKey(uint32 keyNr, float time, const ReturnType& value)
{
    // adjust the value and time of the key
    mKeys[keyNr].SetValue(value);
    mKeys[keyNr].SetTime(time);
}


// set a given key
template <class ReturnType, class StorageType>
MCORE_INLINE void KeyTrackHermite<ReturnType, StorageType>::SetStorageTypeKey(uint32 keyNr, float time, const StorageType& value)
{
    // adjust the value and time of the key
    mKeys[keyNr].SetStorageTypeValue(value);
    mKeys[keyNr].SetTime(time);
}


// check if the keytrack is animated
template <class ReturnType, class StorageType>
MCORE_INLINE bool KeyTrackHermite<ReturnType, StorageType>::CheckIfIsAnimated(const ReturnType& initialPose, float maxError) const
{
    // empty keytracks are never animated
    if (mKeys.GetLength() == 0)
    {
        return false;
    }

    // get the number of keyframes and iterate through them
    const uint32 numKeyFrames = GetNumKeys();
    for (uint32 i = 0; i < numKeyFrames; ++i)
    {
        // if the sampled value is not within the given maximum distance/error of the initial pose, it means we have an animated track
        if (MCore::Compare<ReturnType>::CheckIfIsClose(initialPose, GetKey(i)->GetValue(), maxError) == false)
        {
            return true;
        }
    }

    return false;
}


// get a given tangent
template <class ReturnType, class StorageType>
MCORE_INLINE ReturnType& KeyTrackHermite<ReturnType, StorageType>::GetTangent(uint32 keyNr) const
{
    MCORE_ASSERT(keyNr < mKeys.GetLength());
    MCORE_ASSERT(mTangents);
    return mTangents[keyNr];
}

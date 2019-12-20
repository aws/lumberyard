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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_TINYDOCUMENT_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_TINYDOCUMENT_H
#pragma once


#include <list>
#include <algorithm>
#include <functional>

template <typename V>
class TinyDocumentBase
{
public:
    typedef V Value;

protected:
    Value value;
};

template <typename T>
class HasLimit
{
public:
    enum
    {
        Value = false
    };
};

template <>
class HasLimit<float>
{
public:
    enum
    {
        Value = true
    };
};

template <typename V, bool L>
class TinyDocumentLimited
    : public TinyDocumentBase<V>
{
};

template <typename V>
class TinyDocumentLimited<V, true>
    : public TinyDocumentBase<V>
{
public:
    void SetMin(const V& min)
    {
        this->min = min;
    }

    V GetMin() const
    {
        return this->min;
    }

    void SetMax(const V& max)
    {
        this->max = max;
    }

    V GetMax() const
    {
        return this->max;
    }

private:
    V max;
    V min;
};

template <typename V>
class TinyDocument
    : public TinyDocumentLimited<V, HasLimit<V>::Value >
{
public:
    void SetValue(const typename TinyDocumentBase<V>::Value& value)
    {
        if (this->value == value)
        {
            return;
        }

        this->value = value;

        using namespace std;
        AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")
        for_each(this->listeners.begin(), this->listeners.end(), [this](Listener* listener) { listener->OnTinyDocumentChanged(this); });
        AZ_POP_DISABLE_WARNING
    }

    typename TinyDocumentBase<V>::Value GetValue() const
    {
        return this->value;
    }

    class Listener
    {
    public:
        virtual void OnTinyDocumentChanged(TinyDocument* pDocument) = 0;
    };

    void AddListener(Listener* pListener)
    {
        this->listeners.push_back(pListener);
    }

    void RemoveListener(Listener* pListener)
    {
        this->listeners.remove(pListener);
    }

private:
    std::list<Listener*> listeners;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_TINYDOCUMENT_H

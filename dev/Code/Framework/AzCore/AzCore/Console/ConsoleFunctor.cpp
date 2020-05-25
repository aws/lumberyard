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

#include <AzCore/Console/ConsoleFunctor.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Interface/Interface.h>


namespace AZ
{
    ConsoleFunctorBase* ConsoleFunctorBase::s_deferredHead = nullptr;

    // Needed to guard against calling Interface<T>::Get(), it's not safe to call Interface<T>::Get() prior to Az Environment attach
    // Otherwise this could trigger environment construction, which can assert on Gems
    bool ConsoleFunctorBase::s_deferredHeadInvoked = false;

    const char* GetEnumString(GetValueResult value)
    {
        switch (value)
        {
        case GetValueResult::Success:
            return "GetValueResult::Success";
        case GetValueResult::NotImplemented:
            return "GetValueResult::NotImplemented : ConsoleFunctor object does not implement the GetAs function";
        case GetValueResult::TypeNotConvertible:
            return "GetValueResult::TypeNotConvertible : ConsoleFunctor contained type is not convertible to a ConsoleVar type";
        case GetValueResult::ConsoleVarNotFound:
            return "GetValueResult::ConsoleVarNotFound : ConsoleFunctor command could not be found";
        default:
            break;
        }

        return "";
    }


    ConsoleFunctorBase::ConsoleFunctorBase(const char* name, const char* desc, ConsoleFunctorFlags flags)
        : m_name(name)
        , m_desc(desc)
        , m_flags(flags)
        , m_console(nullptr)
        , m_prev(nullptr)
        , m_next(nullptr)
        , m_isDeferred(true)
    {
        if (s_deferredHeadInvoked)
        {
            m_console = AZ::Interface<IConsole>::Get();
            Link(m_console->RetrieveHeadElement());
            m_isDeferred = false;
        }
        else
        {
            Link(s_deferredHead);
        }
    }


    ConsoleFunctorBase::~ConsoleFunctorBase()
    {
        if (m_console != nullptr)
        {
            Unlink(m_console->RetrieveHeadElement());
        }
        else if (m_isDeferred)
        {
            Unlink(s_deferredHead);
        }
    }


    void ConsoleFunctorBase::Link(ConsoleFunctorBase*& head)
    {
        m_next = head;

        if (head != nullptr)
        {
            head->m_prev = this;
        }

        head = this;
    }


    void ConsoleFunctorBase::Unlink(ConsoleFunctorBase*& head)
    {
        if (head == nullptr)
        {
            // The head has already been destroyed, no need to unlink
            return;
        }

        if (head == this)
        {
            head = m_next;
        }

        ConsoleFunctorBase* prev = m_prev;
        ConsoleFunctorBase* next = m_next;

        if (m_prev != nullptr)
        {
            m_prev->m_next = next;
            m_prev = nullptr;
        }

        if (m_next != nullptr)
        {
            m_next->m_prev = prev;
            m_next = nullptr;
        }
    }


    GetValueResult ConsoleFunctorBase::GetValueAsString(AZStd::string&) const
    {
        return GetValueResult::NotImplemented;
    }
}

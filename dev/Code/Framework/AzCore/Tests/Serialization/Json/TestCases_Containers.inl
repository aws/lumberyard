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

#include <AzCore/Serialization/SerializeContext.h>

namespace JsonSerializationTests
{
    // SimpleContainer

    template<typename T>
    bool SimpleContainer<T>::Equals(const SimpleContainer<T>& rhs, bool fullReflection) const
    {
        return !fullReflection || m_container == rhs.m_container;
    }

    template<typename T>
    void SimpleContainer<T>::Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection)
    {
        if (fullReflection)
        {
            context->Class<SimpleContainer>()
                ->Field("container", &SimpleContainer::m_container);
        }
    }

    template<typename T>
    InstanceWithSomeDefaults<SimpleContainer<T>> SimpleContainer<T>::GetInstanceWithSomeDefaults()
    {
        // With only primitives stored in the container, there are no partial defaults.
        return MakeInstanceWithSomeDefaults(AZStd::make_unique<SimpleContainer>(), "{}", R"({ "container": [] })");
    }

    template<typename T>
    InstanceWithoutDefaults<SimpleContainer<T>> SimpleContainer<T>::GetInstanceWithoutDefaults()
    {
        auto instance = AZStd::make_unique<SimpleContainer>();
        instance->m_container = {{ 181, 124, 188 }};

        const char* json = R"(
                {
                    "container": [ 181, 124, 188 ]
                })";
        return MakeInstanceWithoutDefaults(AZStd::move(instance), json);
    }


    // ComplexContainer

    template<typename T>
    bool ComplexContainer<T>::Equals(const ComplexContainer<T>& rhs, bool fullReflection) const
    {
        if (!fullReflection)
        {
            return true;
        }

        if (m_container.size() != rhs.m_container.size())
        {
            return false;
        }

        auto aIter = m_container.begin();
        auto bIter = rhs.m_container.begin();
        while (aIter != m_container.end())
        {
            if (!aIter->Equals(*bIter, fullReflection))
            {
                return false;
            }
            ++aIter;
            ++bIter;
        }
        return true;
    }

    template<typename T>
    void ComplexContainer<T>::Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection)
    {
        SimpleNested::Reflect(context, fullReflection);
        context->Class<ComplexContainer>()
            ->Field("container", &ComplexContainer::m_container);
    }

    template<typename T>
    InstanceWithSomeDefaults<ComplexContainer<T>> ComplexContainer<T>::GetInstanceWithSomeDefaults()
    {
        auto instance = AZStd::make_unique<ComplexContainer>();
        instance->m_container = {{ PartialInit1(), PartialInit2(), PartialInit3() }};
        
        const char* strippedDefaults = R"(
            {
                "container": [
                    {
                        "nested": {
                            "var1": 188
                        },
                        "var_additional": 213
                    },
                    {},
                    {
                        "nested": {
                            "var2": 342.0
                        }
                    }
                ]
            })";
        const char* keptDefaults = R"(
            {
                "container": [
                    {
                        "nested": {
                            "var1": 188,
                            "var2": 42.0
                        },
                        "var_additional": 213
                    },
                    {
                        "nested": {
                            "var1": 42,
                            "var2": 42.0
                        },
                        "var_additional": 42
                    },
                    {
                        "nested": {
                            "var1": 42,
                            "var2": 342.0
                        },
                        "var_additional": 42
                    }
                ]
            })";

        return MakeInstanceWithSomeDefaults(AZStd::move(instance), strippedDefaults, keptDefaults);
    }

    template<typename T>
    InstanceWithoutDefaults<ComplexContainer<T>> ComplexContainer<T>::GetInstanceWithoutDefaults()
    {
        auto instance = AZStd::make_unique<ComplexContainer>();
        instance->m_container = {{ FullInit1(), FullInit2(), FullInit3() }};

        const char* json = R"(
            {
                "container": [
                    {
                        "nested": {
                            "var1": 188,
                            "var2": 242.0
                        },
                        "var_additional": 313
                    },
                    {
                        "nested": {
                            "var1": 488,
                            "var2": 542.0
                        },
                        "var_additional": 613
                    },
                    {
                        "nested": {
                            "var1": 788,
                            "var2": 842.0
                        },
                        "var_additional": 913
                    }
                ]
            })";
        return MakeInstanceWithoutDefaults(AZStd::move(instance), json);
    }

    template<typename T>
    SimpleNested ComplexContainer<T>::PartialInit1()
    {
        SimpleNested result;
        result.m_nested.m_var1 = 188;
        result.m_varAdditional = 213;
        return result;
    }

    template<typename T>
    SimpleNested ComplexContainer<T>::PartialInit2()
    {
        return SimpleNested();
    }

    template<typename T>
    SimpleNested ComplexContainer<T>::PartialInit3()
    {
        SimpleNested result;
        result.m_nested.m_var2 = 342.0;
        return result;
    }

    template<typename T>
    SimpleNested ComplexContainer<T>::FullInit1()
    {
        SimpleNested result;
        result.m_nested.m_var1 = 188;
        result.m_nested.m_var2 = 242.0;
        result.m_varAdditional = 313;
        return result;
    }

    template<typename T>
    SimpleNested ComplexContainer<T>::FullInit2()
    {
        SimpleNested result;
        result.m_nested.m_var1 = 488;
        result.m_nested.m_var2 = 542.0;
        result.m_varAdditional = 613;
        return result;
    }

    template<typename T>
    SimpleNested ComplexContainer<T>::FullInit3()
    {
        SimpleNested result;
        result.m_nested.m_var1 = 788;
        result.m_nested.m_var2 = 842.0;
        result.m_varAdditional = 913;
        return result;
    }
} // namespace JsonSerializationTests

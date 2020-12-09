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

#pragma once
#include <AzCore/Component/Component.h>

//websocketpp requirements - disables all boost requirements so we can just use asio
#pragma push_macro("ASIO_STANDALONE")
#define BOOST_ALL_NO_LIB
#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_
AZ_PUSH_DISABLE_WARNING(4267 4996, "-Wunknown-warning-option")
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
AZ_POP_DISABLE_WARNING
#undef _WEBSOCKETPP_CPP11_TYPE_TRAITS_
#undef _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#undef ASIO_STANDALONE
#undef BOOST_ALL_NO_LIB
#pragma pop_macro("ASIO_STANDALONE")

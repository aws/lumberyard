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

#include <AzCore/std/string/string.h>

namespace TwitchApi
{
    /*
    ** Results enum, Unknown must be the last element, and its value must be 0x7fffffff
    */
     
    enum class ResultCode : int
    {
        Success,
        InvalidParam,
        TwitchRestError,
        TwitchChannelNoUpdatesToMake,
        Unknown = 0x7ffffff
    };

    /*
    ** Our receipts
    */

    struct ReceiptId
    {
        AZ_TYPE_INFO(ReceiptId, "{F8E5C973-AA16-4C6A-B0C1-A17FFF65DEB4}");

        ReceiptId() 
            : m_id(0) 
        {}
        
        ReceiptId(const ReceiptId& id) = default;

        bool operator!=(const ReceiptId& id) const
        {
            return m_id != id.m_id;
        }

        bool operator==(const ReceiptId& id) const
        {
            return m_id == id.m_id;
        };

        void SetId(AZ::u64 id)
        { 
            m_id = id;
        }

        AZ::u64 GetId() const
        { 
            return m_id;
        }
        
    private:
        AZ::u64 m_id;
    };

    struct PaginationResponse
    {
        AZ_TYPE_INFO(PaginationResponse, "{1AE884B9-7831-4153-AFC3-C2645118D2EE}");

        AZStd::string m_pagination;
    };

    //! Wraps a ResultCode and a ReceiptId
    struct ReturnValue : public ReceiptId
    {
        AZ_TYPE_INFO(ReturnValue, "{74384D0F-666B-4BB1-975D-4EA484B4AF96}");

        ReturnValue()
            : m_result(ResultCode::Unknown)
        {
        }
        
        ReturnValue(const ReceiptId& recieptId, ResultCode result = ResultCode::Success)
            : ReceiptId(recieptId)
            , m_result(result)
        {
        }

        ResultCode m_result;
    };

    //! Create a result class that wraps a response object, a ResultCode, and a ReceiptId
    #define TwitchApi_CreateReturnTypeClass(_valueType, _returnType, _ClassGuid)                                    \
        struct _valueType : public ReturnValue                                                                      \
        {                                                                                                           \
            AZ_TYPE_INFO(_valueType, _ClassGuid);                                                                   \
                                                                                                                    \
            _valueType()                                                                                            \
                : ReturnValue()                                                                                     \
                , m_value()                                                                                         \
                {}                                                                                                  \
            _valueType(const _returnType& value, const ReceiptId& recieptId, ResultCode result=ResultCode::Success) \
                : ReturnValue(recieptId, result)                                                                    \
                , m_value(value)                                                                                    \
                {}                                                                                                  \
            _returnType m_value;                                                                                    \
        };                                                                                                          

    //! Create a response class containing a datum list
    #define TwitchApi_CreateResponseTypeClass(_datumType, _responseType, _ResponseGuid)                             \
        struct _responseType                                                                                        \
        {                                                                                                           \
            AZ_TYPE_INFO(_responseType, _ResponseGuid);                                                             \
                                                                                                                    \
            AZStd::vector<_datumType> m_data;                                                                       \
        };

    //! Create a response class containing a datum list and a pagination cursor string
    #define TwitchApi_CreatePaginationResponseTypeClass(_datumType, _responseType, _ResponseGuid)                   \
        struct _responseType : public PaginationResponse                                                            \
        {                                                                                                           \
            AZ_TYPE_INFO(_responseType, _ResponseGuid);                                                             \
                                                                                                                    \
            AZStd::vector<_datumType> m_data;                                                                       \
        };

}

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(TwitchApi::ResultCode, "{3DEC0917-A1BB-4645-83BA-97A66D8C03BB}");
}

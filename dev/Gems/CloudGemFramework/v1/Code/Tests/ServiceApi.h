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

#include <CloudGemFramework/ServiceRequestJob.h>

namespace CloudGemFrameworkTest
{

    namespace ServiceApi
    {

        struct ComplexBody_string
        {

            AZStd::string data;

            bool OnJsonKey(const char* key, CloudGemFramework::JsonReader& reader)
            {
                if(strcmp(key, "data") == 0) return reader.Accept(data);
                return reader.Ignore();
            }

            bool WriteJson(CloudGemFramework::JsonWriter& writer) const
            {
                bool ok = true;
                ok = ok && writer.StartObject();
                ok = ok && writer.Write("data", data);
                ok = ok && writer.EndObject();
                return ok;
            }

        };

        struct ComplexResult_string
        {

            AZStd::string pathparam;
            AZStd::string queryparam;
            ComplexBody_string bodyparam;
        
            bool OnJsonKey(const char* key, CloudGemFramework::JsonReader& reader)
            {
                if(strcmp(key, "pathparam") == 0) return reader.Accept(pathparam);
                if(strcmp(key, "queryparam") == 0) return reader.Accept(queryparam);
                if(strcmp(key, "bodyparam") == 0) return reader.Accept(bodyparam);
                return reader.Ignore();
            }

            bool WriteJson(CloudGemFramework::JsonWriter& writer) const
            {
                bool ok = true;
                ok = ok && writer.StartObject();
                ok = ok && writer.Write("pathparam", pathparam);
                ok = ok && writer.Write("queryparam", queryparam);
                ok = ok && writer.Write("bodyparam", bodyparam);
                ok = ok && writer.EndObject();
                return ok;
            }

        };

        CLOUD_GEM_SERVICE(TestResourceGroup1);

        class PostTestComplexStringRequest
            : public CloudGemFramework::ServiceRequest
        {

        public:

            SERVICE_REQUEST(TestResourceGroup1, HttpMethod::HTTP_POST, "/test/complex/string/{pathparam}");

            struct Parameters
            {

                AZStd::string pathparam;
                AZStd::string queryparam;
                ComplexBody_string bodyparam;

                bool BuildRequest(CloudGemFramework::RequestBuilder& request)
                {
                    bool ok = true;
                    ok = ok && request.SetPathParameter("{pathparam}", pathparam);
                    ok = ok && request.AddQueryParameter("queryparam", queryparam);
                    ok = ok && request.WriteJsonBodyParameter(bodyparam);
                    return ok;
                }

            };

            Parameters parameters;
            ComplexResult_string result;

        };

        using PostTestComplexStringRequestJob = CloudGemFramework::ServiceRequestJob<PostTestComplexStringRequest>;

    } // namespace ServiceApi

} // namespace CloudGemFrameworkTest

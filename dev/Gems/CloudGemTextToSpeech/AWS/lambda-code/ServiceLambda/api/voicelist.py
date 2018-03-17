#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import service
import boto3

@service.api
def get(request):
    client = boto3.client("polly")
    response = client.describe_voices()
    voices = {}
    for voice in response["Voices"]:
        if voices.get(voice["LanguageName"], []):
            voices[voice["LanguageName"]].append({"voiceId": voice["Id"], "gender": voice["Gender"], "languageCode": voice["LanguageCode"]})               
        else:
            voices[voice["LanguageName"]] = [{"voiceId": voice["Id"], "gender": voice["Gender"], "languageCode": voice["LanguageCode"]}]

    while response.get("NextToken", ""):
        response = client.describe_voices(NextToken=response["NextToken"])
        for voice in response["Voices"]:
            if voices.get(voice["LanguageName"], []):
                voices[voice["LanguageName"]].append({"voiceId": voice["Id"], "gender": voice["Gender"], "languageCode": voice["LanguageCode"]})               
            else:
                voices[voice["LanguageName"]] = [{"voiceId": voice["Id"], "gender": voice["Gender"], "languageCode": voice["LanguageCode"]}]

    return { "voices": voices }
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
import character_config
import tts

@service.api
def post(request, speech):
    character_info = character_config.get_character_info(speech["character"])
    voice = character_info["voice"]
    speech_marks = character_info["speechMarks"]
    message = speech["line"]

    message = tts.add_prosody_tags_to_message(message, character_info)

    request_info = {
        "voice": voice,
        "message": message,
        "speechMarks": speech_marks
    }
    if speech_marks:
        return { "audioUrl": tts.get_voice(request_info, True), "speechMarksUrl": tts.get_speech_marks(request_info, True) }
    else:
        return { "audioUrl": tts.get_voice(request_info, True), "speechMarksUrl": "" }
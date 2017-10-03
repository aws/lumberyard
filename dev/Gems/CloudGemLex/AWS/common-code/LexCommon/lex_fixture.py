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

import json

bots = [
    {
        "bot": {
            "name": "LYTestBot",
            "description" : "Test bot for the speech recognition cloud gem sample level",
            "idleSessionTTLInSeconds": 600,
            "voidId": "none",
            "locale": "en-US",
            "intents": [
                {
                    "intentName": "RequestHelp",
                    "intentVersion": "$LATEST"
                },
                {
                    "intentName": "Ping",
                    "intentVersion": "$LATEST"
                }
            ]
        },
        "intents": [
            {
                "name": "RequestHelp",
                "description": "User requests help",
                "fulfillmentActivity": {
                    "type": "ReturnIntent"
                },
                "sampleUtterances": [
                    "I need help",
                    "Help me",
                    "Send help",
                    "Help"
                ]
            },
            {
                "name": "Ping",
                "description": "User requests map ping",
                "fulfillmentActivity": {
                    "type": "ReturnIntent"
                },
                "sampleUtterances": [
                    "ping",
                    "pint map",
                    "ping map at {MapLocation}",
                    "ping at {MapLocation}",
                    "ping {MapLocation}",
                    "ping map {MapLocation}"
                ],
                "slots": [
                            {
                                "name": "MapLocation",
                                "description": "Where to ping on the map",
                                "priority": 1,
                                "sampleUtterances": [],
                                "slotConstraint": "Required",
                                "slotType": "MapLocation",
                                "valueElicitationPrompt": {
                                    "maxAttempts": 2,
                                    "messages": [
                                        {
                                            "content": "Where should I ping?",
                                            "contentType": "PlainText"
                                        }
                                    ]
                                 }
                            }
                    ]
                }
        ],
        "slot_types": [
            {
                "name": "MapLocation",
                "description": "Where on the map to click",
                "enumerationValues": [
                    {
                    "value": "jungle"
                    },
                    {
                    "value": "river"
                    },
                    {
                    "value": "me"
                    },
                    {
                    "value": "middle lane"
                    },
                    {
                    "value": "top"
                    },
                    {
                    "value": "top jungle"
                    },
                    {
                    "value": "mid lane"
                    },
                    {
                    "value": "bot lane"
                    },
                    {
                    "value": "mid"
                    },
                    {
                    "value": "myself"
                    },
                    {
                    "value": "top lane"
                    },
                    {
                    "value": "bot"
                    },
                    {
                    "value": "bottom"
                    },
                    {
                    "value": "roshan"
                    },
                    {
                    "value": "bottom jungle"
                    },
                    {
                    "value": "middle"
                    },
                    {
                    "value": "bottom lane"
                    }
                ]
            }
        ]        
    },
    {
        "bot": {
            "name": "LYTestBot2",
            "description" : "Test bot for the speech recognition cloud gem sample level (copy)",
            "idleSessionTTLInSeconds": 600,
            "voidId": "none",
            "locale": "en-US",
            "intents": [
                {
                    "intentName": "RequestHelp",
                    "intentVersion": "$LATEST"
                },
                {
                    "intentName": "Ping",
                    "intentVersion": "$LATEST"
                }
            ]
        },
        "intents": [
            {
                "name": "RequestHelp",
                "description": "User requests help",
                "fulfillmentActivity": {
                    "type": "ReturnIntent"
                },
                "sampleUtterances": [
                    "I need help",
                    "Help me",
                    "Send help",
                    "Help"
                ]
            },
            {
                "name": "Ping",
                "description": "User requests map ping",
                "fulfillmentActivity": {
                    "type": "ReturnIntent"
                },
                "sampleUtterances": [
                    "ping",
                    "pint map",
                    "ping map at {MapLocation}",
                    "ping at {MapLocation}",
                    "ping {MapLocation}",
                    "ping map {MapLocation}"
                ],
                "slots": [
                            {
                                "name": "MapLocation",
                                "description": "Where to ping on the map",
                                "priority": 1,
                                "sampleUtterances": [],
                                "slotConstraint": "Required",
                                "slotType": "MapLocation",
                                "valueElicitationPrompt": {
                                    "maxAttempts": 2,
                                    "messages": [
                                        {
                                            "content": "Where should I ping?",
                                            "contentType": "PlainText"
                                        }
                                    ]
                                 }
                            }
                    ]
                }
        ],
        "slot_types": [
            {
                "name": "MapLocation",
                "description": "Where on the map to click",
                "enumerationValues": [
                    {
                    "value": "jungle"
                    },
                    {
                    "value": "river"
                    },
                    {
                    "value": "me"
                    },
                    {
                    "value": "middle lane"
                    },
                    {
                    "value": "top"
                    },
                    {
                    "value": "top jungle"
                    },
                    {
                    "value": "mid lane"
                    },
                    {
                    "value": "bot lane"
                    },
                    {
                    "value": "mid"
                    },
                    {
                    "value": "myself"
                    },
                    {
                    "value": "top lane"
                    },
                    {
                    "value": "bot"
                    },
                    {
                    "value": "bottom"
                    },
                    {
                    "value": "roshan"
                    },
                    {
                    "value": "bottom jungle"
                    },
                    {
                    "value": "middle"
                    },
                    {
                    "value": "bottom lane"
                    }
                ]
            }
        ]        
    }
]

def get_bot_desc(bot_name):
    for bot in bots:
        if bot["bot"]["name"] == bot_name:
                return json.dumps(bot)            
    return ""

def process_bot_desc(bot_desc):
    return "ACCEPTED"

def list_bots():
    ret = []
    count = 0
    for bot in bots:
        bot_list_entry = {
            "bot_name" : bot["bot"]["name"],
            "current_version" : "1",
            "status": (count == 1 and "PUBLISHED" or "READY"),
            "updated": "Thu, Aug 9 2017 4:02 PM",
            "created": "Fri, Aug 10 2017 12:02 PM"
        }
        ret.append(bot_list_entry)
        count += 1
    return ret

def get_bot_status(bot_name):
   return "READY"

def delete_bot(bot_name):
    return "DELETED"

def publish_bot(bot_name, bot_version):
    return "READY"
 

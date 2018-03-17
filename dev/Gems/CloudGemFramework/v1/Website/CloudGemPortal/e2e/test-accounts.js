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

var TestAccounts = module.exports = {
  user: {
    username: "zzztestUser1" + makeId(),
    email: "cgp-integ-test" + makeId() + "@amazon.com",
    password: "Test01)!",
    newPassword: "Test02)@"
  },
  admin: {
    username: "administrator",
    password: "T3mp1@34"
  }
}

/*
 * Generate an ID to append to user names so multiple browsers running in parallel don't use the same user name.
*/
function makeId()
{
    var text = "";
    var possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    for( var i=0; i < 5; i++ )
        text += possible.charAt(Math.floor(Math.random() * possible.length));

    return text;
}
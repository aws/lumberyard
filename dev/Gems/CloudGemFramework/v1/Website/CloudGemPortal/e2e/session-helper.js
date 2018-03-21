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

var page = {
    username: $('#username'),
    password: $('#password'),
    login: $('button[type="submit"]'),
    dropdown: $('#account-dropdown'),
    signout: $('#sign-out')
}

var sessionHelper = module.exports = {
    loginPath: 'login',

     loginAsAdministrator: function(){                 
        browser.wait(until.presenceOf(page.username), 10000, "Unable to login- couldn't find text input with id=username");        
        page.username.sendKeys(browser.params.admin.username);
        page.password.sendKeys(browser.params.admin.password);
        page.login.click();
    },

    signOut: function(){
        var until = protractor.ExpectedConditions;
        browser.wait(until.presenceOf($('#account-dropdown')), 10000, "Can't find account dropdown");
        page.dropdown.click();
        browser.wait(until.presenceOf(page.signout), 10000, "Can't find sign out link");
        page.signout.click();
     },

    screenshot: browser.takeScreenshot().then(function (png) {
        //var stream = fs.createWriteStream("/tmp/screenshot.png");
        //stream.write(new Buffer(png, 'base64'));
        //stream.end();
    })
}


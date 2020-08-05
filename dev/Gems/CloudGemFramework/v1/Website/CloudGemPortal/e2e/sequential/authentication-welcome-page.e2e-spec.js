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
var session = require('../session-helper.js');

/**
 * Authentication Welcome Page test
 *
 * Tests the login screen shows as expected with modal first time user pop-up
 */
describe('Authentication welcome page', function () {
    var page = {
        header: $('div.modal-header h2'),
        welcomeIcon1: $('#welcome-option1-img'),
        welcomeIcon2: $('#welcome-option2-img'),
        submit: $('#modal-dismiss')

    }
    beforeAll(function () {
        console.log("Opening:" + browser.params.url)
        browser.get(browser.params.url)
        console.log("Waiting for the Cloud Gem Portal to load.")
        browser.wait(until.urlContains(session.loginPath), 10000, "urlContains"); // Checks that the current URL contains the expected text           
    }); 

    describe('integration tests', function() {
        it('should display a welcome modal', function () {
            browser.wait(until.presenceOf(page.header), 6000, "Modal header")
            expect(page.welcomeIcon1.isDisplayed()).toEqual(true)
            expect(page.welcomeIcon2.isDisplayed()).toEqual(true)           
        });
    });
});
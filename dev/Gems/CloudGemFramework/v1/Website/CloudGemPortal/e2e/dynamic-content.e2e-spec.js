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

var session = require('./session-helper.js');

/**
 * Dynamic Content Integration Tests
 * For this to run correctly it assumes your dynamic content gem has at least one dynamic content item availible.
 * This test will only run in Chrome currently.  The current version of gecko does not support mouseMove which means Firefox will not work.
 * https://github.com/angular/protractor/issues/4177
 */
describe('Dynamic Content', function() {
    
    beforeAll(function () {        
        browser.get(browser.params.url);
        console.log("Waiting for the Cloud Gem Portal to load.")
        browser.wait(until.urlContains(session.loginPath), 10000, "urlContains"); // Checks that the current URL contains the expected text           
        session.loginAsAdministrator();
        var dynamicContentThumbnail = $('thumbnail-gem[ng-reflect-title="Dynamic Content"]');
        browser.wait(until.presenceOf(dynamicContentThumbnail), 10000, "Could not find Dynamic Content Thumbnail, is the gem in the current project?")
        dynamicContentThumbnail.click()
    }); 


    describe('Integration Tests', function() {
        it('should be able to move dynamic content to the private lane', function() {
            moveLaneAndSubmit(0);
        });
        it('should be able to move dynamic content to the scheduled lane', function() {
            moveLaneAndSubmit(1);
        });
        it('should be able to move dynamic content to the public lane', function() {
            moveLaneAndSubmit(2);
        });
        it('should be able to move dynamic content back to the private lane', function() {
            moveLaneAndSubmit(0);
        });
    });
    function moveLaneAndSubmit(laneIndex) {
        browser.wait(until.presenceOf($$('.action-stub.container').get(0)), 10000, "Couldn't find action stub container in dynamic content.  Is there any content available?");
        browser.actions().mouseMove($$('.action-stub.container').get(0)).perform();
        browser.wait(until.presenceOf($$('div.col-2.action-stub-btn i.fa.fa-cog').get(0)), 10000, "There was no cog icon available to edit dynamic content.");
        $("div.col-2.action-stub-btn i.fa.fa-cog").click();
        browser.wait(until.presenceOf($$('button.l-dropdown').get(0)), 10000, "Could not find dropdown in dynamic content modal");
        $('button.l-dropdown').click();
        browser.wait(until.presenceOf($$('.dynamic-content-dropdown-item').get(laneIndex)), 10000, "Could not access expanded dropdown in dynamic content modal");
        $$('.dynamic-content-dropdown-item').get(laneIndex).click();
        $('#modal-submit').click();
        browser.driver.sleep(2000);
    }
});
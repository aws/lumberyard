var session = require('./session-helper.js');

/**
 * Dynamic Content Integration Tests
 * For this to run correctly it assumes your dynamic content gem has at least one dynamic content item availible.
 * This test will only run in Chrome currently.  The current version of gecko does not support mouseMove which means Firefox will not work.
 * https://github.com/angular/protractor/issues/4177
 */
describe('Dynamic Content', function() {
    var until = protractor.ExpectedConditions;
    beforeAll(function() {
        browser.get('http://localhost:3000');
        session.loginAsAdministrator();
        var dynamicContentThumbnail = $('thumbnail-gem[ng-reflect-title="Dynamic Content"]');
        browser.wait(until.presenceOf(dynamicContentThumbnail), 10000, "Could not find Dynamic Content Thumbnail, is the gem in the current project?")
        dynamicContentThumbnail.click()
    }); 

    afterAll(function() {
        session.signOut();
    })

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
        browser.wait(until.presenceOf($$('.fa.fa-cog').get(0)), 10000, "There was no cog icon available to edit dynamic content.");
        $(".fa.fa-cog").click();
        browser.wait(until.presenceOf($$('button.l-dropdown').get(0)), 10000, "Could not find dropdown in dynamic content modal");
        $('button.l-dropdown').click();
        browser.wait(until.presenceOf($$('.dynamic-content-dropdown-item').get(laneIndex)), 10000, "Could not access expanded dropdown in dynamic content modal");
        $$('.dynamic-content-dropdown-item').get(laneIndex).click();
        $('#modal-submit').click();
        browser.driver.sleep(2000);
    }
});
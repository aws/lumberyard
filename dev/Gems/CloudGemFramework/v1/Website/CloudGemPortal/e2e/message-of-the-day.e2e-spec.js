var session = require('./session-helper.js');

var num_messages = 7;

/**
 * Message of the day integration tests
 * For this to run correctly delete all your messages from Active and Planned sections
 */
describe('Message of the Day', function() {
    var until = protractor.ExpectedConditions;
    beforeAll(function() {
        browser.get('http://localhost:3000');
        session.loginAsAdministrator();
        var motdThumbnail = $('thumbnail-gem[ng-reflect-title="Message of the day"]');
        browser.wait(until.presenceOf(motdThumbnail), 10000, "Unable to find thumbnail for Message of the Day");
        motdThumbnail.click();
    }); 

    afterAll(function() {
        session.signOut()
    });
    describe('Integration Tests', function() {
        it('should be able to create new messages of the day', function() {
            for (let i = 0 ; i < num_messages; i++) {
                browser.wait(until.presenceOf($('button.add-motd-button')), 10000, "Can't find account dropdown");
                $('button.add-motd-button').click();
                $('textarea').sendKeys("Integration Test message");
                $$('.l-checkbox').get(0).click();
                $$('.l-checkbox').get(1).click();
                $$('.datepicker').get(0).clear().then(function() {
                    $$('.datepicker').get(0).sendKeys("2017-01-01");
                });
                $$('.datepicker').get(1).clear().then(function() {
                    $$('.datepicker').get(1).sendKeys("2019-01-01");
                });
                 $('#modal-submit').click(); 
            }
            browser.driver.sleep(1000);
        });
        it('should be able to edit a message', function() {
            browser.wait(until.presenceOf($$('.messages-container .fa.fa-cog').get(0)), 10000, "Can't find cog icon to edit message");
            $$('.messages-container .fa.fa-cog').get(0).click();
            // change the message to not have an end.
            $$('.l-checkbox').get(1).click();

            // change priority from 0
            $('#priority-dropdown').click();
            $$("button.dropdown-item").get(3).click();
            $('#modal-submit').click()
        });

        it('should be able to use pagination', function() {
            browser.wait(until.presenceOf($$('.page-link').get(0)), 10000, "Could not find pagination component");
            var pageLinks = $$('.page-link');
            expect(pageLinks.count()).toBeGreaterThan(0);
            pageLinks.get(-1).click();
            browser.wait(until.presenceOf($$('.page-link').get(0)), 10000, "Could not find pagination component");
            pageLinks.get(0).click();
        });

        it('should be able to delete generated messages', function() {
            for (let i = 0; i < num_messages*4; i++) {
                deleteFirstMessage();
            }
        });

        it('should be able to create and delete a planned message', function() {
            $('button.add-motd-button').click();
            $('textarea').sendKeys("Integration Planned Test message");
            $$('.l-checkbox').get(0).click();
            $$('.datepicker').get(0).clear().then(function() {
                $$('.datepicker').get(0).sendKeys("2020-01-01");
            });
            $('#modal-submit').click();
            deleteFirstMessage();
            browser.driver.sleep(1000);
        })

        it('should be able to navigate to history tab', function() {
            browser.wait(until.presenceOf($$('.tab').get(1)), 10000, "Could not find tab component");
            $$('.tab').get(1).click();
            browser.wait(until.presenceOf($$('.tab').get(0)), 10000, "Could not find tab component");
            $$('.tab').get(0).click();
        })

    });

    function deleteFirstMessage() {
        browser.driver.sleep(1000);
        browser.wait(until.presenceOf($$('.messages-container .fa.fa-trash-o').get(0)), 10000, "Can't find trash icon for deleting messages");
        $$('.messages-container .fa.fa-trash-o').get(0).click();
        $('#modal-submit').click();
    }
});
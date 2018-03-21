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

require('jasmine-expect');

var motdModel = require('./models/message-of-the-day-model.js');
var session = require('./session-helper.js');

var num_messages = 7;

/*
* Message of the day integration tests
* For this to run correctly delete all your messages from Active and Planned sections
*/
describe('Message of the Day', function() {
    var until = protractor.ExpectedConditions;

    beforeAll(function() {             
        browser.get(browser.params.url);
        console.log("Waiting for the Cloud Gem Portal to load.")
        browser.wait(until.urlContains(session.loginPath), 10000, "urlContains"); // Checks that the current URL contains the expected text                 
        session.loginAsAdministrator();
        browser.wait(until.elementToBeClickable(motdModel.thumbnail), 20000, "Unable to find thumbnail for Message of the Day");
        motdModel.thumbnail.click();
    }); 
    
    describe('Integration', function() {
        
        // Facet Tests
        describe(': FACET Tests', function () { 
            it('should contain Overview FACET', function() {
                motdModel.facets.get(0).getText().then(function(text) {
                    expect(text).toBe("Overview");
                });

            });

            it('should contain History FACET', function() {
                motdModel.facets.get(1).getText().then(function (text) {
                    expect(text).toBe("History");
                });

            });

            it('should contain REST Explorer FACET', function() {
                motdModel.facets.get(2).getText().then(function(text) {
                    expect(text).toBe("REST Explorer");
                });

            });

            it('should be able to navigate to History tab and back to Overview', function() {
                motdModel.facets.get(1).click();
                browser.wait(until.elementToBeClickable(motdModel.history.messageContainer), 5000, "Cloud not determine if History page loaded");

                motdModel.facets.get(0).click();
                browser.wait(until.elementToBeClickable(motdModel.overview.addButton), 5000, "Cloud not determine if Overiew page re-loaded");
            });

            it('should be able to navigate to REST Explorer tab and back to Overview', function() {
                motdModel.facets.get(2).click();
                browser.wait(until.elementToBeClickable(motdModel.restExplorer.path.obj), 5000, "Cloud not determine if REST Explorere page loaded");

                motdModel.facets.get(0).click();
                browser.wait(until.elementToBeClickable(motdModel.overview.addButton), 5000, "Cloud not determine if Overiew page re-loaded");
            });
        });
        
        // Overview Tests
        describe('Overview Tests', function() {
            it('should be able to create new messages of the day', function() {
               addMessageFromOverview(num_messages);
            });

            it('should be able to use pagination', function() {
                browser.wait(until.presenceOf(motdModel.overview.pageLinks), 5000, "Could not find pagination links");
                browser.wait(until.elementToBeClickable(motdModel.overview.pageLinks.get(1)), 5000, "Could not find pagination component");
                
                expect(motdModel.overview.pageLinks.count()).toBeGreaterThan(0);
                
                motdModel.overview.pageLinks.get(1).click();

                browser.wait(motdModel.overview.pageLinks.get(1).isSelected(), 5000, "Cloud not determine if page is selected");
                
                browser.wait(until.presenceOf(motdModel.overview.pageLinks), 5000, "Could not find pagination links");
                browser.wait(until.elementToBeClickable(motdModel.overview.del.get(0)), 5000, "Could not find entries of new page");
                browser.wait(until.elementToBeClickable(motdModel.overview.pageLinks.get(0)), 5000, "Could not find pagination component");

                motdModel.overview.pageLinks.get(0).click();
                
                browser.wait(motdModel.overview.pageLinks.get(0).isSelected(), 5000, "Cloud not determine if page is selected");
                browser.wait(until.elementToBeClickable(motdModel.overview.del.get(0)), 5000, "Could not find entries of first page");
                
            });

            it('should be able to delete generated messages', function() {
                for (let i = 0; i < num_messages; i++) {
                    deleteFirstOverviewMessage();
                }

                browser.wait(until.not(until.visibilityOf(motdModel.overview.del.get(0))), 5000, "Entry still found after all elements hould have been deleted");
                browser.wait(until.visibilityOf(motdModel.overview.noMessages.get(0)), 5000, "Could not determine if all messages were deleted");
                browser.wait(until.visibilityOf(motdModel.overview.noMessages.get(1)), 5000, "Could not determine if all messages were deleted");
            });

            it('should be able to create and delete a planned message', function() {
                motdModel.overview.addButton.click();
                motdModel.overview.textarea.sendKeys("Integration Planned Test message");
                motdModel.overview.datepicker.checkbox.get(0).click();
                motdModel.overview.datepicker.obj.get(0).clear().then(function() {
                    motdModel.overview.datepicker.obj.get(0).sendKeys("2020-01-01");
                });
                
                motdModel.overview.submit.click();
                browser.wait(until.not(until.visibilityOf(motdModel.overview.submit)), 5000, "Cloud not determine if modal has closed");
                browser.wait(until.elementToBeClickable(motdModel.overview.del.get(0)), 5000, "Could not find entered item");
                browser.wait(until.visibilityOf(motdModel.overview.noMessages.get(0)), 5000, "Could not determine if all messages were deleted");
                
                deleteFirstOverviewMessage();

                browser.wait(until.not(until.visibilityOf(motdModel.overview.del.get(0))), 5000, "Entry still found after all elements hould have been deleted");
                browser.wait(until.visibilityOf(motdModel.overview.noMessages.get(0)), 5000, "Could not determine if all messages were deleted");
                browser.wait(until.visibilityOf(motdModel.overview.noMessages.get(1)), 5000, "Could not determine if all messages were deleted");
            });

            it('should be able to edit a message', function() {
                addMessageFromOverview(1);
                browser.wait(until.elementToBeClickable(motdModel.overview.edit.get(0)), 5000, "Can't find cog icon to edit message");            
                browser.wait(until.visibilityOf(motdModel.overview.noMessages.get(0)), 5000, "Could not determine if planned messages loaded correctly");
                
                motdModel.overview.edit.get(0).click();

                // change priority from 0
                browser.wait(until.elementToBeClickable(motdModel.overview.priority.obj), 5000, "Could not find priority dropdown");
                motdModel.overview.priority.obj.click();
                browser.wait(until.elementToBeClickable(motdModel.overview.priority.options.get(3)), 5000, "Could not find pagination component");
                motdModel.overview.priority.options.get(3).click();
                
                browser.wait(until.elementToBeClickable(motdModel.overview.submit), 5000, "Could not click modals's submit button.");
                
                motdModel.overview.submit.click();
                browser.wait(until.not(until.visibilityOf(motdModel.overview.submit)), 5000, "Cloud not determine if modal has closed");
                browser.wait(until.elementToBeClickable(motdModel.overview.del.get(0)), 5000, "Could not determine if mofified message loaded");
                browser.wait(until.visibilityOf(motdModel.overview.noMessages.get(0)), 5000, "Could not determine if all Planned messages were deleted");

                deleteFirstOverviewMessage();
                
                browser.wait(until.not(until.visibilityOf(motdModel.overview.del.get(0))), 5000, "Entry still found after all elements hould have been deleted");
                browser.wait(until.visibilityOf(motdModel.overview.noMessages.get(0)), 5000, "Could not determine if all messages were deleted");
                browser.wait(until.visibilityOf(motdModel.overview.noMessages.get(1)), 5000, "Could not determine if all messages were deleted");
            });

            it('should not allow an empty message', function() {
                browser.wait(until.elementToBeClickable(motdModel.overview.addButton), 5000, "Cloud not determine if page is ready");

                motdModel.overview.addButton.click();
                browser.wait(until.elementToBeClickable(motdModel.overview.submit), 5000, "Cloud not determine if modal has closed");

                motdModel.overview.submit.click();
                browser.wait(until.presenceOf(motdModel.overview.formFeedback), 5000, "Could not find form error message");

                motdModel.overview.formFeedback.getText().then(function(text) {
                    expect(text).toBeNonEmptyString();                
                });

                motdModel.overview.closeModal.click();
                browser.wait(until.not(until.visibilityOf(motdModel.overview.closeModal)), 5000, "Cloud not determine if modal has closed");
            });
        });

        // REST Explorer Tests
        describe('REST Explorer tests', function() {
            it('should be able to add a new message via REST explorer', function() {
                motdModel.facets.get(2).click();
                addMessageFromREST('Test REST Message', '', '', 0);  // Add message with no start or end time
                
                browser.wait(until.elementToBeClickable(motdModel.facets.get(0)), 5000, "Could not click on Overview Tab");
                motdModel.facets.get(0).click();

                browser.wait(until.elementToBeClickable(motdModel.overview.del.get(0)), 5000, "Clould not determine if Overview page was loaded");
                expect(motdModel.overview.messageEntries.count()).toEqual(1);

                deleteFirstOverviewMessage();
                
                browser.wait(until.not(until.visibilityOf(motdModel.overview.del.get(0))), 5000, "Entry still found after all elements hould have been deleted");
                browser.wait(until.visibilityOf(motdModel.overview.noMessages.get(0)), 5000, "Could not determine if all messages were deleted");
                browser.wait(until.visibilityOf(motdModel.overview.noMessages.get(1)), 5000, "Could not determine if all messages were deleted");
            });

            it('should be able to add an expired message via REST explorer', function() {
                motdModel.facets.get(2).click();
                addMessageFromREST('Test REST Message', 'Jan 1 2010 12:00', 'Dec 30 2010 12:00', 0); // Add message with a start and ent time in the past

                motdModel.facets.get(1).click();
                browser.wait(until.visibilityOf(motdModel.history.del.get(0)), 5000, "Cloud not determine if history page has loaded");
                
                expect(motdModel.history.messageEntries.count()).toEqual(1);

                deleteFirstHistoryMessage();
                browser.wait(until.not(until.visibilityOf(motdModel.history.del.get(0))), 5000, "Cloud not determine if message has deleted");
                browser.wait(until.visibilityOf(motdModel.history.noMessages), 5000, "Could not determine if all messages were deleted");
            });
        });
    });

    function addMessageFromOverview(count) {
        for (let i = 0; i < count; i++) {                    
            browser.wait(until.elementToBeClickable(motdModel.overview.addButton), 5000, "Cloud not determine if Overivew page is loaded");
            
            motdModel.overview.addButton.click();
            browser.wait(until.elementToBeClickable(motdModel.overview.submit), 5000, "Could not find add modal");

            motdModel.overview.textarea.sendKeys("Integration Test message");
            motdModel.overview.datepicker.checkbox.get(0).click();
            motdModel.overview.datepicker.checkbox.get(1).click();
            
            motdModel.overview.datepicker.obj.get(0).clear().then(function() {
                motdModel.overview.datepicker.obj.get(0).sendKeys("2017-01-01");
            });

            motdModel.overview.datepicker.obj.get(1).clear().then(function() {
                motdModel.overview.datepicker.obj.get(1).sendKeys("2020-01-01");
            });
            
            motdModel.overview.submit.click();
            browser.wait(until.not(until.visibilityOf(motdModel.overview.submit)), 5000, "Cloud not determine if modal has closed");
        }
    }

    function addMessageFromREST(message, startTime, endTime, priority) {
        browser.wait(until.elementToBeClickable(motdModel.restExplorer.path.obj), 5000, "Cloud not determine if REST Explorere page loaded");

        motdModel.restExplorer.path.obj.click();
        browser.wait(until.elementToBeClickable(motdModel.restExplorer.path.options.get(3)), 5000, "Cloud not find opened path dropdown menu");

        motdModel.restExplorer.path.options.get(0).click();
        browser.wait(until.elementToBeClickable(motdModel.restExplorer.verb.obj), 5000, "Cloud not find verb dropdown menu");

        motdModel.restExplorer.verb.obj.click();
        browser.wait(until.elementToBeClickable(motdModel.restExplorer.verb.options.get(1)), 5000, "Could not find the verb dropdown options");

        motdModel.restExplorer.verb.options.get(1).click();
        browser.wait(until.elementToBeClickable(motdModel.restExplorer.adminMessageSwaggerBodyFields.get(3)), 5000, "Could not find the path swagger body");

        motdModel.restExplorer.adminMessageSwaggerBodyFields.get(0).sendKeys(endTime);
        motdModel.restExplorer.adminMessageSwaggerBodyFields.get(1).sendKeys(message);
        motdModel.restExplorer.adminMessageSwaggerBodyFields.get(2).sendKeys(priority);
        motdModel.restExplorer.adminMessageSwaggerBodyFields.get(3).sendKeys(startTime);

        browser.wait(until.elementToBeClickable(motdModel.restExplorer.sendButton), 5000, "Could not access the send button");
        motdModel.restExplorer.sendButton.click();

        browser.wait(until.visibilityOf(motdModel.restExplorer.resposneArea), 5000, "Could not determine if response area was loaded");
    }

    function deleteFirstOverviewMessage() {
        browser.driver.sleep(1000);

        browser.wait(until.elementToBeClickable(motdModel.overview.del.get(0)), 5000, "Can't find trash icon for deleting messages");
        motdModel.overview.del.get(0).click();

        browser.wait(until.elementToBeClickable(motdModel.overview.submit), 5000, "Could not confirm deletion of entry");
        motdModel.overview.submit.click();

        browser.wait(until.not(until.visibilityOf(motdModel.overview.submit)), 5000, "Cloud not determine if modal has closed");
        browser.wait(until.elementToBeClickable(motdModel.facets.get(0)), 5000, "Could not confirm deletion of entry");
    }

    function deleteFirstHistoryMessage() {
        browser.driver.sleep(1000);
        
        browser.wait(until.elementToBeClickable(motdModel.history.del.get(0)), 5000, "Can't find trash icon for deleting messages");
        motdModel.history.del.get(0).click();

        browser.wait(until.elementToBeClickable(motdModel.overview.submit), 5000, "Could not confirm deletion of entry");
        motdModel.overview.submit.click();

        browser.wait(until.not(until.visibilityOf(motdModel.overview.submit)), 5000, "Cloud not determine if modal has closed");
        browser.wait(until.elementToBeClickable(motdModel.facets.get(0)), 5000, "Could not confirm deletion of entry");
    }
});
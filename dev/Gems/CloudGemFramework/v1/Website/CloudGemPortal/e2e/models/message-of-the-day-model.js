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

var motdModel = module.exports = {
    // CGP defaults
    cog: $('.fa.fa-cog'),
    thumbnail: $('thumbnail-gem[ng-reflect-title="Message of the day"]'),
    refresh: $('i.fa.refresh-icon'),
    // Navigation Facets
    facets : $$('message-of-the-day-index facet-generator .row .tab a'),

    // Overview FACET model
    overview: {
        addButton: $('button.add-motd-button'),
        // Add Message Modal
        textarea: $('textarea'),
        formFeedback: $('.form-control-feedback'),
        datepicker: {
            obj: $$('.datepicker'),
            checkbox: $$('.l-checkbox') 
        },
        priority: {
            obj: $('#priority-dropdown'),
            options: $$("button.dropdown-item")
        },
        closeModal: $('#modal-dismiss'),
        submit: $('#modal-submit'),

        // Added Messages
        edit: $$('.edit'),
        del: $$('.fa.fa-trash-o'),
        messageEntries: $$('div table tbody tr'),
        pageLinks: $$('a.page-link'),
        noMessages: $$('.messages-container div')
    },

    // History FACET model
    history: {
        messageContainer: $('.messages-container'),
        messageEntries: $$('div table tbody tr'),
        del: $$('.fa.fa-trash-o'),
        edit: $$('.fa.fa-cog'),
        noMessages: $('.messages-container div')
    },

    // REST Explorer FACET model
    restExplorer: {
        path: {
            obj: $('button#path-dropdown'),
            options: $$('div.d-inline-block div.dropdown-menu.dropdown-menu-center button.dropdown-item')
        },
        verb: {
            obj: $('#verb-dropdown'),
            options: $$('div.dropdown-menu.dropdown-menu-right button.dropdown-item')
        },
        pathSwaggerBody: $('body-tree-view'),
        adminMessageSwaggerBodyFields: $$('body-tree-view div div div input'),
        sendButton: $('div.container.rest-container div button.btn.l-primary'),
        resposneArea: $('.col-12.response-area')
    }
}


(function e(t,n,r){function s(o,u){if(!n[o]){if(!t[o]){var a=typeof require=="function"&&require;if(!u&&a)return a(o,!0);if(i)return i(o,!0);var f=new Error("Cannot find module '"+o+"'");throw f.code="MODULE_NOT_FOUND",f}var l=n[o]={exports:{}};t[o][0].call(l.exports,function(e){var n=t[o][1][e];return s(n?n:e)},l,l.exports,e,t,n,r)}return n[o].exports}var i=typeof require=="function"&&require;for(var o=0;o<r.length;o++)s(r[o]);return s})({1:[function(require,module,exports){
(function (global){

var AMA = global.AMA;
AMA.Storage = require('./StorageClients/LocalStorage.js');
AMA.StorageKeys = require('./StorageClients/StorageKeys.js');
AMA.Util = require('./MobileAnalyticsUtilities.js');
/**
 * @typedef AMA.Client.Options
 * @property {string}                     appId - The Application ID from the Amazon Mobile Analytics Console
 * @property {string}                     [apiVersion=2014-06-05] - The version of the Mobile Analytics API to submit to.
 * @property {object}                     [provider=AWS.config.credentials] - Credentials to use for submitting events.
 *                                                                            **Never check in credentials to source
 *                                                                            control.
 * @property {boolean}                    [autoSubmitEvents=true] - Automatically Submit Events, Default: true
 * @property {number}                     [autoSubmitInterval=10000] - Interval to try to submit events in ms,
 *                                                                     Default: 10s
 * @property {number}                     [batchSizeLimit=256000] - Batch Size in Bytes, Default: 256Kb
 * @property {AMA.Client.SubmitCallback}  [submitCallback=] - Callback function that is executed when events are
 *                                                            successfully submitted
 * @property {AMA.Client.Attributes}      [globalAttributes=] - Attribute to be applied to every event, may be
 *                                                              overwritten with a different value when recording events.
 * @property {AMA.Client.Metrics}         [globalMetrics=] - Metric to be applied to every event, may be overwritten
 *                                                           with a different value when recording events.
 * @property {string}                     [clientId=GUID()] - A unique identifier representing this installation instance
 *                                                            of your app. This will be managed and persisted by the SDK
 *                                                            by default.
 * @property {string}                     [appTitle=] - The title of your app. For example, My App.
 * @property {string}                     [appVersionName=] - The version of your app. For example, V2.0.
 * @property {string}                     [appVersionCode=] - The version code for your app. For example, 3.
 * @property {string}                     [appPackageName=] - The name of your package. For example, com.example.my_app.
 * @property {string}                     [platform=] - The operating system of the device. For example, iPhoneOS.
 * @property {string}                     [plaformVersion=] - The version of the operating system of the device.
 *                                                            For example, 4.0.4.
 * @property {string}                     [model=] - The model of the device. For example, Nexus.
 * @property {string}                     [make=] - The manufacturer of the device. For example, Samsung.
 * @property {string}                     [locale=] - The locale of the device. For example, en_US.
 * @property {AMA.Client.Logger}          [logger=] - Object of logger functions
 * @property {AMA.Storage}                [storage=] - Storage client to persist events, will create a new AMA.Storage if not provided
 * @property {Object}                     [clientOptions=] - Low level client options to be passed to the AWS.MobileAnalytics low level SDK
 */

/**
 * @typedef AMA.Client.Logger
 * @description Uses Javascript Style log levels, one function for each level.  Basic usage is to pass the console object
 *              which will output directly to browser developer console.
 * @property {Function} [log=] - Logger for client log level messages
 * @property {Function} [info=] - Logger for interaction level messages
 * @property {Function} [warn=] - Logger for warn level messages
 * @property {Function} [error=] - Logger for error level messages
 */
/**
 * @typedef AMA.Client.Attributes
 * @type {object}
 * @description A collection of key-value pairs that give additional context to the event. The key-value pairs are
 *              specified by the developer.
 */
/**
 * @typedef AMA.Client.Metrics
 * @type {object}
 * @description A collection of key-value pairs that gives additional measurable context to the event. The pairs
 *              specified by the developer.
 */
/**
 * @callback AMA.Client.SubmitCallback
 * @param {Error} err
 * @param {Null} data
 * @param {string} batchId
 */
/**
 * @typedef AMA.Client.Event
 * @type {object}
 * @description A JSON object representing an event occurrence in your app and consists of the following:
 * @property {string} eventType - A name signifying an event that occurred in your app. This is used for grouping and
 *                                aggregating like events together for reporting purposes.
 * @property {string} timestamp - The time the event occurred in ISO 8601 standard date time format.
 *                                For example, 2014-06-30T19:07:47.885Z
 * @property {AMA.Client.Attributes} [attributes=] - A collection of key-value pairs that give additional context to
 *                                                   the event. The key-value pairs are specified by the developer.
 *                                                   This collection can be empty or the attribute object can be omitted.
 * @property {AMA.Client.Metrics} [metrics=] - A collection of key-value pairs that gives additional measurable context
 *                                             to the event. The pairs specified by the developer.
 * @property {AMA.Session} session - Describes the session. Session information is required on ALL events.
 */
/**
 * @name AMA.Client
 * @namespace AMA.Client
 * @constructor
 * @param {AMA.Client.Options} options - A configuration map for the AMA.Client
 * @returns A new instance of the Mobile Analytics Mid Level Client
 */
AMA.Client = (function () {
    'use strict';
    /**
     * @lends AMA.Client
     */
    var Client = function (options) {
        //This register the bind function for older browsers
        //https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Function/bind
        if (!Function.prototype.bind) {
            Function.prototype.bind = function (oThis) {
                if (typeof this !== 'function') {
                    // closest thing possible to the ECMAScript 5 internal IsCallable function
                    //throw new TypeError('Function.prototype.bind - what is trying to be bound is not callable');
                    this.logger.error('Function.prototype.bind - what is trying to be bound is not callable');
                }
                var aArgs   = Array.prototype.slice.call(arguments, 1),
                    fToBind = this,
                    fBound  = function () {
                        return fToBind.apply(
                            this instanceof AMA.Util.NOP && oThis ? this : oThis,
                            aArgs.concat(Array.prototype.slice.call(arguments))
                        );
                    };
                AMA.Util.NOP.prototype = this.prototype;
                fBound.prototype = new AMA.Util.NOP();
                return fBound;
            };
        }

        this.options = options || {};
        this.options.logger = this.options.logger || {};
        this.logger = {
            log  : this.options.logger.log || AMA.Util.NOP,
            info : this.options.logger.info || AMA.Util.NOP,
            warn : this.options.logger.warn || AMA.Util.NOP,
            error: this.options.logger.error || AMA.Util.NOP
        };
        this.logger.log = this.logger.log.bind(this.options.logger);
        this.logger.info = this.logger.info.bind(this.options.logger);
        this.logger.warn = this.logger.warn.bind(this.options.logger);
        this.logger.error = this.logger.error.bind(this.options.logger);

        this.logger.log('[Function:(AMA)Client Constructor]' +
            (options ? '\noptions:' + JSON.stringify(options) : ''));

        if (options.appId === undefined) {
            this.logger.error('AMA.Client must be initialized with an appId');
            return null; //No need to run rest of init since appId is required
        }
        if (options.platform === undefined) {
            this.logger.error('AMA.Client must be initialized with a platform');
        }
        this.storage = this.options.storage || new AMA.Storage(options.appId);
        this.storage.setLogger(this.logger);

        this.options.apiVersion = this.options.apiVersion || '2014-06-05';
        this.options.provider = this.options.provider || AWS.config.credentials;
        this.options.autoSubmitEvents = options.autoSubmitEvents !== false;
        this.options.autoSubmitInterval = this.options.autoSubmitInterval || 10000;
        this.options.batchSizeLimit = this.options.batchSizeLimit || 256000;
        this.options.submitCallback = this.options.submitCallback || AMA.Util.NOP;
        this.options.globalAttributes = this.options.globalAttributes || {};
        this.options.globalMetrics = this.options.globalMetrics || {};
        this.options.clientOptions = this.options.clientOptions || {};
        this.options.clientOptions.provider = this.options.clientOptions.provider || this.options.provider;
        this.options.clientOptions.apiVersion = this.options.clientOptions.apiVersion || this.options.apiVersion;
        this.options.clientOptions.correctClockSkew = this.options.clientOptions.correctClockSkew !== false;
        this.options.clientOptions.retryDelayOptions = this.options.clientOptions.retryDelayOptions || {};
        this.options.clientOptions.retryDelayOptions.base = this.options.clientOptions.retryDelayOptions.base || 3000;        

        this.storage.set(
            AMA.StorageKeys.GLOBAL_ATTRIBUTES,
            AMA.Util.mergeObjects(this.options.globalAttributes,
                this.storage.get(AMA.StorageKeys.GLOBAL_ATTRIBUTES) || {})
        );
        this.storage.set(
            AMA.StorageKeys.GLOBAL_METRICS,
            AMA.Util.mergeObjects(this.options.globalMetrics,
                this.storage.get(AMA.StorageKeys.GLOBAL_METRICS) || {})
        );

        var v091ClientId = this.storage.get(AMA.StorageKeys.CLIENT_ID);
        try {
            if (window && window.localStorage) {
                var v090Storage = window.localStorage.getItem('AWSMobileAnalyticsStorage');
                if (v090Storage) {
                    try {
                        v090Storage = JSON.parse(v090Storage);
                        var v090ClientId = v090Storage.AWSMobileAnalyticsClientId;
                        if (v090ClientId && v091ClientId && v091ClientId !== v090ClientId) {
                            this.options.globalAttributes.migrationId = v091ClientId;
                        }
                        if (v090ClientId) {
                            v091ClientId = v090ClientId;
                        }
                    } catch (err) {
                        this.logger.warn('Had corrupt v0.9.0 Storage');
                    }
                }
            }
        } catch (err) {
            this.logger.warn('window is undefined, unable to check for v090 data');
        }

        this.options.clientContext = this.options.clientContext || {
                'client'  : {
                    'client_id'       : this.options.clientId || v091ClientId || AMA.Util.GUID(),
                    'app_title'       : this.options.appTitle,
                    'app_version_name': this.options.appVersionName,
                    'app_version_code': this.options.appVersionCode,
                    'app_package_name': this.options.appPackageName
                },
                'env'     : {
                    'platform'        : this.options.platform,
                    'platform_version': this.options.platformVersion,
                    'model'           : this.options.model,
                    'make'            : this.options.make,
                    'locale'          : this.options.locale
                },
                'services': {
                    'mobile_analytics': {
                        'app_id'     : this.options.appId,
                        'sdk_name'   : 'aws-sdk-mobile-analytics-js',
                        'sdk_version': '0.9.2' + ':' + AWS.VERSION
                    }
                },
                'custom'  : {}
            };

        this.storage.set(AMA.StorageKeys.CLIENT_ID, this.options.clientContext.client.client_id);

        this.StorageKeys = {
            'EVENTS'     : 'AWSMobileAnalyticsEventStorage',
            'BATCHES'    : 'AWSMobileAnalyticsBatchStorage',
            'BATCH_INDEX': 'AWSMobileAnalyticsBatchIndexStorage'
        };

        this.outputs = {};
        this.outputs.MobileAnalytics = new AWS.MobileAnalytics(this.options.clientOptions);
        this.outputs.timeoutReference = null;
        this.outputs.batchesInFlight = {};

        this.outputs.events = this.storage.get(this.StorageKeys.EVENTS) || [];
        this.outputs.batches = this.storage.get(this.StorageKeys.BATCHES) || {};
        this.outputs.batchIndex = this.storage.get(this.StorageKeys.BATCH_INDEX) || [];

        if (this.options.autoSubmitEvents) {
            this.submitEvents();
        }
    };

    Client.prototype.validateEvent = function (event) {
        var self = this, invalidMetrics = [];

        function customNameErrorFilter(name) {
            if (name.length === 0) {
                return true;
            }
            return name.length > 50;
        }

        function customAttrValueErrorFilter(name) {
            return event.attributes[name] && event.attributes[name].length > 200;
        }

        function validationError(errorMsg) {
            self.logger.error(errorMsg);
            return null;
        }

        invalidMetrics = Object.keys(event.metrics).filter(function (metricName) {
            return typeof event.metrics[metricName] !== 'number';
        });
        if (event.version !== 'v2.0') {
            return validationError('Event must have version v2.0');
        }
        if (typeof event.eventType !== 'string') {
            return validationError('Event Type must be a string');
        }
        if (invalidMetrics.length > 0) {
            return validationError('Event Metrics must be numeric (' + invalidMetrics[0] + ')');
        }
        if (Object.keys(event.metrics).length + Object.keys(event.attributes).length > 40) {
            return validationError('Event Metric and Attribute Count cannot exceed 40');
        }
        if (Object.keys(event.attributes).filter(customNameErrorFilter).length) {
            return validationError('Event Attribute names must be 1-50 characters');
        }
        if (Object.keys(event.metrics).filter(customNameErrorFilter).length) {
            return validationError('Event Metric names must be 1-50 characters');
        }
        if (Object.keys(event.attributes).filter(customAttrValueErrorFilter).length) {
            return validationError('Event Attribute values cannot be longer than 200 characters');
        }
        return event;
    };

    /**
     * AMA.Client.createEvent
     * @param {string} eventType - Custom Event Type to be displayed in Console
     * @param {AMA.Session} session - Session Object (required for use within console)
     * @param {string} session.id - Identifier for current session
     * @param {string} session.startTimestamp - Timestamp that indicates the start of the session
     * @param [attributes=] - Custom attributes
     * @param [metrics=] - Custom metrics
     * @returns {AMA.Event}
     */
    Client.prototype.createEvent = function (eventType, session, attributes, metrics) {
        var that = this;
        this.logger.log('[Function:(AMA.Client).createEvent]' +
            (eventType ? '\neventType:' + eventType : '') +
            (session ? '\nsession:' + session : '') +
            (attributes ? '\nattributes:' + JSON.stringify(attributes) : '') +
            (metrics ? '\nmetrics:' + JSON.stringify(metrics) : ''));
        attributes = attributes || {};
        metrics = metrics || {};

        AMA.Util.mergeObjects(attributes, this.options.globalAttributes);
        AMA.Util.mergeObjects(metrics, this.options.globalMetrics);

        Object.keys(attributes).forEach(function (name) {
            if (typeof attributes[name] !== 'string') {
                try {
                    attributes[name] = JSON.stringify(attributes[name]);
                } catch (e) {
                    that.logger.warn('Error parsing attribute ' + name);
                }
            }
        });
        var event = {
            eventType : eventType,
            timestamp : new Date().toISOString(),
            session   : {
                id            : session.id,
                startTimestamp: session.startTimestamp
            },
            version   : 'v2.0',
            attributes: attributes,
            metrics   : metrics
        };
        if (session.stopTimestamp) {
            event.session.stopTimestamp = session.stopTimestamp;
            event.session.duration = new Date(event.stopTimestamp).getTime() - new Date(event.startTimestamp).getTime();
        }
        return this.validateEvent(event);
    };

    /**
     * AMA.Client.pushEvent
     * @param {AMA.Event} event - event to be pushed onto queue
     * @returns {int} Index of event in outputs.events
     */
    Client.prototype.pushEvent = function (event) {
        if (!event) {
            return -1;
        }
        this.logger.log('[Function:(AMA.Client).pushEvent]' +
            (event ? '\nevent:' + JSON.stringify(event) : ''));
        //Push adds to the end of array and returns the size of the array
        var eventIndex = this.outputs.events.push(event);
        this.storage.set(this.StorageKeys.EVENTS, this.outputs.events);
        return (eventIndex - 1);
    };

    /**
     * Helper to record events, will automatically submit if the events exceed batchSizeLimit
     * @param {string}                eventType - Custom event type name
     * @param {AMA.Session}           session - Session object
     * @param {AMA.Client.Attributes} [attributes=] - Custom attributes
     * @param {AMA.Client.Metrics}    [metrics=] - Custom metrics
     * @returns {AMA.Event} The event that was recorded
     */
    Client.prototype.recordEvent = function (eventType, session, attributes, metrics) {
        this.logger.log('[Function:(AMA.Client).recordEvent]' +
            (eventType ? '\neventType:' + eventType : '') +
            (session ? '\nsession:' + session : '') +
            (attributes ? '\nattributes:' + JSON.stringify(attributes) : '') +
            (metrics ? '\nmetrics:' + JSON.stringify(metrics) : ''));
        var index, event = this.createEvent(eventType, session, attributes, metrics);
        if (event) {
            index = this.pushEvent(event);
            if (AMA.Util.getRequestBodySize(this.outputs.events) >= this.options.batchSizeLimit) {
                this.submitEvents();
            }
            return this.outputs.events[index];
        }
        return null;
    };

    /**
     * recordMonetizationEvent
     * @param session
     * @param {Object}        monetizationDetails - Details about Monetization Event
     * @param {string}        monetizationDetails.currency - ISO Currency of event
     * @param {string}        monetizationDetails.productId - Product Id of monetization event
     * @param {number}        monetizationDetails.quantity - Quantity of product in transaction
     * @param {string|number} monetizationDetails.price - Price of product either ISO formatted string, or number
     *                                                    with associated ISO Currency
     * @param {AMA.Client.Attributes} [attributes=] - Custom attributes
     * @param {AMA.Client.Metrics}    [metrics=] - Custom metrics
     * @returns {event} The event that was recorded
     */
    Client.prototype.recordMonetizationEvent = function (session, monetizationDetails, attributes, metrics) {
        this.logger.log('[Function:(AMA.Client).recordMonetizationEvent]' +
            (session ? '\nsession:' + session : '') +
            (monetizationDetails ? '\nmonetizationDetails:' + JSON.stringify(monetizationDetails) : '') +
            (attributes ? '\nattributes:' + JSON.stringify(attributes) : '') +
            (metrics ? '\nmetrics:' + JSON.stringify(metrics) : ''));

        attributes = attributes || {};
        metrics = metrics || {};
        attributes._currency = monetizationDetails.currency || attributes._currency;
        attributes._product_id = monetizationDetails.productId || attributes._product_id;
        metrics._quantity = monetizationDetails.quantity || metrics._quantity;
        if (typeof monetizationDetails.price === 'number') {
            metrics._item_price = monetizationDetails.price || metrics._item_price;
        } else {
            attributes._item_price_formatted = monetizationDetails.price || attributes._item_price_formatted;
        }
        return this.recordEvent('_monetization.purchase', session, attributes, metrics);
    };
    /**
     * submitEvents
     * @param {Object} [options=] - options for submitting events
     * @param {Object} [options.clientContext=this.options.clientContext] - clientContext to submit with defaults
     *                                                                      to options.clientContext
     * @param {SubmitCallback} [options.submitCallback=this.options.submitCallback] - Callback function that is executed
     *                                                                                when events are successfully
     *                                                                                submitted
     * @returns {Array} Array of batch indices that were submitted
     */
    Client.prototype.submitEvents = function (options) {
        options = options || {};
        options.submitCallback = options.submitCallback || this.options.submitCallback;
        this.logger.log('[Function:(AMA.Client).submitEvents]' +
            (options ? '\noptions:' + JSON.stringify(options) : ''));


        if (this.options.autoSubmitEvents) {
            clearTimeout(this.outputs.timeoutReference);
            this.outputs.timeoutReference = setTimeout(this.submitEvents.bind(this), this.options.autoSubmitInterval);
        }
        var warnMessage;
        //Get distribution of retries across clients by introducing a weighted rand.
        //Probability will increase over time to an upper limit of 60s
        if (this.outputs.isThrottled && this.throttlingSuppressionFunction() < Math.random()) {
            warnMessage = 'Prevented submission while throttled';
        } else if (Object.keys(this.outputs.batchesInFlight).length > 0) {
            warnMessage = 'Prevented submission while batches are in flight';
        } else if (this.outputs.batches.length === 0 && this.outputs.events.length === 0) {
            warnMessage = 'No batches or events to be submitted';
        } else if (this.outputs.lastSubmitTimestamp && AMA.Util.timestamp() - this.outputs.lastSubmitTimestamp < 1000) {
            warnMessage = 'Prevented multiple submissions in under a second';
        }
        if (warnMessage) {
            this.logger.warn(warnMessage);
            return [];
        }
        this.generateBatches();

        this.outputs.lastSubmitTimestamp = AMA.Util.timestamp();
        if (this.outputs.isThrottled) {
            //Only submit the first batch if throttled
            this.logger.warn('Is throttled submitting first batch');
            options.batchId = this.outputs.batchIndex[0];
            return [this.submitBatchById(options)];
        }

        return this.submitAllBatches(options);
    };

    Client.prototype.throttlingSuppressionFunction = function (timestamp) {
        timestamp = timestamp || AMA.Util.timestamp();
        return Math.pow(timestamp - this.outputs.lastSubmitTimestamp, 2) / Math.pow(60000, 2);
    };

    Client.prototype.generateBatches = function () {
        while (this.outputs.events.length > 0) {
            var lastIndex = this.outputs.events.length;
            this.logger.log(this.outputs.events.length + ' events to be submitted');
            while (lastIndex > 1 &&
            AMA.Util.getRequestBodySize(this.outputs.events.slice(0, lastIndex)) > this.options.batchSizeLimit) {
                this.logger.log('Finding Batch Size (' + this.options.batchSizeLimit + '): ' + lastIndex + '(' +
                    AMA.Util.getRequestBodySize(this.outputs.events.slice(0, lastIndex)) + ')');
                lastIndex -= 1;
            }
            if (this.persistBatch(this.outputs.events.slice(0, lastIndex))) {
                //Clear event queue
                this.outputs.events.splice(0, lastIndex);
                this.storage.set(this.StorageKeys.EVENTS, this.outputs.events);
            }
        }
    };

    Client.prototype.persistBatch = function (eventBatch) {
        this.logger.log(eventBatch.length + ' events in batch');
        if (AMA.Util.getRequestBodySize(eventBatch) < 512000) {
            var batchId = AMA.Util.GUID();
            //Save batch so data is not lost.
            this.outputs.batches[batchId] = eventBatch;
            this.storage.set(this.StorageKeys.BATCHES, this.outputs.batches);
            this.outputs.batchIndex.push(batchId);
            this.storage.set(this.StorageKeys.BATCH_INDEX, this.outputs.batchIndex);
            return true;
        }
        this.logger.error('Events too large');
        return false;
    };

    Client.prototype.submitAllBatches = function (options) {
        options.submitCallback = options.submitCallback || this.options.submitCallback;
        this.logger.log('[Function:(AMA.Client).submitAllBatches]' +
            (options ? '\noptions:' + JSON.stringify(options) : ''));
        var indices = [],
            that    = this;
        this.outputs.batchIndex.forEach(function (batchIndex) {
            options.batchId = batchIndex;
            options.clientContext = options.clientContext || that.options.clientContext;
            if (!that.outputs.batchesInFlight[batchIndex]) {
                indices.push(that.submitBatchById(options));
            }
        });
        return indices;
    };

    Client.NON_RETRYABLE_EXCEPTIONS = ['BadRequestException', 'SerializationException', 'ValidationException'];
    Client.prototype.submitBatchById = function (options) {
        if (typeof(options) !== 'object' || !options.batchId) {
            this.logger.error('Invalid Options passed to submitBatchById');
            return;
        }
        options.submitCallback = options.submitCallback || this.options.submitCallback;
        this.logger.log('[Function:(AMA.Client).submitBatchById]' +
            (options ? '\noptions:' + JSON.stringify(options) : ''));
        var eventBatch = {
            'events'       : this.outputs.batches[options.batchId],
            'clientContext': JSON.stringify(options.clientContext || this.options.clientContext)
        };
        this.outputs.batchesInFlight[options.batchId] = AMA.Util.timestamp();
        this.outputs.MobileAnalytics.putEvents(eventBatch,
            this.handlePutEventsResponse(options.batchId, options.submitCallback));
        return options.batchId;
    };

    Client.prototype.handlePutEventsResponse = function (batchId, callback) {
        var self = this;
        return function (err, data) {
            var clearBatch = true,
                wasThrottled = self.outputs.isThrottled;
            if (err) {
                self.logger.error(err, data);
                if (err.statusCode === undefined || err.statusCode === 400) {
                    if (Client.NON_RETRYABLE_EXCEPTIONS.indexOf(err.code) < 0) {
                        clearBatch = false;
                    }
                    self.outputs.isThrottled = err.code === 'ThrottlingException';
                    if (self.outputs.isThrottled) {
                        self.logger.warn('Application is currently throttled');
                    }
                }
            } else {
                self.logger.info('Events Submitted Successfully');
                self.outputs.isThrottled = false;
            }
            if (clearBatch) {
                self.clearBatchById(batchId);
            }
            delete self.outputs.batchesInFlight[batchId];
            callback(err, data, batchId);
            if (wasThrottled && !self.outputs.isThrottled) {
                self.logger.warn('Was throttled flushing remaining batches', callback);
                self.submitAllBatches({
                    submitCallback: callback
                });
            }
        };
    };

    Client.prototype.clearBatchById = function (batchId) {
        this.logger.log('[Function:(AMA.Client).clearBatchById]' +
            (batchId ? '\nbatchId:' + batchId : ''));
        if (this.outputs.batchIndex.indexOf(batchId) !== -1) {
            delete this.outputs.batches[batchId];
            this.outputs.batchIndex.splice(this.outputs.batchIndex.indexOf(batchId), 1);

            // Persist latest batches / events
            this.storage.set(this.StorageKeys.BATCH_INDEX, this.outputs.batchIndex);
            this.storage.set(this.StorageKeys.BATCHES, this.outputs.batches);
        }
    };

    return Client;
}());
module.exports = AMA.Client;

}).call(this,typeof global !== "undefined" ? global : typeof self !== "undefined" ? self : typeof window !== "undefined" ? window : {})
},{"./MobileAnalyticsUtilities.js":4,"./StorageClients/LocalStorage.js":5,"./StorageClients/StorageKeys.js":6}],2:[function(require,module,exports){
(function (global){
var AMA = global.AMA;
AMA.Storage = require('./StorageClients/LocalStorage.js');
AMA.StorageKeys = require('./StorageClients/StorageKeys.js');
AMA.Util = require('./MobileAnalyticsUtilities.js');
/**
 * @name AMA.Session
 * @namespace AMA.Session
 * @constructor
 * @param {Object=}     [options=] - A configuration map for the Session
 * @param {string=}     [options.sessionId=Utilities.GUID()]- A sessionId for session.
 * @param {string=}     [options.appId=new Date().toISOString()] - The start Timestamp (default now).
 * @param {number=}     [options.sessionLength=600000] - Length of session in Milliseconds (default 10 minutes).
 * @param {AMA.Session.ExpirationCallback=}   [options.expirationCallback] - Callback Function for when a session expires
 * @param {AMA.Client.Logger=} [options.logger=] - Object containing javascript style logger functions (passing console
 *                                                 will output to browser dev consoles)
 */
/**
 * @callback AMA.Session.ExpirationCallback
 * @param {AMA.Session} session
 * @returns {boolean|int} - Returns either true to extend the session by the sessionLength or an int with the number of
 *                          seconds to extend the session.  All other values will clear the session from storage.
 */
AMA.Session = (function () {
    'use strict';
    /**
     * @lends AMA.Session
     */
    var Session = function (options) {
        this.options = options || {};
        this.options.logger = this.options.logger || {};
        this.logger = {
            log: this.options.logger.log || AMA.Util.NOP,
            info: this.options.logger.info || AMA.Util.NOP,
            warn: this.options.logger.warn || AMA.Util.NOP,
            error: this.options.logger.error || AMA.Util.NOP
        };
        this.logger.log = this.logger.log.bind(this.options.logger);
        this.logger.info = this.logger.info.bind(this.options.logger);
        this.logger.warn = this.logger.warn.bind(this.options.logger);
        this.logger.error = this.logger.error.bind(this.options.logger);
        this.logger.log('[Function:(AWS.MobileAnalyticsClient)Session Constructor]' +
            (options ? '\noptions:' + JSON.stringify(options) : ''));
        this.options.expirationCallback = this.options.expirationCallback || AMA.Util.NOP;
        this.id = this.options.sessionId || AMA.Util.GUID();
        this.sessionLength = this.options.sessionLength || 600000; //Default session length is 10 minutes
        //Suffix the AMA.Storage Keys with Session Id to ensure proper scope
        this.StorageKeys = {
            'SESSION_ID': AMA.StorageKeys.SESSION_ID + this.id,
            'SESSION_EXPIRATION': AMA.StorageKeys.SESSION_EXPIRATION + this.id,
            'SESSION_START_TIMESTAMP': AMA.StorageKeys.SESSION_START_TIMESTAMP + this.id
        };
        this.startTimestamp = this.options.startTime ||
            this.options.storage.get(this.StorageKeys.SESSION_START_TIMESTAMP) ||
            new Date().toISOString();
        this.expirationDate = parseInt(this.options.storage.get(this.StorageKeys.SESSION_EXPIRATION), 10);
        if (isNaN(this.expirationDate)) {
            this.expirationDate = (new Date().getTime() + this.sessionLength);
        }
        this.options.storage.set(this.StorageKeys.SESSION_ID, this.id);
        this.options.storage.set(this.StorageKeys.SESSION_EXPIRATION, this.expirationDate);
        this.options.storage.set(this.StorageKeys.SESSION_START_TIMESTAMP, this.startTimestamp);
        this.sessionTimeoutReference = setTimeout(this.expireSession.bind(this), this.sessionLength);
    };

    /**
     * Expire session and clear session
     * @param {expirationCallback=} Callback function to call when sessions expire
     */
    Session.prototype.expireSession = function (expirationCallback) {
        this.logger.log('[Function:(Session).expireSession]');
        expirationCallback = expirationCallback || this.options.expirationCallback;
        var shouldExtend = expirationCallback(this);
        if (typeof shouldExtend === 'boolean' && shouldExtend) {
            shouldExtend = this.options.sessionLength;
        }
        if (typeof shouldExtend === 'number') {
            this.extendSession(shouldExtend);
        } else {
            this.clearSession();
        }
    };

    /**
     * Clear session from storage system
     */
    Session.prototype.clearSession = function () {
        this.logger.log('[Function:(Session).clearSession]');
        clearTimeout(this.sessionTimeoutReference);
        this.options.storage.delete(this.StorageKeys.SESSION_ID);
        this.options.storage.delete(this.StorageKeys.SESSION_EXPIRATION);
        this.options.storage.delete(this.StorageKeys.SESSION_START_TIMESTAMP);
    };



    /**
     * Extend session by adding to the expiration timestamp
     * @param {int} [sessionExtensionLength=sessionLength] - The number of milliseconds to add to the expiration date
     *                                                       (session length by default).
     */
    Session.prototype.extendSession = function (sessionExtensionLength) {
        this.logger.log('[Function:(Session).extendSession]' +
                        (sessionExtensionLength ? '\nsessionExtensionLength:' + sessionExtensionLength : ''));
        sessionExtensionLength = sessionExtensionLength || this.sessionLength;
        this.setSessionTimeout(this.expirationDate + parseInt(sessionExtensionLength, 10));
    };

    /**
     * @param {string} [stopDate=now] - The ISO Date String to set the stopTimestamp to (now for default).
     */
    Session.prototype.stopSession = function (stopDate) {
        this.logger.log('[Function:(Session).stopSession]' +  (stopDate ? '\nstopDate:' + stopDate : ''));
        this.stopTimestamp = stopDate || new Date().toISOString();
    };

    /**
     * Reset session timeout to expire in a given number of seconds
     * @param {int} [milliseconds=sessionLength] - The number of milliseconds until the session should expire (from now). 
     */
    Session.prototype.resetSessionTimeout = function (milliseconds) {
        this.logger.log('[Function:(Session).resetSessionTimeout]' +
                        (milliseconds ? '\nmilliseconds:' + milliseconds : ''));
        milliseconds = milliseconds || this.sessionLength;
        this.setSessionTimeout(new Date().getTime() + milliseconds);
    };

    /**
     * Setter for the session timeout
     * @param {int} timeout - epoch timestamp
     */
    Session.prototype.setSessionTimeout = function (timeout) {
        this.logger.log('[Function:(Session).setSessionTimeout]' +  (timeout ? '\ntimeout:' + timeout : ''));
        clearTimeout(this.sessionTimeoutReference);
        this.expirationDate = timeout;
        this.options.storage.set(this.StorageKeys.SESSION_EXPIRATION, this.expirationDate);
        this.sessionTimeoutReference = setTimeout(this.expireSession.bind(this),
            this.expirationDate - (new Date()).getTime());
    };
    return Session;
}());

module.exports = AMA.Session;

}).call(this,typeof global !== "undefined" ? global : typeof self !== "undefined" ? self : typeof window !== "undefined" ? window : {})
},{"./MobileAnalyticsUtilities.js":4,"./StorageClients/LocalStorage.js":5,"./StorageClients/StorageKeys.js":6}],3:[function(require,module,exports){
(function (global){
var AMA = global.AMA;
AMA.Storage = require('./StorageClients/LocalStorage.js');
AMA.StorageKeys = require('./StorageClients/StorageKeys.js');
AMA.Session = require('./MobileAnalyticsSession.js');
AMA.Client = require('./MobileAnalyticsClient.js');

/**
 * @typedef AMA.Manager.Options
 * @augments AMA.Client.Options
 * @property {AMA.Session.ExpirationCallback} [expirationCallback=] - Callback function to call when sessions expire
 */

/**
 * @name AMA.Manager
 * @namespace AMA.Manager
 * @constructor
 * @param {AMA.Client.Options|AMA.Client} options - A configuration map for the AMA.Client or an instantiated AMA.Client
 * @see AMA.Client
 */
AMA.Manager = (function () {
    'use strict';
    /**
     * @lends AMA.Manager
     */
    var Manager = function (options) {
        if (options instanceof AMA.Client) {
            this.client = options;
        } else {
            options._autoSubmitEvents = options.autoSubmitEvents;
            options.autoSubmitEvents = false;
            this.client = new AMA.Client(options);
            options.autoSubmitEvents = options._autoSubmitEvents !== false;
            delete options._autoSubmitEvents;
        }
        this.options = this.client.options;
        this.outputs = this.client.outputs;

        this.options.expirationCallback = this.options.expirationCallback || AMA.Util.NOP;
        function checkForStoredSessions(context) {
            context.client.storage.each(function (key) {
                if (key.indexOf(AMA.StorageKeys.SESSION_ID) === 0) {
                    context.outputs.session = new AMA.Session({
                        storage           : context.client.storage,
                        sessionId         : context.client.storage.get(key),
                        sessionLength     : context.options.sessionLength,
                        expirationCallback: function (session) {
                            var shouldExtend = context.options.expirationCallback(session);
                            if (shouldExtend === true || typeof shouldExtend === 'number') {
                                return shouldExtend;
                            }
                            context.stopSession();
                        }
                    });
                    if (new Date().getTime() > context.outputs.session.expirationDate) {
                        context.outputs.session.expireSession();
                        delete context.outputs.session;
                    }
                }
            });
        }

        checkForStoredSessions(this);
        if (!this.outputs.session) {
            this.startSession();
        }
        if (this.options.autoSubmitEvents) {
            this.client.submitEvents();
        }
    };

    /**
     * submitEvents
     * @param {Object} [options=] - options for submitting events
     * @param {Object} [options.clientContext=this.options.clientContext] - clientContext to submit with defaults to
     *                                                                      options.clientContext
     * @returns {Array} Array of batch indices that were submitted
     */
    Manager.prototype.submitEvents = function (options) {
        return this.client.submitEvents(options);
    };

    /**
     * Function to start a session
     * @returns {AMA.Client.Event} The start session event recorded
     */
    Manager.prototype.startSession = function () {
        this.client.logger.log('[Function:(AMA.Manager).startSession]');
        if (this.outputs.session) {
            //Clear Session
            this.outputs.session.clearSession();
        }
        this.outputs.session = new AMA.Session({
            storage: this.client.storage,
            logger: this.client.options.logger,
            sessionLength: this.options.sessionLength,
            expirationCallback: function (session) {
                var shouldExtend = this.options.expirationCallback(session);
                if (shouldExtend === true || typeof shouldExtend === 'number') {
                    return shouldExtend;
                }
                this.stopSession();
            }.bind(this)
        });
        return this.recordEvent('_session.start');
    };

    /**
     * Function to extend the current session.
     * @param {int} [milliseconds=options.sessionLength] - Milliseconds to extend the session by, will default
     *                                                     to another session length
     * @returns {int} The Session expiration (in Milliseconds)
     */
    Manager.prototype.extendSession = function (milliseconds) {
        return this.outputs.session.extendSession(milliseconds || this.options.sessionLength);
    };

    /**
     * Function to stop the current session
     * @returns {AMA.Client.Event} The stop session event recorded
     */
    Manager.prototype.stopSession = function () {
        this.client.logger.log('[Function:(AMA.Manager).stopSession]');
        this.outputs.session.stopSession();
        this.outputs.session.expireSession(AMA.Util.NOP);
        return this.recordEvent('_session.stop');
    };

    /**
     * Function to stop the current session and start a new one
     * @returns {AMA.Session} The new Session Object for the SessionManager
     */
    Manager.prototype.renewSession = function () {
        this.stopSession();
        this.startSession();
        return this.outputs.session;
    };

    /**
     * Function that constructs a Mobile Analytics Event
     * @param {string} eventType - Custom Event Type to be displayed in Console
     * @param {AMA.Client.Attributes} [attributes=] - Map of String attributes
     * @param {AMA.Client.Metrics} [metrics=] - Map of numeric values
     * @returns {AMA.Client.Event}
     */
    Manager.prototype.createEvent = function (eventType, attributes, metrics) {
        return this.client.createEvent(eventType, this.outputs.session, attributes, metrics);
    };

    /**
     * Function to record a custom event
     * @param eventType - Custom event type name
     * @param {AMA.Client.Attributes} [attributes=] - Custom attributes
     * @param {AMA.Client.Metrics} [metrics=] - Custom metrics
     * @returns {AMA.Client.Event} The event that was recorded
     */
    Manager.prototype.recordEvent = function (eventType, attributes, metrics) {
        return this.client.recordEvent(eventType, this.outputs.session, attributes, metrics);
    };

    /**
     * Function to record a monetization event
     * @param {Object} monetizationDetails - Details about Monetization Event
     * @param {string} monetizationDetails.currency - ISO Currency of event
     * @param {string} monetizationDetails.productId - Product Id of monetization event
     * @param {number} monetizationDetails.quantity - Quantity of product in transaction
     * @param {string|number} monetizationDetails.price - Price of product either ISO formatted string, or number
     *                                                    with associated ISO Currency
     * @param {AMA.Client.Attributes} [attributes=] - Custom attributes
     * @param {AMA.Client.Metrics} [metrics=] - Custom metrics
     * @returns {AMA.Client.Event} The event that was recorded
     */
    Manager.prototype.recordMonetizationEvent = function (monetizationDetails, attributes, metrics) {
        return this.client.recordMonetizationEvent(this.outputs.session, monetizationDetails, attributes, metrics);
    };
    return Manager;
}());

module.exports = AMA.Manager;

}).call(this,typeof global !== "undefined" ? global : typeof self !== "undefined" ? self : typeof window !== "undefined" ? window : {})
},{"./MobileAnalyticsClient.js":1,"./MobileAnalyticsSession.js":2,"./StorageClients/LocalStorage.js":5,"./StorageClients/StorageKeys.js":6}],4:[function(require,module,exports){
(function (global){
var AMA = global.AMA;

AMA.Util = (function () {
    'use strict';
    function s4() {
        return Math.floor((1 + Math.random()) * 0x10000)
            .toString(16)
            .substring(1);
    }
    function utf8ByteLength(str) {
        if (typeof str !== 'string') {
            str = JSON.stringify(str);
        }
        var s = str.length, i, code;
        for (i = str.length - 1; i >= 0; i -= 1) {
            code = str.charCodeAt(i);
            if (code > 0x7f && code <= 0x7ff) {
                s += 1;
            } else if (code > 0x7ff && code <= 0xffff) {
                s += 2;
            }
            if (code >= 0xDC00 && code <= 0xDFFF) { /*trail surrogate*/
                i -= 1;
            }
        }
        return s;
    }
    function guid() {
        return s4() + s4() + '-' + s4() + '-' + s4() + '-' + s4() + '-' + s4() + s4() + s4();
    }
    function mergeObjects(override, initial) {
        Object.keys(initial).forEach(function (key) {
            if (initial.hasOwnProperty(key)) {
                override[key] = override[key] || initial[key];
            }
        });
        return override;
    }
    function copy(original, extension) {
        return mergeObjects(JSON.parse(JSON.stringify(original)), extension || {});
    }
    function NOP() {
        return undefined;
    }

    function timestamp() {
        return new Date().getTime();
    }
    return {
        copy: copy,
        GUID: guid,
        getRequestBodySize: utf8ByteLength,
        mergeObjects: mergeObjects,
        NOP: NOP,
        timestamp: timestamp
    };
}());

module.exports = AMA.Util;

}).call(this,typeof global !== "undefined" ? global : typeof self !== "undefined" ? self : typeof window !== "undefined" ? window : {})
},{}],5:[function(require,module,exports){
(function (global){
var AMA = global.AMA;
AMA.Util = require('../MobileAnalyticsUtilities.js');

AMA.Storage = (function () {
    'use strict';
    var LocalStorageClient = function (appId) {
        this.storageKey = 'AWSMobileAnalyticsStorage-' + appId;
        global[this.storageKey] = global[this.storageKey] || {};
        this.cache = global[this.storageKey];
        this.cache.id = this.cache.id || AMA.Util.GUID();
        this.logger = {
            log: AMA.Util.NOP,
            info: AMA.Util.NOP,
            warn: AMA.Util.NOP,
            error: AMA.Util.NOP
        };
        this.reload();
    };
    // Safari, in Private Browsing Mode, looks like it supports localStorage but all calls to setItem
    // throw QuotaExceededError. We're going to detect this and just silently drop any calls to setItem
    // to avoid the entire page breaking, without having to do a check at each usage of Storage.
    /*global Storage*/
    if (typeof localStorage === 'object' && Storage === 'object') {
        try {
            localStorage.setItem('TestLocalStorage', 1);
            localStorage.removeItem('TestLocalStorage');
        } catch (e) {
            Storage.prototype._setItem = Storage.prototype.setItem;
            Storage.prototype.setItem = AMA.Util.NOP;
            console.warn('Your web browser does not support storing settings locally. In Safari, the most common cause of this is using "Private Browsing Mode". Some settings may not save or some features may not work properly for you.');
        }
    }

    LocalStorageClient.prototype.type = 'LOCAL_STORAGE';
    LocalStorageClient.prototype.get = function (key) {
        return this.cache[key];
    };
    LocalStorageClient.prototype.set = function (key, value) {
        this.cache[key] = value;
        return this.saveToLocalStorage();
    };
    LocalStorageClient.prototype.delete = function (key) {
        delete this.cache[key];
        this.saveToLocalStorage();
    };
    LocalStorageClient.prototype.each = function (callback) {
        var key;
        for (key in this.cache) {
            if (this.cache.hasOwnProperty(key)) {
                callback(key, this.cache[key]);
            }
        }
    };
    LocalStorageClient.prototype.saveToLocalStorage = function saveToLocalStorage() {
        if (this.supportsLocalStorage()) {
            try {
                this.logger.log('[Function:(AWS.MobileAnalyticsClient.Storage).saveToLocalStorage]');
                window.localStorage.setItem(this.storageKey, JSON.stringify(this.cache));
                this.logger.log('LocalStorage Cache: ' + JSON.stringify(this.cache));
            } catch (saveToLocalStorageError) {
                this.logger.log('Error saving to LocalStorage: ' + JSON.stringify(saveToLocalStorageError));
            }
        } else {
            this.logger.log('LocalStorage is not available');
        }
    };
    LocalStorageClient.prototype.reload = function loadLocalStorage() {
        if (this.supportsLocalStorage()) {
            var storedCache;
            try {
                this.logger.log('[Function:(AWS.MobileAnalyticsClient.Storage).loadLocalStorage]');
                storedCache = window.localStorage.getItem(this.storageKey);
                this.logger.log('LocalStorage Cache: ' + storedCache);
                if (storedCache) {
                    //Try to parse, if corrupt delete
                    try {
                        this.cache = JSON.parse(storedCache);
                    } catch (parseJSONError) {
                        //Corrupted stored cache, delete it
                        this.clearLocalStorage();
                    }
                }
            } catch (loadLocalStorageError) {
                this.logger.log('Error loading LocalStorage: ' + JSON.stringify(loadLocalStorageError));
                this.clearLocalStorage();
            }
        } else {
            this.logger.log('LocalStorage is not available');
        }
    };
    LocalStorageClient.prototype.setLogger = function (logFunction) {
        this.logger = logFunction;
    };
    LocalStorageClient.prototype.supportsLocalStorage = function supportsLocalStorage() {
        try {
            return window && window.localStorage;
        } catch (supportsLocalStorageError) {
            return false;
        }
    };
    LocalStorageClient.prototype.clearLocalStorage = function clearLocalStorage() {
        this.cache = {};
        if (this.supportsLocalStorage()) {
            try {
                this.logger.log('[Function:(AWS.MobileAnalyticsClient.Storage).clearLocalStorage]');
                window.localStorage.removeItem(this.storageKey);
                //Clear Cache
                global[this.storageKey] = {};
            } catch (clearLocalStorageError) {
                this.logger.log('Error clearing LocalStorage: ' + JSON.stringify(clearLocalStorageError));
            }
        } else {
            this.logger.log('LocalStorage is not available');
        }
    };
    return LocalStorageClient;
}());

module.exports = AMA.Storage;

}).call(this,typeof global !== "undefined" ? global : typeof self !== "undefined" ? self : typeof window !== "undefined" ? window : {})
},{"../MobileAnalyticsUtilities.js":4}],6:[function(require,module,exports){
(function (global){
var AMA = global.AMA;

AMA.StorageKeys = {
    'CLIENT_ID': 'AWSMobileAnalyticsClientId',
    'GLOBAL_ATTRIBUTES': 'AWSMobileAnalyticsGlobalAttributes',
    'GLOBAL_METRICS': 'AWSMobileAnalyticsGlobalMetrics',
    'SESSION_ID': 'MobileAnalyticsSessionId',
    'SESSION_EXPIRATION': 'MobileAnalyticsSessionExpiration',
    'SESSION_START_TIMESTAMP': 'MobileAnalyticsSessionStartTimeStamp'
};

module.exports = AMA.StorageKeys;

}).call(this,typeof global !== "undefined" ? global : typeof self !== "undefined" ? self : typeof window !== "undefined" ? window : {})
},{}],7:[function(require,module,exports){
(function (global){
/**
 * @module AMA
 * @description The global namespace for Amazon Mobile Analytics
 * @see AMA.Manager
 * @see AMA.Client
 * @see AMA.Session
 */
global.AMA = global.AMA || {};
require('./MobileAnalyticsClient.js');
require('./MobileAnalyticsUtilities.js');
require('./StorageClients/StorageKeys.js');
require('./StorageClients/LocalStorage.js');
require('./MobileAnalyticsSession.js');
require('./MobileAnalyticsSessionManager.js');
module.exports = global.AMA;

}).call(this,typeof global !== "undefined" ? global : typeof self !== "undefined" ? self : typeof window !== "undefined" ? window : {})
},{"./MobileAnalyticsClient.js":1,"./MobileAnalyticsSession.js":2,"./MobileAnalyticsSessionManager.js":3,"./MobileAnalyticsUtilities.js":4,"./StorageClients/LocalStorage.js":5,"./StorageClients/StorageKeys.js":6}]},{},[7]);

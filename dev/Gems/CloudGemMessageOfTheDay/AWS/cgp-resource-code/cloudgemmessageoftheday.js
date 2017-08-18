System.register(['@angular/core', 'app/view/game/module/cloudgems/class/index', 'app/shared/class/index', 'app/shared/component/index', 'app/view/game/module/shared/component/index'], function(exports_1, context_1) {
    "use strict";
    var __moduleName = context_1 && context_1.id;
    var __extends = (this && this.__extends) || function (d, b) {
        for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p];
        function __() { this.constructor = d; }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
    var __decorate = (this && this.__decorate) || function (decorators, target, key, desc) {
        var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
        if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
        else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
        return c > 3 && r && Object.defineProperty(target, key, r), r;
    };
    var __metadata = (this && this.__metadata) || function (k, v) {
        if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(k, v);
    };
    var core_1, index_1, index_2, index_3, index_4, index_5;
    var MotdMode, Motd, MotdAPI, MotdForm;
    function getInstance(context) {
        var gem = new Motd();
        gem.initialize(context);
        return gem;
    }
    exports_1("getInstance", getInstance);
    return {
        setters:[
            function (core_1_1) {
                core_1 = core_1_1;
            },
            function (index_1_1) {
                index_1 = index_1_1;
                index_2 = index_1_1;
            },
            function (index_3_1) {
                index_3 = index_3_1;
            },
            function (index_4_1) {
                index_4 = index_4_1;
            },
            function (index_5_1) {
                index_5 = index_5_1;
            }],
        execute: function() {
            (function (MotdMode) {
                MotdMode[MotdMode["List"] = 0] = "List";
                MotdMode[MotdMode["Edit"] = 1] = "Edit";
                MotdMode[MotdMode["Delete"] = 2] = "Delete";
                MotdMode[MotdMode["Add"] = 3] = "Add";
            })(MotdMode || (MotdMode = {}));
            exports_1("MotdMode", MotdMode);
            Motd = (function (_super) {
                __extends(Motd, _super);
                //Required.  Firefox will complain if the constructor is not present with no arguments
                function Motd() {
                    _super.call(this);
                    this.identifier = "motd";
                    this.subNavActiveIndex = 0;
                    // Submenu item section
                    this.subMenuItems = [
                        "Overview",
                        "History"
                    ];
                    this.tackable = {
                        displayName: "Message of the day",
                        iconSrc: "https://m.media-amazon.com/images/G/01/cloudcanvas/images/message_of_the_day._V536715120_.png",
                        cost: "Low",
                        state: new index_1.TackableStatus(),
                        metric: new index_1.TackableMeasure()
                    };
                    this.pageSize = 5;
                }
                Motd.prototype.classType = function () {
                    var clone = Motd;
                    return clone;
                };
                Motd.prototype.initialize = function (context) {
                    _super.prototype.initialize.call(this, context);
                    this.apiHandler = new MotdAPI(this.context.meta.cloudFormationOutputMap["ServiceUrl"].outputValue, this.context.provider.http, this.context.provider.aws);
                    this.apiHandler.report(this.tackable.metric);
                    this.apiHandler.assign(this.tackable.state);
                };
                Object.defineProperty(Motd.prototype, "htmlTemplateUrl", {
                    get: function () {
                        return this.context.provider.definition.isProd ? this.getSignedUrl(this.context.meta.key + "/cloudgemmessageoftheday.html") : "external/cloudgemmessageoftheday/cloudgemmessageoftheday.html";
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(Motd.prototype, "styleTemplateUrl", {
                    get: function () {
                        return this.context.provider.definition.isProd ? this.getSignedUrl(this.context.meta.key + "/cloudgemmessageoftheday.css") : "external/cloudgemmessageoftheday/cloudgemmessageoftheday.css";
                    },
                    enumerable: true,
                    configurable: true
                });
                Motd.prototype.ngOnInit = function () {
                    // bind proper scopes
                    this.updateMessages = this.updateMessages.bind(this);
                    this.updateActiveMessages = this.updateActiveMessages.bind(this);
                    this.updatePlannedMessages = this.updatePlannedMessages.bind(this);
                    this.updateExpiredMessages = this.updateExpiredMessages.bind(this);
                    this.addModal = this.addModal.bind(this);
                    this.editModal = this.editModal.bind(this);
                    this.deleteModal = this.deleteModal.bind(this);
                    this.dismissModal = this.dismissModal.bind(this);
                    this.validate = this.validate.bind(this);
                    this.validateDate = this.validateDate.bind(this);
                    this.delete = this.delete.bind(this);
                    this.onSubmit = this.onSubmit.bind(this);
                    // end of bind scopes
                    this.motdModes = MotdMode;
                    this.updateMessages();
                    this.currentMessage = this.default();
                };
                Motd.prototype.validate = function (model) {
                    var isValid = true;
                    var maxChar = 700;
                    model.priority.valid = Number(model.priority.value) >= 0;
                    isValid = isValid && model.priority.valid;
                    model.message.valid = false;
                    if (!model.message.value || !/\S/.test(model.message.value)) {
                        model.message.message = "The message was invalid";
                    }
                    else if (model.message.value.length < 1) {
                        model.message.message = "The message was not long enough.";
                    }
                    else if (model.message.value.length > 700) {
                        model.message.message = "The message was too long. We counted " + model.message.value.length + " characters.  The limit is " + maxChar + " characters.";
                    }
                    else {
                        model.message.valid = true;
                    }
                    isValid = isValid && model.message.valid;
                    isValid = isValid && this.validateDate(model);
                    return isValid;
                };
                Motd.prototype.validateDate = function (model) {
                    var isValid = true;
                    var start = index_3.DateTimeUtil.toObjDate(model.start);
                    var end = index_3.DateTimeUtil.toObjDate(model.end);
                    if (this.currentMessage.hasEnd) {
                        model.end.valid = model.end.date && model.end.date.year && model.end.date.month && model.end.date.day ? true : false;
                        if (!model.end.valid)
                            model.date = { message: "The end date is not a valid date." };
                        isValid = isValid && model.end.valid;
                    }
                    if (this.currentMessage.hasStart) {
                        model.start.valid = model.start.date.year && model.start.date.month && model.start.date.day ? true : false;
                        if (!model.start.valid)
                            model.date = { message: "The end date is not greater than today." };
                        isValid = isValid && model.start.valid;
                    }
                    if (this.currentMessage.hasEnd && this.currentMessage.hasStart) {
                        var isValidDateRange = (start < end);
                        isValid = isValid && isValidDateRange;
                        if (!isValidDateRange) {
                            model.date = { message: "The start date must be less than the end date." };
                            model.start.valid = false;
                        }
                    }
                    return isValid;
                };
                Motd.prototype.onSubmit = function (model) {
                    var _this = this;
                    if (this.validate(model)) {
                        var body_1 = {
                            message: model.message.value,
                            priority: model.priority.value
                        };
                        var start = null, end = null;
                        if (model.hasStart && model.start.date && model.start.date.year) {
                            start = index_3.DateTimeUtil.toDate(model.start);
                            body_1['startTime'] = start;
                        }
                        if (model.hasEnd && model.end.valid) {
                            end = index_3.DateTimeUtil.toDate(model.end);
                            body_1['endTime'] = end;
                        }
                        toastr.info("Saving message.");
                        this.modalRef.close();
                        if (model.UniqueMsgID) {
                            this.apiHandler.put(model.UniqueMsgID, body_1).subscribe(function () {
                                toastr.clear();
                                toastr.success("The message '" + body_1.message + "' has been updated.");
                                _this.updateMessages();
                            }, function (err) {
                                toastr.clear();
                                toastr.error("The message did not update correctly. " + err.message);
                            });
                        }
                        else {
                            this.apiHandler.post(body_1).subscribe(function () {
                                toastr.clear();
                                toastr.success("The message '" + body_1.message + "' has been created.");
                                _this.updateMessages();
                            }, function (err) {
                                toastr.clear();
                                toastr.error("The message did not update correctly. " + err.message);
                            });
                        }
                        this.mode = MotdMode.List;
                    }
                };
                // TODO: These section are pretty similar and should be extracted into a seperate component
                Motd.prototype.updateActiveMessages = function (currentPageIndex) {
                    var _this = this;
                    // Get total messages for pagination
                    this.apiHandler.getMessages("active", 0, 1000).subscribe(function (response) {
                        var obj = JSON.parse(response.body.text());
                        var numberActiveMessages = obj.result.list.length;
                        _this.activeMessagePages = Math.ceil(numberActiveMessages / _this.pageSize);
                    });
                    this.isLoadingActiveMessages = true;
                    this.apiHandler.getMessages("active", (currentPageIndex - 1) * this.pageSize, this.pageSize).subscribe(function (response) {
                        var obj = JSON.parse(response.body.text());
                        _this.activeMessages = obj.result.list.sort(function (msg1, msg2) {
                            if (msg1.priority > msg2.Priority)
                                return 1;
                            if (msg1.priority < msg2.priority)
                                return -1;
                            return 0;
                        });
                        _this.isLoadingActiveMessages = false;
                    }, function (err) {
                        toastr.error("The active messages of the day did not refresh properly. " + err);
                        _this.isLoadingActiveMessages = false;
                    });
                };
                Motd.prototype.updatePlannedMessages = function (currentPageIndex) {
                    var _this = this;
                    // Get total messages for pagination
                    this.apiHandler.getMessages("planned", 0, 1000).subscribe(function (response) {
                        var obj = JSON.parse(response.body.text());
                        var numberPlannedMessages = obj.result.list.length;
                        _this.plannedMessagePages = Math.ceil(numberPlannedMessages / _this.pageSize);
                    });
                    this.isLoadingPlannedMessages = true;
                    this.apiHandler.getMessages("planned", (currentPageIndex - 1) * this.pageSize, this.pageSize).subscribe(function (response) {
                        var obj = JSON.parse(response.body.text());
                        _this.plannedMessages = obj.result.list;
                        _this.isLoadingPlannedMessages = false;
                    }, function (err) {
                        toastr.error("The planned messages of the day did not refresh properly. " + err);
                        _this.isLoadingPlannedMessages = false;
                    });
                };
                Motd.prototype.updateExpiredMessages = function (currentPageIndex) {
                    var _this = this;
                    // Get total messages for pagination
                    var pageSize = this.pageSize * 2;
                    this.apiHandler.getMessages("expired", 0, 1000).subscribe(function (response) {
                        var obj = JSON.parse(response.body.text());
                        var numberExpiredPages = obj.result.list.length;
                        _this.expiredMessagePages = Math.ceil(numberExpiredPages / pageSize);
                    });
                    this.isLoadingExpiredMessages = true;
                    this.apiHandler.getMessages("expired", (currentPageIndex - 1) * pageSize, pageSize).subscribe(function (response) {
                        var obj = JSON.parse(response.body.text());
                        _this.expiredMessages = obj.result.list;
                        _this.isLoadingExpiredMessages = false;
                    }, function (err) {
                        toastr.error("The expired messages of the day did not refresh properly. " + err);
                        _this.isLoadingExpiredMessages = false;
                    });
                };
                Motd.prototype.updateMessages = function () {
                    if (this.subNavActiveIndex == 0) {
                        this.updateActiveMessages(1);
                        this.updatePlannedMessages(1);
                    }
                    else if (this.subNavActiveIndex == 1) {
                        this.updateExpiredMessages(1);
                    }
                };
                /* Modal functions for MoTD*/
                Motd.prototype.addModal = function () {
                    this.currentMessage = this.default();
                    this.mode = MotdMode.Add;
                };
                Motd.prototype.editModal = function (model) {
                    var startDate = null;
                    var endDate = null;
                    var hasStart = true;
                    var hasEnd = true;
                    // If the message already has a start date, use that.  Otherwise we should indicate there isn't one by setting the hasStart flag here.  Same case for hasEnd
                    if (model.startTime) {
                        startDate = new Date(model.startTime);
                    }
                    else {
                        startDate = new Date();
                        hasStart = false;
                    }
                    if (model.endTime) {
                        endDate = new Date(model.endTime);
                    }
                    else {
                        endDate = new Date();
                        hasEnd = false;
                    }
                    var editModel = new MotdForm({
                        UniqueMsgID: model.UniqueMsgID,
                        start: {
                            date: {
                                year: startDate.getFullYear(),
                                month: startDate.getMonth() + 1,
                                day: startDate.getDate(),
                            },
                            time: (hasStart) ? { hour: startDate.getHours(), minute: startDate.getMinutes() } : { hour: 12, minute: 0 },
                            valid: true,
                            isScheduled: startDate.getFullYear() > 0 ? true : false
                        },
                        end: {
                            date: {
                                year: endDate.getFullYear(),
                                month: endDate.getMonth() + 1,
                                day: endDate.getDate(),
                            },
                            time: (hasEnd) ? { hour: endDate.getHours(), minute: endDate.getMinutes() } : { hour: 12, minute: 0 },
                            valid: true,
                            isScheduled: startDate.getFullYear() > 0 ? true : false
                        },
                        priority: {
                            valid: true,
                            value: model.priority
                        },
                        message: {
                            valid: true,
                            value: model.message
                        },
                        hasStart: hasStart,
                        hasEnd: hasEnd
                    });
                    this.currentMessage = editModel;
                    this.mode = MotdMode.Edit;
                };
                Motd.prototype.deleteModal = function (model) {
                    model.end = {};
                    model.start = {};
                    this.mode = MotdMode.Delete;
                    this.currentMessage = model;
                };
                Motd.prototype.dismissModal = function () {
                    this.mode = MotdMode.List;
                };
                Motd.prototype.getSubNavItem = function (subNavItemIndex) {
                    this.subNavActiveIndex = subNavItemIndex;
                    this.updateMessages();
                };
                /* form function */
                Motd.prototype.delete = function () {
                    var _this = this;
                    this.modalRef.close();
                    this.mode = MotdMode.List;
                    toastr.info("Deleting");
                    this.apiHandler.delete(this.currentMessage.UniqueMsgID).subscribe(function (response) {
                        toastr.clear();
                        toastr.success("The message '" + _this.currentMessage.message + "'");
                        _this.paginationRef.reset();
                        _this.updateMessages();
                    }, function (err) {
                        toastr.error("The message '" + _this.currentMessage.message + "' did not delete. " + err);
                    });
                };
                Motd.prototype.dprModelUpdated = function (model) {
                    this.currentMessage.hasStart = model.hasStart;
                    this.currentMessage.hasEnd = model.hasEnd;
                    this.currentMessage.start = model.start;
                    this.currentMessage.start.valid = true;
                    this.currentMessage.end = model.end;
                    this.currentMessage.end.valid = true;
                };
                Motd.prototype.default = function () {
                    var start = this.getDefaultStartDateTime();
                    var end = this.getDefaultEndDateTime();
                    return new MotdForm({
                        start: start,
                        end: end,
                        priority: {
                            valid: true,
                            value: 0
                        },
                        message: {
                            valid: true
                        },
                        hasStart: false,
                        hasEnd: false
                    });
                };
                Motd.prototype.getDefaultStartDateTime = function () {
                    var today = new Date();
                    return {
                        date: {
                            year: today.getFullYear(),
                            month: today.getMonth() + 1,
                            day: today.getDate()
                        },
                        time: { hour: 12, minute: 0 },
                        valid: true
                    };
                };
                ;
                Motd.prototype.getDefaultEndDateTime = function () {
                    var today = new Date();
                    return {
                        date: {
                            year: today.getFullYear(),
                            month: today.getMonth() + 1,
                            day: today.getDate() + 1
                        },
                        time: { hour: 12, minute: 0 },
                        valid: true
                    };
                };
                Motd.prototype.substring = function (string, length) {
                    return index_3.StringUtil.toEtcetera(string, length);
                };
                __decorate([
                    core_1.ViewChild(index_4.ModalComponent), 
                    __metadata('design:type', index_4.ModalComponent)
                ], Motd.prototype, "modalRef", void 0);
                __decorate([
                    core_1.ViewChild(index_5.PaginationComponent), 
                    __metadata('design:type', index_5.PaginationComponent)
                ], Motd.prototype, "paginationRef", void 0);
                return Motd;
            }(index_2.DynamicGem));
            exports_1("Motd", Motd);
            MotdAPI = (function (_super) {
                __extends(MotdAPI, _super);
                function MotdAPI(serviceBaseURL, http, aws) {
                    _super.call(this, serviceBaseURL, http, aws);
                }
                MotdAPI.prototype.report = function (metric) {
                    metric.name = "Total Active Messages";
                    metric.value = "Loading...";
                    _super.prototype.get.call(this, "admin/messages?count=500&filter=active&index=0").subscribe(function (response) {
                        var obj = JSON.parse(response.body.text());
                        metric.value = obj.result.list.length;
                    });
                };
                MotdAPI.prototype.assign = function (tackableStatus) {
                    tackableStatus.label = "Loading";
                    tackableStatus.styleType = "Loading";
                    _super.prototype.get.call(this, "service/status").subscribe(function (response) {
                        var obj = JSON.parse(response.body.text());
                        tackableStatus.label = obj.result.status == "online" ? "Online" : "Offline";
                        tackableStatus.styleType = obj.result.status == "online" ? "Enabled" : "Offline";
                    }, function (err) {
                        tackableStatus.label = "Offline";
                        tackableStatus.styleType = "Offline";
                    });
                };
                MotdAPI.prototype.delete = function (id) {
                    return _super.prototype.delete.call(this, "admin/messages/" + id);
                };
                MotdAPI.prototype.put = function (id, body) {
                    return _super.prototype.put.call(this, "admin/messages/" + id, body);
                };
                MotdAPI.prototype.post = function (body) {
                    return _super.prototype.post.call(this, "admin/messages/", body);
                };
                MotdAPI.prototype.getMessages = function (filter, startIndex, count) {
                    return _super.prototype.get.call(this, "admin/messages?count=" + count + "&filter=" +
                        filter + "&index=" + startIndex);
                };
                return MotdAPI;
            }(index_3.ApiHandler));
            exports_1("MotdAPI", MotdAPI);
            MotdForm = (function () {
                function MotdForm(motdInfo) {
                    this.UniqueMsgID = motdInfo.UniqueMsgID;
                    this.start = motdInfo.start;
                    this.end = motdInfo.end;
                    this.message = motdInfo.message;
                    this.priority = motdInfo.priority;
                    this.hasStart = motdInfo.hasStart;
                    this.hasEnd = motdInfo.hasEnd;
                }
                MotdForm.prototype.clearStartDate = function () {
                    this.start = {
                        date: {}
                    };
                };
                return MotdForm;
            }());
        }
    }
});

System.register(['app/view/game/module/cloudgems/class/index', '@angular/core', 'app/shared/class/index', 'app/shared/component/index'], function(exports_1, context_1) {
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
    var index_1, core_1, index_2, index_3, index_4;
    var DynamicContentMode, DynamicContentStage, DynamicContent, DynamicContentAPI;
    function getInstance(context) {
        var gem = new DynamicContent();
        gem.initialize(context);
        return gem;
    }
    exports_1("getInstance", getInstance);
    return {
        setters:[
            function (index_1_1) {
                index_1 = index_1_1;
                index_2 = index_1_1;
            },
            function (core_1_1) {
                core_1 = core_1_1;
            },
            function (index_3_1) {
                index_3 = index_3_1;
            },
            function (index_4_1) {
                index_4 = index_4_1;
            }],
        execute: function() {
            (function (DynamicContentMode) {
                DynamicContentMode[DynamicContentMode["View"] = 0] = "View";
                DynamicContentMode[DynamicContentMode["Edit"] = 1] = "Edit";
                DynamicContentMode[DynamicContentMode["Delete"] = 2] = "Delete";
            })(DynamicContentMode || (DynamicContentMode = {}));
            exports_1("DynamicContentMode", DynamicContentMode);
            (function (DynamicContentStage) {
                DynamicContentStage[DynamicContentStage["Private"] = 0] = "Private";
                DynamicContentStage[DynamicContentStage["Scheduled"] = 1] = "Scheduled";
                DynamicContentStage[DynamicContentStage["Public"] = 2] = "Public";
            })(DynamicContentStage || (DynamicContentStage = {}));
            exports_1("DynamicContentStage", DynamicContentStage);
            DynamicContent = (function (_super) {
                __extends(DynamicContent, _super);
                function DynamicContent() {
                    _super.call(this);
                    this.identifier = "dynamiccontent";
                    // Submenu item section
                    this.subMenuItems = [
                        "Overview"
                    ];
                    this.tackable = {
                        displayName: "Dynamic Content",
                        iconSrc: "https://m.media-amazon.com/images/G/01/cloudcanvas/images/dynamic_content_icon._V536715120_.png",
                        cost: "High",
                        state: new index_1.TackableStatus(),
                        metric: new index_1.TackableMeasure()
                    };
                }
                DynamicContent.prototype.classType = function () {
                    var clone = DynamicContent;
                    return clone;
                };
                Object.defineProperty(DynamicContent.prototype, "htmlTemplateUrl", {
                    get: function () {
                        return this.context.provider.definition.isProd ? this.getSignedUrl(this.context.meta.key + "/cloudgemdynamiccontent.html") : "external/cloudgemdynamiccontent/cloudgemdynamiccontent.html";
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(DynamicContent.prototype, "styleTemplateUrl", {
                    get: function () {
                        return this.context.provider.definition.isProd ? this.getSignedUrl(this.context.meta.key + "/cloudgemdynamiccontent.css") : "external/cloudgemdynamiccontent/cloudgemdynamiccontent.css";
                    },
                    enumerable: true,
                    configurable: true
                });
                DynamicContent.prototype.initialize = function (context) {
                    _super.prototype.initialize.call(this, context);
                    this.apiHandler = new DynamicContentAPI(this.context.meta.cloudFormationOutputMap["ServiceUrl"].outputValue, this.context.provider.http, this.context.provider.aws);
                    this.apiHandler.report(this.tackable.metric);
                    this.apiHandler.assign(this.tackable.state);
                };
                DynamicContent.prototype.ngOnInit = function () {
                    this.bucket = "swimlane";
                    this.bucketHeight = "700";
                    this.mode = DynamicContentMode.View;
                    this.stages = [{ displayName: "Private", state: "PRIVATE", value: DynamicContentStage.Private, tree: [] },
                        { displayName: "Scheduled", state: "WINDOW", value: DynamicContentStage.Scheduled, tree: [] },
                        { displayName: "Public", state: "PUBLIC", value: DynamicContentStage.Public, tree: [] }];
                    this.publicStage = this.stages[2];
                    this.transitionStages = [{ name: this.stages[DynamicContentStage.Private].displayName, val: this.stages[DynamicContentStage.Private].value },
                        { name: this.stages[DynamicContentStage.Scheduled].displayName, val: this.stages[DynamicContentStage.Scheduled].value },
                        { name: this.stages[DynamicContentStage.Public].displayName, val: this.stages[DynamicContentStage.Public].value }];
                    this.currentDynamicContent = {};
                    this.bShowDragTutorial = true;
                    //bind the proper scopes
                    this.onClick = this.onClick.bind(this);
                    this.onDrop = this.onDrop.bind(this);
                    this.onEdit = this.onEdit.bind(this);
                    this.onSubmit = this.onSubmit.bind(this);
                    this.onDelete = this.onDelete.bind(this);
                    this.onDeleteSubmit = this.onDeleteSubmit.bind(this);
                    this.validate = this.validate.bind(this);
                    this.deleteContent = this.deleteContent.bind(this);
                    this.setViewMode = this.setViewMode.bind(this);
                    this.updateView = this.updateView.bind(this);
                    this.postContent = this.postContent.bind(this);
                    this.getContent();
                };
                DynamicContent.prototype.onEdit = function (model) {
                    var currentStage = this.findStage(model);
                    var targetStage = model.target !== undefined && model.target >= 0 ? model.target : currentStage.value.toString();
                    this.currentDynamicContent = model;
                    this.currentDynamicContent.stage = targetStage.toString();
                    var startDate = model.StagingStart ? new Date(model.StagingStart) : null;
                    var endDate = model.StagingEnd ? new Date(model.StagingEnd) : null;
                    this.currentDynamicContent.start = index_4.DateTimeRangePickerComponent.default(startDate);
                    this.currentDynamicContent.end = index_4.DateTimeRangePickerComponent.default(endDate);
                    this.currentDynamicContent.start.isScheduled = index_3.DateTimeUtil.isValid(startDate);
                    this.currentDynamicContent.end.isScheduled = index_3.DateTimeUtil.isValid(endDate);
                    this.currentDynamicContent.validation = {
                        date: {
                            isValid: true,
                            message: ""
                        }
                    };
                    for (var i = 0; i < this.stages.length; i++) {
                        var stage = this.stages[i];
                        if (Number(this.currentDynamicContent.stage) === stage.value) {
                            this.currentDynamicContent.stage = stage.displayName;
                            break;
                        }
                    }
                    this.currentStageDisplayName = currentStage.displayName;
                    this.mode = DynamicContentMode.Edit;
                };
                DynamicContent.prototype.updateView = function () {
                    this.setViewMode();
                    this.getContent();
                };
                DynamicContent.prototype.dprModelUpdated = function (model) {
                    var _this = this;
                    // trigger new round of change detection
                    setTimeout(function () {
                        _this.currentDynamicContent.start = model.start;
                        _this.currentDynamicContent.end = model.end;
                        _this.currentDynamicContent.start.valid = true;
                        _this.currentDynamicContent.end.valid = true;
                        _this.currentDynamicContent.end.valid = true;
                        _this.currentDynamicContent.start.isScheduled = model.hasStart;
                        _this.currentDynamicContent.end.isScheduled = model.hasEnd;
                        _this.currentDynamicContent.hasRequiredStartDateTime = (_this.currentDynamicContent.stage === 'Scheduled' && !model.hasEnd) ? true : false;
                        _this.currentDynamicContent.hasRequiredEndDateTime = (_this.currentDynamicContent.stage === 'Scheduled' && !model.hasStart) ? true : false;
                    });
                };
                DynamicContent.prototype.validate = function (model) {
                    var isValid = true;
                    var stageIdx = Number(this.currentDynamicContent.stage);
                    isValid = isValid && stageIdx >= 0;
                    if (DynamicContentStage.Public == stageIdx) {
                        this.currentDynamicContent.end.isScheduled = false;
                        this.currentDynamicContent.start.isScheduled = false;
                    }
                    else {
                        if (this.currentDynamicContent.start.isScheduled) {
                            isValid = isValid && this.validateDate(model.start);
                        }
                        if (this.currentDynamicContent.end.isScheduled) {
                            isValid = isValid && this.validateDate(model.end);
                        }
                        if (this.currentDynamicContent.start.isScheduled && this.currentDynamicContent.end.isScheduled) {
                            var start = index_3.DateTimeUtil.toObjDate(model.start);
                            var end = index_3.DateTimeUtil.toObjDate(model.end);
                            var isValidDateRange = (start < end);
                            isValid = isValid && isValidDateRange;
                            if (!isValidDateRange) {
                                this.currentDynamicContent.validation.date = { message: "The start date must be less than the end date." };
                                this.currentDynamicContent.validation.date.isValid = false;
                            }
                        }
                    }
                    return isValid;
                };
                DynamicContent.prototype.validateDate = function (model) {
                    return model && model.date && model.date.year && model.date.month && model.date.day ? true : false;
                };
                DynamicContent.prototype.onSubmit = function (model) {
                    var submitStage = "PRIVATE";
                    for (var _i = 0, _a = this.stages; _i < _a.length; _i++) {
                        var stage = _a[_i];
                        if (stage.displayName === this.currentDynamicContent.stage) {
                            submitStage = stage.state;
                        }
                    }
                    status = submitStage;
                    if (!status) {
                        toastr.error("An error was encountered with pak: " + JSON.stringify(this.currentDynamicContent), "Error generating 'pak' hierarchy");
                        return;
                    }
                    var obj = {
                        FileName: model.FileName,
                        StagingStatus: status
                    };
                    var start = null, end = null;
                    if (status !== this.publicStage.state) {
                        if (this.currentDynamicContent.start.isScheduled) {
                            start = index_3.DateTimeUtil.toDate(this.currentDynamicContent.start);
                        }
                        if (this.currentDynamicContent.end.isScheduled) {
                            end = index_3.DateTimeUtil.toDate(this.currentDynamicContent.end);
                        }
                    }
                    //if (start !== null)
                    obj['StagingStart'] = start;
                    //if (end !== null)
                    obj['StagingEnd'] = end;
                    var paks = [];
                    if (model.isMissing === undefined)
                        paks.push(obj);
                    this.getChildrenToMigrate(paks, model.children, start, end, status);
                    var payload = {
                        FileList: paks
                    };
                    toastr.info("Saving changes.");
                    this.postContent(payload);
                    this.setViewMode();
                };
                DynamicContent.prototype.getChildrenToMigrate = function (paks, branch, start, end, status) {
                    for (var i = 0; i < branch.length; i++) {
                        var decendant = branch[i];
                        if (decendant.checked && decendant.isMissing === undefined) {
                            var obj = {
                                FileName: decendant.FileName,
                                StagingStatus: status
                            };
                            //if (start !== null)
                            obj['StagingStart'] = start;
                            //if (end !== null)
                            obj['StagingEnd'] = end;
                            paks.push(obj);
                        }
                        this.getChildrenToMigrate(paks, decendant.children, start, end, status);
                    }
                };
                DynamicContent.prototype.clear = function () {
                    for (var i = 0; i < this.stages.length; i++) {
                        var stage = this.stages[i];
                        stage.tree = [];
                    }
                };
                DynamicContent.prototype.onDelete = function (model) {
                    this.currentDynamicContent = model;
                    this.mode = DynamicContentMode.Delete;
                };
                DynamicContent.prototype.onDeleteSubmit = function (model) {
                    var obj = {
                        FileName: model.FileName,
                    };
                    var paks = [];
                    paks.push(obj);
                    this.getChildrenToDelete(paks, model.children);
                    toastr.info("Saving changes.");
                    // Issue a DELETE request for each file to be deleted in the hierarchy
                    var deletionTracker = {
                        count: 0,
                        length: paks.length
                    };
                    for (var _i = 0, paks_1 = paks; _i < paks_1.length; _i++) {
                        var pak = paks_1[_i];
                        this.deleteContent(pak.FileName, deletionTracker);
                    }
                    this.setViewMode();
                };
                DynamicContent.prototype.getChildrenToDelete = function (paks, branch) {
                    for (var i = 0; i < branch.length; i++) {
                        var decendant = branch[i];
                        var obj = {
                            FileName: decendant.FileName,
                        };
                        paks.push(obj);
                        this.getChildrenToDelete(paks, decendant.children);
                    }
                };
                DynamicContent.prototype.onDrop = function (element, target, source) {
                    //find the element that we need to trigger the edit action on.
                    var modelId = element.firstChild.id;
                    var targetContainerId = target.id;
                    var sourceContainerId = source.id;
                    var targetContainer, sourceContainer;
                    //locate the source/target containers
                    for (var i = 0; i < this.stages.length; i++) {
                        var container = this.stages[i];
                        if (targetContainerId == container.state) {
                            targetContainer = container;
                        }
                        if (sourceContainerId == container.state) {
                            sourceContainer = container;
                        }
                    }
                    var model = undefined;
                    //find the model in the source container
                    for (var i = 0; i < sourceContainer.tree.length; i++) {
                        if (modelId == sourceContainer.tree[i].FileName) {
                            model = sourceContainer.tree[i];
                            break;
                        }
                    }
                    model.target = targetContainer.value;
                    this.bShowDragTutorial = false;
                    this.onEdit(model);
                };
                DynamicContent.prototype.onClick = function (model) {
                    this.currentDynamicContent = model;
                };
                DynamicContent.prototype.remove = function (arr, value) {
                    var index = arr.indexOf(value);
                    if (index !== -1) {
                        arr.splice(index, 1);
                    }
                };
                DynamicContent.prototype.setViewMode = function () {
                    this.mode = DynamicContentMode.View;
                };
                DynamicContent.prototype.clearScheduledDates = function () {
                    this.currentDynamicContent.start = {};
                    this.currentDynamicContent.end = {};
                };
                DynamicContent.prototype.hierarchize = function (tree) {
                    //ground the roots and identify the decendants
                    var decendants = [];
                    for (var i = 0; i < tree.length; i++) {
                        var decendant = tree[i];
                        decendant.children = [];
                        if (decendant.StagingStatus != this.publicStage.state) {
                            if (Date.parse(decendant.StagingStart) > 0 && Date.parse(decendant.StagingEnd) > 0) {
                                decendant.scheduledDateTime = decendant.StagingStart + " to " + decendant.StagingEnd;
                            }
                            else if (Date.parse(decendant.StagingStart) > 0) {
                                decendant.scheduledDateTime = decendant.StagingStart + " - No End";
                            }
                            else if (Date.parse(decendant.StagingEnd) > 0) {
                                decendant.scheduledDateTime = "No Start - " + decendant.StagingEnd;
                            }
                        }
                        if (!decendant.Parent) {
                            this.ground(decendant);
                        }
                        else {
                            decendants.push(decendant);
                        }
                    }
                    //branch the decendants, must be a two step as some roots may not be grounded before you start branching
                    //iterate the branches until all have found parents or X iterations
                    var maxiterations = (tree.length * tree.length) + 1;
                    var iteration = 1;
                    while (decendants.length > 0 && iteration < maxiterations) {
                        var decendant = decendants.splice(0, 1)[0];
                        decendant.checked = true;
                        //assuming the decendant and parent have the same StagingStatus
                        var branch = this.findTree(decendant);
                        var parent_1 = this.find(branch, decendant.Parent);
                        if (parent_1) {
                            //parent found.
                            parent_1.push(decendant);
                        }
                        else {
                            //no parents found.  Push him to the back of the array to check later.
                            decendants.push(decendant);
                        }
                        iteration++;
                    }
                    //zombies can occur when you don't move a full hierarchy to another state.
                    //place all zombie decendants at the root of their tree
                    for (var i = 0; i < decendants.length; i++) {
                        var decendant = decendants[i];
                        var branch = this.findTree(decendant);
                        var parent_2 = this.findChildById(decendant.Parent);
                        var parentInBranch = this.findChildObjectById(branch, decendant.Parent);
                        if (parentInBranch) {
                            parent_2.statusClassNames = null;
                            parent_2 = parentInBranch;
                        }
                        else if (parent_2) {
                            parent_2.isMissingParent = undefined;
                            parent_2.subText = "This is the parent package for '" + decendant.FileName + "'.";
                            parent_2.statusClassNames = "fa fa-warning warning-overlay-color";
                        }
                        if (parent_2 !== null && parent_2 !== undefined) {
                            if (parent_2.StagingStatus != decendant.StagingStatus && parent_2.isMissingParent === undefined) {
                                var duplicatedMissingParent = this.findChildObjectById(this.findTree(decendant), parent_2.FileName);
                                var missingParent = Object.assign({}, duplicatedMissingParent !== null ? duplicatedMissingParent : parent_2);
                                parent_2.className = "located-parent";
                                missingParent.children = [];
                                missingParent.isMissing = true;
                                missingParent.isMissingParent = true;
                                missingParent.secondaryText = "(Missing)";
                                missingParent.className = "missing";
                                missingParent.subText = "Fix: Drag '" + parent_2.FileName + "' to this pak or this pak to the '" + parent_2.FileName + "'.";
                                missingParent.children.push(decendant);
                                branch.push(missingParent);
                            }
                            else {
                                parent_2.children.push(decendant);
                            }
                            decendant.secondaryText = "(Hidden)";
                            decendant.subText = "Hidden due to missing '" + parent_2.FileName + "' in this swimlane.";
                            decendant.className = "missing-decendant";
                            parent_2.isMissingParent = true;
                        }
                        else {
                            branch.push(decendant);
                        }
                    }
                };
                DynamicContent.prototype.ground = function (root) {
                    if (!root.StagingStatus) {
                        toastr.error("We did not find a 'StagingStatus' attribute on pak '" + root.FileName + "'", "Error generating 'pak' hierarchy");
                        return;
                    }
                    var status = root.StagingStatus.toUpperCase();
                    var bFound = false;
                    for (var i = 0; i < this.stages.length; i++) {
                        var stage = this.stages[i];
                        if (status == stage.state) {
                            stage.tree.push(root);
                            bFound = true;
                            break;
                        }
                    }
                    if (!bFound)
                        toastr.error("We did not find a known 'StagingStatus' attribute on pak '" + root.FileName + "'", "Error generating 'pak' hierarchy");
                };
                DynamicContent.prototype.find = function (branch, id) {
                    var result = null;
                    for (var i = 0; i < branch.length; i++) {
                        var decendant = branch[i];
                        if (decendant.FileName == id) {
                            if (!decendant.children)
                                decendant.children = [];
                            result = decendant.children;
                            break;
                        }
                        result = this.find(decendant.children, id);
                        if (result)
                            break;
                    }
                    return result;
                };
                DynamicContent.prototype.findChildById = function (id) {
                    for (var i = 0; i < this.stages.length; i++) {
                        var branch = this.stages[i];
                        for (var x = 0; x < branch.tree.length; x++) {
                            var decendant = branch.tree[x];
                            if (decendant.FileName == id) {
                                return decendant;
                            }
                            var obj = this.findChildObjectById(decendant.children, id);
                            if (obj !== null)
                                return obj;
                        }
                    }
                    return null;
                };
                DynamicContent.prototype.findChildObjectById = function (branch, id) {
                    var result = null;
                    for (var i = 0; i < branch.length; i++) {
                        var decendant = branch[i];
                        if (decendant.FileName == id) {
                            result = decendant;
                            break;
                        }
                        result = this.findChildObjectById(decendant.children, id);
                        if (result)
                            break;
                    }
                    return result;
                };
                DynamicContent.prototype.findTree = function (entry) {
                    var branch = [];
                    var status = entry.StagingStatus.toUpperCase();
                    for (var i = 0; i < this.stages.length; i++) {
                        var stage = this.stages[i];
                        if (status == stage.state) {
                            branch = stage.tree;
                            break;
                        }
                    }
                    return branch;
                };
                DynamicContent.prototype.findStage = function (entry) {
                    var branch = null;
                    var status = entry.StagingStatus.toUpperCase();
                    for (var i = 0; i < this.stages.length; i++) {
                        var stage = this.stages[i];
                        if (status == stage.state) {
                            return stage;
                        }
                    }
                    return null;
                };
                //REST commands
                DynamicContent.prototype.postContent = function (model) {
                    var _this = this;
                    this.modalRef.close(true);
                    this.apiHandler.post(model).subscribe(function (response) {
                        toastr.clear();
                        toastr.success("The package " + (model.FileList.length > 0 ? "'" + model.FileList[0].FileName + "'" : "") + " was successfully updated.");
                        _this.updateView();
                    }, function (err) {
                        toastr.clear();
                        toastr.error("The update of the dynamic content package " + (model.FileList.length > 0 ? "'" + model.FileList[0].FileName + "'" : "") + " failed: " + err.message);
                    });
                };
                DynamicContent.prototype.getContent = function () {
                    var _this = this;
                    this.clear();
                    this.isLoading = true;
                    this.hasNoContent = false;
                    this.apiHandler.get().subscribe(function (response) {
                        var obj = JSON.parse(response.body.text());
                        var content = obj.result.PortalFileInfoList;
                        if (content.length == 0) {
                            _this.hasNoContent = true;
                        }
                        //create the hierarchy
                        _this.hierarchize(content);
                        toastr.clear();
                        _this.isLoading = false;
                    }, function (err) {
                        toastr.clear();
                        toastr.error("The listing of dynamic content packages failed. " + err.message);
                    });
                };
                DynamicContent.prototype.isDeletionComplete = function (deletionTracker) {
                    if (deletionTracker === null) {
                        return true;
                    }
                    else {
                        deletionTracker.count++;
                        if (deletionTracker.count == (deletionTracker.length)) {
                            return true;
                        }
                    }
                    return false;
                };
                DynamicContent.prototype.deleteContent = function (model, deletionTracker) {
                    var _this = this;
                    if (deletionTracker === void 0) { deletionTracker = null; }
                    this.isLoading = true;
                    this.modalRef.close(true);
                    this.apiHandler.delete(model).subscribe(function (response) {
                        toastr.success("The package '" + model + "' was successfully deleted.");
                        _this.getContent();
                        if (_this.isDeletionComplete(deletionTracker)) {
                            toastr.clear();
                        }
                    }, function (err) {
                        if (_this.isDeletionComplete(deletionTracker)) {
                            toastr.clear();
                        }
                        toastr.error("The deletion of package '" + model + "' failed. " + err.message);
                    });
                };
                __decorate([
                    core_1.ViewChild(index_4.ModalComponent), 
                    __metadata('design:type', index_4.ModalComponent)
                ], DynamicContent.prototype, "modalRef", void 0);
                return DynamicContent;
            }(index_2.DynamicGem));
            exports_1("DynamicContent", DynamicContent);
            DynamicContentAPI = (function (_super) {
                __extends(DynamicContentAPI, _super);
                function DynamicContentAPI(serviceBaseURL, http, aws) {
                    _super.call(this, serviceBaseURL, http, aws);
                }
                DynamicContentAPI.prototype.report = function (metric) {
                    metric.name = "Total Files";
                    metric.value = "Loading...";
                    _super.prototype.get.call(this, "portal/content").subscribe(function (response) {
                        var obj = JSON.parse(response.body.text());
                        var content = obj.result.PortalFileInfoList;
                        metric.value = content.length;
                    });
                };
                DynamicContentAPI.prototype.assign = function (tackableStatus) {
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
                DynamicContentAPI.prototype.delete = function (id) {
                    return _super.prototype.delete.call(this, "portal/info/" + id);
                };
                DynamicContentAPI.prototype.post = function (body) {
                    return _super.prototype.post.call(this, "portal/content", body);
                };
                DynamicContentAPI.prototype.get = function () {
                    return _super.prototype.get.call(this, "portal/content");
                };
                return DynamicContentAPI;
            }(index_3.ApiHandler));
            exports_1("DynamicContentAPI", DynamicContentAPI);
        }
    }
});

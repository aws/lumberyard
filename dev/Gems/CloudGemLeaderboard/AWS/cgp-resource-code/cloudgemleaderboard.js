System.register(['@angular/core', '@angular/forms', 'app/view/game/module/cloudgems/class/index', 'app/shared/class/index', 'app/shared/component/index'], function(exports_1, context_1) {
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
    var core_1, forms_1, index_1, index_2, index_3, index_4;
    var cacheTimeLimitInSeconds, LeaderboardMode, PlayerMode, Leaderboard, LeaderboardAPI, PageModel, LeaderboardForm, PlayerMeasure;
    function getInstance(context) {
        var gem = new Leaderboard();
        gem.initialize(context);
        return gem;
    }
    exports_1("getInstance", getInstance);
    return {
        setters:[
            function (core_1_1) {
                core_1 = core_1_1;
            },
            function (forms_1_1) {
                forms_1 = forms_1_1;
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
            }],
        execute: function() {
            cacheTimeLimitInSeconds = 300; // 5 minute
            (function (LeaderboardMode) {
                LeaderboardMode[LeaderboardMode["Add"] = 0] = "Add";
                LeaderboardMode[LeaderboardMode["List"] = 1] = "List";
                LeaderboardMode[LeaderboardMode["Show"] = 2] = "Show";
                LeaderboardMode[LeaderboardMode["Edit"] = 3] = "Edit";
                LeaderboardMode[LeaderboardMode["Delete"] = 4] = "Delete";
                LeaderboardMode[LeaderboardMode["Unban"] = 5] = "Unban";
                LeaderboardMode[LeaderboardMode["ProcessQueue"] = 6] = "ProcessQueue";
            })(LeaderboardMode || (LeaderboardMode = {}));
            exports_1("LeaderboardMode", LeaderboardMode);
            (function (PlayerMode) {
                PlayerMode[PlayerMode["List"] = 0] = "List";
                PlayerMode[PlayerMode["BanUser"] = 1] = "BanUser";
                PlayerMode[PlayerMode["DeleteScore"] = 2] = "DeleteScore";
            })(PlayerMode || (PlayerMode = {}));
            exports_1("PlayerMode", PlayerMode);
            Leaderboard = (function (_super) {
                __extends(Leaderboard, _super);
                //Need an empty constructor or Firefox will complain.
                function Leaderboard() {
                    _super.call(this);
                    this.tackable = {
                        displayName: "Leaderboard",
                        iconSrc: "https://m.media-amazon.com/images/G/01/cloudcanvas/images/Leadboard_icon._V536715120_.png",
                        cost: "High",
                        state: new index_1.TackableStatus(),
                        metric: new index_1.TackableMeasure()
                    };
                    this.pageSize = 25;
                    this.subNavActiveIndex = 0;
                    // Form Controls
                    this.searchScoresControl = new forms_1.FormControl();
                }
                Leaderboard.prototype.classType = function () {
                    var clone = Leaderboard;
                    return clone;
                };
                Object.defineProperty(Leaderboard.prototype, "htmlTemplateUrl", {
                    get: function () {
                        return this.context.provider.definition.isProd ? this.getSignedUrl(this.context.meta.key + "/cloudgemleaderboard.html") : "external/cloudgemleaderboard/cloudgemleaderboard.html";
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(Leaderboard.prototype, "styleTemplateUrl", {
                    get: function () {
                        return this.context.provider.definition.isProd ? this.getSignedUrl(this.context.meta.key + "/cloudgemleaderboard.css") : "external/cloudgemleaderboard/cloudgemleaderboard.css";
                    },
                    enumerable: true,
                    configurable: true
                });
                Leaderboard.prototype.initialize = function (context) {
                    _super.prototype.initialize.call(this, context);
                    this.apiHandler = new LeaderboardAPI(this.context.meta.cloudFormationOutputMap["ServiceUrl"].outputValue, this.context.provider.http, this.context.provider.aws);
                    this.apiHandler.report(this.tackable.metric);
                    this.apiHandler.assign(this.tackable.state);
                };
                Leaderboard.prototype.ngOnInit = function () {
                    this.aggregateModes = [
                        {
                            display: "Overwrite",
                            server: "OVERWRITE"
                        },
                        {
                            display: "Increment",
                            server: "INCREMENT"
                        }
                    ];
                    //bind proper scopes
                    this.addModal = this.addModal.bind(this);
                    this.editModal = this.editModal.bind(this);
                    this.deleteModal = this.deleteModal.bind(this);
                    this.dismissModal = this.dismissModal.bind(this);
                    this.list = this.list.bind(this);
                    this.addLeaderboard = this.addLeaderboard.bind(this);
                    this.editLeaderboard = this.editLeaderboard.bind(this);
                    this.show = this.show.bind(this);
                    this.deleteLeaderboard = this.deleteLeaderboard.bind(this);
                    this.deleteScoreModal = this.deleteScoreModal.bind(this);
                    this.deleteScore = this.deleteScore.bind(this);
                    this.getBannedPlayers = this.getBannedPlayers.bind(this);
                    this.banPlayerModal = this.banPlayerModal.bind(this);
                    this.banPlayer = this.banPlayer.bind(this);
                    this.dismissPlayerModal = this.dismissPlayerModal.bind(this);
                    this.unbanPlayerModal = this.unbanPlayerModal.bind(this);
                    this.unbanPlayer = this.unbanPlayer.bind(this);
                    this.processLeaderboardQueue = this.processLeaderboardQueue.bind(this);
                    this.checkLocalScoreCacheForChanges = this.checkLocalScoreCacheForChanges.bind(this);
                    // end of scope binding
                    // Pass modes to view and set default mode
                    this.leaderboardModes = LeaderboardMode;
                    this.leaderboards = [];
                    this.list();
                    this.playerModes = PlayerMode;
                    this.playerMode = PlayerMode.List;
                    this.currentPlayerSearch = "";
                    this.currentLeaderboard = new LeaderboardForm("", null, null, null);
                };
                Leaderboard.prototype.playerSearchUpdated = function (searchResult) {
                    var _this = this;
                    if (searchResult.value || searchResult.value.length > 0) {
                        this.currentLeaderboard.numberOfPages = 0;
                        this.apiHandler.searchLeaderboard(this.currentLeaderboard.name, searchResult.value)
                            .subscribe(function (response) {
                            var obj = JSON.parse(response.body.text());
                            var searchResults = obj.result.scores;
                            _this.currentScores = searchResults;
                            _this.currentScores = _this.checkLocalScoreCacheForChanges();
                            _this.isLoadingScores = false;
                        }, function (err) {
                            toastr.error(err);
                            _this.isLoadingScores = false;
                        });
                    }
                    else {
                        // If there is no value just get the default list of scores for the leaderboard
                        this.show(this.currentLeaderboard);
                    }
                };
                Leaderboard.prototype.validate = function (model) {
                    var isValid = true;
                    var split = model.name.split(" ");
                    model.validation = {
                        id: {},
                        min: {},
                        max: {}
                    };
                    if (split.length > 1) {
                        isValid = false;
                        model.validation.id = {
                            valid: false,
                            message: "The leaderboard identifier can not have any spaces in it."
                        };
                    }
                    else if (split.length == 0 || split[0].length == 0) {
                        isValid = false;
                        model.validation.id = {
                            valid: false,
                            message: "The leaderboard identifier can not be empty."
                        };
                    }
                    else if (!split[0].toString().match(/^[0-9a-zA-Z]+$/)) {
                        isValid = false;
                        model.validation.id = {
                            valid: false,
                            message: "The leaderboard identifier can not contains non-alphanumeric characters"
                        };
                    }
                    else {
                        split[0] = split[0].trim();
                    }
                    model.validation.max = {
                        valid: true
                    };
                    model.validation.min = {
                        valid: true
                    };
                    if (model.max != null && Number(model.max) === NaN) {
                        model.validation.max = false;
                    }
                    else if (model.max != null) {
                        model.validation.max = { valid: Number(model.max) >= 0 };
                    }
                    if (model.min != null && Number(model.min) === NaN) {
                        model.validation.min = false;
                    }
                    else if (model.min != null) {
                        model.validation.min = { valid: Number(model.min) >= 0 };
                    }
                    if (!model.validation.max.valid) {
                        model.validation.max.message = "This value must be numeric and greater than zero.";
                        isValid = false;
                    }
                    if (!model.validation.min.valid) {
                        model.validation.min.message = "This value must be numeric and greater than zero.";
                        isValid = false;
                    }
                    if (model.validation.max.valid && model.validation.min.valid) {
                        var max = Number(model.max);
                        var min = Number(model.min);
                        if (min > max) {
                            model.validation.max.valid = false;
                            model.validation.min.valid = false;
                            model.validation.min.message = "The minimum reportable value must be a greater than the maximum reportable value.";
                            isValid = false;
                        }
                    }
                    var addMode = LeaderboardMode.Add;
                    if (Number(model.state) == addMode) {
                        for (var i = 0; i < model.leaderboards.length; i++) {
                            var id = model.leaderboards[i].name;
                            if (id == split[0]) {
                                isValid = false;
                                model.validation.id = {
                                    valid: false,
                                    message: "That leaderboard identifier is already in use in your project.  Please choose another name."
                                };
                                break;
                            }
                        }
                    }
                    if (isValid)
                        model.validation = null;
                    return isValid;
                };
                // Leaderboard action stub functions
                Leaderboard.prototype.addModal = function (leaderboard) {
                    this.mode = LeaderboardMode.Add;
                    this.currentLeaderboard = new LeaderboardForm("", this.aggregateModes[0].display);
                    this.currentLeaderboard.leaderboards = this.leaderboards;
                    this.currentLeaderboard.state = this.mode.toString();
                };
                Leaderboard.prototype.editModal = function (leaderboard) {
                    this.mode = LeaderboardMode.Edit;
                    this.currentLeaderboard = new LeaderboardForm(leaderboard.name, leaderboard.mode, leaderboard.min, leaderboard.max);
                    this.currentLeaderboard.leaderboards = this.leaderboards;
                    this.currentLeaderboard.state = this.mode.toString();
                };
                Leaderboard.prototype.deleteModal = function (leaderboard) {
                    this.mode = LeaderboardMode.Delete;
                    this.currentLeaderboard = leaderboard;
                };
                Leaderboard.prototype.dismissModal = function () {
                    this.mode = LeaderboardMode.List;
                };
                Leaderboard.prototype.show = function (leaderboard, currentPageIndex) {
                    var _this = this;
                    if (currentPageIndex === void 0) { currentPageIndex = 1; }
                    this.currentLeaderboard = leaderboard;
                    this.mode = LeaderboardMode.Show;
                    this.isLoadingScores = true;
                    if (leaderboard.additional_data === undefined) {
                        leaderboard.additional_data = {
                            users: [],
                            page: currentPageIndex,
                            page_size: this.pageSize
                        };
                        leaderboard.numberOfPages = 0;
                    }
                    leaderboard.additional_data.page = currentPageIndex - 1;
                    if (this.getScoresSub) {
                        this.getScoresSub.unsubscribe();
                    }
                    this.getScoresSub = this.apiHandler.getEntries(leaderboard.name, leaderboard.additional_data).subscribe(function (response) {
                        var json = JSON.parse(response.body.text());
                        _this.currentScores = json.result.scores;
                        leaderboard.numberOfPages = json.result.total_pages;
                        _this.currentScores = _this.checkLocalScoreCacheForChanges();
                        _this.isLoadingScores = false;
                    }, function (err) {
                        toastr.error("The leaderboard with id '" + leaderboard.name + "' failed to retrieve its data: " + err);
                        _this.isLoadingScores = false;
                    });
                };
                Leaderboard.prototype.list = function () {
                    var _this = this;
                    this.isLoadingLeaderboards = true;
                    this.mode = LeaderboardMode.List;
                    this.currentScores = null;
                    this.apiHandler.get("stats").subscribe(function (response) {
                        var obj = JSON.parse(response.body.text());
                        _this.leaderboards = obj.result.stats;
                        _this.isLoadingLeaderboards = false;
                    }, function (err) {
                        toastr.error("The leaderboards did not refresh properly: " + err);
                        _this.isLoadingLeaderboards = false;
                    });
                };
                // Functions inside modals
                Leaderboard.prototype.addLeaderboard = function (model) {
                    var _this = this;
                    if (this.validate(model)) {
                        var body = this.getPostObject();
                        this.modalRef.close();
                        this.apiHandler.addLeaderboard(body).subscribe(function (response) {
                            toastr.success("The leaderboard with id '" + _this.currentLeaderboard.name + "' was created.");
                            _this.currentLeaderboard = new LeaderboardForm("");
                            _this.list();
                            _this.mode = LeaderboardMode.List;
                        }, function (err) {
                            toastr.error("The leaderboard could not be created: " + err);
                            _this.list();
                        });
                    }
                };
                Leaderboard.prototype.editLeaderboard = function (model) {
                    var _this = this;
                    if (this.validate(model)) {
                        var body = this.getPostObject();
                        this.modalRef.close();
                        this.apiHandler.editLeaderboard(body).subscribe(function (response) {
                            toastr.success("The leaderboard with id '" + _this.currentLeaderboard.name + "' was updated.");
                            _this.list();
                            _this.mode = LeaderboardMode.List;
                        }, function (err) {
                            toastr.error("The leaderboard could not be updated: " + err);
                            _this.list();
                        });
                    }
                };
                Leaderboard.prototype.deleteLeaderboard = function () {
                    var _this = this;
                    this.modalRef.close();
                    this.apiHandler.deleteLeaderboard(this.currentLeaderboard.name).subscribe(function (response) {
                        toastr.success("The leaderboard with id '" + _this.currentLeaderboard.name + "' has been deleted.");
                        _this.list();
                        _this.mode = LeaderboardMode.List;
                    }, function (err) {
                        toastr.error("The leaderboard could not be deleted: " + err);
                    });
                };
                Leaderboard.prototype.subNavUpdated = function (activeIndex) {
                    this.subNavActiveIndex = activeIndex;
                    if (this.subNavActiveIndex == 0) {
                        this.list();
                    }
                    else if (this.subNavActiveIndex == 1) {
                        this.getBannedPlayers();
                    }
                };
                Leaderboard.prototype.deleteScoreModal = function (model) {
                    this.playerMode = PlayerMode.DeleteScore;
                    this.currentScore = model;
                };
                Leaderboard.prototype.deleteScore = function () {
                    var _this = this;
                    this.isLoadingScores = true;
                    this.modalRef.close();
                    this.apiHandler.deleteScore(this.currentLeaderboard.name, this.currentScore.user).subscribe(function (response) {
                        toastr.success("The score for player '" + _this.currentScore.user + "' was deleted successfully.");
                        _this.context.provider.cache.setObject(_this.currentScore, _this.currentScore);
                        _this.currentScores = _this.checkLocalScoreCacheForChanges();
                        _this.isLoadingScores = false;
                        _this.playerMode = PlayerMode.List;
                    }, function (err) {
                        _this.playerMode = PlayerMode.List;
                        toastr.error("The score for could not be deleted: " + err);
                    });
                };
                Leaderboard.prototype.getBannedPlayers = function () {
                    var _this = this;
                    this.isLoadingBannedPlayers = true;
                    this.apiHandler.get("player/ban_list").subscribe(function (response) {
                        _this.isLoadingBannedPlayers = false;
                        var obj = JSON.parse(response.body.text());
                        _this.bannedPlayers = obj.result.players;
                    }, function (err) {
                        _this.isLoadingBannedPlayers = false;
                        toastr.error("Unable to get the list of banned players: " + err);
                    });
                };
                Leaderboard.prototype.banPlayerModal = function (score) {
                    this.playerMode = PlayerMode.BanUser;
                    this.currentScore = score;
                };
                Leaderboard.prototype.banPlayer = function () {
                    var _this = this;
                    this.isLoadingScores = true;
                    this.modalRef.close();
                    this.apiHandler.banPlayer(this.currentScore.user).subscribe(function (response) {
                        var obj = JSON.parse(response.body.text());
                        toastr.success("The player '" + _this.currentScore.user + "' has been banned.");
                        _this.context.provider.cache.set(_this.currentScore.user, _this.currentScore.user);
                        _this.currentScores = _this.checkLocalScoreCacheForChanges();
                        _this.isLoadingScores = false;
                        _this.playerMode = PlayerMode.List;
                    }, function (err) {
                        _this.playerMode = PlayerMode.List;
                        toastr.error("The player could not be banned: " + err);
                    });
                };
                Leaderboard.prototype.unbanPlayerModal = function (model) {
                    this.currentPlayer = model;
                    this.mode = LeaderboardMode.Unban;
                };
                Leaderboard.prototype.unbanPlayer = function () {
                    var _this = this;
                    this.modalRef.close();
                    this.apiHandler.unbanPlayer(this.currentPlayer).subscribe(function (response) {
                        var obj = JSON.parse(response.body.text());
                        toastr.success("The player '" + _this.currentPlayer + "' has had their ban lifted");
                        toastr.success("The player '" + _this.currentPlayer + "' has had their ban lifted.");
                        _this.context.provider.cache.remove(_this.currentPlayer);
                        _this.getBannedPlayers();
                        _this.mode = LeaderboardMode.List;
                    }, function (err) {
                        _this.mode = LeaderboardMode.List;
                        toastr.error("The ban could not be removed: " + err);
                    });
                };
                Leaderboard.prototype.dismissPlayerModal = function () {
                    this.playerMode = PlayerMode.List;
                };
                Leaderboard.prototype.promptProcessLeaderboard = function () {
                    this.mode = LeaderboardMode.ProcessQueue;
                };
                Leaderboard.prototype.processLeaderboardQueue = function () {
                    this.modalRef.close();
                    this.mode = LeaderboardMode.List;
                    this.apiHandler.processRecords().subscribe(function (response) {
                        toastr.success("The leaderboard processor has been started. Processing should only take a few moments.");
                    }, function (err) {
                        toastr.error("The leaderboard process failed to start" + err);
                    });
                };
                Leaderboard.prototype.checkLocalScoreCacheForChanges = function () {
                    var currentCachedScores = this.currentScores;
                    var indexBlacklist = Array();
                    for (var i = 0; i < this.currentScores.length; i++) {
                        var isScoreDeleted = this.context.provider.cache.objectExists(this.currentScores[i]);
                        var isPlayerBanned = this.context.provider.cache.exists(this.currentScores[i].user);
                        if (isScoreDeleted || isPlayerBanned) {
                            indexBlacklist.push(i);
                        }
                    }
                    var itemsRemoved = 0;
                    indexBlacklist.forEach(function (blackListedIndex) {
                        currentCachedScores.splice(blackListedIndex - itemsRemoved, 1);
                        itemsRemoved++;
                    });
                    return currentCachedScores;
                };
                Leaderboard.prototype.getPostObject = function () {
                    if (this.currentLeaderboard.min == null && this.currentLeaderboard.max == null) {
                        var body = new LeaderboardForm(this.currentLeaderboard.name, this.currentLeaderboard.mode.toUpperCase());
                    }
                    else if (this.currentLeaderboard.min != null && this.currentLeaderboard.max != null) {
                        var body = new LeaderboardForm(this.currentLeaderboard.name, this.currentLeaderboard.mode.toUpperCase(), this.currentLeaderboard.min, this.currentLeaderboard.max);
                    }
                    else if (this.currentLeaderboard.min != null) {
                        var body = new LeaderboardForm(this.currentLeaderboard.name, this.currentLeaderboard.mode.toUpperCase(), this.currentLeaderboard.min);
                    }
                    else if (this.currentLeaderboard.max != null) {
                        var body = new LeaderboardForm(this.currentLeaderboard.name, this.currentLeaderboard.mode.toUpperCase(), undefined, this.currentLeaderboard.max);
                    }
                    return body;
                };
                __decorate([
                    core_1.ViewChild(index_4.ModalComponent), 
                    __metadata('design:type', index_4.ModalComponent)
                ], Leaderboard.prototype, "modalRef", void 0);
                return Leaderboard;
            }(index_2.DynamicGem));
            exports_1("Leaderboard", Leaderboard);
            LeaderboardAPI = (function (_super) {
                __extends(LeaderboardAPI, _super);
                function LeaderboardAPI(serviceBaseURL, http, aws) {
                    _super.call(this, serviceBaseURL, http, aws);
                }
                LeaderboardAPI.prototype.report = function (metric) {
                    metric.name = "Total Leaderboards";
                    metric.value = "Loading...";
                    _super.prototype.get.call(this, "stats").subscribe(function (response) {
                        var obj = JSON.parse(response.body.text());
                        metric.value = obj.result.stats.length;
                    }, function (err) {
                        metric.value = "Offline";
                    });
                };
                LeaderboardAPI.prototype.assign = function (tackableStatus) {
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
                LeaderboardAPI.prototype.getEntries = function (leaderboard, body) {
                    return _super.prototype.post.call(this, "leaderboard/" + leaderboard, body);
                };
                LeaderboardAPI.prototype.deleteLeaderboard = function (id) {
                    return _super.prototype.delete.call(this, "stats/" + id);
                };
                LeaderboardAPI.prototype.addLeaderboard = function (body) {
                    return _super.prototype.post.call(this, "stats", body);
                };
                LeaderboardAPI.prototype.editLeaderboard = function (body) {
                    return _super.prototype.post.call(this, "stats", body);
                };
                LeaderboardAPI.prototype.searchLeaderboard = function (leaderboardid, id) {
                    return _super.prototype.get.call(this, "score/" + leaderboardid + "/" + id);
                };
                LeaderboardAPI.prototype.deleteScore = function (leaderboardId, currentScore) {
                    return _super.prototype.delete.call(this, "score/" + leaderboardId + "/" + currentScore);
                };
                LeaderboardAPI.prototype.banPlayer = function (user) {
                    return _super.prototype.post.call(this, "player/ban/" + user);
                };
                LeaderboardAPI.prototype.unbanPlayer = function (user) {
                    return _super.prototype.delete.call(this, "player/ban/" + encodeURIComponent(user));
                };
                LeaderboardAPI.prototype.processRecords = function () {
                    return _super.prototype.get.call(this, "process_records");
                };
                return LeaderboardAPI;
            }(index_3.ApiHandler));
            exports_1("LeaderboardAPI", LeaderboardAPI);
            ///Models
            PageModel = (function () {
                function PageModel() {
                }
                return PageModel;
            }());
            LeaderboardForm = (function () {
                function LeaderboardForm(name, mode, min, max, leaderboards, state) {
                    this.name = name;
                    this.mode = mode;
                    this.min = min == "" ? null : min;
                    this.max = max == "" ? null : max;
                    this.leaderboards = leaderboards;
                    this.state = state;
                }
                return LeaderboardForm;
            }());
            PlayerMeasure = (function () {
                function PlayerMeasure() {
                }
                return PlayerMeasure;
            }());
        }
    }
});

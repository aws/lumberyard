System.register(['@angular/core', '@angular/forms', 'app/view/game/module/cloudgems/class/index', 'app/shared/class/index', 'app/shared/component/index', "app/view/game/module/shared/class/index"], function(exports_1, context_1) {
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
    var core_1, forms_1, index_1, index_2, index_3, index_4, index_5;
    var Mode, PlayerAccount, PlayerAccountAPI, ValidationModelEntry, ValidationModel, PlayerAccountEditModel, SerializationModel, PlayerAccountModel, IdentityModel, CognitoModel;
    function getInstance(context) {
        var gem = new PlayerAccount();
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
            },
            function (index_5_1) {
                index_5 = index_5_1;
            }],
        execute: function() {
            (function (Mode) {
                Mode[Mode["List"] = 0] = "List";
                Mode[Mode["Edit"] = 1] = "Edit";
                Mode[Mode["Delete"] = 2] = "Delete";
                Mode[Mode["Add"] = 3] = "Add";
                Mode[Mode["Show"] = 4] = "Show";
                Mode[Mode["Ban"] = 5] = "Ban";
            })(Mode || (Mode = {}));
            exports_1("Mode", Mode);
            PlayerAccount = (function (_super) {
                __extends(PlayerAccount, _super);
                //Required.  Firefox will complain if the constructor is not present with no arguments
                function PlayerAccount() {
                    _super.call(this);
                    this.identifier = "playeraccount";
                    this.activeSubMenuIndex = 0;
                    this.searchControl = new forms_1.FormControl();
                    this.dateTimeUtil = index_3.DateTimeUtil;
                    this.accountStatus = {
                        active: "active",
                        pending: "pending",
                        anonymous: "anonymous",
                        unconfirmed: "unconfirmed",
                        archived: "archived",
                        confirmed: "confirmed",
                        compromised: "compromised",
                        reset_required: "reset_required",
                        force_change_password: "force_change_password",
                        disabled: "disabled",
                        unknown: "unknown"
                    };
                    this.awsCognitoLink = "https://console.aws.amazon.com/cognito/home";
                    this.tackable = {
                        displayName: "Player Account",
                        iconSrc: "https://m.media-amazon.com/images/G/01/cloudcanvas/images/player_account._V536715123_.png",
                        cost: "Low",
                        state: new index_1.TackableStatus(),
                        metric: new index_1.TackableMeasure()
                    };
                }
                PlayerAccount.prototype.classType = function () {
                    var clone = PlayerAccount;
                    return clone;
                };
                Object.defineProperty(PlayerAccount.prototype, "htmlTemplateUrl", {
                    get: function () {
                        return this.context.provider.definition.isProd ? this.getSignedUrl(this.context.meta.key + "/cloudgemplayeraccount.html") : "external/cloudgemplayeraccount/cloudgemplayeraccount.html";
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(PlayerAccount.prototype, "styleTemplateUrl", {
                    get: function () {
                        return this.context.provider.definition.isProd ? this.getSignedUrl(this.context.meta.key + "/cloudgemplayeraccount.css") : "external/cloudgemplayeraccount/cloudgemplayeraccount.css";
                    },
                    enumerable: true,
                    configurable: true
                });
                Object.defineProperty(PlayerAccount.prototype, "searchType", {
                    get: function () {
                        var savedType = localStorage.getItem(this.identifier + "searchTypeId");
                        var n_SavedType = parseInt(savedType);
                        if (savedType === null || savedType === undefined || savedType === "" || n_SavedType === NaN) {
                            return 0;
                        }
                        return n_SavedType;
                    },
                    set: function (value) {
                        if (value === undefined) {
                            return;
                        }
                        localStorage.setItem(this.identifier + "searchTypeId", value.toString());
                    },
                    enumerable: true,
                    configurable: true
                });
                PlayerAccount.prototype.initialize = function (context) {
                    _super.prototype.initialize.call(this, context);
                    this.apiHandler = new PlayerAccountAPI(this.context.meta.cloudFormationOutputMap["ServiceUrl"].outputValue, this.context.provider.http, this.context.provider.aws);
                    this.apiHandler.report(this.tackable.metric);
                    this.apiHandler.assign(this.tackable.state);
                };
                PlayerAccount.prototype.ngOnInit = function () {
                    this.mode = Mode.List;
                    this.modes = Mode;
                    // bind proper scopes
                    this.show = this.show.bind(this);
                    this.onAdd = this.onAdd.bind(this);
                    this.dismissAddModal = this.dismissAddModal.bind(this);
                    this.addAccount = this.addAccount.bind(this);
                    this.onEdit = this.onEdit.bind(this);
                    this.dismissEditModal = this.dismissEditModal.bind(this);
                    this.edit = this.edit.bind(this);
                    this.changePassword = this.changePassword.bind(this);
                    this.confirmAccount = this.confirmAccount.bind(this);
                    this.banAccount = this.banAccount.bind(this);
                    this.removeBan = this.removeBan.bind(this);
                    this.apiHandler.searchAccountId = this.apiHandler.searchAccountId.bind(this.apiHandler);
                    this.apiHandler.searchPlayerName = this.apiHandler.searchPlayerName.bind(this.apiHandler);
                    this.apiHandler.searchCognitoId = this.apiHandler.searchCognitoId.bind(this.apiHandler);
                    this.apiHandler.searchCognitoUsername = this.apiHandler.searchCognitoUsername.bind(this.apiHandler);
                    this.apiHandler.searchCognitoEmail = this.apiHandler.searchCognitoEmail.bind(this.apiHandler);
                    this.apiHandler.searchBannedPlayers = this.apiHandler.searchBannedPlayers.bind(this.apiHandler);
                    for (var customAction in this.actionStubActions) {
                        this.actionStubActions[customAction].onClick = this.actionStubActions[customAction].onClick.bind(this);
                    }
                    // end of bind scopes
                    this.searchTypes = [
                        { text: 'Account Id', functionCb: this.apiHandler.searchAccountId },
                        { text: 'Player Name', functionCb: this.apiHandler.searchPlayerName },
                        { text: 'Cognito Identity', functionCb: this.apiHandler.searchCognitoId },
                        { text: 'Cognito Username', functionCb: this.apiHandler.searchCognitoUsername },
                        { text: 'Cognito Email', functionCb: this.apiHandler.searchCognitoEmail }
                    ];
                    this.currentSearchType = this.searchTypes[0];
                    this.list();
                };
                PlayerAccount.prototype.search = function (searchResult) {
                    var _this = this;
                    if (searchResult.value === null || searchResult.value.length === 0) {
                        this.list();
                        return;
                    }
                    searchResult.value = searchResult.value.trim();
                    var handler = null;
                    for (var _i = 0, _a = this.searchTypes; _i < _a.length; _i++) {
                        var searchType = _a[_i];
                        if (searchType.text === searchResult.id) {
                            handler = searchType.functionCb;
                            break;
                        }
                    }
                    this.mode = Mode.List;
                    this.isLoading = true;
                    handler(searchResult.value).subscribe(function (response) {
                        var obj = JSON.parse(response.body.text());
                        if (obj.result.Accounts === undefined) {
                            _this.listModel = [];
                            _this.listModel.push(obj.result);
                        }
                        else {
                            _this.listModel = obj.result.Accounts;
                        }
                        _this.isLoading = false;
                    }, function (err) {
                        toastr.error(err.message);
                        _this.isLoading = false;
                    });
                };
                PlayerAccount.prototype.containsAccountStatus = function (entry, status) {
                    if (entry === undefined || entry.IdentityProviders === undefined)
                        return false;
                    return entry.IdentityProviders.Cognito.status.toLowerCase().indexOf(status) != -1;
                };
                PlayerAccount.prototype.list = function () {
                    var _this = this;
                    this.mode = Mode.List;
                    this.isLoading = true;
                    this.listModel = [];
                    if (this.getAccountsSub) {
                        this.getAccountsSub.unsubscribe();
                    }
                    this.getAccountsSub = this.apiHandler.emptySearch().subscribe(function (response) {
                        var obj = JSON.parse(response.body.text());
                        // TODO: This is a temp filtering to get the blacklisted accounts.  Ideally this would be a seperate API or possible search result
                        if (_this.activeSubMenuIndex === 1) {
                            for (var _i = 0, _a = obj.result.Accounts; _i < _a.length; _i++) {
                                var account = _a[_i];
                                if (account.AccountBlacklisted) {
                                    _this.listModel.push(account);
                                }
                            }
                        }
                        else {
                            for (var _b = 0, _c = obj.result.Accounts; _b < _c.length; _b++) {
                                var account = _c[_b];
                                if (!account.AccountBlacklisted) {
                                    _this.listModel.push(account);
                                }
                            }
                        }
                        _this.isLoading = false;
                    }, function (err) {
                        toastr.clear();
                        toastr.error("The player list failed to update with error. " + err.message);
                    });
                };
                PlayerAccount.prototype.show = function (entry) {
                    this.mode = Mode.Show;
                    this.actionStubActions = [];
                    if (entry.IdentityProviders && entry.IdentityProviders.Cognito.status.toString() === "RESET_REQUIRED") {
                        this.actionStubActions.push(new index_5.ActionItem("Reset Password", this.changePassword));
                    }
                    else if (entry.IdentityProviders && entry.IdentityProviders.Cognito.status !== "FORCE_CHANGE_PASSWORD"
                        && entry.IdentityProviders.Cognito.status !== "CONFIRMED") {
                        this.actionStubActions.push(new index_5.ActionItem("Confirm Account", this.confirmAccount));
                    }
                    this.showModel = entry;
                };
                PlayerAccount.prototype.onAdd = function () {
                    var playerAccount = new PlayerAccountModel();
                    this.addModel = {
                        serialization: {
                            CognitoUsername: "",
                            IdentityProviders: {
                                Cognito: {
                                    gender: "None"
                                }
                            }
                        },
                        validation: new ValidationModel()
                    };
                    this.mode = Mode.Add;
                };
                PlayerAccount.prototype.onEdit = function (model) {
                    this.editModel = {
                        accountId: model.AccountId,
                        username: model.CognitoUsername,
                        status: model.IdentityProviders ? model.IdentityProviders.Cognito.status : 'ANONYMOUS',
                        lastModified: model.IdentityProviders ? model.IdentityProviders.Cognito.last_modified_date : '',
                        serialization: {
                            PlayerName: model.PlayerName
                        },
                        validation: new ValidationModel()
                    };
                    if (model.IdentityProviders) {
                        this.editModel.serialization.IdentityProviders = {
                            Cognito: {
                                email: model.IdentityProviders.Cognito.email,
                                given_name: model.IdentityProviders.Cognito.given_name,
                                family_name: model.IdentityProviders.Cognito.family_name,
                                locale: model.IdentityProviders.Cognito.locale,
                                nickname: model.IdentityProviders.Cognito.nickname,
                                gender: !model.IdentityProviders.Cognito.gender ? 'None' : model.IdentityProviders.Cognito.gender
                            }
                        };
                    }
                    this.mode = Mode.Edit;
                };
                PlayerAccount.prototype.validate = function (model, requireUsername) {
                    var hasErrors = false;
                    if (requireUsername && !model.serialization.CognitoUsername) {
                        model.validation.username.isvalid = false;
                        hasErrors = true;
                    }
                    if (model.serialization.IdentityProviders && !model.serialization.IdentityProviders.Cognito.email) {
                        model.validation.email.isvalid = false;
                        hasErrors = true;
                    }
                    if (!hasErrors)
                        return true;
                };
                PlayerAccount.prototype.addAccount = function () {
                    var _this = this;
                    if (this.validate(this.addModel, true)) {
                        this.apiHandler.createAccount(this.addModel.serialization).subscribe(function (response) {
                            _this.modalRef.close();
                            toastr.clear();
                            toastr.success("The player '" + _this.addModel.serialization.CognitoUsername + "' has been added");
                            _this.list();
                        }, function (err) {
                            toastr.clear();
                            toastr.error("Unable to create new account: " + err.message);
                            console.error(err);
                        });
                    }
                };
                PlayerAccount.prototype.edit = function (model) {
                    var _this = this;
                    if (this.validate(model, false)) {
                        if (model.serialization.IdentityProviders && model.serialization.IdentityProviders.Cognito.gender == 'None')
                            model.serialization.IdentityProviders.Cognito.gender = "";
                        this.apiHandler.editAccount(model.accountId, model.serialization).subscribe(function (response) {
                            _this.modalRef.close();
                            toastr.clear();
                            toastr.success("The player '" + model.username + "' has been updated.");
                            _this.list();
                        }, function (err) {
                            toastr.clear();
                            toastr.error("The player did not save correctly. " + err.message);
                        });
                    }
                };
                PlayerAccount.prototype.banAccount = function () {
                    var _this = this;
                    var banPlayerModel = {
                        "AccountBlacklisted": true
                    };
                    this.apiHandler.editAccount(this.currentAccount.AccountId, banPlayerModel).subscribe(function (response) {
                        toastr.success("The player '" + _this.currentAccount.CognitoUsername + "' has been banned");
                        _this.modalRef.close();
                        _this.list();
                    }, function (err) {
                        toastr.clear();
                        toastr.error("The player could not be banned. " + err.message);
                    });
                };
                PlayerAccount.prototype.removeBan = function () {
                    var _this = this;
                    var banPlayerModel = {
                        "AccountBlacklisted": false
                    };
                    this.apiHandler.editAccount(this.currentAccount.AccountId, banPlayerModel).subscribe(function (response) {
                        toastr.success("The player '" + _this.currentAccount.CognitoUsername + "'is no longer banned");
                        _this.modalRef.close();
                        _this.list();
                    }, function (err) {
                        toastr.clear();
                        toastr.error("Unable to remove the ban. " + err.message);
                        console.log(err);
                    });
                };
                PlayerAccount.prototype.changePassword = function (user) {
                    var _this = this;
                    this.apiHandler.resetUserPassword(user.CognitoUsername).subscribe(function (response) {
                        toastr.clear();
                        toastr.success("Password has been reset");
                        user.IdentityProviders.Cognito.status = _this.accountStatus.reset_required.toUpperCase();
                        ;
                    }, function (err) {
                        toastr.clear();
                        toastr.error("Could not reset password: " + err.message);
                        console.log(err);
                    });
                };
                PlayerAccount.prototype.confirmAccount = function (user) {
                    var _this = this;
                    this.apiHandler.confirmUser(user.CognitoUsername).subscribe(function (response) {
                        toastr.clear();
                        toastr.success("User has been confirmed");
                        _this.apiHandler.searchAccountId(user.AccountId);
                        user.IdentityProviders.Cognito.status = _this.accountStatus.confirmed.toUpperCase();
                    }, function (err) {
                        toastr.clear();
                        toastr.error("Could not reset password: " + err.message);
                        console.log(err);
                    });
                };
                PlayerAccount.prototype.changeSubMenuIndex = function (index) {
                    this.activeSubMenuIndex = index;
                    this.list();
                };
                PlayerAccount.prototype.dismissAddModal = function () {
                    this.mode = Mode.List;
                };
                PlayerAccount.prototype.dismissEditModal = function () {
                    this.mode = Mode.Show;
                };
                __decorate([
                    core_1.ViewChild(index_4.ModalComponent), 
                    __metadata('design:type', index_4.ModalComponent)
                ], PlayerAccount.prototype, "modalRef", void 0);
                return PlayerAccount;
            }(index_2.DynamicGem));
            exports_1("PlayerAccount", PlayerAccount);
            //REST API Handler
            PlayerAccountAPI = (function (_super) {
                __extends(PlayerAccountAPI, _super);
                function PlayerAccountAPI(serviceBaseURL, http, aws) {
                    _super.call(this, serviceBaseURL, http, aws);
                }
                PlayerAccountAPI.prototype.report = function (metric) {
                    metric.name = "Registered Player(s)";
                    metric.value = "Loading...";
                    this.get("admin/identityProviders/Cognito").subscribe(function (response) {
                        var obj = JSON.parse(response.body.text());
                        metric.value = obj.result.EstimatedNumberOfUsers.toString();
                    });
                };
                PlayerAccountAPI.prototype.assign = function (tackableStatus) {
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
                PlayerAccountAPI.prototype.searchCognitoEmail = function (email) {
                    return _super.prototype.get.call(this, "admin/accountSearch?CognitoIdentityId=&CognitoUsername=&Email=" + encodeURIComponent(email) + "&StartPlayerName=");
                };
                PlayerAccountAPI.prototype.searchCognitoUsername = function (name) {
                    return _super.prototype.get.call(this, "admin/accountSearch?CognitoIdentityId=&CognitoUsername=" + encodeURIComponent(name) + "&Email=&StartPlayerName=");
                };
                PlayerAccountAPI.prototype.searchCognitoId = function (id) {
                    return _super.prototype.get.call(this, "admin/accountSearch?CognitoIdentityId=" + encodeURIComponent(id) + "&CognitoUsername=&Email=&StartPlayerName=");
                };
                PlayerAccountAPI.prototype.searchPlayerName = function (name) {
                    return _super.prototype.get.call(this, "admin/accountSearch?CognitoIdentityId=&CognitoUsername=&Email=&StartPlayerName=" + encodeURIComponent(name));
                };
                PlayerAccountAPI.prototype.searchAccountId = function (id) {
                    return _super.prototype.get.call(this, "admin/accounts/" + encodeURIComponent(id));
                };
                PlayerAccountAPI.prototype.searchBannedPlayers = function (id) {
                    // TODO: Call search API for banned players here
                    return _super.prototype.get.call(this, "admin/accounts/" + encodeURIComponent(id));
                };
                PlayerAccountAPI.prototype.emptySearch = function () {
                    return _super.prototype.get.call(this, "admin/accountSearch");
                };
                PlayerAccountAPI.prototype.editAccount = function (id, body) {
                    return _super.prototype.put.call(this, "admin/accounts/" + encodeURIComponent(id), body);
                };
                PlayerAccountAPI.prototype.createAccount = function (body) {
                    return _super.prototype.post.call(this, "admin/accounts", body);
                };
                PlayerAccountAPI.prototype.resetUserPassword = function (name) {
                    return _super.prototype.post.call(this, "admin/identityProviders/Cognito/users/" + name + "/resetUserPassword");
                };
                PlayerAccountAPI.prototype.confirmUser = function (name) {
                    return _super.prototype.post.call(this, "admin/identityProviders/Cognito/users/" + name + "/confirmSignUp");
                };
                return PlayerAccountAPI;
            }(index_3.ApiHandler));
            exports_1("PlayerAccountAPI", PlayerAccountAPI);
            //end rest api handler
            //View models
            ValidationModelEntry = (function () {
                function ValidationModelEntry(isvalid, message) {
                    if (isvalid === void 0) { isvalid = true; }
                    if (message === void 0) { message = null; }
                    this.isvalid = true;
                    this.message = null;
                    this.isvalid = isvalid;
                    this.message = message;
                }
                return ValidationModelEntry;
            }());
            exports_1("ValidationModelEntry", ValidationModelEntry);
            ValidationModel = (function () {
                function ValidationModel() {
                    this.username = new ValidationModelEntry(true, "Username is required");
                    this.playerName = new ValidationModelEntry();
                    this.gender = new ValidationModelEntry();
                    this.locale = new ValidationModelEntry();
                    this.givenName = new ValidationModelEntry();
                    this.middleName = new ValidationModelEntry();
                    this.familyName = new ValidationModelEntry();
                    this.email = new ValidationModelEntry(true, "Email is required");
                    this.accountId = new ValidationModelEntry();
                    this.birthdate = new ValidationModelEntry();
                    this.phone = new ValidationModelEntry();
                    this.nickName = new ValidationModelEntry();
                }
                return ValidationModel;
            }());
            exports_1("ValidationModel", ValidationModel);
            PlayerAccountEditModel = (function () {
                function PlayerAccountEditModel() {
                }
                return PlayerAccountEditModel;
            }());
            exports_1("PlayerAccountEditModel", PlayerAccountEditModel);
            SerializationModel = (function () {
                function SerializationModel() {
                }
                return SerializationModel;
            }());
            exports_1("SerializationModel", SerializationModel);
            PlayerAccountModel = (function () {
                function PlayerAccountModel() {
                }
                return PlayerAccountModel;
            }());
            exports_1("PlayerAccountModel", PlayerAccountModel);
            IdentityModel = (function () {
                function IdentityModel() {
                }
                return IdentityModel;
            }());
            exports_1("IdentityModel", IdentityModel);
            CognitoModel = (function () {
                function CognitoModel() {
                }
                return CognitoModel;
            }());
            exports_1("CognitoModel", CognitoModel);
        }
    }
});
//end view models

import { Observable } from 'rxjs/rx';
import { Http } from '@angular/http';
import { ViewChild, SecurityContext } from '@angular/core';
import { FormControl } from '@angular/forms';
import { Tackable, Measurable, Stateable, StyleType, Gemifiable, TackableStatus, TackableMeasure } from 'app/view/game/module/cloudgems/class/index';
import { DynamicGem } from 'app/view/game/module/cloudgems/class/index';
import { DateTimeUtil, ApiHandler, StringUtil } from 'app/shared/class/index';
import { ModalComponent } from 'app/shared/component/index';
import { AwsService } from "app/aws/aws.service";
import { ActionItem } from "app/view/game/module/shared/class/index";
import { Subscription } from 'rxjs/rx';
import { SearchResult } from 'app/view/game/module/shared/component/index';

declare const toastr: any;

export enum Mode {
    List = 0,
    Edit = 1,
    Delete = 2,
    Add = 3,
    Show = 4,
    Ban = 5
}

export class PlayerAccount extends DynamicGem {
    public identifier = "playeraccount";
    public activeSubMenuIndex = 0;
    public apiHandler: PlayerAccountAPI;
    private isLoading: boolean;
    private mode: Mode;
    private modes: any;
    private addModel: PlayerAccountEditModel;
    private editModel: PlayerAccountEditModel;
    private listModel: PlayerAccountModel[];
    private showModel: PlayerAccountModel;
    public searchControl = new FormControl();
    public dateTimeUtil = DateTimeUtil;

    private accountStatus = {
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
    }

    private searchTypes: [{ text: string, functionCb: Function }];

    private actionStubActions: ActionItem[];
    private awsCognitoLink = "https://console.aws.amazon.com/cognito/home"

    private currentSearchType: { text: string, functionCb: Function };

    private currentAccount: PlayerAccountModel;
    private getAccountsSub: Subscription;

    @ViewChild(ModalComponent) modalRef: ModalComponent;

    public classType(): any {
        const clone = PlayerAccount
        return clone;
    }

    public get htmlTemplateUrl(): string {
        return this.context.provider.definition.isProd ? this.getSignedUrl(this.context.meta.key + "/cloudgemplayeraccount.html") : "external/cloudgemplayeraccount/cloudgemplayeraccount.html";
    }
    public get styleTemplateUrl(): string {
        return this.context.provider.definition.isProd ? this.getSignedUrl(this.context.meta.key + "/cloudgemplayeraccount.css") : "external/cloudgemplayeraccount/cloudgemplayeraccount.css";
    }

    private get searchType(): number {
        let savedType = localStorage.getItem(this.identifier + "searchTypeId");
        let n_SavedType = parseInt(savedType);
        if (savedType === null || savedType === undefined || savedType === "" || n_SavedType === NaN) {
            return 0;
        }
        return n_SavedType;
    }

    private set searchType(value: number) {
        if (value === undefined) {
            return;
        }
        localStorage.setItem(this.identifier + "searchTypeId", value.toString());
    }

    //Required.  Firefox will complain if the constructor is not present with no arguments
    constructor() {
        super();
    }

    public initialize(context: any) {
        super.initialize(context);
        this.apiHandler = new PlayerAccountAPI(this.context.meta.cloudFormationOutputMap["ServiceUrl"].outputValue, this.context.provider.http, this.context.provider.aws);
        this.apiHandler.report(this.tackable.metric);
        this.apiHandler.assign(this.tackable.state);
    }

    public tackable: Tackable = <Tackable>{
        displayName: "Player Account",
        iconSrc: "https://m.media-amazon.com/images/G/01/cloudcanvas/images/player_account._V536715123_.png",
        cost: "Low",
        state: new TackableStatus(),
        metric: new TackableMeasure()
    };

    ngOnInit() {
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

        for (let customAction in this.actionStubActions) {
            this.actionStubActions[customAction].onClick = this.actionStubActions[customAction].onClick.bind(this);
        }
        // end of bind scopes

        this.searchTypes = [
            { text: 'Account Id', functionCb: this.apiHandler.searchAccountId },
            { text: 'Player Name', functionCb: this.apiHandler.searchPlayerName },
            { text: 'Cognito Identity', functionCb: this.apiHandler.searchCognitoId },
            { text: 'Cognito Username', functionCb: this.apiHandler.searchCognitoUsername },
            { text: 'Cognito Email', functionCb: this.apiHandler.searchCognitoEmail }
        ]

        this.currentSearchType = this.searchTypes[0];
        this.list();
    }

    private search(searchResult: SearchResult): void {
        if (searchResult.value === null || searchResult.value.length === 0) {
            this.list();
            return;
        }

        searchResult.value = searchResult.value.trim();
        let handler = null;
        for (let searchType of this.searchTypes) {
            if (searchType.text === searchResult.id) {
                handler = searchType.functionCb;
                break;
            }
        }
        this.mode = Mode.List;
        this.isLoading = true;
        handler(searchResult.value).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (obj.result.Accounts === undefined) {
                this.listModel = [];
                this.listModel.push(obj.result);
            } else {
                this.listModel = obj.result.Accounts;
            }

            this.isLoading = false;
        }, err => {
            toastr.error(err.message);
            this.isLoading = false;
        })
    }

    private containsAccountStatus(entry: PlayerAccountModel, status: string): boolean{
        if (entry === undefined || entry.IdentityProviders === undefined)
            return false;
        return entry.IdentityProviders.Cognito.status.toLowerCase().indexOf(status) != -1
    }

    private list(): void {
        this.mode = Mode.List
        this.isLoading = true;
        this.listModel = [];
        if (this.getAccountsSub) {
            this.getAccountsSub.unsubscribe();
        }
        this.getAccountsSub = this.apiHandler.emptySearch().subscribe(response => {
            let obj = JSON.parse(response.body.text());
            // TODO: This is a temp filtering to get the blacklisted accounts.  Ideally this would be a seperate API or possible search result
            if (this.activeSubMenuIndex === 1) {
                for (let account of obj.result.Accounts) {
                    if (account.AccountBlacklisted) {
                        this.listModel.push(account);
                    }
                }
            } else {
                for (let account of obj.result.Accounts) {
                    if (!account.AccountBlacklisted) {
                        this.listModel.push(account);
                    }
                }
            }
            this.isLoading = false;
        }, err => {
            toastr.clear();
            toastr.error("The player list failed to update with error. " + err.message)
            });
    }

    private show(entry: PlayerAccountModel) {
        this.mode = Mode.Show

        this.actionStubActions = [];
        if (entry.IdentityProviders && entry.IdentityProviders.Cognito.status.toString() === "RESET_REQUIRED") {
            this.actionStubActions.push(new ActionItem("Reset Password", this.changePassword));
        } else if (entry.IdentityProviders && entry.IdentityProviders.Cognito.status !== "FORCE_CHANGE_PASSWORD"
            && entry.IdentityProviders.Cognito.status !== "CONFIRMED" ) {
            this.actionStubActions.push(new ActionItem("Confirm Account", this.confirmAccount));
        }

        this.showModel = entry;
    }


    private onAdd() {
        let playerAccount = new PlayerAccountModel();

        this.addModel = <PlayerAccountEditModel> {
            serialization: <SerializationModel> {
                CognitoUsername: "",
                IdentityProviders: <IdentityModel> {
                  Cognito: <CognitoModel> {
                    gender: "None"
                  }
                }
            },
            validation: new ValidationModel()
        }
        this.mode = Mode.Add;
    }

    private onEdit(model: PlayerAccountModel) {

        this.editModel = <PlayerAccountEditModel> {
            accountId: model.AccountId,
            username: model.CognitoUsername,
            status: model.IdentityProviders ? model.IdentityProviders.Cognito.status : 'ANONYMOUS',
            lastModified: model.IdentityProviders ? model.IdentityProviders.Cognito.last_modified_date : '',
            serialization: <SerializationModel>{
                PlayerName: model.PlayerName
            },
            validation: new ValidationModel()
        }

        if (model.IdentityProviders) {
            this.editModel.serialization.IdentityProviders = <IdentityModel>{
                Cognito: <CognitoModel>{
                    email: model.IdentityProviders.Cognito.email,
                    given_name: model.IdentityProviders.Cognito.given_name,
                    family_name: model.IdentityProviders.Cognito.family_name,
                    locale: model.IdentityProviders.Cognito.locale,
                    nickname: model.IdentityProviders.Cognito.nickname,
                    gender: !model.IdentityProviders.Cognito.gender ? 'None' : model.IdentityProviders.Cognito.gender
                }
            }
        }

        this.mode = Mode.Edit;
    }

    private validate(model, requireUsername): boolean {
        let hasErrors = false;
        if (requireUsername && !model.serialization.CognitoUsername) {
            model.validation.username.isvalid = false;
            hasErrors = true;
        }
        if (model.serialization.IdentityProviders && !model.serialization.IdentityProviders.Cognito.email) {
            model.validation.email.isvalid = false;
            hasErrors = true;
        }
        if (!hasErrors) return true;
    }

    private addAccount() {
        if (this.validate(this.addModel, true)) {
            this.apiHandler.createAccount(this.addModel.serialization).subscribe((response) => {
                this.modalRef.close();
                toastr.clear();
                toastr.success("The player '" + this.addModel.serialization.CognitoUsername + "' has been added");
                this.list();
            }, (err) => {
                toastr.clear();
                toastr.error("Unable to create new account: " + err.message);
                console.error(err);
            })
        }
    }

    private edit(model: PlayerAccountEditModel) {
        if (this.validate(model, false)) {
            if (model.serialization.IdentityProviders && model.serialization.IdentityProviders.Cognito.gender == 'None')
                model.serialization.IdentityProviders.Cognito.gender = ""

            this.apiHandler.editAccount(model.accountId, model.serialization).subscribe((response) => {
                this.modalRef.close();
                toastr.clear();
                toastr.success("The player '" +  model.username + "' has been updated.");
                this.list();
            }, (err) => {
                toastr.clear();
                toastr.error("The player did not save correctly. " + err.message)
            });
        }
    }

    private banAccount() {
        let banPlayerModel = {
            "AccountBlacklisted": true
        }
        this.apiHandler.editAccount(this.currentAccount.AccountId, banPlayerModel).subscribe((response) => {
            toastr.success("The player '" + this.currentAccount.CognitoUsername + "' has been banned");
            this.modalRef.close();
            this.list();
        }, (err) => {
            toastr.clear();
            toastr.error("The player could not be banned. " + err.message)
        });
    }

    private removeBan() {
        let banPlayerModel = {
            "AccountBlacklisted": false
        }
        this.apiHandler.editAccount(this.currentAccount.AccountId, banPlayerModel).subscribe((response) => {
            toastr.success("The player '" + this.currentAccount.CognitoUsername + "'is no longer banned");
            this.modalRef.close();
            this.list();
        }, (err) => {
            toastr.clear();
            toastr.error("Unable to remove the ban. " + err.message)
            console.log(err);
        });
    }

    private changePassword (user: PlayerAccountModel) {
        this.apiHandler.resetUserPassword(user.CognitoUsername).subscribe((response) => {
            toastr.clear();
            toastr.success("Password has been reset");
            user.IdentityProviders.Cognito.status = this.accountStatus.reset_required.toUpperCase();;
        }, (err) => {
            toastr.clear();
            toastr.error("Could not reset password: " + err.message)
            console.log(err);
        });
    }

    private confirmAccount (user: PlayerAccountModel) {
        this.apiHandler.confirmUser(user.CognitoUsername).subscribe((response) => {
            toastr.clear();
            toastr.success("User has been confirmed");
            this.apiHandler.searchAccountId(user.AccountId);
            user.IdentityProviders.Cognito.status = this.accountStatus.confirmed.toUpperCase();
        }, (err) => {
            toastr.clear();
            toastr.error("Could not reset password: " + err.message)
            console.log(err);
        });
    }

    private changeSubMenuIndex(index: number) {
        this.activeSubMenuIndex = index;
        this.list();
    }

    private dismissAddModal() {
      this.mode = Mode.List;
    }

    private dismissEditModal() {
        this.mode = Mode.Show;
    }
}

export function getInstance(context: any): Gemifiable {
    let gem = new PlayerAccount();
    gem.initialize(context);
    return gem;
}

//REST API Handler
export class PlayerAccountAPI extends ApiHandler {
    constructor(serviceBaseURL: string, http: Http, aws: AwsService) {
        super(serviceBaseURL, http, aws);
    }

    public report(metric: Measurable) {
        metric.name = "Registered Player(s)";
        metric.value = "Loading...";
        this.get("admin/identityProviders/Cognito").subscribe(response => {
            let obj = JSON.parse(response.body.text());
            metric.value = obj.result.EstimatedNumberOfUsers.toString();
        })
    }

    public assign(tackableStatus: TackableStatus) {
        tackableStatus.label = "Loading";
        tackableStatus.styleType = "Loading";
        super.get("service/status").subscribe(response => {
            let obj = JSON.parse(response.body.text());
            tackableStatus.label = obj.result.status == "online" ? "Online" : "Offline";
            tackableStatus.styleType = obj.result.status == "online" ? "Enabled" : "Offline";
        }, err => {
            tackableStatus.label = "Offline";
            tackableStatus.styleType = "Offline";
        })
    }

    public searchCognitoEmail(email: string): Observable<any> {
        return super.get("admin/accountSearch?CognitoIdentityId=&CognitoUsername=&Email=" + encodeURIComponent(email) +"&StartPlayerName=");
    }

    public searchCognitoUsername(name: string): Observable<any> {
        return super.get("admin/accountSearch?CognitoIdentityId=&CognitoUsername=" + encodeURIComponent(name)+"&Email=&StartPlayerName=");
    }

    public searchCognitoId(id: string): Observable<any> {
        return super.get("admin/accountSearch?CognitoIdentityId=" + encodeURIComponent(id)+"&CognitoUsername=&Email=&StartPlayerName=");
    }

    public searchPlayerName(name: string): Observable<any> {
        return super.get("admin/accountSearch?CognitoIdentityId=&CognitoUsername=&Email=&StartPlayerName=" + encodeURIComponent(name));
    }

    public searchAccountId(id: string): Observable<any> {
        return super.get("admin/accounts/" + encodeURIComponent(id));
    }

    public searchBannedPlayers(id: string): Observable<any> {
        // TODO: Call search API for banned players here
        return super.get("admin/accounts/" + encodeURIComponent(id));
    }

    public emptySearch(): Observable<any> {
        return super.get("admin/accountSearch");
    }

    public editAccount(id: string, body: any) {
        return super.put("admin/accounts/" + encodeURIComponent(id), body);
    }
    public createAccount(body: any) {
        return super.post("admin/accounts" , body);
    }
    public resetUserPassword(name: string) {
        return super.post("admin/identityProviders/Cognito/users/" + name + "/resetUserPassword");
    }
    public confirmUser(name: string) {
        return super.post("admin/identityProviders/Cognito/users/" + name + "/confirmSignUp");
    }
}
//end rest api handler

//View models
export class ValidationModelEntry {
    public isvalid: boolean = true;
    public message: string = null;

    constructor(isvalid: boolean = true, message: string = null) {
        this.isvalid = isvalid;
        this.message = message;
    }

}

export class ValidationModel {
    public username = new ValidationModelEntry(true, "Username is required");
    public playerName = new ValidationModelEntry();
    public gender = new ValidationModelEntry();
    public locale = new ValidationModelEntry();
    public givenName = new ValidationModelEntry();
    public middleName = new ValidationModelEntry();
    public familyName = new ValidationModelEntry();
    public email = new ValidationModelEntry(true, "Email is required");
    public accountId = new ValidationModelEntry();
    public birthdate = new ValidationModelEntry();
    public phone = new ValidationModelEntry();
    public nickName = new ValidationModelEntry();
   }

export class PlayerAccountEditModel {
    public accountId: string;
    public username: string; //immutable
    public lastModified: number;
    public serialization: SerializationModel;
    public validation: ValidationModel;
}

export class SerializationModel {
    public CognitoUsername?: string;
    public PlayerName?: string;
    public IdentityProviders?: IdentityModel;
}

export class PlayerAccountModel {
    public PlayerName: string;
    public CognitoIdentityId: string;
    public IdentityProviders: IdentityModel;
    public CognitoUsername: string;
    public AccountBlacklisted: boolean;
    public Email: string;
    public AccountId: string;
}

export class IdentityModel {
    public Cognito: CognitoModel;
}

export class CognitoModel {
    public username: string;
    public status: string;
    public create_date: number;
    public last_modified_date: number;
    public enabled: boolean;
    public IdentityProviderId: string;
    public email: string;
    public given_name: string;
    public family_name: string;
    public locale: string;
    public nickname: string;
    public gender: string;
}
//end view models

import { ViewChild, Input, Component, ViewContainerRef } from '@angular/core';
import { FormControl } from '@angular/forms';
import { DateTimeUtil } from 'app/shared/class/index';
import { ModalComponent } from 'app/shared/component/index';
import { ActionItem } from "app/view/game/module/shared/class/index";
import { Subscription } from 'rxjs/Subscription';
import { SearchResult } from 'app/view/game/module/shared/component/index';
import { AbstractCloudGemIndexComponent } from 'app/view/game/module/cloudgems/class/index';
import {
    PlayerAccountApi,
    PlayerAccountThumbnailComponent,
    PlayerAccountEditModel,
    PlayerAccountModel,
    SerializationModel,
    ValidationModel,
    ValidationModelEntry,
    IdentityModel,
    CognitoModel
} from './index'
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { ToastsManager } from 'ng2-toastr';
import { LyMetricService } from 'app/shared/service/index';

export enum Mode {
    List = 0,
    Edit = 1,
    Delete = 2,
    Add = 3,
    Show = 4,
    Ban = 5
}

@Component({
    selector: 'player-account-index',
    templateUrl: 'node_modules/@cloud-gems/cloudgemplayeraccount/index.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemplayeraccount/index.component.css']
})
export class PlayerAccountIndexComponent extends AbstractCloudGemIndexComponent {
    @Input() context: any;
    private _apiHandler: PlayerAccountApi; 
    private activeSubMenuIndex = 0;    
    private searchControl = new FormControl();
    private dateTimeUtil = DateTimeUtil; 
    private isLoading: boolean;
    private mode: Mode;
    private modes: any;
    private addModel: PlayerAccountEditModel;
    private editModel: PlayerAccountEditModel;
    private listModel: PlayerAccountModel[];
    private showModel: PlayerAccountModel;
    private searchTypes: [{ text: string, functionCb: Function, previousPaginationToken?: string, nextPaginationToken?: string }];
    private actionStubActions: ActionItem[];
    private awsCognitoLink = "https://console.aws.amazon.com/cognito/home"
    private currentSearch: any;
    private currentAccount: PlayerAccountModel;
    private getAccountsSub: Subscription;
    // Whether or not to use pagination for the current search parameters.  Right now this is only enabled for Cognito username searches.
    private currentPageIndex: number = 0;
    private enablePagination: boolean = false;
    private previousPaginationToken: string = "";
    private nextPaginationToken: string = "";

    @ViewChild(ModalComponent) modalRef: ModalComponent;

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
    
    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager, vcr: ViewContainerRef, private metric: LyMetricService) {
        super()
    }
    
    ngOnInit() {
        this.mode = Mode.List;
        this.modes = Mode;
        this._apiHandler = new PlayerAccountApi(this.context.ServiceUrl, this.http, this.aws, this.metric, this.context.identifier);        
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
        this._apiHandler.searchAccountId = this._apiHandler.searchAccountId.bind(this._apiHandler);
        this._apiHandler.searchPlayerName = this._apiHandler.searchPlayerName.bind(this._apiHandler);
        this._apiHandler.searchCognitoId = this._apiHandler.searchCognitoId.bind(this._apiHandler);
        this._apiHandler.searchCognitoUsername = this._apiHandler.searchCognitoUsername.bind(this._apiHandler);
        this._apiHandler.searchCognitoEmail = this._apiHandler.searchCognitoEmail.bind(this._apiHandler);
        this._apiHandler.searchBannedPlayers = this._apiHandler.searchBannedPlayers.bind(this._apiHandler);

        for (let customAction in this.actionStubActions) {
            this.actionStubActions[customAction].onClick = this.actionStubActions[customAction].onClick.bind(this);
        }
        // end of bind scopes

        this.searchTypes = [
            { text: 'Account Id', functionCb: this._apiHandler.searchAccountId },
            { text: 'Player Name', functionCb: this._apiHandler.searchPlayerName },
            { text: 'Cognito Identity', functionCb: this._apiHandler.searchCognitoId },
            { text: 'Cognito Username', functionCb: this._apiHandler.searchCognitoUsername },
            { text: 'Cognito Email', functionCb: this._apiHandler.searchCognitoEmail }
        ]

        this.list();
    }

    private get searchType(): number {
        let savedType = localStorage.getItem(this.context.name + "searchTypeId");
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
        localStorage.setItem(this.context.name + "searchTypeId", value.toString());
    }

    private search(searchResult: SearchResult, paginationToken: string): void {
        this.enablePagination = false;
        this.currentSearch = searchResult;
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

        handler(searchResult.value, paginationToken).subscribe(response => {
            let obj = JSON.parse(response.body.text());

            // store pagination tokens if they exist
            this.nextPaginationToken = (obj.result.next) ? obj.result.next : "";
            this.previousPaginationToken = (obj.result.previous) ? obj.result.previous : "";
            this.enablePagination = ( obj.result.previous || obj.result.next) ? true : false;

            if (obj.result.Accounts === undefined) {
                this.listModel = [];
                this.listModel.push(obj.result);
            } else {
                this.listModel = obj.result.Accounts;
            }
            this.isLoading = false;
        }, err => {            
            this.isLoading = false;
            this.list();
        })

    }

    private pageChanged(pageChanged: number) {
        if (this.currentSearch.id === "Player Name") {
            // previous page is -1, next page is 1
            if(pageChanged === -1) {
                this.search(this.currentSearch, this.previousPaginationToken);
                this.currentPageIndex--;
            } else {
                this.search(this.currentSearch, this.nextPaginationToken);
                this.currentPageIndex++;
            }
        }
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
        this.getAccountsSub = this._apiHandler.emptySearch().subscribe(response => {
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
            this.toastr.clearAllToasts();
            this.toastr.error("The player list failed to update with error. " + err.message)
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
            this._apiHandler.createAccount(this.addModel.serialization).subscribe((response) => {
                this.modalRef.close();
                this.toastr.clearAllToasts();
                this.toastr.success("The player '" + this.addModel.serialization.CognitoUsername + "' has been added");
                this.list();
            }, (err) => {
                this.toastr.clearAllToasts();
                this.toastr.error("Unable to create new account: " + err.message);
                console.error(err);
            })
        }
    }

    private edit(model: PlayerAccountEditModel) {
        if (this.validate(model, false)) {
            if (model.serialization.IdentityProviders && model.serialization.IdentityProviders.Cognito.gender == 'None')
                model.serialization.IdentityProviders.Cognito.gender = ""

            this._apiHandler.editAccount(model.accountId, model.serialization).subscribe((response) => {
                this.modalRef.close();
                this.toastr.clearAllToasts();
                this.toastr.success("The player '" +  model.username + "' has been updated.");
                this.list();
            }, (err) => {
                this.toastr.clearAllToasts();
                this.toastr.error("The player did not save correctly. " + err.message)
            });
        }
    }

    private banAccount() {
        let banPlayerModel = {
            "AccountBlacklisted": true
        }
        this._apiHandler.editAccount(this.currentAccount.AccountId, banPlayerModel).subscribe((response) => {
            this.toastr.success("The player '" + this.currentAccount.CognitoUsername + "' has been banned");
            this.modalRef.close();
            this.list();
        }, (err) => {
            this.toastr.clearAllToasts();
            this.toastr.error("The player could not be banned. " + err.message)
        });
    }

    private removeBan() {
        let banPlayerModel = {
            "AccountBlacklisted": false
        }
        this._apiHandler.editAccount(this.currentAccount.AccountId, banPlayerModel).subscribe((response) => {
            this.toastr.success("The player '" + this.currentAccount.CognitoUsername + "'is no longer banned");
            this.modalRef.close();
            this.list();
        }, (err) => {
            this.toastr.clearAllToasts();
            this.toastr.error("Unable to remove the ban. " + err.message)
            console.log(err);
        });
    }

    private changePassword (user: PlayerAccountModel) {
        this._apiHandler.resetUserPassword(user.CognitoUsername).subscribe((response) => {
            this.toastr.clearAllToasts();
            this.toastr.success("Password has been reset");
            user.IdentityProviders.Cognito.status = this.accountStatus.reset_required.toUpperCase();;
        }, (err) => {
            this.toastr.clearAllToasts();
            this.toastr.error("Could not reset password: " + err.message)
            console.log(err);
        });
    }

    private confirmAccount (user: PlayerAccountModel) {
        this._apiHandler.confirmUser(user.CognitoUsername).subscribe((response) => {
            this.toastr.clearAllToasts();
            this.toastr.success("User has been confirmed");
            this._apiHandler.searchAccountId(user.AccountId);
            user.IdentityProviders.Cognito.status = this.accountStatus.confirmed.toUpperCase();
        }, (err) => {
            this.toastr.clearAllToasts();
            this.toastr.error("Could not reset password: " + err.message)
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



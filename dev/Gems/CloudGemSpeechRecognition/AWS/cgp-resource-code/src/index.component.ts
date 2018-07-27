import { Http } from '@angular/http';
import { ViewChild, ElementRef, Input, Component, ViewContainerRef } from '@angular/core';
import { ModalComponent } from 'app/shared/component/index';
import { AwsService } from "app/aws/aws.service";
import { BotEntry, IntentEntry, SlotTypeEntry, SpeechToTextApi } from './index'
import { Subscription } from 'rxjs/rx';
import { AbstractCloudGemIndexComponent } from 'app/view/game/module/cloudgems/class/index';
import { ToastsManager } from 'ng2-toastr/ng2-toastr';
import { PaginationComponent } from 'app/view/game/module/shared/component/index';
import { ActionItem } from "app/view/game/module/shared/class/index";
import { LyMetricService, BreadcrumbService } from 'app/shared/service/index';

export enum Mode {
    Show,
    Publish,
    List,
    Dismiss,
    Delete,
    AddIntentsToSelectedBot,
    Create,
    ShowIntentDependency,
    Build,
    AddIntentToCurrentBot
}

type IntentAndSlotTypeCategory = "Builtin" | "Custom" | "New";

@Component({
    selector: 'speech-to-text-index',
    templateUrl: 'node_modules/@cloud-gems/cloudgemspeechrecognition/index.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemspeechrecognition/index.component.css']
})

export class SpeechToTextIndexComponent extends AbstractCloudGemIndexComponent {
    @Input() context: any;
    private mainPageSubNavActiveIndex = 0;
    private botEditorSubNavItemIndex = 0;
    private sidebarIndex = 0;
    private _apiHandler: SpeechToTextApi;

    // define types
    private mode: Mode;
    private sttModes: any;

    private bots: BotEntry[];
    private botsOnCurrentPage: BotEntry[];
    private currentBot: BotEntry;
    private slotTypesUsedByCurrentBot: SlotTypeEntry[];
    private intentsUsedByCurrentBot: Object[];
    private isLoadingBots: boolean;
    private isLoadingBotVersions: boolean;
    private showBotEditor: boolean = false;
    private selectedBotsNum: number = 0;

    private aliasesOnCurrentPage: Object[];
    private isLoadingBotAliases: boolean;
    private newAlias: Object;

    private builtinSlotTypes: SlotTypeEntry[];
    private customSlotTypes: SlotTypeEntry[];
    private slotTypesOnCurrentPage: SlotTypeEntry[];
    private currentSlotType: SlotTypeEntry;
    private isLoadingSlotTypes: boolean;
    private slotTypeCategory: IntentAndSlotTypeCategory;
    private showSlotTypeEditor: boolean = false;
    private newSlotTypeValue: string;
    private selectedSlotTypesNum: number = 0;

    private builtinIntents: IntentEntry[];
    private customIntents: IntentEntry[];
    private intentsOnCurrentPage: IntentEntry[];
    private currentIntent: IntentEntry;
    private newIntent: IntentEntry;
    private currentSlot: Object;
    private isLoadingIntents: boolean;
    private isLoadingIntentDependency: boolean;
    private intentCategory: IntentAndSlotTypeCategory;
    private intentDependency: BotEntry[];
    private showIntentEditor: boolean = false;
    private newIntentName: Object;
    private selectedIntentsNum: number = 0;

    private exportAction = new ActionItem("Export", this.export);
    private publishAction = new ActionItem("Publish", this.publishModal);
    private buildAction = new ActionItem("Build", this.buildModal);
    private botActions: ActionItem[] = [
        this.buildAction,
        this.publishAction,
        this.exportAction];

    private pageSize: number = 10;
    private Math: any;
    private botPages: number;
    private slotTypePages: number;
    private intentPages: number;
    private aliasPages: number;
    private botPageIndex: number;
    private intentPageIndex: number;
    private slotTypePageIndex: number;
    private aliasPageIndex: number;

    private createBotTip: string;
    private publishBotTip: string;

    private sortDir: string;

    private createModalType: string;


    @ViewChild(ModalComponent) modalRef: ModalComponent;
    @ViewChild(PaginationComponent) paginationRef: PaginationComponent;

    @ViewChild('upload') uploadButtonRef: ElementRef;
    @ViewChild('selectAllBots') selectAllBotsRef: ElementRef;
    @ViewChild('selectAllIntents') selectAllIntentsRef: ElementRef;
    @ViewChild('selectAllSlotTypes') selectAllSlotTypesRef: ElementRef;
    @ViewChild('consoleLink') consoleLinkRef: ElementRef;

    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager, vcr: ViewContainerRef, private metric: LyMetricService, private breadcrumbs: BreadcrumbService) {
        super()
        this.toastr.setRootViewContainerRef(vcr);
    }

    /**
     * Initialize values
	**/
    ngOnInit() {
        this._apiHandler = new SpeechToTextApi(this.context.ServiceUrl, this.http, this.aws, this.metric, this.context.identifier);

        // bind proper scopes for callbacks that have a this reference
        this.dismissModal = this.dismissModal.bind(this);
        this.publishModal = this.publishModal.bind(this);
        this.buildModal = this.buildModal.bind(this);
        this.deleteModal = this.deleteModal.bind(this);
        this.createModal = this.createModal.bind(this);
        this.addIntentsToSelectedBotModal = this.addIntentsToSelectedBotModal.bind(this);
        this.addIntentToCurrentBotModal = this.addIntentToCurrentBotModal.bind(this);
        this.showIntentDependencyModal = this.showIntentDependencyModal.bind(this);

        this.exportAction.onClick = this.exportAction.onClick.bind(this);
        this.publishAction.onClick = this.publishAction.onClick.bind(this);
        this.buildAction.onClick = this.buildAction.onClick.bind(this);
        // end of bind scopes

        this.sttModes = Mode;
        this.sortDir == "";
        this.bots = [];
        this.createBotTip = "Bots can be imported from a JSON file. Information on creating the file can be found here: https://github.com/awslabs/aws-lex-web-ui/tree/master/templates.";
        this.publishBotTip = "It could take several minutes to publish the bot.";
        this.newAlias = { name: "", botName: "", botVersion: "" };
        this.currentIntent = this.defaultIntent();
        this.mode = Mode.List;
    }

    /**
     * Validate the bot properties
	 * @param bot the bot to validate
	**/
    protected validateBot(bot): boolean {
        let isValid: boolean = true;       
        bot.idleSessionTTLInSeconds.valid = false;
        if (bot.idleSessionTTLInSeconds.value && bot.idleSessionTTLInSeconds.value < 60) {
            bot.idleSessionTTLInSeconds.message = "The session timeout cannot be shorter then 60s";
        }
        else {
            bot.idleSessionTTLInSeconds.valid = true;
        }
        isValid = isValid && bot.idleSessionTTLInSeconds.valid;

        let maxChar = 50;
        bot.name.valid = false;
        if (!bot.name.value || !/\S/.test(bot.name.value)) {
            bot.name.message = "The name was invalid";
        } else if (bot.name.value.length < 2) {
            bot.name.message = "The name was not long enough."
        } else if (bot.name.value.length > 50) {
            bot.name.message = "The name was too long. We counted " + bot.name.value.length + " characters.  The limit is " + maxChar + " characters."
        } else if (bot.name.value.indexOf(" ") > -1) {
            bot.name.message = "The name was invalid";
        } else {
            bot.name.valid = true;
        }
        isValid = isValid && bot.name.valid;

        bot.childDirected.valid = false;
        if (bot.childDirected.value == undefined) {
            bot.childDirected.message = "The COPPA was invalid";
        }
        else {
            bot.childDirected.valid = true;
        }
        isValid = isValid && bot.childDirected.valid;

        return isValid;
    }

    /**
     * Validate the bot timeout property
     * @param bot the bot to validate
    **/
    protected validateTimeout(bot): boolean {
        let isValid: boolean = true;
        bot.idleSessionTTLInSeconds.valid = false;
        if (bot.idleSessionTTLInSeconds.value && bot.idleSessionTTLInSeconds.value < 60) {
            bot.idleSessionTTLInSeconds.message = "The session timeout cannot be shorter then 60s";
        }
        else {
            bot.idleSessionTTLInSeconds.valid = true;
        }
        isValid = isValid && bot.idleSessionTTLInSeconds.valid;

        return isValid;
    }

    /**
     * Validate the intent properties
	 * @param intent the intent to validate
	**/
    protected validateIntent(intent): boolean {
        let isValid: boolean = true;
        let maxChar = 100;

        intent.name.valid = false;
        if (!intent.name.value || !/\S/.test(intent.name.value)) {
            intent.name.message = "The name was invalid";
        } else if (intent.name.value.length < 1) {
            intent.name.message = "The name was not long enough."
        } else if (intent.name.value.length > 100) {
            intent.name.message = "The name was too long. We counted " + intent.name.value.length + " characters.  The limit is " + maxChar + " characters."
        } else if (intent.name.value.indexOf(" ") > -1) {
            intent.name.message = "The name was invalid";
        } else {
            intent.name.valid = true;
        }
        isValid = isValid && intent.name.valid;

        return isValid;
    }

    /**
     * Validate the new builtin intent name
	**/
    protected validateNewBuiltinIntentName(): boolean {
        let maxChar = 100;
        this.newIntentName["valid"] = false;
        if (!this.newIntentName["value"] || !/\S/.test(this.newIntentName["value"])) {
            this.newIntent["message"] = "The name was invalid";
        } else if (this.newIntentName["value"].length < 1) {
            this.newIntentName["message"] = "The name was not long enough."
        } else if (this.newIntentName["value"].length > 100) {
            this.newIntentName["message"] = "The name was too long. We counted " + this.newIntentName["value"].length + " characters.  The limit is " + maxChar + " characters."
        } else if (this.newIntentName["value"].indexOf(" ") > -1) {
            this.newIntentName["message"] = "The name was invalid";
        } else {
            this.newIntentName["valid"] = true;
        }

        return this.newIntentName["valid"];
    }

    /**
     * Validate the slot type properties
	 * @param slotType the slot type to validate
	**/
    protected validateSlotType(slotType): boolean {
        let isValid: boolean = true;
        let maxChar = 100;

        slotType.name.valid = false;
        if (!slotType.name.value || !/\S/.test(slotType.name.value)) {
            slotType.name.message = "The name was invalid";
        } else if (slotType.name.value.length < 1) {
            slotType.name.message = "The name was not long enough."
        } else if (slotType.name.value.length > 100) {
            slotType.name.message = "The name was too long. We counted " + slotType.name.value.length + " characters.  The limit is " + maxChar + " characters."
        } else if (slotType.name.value.indexOf(" ") > -1) {
            slotType.name.message = "The name was invalid";
        } else {
            slotType.name.valid = true;
        }
        isValid = isValid && slotType.name.valid;

        slotType.enumerationValues.valid = false;
        if (!slotType.enumerationValues.value) {
            slotType.enumerationValues.message = "The value was invalid";
        } else if (slotType.enumerationValues.value.length < 1) {
            slotType.enumerationValues.message = "No value is provided."
        }
        else {
            slotType.enumerationValues.valid = true;
        }
        isValid = isValid && slotType.enumerationValues.valid;

        return isValid;
    }

    /**
     * Update the table content on the current tab
	**/
    public update(): void {
        //Empty string gets passed as no value causing API to fail.
        if (this.mainPageSubNavActiveIndex == 0) {
            this.bots = [];
            this.updateBots("t");
        }
        else if (this.mainPageSubNavActiveIndex == 1) {
            this.intentCategory = "Custom";
            this.updateCustomIntents("t");
        }
        else if (this.mainPageSubNavActiveIndex == 2) {
            this.slotTypeCategory = "Custom";
            this.updateCustomSlotTypes("t");
        }
    }

    /**
     * Open the bot editor to edit the current bot
	 * @param bot the bot to edit
	**/
    public showBot(bot: BotEntry): void {
        if (this.modalRef) {
            this.modalRef.close();
        }
        this.updateBuiltinSlotTypes("t");
        this.currentBot = new BotEntry(bot, this.toastr, this._apiHandler);
        this.breadcrumbs.addBreadcrumb(this.currentBot.name, null);
        this.getBotEditorContent();
    }

    /**
     * Export the bot
     * @param bot the bot to export
	**/
    public export(bot: BotEntry): void {
        bot.export();
    }

    /**
     * Build the current bot
	**/
    public build(): void {
        this.modalRef.close();
        this.mode = Mode.List;
        this.currentBot.build();
    }

    /**
     * Publish the current bot
	**/
    public publish(): void {
        this.modalRef.close();
        this.mode = Mode.List;
        this.currentBot.publish(this.newAlias["name"]);
    }

    /**
     * Initialize the content shown in the bot editor
	**/
    private getBotEditorContent(): void {
        this.currentBot.get(this.currentBot.version).then(function (bot) {
            this.currentBot = new BotEntry(bot, this.toastr, this._apiHandler);
            this.intentsUsedByCurrentBot = this.currentBot["intents"] ? this.currentBot["intents"] : [];
            this.slotTypesUsedByCurrentBot = [];
            this.switchSideBarTabs(0);
            this.showBotEditor = true;
            if (this.intentsUsedByCurrentBot.length == 0) {
                this.addIntentToCurrentBotModal();
            }
        }.bind(this))
    }

    /**
     * Switch to another tab of the side bar
     * @param index the index of the side bar to switch to
	**/
    public switchSideBarTabs(index): void {
        this.sidebarIndex = index;
        if (this.botEditorSubNavItemIndex == 0) {
            if (index >= 0 && index < this.intentsUsedByCurrentBot.length) {
                this.currentIntent = this.defaultIntent();
                this.currentIntent["name"] = this.intentsUsedByCurrentBot[index]["intentName"];
                this.currentIntent["version"] = this.intentsUsedByCurrentBot[index]["intentVersion"];
                this.getSlotTypesUsedByCurrentIntent(this.currentIntent);
            }
            else if (index < this.intentsUsedByCurrentBot.length + this.slotTypesUsedByCurrentBot.length) {
                this.currentSlotType = this.slotTypesUsedByCurrentBot[index - this.intentsUsedByCurrentBot.length];
            }
        }
        else {
            if (this.sidebarIndex == 0) {
                this.updateBotAliases("e");
            }
        }
    }

    /**
     * Select an existing custom intent in the AddIntentToCurrentBot modal
     * @param intent the existing custom intent to add
	**/
    public selectCustomIntent(intent): void {
        intent.isSelected = !intent.isSelected;
        for (let customIntent of this.customIntents) {
            customIntent.isSelected = customIntent['name'] == intent['name'] ? customIntent.isSelected : false;
        }
        this.intentCategory = "Custom";
    }

    /**
     * Select a built-in intent in the AddIntentToCurrentBot modal
     * @param intent the built-in intent to add
	**/
    public selectBuiltinIntent(intent): void {
        this.newIntent = intent;
        this.newIntentName = { name: "", valid: true };
        this.intentCategory = "Builtin";
    }

    /**
     * Select to create a new intent in the AddIntentToCurrentBot modal
	**/
    public selectNewIntent(): void {
        this.newIntent = this.defaultIntent();
        this.intentCategory = "New";
    }

    /**
     * Add the selected intent(s) to the current bot
	**/
    public addIntentsToCurrentBot(): void {
        if (this.currentBot.created) {
            this.modalRef.close();
            this.mode = Mode.List;
            this.currentBot.get("$LATEST").then(function (bot) {
                this.currentBot = bot;
                if (this.intentCategory == "Custom") {
                    this.addExistingCustomIntentsToBot(this.customIntents, this.currentBot);
                }
                else if (this.intentCategory == "Builtin") {
                    this.addBuiltinIntentToBot(this.newIntent, this.currentBot);
                }
            }.bind(this))
        }
        else if (this.intentCategory == "Builtin") {
            if (this.validateNewBuiltinIntentName()) {
                this.modalRef.close();
                this.mode = Mode.List;
                this.addBuiltinIntentToBot(this.newIntent, this.currentBot);
            }
        }
        else if (this.intentCategory == "New") {
            if (this.validateIntent(this.newIntent)) {
                this.modalRef.close();
                this.mode = Mode.List;
                this.addNewCustomIntentToBot(this.currentBot);
            }
        }
        else if (this.intentCategory == "Custom") {
            this.addExistingCustomIntentsToBot(this.customIntents, this.currentBot);
        }
    }

    /**
     * Add a built-in intent to the selected bot
     * @param intent the built-in intent to add
     * @param bot the bot to accept the new intent
	**/
    public addBuiltinIntentToBot(intent, bot: BotEntry): void {
        this._apiHandler.getBuiltinIntent(intent.name).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (!obj.result.intent.error) {
                obj.result.intent.name = { valid: true, value: this.newIntentName["value"] }
                this.newIntent = new IntentEntry(obj.result.intent, this.toastr, this._apiHandler);
                this.newIntent.parentIntentSignature = intent.name;
                this.newIntent.fulfillmentActivity = { 'type': "ReturnIntent" };
                this.newIntentName = { valid: true, value: "" };
                this.addNewCustomIntentToBot(bot);
            }
            else {
                this.toastr.error("The intent '" + intent.name + "' was not added to bot properly. " + obj.result.intent.error);
            }
        }, err => {
            this.toastr.error("The intent '" + intent.name + "' was not added to bot properly. " + err.message);
        });
    }

    /**
     * Add an existing custom intent to the selected bot
     * @param intent the existing custom intent to add
     * @param bot the bot to accept the new intent
	**/
    public addExistingCustomIntentsToBot(intents, bot: BotEntry): void {
        if (this.modalRef) {
            this.modalRef.close();
            this.mode = Mode.List;
        }
        if (!this.currentBot["intents"]) {
            this.currentBot["intents"] = [];
            this.intentsUsedByCurrentBot = this.currentBot["intents"];
        }

        this.currentBot.get("$LATEST").then(function (latestBot) {
            for (let intent of intents) {
                if (intent.isSelected) {
                    latestBot["intents"] = latestBot["intents"] ? latestBot["intents"] : [];
                    this.currentIntent = latestBot["intents"].length == 0 ? intent : this.currentIntent;
                    latestBot["intents"].push({ intentName: intent.name, intentVersion: intent.version });
                    this.currentBot["intents"].push({ intentName: intent.name, intentVersion: intent.version });
                    intent.isSelected = false;
                    if (this.currentBot["intents"].length == 1) {
                        this.switchSideBarTabs(0);
                    }
                }
            }

            if (latestBot["aliases"]) {
                delete latestBot["aliases"];
            }
            latestBot.save("SAVE");
        }.bind(this))
    }

    /**
     * Check whether the intent exsits in the current bot
     * @param intent the intent to check
     * @return true if the current bot has the intent
	**/
    public intentExistsInCurrentBot(intent): boolean {
        if (!this.currentBot["intents"]) {
            return false;
        }

        for (let existingIntent of this.currentBot["intents"]) {
            if (existingIntent["intentName"] == intent.name) {
                return true;
            }
        }
        return false;
    }

     /**
     * Remove an intent from the current bot
     * @param intent the intent to remove
     * @param index the intent index in the list
	**/
    public removeIntentFromBot(intent, index): void {
        if (this.currentBot.intents.length == 1) {
            this.toastr.error("Cannot remove '" + intent.intentName + "'. A bot must have at least one Intent.");
            return;
        }

        this.currentBot.get("$LATEST").then(function (bot) {
            for (let i = 0; i < bot["intents"].length; i++) {
                if (bot["intents"][i]["intentName"] == intent["intentName"]) {
                    bot["intents"].splice(i, 1);
                    break;
                }
            }
            bot.save("SAVE").then(function () {
                this.removeIntentAndSlotTypesFromSidebar(intent, index);
            }.bind(this));
        }.bind(this))
    }

    /**
     * Get a list of aliases for a specified Amazon Lex bot
     * @param PageToken A pagination token for fetching the next page of aliases. If the response to this call is truncated, Amazon Lex returns a pagination token in the response.
	**/
    public updateBotAliases(PageToken: string): void {
        this.sortDir = "asc";
        this.isLoadingBotAliases = true;
        this._apiHandler.getBotAliases(this.currentBot.name, PageToken).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (!obj.result.aliases.error) {
                this.currentBot["aliases"] = PageToken.length <= 1 ? obj.result.aliases : this.currentBot["aliases"].concat(obj.result.aliases);
                if (obj.result.nextToken != "") {
                    this.updateBotAliases(obj.result.nextToken);
                }
                else {
                    this.sort(this.currentBot["aliases"], "name", this.sortDir);
                    this.updatePaginationInfo();
                    this.updateCurrentPage(1);
                    this.updateCurrentBotVersions("e");
                    this.isLoadingBotAliases = false;
                }
            }
            else {
                this.toastr.error("The bots did not refresh properly. " + obj.result.aliases.error);
                this.isLoadingBotAliases = false;
            }
        }, err => {
            this.toastr.error("The bots did not refresh properly. " + err.message);
            this.isLoadingBotAliases = false;
        });
    }

    /**
     * Add a new alias to the current bot
	**/
    public addBotAlias(): void {
        this.currentBot.saveAlias(this.newAlias).then(function () {
            this.currentBot["aliases"].unshift(this.newAlias);
            this.updatePaginationInfo();
            this.updateCurrentPage(1);
            this.newAlias = { name: "", botName: "", botVersion: "" };
        }.bind(this));
    }

    /**
     * Delete an alias from the current bot
     * @param alias the alias to delete
	**/
    public deleteBotAlias(alias): void {
        this.currentBot.deleteAlias(alias).then(function () {
            let index = this.currentBot["aliases"].indexOf(alias, 0);
            if (index > -1) {
                this.currentBot["aliases"].splice(index, 1);
            }
            this.updatePaginationInfo();
            this.updateCurrentPage(1);
        }.bind(this))
    }

    /**
     * Save the intent changes to the current bot
     * @param $event the intent that contain the changes
	**/
    public saveIntentChangesToCurrentBot($event: IntentEntry): void {
        this.currentIntent = $event;
        this.currentBot.get("$LATEST").then(function (bot) {
            for (let intent of bot.intents) {
                if (intent["intentName"] = $event.name) {
                    this.getSlotTypesUsedByCurrentIntent($event);
                    intent["intentVersion"] = $event.version;
                    bot.save("SAVE");
                    break;
                }
            }
        }.bind(this))
    }

    /**
     * Gets a list of built-in intents
     * @param PageToken A pagination token that fetches the next page of intents. If this API call is truncated, Amazon Lex returns a pagination token in the response.
	**/
    public updateBuiltinIntents(PageToken: string): void {
        this.sortDir = "asc";
        this.isLoadingIntents = true;
        this._apiHandler.getBuiltinIntents(encodeURIComponent(PageToken)).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (!obj.result.intents.error) {
                this.builtinIntents = PageToken.length <= 1 ? obj.result.intents : this.builtinIntents.concat(obj.result.intents);
                if (obj.result.nextToken != "") {
                    this.updateBuiltinIntents(obj.result.nextToken);
                }
                else {
                    for (let intent of this.builtinIntents) {
                        this.initializeIntent(intent);
                    }
                    this.sort(this.builtinIntents, "name", this.sortDir);
                    this.updatePaginationInfo();
                    this.updateCurrentPage(1);
                    this.isLoadingIntents = false;
                }
            }
            else {
                this.toastr.error("The slot types did not refresh properly. " + obj.result.intents.error);
                this.isLoadingIntents = false;
            }
        }, err => {
            this.toastr.error("The slot types did not refresh properly. " + err.message);
            this.isLoadingIntents = false;
        });
    }

    /**
     * Gets a list of custom intents
     * @param PageToken A pagination token that fetches the next page of intents. If this API call is truncated, Amazon Lex returns a pagination token in the response.
	**/
    public updateCustomIntents(PageToken: string): void {
        this.sortDir = "asc";
        this.isLoadingIntents = true;
        this._apiHandler.getCustomIntents(encodeURIComponent(PageToken)).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (!obj.result.intents.error) {
                this.customIntents = PageToken.length <= 1 ? [] : this.customIntents;
                for (let intent of obj.result.intents) {
                    let newIntent = new IntentEntry(intent, this.toastr, this._apiHandler);
                    this.customIntents.push(newIntent);
                }
                if (obj.result.nextToken != "") {
                    this.updateCustomIntents(obj.result.nextToken);
                }
                else {
                    for (let intent of this.customIntents) {
                        this.initializeIntent(intent);
                    }
                    this.sort(this.customIntents, "name", this.sortDir);
                    this.updatePaginationInfo();
                    this.updateCurrentPage(1);
                    this.isLoadingIntents = false;
                }
            }
            else {
                this.toastr.error("The slot types did not refresh properly. " + obj.result.intents.error);
                this.isLoadingIntents = false;
            }
        }, err => {
            this.toastr.error("The slot types did not refresh properly. " + err.message);
            this.isLoadingIntents = false;
        });
    }

    /**
     * Open the intent editor to edit the current intent
     * @param intent the intent to edit
	**/
    public showIntent(intent: IntentEntry): void {
        if (this.intentCategory == "Builtin") {
            return;
        }

        this.currentIntent = new IntentEntry(intent, this.toastr, this._apiHandler);;
        this.breadcrumbs.addBreadcrumb(this.currentIntent.name, null);
        this.showIntentEditor = true;
    }

    /**
     * Change the category of the intents to list
	**/
    public switchCategory(): void {
        if (this.mainPageSubNavActiveIndex == 1) {
            this.selectedIntentsNum = 0;
            if (this.intentCategory == "Custom") {
                this.updateCustomIntents("t");
            }
            else if (this.intentCategory == "Builtin") {
                this.updateBuiltinIntents("t");
            }
        }
        else if (this.mainPageSubNavActiveIndex == 2) {
            this.selectedSlotTypesNum = 0;
            if (this.slotTypeCategory == "Custom") {
                this.updateCustomSlotTypes("t");
            }
            else if (this.slotTypeCategory == "Builtin") {
                this.updateBuiltinSlotTypes("t");
            }
        }
    }

    /**
     * Gets a list of built-in slot types
     * @param PageToken A pagination token that fetches the next page of slot types. If this API call is truncated, Amazon Lex returns a pagination token in the response.
	**/
    public updateBuiltinSlotTypes(PageToken: string): void {
        this.sortDir = "asc";
        this.isLoadingSlotTypes = true;
        this._apiHandler.getBuiltinSlotTypes(encodeURIComponent(PageToken)).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (!obj.result.slotTypes.error) {
                this.builtinSlotTypes = PageToken.length <= 1 ? obj.result.slotTypes : this.builtinSlotTypes.concat(obj.result.slotTypes);
                if (obj.result.nextToken != "") {
                    this.updateBuiltinSlotTypes(obj.result.nextToken);
                }
                else {
                    for (let slotType of this.builtinSlotTypes) {
                        this.initializeSlotType(slotType);
                    }
                    this.sort(this.builtinSlotTypes, "name", this.sortDir);
                    this.updatePaginationInfo();
                    this.updateCurrentPage(1);
                    this.isLoadingSlotTypes = false;
                }
            }
            else {
                this.toastr.error("The builtin slot types did not refresh properly. " + obj.result.slotTypes.error);
                this.isLoadingSlotTypes = false;
            }
        }, err => {
            this.toastr.error("The builtin slot types did not refresh properly. " + err.message);
            this.isLoadingSlotTypes = false;
        });
    }

    /**
     * Gets a list of custom slot types
     * @param PageToken A pagination token that fetches the next page of slot types. If this API call is truncated, Amazon Lex returns a pagination token in the response.
	**/
    public updateCustomSlotTypes(PageToken: string): void {
        this.sortDir = "asc";
        this.isLoadingSlotTypes = true;
        this._apiHandler.getCustomSlotTypes(encodeURIComponent(PageToken)).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (!obj.result.slotTypes.error) {
                this.customSlotTypes = PageToken.length <= 1 ? [] : this.customSlotTypes;
                for (let slotType of obj.result.slotTypes) {
                    let newSlotType = new SlotTypeEntry(slotType, this.toastr, this._apiHandler);
                    this.customSlotTypes.push(newSlotType);
                }
                if (obj.result.nextToken != "") {
                    this.updateCustomSlotTypes(obj.result.nextToken);
                }
                else {
                    for (let slotType of this.customSlotTypes) {
                        this.initializeSlotType(slotType);
                    }
                    this.sort(this.customSlotTypes, "name", this.sortDir);
                    this.updatePaginationInfo();
                    this.updateCurrentPage(1);
                    this.isLoadingSlotTypes = false;
                }
            }
            else {
                this.toastr.error("The custom slot types did not refresh properly. " + obj.result.slotTypes.error);
                this.isLoadingSlotTypes = false;
            }
        }, err => {
            this.toastr.error("The custom slot types did not refresh properly. " + err.message);
            this.isLoadingSlotTypes = false;
        });
    }

    /**
     * Open the slot type editor to edit the current slot type
     * @param slotType the slot type to edit
	**/
    public showSlotType(slotType: SlotTypeEntry): void {
        if (this.slotTypeCategory == "Builtin") {
            return;
        }
        this.currentSlotType = new SlotTypeEntry(slotType, this.toastr, this._apiHandler);
        this.breadcrumbs.addBreadcrumb(this.currentSlotType.name, null);
        this.showSlotTypeEditor = true;
    }

    /**
     * Define the Create modal
     * @param type the type of the model to create
	**/
    public createModal(type): void {
        this.mode = Mode.Create;
        this.createModalType = type;
        if (type == "bot") {
            this.currentBot = this.defaultBot();
        }
        else if (type == "intent") {
            this.currentIntent = this.defaultIntent();
        }
        else if (type == "slotType") {
            this.currentSlotType = this.defaultSlotType();
        }
    }

    /**
     * Define the Build modal
     * @param bot the bot to build
	**/
    public buildModal(bot): void {
        this.mode = Mode.Build;
        this.currentBot = bot;
    }

    /**
     * Define the Publish modal
     * @param bot the bot to publish
	**/
    public publishModal(bot): void {
        this.mode = Mode.Publish;
        this.currentBot = bot;
        this.newAlias["name"] = "";
    }

    /**
     * Define the Dismiss modal
     * @param bot the bot to edit
	**/
    public dismissModal(bot): void {
        this.mode = Mode.List;
    }

    /**
     * Define the Delete modal
     * @param model the model to delete
	**/
    public deleteModal(model): void {
        this.mode = Mode.Delete;
        if (this.mainPageSubNavActiveIndex == 0) {
            this.currentBot = model;
        }
        else if (this.mainPageSubNavActiveIndex == 1) {
            this.currentIntent = model;
        }
        else if (this.mainPageSubNavActiveIndex == 2) {
            this.currentSlotType = model;
        }
    }

    /**
     * Define the AddIntentsToSelectedBot modal
	**/
    public addIntentsToSelectedBotModal(): void {
        this.updateBots('t');
        if (this.intentCategory == "Builtin") {
            this.newIntentName = { name: "", valid: true };
        }
        this.mode = Mode.AddIntentsToSelectedBot;
    }

    /**
     * Define the AddIntentToCurrentBot modal
	**/
    public addIntentToCurrentBotModal(): void {
        if (!this.builtinIntents || this.builtinIntents.length == 0) {
            this.updateBuiltinIntents('e');
        }
        else {
            for (let builtinIntent of this.builtinIntents) {
                builtinIntent.isSelected = false;
            }
        }
        if (!this.customIntents || this.customIntents.length == 0) {
            this.updateCustomIntents('e');
        }
        else {
            for (let customIntent of this.customIntents) {
                customIntent.isSelected = false;
            }
        }
        this.intentCategory = "Custom";
        this.mode = Mode.AddIntentToCurrentBot;
    }

    /**
     * Define the ShowIntentDependency modal
     * @param intent the intent to show the dependency
	**/
    public showIntentDependencyModal(intent): void {
        this.currentIntent = intent;
        this.mode = Mode.ShowIntentDependency;
        this.isLoadingIntentDependency = true;
        this.currentIntent.getDependency().then(function (intentDependency) {
            this.intentDependency = intentDependency;
            this.isLoadingIntentDependency = false;
        }.bind(this), function () {
            this.isLoadingIntentDependency = false;
        }.bind(this))
    }

    /**
     * Get the title of the Create modal
	**/
    public createModalTitle(): string {
        if (this.createModalType == "bot") {
            return "Create Bot";
        }
        else if (this.createModalType == "intent") {
            return "Create Intent";
        }
        else if (this.createModalType == "slotType") {
            return "Create Slot Type";
        }
    }

    /**
     * Create a new model
	**/
    public create(): void {  
        if (this.createModalType == "bot") {
            if (this.validateBot(this.currentBot)) {
                this.modalRef.close();
                this.mode = Mode.List;
                this.currentBot.name = this.currentBot["name"]["value"];
                this.currentBot.idleSessionTTLInSeconds = this.currentBot["idleSessionTTLInSeconds"]["value"];
                this.currentBot.childDirected = this.currentBot["childDirected"]["value"];
                this.currentBot.save("SAVE").then(function () {
                    this.update();
                }.bind(this));
            }
        }
        else if (this.createModalType == "intent") {
            if (this.validateIntent(this.currentIntent)) {
                this.modalRef.close();
                this.mode = Mode.List;
                this.currentIntent.name = this.currentIntent["name"]["value"];
                this.currentIntent.save().then(function () {
                    this.update();
                }.bind(this));
            }
        }
        else if (this.createModalType == "slotType") {
            if (this.validateSlotType(this.currentSlotType)) {
                this.modalRef.close();
                this.mode = Mode.List;
                this.currentSlotType.name = this.currentSlotType["name"]["value"];
                this.currentSlotType.enumerationValues = this.currentSlotType["enumerationValues"]["value"];
                this.currentSlotType.save().then(function () {
                    this.update();
                }.bind(this));
            }
        }   
    }

    /**
     * Get the title of the Delete modal
	**/
    public deleteModalTitle(): string {
        if (this.mainPageSubNavActiveIndex == 0) {
            return "Delete Bot";
        }
        else if (this.mainPageSubNavActiveIndex == 1) {
            return "Delete Intent";
        }
        else if (this.mainPageSubNavActiveIndex == 2) {
            return "Delete Slot Type";
        }
    }

    /**
     * Delete the current model
	**/
    public delete(): void {
        this.modalRef.close();
        this.mode = Mode.List;
        if (this.mainPageSubNavActiveIndex == 0) {
            this.deleteBot(this.currentBot);
        }
        else if (this.mainPageSubNavActiveIndex == 1) {
            this.deleteIntent(this.currentIntent);
        }
        else if (this.mainPageSubNavActiveIndex == 2) {
            this.deleteSlotType(this.currentSlotType);
        }
    }

    /**
     * Update the list of models shown on the current page
	**/
    public updateCurrentPage(pageIndex): void {
        let startIndex = (pageIndex - 1) * this.pageSize;
        let endIndex = pageIndex * this.pageSize;
        if (this.mainPageSubNavActiveIndex == 0) {
            if (!this.showBotEditor) {
                this.botPageIndex = pageIndex;
                this.botsOnCurrentPage = this.bots.slice(startIndex, endIndex);
            }
            else if (this.botEditorSubNavItemIndex == 1 && this.sidebarIndex == 0) {
                this.aliasPageIndex = pageIndex;
                this.aliasesOnCurrentPage = this.currentBot["aliases"].slice(startIndex, endIndex);
            }

        }
        else if (this.mainPageSubNavActiveIndex == 1) {
            this.intentPageIndex = pageIndex;
            this.intentsOnCurrentPage = this.intentCategory == "Builtin" ? this.builtinIntents.slice(startIndex, endIndex) : this.customIntents.slice(startIndex, endIndex);
        }
        else if (this.mainPageSubNavActiveIndex == 2) {
            this.slotTypePageIndex = pageIndex;
            this.slotTypesOnCurrentPage = this.slotTypeCategory == "Builtin" ? this.builtinSlotTypes.slice(startIndex, endIndex) : this.customSlotTypes.slice(startIndex, endIndex);
        }
    }

    /**
     * Click the hidden upload button to upload a local file
	**/
    public clickUploadButton(): void {
        this.uploadButtonRef.nativeElement.click();
    }

    public uploadBotDefinition($event): void {
        let inputValue = $event.target;
        let file = inputValue.files[0];
        let fileReader = new FileReader();
        fileReader.onload = function (e) {
            let result = e.target["result"];
            let newBot = JSON.parse(result)["bot"];
            let body = {
                "desc_file": JSON.parse(result)
            }
            this._apiHandler.putDesc(body).subscribe(response => {
                let obj = JSON.parse(response.body.text());
                if (obj.result.status == "ACCEPTED") {
                    this.toastr.success("The bot '" + newBot.name + "' was uploaded successfully.");
                    this.update();
                }
                else {
                    this.toastr.error(obj.result.status);
                }
            }, (err) => {
                this.toastr.error("The bot '" + newBot.name + "' was not uploaded properly. " + err.message);
            });
        }.bind(this)
        fileReader.readAsText(file);
        inputValue.value = "";
    }

    /**
     * Open the AWS console for advanced settings
	**/
    public openAWSConsole(): void {
        this.consoleLinkRef.nativeElement.href = "https://console.aws.amazon.com/lex/home?region" + this.aws.context.region;
    }

    /**
     * Sort the table based on the name column
	**/
    public sortTable(): void {
        let table = [];
        if (this.mainPageSubNavActiveIndex == 0) {
            if (!this.showBotEditor) {
                table = this.bots;
            }
        }
        else if (this.mainPageSubNavActiveIndex == 1) {
            table = this.intentCategory == 'Custom' ? this.customIntents : this.builtinIntents;
        }
        else if (this.mainPageSubNavActiveIndex == 2) {
            table = this.slotTypeCategory == 'Custom' ? this.customSlotTypes : this.builtinSlotTypes;
        }

        this.sort(table, "name", this.getSortOrder());
        this.paginationRef.reset();
    }

    /**
     * Show all the versions of the selected model
     * @param model the model to show all the versions
     * @param index the index of the model in the current table
	**/
    public expandAllVersions(model, index): void {
        if (model.isLoading) {
            return;
        }

        if (!model.expandAllVersions) {
            if (this.mainPageSubNavActiveIndex == 0) {
                this.expandBotVersions(model, index + 1);
            }
            else if (this.mainPageSubNavActiveIndex == 1) {
                this.expandIntentVersions(model, index + 1);
            }
            if (this.mainPageSubNavActiveIndex == 2) {
                this.expandSlotTypeVersions(model, index + 1);
            }
        }
        else {
            this.hideAllVersions(index);
        }
        model.expandAllVersions = !model.expandAllVersions;
    }

    /**
     * Select all the entries in the table
     * @param $event the status of the check box
     * @param type the type of the models to select
	**/
    public selectAll($event, type?): void {
        let selectAll = $event.target.checked;
        let lib = [];
        if (this.mainPageSubNavActiveIndex == 0) {
            lib = this.bots;
        }
        else if (this.mainPageSubNavActiveIndex == 1) {
            lib = this.intentCategory == "Builtin" ? this.builtinIntents : this.customIntents;
        }
        else if (this.mainPageSubNavActiveIndex == 2) {
            lib = this.slotTypeCategory == "Builtin" ? this.builtinSlotTypes : this.customSlotTypes;
        }

        for (let model of lib) {
            if (model.isSelected != selectAll) {
                this.updateSelectedEntriesNum(model);
                model.isSelected = selectAll;
            }
        }
    }

    /**
     * Remove an entry from a list
     * @param list the list to remove the entry from
     * @param entry the entry to remove
	**/
    public removeEntry(list, entry): void {
        let index = list.indexOf(entry, 0);
        if (index > -1) {
            list.splice(index, 1);
        }
    }

    /**
     * Update the number of selected entries shown on the page
     * @param model the model whose status is changed
	**/
    public updateSelectedEntriesNum(model): void {
        let change = model.isSelected  ? 1 : -1;
        if (this.mainPageSubNavActiveIndex == 0) {
            // Update the number according to the platform
            // The value for model.isSelected on Edge is the opposite of that on Chrome or FireFox
            this.selectedBotsNum += window.navigator.msSaveOrOpenBlob ? change : -change;
            if (model.isSelected) {
                this.selectAllBotsRef.nativeElement.checked = false;
            }
        }
        else if (this.mainPageSubNavActiveIndex == 1) {
            this.selectedIntentsNum += window.navigator.msSaveOrOpenBlob ? change : -change;
            if (model.isSelected) {
                this.selectAllIntentsRef.nativeElement.checked = false;
            }
        }
        else if (this.mainPageSubNavActiveIndex == 2) {
            this.selectedSlotTypesNum += window.navigator.msSaveOrOpenBlob ? change : -change;
            if (model.isSelected) {
                this.selectAllSlotTypesRef.nativeElement.checked = false;
            }
        }
    }

    /**
     * Update the content when switch to a new tab on the main page
     * @param subNavItemIndex the index of the new tab
	**/
    public getMainPageSubNavItem(subNavItemIndex: number): void {
        if (this.showBotEditor || this.showIntentEditor || this.showSlotTypeEditor) {
            this.breadcrumbs.removeLastBreadcrumb();
        }
        this.mainPageSubNavActiveIndex = subNavItemIndex;
        this.showBotEditor = false;
        this.showIntentEditor = false;
        this.showSlotTypeEditor = false;
        this.selectedBotsNum = 0;
        this.selectedIntentsNum = 0;
        this.selectedSlotTypesNum = 0;
        this.update();
    }

    /**
     * Update the content when switch to a new tab in the bot editor
     * @param botEditorSubNavItemIndex the index of the new tab
	**/
    public getBotSubNavItem(botEditorSubNavItemIndex: number): void {
        this.botEditorSubNavItemIndex = botEditorSubNavItemIndex;
        this.switchSideBarTabs(0);
    }

    /**
     * Gets a list of bots
     * @param PageToken A pagination token that fetches the next page of bots. If this API call is truncated, Amazon Lex returns a pagination token in the response.
	**/
    private updateBots(PageToken: string): void {
        this.sortDir = "asc";
        this.isLoadingBots = true;
        this._apiHandler.getBots(PageToken).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (!obj.result.bots.error) {
                this.bots = PageToken.length <= 1 ? [] : this.bots;
                for (let bot of obj.result.bots) {
                    let newBot = new BotEntry(bot, this.toastr, this._apiHandler);
                    this.bots.push(newBot);
                }
                let s = this.bots[0];
                if (obj.result.nextToken != "") {
                    this.updateBots(obj.result.nextToken);
                }
                else {
                    for (let bot of this.bots) {
                        bot.checkBotStatus();
                    }
                    this.sort(this.bots, "name", this.sortDir);
                    this.updatePaginationInfo();
                    this.updateCurrentPage(1);
                    this.isLoadingBots = false;
                }
            }
            else {
                this.toastr.error("The bots did not refresh properly. " + obj.result.bots.error);
                this.isLoadingBots = false;
            }
        }, err => {
            this.toastr.error("The bots did not refresh properly. " + err.message);
            this.isLoadingBots = false;
        });
    }

    /**
     * Show all the versions of the selected bot
     * @param bot the bot to show all the versions
     * @param index the index of the bot in the current table
	**/
    private expandBotVersions(bot: BotEntry, index): void {
        bot.getVersions("t").then(function () {
            for (let botEntry of bot.versions) {
                if (botEntry.version != "$LATEST") {
                    this.bots.splice((this.botPageIndex - 1) * this.pageSize + index, 0, botEntry);
                    this.botsOnCurrentPage.splice(index, 0, botEntry);
                }
            }
        }.bind(this))
    }

    /**
     * Get information about all of the versions of a bot
     * @param PageToken A pagination token that fetches the next page of bot versions. If this API call is truncated, Amazon Lex returns a pagination token in the response
	**/
    private updateCurrentBotVersions(PageToken: string): void {
        this.isLoadingBotVersions = true;
        this.currentBot.getVersions("t").then(function () {
            this.isLoadingBotVersions = false;
        }.bind(this), function () {
            this.isLoadingBotVersions = false;
        }.bind(this))
    }

    /**
     * Add a new custom intent to the bot
     * @param bot The bot to accept the intent
	**/
    private addNewCustomIntentToBot(bot: BotEntry): void {
        this.newIntent.name = this.newIntent["name"]["value"];
        this.newIntent.save().then(function () {
            this.newIntent.isSelected = true;
            this.newIntent.version = "$LATEST";
            let intents = [this.newIntent];
            this.addExistingCustomIntentsToBot(intents, bot);
        }.bind(this));
    }

    /**
     * Delete an existing bot
     * @param bot The bot to delete
	**/
    private deleteBot(bot: BotEntry): void {
        bot.delete().then(function () {
            let index = this.bots.indexOf(bot, 0);
            if (index > -1) {
                if (bot.version == "$LATEST") {
                    this.hideAllVersions(index);
                }
                this.bots.splice(index, 1);
            }
            if (bot.isSelected) {
                this.selectedBotsNum--;
            }
            this.updatePaginationInfo();
            this.paginationRef.reset();
        }.bind(this));
    }

    /**
     * Show all the versions of the selected intent
     * @param intent the intent to show all the versions
     * @param index the index of the intent in the current table
	**/
    private expandIntentVersions(intent: IntentEntry, index): void {
        intent.getVersions("t").then(function () {
            for (let intentEntry of intent.versions) {
                if (intentEntry.version != "$LATEST") {
                    this.customIntents.splice((this.intentPageIndex - 1) * this.pageSize + index, 0, intentEntry);
                    this.intentsOnCurrentPage.splice(index, 0, intentEntry);
                }
            }
        }.bind(this))
    }

    /**
     * Initialize the information of a new intent
     * @param intent the intent to initialize
	**/
    private initializeIntent(intent): void {
        intent.updated = intent.updated ? intent.updated : "";
        intent.created = intent.created ? intent.created : "";
        intent.version = intent.version ? intent.version : "";
    }

    /**
     * Get a list of slot types used by the intent
     * @param intent the intent to get the slot type list for
	**/
    private getSlotTypesUsedByCurrentIntent(intent: IntentEntry): void {
        intent.get(intent.version).then(function (latestIntent) {
            this.slotTypesUsedByCurrentBot = [];
            for (let slot of latestIntent["slots"]) {
                if (slot["slotTypeVersion"]) {
                    let slotType = this.defaultSlotType();
                    slotType.name = slot["slotType"];
                    slotType.version = slot["slotTypeVersion"];
                    this.slotTypesUsedByCurrentBot.push(slotType);
                }
            }
        }.bind(this))
    }

    /**
     * Remove the slot types used by the intent and the intent itself from the sidebar
     * @param intent the intent to remove
     * @param index the intent index in the list
    **/
    private removeIntentAndSlotTypesFromSidebar(intent: IntentEntry, index): void {
        let tmpIntent = this.defaultIntent();
        tmpIntent["name"] = intent["intentName"];
        tmpIntent.get(intent["intentVersion"]).then(function (latestIntent) {
            let slotTypesUsedByCurrentIntent = [];
            for (let slot of latestIntent["slots"]) {
                if (!slot["slotTypeVersion"]) {
                    continue;
                }
                for (let i = 0; i < this.slotTypesUsedByCurrentBot.length; i++) {
                    if (slot["slotType"] == this.slotTypesUsedByCurrentBot[i].name) {
                        this.slotTypesUsedByCurrentBot.splice(i, 1);
                        break;
                    }
                }
            }
            this.currentBot["intents"].splice(index, 1);
            if (index < this.sidebarIndex) {
                this.sidebarIndex--;
                this.switchSideBarTabs(this.sidebarIndex);
            }
            else if (index == this.sidebarIndex) {
                this.switchSideBarTabs(this.sidebarIndex);
            }
        }.bind(this))
    }

    /**
     * Delete an existing intent
     * @param intent The intent to delete
	**/
    private deleteIntent(intent: IntentEntry): void {
        intent.delete().then(function () {
            let index = this.customIntents.indexOf(intent, 0);
            if (index > -1) {
                if (intent.version == "$LATEST") {
                    this.hideAllVersions(index);
                }
                this.customIntents.splice(index, 1);
            }
            if (intent.isSelected) {
                this.selectedIntentsNum--;
            }
            this.updatePaginationInfo();
            this.paginationRef.reset();
        }.bind(this))
    }

    /**
     * Show all the versions of the selected slot type
     * @param slotType the slot type to show all the versions
     * @param index the index of the slot type in the current table
	**/
    private expandSlotTypeVersions(slotType: SlotTypeEntry, index): void {
        slotType.getVersions("t").then(function () {
            for (let slotTypeEntry of slotType.versions) {
                if (slotTypeEntry.version != "$LATEST") {
                    this.customSlotTypes.splice((this.botPageIndex - 1) * this.pageSize + index, 0, slotTypeEntry);
                    this.slotTypesOnCurrentPage.splice(index, 0, slotTypeEntry);
                }
            }
        }.bind(this))
    }

    /**
     * Initialize the information of a new slot type
     * @param intent The intent to initialize
	**/
    private initializeSlotType(slotType): void {
        slotType.update = slotType.update ? slotType.update : "";
        slotType.created = slotType.created ? slotType.created : "";
        slotType.version = slotType.version ? slotType.version : "";
    }

    /**
     * Add a new value to the current slot type
	**/
    public addNewValueToSlotType(): void {
        if (this.newSlotTypeValue != "") {
            this.currentSlotType.enumerationValues["value"].push({ value: this.newSlotTypeValue });
            this.newSlotTypeValue = "";
        }
    }

    /**
     * Delete the slot type value entry if it is empty
     * @param $event The new slot type value
    * @param index The index of the value in the list
	**/
    public slotTypeValueChange($event, index): void {
        if ($event == "") {
            this.currentSlotType.enumerationValues["value"].splice(index, 1);
        }
    }

    /**
     * Delete an existing slot type
     * @param slotType The slot type to delete
	**/
    private deleteSlotType(slotType: SlotTypeEntry): void {
        slotType.delete().then(function () {
            let index = this.customSlotTypes.indexOf(slotType, 0);
            if (index > -1) {
                if (slotType.version == "$LATEST") {
                    this.hideAllVersions(index);
                }
                this.customSlotTypes.splice(index, 1);
            }
            if (slotType.isSelected) {
                this.selectedSlotTypesNum--;
            }
            this.updatePaginationInfo();
            this.paginationRef.reset();
        }.bind(this))
    }

    /**
     * Generate a default bot
	**/
    private defaultBot(): BotEntry {
        return new BotEntry({
            name: {
                valid: true,
                value: ""
            },
            locale: "en-US",
            childDirected: {
                valid: true
            },
            clarificationPrompt: {
                'messages': [
                    {
                        'contentType': 'PlainText',
                        'content': 'Sorry, can you please repeat that?'
                    },
                ],
                'maxAttempts': 5
            },
            abortStatement: {
                'messages': [
                    {
                        'contentType': 'PlainText',
                        'content': 'Sorry, I could not understand. Goodbye.'
                    },
                ]
            },
            idleSessionTTLInSeconds: {
                valid: true,
                value: 300
            }
        }, this.toastr, this._apiHandler);
    }

    /**
     * Generate a default intente
	**/
    private defaultIntent(): IntentEntry {
        return new IntentEntry({
            name: {
                valid: true,
                value: ""
            },
            fulfillmentActivity: {
                'type': "ReturnIntent"
            }
        }, this.toastr, this._apiHandler);
    }

    /**
     * Generate a default slot type
	**/
    private defaultSlotType(): SlotTypeEntry {
        return new SlotTypeEntry({
            name: {
                valid: true,
                value: ""
            },
            description: "",
            enumerationValues: {
                valid: true,
                value: []
            },
        }, this.toastr, this._apiHandler)
    }

    /**
     * hide the version list shown on the current table
     * @param index The index of the model to hide the version list
	**/
    private hideAllVersions(index): void {
        if (this.mainPageSubNavActiveIndex == 0) {
            while (index + 1 < this.botsOnCurrentPage.length && this.botsOnCurrentPage[index + 1].name == this.botsOnCurrentPage[index].name) {
                this.bots.splice((this.botPageIndex - 1) * this.pageSize + index + 1, 1);
                this.botsOnCurrentPage.splice(index + 1, 1);
            }
        }
        else if (this.mainPageSubNavActiveIndex == 1) {
            while (index + 1 < this.intentsOnCurrentPage.length && this.intentsOnCurrentPage[index + 1].name == this.intentsOnCurrentPage[index].name) {
                this.customIntents.splice((this.intentPageIndex - 1) * this.pageSize + index + 1, 1);
                this.intentsOnCurrentPage.splice(index + 1, 1);
            }
        }
        else if (this.mainPageSubNavActiveIndex == 2) {
            while (index + 1 < this.slotTypesOnCurrentPage.length && this.slotTypesOnCurrentPage[index + 1].name == this.slotTypesOnCurrentPage[index].name) {
                this.customSlotTypes.splice((this.slotTypePageIndex - 1) * this.pageSize + index + 1, 1);
                this.slotTypesOnCurrentPage.splice(index + 1, 1);
            }
        }
    }

    /**
     * Update the paginiation information
	**/
    private updatePaginationInfo() {
        if (this.mainPageSubNavActiveIndex == 0) {
            if (this.showBotEditor && this.botEditorSubNavItemIndex == 1 && this.sidebarIndex == 0) {
                let numberAliases: number = this.currentBot["aliases"].length;
                this.aliasPages = Math.ceil(numberAliases / this.pageSize);
            }
            else {
                let numberBots: number = this.bots.length;
                this.botPages = Math.ceil(numberBots / this.pageSize);
            }
        }
        else if (this.mainPageSubNavActiveIndex == 1) {
            let numberIntents: number = this.intentCategory == "Builtin" ? this.builtinIntents.length : this.customIntents.length;
            this.intentPages = Math.ceil(numberIntents / this.pageSize);
        }
        else if (this.mainPageSubNavActiveIndex == 2) {
            let numberSlotTypes: number = this.slotTypeCategory == "Builtin" ? this.builtinSlotTypes.length : this.customSlotTypes.length;
            this.slotTypePages = Math.ceil(numberSlotTypes / this.pageSize);
        }
    }

    /**
     * Get the sort order of the current table
	**/
    private getSortOrder(): string {
        if (this.sortDir == "") {
            this.sortDir = "asc";
        }
        else {
            this.sortDir = this.sortDir == "asc" ? "desc" : "asc";
        }
        return this.sortDir;
    }

    /**
     * Sort the table by the key using the quick sort algorithm
     * @param left The index of the first item in the table
     * @param right The index of the last item in the table
     * @param table The table to sort
     * @param key The key to sort by
     * @param dir The sort direction
	**/
    private sort(table, key, dir): void {
        if (dir == "asc") {
            table.sort((a, b) => a[key] < b[key] ? -1 : a[key] > b[key] ? 1 : 0);
        }
        else {
            table.sort((a, b) => a[key] > b[key] ? -1 : a[key] < b[key] ? 1 : 0);
        }
    }
}
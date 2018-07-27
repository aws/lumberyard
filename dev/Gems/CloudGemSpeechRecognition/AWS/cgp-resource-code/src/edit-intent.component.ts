import { Http } from '@angular/http';
import { ViewChild, ElementRef, Input, Output, Component, EventEmitter, ViewContainerRef } from '@angular/core';
import { ModalComponent } from 'app/shared/component/index';
import { AwsService } from "app/aws/aws.service";
import { BotEntry, IntentEntry, SlotTypeEntry, SpeechToTextApi } from './index'
import { Subscription } from 'rxjs/rx';
import { AbstractCloudGemIndexComponent } from 'app/view/game/module/cloudgems/class/index';
import { ToastsManager } from 'ng2-toastr/ng2-toastr';
import { PaginationComponent } from 'app/view/game/module/shared/component/index';
import { ActionItem } from "app/view/game/module/shared/class/index";
import { LyMetricService } from 'app/shared/service/index';

import {OnInit } from '@angular/core'

@Component({
    selector: 'edit-intent',
    templateUrl: 'node_modules/@cloud-gems/cloudgemspeechrecognition/edit-intent.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemspeechrecognition/edit-intent.component.css']
})

export class SpeechToTextEditIntentComponent {
    @Input() originalIntent: IntentEntry;
    @Input() context: any;
    @Input() oldVersionWarning: string;
    @Output() originalIntentChange = new EventEmitter<any>();

    @ViewChild(ModalComponent) modalRef: ModalComponent;

    private _apiHandler: SpeechToTextApi;

    private listSampleUtterances: boolean = true;
    private showLambdaInitializationAndValidation: boolean;
    private lambdaInitializationAndValidationEnabled: boolean;
    private showSlots: boolean = true;
    private showConfirmationPrompt: boolean;
    private confirmationPromptEnabled: boolean;
    private showFulfillment: boolean;

    private currentIntent: IntentEntry;
    private builtinSlotTypes: SlotTypeEntry[];
    private customSlotTypes: SlotTypeEntry[];
    private slotTypeVersionMappings: Object = {};
    private newSlot: Object;
     
    private isLoadingIntent: boolean;   
    private isLoadingBuiltinSlotTypes: boolean;
    private isLoadingCustomSlotTypes: boolean;
    private newSampleUtterance: string = "";

    private sortDir: string;

    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager, private vcr: ViewContainerRef, private metric: LyMetricService) {
    }

    /**
     * Initialize values
	**/
    ngOnInit() {
        this._apiHandler = new SpeechToTextApi(this.context.ServiceUrl, this.http, this.aws, this.metric, this.context.identifier);
        this.newSlot = {};
        this.updateBuiltinSlotTypes("t");
        this.updateCustomSlotTypes("t");
        this.getIntent(this.originalIntent);       
    }

    /**
     * Update the intent on display when switch to a new tab
     * @param changes any change of the component
	**/
    ngOnChanges(changes: any) {
        if (!changes.originalIntent.previousValue || (changes.originalIntent.previousValue.name == changes.originalIntent.currentValue.name)) {
            return;
        }
        this.getIntent(this.originalIntent);
    }

    /**
     * Mechanism for tracking items in a list
     * @param index index of the array
     * @param obj index of the current item
	**/
    public customTrackBy(index: number, obj: any): any {
        return index;
    }

    /**
     * Change the version of the current intent
	**/
    public changeIntentVersion(): void {
        this.getIntent(this.currentIntent);
        this.originalIntentChange.emit(this.currentIntent);
    }

    /**
     * Show the detail of each property
     * @param name the name of the property
	**/
    public expandProperty(name): void {
        if (name == "sampleUtterances") {
            this.listSampleUtterances = !this.listSampleUtterances;
        }
        else if (name == "lambdaInitializationAndValidation") {
            this.showLambdaInitializationAndValidation = !this.showLambdaInitializationAndValidation;
        }
        else if (name == "slots") {
            this.showSlots = !this.showSlots;
        }
        else if (name == "confirmationPrompt") {
            this.showConfirmationPrompt = !this.showConfirmationPrompt;
        }
        else if (name == "fulfillment") {
            this.showFulfillment = !this.showFulfillment;
        }
    }

    /**
     * Add a new sample utterance
     * @param newSampleUtterance the new sample utterance to add
	**/
    public addNewSampleUtterance(): void {
        if (this.newSampleUtterance != "") {
            if (this.currentIntent["sampleUtterances"]) {
                this.currentIntent["sampleUtterances"].push(this.newSampleUtterance);
            }
            else {
                this.currentIntent["sampleUtterances"] = [this.newSampleUtterance];
            }
        }
        this.newSampleUtterance = "";
    }

    /**
     * Change the sample utterance content. Delete the entry if it is empty
     * @param $event the new content of the sample utterance
     * @param index the index of the sample utterance in the list
	**/
    public currentSampleUtteranceChange($event, index): void {
        this.currentIntent.sampleUtterances[index] = $event;
        if ($event == "") {
            this.currentIntent.sampleUtterances.splice(index, 1);
        }
    }

    /**
     * Remove the selected sample utterance
     * @param index the index of the sample utterance in the list
	**/
    public removeSampleUtterance(index): void {
        this.currentIntent.sampleUtterances.splice(index, 1);
    }

    /**
     * Set the Lambda initialization and validation option
     * @param $event true if the Lambda initialization and validation is enabled
    **/
    public changeLambdaInitializationAndValidationState($event): void {
        let state = $event.target.checked;
        if (!state) {
            this.lambdaInitializationAndValidationEnabled = false;
            delete this.currentIntent.dialogCodeHook;
        }
        else {
            this.currentIntent.dialogCodeHook = { uri: "", messageVersion: "1.0" };
        }
    }

    /**
     * Add a new slot to the intent
	**/
    public addSlot(): void {
        if (!this.newSlot["name"] || !this.newSlot['slotType']) {
            return;
        }

        this.newSlot["slotConstraint"] = "Required";
        this.newSlot["priority"] = this.currentIntent["slots"].length + 1;
        if (!this.isBuiltinSlotType(this.newSlot['slotType'])) {
            this.updateSlotTypeVersions(this.newSlot['slotType']);
            this.newSlot["slotTypeVersion"] = "$LATEST";
        }

        this.currentIntent["slots"].push(JSON.parse(JSON.stringify(this.newSlot)));
        this.newSlot = {};
    }

    /**
     * Change the priority of a slot
     * @param index the index of the slot to change
     * @param dir the direction to change the priority
	**/
    public changePriority(index, dir): void {
        let tmpSlot = JSON.parse(JSON.stringify(this.currentIntent["slots"][index - dir]));
        this.currentIntent["slots"][index - dir] = JSON.parse(JSON.stringify(this.currentIntent["slots"][index]));
        this.currentIntent["slots"][index - dir]["priority"] -= dir;
        this.currentIntent["slots"][index] = JSON.parse(JSON.stringify(tmpSlot));
        this.currentIntent["slots"][index]["priority"] += dir;
    }

    /**
     * Set the slot constraint
     * @param $event true if the slot is required
     * @param slot the slot to set
	**/
    public setslotConstraint($event, slot): void {
        let select = $event.target.checked;
        slot["slotConstraint"] = select ? "Required" : "Optional";
    }

    /**
     * Remove an existing slot
     * @param index the index of the slot in the list
	**/
    public removeSlot(index): void {
        for (let i = index; i < this.currentIntent["slots"].length - 1; i++) {
            this.changePriority(i, -1);
        }
        this.currentIntent["slots"].splice(this.currentIntent["slots"].length - 1, 1);
    }

    /**
     * Set the confirmation prompt option
     * @param $event true if the confirmation prompt is enabled
	**/
    public changeConfirmationPromptState($event): void {
        let state = $event.target.checked;
        if (!state) {
            this.confirmationPromptEnabled = false;
            delete this.currentIntent.confirmationPrompt;
            delete this.currentIntent.rejectionStatement;
        }
    }

    /**
     * Gets a list of built-in slot types
     * @param PageToken A pagination token that fetches the next page of slot types. If this API call is truncated, Amazon Lex returns a pagination token in the response.
	**/
    public updateBuiltinSlotTypes(PageToken: string): void {
        this.sortDir = "asc";
        this.isLoadingBuiltinSlotTypes = true;
        this._apiHandler.getBuiltinSlotTypes(encodeURIComponent(PageToken)).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (!obj.result.slotTypes.error) {
                this.builtinSlotTypes = PageToken.length <= 1 ? obj.result.slotTypes : this.builtinSlotTypes.concat(obj.result.slotTypes);
                if (obj.result.nextToken != "") {
                    this.updateBuiltinSlotTypes(obj.result.nextToken);
                }
                else {
                    this.builtinSlotTypes.sort((a, b) => a["name"] < b["name"] ? -1 : a["name"] > b["name"] ? 1 : 0);
                    this.isLoadingBuiltinSlotTypes = false;
                }
            }
            else {
                this.toastr.error("The builtin slot types did not refresh properly. " + obj.result.slotTypes.error);
                this.isLoadingBuiltinSlotTypes = false;
            }
        }, err => {
            this.toastr.error("The builtin slot types did not refresh properly. " + err.message);
            this.isLoadingBuiltinSlotTypes = false;
        });
    }

    /**
     * Gets a list of custom slot types
     * @param PageToken A pagination token that fetches the next page of slot types. If this API call is truncated, Amazon Lex returns a pagination token in the response.
	**/
    public updateCustomSlotTypes(PageToken: string): void {
        this.sortDir = "asc";
        this.isLoadingCustomSlotTypes = true;
        this._apiHandler.getCustomSlotTypes(encodeURIComponent(PageToken)).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (!obj.result.slotTypes.error) {
                this.customSlotTypes = PageToken.length <= 1 ? obj.result.slotTypes : this.customSlotTypes.concat(obj.result.slotTypes);
                if (obj.result.nextToken != "") {
                    this.updateCustomSlotTypes(obj.result.nextToken);
                }
                else {
                    this.customSlotTypes.sort((a, b) => a["name"] < b["name"] ? -1 : a["name"] > b["name"] ? 1 : 0);
                    this.isLoadingCustomSlotTypes = false;
                }
            }
            else {
                this.toastr.error("The builtin slot types did not refresh properly. " + obj.result.slotTypes.error);
                this.isLoadingCustomSlotTypes = false;
            }
        }, err => {
            this.toastr.error("The builtin slot types did not refresh properly. " + err.message);
            this.isLoadingCustomSlotTypes = false;
        });
    }

    /**
     * Save the current intent
    **/
    public saveIntent(): void {
        this.currentIntent.get("$LATEST").then(function (intent) {
            this.currentIntent.checksum = intent.checksum;
            if (this.currentIntent.fulfillmentActivity.type === 'CodeHook') {
                this.currentIntent.fulfillmentActivity.codeHook.messageVersion = '1.0';
            }
            this.currentIntent.save();
        }.bind(this))
    }

    /**
     * Check whether the selected slot type is a built-in slot type
     * @param slotType the name of the slot type to check
     * @return true if the selected slot type is a built-in slot type
    **/
    private isBuiltinSlotType(slotType): boolean {
        for (let builtinSlotType of this.builtinSlotTypes) {
            if (slotType == builtinSlotType.name) {
                return true;
            }
        }
        return false;
    }

    /**
     * Get the information of the selected intent
     * @param intent the intent to get
    **/
    private getIntent(intent: IntentEntry): void {
        this.isLoadingIntent = true;
        intent.get(intent.version).then(function (response) {
            this.currentIntent = response;
            if (this.currentIntent.slots) {
                this.currentIntent.slots.sort((a, b) => a["priority"] < b["priority"] ? -1 : a["priority"] > b["priority"] ? 1 : 0);
                for (let slot of this.currentIntent.slots) {
                    if (slot.slotTypeVersion) {
                        this.updateSlotTypeVersions(slot.slotType);
                    }
                }
            }
            this.showConfirmationPrompt = this.currentIntent.confirmationPrompt && this.currentIntent.confirmationPrompt["messages"];
            this.confirmationPromptEnabled = this.showConfirmationPrompt;
            this.showLambdaInitializationAndValidation = this.currentIntent.dialogCodeHook && this.currentIntent.dialogCodeHook["uri"];
            this.lambdaInitializationAndValidationEnabled = this.showLambdaInitializationAndValidation;
            this.showFulfillment = this.currentIntent.fulfillmentActivity;
            this.getIntentVersions(this.currentIntent);
        }.bind(this), function () {
            this.isLoadingIntent = false;
        }.bind(this))
    }

    /**
     * Get the all the versions of the selected intent
     * @param intent the intent to get all the versions
    **/
    private getIntentVersions(intent: IntentEntry): void {
        this.currentIntent.getVersions("t").then(function () {
            this.isLoadingIntent = false;
        }.bind(this), function () {
            this.isLoadingIntent = false;
        }.bind(this));
    }

    /**
     * Get the all the versions of the selected custom slot type
     * @param slotTypeName the slot type to get all the versions
    **/
    public updateSlotTypeVersions(slotTypeName): void {
        if (this.slotTypeVersionMappings[slotTypeName]) {
            return;
        }

        let slotType = new SlotTypeEntry({ name: slotTypeName }, this.toastr, this._apiHandler);
        slotType.getVersions("t").then(function () {
            this.slotTypeVersionMappings[slotTypeName] = slotType.versions;
        }.bind(this));
    }

    /**
     * Clear the current lambda function ARN when users select to return parameters to client
    **/
    public clearLambdaFunctionArn(): void {
        if (this.currentIntent.fulfillmentActivity["type"] === 'CodeHook') {
            this.currentIntent.fulfillmentActivity["codeHook"] = {};
            this.currentIntent.fulfillmentActivity["codeHook"]["uri"] = "";
        }
        else {
            delete this.currentIntent.fulfillmentActivity['codeHook'];
        }
    }
}
import { SpeechToTextApi } from './index';
import { ToastsManager } from 'ng2-toastr/ng2-toastr';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";

export class IntentEntry {
    name: string;
    description: string;
    version: string;
    update: string;
    created: string;
    slots: Object[];
    sampleUtterances: string[];
    confirmationPrompt: Object;
    rejectionStatement: Object;
    followUpPrompt: Object;
    conclusionStatement: Object;
    dialogCodeHook: Object;
    fulfillmentActivity: Object;
    checksum: string;
    parentIntentSignature: string;
    isSelected: boolean;
    expandAllVersions: boolean;
    versions;
    isLoading: boolean;

    constructor(intentInfo: any, private toastr: ToastsManager, private _apiHandler: SpeechToTextApi) {
        this.name = intentInfo.name;
        this.description = intentInfo.description;
        this.version = intentInfo.version;
        this.update = intentInfo.update;
        this.created = intentInfo.created;
        this.slots = intentInfo.slots;
        this.sampleUtterances = intentInfo.sampleUtterances;
        this.confirmationPrompt = intentInfo.confirmationPrompt;
        this.conclusionStatement = intentInfo.conclusionStatement;
        this.rejectionStatement = intentInfo.rejectionStatement;
        this.followUpPrompt = intentInfo.followUpPrompt;
        this.dialogCodeHook = intentInfo.dialogCodeHook;
        this.fulfillmentActivity = intentInfo.fulfillmentActivity;
        this.checksum = intentInfo.checksum;
        this.parentIntentSignature = intentInfo.parentIntentSignature;
        this.isSelected = false;
        this.expandAllVersions = false;
        this.isLoading = false;
    }

    /**
     * Get the intent of a specific verison
     * @param version The version of the intent to get
	**/
    public get(version: string): any {
        var promise = new Promise(function (resolve, reject) {
            this._apiHandler.getIntent(this.name, version).subscribe(response => {
                let obj = JSON.parse(response.body.text());
                if (!obj.result.intent.error) {
                    let intent = new IntentEntry(obj.result.intent, this.toastr, this._apiHandler);
                    resolve(intent);
                }
                else {
                    this.toastr.error("The intent '" + this.name + "' did not refresh properly. " + obj.result.intent.error);
                    reject();
                }
            }, err => {
                this.toastr.error("The intent '" + this.name + "' did not refresh properly. " + err.message);
                reject();
            });
        }.bind(this))
        return promise;
    }

    /**
     * Gets information about all of the versions of a intent
     * @param PageToken A pagination token for fetching the next page of intent versions. If the response to this call is truncated, Amazon Lex returns a pagination token in the response.
	**/
    public getVersions(PageToken: string): any {
        this.isLoading = true;
        var promise = new Promise(function (resolve, reject) {
            this._apiHandler.getIntentVersions(this.name, PageToken).subscribe(response => {
                let obj = JSON.parse(response.body.text());
                if (!obj.result.intents.error) {
                    if (PageToken.length <= 1) {
                        this.versions = [];
                    }
                    for (let intent of obj.result.intents) {
                        this.versions.push(new IntentEntry(intent, this.toastr, this._apiHandler));
                    }
                    if (obj.result.nextToken != "") {
                        this.getVersions(obj.result.nextToken);
                    }
                    else {
                        resolve();
                        this.isLoading = false;
                    }
                }
                else {
                    this.toastr.error("The intent versions did not refresh properly. " + obj.result.intents.error);
                    reject();
                    this.isLoading = false;
                }
            }, err => {
                this.toastr.error("The intent versions did not refresh properly. " + err.message);
                reject();
                this.isLoading = false;
            });
        }.bind(this))
        return promise;
    }

    /**
     * Save the intent
	**/
    public save(): any {
        var promise = new Promise(function (resolve, reject) {
            let body = { "intent": this.getInfo() };
            this._apiHandler.createIntent(body).subscribe(response => {
                let obj = JSON.parse(response.body.text());
                if (obj.result.status == "ACCEPTED") {
                    this.toastr.success("The intent '" + this.name + "' was saved.");
                    resolve();
                }
                else {
                    this.toastr.error("The intent '" + this.name + "' was not saved successfully. " + obj.result.status);
                    reject();
                }
            }, err => {
                this.toastr.error("The intent '" + this.name + "' was not saved successfully. " + err.message);
                reject();
            });
        }.bind(this))
        return promise;
    }

    /**
     * Delete the intent
	**/
    public delete(): any {
        var promise = new Promise(function (resolve, reject) {
            if (this.version == "$LATEST") {
                this._apiHandler.deleteIntent(this.name).subscribe(response => {
                    let obj = JSON.parse(response.body.text());
                    if (obj.result.status == "INUSE") {
                        this.toastr.error("The intent '" + this.name + "' is in use.");
                        reject();
                    }
                    else if (obj.result.status == "DELETED") {
                        this.toastr.success("The intent '" + this.name + "' was deleted.");
                        resolve();
                    }
                    else {
                        this.toastr.error("The intent '" + this.name + "' could not be deleted. " + obj.result.status);
                        reject();
                    }
                });
            }
            else {
                this._apiHandler.deleteIntentVersion(this.name, this.version).subscribe(response => {
                    let obj = JSON.parse(response.body.text());
                    if (obj.result.status == "INUSE") {
                        this.toastr.error("The selected version of the intent '" + this.name + "' is in use.");
                        reject();
                    }
                    else if (obj.result.status == "DELETED") {
                        this.toastr.success("The selected version of the intent '" + this.name + "' was deleted.");
                        resolve();
                    }
                    else {
                        this.toastr.error("The selected version of the intent '" + this.name + "' could not be deleted. " + obj.result.status);
                        reject();
                    }
                });
            }
        }.bind(this))
        return promise;
    }

    /**
     * Get the dependency mappings of all the intents
	**/
    public getDependency(): any {
        var promise = new Promise(function (resolve, reject) {
        this._apiHandler.getIntentDenendency().subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (!obj.result.dependency.error) {
                let intentDependency = obj.result.dependency;
                resolve(intentDependency);
            }
            else {
                this.toastr.error("The intent dependency did not refresh properly. " + obj.result.dependency.error);
                reject();
            }
        }, err => {
            this.toastr.error("The intent dependency did not refresh properly. " + err.message);
            reject();
        })
        }.bind(this))
        return promise;
    }

    /**
     * Generate an object that contains all the necessary information to update a intent
     * @return an object that contains all the necessary information to update a intent
	**/
    private getInfo(): Object {
        let intentInfo = {}
        let properties = ["name", "description", "slots", "sampleUtterances", "confirmationPrompt", "rejectionStatement", "followUpPrompt", "conclusionStatement", "dialogCodeHook", "fulfillmentActivity", "parentIntentSignature", "checksum"];

        for (let property of properties) {
            if (this[property] != undefined) {
                intentInfo[property] = this[property];
            }
        }
        return intentInfo;
    }
}
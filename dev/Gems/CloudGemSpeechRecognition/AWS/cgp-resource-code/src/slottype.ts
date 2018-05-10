import { SpeechToTextApi } from './index';
import { ToastsManager } from 'ng2-toastr/ng2-toastr';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
export class SlotTypeEntry {
    name: string;
    description: string;
    valueElicitationPrompt: Object;
    enumerationValues: Object[];
    update: string;
    created: string;
    checksum: string;
    valueSelectionStrategy: string;
    version: string;
    isSelected: boolean;
    expandAllVersions: boolean;
    versions: SlotTypeEntry[];
    isLoading: boolean;

    constructor(slotTypeInfo: any, private toastr: ToastsManager, private _apiHandler: SpeechToTextApi) {
        this.name = slotTypeInfo.name;
        this.description = slotTypeInfo.description;
        this.enumerationValues = slotTypeInfo.enumerationValues;
        this.update = slotTypeInfo.update;
        this.created = slotTypeInfo.created;
        this.checksum = slotTypeInfo.checksum;
        this.version = slotTypeInfo.version;
        this.valueSelectionStrategy = slotTypeInfo.valueSelectionStrategy;
        this.isSelected = false;
        this.expandAllVersions = false;
        this.isLoading = false;
    }

    /**
     * Get the slot type of a specific verison
     * @param version The version of the slot type to get
	**/
    public get(version: string): any {
        var promise = new Promise(function (resolve, reject) {
            this._apiHandler.getSlotType(this.name, this.version).subscribe(response => {
                let obj = JSON.parse(response.body.text());
                if (!obj.result.slotType.error) {
                    this.currentSlotType = new SlotTypeEntry(obj.result.slotType, this.toastr, this._apiHandler);
                    this.isLoadingSlotType = false;
                    resolve(this.currentSlotType);
                }
                else {
                    this.isLoadingSlotType = false;
                    this.toastr.error("The intent '" + this.name + "' was not refreshed properly. " + obj.result.slotType.error);
                    reject();
                }
                this.isLoadingSlotType = false;
            }, err => {
                this.isLoadingSlotType = false;
                this.toastr.error("The intent '" + this.name + "' was not refreshed properly. " + err.message);
                reject();
            });
        }.bind(this));

        return promise;
    }

    /**
     * Gets information about all of the versions of a slot type
     * @param PageToken A pagination token for fetching the next page of slot type versions. If the response to this call is truncated, Amazon Lex returns a pagination token in the response.
	**/
    public getVersions(PageToken: string): any {
        this.isLoading = true;
        var promise = new Promise(function (resolve, reject) {
            this._apiHandler.getSlotTypeVersions(this.name, PageToken).subscribe(response => {
                let obj = JSON.parse(response.body.text());
                if(!obj.result.slotTypes.error) {
                    if (PageToken.length <= 1) {
                        this.versions = [];
                    }
                    for (let slotType of obj.result.slotTypes) {
                        this.versions.push(new SlotTypeEntry(slotType, this.toastr, this._apiHandler));
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
                    this.toastr.error("The slot type versions did not refresh properly. " + obj.result.slotTypes.error);
                    reject();
                    this.isLoading = false;
                }
            }, err => {
                this.toastr.error("The slot type versions did not refresh properly. " + err.message);
                reject();
                this.isLoading = false;
            });
        }.bind(this))
        return promise;
    }

    /**
     * Save the slot type
	**/
    public save(): any {
        var promise = new Promise(function (resolve, reject) {
            let body = { "slotType": this.getInfo() };
            this._apiHandler.createSlotType(body).subscribe(response => {
                let obj = JSON.parse(response.body.text());
                if (obj.result.status == "ACCEPTED") {
                    this.toastr.success("The slot type '" + this.name + "' was saved.");
                    resolve();
                }
                else {
                    this.toastr.error("The slot type '" + this.name + "' was not saved successfully. " + obj.result.status );
                    reject();
                }
            }, err => {
                this.toastr.error("The slot type '" + this.name + "' was not saved successfully. " + err.message);
                reject();
            });
        }.bind(this))
        return promise;
    }

    /**
     * Delete the slot type
	**/
    public delete(): any {
        var promise = new Promise(function (resolve, reject) {
            if (this.version == "$LATEST") {
                this._apiHandler.deleteSlotType(this.name).subscribe(response => {
                    let obj = JSON.parse(response.body.text());
                    if (obj.result.status == "INUSE") {
                        this.toastr.error("The slot type '" + this.name + "' is in use.");
                        reject();
                    }
                    else if (obj.result.status == "DELETED") {
                        this.toastr.success("The slot type '" + this.name + "' was deleted.");
                        resolve();
                    }
                    else {
                        this.toastr.error("The slot type '" + this.name + "' could not be deleted. " + obj.result.status);
                        reject();
                    }
                })
            }
            else {
                this._apiHandler.deleteSlotTypeVersion(this.name, this.version).subscribe(response => {
                    let obj = JSON.parse(response.body.text());
                    if (obj.result.status == "INUSE") {
                        this.toastr.error("The selected version of the slot type '" + this.name + "' is in use.");
                        reject();
                    }
                    else if (obj.result.status == "DELETED") {
                        this.toastr.success("The selected version of the slot type '" + this.name + "' was deleted.");
                        resolve();
                    }
                    else {
                        this.toastr.error("The selected version of the slot type '" + this.name + "' could not be deleted. " + obj.result.status);
                        reject();
                    }
                })
            }
        }.bind(this))
        return promise;
    }

    /**
     * Generate an object that contains all the necessary information to update a slot type
     * @return an object that contains all the necessary information to update a slot type
	**/
    private getInfo(): Object {
        let slotTypeInfo = {}
        let properties = ["name", "description", "enumerationValues", "checksum", "valueSelectionStrategy"];

        for (let property of properties) {
            if (this[property] != undefined) {
                slotTypeInfo[property] = this[property];
            }
        }
        return slotTypeInfo;
    }
}
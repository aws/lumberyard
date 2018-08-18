import { SpeechToTextApi } from './index';
import { ToastsManager } from 'ng2-toastr/ng2-toastr';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";

export type ProcessBehavior = "SAVE" | "BUILD";

export class BotEntry {
    name: string;
    description: string;
    version: string;
    checksum: string;
    processBehavior: ProcessBehavior;
    status: string;
    update: string;
    created: string;
    clarificationPrompt: Object[];
    abortStatement: Object[];
    idleSessionTTLInSeconds: number;
    intents: Object[];
    locale: string;
    voiceId: string;
    childDirected: boolean;
    expandAllVersions: boolean;
    isSelected: boolean;
    versions: BotEntry[];
    isLoading: boolean;

    constructor(botInfo: any, private toastr: ToastsManager, private _apiHandler: SpeechToTextApi) {
        this.name = botInfo.name;
        this.description = botInfo.description;
        this.checksum = botInfo.checksum;
        this.version = botInfo.version;
        this.processBehavior = botInfo.processBehavior;
        this.idleSessionTTLInSeconds = botInfo.idleSessionTTLInSeconds;
        this.childDirected = botInfo.childDirected;
        this.intents = botInfo.intents;
        this.status = botInfo.status;
        this.update = botInfo.update;
        this.created = botInfo.created;
        this.clarificationPrompt = botInfo.clarificationPrompt;
        this.abortStatement = botInfo.abortStatement;
        this.locale = botInfo.locale;
        this.voiceId = botInfo.voiceId;
        this.expandAllVersions = false;
        this.isSelected = false;
        this.isLoading = false;
    }

    /**
     * Get the bot of a specific verison
     * @param version The version of the bot to get
	**/
    public get(version: string): any {
        var promise = new Promise(function (resolve, reject) {
            this._apiHandler.getBot(this.name, this.version).subscribe(response => {
                let obj = JSON.parse(response.body.text());
                if (!obj.result.bot.error) {
                    let bot = new BotEntry(obj.result.bot, this.toastr, this._apiHandler);
                    resolve(bot);
                }
                else {
                    this.toastr.error("The bot '" + this.name + "' did not refresh properly. " + obj.result.bot.error);
                    reject();
                }
            }, err => {
                this.toastr.error("The bot '" + this.name + "' did not refresh properly. " + err.message);
                reject();
            });
        }.bind(this));
        return promise;
    }

    /**
     * Gets information about all of the versions of a bot
     * @param PageToken A pagination token for fetching the next page of bot versions. If the response to this call is truncated, Amazon Lex returns a pagination token in the response.
	**/
    public getVersions(PageToken: string): any {
        this.isLoading = true;
        var promise = new Promise(function (resolve, reject) {
            this._apiHandler.getBotVersions(this.name, PageToken).subscribe(response => {
                let obj = JSON.parse(response.body.text());
                if (!obj.result.bots.error) {
                    if (PageToken.length <= 1) {
                        this.versions = [];
                    }
                    for (let bot of obj.result.bots) {
                        this.versions.push(new BotEntry(bot, this.toastr, this._apiHandler));
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
                    this.toastr.error("The bot versions did not refresh properly. " + obj.result.bots.error);
                    reject();
                    this.isLoading = false;
                }
            }, err => {
                this.toastr.error("The bot versions did not refresh properly. " + err.message);
                reject();
                this.isLoading = false;
            });
        }.bind(this));
        return promise;
    }

    /**
     * Export the bot
	**/
    public export(): void {
        this._apiHandler.getDesc(this.name, this.version).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (!obj.result.desc_file.error) {
                let descFile = obj.result.desc_file;
                let blob = new Blob([JSON.stringify(descFile, null, 4)], { type: "application/json" });
                let fileName = this.name + ".json";

                if (window.navigator.msSaveOrOpenBlob) //Edge
                {
                    window.navigator.msSaveBlob(blob, fileName);
                }
                else //Chrome & FireFox
                {
                    let a = document.createElement("a");
                    let fileURL = URL.createObjectURL(blob);
                    a.href = fileURL;
                    a.download = fileName;
                    window.document.body.appendChild(a);
                    a.click();
                    window.document.body.removeChild(a);
                    URL.revokeObjectURL(fileURL);
                }
                this.toastr.success("The bot '" + this.name + "' was exported.");
            }
            else {
                this.toastr.error("The bot '" + this.name + "' was not exported properly. " + obj.result.desc_file.error);
            }
        }, err => {
            this.toastr.error("The bot '" + this.name + "' was not exported properly. " + err.message);
        });
    }

    /**
     * Build the bot
	**/
    public build(): void {
        this._apiHandler.buildBot(this.name).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (obj.result.status == "READY") {
                this.status = "BUILDING";
                this.checkBotStatus();
            }
            else {
                this.toastr.error("The bot '" + this.name + "' was not built properly. " + obj.result.status);
            }
        }, err => {
            this.toastr.error("The bot '" + this.name + "' was not built properly. " + err.message);
        });
    }

    /**
     * Publish the bot using a given alias name
     * @param aliasName The alias name used to publish the bot
	**/
    public publish(aliasName: string): void {
        this.status = "PUBLISHING";
        this._apiHandler.publishBot(this.name, aliasName).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (obj.result.status == "READY") {
                this.toastr.success("The bot '" + this.name + "' was published.");
                this.checkBotStatus();
            }
            else {
                this.toastr.error("The bot '" + this.name + "' was not published properly. " + obj.result.status);
                this.status = "FAILED";
            }
        }, (err) => {
            this.toastr.error("The bot '" + this.name + "' was not published properly. " + err.message);
            this.status = "FAILED";
        });
    }

    /**
     * Delete the bot
	**/
    public delete(): any {
        var promise = new Promise(function (resolve, reject) {
            if (this.version == "$LATEST") {
                this._apiHandler.deleteBot(this.name).subscribe(response => {
                    let obj = JSON.parse(response.body.text());
                    if (obj.result.status == "INUSE") {
                        this.toastr.error("The bot '" + this.name + "' is currently used by an alias. Delete the alisas from '" + this.name + "'");
                        reject();
                    }
                    else if (obj.result.status == "DELETED") {
                        this.toastr.success("The bot '" + this.name + "' was deleted.");
                        resolve();
                    }
                    else {
                        this.toastr.error("The bot '" + this.name + "' could not be deleted. " + obj.result.status);
                        reject();
                    }
                });
            }
            else {
                this._apiHandler.deleteBotVersion(this.name, this.version).subscribe(response => {
                    let obj = JSON.parse(response.body.text());
                    if (obj.result.status == "INUSE") {
                        this.toastr.error("The selected version of the bot '" + this.name + "' is currently used by an alias. Delete the alisas from '" + this.name + "'");
                        reject();
                    }
                    else if (obj.result.status == "DELETED") {
                        this.toastr.success("The selected version of the bot '" + this.name + "' was deleted.");
                        resolve();
                    }
                    else {
                        this.toastr.error("The selected version of the bot '" + this.name + "' could not be deleted. " + obj.result.status);
                        reject();
                    }
                });
            }
        }.bind(this));
        return promise;
    }

    /**
     * Save the bot
     * @param processBehavior If you set the processBehavior element to Build , Amazon Lex builds the bot so that it can be run. If you set the element to Save Amazon Lex saves the bot, but doesn't build it.
	**/
    public save(processBehavior: ProcessBehavior): any {
        var promise = new Promise(function (resolve, reject) {
            this.processBehavior = processBehavior;
            let body = { "bot": this.getInfo() };
            this._apiHandler.updateBot(body).subscribe(response => {
                let obj = JSON.parse(response.body.text());
                if (obj.result.status == "ACCEPTED") {
                    this.toastr.success("The bot '" + this.name + "' was updated.");
                    if (this.processBehavior == "BUILD") {
                        this.status = "BUILDING";
                        this.checkBotStatus();
                    }
                    resolve();
                }
            else {
                this.toastr.error("The bot '" + this.name + "' was not updated successfully. " + obj.result.status);
                reject();
            }
            }, err => {
                this.toastr.error("The bot '" + this.name + "' was not updated successfully. " + err.message);
                reject();
            });
        }.bind(this));
        return promise;
    }

    /**
     * Save the bot alias
     * @param alias The alias to save
	**/
    public saveAlias(alias: Object): any{
        var promise = new Promise(function (resolve, reject) {
            alias["botName"] = this.name;
            let body = { alias: alias };
            this._apiHandler.updateBotAlias(body).subscribe(response => {
                let obj = JSON.parse(response.body.text());
                if (obj.result.status == "ACCEPTED") {
                    this.toastr.success("The bot alias '" + alias["name"] + "' was updated successfully.");
                    resolve();
                }
                else {
                    this.toastr.error("The bot alias'" + alias["name"] + "' was not updated properly. " + obj.result.status);
                    reject();
                }
            }, (err) => {
                this.toastr.error("The bot alias'" + alias["name"] + "' was not updated properly. " + err.message);
                reject();
            });
        }.bind(this));
        return promise;
    }

    /**
     * Delete the bot alias
     * @param alias The alias to delete
	**/
    public deleteAlias(alias): any {
        var promise = new Promise(function (resolve, reject) {
            this._apiHandler.deleteBotAlias(alias["name"], this.name).subscribe(response => {
                let obj = JSON.parse(response.body.text());
                if (obj.result.status == "DELETED") {
                    this.toastr.success("The bot alias '" + alias["name"] + "' was deleted successfully.");
                    resolve();
                }
                else {
                    this.toastr.error("The bot alias'" + alias["name"] + "' was not deleted properly. " + obj.result.status);
                    reject();
                }
            }, (err) => {
                this.toastr.error("The bot alias'" + alias["name"] + "' was not deleted properly. " + err.message);
                reject();
            });
        }.bind(this));
        return promise;
    }

    /**
     * Check the status of the bot
	**/
    public checkBotStatus(): void {
        this._apiHandler.getBotStatus(this.name).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (obj.result.status.indexOf("ERROR") > -1) {
                this.toastr.error("The status of bot '" + this.name + "' did not refresh properly. " + obj.result.status);
            }
            else {
                if (obj.result.status == "BUILDING" || obj.result.status == "PUBLISHING") {
                    this.checkBotStatus();
                }
                else {
                    this.status = obj.result.status;
                }
            }
        }, err => {
            this.toastr.error("The status of bot '" + this.name + "' did not refresh properly. " + err.message);
        });
    }

    /**
     * Generate an object that contains all the necessary information to update a bot
     * @return an object that contains all the necessary information to update a bot
	**/
    private getInfo(): Object {
        let botInfo = {}
        let properties = ["name", "description", "locale", "childDirected", "intents", "idleSessionTTLInSeconds", "voiceId", "checksum", "processBehavior", "clarificationPrompt", "abortStatement"];

        for (let property of properties) {
            if (this[property] != undefined) {
                botInfo[property] = this[property];
            }
        }
        return botInfo;
    }
}
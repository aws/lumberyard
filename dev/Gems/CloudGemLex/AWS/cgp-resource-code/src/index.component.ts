import { Http } from '@angular/http';
import { ViewChild, ElementRef, Input, Component, ViewContainerRef } from '@angular/core';
import { ModalComponent } from 'app/shared/component/index';
import { AwsService } from "app/aws/aws.service";
import { BotEntry, SpeechToTextApi } from './index'
import { Subscription } from 'rxjs/rx';
import { AbstractCloudGemIndexComponent } from 'app/view/game/module/cloudgems/class/index';
import { ToastsManager } from 'ng2-toastr/ng2-toastr';
import { PaginationComponent } from 'app/view/game/module/shared/component/index';
import { ActionItem } from "app/view/game/module/shared/class/index";
import { LyMetricService } from 'app/shared/service/index';

export enum Mode {
    Publish,
    List,
    Dismiss
}

@Component({
    selector: 'speech-to-text-index',
    templateUrl: 'node_modules/@cloud-gems/cloudgemlex/index.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemlex/index.component.css']
})

export class SpeechToTextIndexComponent extends AbstractCloudGemIndexComponent {
    @Input() context: any;
    private subNavActiveIndex = 0;
    private _apiHandler: SpeechToTextApi;

    // define types
    private mode: Mode;
    private sttModes: any;
    private bots: BotEntry[];
    private botsOnCurrentPage: BotEntry[];
    private currentBot: BotEntry;
    private isLoadingBots: boolean;

    private selectedBotsNum;
    private sortDir: string;

    private exportAction = new ActionItem("Export", this.export);
    private publishAction = new ActionItem("Publish", this.publishModal);
    private unpublishedBotActions: ActionItem[] = [
        this.exportAction,
        this.publishAction];
    private publishedBotActions: ActionItem[] = [
        this.exportAction];

    private pageSize: number = 10;
    private Math: any;
    private botPages: number;

    private createBotTip: string;
    private publishBotTip: string;
    private alias: string;

    @ViewChild(ModalComponent) modalRef: ModalComponent;
    @ViewChild(PaginationComponent) paginationRef: PaginationComponent;

    @ViewChild('upload') uploadButtonRef: ElementRef;
    @ViewChild('selectAllBots') selectAllBotsRef: ElementRef;
    @ViewChild('consoleLink') consoleLinkRef: ElementRef;

    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager, vcr: ViewContainerRef, private metric: LyMetricService) {
        super()
        this.toastr.setRootViewContainerRef(vcr);
    }

    // initialize values
    ngOnInit() {
        this._apiHandler = new SpeechToTextApi(this.context.ServiceUrl, this.http, this.aws, this.metric, this.context.identifier);

        // bind proper scopes for callbacks that have a this reference
        this.dismissModal = this.dismissModal.bind(this);
        this.publishModal = this.publishModal.bind(this);
        this.exportAction.onClick = this.exportAction.onClick.bind(this);
        this.publishAction.onClick = this.publishAction.onClick.bind(this);
        // end of bind scopes

        this.sttModes = Mode;
        this.selectedBotsNum = 0;
        this.sortDir == "";
        this.alias = "";
        this.bots = [];
        this.createBotTip = "Create a bot using the configuration file. The process of creating a bot with JSON is described here: https://github.com/awslabs/aws-lex-web-ui/tree/master/templates. The created bots cannot be deleted until the next release.";
        this.publishBotTip = "It could take several minutes to publish the bot.";
        this.update();
    }

    public getSubNavItem(subNavItemIndex: number): void {
        this.subNavActiveIndex = subNavItemIndex;
        this.update();
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
            this._apiHandler.putDesc(body).subscribe(() => {
                this.toastr.success("The bot '" + newBot.name + "' was uploaded successfully.");
                this.update();
            }, (err) => {
                this.toastr.error("The bot '" + newBot.name + "' was not uploaded properly. " + err.message);
                });
        }.bind(this)
        fileReader.readAsText(file);
        inputValue.value = "";
    }

    public update(): void {
        this.bots = [];
        //Empty string gets passed as no value causing API to fail.
        this.updateCurrentPage("t");
    }

    public updateCurrentPage(PageToken: string): void {
        this.sortDir = "asc";
        this.isLoadingBots = true;
        this._apiHandler.getBots(PageToken).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            for (let bot of obj.result.bots) {
                this.bots.push(bot);
                this.checkBotStatus(bot);
            }
            if (obj.result.next_token != "") {
                this.updateCurrentPage(obj.result.nextToken);
            }
            else {
                this.updatePaginationInfo();
                this.sort(0, this.bots.length - 1, this.bots, "bot_name", this.sortDir);
                this.updateBots(1);
                this.isLoadingBots = false;
            }   
        }, err => {
            this.toastr.error("The bots did not refresh properly. " + err.message);
            this.isLoadingBots = false;
        });
    }

    public export(bot): void {
        this._apiHandler.getDesc(bot.bot_name).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            let descFile = obj.result.desc_file;
            let jsonUrl = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(descFile, null, 4));
            let tmpElement = document.createElement('a');
            tmpElement.href = jsonUrl;
            tmpElement.download = bot.bot_name + '.json';
            document.body.appendChild(tmpElement);
            tmpElement.click();
            document.body.removeChild(tmpElement);
            this.toastr.success("The bot '" + bot.bot_name + "' was exported.");
        }, err => {
            this.toastr.error("The bot '" + bot.bot_name + "' was not exported properly. " + err.message);
        });
    }

    public publishModal(bot): void {
        this.mode = Mode.Publish;
        this.currentBot = bot;
        this.alias = "";
    }

    public publish(): void {
        this.modalRef.close();
        this.mode = Mode.List;
        this.currentBot.status = "PUBLISHING";
        this._apiHandler.publishBot(this.currentBot.bot_name, this.alias).subscribe(() => {
            this.toastr.success("The bot '" + this.currentBot.bot_name + "' was published.");
            this.checkBotStatus(this.currentBot);
        }, (err) => {
            this.toastr.error("The bot '" + this.currentBot.bot_name + "' was not published properly. " + err.message);
            this.currentBot.status = "FAILED";
        });
    }

    public sortTable(): void {
        let table = this.bots;
        let key = "bot_name";

        this.sort(0, table.length - 1, table, key, this.getSortOrder(key));
        this.paginationRef.reset();
    }

    public updateSelectedBotsNum(bot): void {
        if (bot.isSelected) {
            this.selectedBotsNum--;
        }
        else {
            this.selectedBotsNum++;
        }
    }

    public updateBots(pageIndex): void {
        let startIndex = (pageIndex - 1) * this.pageSize;
        let endIndex = pageIndex * this.pageSize;
        this.botsOnCurrentPage = this.bots.slice(startIndex, endIndex);
    }

    public dismissModal(bot): void {
        this.mode = Mode.List;
    }

    public openAWSConsole(): void {
        this.consoleLinkRef.nativeElement.href = "https://console.aws.amazon.com/lex/home?region" + this.aws.context.region;
    }

    //Interation with the HTML elements
    public clickUploadButton(): void {
        this.uploadButtonRef.nativeElement.click();
    }

    private getSortOrder(key): string {
        if (this.sortDir == "") {
            this.sortDir = "asc";
        }
        else {
            this.sortDir = this.sortDir == "asc" ? "desc" : "asc";
        }
        return this.sortDir;
    }

    private sort(left, right, table, key, dir): void {
        // Use quick sort algorithm to sort the table by a given column
        let i = left, j = right;
        let pivot = table[Math.floor((left + right) / 2)];

        /* partition */
        while (i <= j) {
            if (dir == "asc") {
                while (table[i][key].toLowerCase() < pivot[key].toLowerCase()) {
                    i++;
                }
                while (table[j][key].toLowerCase() > pivot[key].toLowerCase()) {
                    j--;
                }
            }
            else if (dir == "desc") {
                while (table[i][key].toLowerCase() > pivot[key].toLowerCase())
                    i++;
                while (table[j][key].toLowerCase() < pivot[key].toLowerCase())
                    j--;
            }

            if (i <= j) {
                let tmp = JSON.parse(JSON.stringify(table[i]));
                table[i] = JSON.parse(JSON.stringify(table[j]));
                table[j] = JSON.parse(JSON.stringify(tmp));

                i++;
                j--;
            }
        };

        /* recursion */
        if (left < j)
            this.sort(left, j, table, key, dir);
        if (i < right)
            this.sort(i, right, table, key, dir);
    }

    private updatePaginationInfo() {
        let numberBots: number = this.bots.length;
        this.botPages = Math.ceil(numberBots / this.pageSize);
    }

    private checkBotStatus(bot): void {
        this._apiHandler.getBotStatus(bot.bot_name).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (obj.result.status == "BUILDING" || obj.result.status == "PUBLISHING") {
                this.checkBotStatus(bot);
            }
            else {
                for (let existingbBot of this.bots) {
                    if (existingbBot.bot_name == bot.bot_name) {
                        existingbBot.status = obj.result.status;
                        break;
                    }
                }
            }
        }, err => {
            this.toastr.error("The status of bot '" + bot.bot_name + "' did not refresh properly. " + err.message);
            this.isLoadingBots = false;
        });
    }
}
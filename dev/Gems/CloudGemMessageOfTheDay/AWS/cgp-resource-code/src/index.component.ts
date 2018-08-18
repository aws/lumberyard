import { ViewChild, Input, Component, ViewContainerRef } from '@angular/core';
import { FormControl } from '@angular/forms';
import { DateTimeUtil, StringUtil } from 'app/shared/class/index';
import { ModalComponent } from 'app/shared/component/index';
import { ActionItem } from "app/view/game/module/shared/class/index";
import { Subscription } from 'rxjs/rx';
import { SearchResult } from 'app/view/game/module/shared/component/index';
import { MotdForm, MessageOfTheDayApi} from './index'
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { PaginationComponent } from 'app/view/game/module/shared/component/index';
import { AbstractCloudGemIndexComponent } from 'app/view/game/module/cloudgems/class/index';
import { ToastsManager } from 'ng2-toastr';
import { LyMetricService } from 'app/shared/service/index';

export enum MotdMode { 
    List,
    Edit,
    Delete,
    Add
}

@Component({
    selector: 'message-of-the-day-index',
    templateUrl: 'node_modules/@cloud-gems/cloudgemmessageoftheday/index.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemmessageoftheday/index.component.css']
})
export class MessageOfTheDayIndexComponent extends AbstractCloudGemIndexComponent {
    @Input() context: any;    
    private subNavActiveIndex = 0;
    private _apiHandler: MessageOfTheDayApi;
    private mode: MotdMode;
    private motdModes: any;
    private motdForm: MotdForm;
    private activeMessages: MotdForm[];
    private plannedMessages: MotdForm[];
    private expiredMessages: MotdForm[];

    private isLoadingActiveMessages: boolean;
    private isLoadingPlannedMessages: boolean;
    private isLoadingExpiredMessages: boolean;

    private activeMessagePages: number;
    private plannedMessagePages: number;
    private expiredMessagePages: number;

    private currentMessage: MotdForm;
    private pageSize: number = 5;
    private Math: any;

    @ViewChild(ModalComponent) modalRef: ModalComponent;
    @ViewChild(PaginationComponent) paginationRef: PaginationComponent;

    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager, vcr: ViewContainerRef, private metric: LyMetricService) {
        super()
        this.toastr.setRootViewContainerRef(vcr);        
    }

    ngOnInit() {
        this._apiHandler = new MessageOfTheDayApi(this.context.ServiceUrl, this.http, this.aws, this.metric, this.context.identifier);        
        // bind proper scopes
        this.updateMessages = this.updateMessages.bind(this);
        this.updateActiveMessages = this.updateActiveMessages.bind(this);
        this.updatePlannedMessages = this.updatePlannedMessages.bind(this);
        this.updateExpiredMessages = this.updateExpiredMessages.bind(this);
        this.addModal = this.addModal.bind(this);
        this.editModal = this.editModal.bind(this);
        this.deleteModal = this.deleteModal.bind(this);
        this.dismissModal = this.dismissModal.bind(this);
        this.validate = this.validate.bind(this);        
        this.delete = this.delete.bind(this);
        this.onSubmit = this.onSubmit.bind(this);
        // end of bind scopes

        this.motdModes = MotdMode;
        this.updateMessages();
        this.currentMessage = this.default();
    }

    protected validate(model): boolean {
        let isValid: boolean = true;
        let maxChar = 700;
        model.priority.valid = Number(model.priority.value) >= 0;
        isValid = isValid && model.priority.valid;
        model.message.valid = false;
        if (!model.message.value || !/\S/.test(model.message.value)) {
            model.message.message = "The message was invalid";
        } else if (model.message.value.length < 1) {
            model.message.message = "The message was not long enough."
        } else if (model.message.value.length > 700) {
            model.message.message = "The message was too long. We counted " + model.message.value.length + " characters.  The limit is " + maxChar + " characters."
        } else {
            model.message.valid = true;
        }

        isValid = isValid && model.message.valid && model.datetime.valid;
        
        return isValid;
    }
    
    protected onSubmit(model): void {
        if (!this.validate(model))
            return

        let body = {
            message: model.message.value,
            priority: model.priority.value
        }
        let start = null, end = null;
        if (model.hasStart && model.start.date && model.start.date.year) {
            start = DateTimeUtil.toDate(model.start);
            body['startTime'] = start;
        }
        if (model.hasEnd && model.end.valid) {
            end = DateTimeUtil.toDate(model.end);
            body['endTime'] = end;
        }

        this.modalRef.close();
        if (model.UniqueMsgID) {
            this._apiHandler.put(model.UniqueMsgID, body).subscribe(() => {
                this.toastr.success("The message '" + body.message + "' has been updated.");
                this.updateMessages();

            }, (err) => {
                this.toastr.error("The message did not update correctly. " + err.message)
            });
        } else {
            this._apiHandler.post(body).subscribe(() => {
                this.toastr.success("The message '" + body.message + "' has been created.");
                this.updateMessages();

            }, (err) => {
                this.toastr.error("The message did not update correctly. " + err.message)
            });
        }

        this.mode = MotdMode.List;        
    }

    // TODO: These section are pretty similar and should be extracted into a seperate component
    public updateActiveMessages(currentPageIndex: number): void {
        // Get total messages for pagination
        this._apiHandler.getMessages("active", 0, 1000).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            let numberActiveMessages: number = obj.result.list.length;
            this.activeMessagePages = Math.ceil(numberActiveMessages / this.pageSize);
        });
        this.isLoadingActiveMessages = true;
        this._apiHandler.getMessages("active", (currentPageIndex - 1) * this.pageSize, this.pageSize).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.activeMessages = obj.result.list.sort((msg1, msg2) => {
                if (msg1.priority > msg2.Priority)
                    return 1;
                if (msg1.priority < msg2.priority)
                    return -1;
                return 0;
            });
            this.isLoadingActiveMessages = false;
        }, err => {
            this.toastr.error("The active messages of the day did not refresh properly. " + err);
            this.isLoadingActiveMessages = false;
        });
    }
    public updatePlannedMessages(currentPageIndex: number): void {
        // Get total messages for pagination
        this._apiHandler.getMessages("planned", 0, 1000).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            let numberPlannedMessages: number = obj.result.list.length;
            this.plannedMessagePages = Math.ceil(numberPlannedMessages / this.pageSize);
        });
        this.isLoadingPlannedMessages = true;
        this._apiHandler.getMessages("planned", (currentPageIndex - 1) * this.pageSize, this.pageSize).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.plannedMessages = obj.result.list;
            this.isLoadingPlannedMessages = false;
        }, err => {
            this.toastr.error("The planned messages of the day did not refresh properly. " + err);
            this.isLoadingPlannedMessages = false;
        });
    }
    public updateExpiredMessages(currentPageIndex: number): void {
        // Get total messages for pagination
        let pageSize = this.pageSize * 2
        this._apiHandler.getMessages("expired", 0, 1000).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            let numberExpiredPages: number = obj.result.list.length;
            this.expiredMessagePages = Math.ceil(numberExpiredPages / pageSize)
        });
        this.isLoadingExpiredMessages = true;
        this._apiHandler.getMessages("expired", (currentPageIndex - 1) * pageSize, pageSize).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.expiredMessages = obj.result.list;
            this.isLoadingExpiredMessages = false;
        }, err => {
            this.toastr.error("The expired messages of the day did not refresh properly. " + err);
            this.isLoadingExpiredMessages = false;
        });
    }

    public updateMessages(): void {
        if (this.subNavActiveIndex == 0) {
            this.updateActiveMessages(1);
            this.updatePlannedMessages(1);
        }
        else if (this.subNavActiveIndex == 1) {
            this.updateExpiredMessages(1);
        }
    }

    /* Modal functions for MoTD*/
    public addModal(): void {
        this.currentMessage = this.default();
        this.mode = MotdMode.Add;
    }

    public editModal(model): void {
        let startDate = null;
        let endDate = null;
        var hasStart = true;
        var hasEnd = true;

        // If the message already has a start date, use that.  Otherwise we should indicate there isn't one by setting the hasStart flag here.  Same case for hasEnd
        if (model.startTime) {
            startDate = new Date(model.startTime);
        } else {
            startDate = new Date();
            hasStart = false;
        }

        if (model.endTime) {
            endDate = new Date(model.endTime);
        } else {
            endDate = new Date();
            hasEnd = false;
        }

        let editModel = new MotdForm({
            UniqueMsgID: model.UniqueMsgID,
            start: {
                date: {
                    year: startDate.getFullYear(),
                    month: startDate.getMonth() + 1,
                    day: startDate.getDate(),
                },
                time: (hasStart) ? { hour: startDate.getHours(), minute: startDate.getMinutes() } : { hour: 12, minute: 0 },
                valid: true,
                isScheduled: startDate.getFullYear() > 0 ? true : false
            },
            end: {
                date: {
                    year: endDate.getFullYear(),
                    month: endDate.getMonth() + 1,
                    day: endDate.getDate(),
                },
                time: (hasEnd) ? { hour: endDate.getHours(), minute: endDate.getMinutes() } : { hour: 12, minute: 0 },
                valid: true,
                isScheduled: startDate.getFullYear() > 0 ? true : false
            },
            priority: {
                valid: true,
                value: model.priority
            },
            message: {
                valid: true,
                value: model.message
            },
            hasStart: hasStart,
            hasEnd: hasEnd
        });

        this.currentMessage = editModel;
        this.mode = MotdMode.Edit;
    }
    public deleteModal(model): void {
        model.end = {};
        model.start = {};
        this.mode = MotdMode.Delete;
        this.currentMessage = model;
    }

    public dismissModal(): void {
        this.mode = MotdMode.List;
    }

    public getSubNavItem(subNavItemIndex: number): void {
        this.subNavActiveIndex = subNavItemIndex;
        this.updateMessages();
    }


    /* form function */
    public delete(): void {
        this.modalRef.close();
        this.mode = MotdMode.List;
        this._apiHandler.delete(this.currentMessage.UniqueMsgID).subscribe(response => {
            this.toastr.success("The message '" + this.currentMessage.message + "'" + "was deleted");
            this.paginationRef.reset();
            this.updateMessages();
        }, err => {
            this.toastr.error("The message '" + this.currentMessage.message + "' did not delete. " + err);
        });
    }

    protected dprModelUpdated(model): void {
        this.currentMessage.hasStart = model.hasStart;
        this.currentMessage.hasEnd = model.hasEnd;

        this.currentMessage.start = model.start;
        this.currentMessage.start.valid = model.start.valid;
        this.currentMessage.end = model.end;
        this.currentMessage.end.valid = model.end.valid;
        this.currentMessage.datetime = {
            valid: model.valid,
            message: model.date.message
        }
    }

    private default(): MotdForm {
        var start = this.getDefaultStartDateTime();
        var end = this.getDefaultEndDateTime();
        return new MotdForm({
            start: start,
            end: end,
            priority: {
                valid: true,
                value: 0
            },
            message: {
                valid: true
            },           
            hasStart: false,
            hasEnd: false
        });
    }

    private getDefaultStartDateTime() {
        var today = new Date();
        return {
            date: {
                year: today.getFullYear(),
                month: today.getMonth() + 1,
                day: today.getDate()
            },
            time: { hour: 12, minute: 0 },
            valid: true
        }
    };

    private getDefaultEndDateTime() {
        var today = new Date();
        return {
            date: {
                year: today.getFullYear(),
                month: today.getMonth() + 1,
                day: today.getDate() + 1
            },
            time: { hour: 12, minute: 0 },
            valid: true
        }
    }

    private substring(string: string, length: number) {
        return StringUtil.toEtcetera(string, length)
    }
}
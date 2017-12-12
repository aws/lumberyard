import { Observable } from 'rxjs/rx';
import { Http } from '@angular/http';
import { ViewChild, SecurityContext } from '@angular/core';
import { Tackable, Measurable, Stateable, StyleType, Gemifiable, TackableStatus, TackableMeasure } from 'app/view/game/module/cloudgems/class/index';
import { DynamicGem } from 'app/view/game/module/cloudgems/class/index';
import { DateTimeUtil, ApiHandler, StringUtil } from 'app/shared/class/index';
import { ModalComponent } from 'app/shared/component/index';
import { PaginationComponent } from 'app/view/game/module/shared/component/index';
import { AwsService } from "app/aws/aws.service";

declare const toastr: any;

export enum MotdMode {
    List,
    Edit,
    Delete,
    Add
}

export class Motd extends DynamicGem {
    public identifier = "motd";
    public subNavActiveIndex = 0;
    public apiHandler: MotdAPI;
    // Submenu item section
    subMenuItems = [
        "Overview",
        "History"
    ];

    public classType(): any {
        const clone = Motd
        return clone;
    }

    //Required.  Firefox will complain if the constructor is not present with no arguments
    constructor() {
        super();
    }

    public initialize(context: any) {
        super.initialize(context);
        this.apiHandler = new MotdAPI(this.context.meta.cloudFormationOutputMap["ServiceUrl"].outputValue, this.context.provider.http, this.context.provider.aws);
        this.apiHandler.report(this.tackable.metric);
        this.apiHandler.assign(this.tackable.state);
    }

    public tackable: Tackable = <Tackable>{
        displayName: "Message of the day",
        iconSrc: "https://m.media-amazon.com/images/G/01/cloudcanvas/images/message_of_the_day._V536715120_.png",
        cost: "Low",
        state: new TackableStatus(),
        metric: new TackableMeasure()
    };

    public get htmlTemplateUrl(): string {
        return this.context.provider.definition.isProd ? this.getSignedUrl(this.context.meta.key + "/cloudgemmessageoftheday.html") : "external/cloudgemmessageoftheday/cloudgemmessageoftheday.html";
    }
    public get styleTemplateUrl(): string {
        return this.context.provider.definition.isProd ? this.getSignedUrl(this.context.meta.key + "/cloudgemmessageoftheday.css") : "external/cloudgemmessageoftheday/cloudgemmessageoftheday.css";
    }

    // define types
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

    ngOnInit() {
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
        this.validateDate = this.validateDate.bind(this);
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
        if (!model.message.value || !/\S/.test(model.message.value))  {
            model.message.message = "The message was invalid";
        } else if(model.message.value.length < 1 ) {
                model.message.message = "The message was not long enough."
        } else if (model.message.value.length > 700) {
            model.message.message = "The message was too long. We counted " + model.message.value.length + " characters.  The limit is " + maxChar+" characters."
        } else {
            model.message.valid = true;
        }

        isValid = isValid && model.message.valid;

        isValid = isValid && this.validateDate(model);

        return isValid;
    }

    private validateDate(model): boolean {
        let isValid: boolean = true;

        let start = DateTimeUtil.toObjDate(model.start);
        let end = DateTimeUtil.toObjDate(model.end);

        if (this.currentMessage.hasEnd) {
            model.end.valid = model.end.date && model.end.date.year && model.end.date.month && model.end.date.day ? true : false
            if (!model.end.valid)
                model.date = { message: "The end date is not a valid date." }

            isValid = isValid && model.end.valid;
        }
        if (this.currentMessage.hasStart) {
            model.start.valid = model.start.date.year && model.start.date.month && model.start.date.day ? true : false
            if (!model.start.valid)
                model.date = { message: "The end date is not greater than today." }
            isValid = isValid && model.start.valid;
        }

        if (this.currentMessage.hasEnd && this.currentMessage.hasStart) {
            let isValidDateRange = (start < end);
            isValid = isValid && isValidDateRange
            if (!isValidDateRange) {
                model.date = { message: "The start date must be less than the end date." }
                model.start.valid = false;
            }
        }
        return isValid;
    }

    protected onSubmit(model): void {
        if (this.validate(model)) {
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
            toastr.info("Saving message.")

            this.modalRef.close();
            if(model.UniqueMsgID) {
                this.apiHandler.put(model.UniqueMsgID, body).subscribe(() => {
                    toastr.clear();
                    toastr.success("The message '" + body.message + "' has been updated.");
                    this.updateMessages();

                }, (err) => {
                    toastr.clear();
                    toastr.error("The message did not update correctly. "+err.message)
                });
            } else {
                this.apiHandler.post(body).subscribe(() => {
                    toastr.clear();
                    toastr.success("The message '" + body.message + "' has been created.");
                    this.updateMessages();

                }, (err) => {
                    toastr.clear();
                    toastr.error("The message did not update correctly. "+err.message)
                });
            }

            this.mode = MotdMode.List;
        }
    }

    // TODO: These section are pretty similar and should be extracted into a seperate component
    public updateActiveMessages(currentPageIndex: number): void {
        // Get total messages for pagination
        this.apiHandler.getMessages("active", 0, 1000).subscribe( response => {
            let obj = JSON.parse(response.body.text());
            let numberActiveMessages: number = obj.result.list.length;
            this.activeMessagePages = Math.ceil(numberActiveMessages / this.pageSize);
        });
        this.isLoadingActiveMessages = true;
        this.apiHandler.getMessages("active", (currentPageIndex - 1) * this.pageSize, this.pageSize).subscribe(response => {
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
            toastr.error("The active messages of the day did not refresh properly. " + err);
            this.isLoadingActiveMessages = false;
        });
    }
    public updatePlannedMessages(currentPageIndex: number): void {
        // Get total messages for pagination
        this.apiHandler.getMessages("planned", 0, 1000).subscribe( response => {
            let obj = JSON.parse(response.body.text());
            let numberPlannedMessages: number = obj.result.list.length;
            this.plannedMessagePages = Math.ceil(numberPlannedMessages / this.pageSize);
        });
        this.isLoadingPlannedMessages = true;
        this.apiHandler.getMessages("planned", (currentPageIndex - 1) * this.pageSize, this.pageSize).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.plannedMessages = obj.result.list;
            this.isLoadingPlannedMessages = false;
        }, err => {
            toastr.error("The planned messages of the day did not refresh properly. " + err);
            this.isLoadingPlannedMessages = false;
        });
    }
    public updateExpiredMessages(currentPageIndex: number): void {
         // Get total messages for pagination
        let pageSize = this.pageSize * 2
        this.apiHandler.getMessages("expired", 0, 1000).subscribe( response => {
            let obj = JSON.parse(response.body.text());
            let numberExpiredPages: number = obj.result.list.length;
            this.expiredMessagePages = Math.ceil(numberExpiredPages / pageSize)
        });
        this.isLoadingExpiredMessages = true;
        this.apiHandler.getMessages("expired", (currentPageIndex - 1) * pageSize, pageSize ).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.expiredMessages = obj.result.list;
            this.isLoadingExpiredMessages = false;
        }, err => {
            toastr.error("The expired messages of the day did not refresh properly. " + err);
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

    public editModal(model) : void {
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
    public deleteModal(model) : void {
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
        toastr.info("Deleting");
        this.apiHandler.delete(this.currentMessage.UniqueMsgID).subscribe(response => {
            toastr.clear();
            toastr.success("The message '" + this.currentMessage.message + "'");
            this.paginationRef.reset();
            this.updateMessages();
        }, err => {
            toastr.error("The message '" + this.currentMessage.message + "' did not delete. " + err);
        });
    }

    protected dprModelUpdated(model): void {
        this.currentMessage.hasStart = model.hasStart;
        this.currentMessage.hasEnd = model.hasEnd;

        this.currentMessage.start = model.start;
        this.currentMessage.start.valid = true;
        this.currentMessage.end = model.end;
        this.currentMessage.end.valid = true;
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

export class MotdAPI extends ApiHandler {
    constructor(serviceBaseURL: string, http: Http, aws: AwsService) {
        super(serviceBaseURL, http, aws);
    }

    public report(metric: Measurable) {
        metric.name = "Total Active Messages";
        metric.value = "Loading...";
        super.get("admin/messages?count=500&filter=active&index=0").subscribe(response => {
            let obj = JSON.parse(response.body.text());
            metric.value = obj.result.list.length;
        });
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

    public delete(id: string): Observable<any> {
        return super.delete("admin/messages/" + id);
    }

    public put(id: string, body: any): Observable<any> {
        return super.put("admin/messages/" + id, body);
    }

    public post(body: any): Observable<any> {
        return super.post("admin/messages/", body);
    }

    public getMessages(filter: string, startIndex: number, count: number): Observable<any> {
        return super.get("admin/messages?count=" + count + "&filter=" +
            filter + "&index=" + startIndex);
    }
}

class MotdForm {
    UniqueMsgID: string;
    message: string;
    priority: number;
    start?: any;
    end?: any;
    hasStart?: boolean;
    hasEnd?: boolean;

    constructor(motdInfo: any) {
        this.UniqueMsgID = motdInfo.UniqueMsgID;
        this.start = motdInfo.start;
        this.end = motdInfo.end;
        this.message = motdInfo.message;
        this.priority = motdInfo.priority;
        this.hasStart = motdInfo.hasStart;
        this.hasEnd = motdInfo.hasEnd;
    }

    public clearStartDate() {
        this.start = {
            date: {}
        };
    }

}

export function getInstance(context: any): Gemifiable {
    let gem = new Motd();
    gem.initialize(context);
    return gem;
}

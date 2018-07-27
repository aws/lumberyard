import { ViewChild, Input, Output, Component, EventEmitter } from '@angular/core';
import { AbstractCloudGemIndexComponent } from 'app/view/game/module/cloudgems/class/index';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { LyMetricService } from 'app/shared/service/index';
import { CloudGemDefectReporterApi } from './index';
import { ToastsManager } from 'ng2-toastr/ng2-toastr';
import { ModalComponent } from 'app/shared/component/index';

export enum ReportDetailMode {
    Show,
    AddComment,
    EditComment
} 

@Component({
    selector: 'defect-details-page',
    templateUrl: 'node_modules/@cloud-gems/cloudgemdefectreporter/defect-details-page.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemdefectreporter/defect-details-page.component.css']
})

export class CloudGemDefectReporterDefectDetailsPageComponent {    
    @Input() context: any;
    @Input() toDefectListPageCallback: Function;
    @Input() defect: Object;
    @Input() configurationMappings: any;

    private _apiHandler: CloudGemDefectReporterApi;
    private mode: ReportDetailMode;
    private Modes: any;

    private isLoading: boolean = false;
    private currentComment: Object;
    private currentCommentIndex: number;

    private rawDataKeys: string[];  

    @ViewChild(ModalComponent) modalRef: ModalComponent;
  

    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager, private metric: LyMetricService) {
    }
  

    ngOnInit() {
        this._apiHandler = new CloudGemDefectReporterApi(this.context.ServiceUrl, this.http, this.aws, this.metric, this.context.identifier);

        this.loadDefect();
        this.Modes = ReportDetailMode;
    }

    /**
    * Add or remove a bookmark
    **/
    public changeBookmarkStatus(): void {
        this.defect['bookmark'] = this.defect['bookmark'] === 0 ? 1 : 0;
        this.updatePropertyValue('bookmark', this.defect['bookmark']);

        this._apiHandler.updateReportHeader(this.defect).subscribe(response => {
        }, err => {
            this.toastr.error("Failed to update bookmarks. " + err.message);
        });
    }

    /**
    * Mark the report as read/unread
    **/
    public changeReportStatus(): void {
        this.defect['report_status'] = this.defect['report_status'] === 'read' ? 'unread' : 'read';
        this.updatePropertyValue('report_status', this.defect['report_status']);
     
        this._apiHandler.updateReportHeader(this.defect).subscribe(response => {
        }, err => {
            this.toastr.error("Failed to update the status of the current report. " + err.message);
        });
    }

    /**
    * Get the status of the report and to update the button
    **/
    public getReportStatus(): string {
        return this.defect['report_status'] === 'read' ? "Mark as Unread" : "Mark as Read"
    }

    /**
    * Get the status of the bookmark and to update the button
    **/
    public getBookmarkStatus(): string {
        return this.defect['bookmark'] === 0 ? "Add to Bookmarks" : "Remove from Bookmarks"
    }

    /**
    * Add a new comment to the report
    **/
    public addComment(): void {
        this.modalRef.close();

        let today = new Date();
        let comment = { date: today.toString(), content: this.currentComment['content'], user: this.currentComment['user'] };
        this.defect["comments"].push(comment);

        this._apiHandler.updateReportComments(this.defect).subscribe(response => {
        }, err => {
            this.toastr.error("Failed to add the comment. " + err.message);
        });
    }

    /**
    * Update an existing comment to the report
    **/
    public updateComment(): void {
        this.modalRef.close();

        let today = new Date();
        this.defect["comments"][this.currentCommentIndex] = { date: today.toString(), content: this.currentComment['content'], user: this.currentComment['user'] };

        this._apiHandler.updateReportComments(this.defect).subscribe(response => {
        }, err => {
            this.toastr.error("Failed to add the comment. " + err.message);
        });
    }

    /**
    * Delete the selected comment
    * @param index Index of the comment to delete
    **/
    public deleteComment(index): void {
        this.defect["comments"].splice(index, 1);

        this._apiHandler.updateReportComments(this.defect).subscribe(response => {
        }, err => {
            this.toastr.error("Failed to delete the comment. " + err.message);
        });
    }

    /**
    * Define AddComment modal
    **/
    public addCommentModal = () => {
        this.currentComment = { user: this.aws.context.authentication.user.username };
        this.mode = ReportDetailMode.AddComment;
    }

    /**
    * Define EditComment modal
    **/
    public editCommentModal = (index: number) => {
        this.currentComment = JSON.parse(JSON.stringify(this.defect["comments"][index]));
        this.currentCommentIndex = index;
        this.mode = ReportDetailMode.EditComment;
    }

    /**
    * Define Dismiss modal
    **/
    public dismissModal = () => {
        this.mode = ReportDetailMode.Show;
    }

    /**
    * Loads a defect from list
    * @param defect the defect report to load
    **/
    private loadDefect() {
        this.isLoading = true;
        for (let mapping of this.configurationMappings) {
            mapping['value'] = undefined;
            for (let key of mapping['key'].split('.')) {
                mapping['value'] = this.defect[key];
                if (!mapping['value']) {
                    break;
                }
            }
        }

        this.rawDataKeys = Object.keys(this.defect);
        this.removePropertyNameFromRawDataKeys('prop');
        this.removePropertyNameFromRawDataKeys('comments');
        this.removePropertyNameFromRawDataKeys('bookmark');
        this.removePropertyNameFromRawDataKeys('report_status');
        this.getAttachments();
        this.getComments();
    }

    /**
    * Get the attachments of the current report
    **/
    private getAttachments() {
        let attachmentList = this.defect["attachment_id"] ? JSON.parse(this.defect["attachment_id"]) : [];
        for (let attachment of attachmentList) {
            this.defect[attachment["name"] + "_attachment"] = { "id": attachment["id"], 'extension': attachment["extension"] };
            this.removePropertyNameFromRawDataKeys(attachment["name"] + "_attachment");
        }
    }

    /**
    * Get the comments of the current report
    **/
    public getComments(): void {
        this._apiHandler.getReportComments(this.defect['universal_unique_identifier']).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.defect['comments'] = obj.result.comments;
            this.isLoading = false;
        }, err => {
            this.isLoading = false;
            this.toastr.error("Failed to get comments. " + err.message);
        });
    }

    /**
    * Use function to call out into index.component.ts when handling the onShow click
    **/
    private onShowDefectListPage = () => {
        if (this.toDefectListPageCallback) {
            this.toDefectListPageCallback();
        } 
    }

    /**
    * Remove the additional property from the raw data on display
    * @param key the key to remove
    **/
    private removePropertyNameFromRawDataKeys(key: string): void {
        let index = this.rawDataKeys.indexOf(key);
        if (index !== -1) {
            this.rawDataKeys.splice(index, 1);
        }
    }

    /**
    * Update the property value in the mappings
    * @param key the property to update
    * @param newValue the new value for the property
    **/
    private updatePropertyValue(key: string, newValue: string): void {
        for (let mapping of this.configurationMappings) {
            if (mapping['key'] === key) {
                mapping['value'] = newValue;
                break;
            }
        }
    }
}
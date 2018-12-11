import { ViewChild, Input, Component, Output, EventEmitter } from '@angular/core';
import { LyMetricService } from 'app/shared/service/index';
import { AwsService } from "app/aws/aws.service";
import { CloudGemDefectReporterApi } from './index';
import { ToastsManager } from 'ng2-toastr';
import { ModalComponent } from 'app/shared/component/index';

export enum ReportDetailMode {
    Show,
    AddComment,
    EditComment,
    CreateJiraTicket
} 

@Component({
    selector: 'defect-detail-overview-tab',
    templateUrl: 'node_modules/@cloud-gems/cloudgemdefectreporter/defect-detail-overview-tab.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemdefectreporter/defect-details-page.component.css']
})


export class CloudGemDefectReporterDefectDetailOverviewTabComponent {
    @Input() defect: any;
    @Input() isLoading: any;
    @Input() isJiraIntegrationEnabled: any;
    @Input() configurationMappings: any;
    @Input() context: any;
    @Input() toDefectListPageCallback: any;
    @Input() defectReporterApiHandler: any;
    @Output() updateJiraMappings = new EventEmitter<any>();

    private mode: ReportDetailMode;
    private Modes: any;

    private currentComment: Object;
    private currentCommentIndex: number;
    private currentReport: Object;
    private currentDefect: Object;
    private isLoadingjiraServer = false;

    @ViewChild(ModalComponent) modalRef: ModalComponent;
    @ViewChild('CreateJiraIssueWindow') createJiraIssueWindow;

    constructor(private aws: AwsService, private toastr: ToastsManager, private metric: LyMetricService) {
    }

    ngOnInit() {
        this.Modes = ReportDetailMode;
    }

    /**
    * Add or remove a bookmark
    **/
    public changeBookmarkStatus(): void {
        this.defect['bookmark'] = this.defect['bookmark'] === 0 ? 1 : 0;
        this.updatePropertyValue('bookmark', this.defect['bookmark']);

        this.updateReportHeader("Failed to update bookmarks. ");
    }

    /**
    * Mark the report as read/unread
    **/
    public changeReportStatus(): void {
        this.defect['report_status'] = this.defect['report_status'] === 'read' ? 'unread' : 'read';
        this.updatePropertyValue('report_status', this.defect['report_status']);

        this.updateReportHeader("Failed to update the status of the current report. ");
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
    * Define EditComment modal
    **/
    public editCommentModal = (index: number) => {
        this.currentComment = JSON.parse(JSON.stringify(this.defect["comments"][index]));
        this.currentCommentIndex = index;
        this.mode = ReportDetailMode.EditComment;
    }

    /**
    * Update an existing comment to the report
    **/
    public updateComment(): void {
        this.modalRef.close();

        let today = new Date();
        this.defect["comments"][this.currentCommentIndex] = { date: today.toString(), content: this.currentComment['content'], user: this.currentComment['user'] };

        this.defectReporterApiHandler.updateReportComments(this.defect).subscribe(response => {
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

        this.defectReporterApiHandler.updateReportComments(this.defect).subscribe(response => {
        }, err => {
            this.toastr.error("Failed to delete the comment. " + err.message);
        });
    }

    /**
    * Add a new comment to the report
    **/
    public addComment(): void {
        this.modalRef.close();

        let today = new Date();
        let comment = { date: today.toString(), content: this.currentComment['content'], user: this.currentComment['user'] };
        this.defect["comments"].push(comment);

        this.defectReporterApiHandler.updateReportComments(this.defect).subscribe(response => {
        }, err => {
            this.toastr.error("Failed to add the comment. " + err.message);
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
    * Define CreateJiraTicket modal
    **/
    public createJiraTicketModal = () => {
        this.currentReport = {};
        for (let key of Object.keys(this.defect)) {
            try {
                this.currentReport[key] = { value: JSON.parse(this.defect[key]), valid: true, message: `Required field cannot be empty.` }
            } catch (e) {
                this.currentReport[key] = { value: this.defect[key], valid: true, message: `Required field cannot be empty.` }
            }
        }

        this.mode = ReportDetailMode.CreateJiraTicket;
    }

    /**
    * Define Dismiss modal
    **/
    public dismissModal = () => {
        this.mode = ReportDetailMode.Show;
    }

    /**
    * View the Jira ticket which was created based on the current report
    **/
    private viewJiraIssue(): void {
        this.isLoadingjiraServer = true;
        this.defectReporterApiHandler.getJiraCredentialsStatus().subscribe(response => {
            let obj = JSON.parse(response.body.text());
            if (!obj.result.exist) {
                this.toastr.error("Failed to connect to Jira. The Jira credentials don't exist");
            }
            else {
                window.open(obj.result.server + "/browse/" + this.defect['jira_status'], "_blank");
            }
            this.isLoadingjiraServer = false;
        }, err => {
            this.toastr.error("Failed to connect to Jira. " + err.message);
            this.isLoadingjiraServer = false;
        });
    }

    /**
    * Create a new Jira ticket based on the current defect report
    **/
    private createJiraIssue(): void {
        if (this.createJiraIssueWindow.validateJiraFields()) {
            this.modalRef.close();

            this.defectReporterApiHandler.createJiraIssue([this.createJiraIssueWindow.retriveReportData(this.currentDefect)]).subscribe(response => {
                this.getReportJiraIssueNumber();
                this.toastr.success("A new Jira ticket was created successfully.");
            }, err => {
                this.toastr.error("Failed to create a new Jira ticket. " + err.message);
            });
        }
    }

    /**
    * Get the Jira issue number of the current report
    **/
    private getReportJiraIssueNumber(): void {
        this.defectReporterApiHandler.getReportHeaders().subscribe(response => {
            let obj = JSON.parse(response.body.text());
            let reportHeaders = obj.result;
            for (let reportHeader of reportHeaders) {
                if (reportHeader['universal_unique_identifier'] === this.defect['universal_unique_identifier']) {
                    this.defect['jira_status'] = reportHeader['jira_status'];
                    break;
                }
            }
        })
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


    /**
    * Update the report header information
    * @param errorMessage the message to display when the Api call fails
    **/
    private updateReportHeader(errorMessage): void {
        this.defectReporterApiHandler.updateReportHeader(this.defect).subscribe(response => {
        }, err => {
            this.toastr.error(errorMessage + err.message);
        });
    }

    /**
    * Send the event to update Jira integration settings
    **/
    private updateJiraIntegrationSettings(): void {
        this.modalRef.close();
        this.updateJiraMappings.emit();
    }

    /**
    * Close the Create Jira Issue window
    **/
    private closeCreateJiraIssueWindow(): void {
        this.modalRef.close();
        this.dismissModal();
    }

    private updateReportsToSubmitList(defect): void {
        this.currentDefect = defect;
    }
}
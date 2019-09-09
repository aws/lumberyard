import { Input, Output, Component, EventEmitter } from '@angular/core';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { LyMetricService } from 'app/shared/service/index';
import { CloudGemDefectReporterApi } from './index';
import { ToastsManager } from 'ng2-toastr/ng2-toastr';

class Attachment {
    id: string;
    extension: string;
    url: string;

    constructor(id: string, extension: string, url?: string) {
        this.id = id;
        this.extension = extension;
        this.url = url;
    }
}
export class TextAttachment extends Attachment{
    constructor(id: string, extension: string, url?: string) {
        super(id, extension, url);
    }
 }
export class ImageAttachment extends Attachment{
    constructor(id: string, extension: string, url?: string) {
        super(id, extension, url);
    }
}

@Component({
    selector: 'defect-details-page',
    templateUrl: 'node_modules/@cloud-gems/cloudgemdefectreporter/defect-details-page.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemdefectreporter/defect-details-page.component.css',
                'node_modules/@cloud-gems/cloudgemdefectreporter/create-jira-issue-window.component.css']
})


export class CloudGemDefectReporterDefectDetailsPageComponent {
    @Input() context: any;
    @Input() toDefectListPageCallback: Function;
    @Input() isJiraIntegrationEnabled: any;
    // Should create an actual Defect class and not use any
    @Input() defect: any;
    @Input() configurationMappings: any;;
    @Output() updateJiraMappings = new EventEmitter<any>();

    private _apiHandler: CloudGemDefectReporterApi;

    private isLoadingDefectDetails = false;

    private rawDataKeys: string[];

    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager, private metric: LyMetricService) {
    }


    ngOnInit() {
        this._apiHandler = new CloudGemDefectReporterApi(this.context.ServiceUrl, this.http, this.aws, this.metric, this.context.identifier);
        this.loadDefect();
    }

    /**
    * Loads a defect from list
    * @param defect the defect report to load
    **/
    private loadDefect() {
        this.isLoadingDefectDetails = true;
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
        if (this.defect['jira_status'] === 'filed') {
            this.getReportJiraIssueNumber();
        }
    }

    /**
    * Get the attachments of the current report
    **/
    private getAttachments() {
        let attachmentList = this.defect["attachment_id"] ? JSON.parse(this.defect["attachment_id"]) : [];
        // Appending the keys here but should use a proper model in the future
        this.defect.imageAttachments = new Array<ImageAttachment>();
        this.defect.textAttachments = new Array<TextAttachment>();
        for (let attachment of attachmentList) {

            // If this is a text file add it to our text attachments array, otherwise assume it's an image and use the other array
            if (attachment.extension === "txt") {
                this.defect.textAttachments.push(new TextAttachment(attachment.id, attachment.extension));
            } else {
                this.defect.imageAttachments.push(new ImageAttachment(attachment.id, attachment.extension));
            }
        }
    }

    /**
    * Get the comments of the current report
    **/
    public getComments(): void {
        this._apiHandler.getReportComments(this.defect['universal_unique_identifier']).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.defect['comments'] = obj.result.comments;
            this.isLoadingDefectDetails = false;
        }, err => {
            this.isLoadingDefectDetails = false;
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
    * Get the Jira issue number of the current report
    **/
    private getReportJiraIssueNumber(): void {
        this._apiHandler.getReportHeaders().subscribe(response => {
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
}
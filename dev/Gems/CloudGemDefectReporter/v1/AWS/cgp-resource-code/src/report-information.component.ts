import { Input, Output, Component, EventEmitter } from '@angular/core';
import { LyMetricService } from 'app/shared/service/index';
import { AwsService } from "app/aws/aws.service";
import { ToastsManager } from 'ng2-toastr/ng2-toastr';

@Component({
    selector: 'report-information',
    templateUrl: 'node_modules/@cloud-gems/cloudgemdefectreporter/report-information.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemdefectreporter/defect-details-page.component.css']
})


export class CloudGemDefectReporterReportInformationComponent {
    @Input() context: any;
    @Input() reportInformation: any;
    @Input() configurationMappings: any;
    @Output() deleteComment = new EventEmitter<any>();
    @Output() editComment = new EventEmitter<any>();

    private customizedReportPropertyMappings: Object;

    constructor(private aws: AwsService, private toastr: ToastsManager, private metric: LyMetricService) {   
    }

    ngOnInit() {
        this.reportInformation['report_time'] = this.reportInformation.p_server_timestamp_month + '/' + this.reportInformation.p_server_timestamp_day + '/' + this.reportInformation.p_server_timestamp_year + this.reportInformation.p_server_timestamp_hour + ':00:00';
        this.getScreenshotUrl();   
    }

    /**
    * Get the presigned URL of the screenshot
    **/
    private getScreenshotUrl(): void {
        let key = this.reportInformation.screenshot_attachment["id"];
        let signedUrl = this.aws.context.s3.getSignedUrl('getObject', {
            Bucket: this.context.SanitizedBucketName, Key: key, Expires: 600
        })
        this.reportInformation.screenshot_attachment["url"] = signedUrl;
    }

    /**
    * Download an attachment
    * @param attachment the attachment to download
    **/
    private downloadAttachment(attachment): void {
        let response = this.aws.context.s3.getObject({ Bucket: this.context.SanitizedBucketName, Key: attachment["id"] }, function (err, data) {
            if (err) {
                this.toastr.error(`The attachment failed to download. The received error was '${err.message}'`);
            }
            else {
                var blob = new Blob([data.Body], { type: data.ContentType });

                if (window.navigator.msSaveOrOpenBlob) //Edge
                {
                    window.navigator.msSaveBlob(blob, attachment["id"] + '.' + attachment["extension"]);
                }
                else //Chrome & FireFox
                {
                    let a = document.createElement("a");
                    let fileURL = URL.createObjectURL(blob);
                    a.href = fileURL;
                    a.download = attachment["id"] + '.' + attachment["extension"];
                    window.document.body.appendChild(a);
                    a.click();
                    window.document.body.removeChild(a);
                    URL.revokeObjectURL(fileURL);
                }
            }
        }.bind(this))
    }
}
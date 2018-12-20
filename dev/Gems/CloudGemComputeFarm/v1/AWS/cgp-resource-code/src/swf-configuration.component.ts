import { Input, Component, ViewChild, ElementRef } from '@angular/core';
import { CloudGemComputeFarmApi } from './index'
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { ModalComponent } from 'app/shared/component/index';
import { ToastsManager } from 'ng2-toastr';
import { WorkflowConfig } from './workflow-config'

export enum Mode {
    Show,
    Upload,
    UploadProgress,
    Download
}

@Component({
    selector: 'swf-configuration',
    templateUrl: 'node_modules/@cloud-gems/cloudgemcomputefarm/swf-configuration.component.html',
    styleUrls: [
        'node_modules/@cloud-gems/cloudgemcomputefarm/swf-configuration.component.css',
        'node_modules/@cloud-gems/cloudgemcomputefarm/index.component.css'
    ]
})

export class SWFConfigurationComponent {
    @Input() context: any;
    @Input() workflowConfig: WorkflowConfig;
    private _apiHandler: CloudGemComputeFarmApi;

    private s3Path = "";
    private uploadFileInfo: any;
    private displayUploadError: boolean = false;

    private configurationProperties: { name: string, type: string }[] = [];

    private buildResults: Object[];
    private selectAllResults: boolean = false;

    private mode: Mode = Mode.Show;
    private SwfModes: any;

    @ViewChild('buildResultModal') buildResultModalRef: ModalComponent;
    @ViewChild('uploadModal') uploadModalRef: ModalComponent;
    @ViewChild('upload') uploadButtonRef: ElementRef;

    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager) {
    }

    ngOnInit() {
        this._apiHandler = new CloudGemComputeFarmApi(this.context.ServiceUrl, this.http, this.aws);

        this.SwfModes = Mode;

        this.initializeSWFConfiguration();
    }

    /**
     * Define the download build results modal
    **/
    public downloadBuildResultsModal = () => {
        let prefix = this.workflowConfig.s3_dir.length ? (this.workflowConfig.s3_dir + "/") : "";

        var params = {
            Bucket: this.context.ComputeFarmBucketName,
            Prefix: prefix
        };
        this.aws.context.s3.listObjects(params, function (err, data) {
            if (err) {
                this.toastr.error("Failed to list all the build results. " + err.message);
            }
            else {
                this.buildResults = [];
                for (let result of data["Contents"]) {
                    if (result["Size"] !== 0) {
                        this.buildResults.push({ key: result["Key"], isSelected: false });
                    }
                }
                this.mode = Mode.Download;
            }
        }.bind(this));
    }

    /**
     * Download the build results
	**/
    public downloadBuildResults(): void {
        for (let result of this.buildResults) {
            if (result["isSelected"]) {
                var params = {
                    Bucket: this.context.ComputeFarmBucketName,
                    Key: result["key"]
                };
                var url = this.aws.context.s3.getSignedUrl('getObject', params);
                window.open(url);
            }
        }

        this.buildResultModalRef.close();
    }

    /**
     * Define the dismiss modal
    **/
    public dismissModal = ()=> {
        if (this.mode != Mode.UploadProgress) {
            this.mode = Mode.Show;
        }
    }

    /**
     * Select/Unselect all the build results
    **/
    public selectAll(): void {
        for (let result of this.buildResults) {
            result["isSelected"] = this.selectAllResults;
        }
    }

    /**
     * Open the upload modal
     */
    public uploadModal(): void {
        this.displayUploadError = false;
        this.mode = Mode.Upload;
    }

    public submitUpload = ()=> {
        if (!this.uploadFileInfo || !this.s3Path.length) {
            this.displayUploadError = true;
        }
        else {
            var params = { Bucket: this.context.ComputeFarmBucketName, Key: this.s3Path, Body: this.uploadFileInfo };
            this.aws.context.s3.upload(params, function (err, data) {
                if (data) {
                    this.toastr.success("File uploaded successfully.")
                }
                else {
                    this.toastr.error("Failed to upload file. " + err.message);
                }

                if (this.mode == Mode.UploadProgress) {
                    this.mode = Mode.Show;
                }
            }.bind(this));

            this.mode = Mode.UploadProgress;
            this.uploadFileInfo = null;
            this.uploadModalRef.close();
        }
    }

    public changeUploadFile(evt: any): void {
        let fileElem = evt.target;

        if (fileElem.files.length) {
            let file = fileElem.files[0];
            this.uploadFileInfo = file;
            this.s3Path = file.name;
        }
        else {
            this.uploadFileInfo = null;
        }
    }

    /**
     * Initialize the SWF configuration
    **/
    private initializeSWFConfiguration(): void {
        for (let key of Object.keys(this.workflowConfig)) {
            this.configurationProperties.push({ name: key, type: typeof this.workflowConfig[key] });
        }
    }
}

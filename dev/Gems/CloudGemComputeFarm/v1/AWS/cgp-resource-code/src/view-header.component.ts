import { Input, Component, ViewChild, ElementRef } from '@angular/core';
import { CloudGemComputeFarmApi } from './index'
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { ModalComponent } from 'app/shared/component/index';
import { CloudGemComputeFarmIndexComponent } from "./index.component"
import { ToastsManager } from 'ng2-toastr';
import { WorkflowConfig } from './workflow-config'
import { LiveRunner, BuildState } from "./live-runner"
import { BuildIdManager } from "./build-id-manager"

enum DisplayMode {
    Normal,
    RunDialog,
    HistoryDialog
}

@Component({
    selector: 'view-header',
    templateUrl: 'node_modules/@cloud-gems/cloudgemcomputefarm/view-header.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemcomputefarm/index.component.css']
})

export class ViewHeaderComponent {
    @Input() context: any;
    @Input() index: CloudGemComputeFarmIndexComponent;
    @Input() workflowConfig: WorkflowConfig;

    @ViewChild('runModal') runModalRef: ModalComponent;
    @ViewChild('historyModal') historyModalRef: ModalComponent;

    private _apiHandler: CloudGemComputeFarmApi = null;

    private runner: LiveRunner = null;
    private initialized = false;
    private runId: string;
    activeWorkflowStatus: string = "";
    statusDecoration: string = "";
    canStartWorkflow = false;
    canCancelWorkflow = false;
    invalidWorkflowInput = false;
    executionName: string = "";
    executionContext: any = null;
    progressRows: any = [];
    startTime: string = "--";
    endTime: string = "--";
    canDownload = false;

    private buildIdManager: BuildIdManager = null;
    private selectedBuildIndex = 0;

    private DisplayMode = DisplayMode;
    private displayMode: DisplayMode = DisplayMode.Normal;
    
    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager) {
    }

    ngOnInit() {
        this.runner = this.index.liveRunner;
        this._apiHandler = this.index.getApiHandler();
        this.buildIdManager = this.index.getBuildIdManager();
    }

    updateStatus(): void {
        this.initialized = true;

        if (this.runner.execStatus !== undefined) {
            if (this.runner.execStatus == "OPEN") {
                if (this.runner.buildState == BuildState.Cancelling) {
                    this.activeWorkflowStatus = "CANCELLING";
                    this.statusDecoration = "fa fa-times-circle status-cancelling";
                    this.canCancelWorkflow = false;
                }
                else {
                    this.activeWorkflowStatus = "RUNNING";
                    this.statusDecoration = "fa fa-clock-o status-running";
                    this.canCancelWorkflow = true;
                }

                this.invalidWorkflowInput = false;
                this.canStartWorkflow = false;
                this.canDownload = false;
            }
            else {
                let closeStatus = this.runner.execInfo['closeStatus'];
                this.activeWorkflowStatus = closeStatus;
                this.statusDecoration = (closeStatus == "COMPLETED") ? "fa fa-check-circle status-completed" : 
                    "fa fa-times-circle status-failed";
                this.invalidWorkflowInput = !this.workflowConfig.isValid();
                this.canStartWorkflow = !this.invalidWorkflowInput;
                this.canCancelWorkflow = false;
                this.canDownload = (closeStatus === "COMPLETED");
            }

            this.startTime = this.getDateString(this.runner.execInfo['startTimestamp']);
            this.endTime = this.getDateString(this.runner.execInfo['closeTimestamp']);
        }
        else {
            this.invalidWorkflowInput = !this.workflowConfig.isValid();
            this.canStartWorkflow = !this.invalidWorkflowInput;
            this.canCancelWorkflow = false;
            this.activeWorkflowStatus = "--";
            this.statusDecoration = "";
            this.startTime = "--";
            this.endTime = "--";
            this.canDownload = false;
        }

        if (this.runner.executionContext && this.runner.executionContext['progress'] !== undefined) {
            let progress = this.runner.executionContext['progress'];
            let keys = Object.keys(progress).sort();

            this.progressRows = keys.map(key => [key, progress[key][0], progress[key][1]]);
        }
        else {
            this.progressRows = [];
        }
    }

    /**
     * Download the build results
	**/
    private downloadResult(): void {
        let merge_key = this.workflowConfig.s3_dir.length ? (this.workflowConfig.s3_dir + "/") : "";

        merge_key += this.workflowConfig.s3_zip.substring(0, this.workflowConfig.s3_zip.indexOf(".zip"));
        merge_key += "/merge/merge_final.zip";

        var params = {
            Bucket: this.context.ComputeFarmBucketName,
            Key: merge_key
        };
        var url = this.aws.context.s3.getSignedUrl('getObject', params);
        window.open(url);
    }

    private getDateString(dateField: any): string {
        if (dateField === undefined) {
            return "--";
        }

        let dt = new Date(dateField.substr(0, 19) + "Z");
        return dt.toString();
    }

    private openRunDialog() {
        this.displayMode = DisplayMode.RunDialog;
    }

    private dismissModal = ()=> {
        this.displayMode = DisplayMode.Normal;
    }

    private runWorkflow() {
        if (this.runner) {
            this.canStartWorkflow = false;
            this.canCancelWorkflow = false;
            this.runner.setRunId("");
            this.runner.ignoreNextStatusUpdate();
            this.canDownload = false;

            this._apiHandler.run(this.executionName, JSON.stringify(this.workflowConfig)).subscribe(() => {
                this.toastr.success("The workflow started successfully");

            }, (err) => {
                this.toastr.error("The workflow failed to start. " + err.message)
            });
        }

        this.displayMode = DisplayMode.Normal;
        this.runModalRef.close();
    }

    private cancelWorkflow() {
        if (this.runner) {
            this.canCancelWorkflow = false;
            this.runner.ignoreNextStatusUpdate();

            if (this.runner.buildState != BuildState.Idle) {
                this._apiHandler.cancel(this.runner.activeWorkflowId).subscribe(response => {
                    this.toastr.success("Terminated the workflow successfully.");
                }, err => {
                    this.toastr.error("Failed to terminate the workflow: " + err.message);
                });
            }
        }
    }

    private openHistoryDialog() {
        this.buildIdManager.refresh();
        this.displayMode = DisplayMode.HistoryDialog;
        this.selectedBuildIndex = 0;
    }

    private promptDeleteAllBuilds() {
        if (confirm("Delete all logs from previous builds?")) {
            this.buildIdManager.clearAndReferesh();
            this.runner.setRunId("");
            this.runner.ignoreNextStatusUpdate();
            this.historyModalRef.close();
            this.displayMode = DisplayMode.Normal;
        }
    }

    private changeBuildIndex(index) {
        this.selectedBuildIndex = index;
    }

    private submitViewBuild() {
        if (this.buildIdManager.buildIds.length) {
            this.runner.setRunId(this.buildIdManager.buildIds[this.selectedBuildIndex].id);
            this.displayMode = DisplayMode.Normal;
            this.historyModalRef.close();
        }
    }
}

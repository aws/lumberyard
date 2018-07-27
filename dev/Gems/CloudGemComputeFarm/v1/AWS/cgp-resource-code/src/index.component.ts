import { AwsContext } from "app/aws/context.class";
import { Input, Component, ViewChild, ElementRef } from '@angular/core';
import { AbstractCloudGemIndexComponent } from 'app/view/game/module/cloudgems/class/index';
import { CloudGemComputeFarmApi, SWFConfigurationComponent } from './index' 
import { Http } from '@angular/http';
import { Observable } from 'rxjs/Observable';
import { AwsService } from "app/aws/aws.service";
import { ProgressView } from "./progress-view"
import { TableView } from "./table-view"
import { LiveRunner } from "./live-runner"
import { TestRunner } from "./test-runner"
import { LyMetricService } from "app/shared/service/index";
import { BuildIdManager } from "./build-id-manager"
import { WorkflowConfig } from "./workflow-config"
import { EventLog } from "./event-log"
import { ViewHeaderComponent } from "./view-header.component"
import { ToastsManager } from 'ng2-toastr';

export interface CloudGemComputeFarmInterface {
    frameUpdate(currentTime: Date, events: Array<any>): void;
    statusUpdate(): void;
    isLoadingBuilds(): boolean;
}

@Component({
    selector: 'cloudgemcomputefarm-index',
    templateUrl: 'node_modules/@cloud-gems/cloudgemcomputefarm/index.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemcomputefarm/index.component.css']
})

export class CloudGemComputeFarmIndexComponent extends AbstractCloudGemIndexComponent implements CloudGemComputeFarmInterface {
    @Input() context: any;  //REQUIRED
    private _apiHandler: CloudGemComputeFarmApi;
    private _currentTime: Date = null;
    private _currentWorkflowId: any = null;

    private buildIdManager: BuildIdManager = null;
    private workflowConfig: WorkflowConfig = new WorkflowConfig();
    private indexView: CloudGemComputeFarmIndexComponent = this;
    private eventLog: EventLog = new EventLog();
    private progressView = null;
    private tableView = null;

    private viewOption: number = 0;
    private dataLoaded = false;
    
    private _testMode = false;
    testRunner: TestRunner = null;
    liveRunner: LiveRunner = null;

    
    @ViewChild('swfConfig') swfConfigurationComponent: SWFConfigurationComponent;
    @ViewChild('header') headerComponent: ViewHeaderComponent;
  
    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager) {
        super()
    }

    ngOnInit() {
        this._apiHandler = new CloudGemComputeFarmApi(this.context.ServiceUrl, this.http, this.aws);

        this.tableView = new TableView(this.eventLog);
        this.progressView = new ProgressView(this.eventLog);
        this.buildIdManager = new BuildIdManager(this, this._apiHandler, this.toastr);

        if (this._testMode) {
            this.testRunner = new TestRunner(this, this.http);
        }
        else {
            this.liveRunner = new LiveRunner(this, this._apiHandler);
        }
    }

    frameUpdate(currentTime: Date, events: Array<any>): void {
        this._currentTime = currentTime;

        this.eventLog.updateEvents(events);
        this.tableView.updateTable(currentTime);
        this.progressView.updateProgress(currentTime);
        this.dataLoaded = true;
    }

    statusUpdate(): void {
        if (this._currentWorkflowId != this.liveRunner.activeWorkflowId) {
            this._currentWorkflowId = this.liveRunner.activeWorkflowId;
            
            if (this.liveRunner.activeExecution) {
                this.liveRunner.setRunId(JSON.stringify(this.liveRunner.activeExecution));
            }

            this.eventLog.clear();
            this.tableView.clearTable();
            this.progressView.clearProgress();
            this.dataLoaded = false;
        }

        if (this.headerComponent) {
            this.headerComponent.updateStatus();
        }
    }

    isLoadingBuilds(): boolean {
        return this.buildIdManager.isLoading();
    }
      
    runTests(): void {
        if (this.testRunner) {
            this.testRunner.run();
        }
    }

    getApiHandler(): CloudGemComputeFarmApi {
        return this._apiHandler
    }

    getBuildIdManager(): BuildIdManager {
        return this.buildIdManager;
    }
}

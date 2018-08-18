import { CloudGemComputeFarmApi } from './index' 
import { CloudGemComputeFarmInterface } from "./index.component"

export enum BuildState {
    Idle,
    Running,
    Terminated,
    Cancelling,
    Completed
}

export class LiveRunner {
    private _mostRecentEventTime: Date = null;
    private _currentTime: Date = null;
    private _eventsTimer;
    private _loadingEvents = false;
    private _runId: string = "";
    public loaded = false;

    private _statusTimer;
    private _ignoreNextStatusUpdate = false;
    private _loadingStatus = false;
    execInfo: any = null;
    execStatus: string = undefined;
    executionContext: any = null;
    activeExecution: any = null;
    activeWorkflowId: string = "--";
    buildState: BuildState = BuildState.Idle;
    fetchingEvents: boolean = false;
    BuildState = BuildState;

    constructor(private _computeFarmInterface: CloudGemComputeFarmInterface, private _apiHandler: CloudGemComputeFarmApi) {
        this.loaded = true;
        this.fetchEvents();

        var self = this;
        
        this._eventsTimer = window.setInterval(function() { self.fetchEvents(); }, 1000);
        this._statusTimer = window.setInterval(function() { self.fetchStatus(); }, 1000);

        this.fetchStatus();
    }

    setRunId(runId: string): void {
        this._runId = runId;
        this._mostRecentEventTime = null;
    }

    fetchEvents(): void {
        if (this._loadingEvents || !this._runId.length || (!this.fetchingEvents && this._mostRecentEventTime)) {
            this._loadingEvents = false;
            return;
        }

        var fetchTime = this._mostRecentEventTime ? (this._mostRecentEventTime.getTime() / 1000) : 0
        this._loadingEvents = true;

        this._apiHandler.getEvents(encodeURIComponent(this._runId), fetchTime).subscribe(response => {
            this._loadingEvents = false;
            let obj = JSON.parse(response.body.text());
            let result: any = obj['result'];
            let runId: string = result['run-id'];

            if (runId != this._runId) {
                return;
            }

            let events: Array<any> = result['events'];
            let closeTime = this.execInfo['closeTimestamp'];

            if (closeTime !== undefined && closeTime.length) {
                this._currentTime = new Date(closeTime.substr(0, 19) + "Z");
            }
            else {
                this._currentTime = new Date();
            }

            if (events.length) {
                this._mostRecentEventTime = new Date(events[events.length - 1]['timestamp'] + "Z");
            }

            this._computeFarmInterface.frameUpdate(this._currentTime, events);
        });
    }

    fetchStatus(): void {
        if (this._loadingStatus || this._computeFarmInterface.isLoadingBuilds()) {
            return;
        }

        this._loadingStatus = true;
        this._ignoreNextStatusUpdate = false;

        this._apiHandler.status(this._runId).subscribe(response => {
            if (this._ignoreNextStatusUpdate || this._computeFarmInterface.isLoadingBuilds()) {
                this._loadingStatus = false;
                return;
            }

            this._loadingStatus = false;
            let obj = JSON.parse(response.body.text());
            let result = JSON.parse(obj['result']);
            let context = result['latestExecutionContext'];
            this.execInfo = result['executionInfo'];
            this.execStatus = (this.execInfo !== undefined) ? this.execInfo['executionStatus'] : undefined;

            if (context !== undefined && context.length) {
                this.executionContext = JSON.parse(context);
            }

            if (this.execStatus !== undefined) {
                this.activeExecution = this.execInfo['execution'];
                this.activeWorkflowId = this.activeExecution['workflowId'];

                if (this.execStatus == "OPEN") {
                    if (this.execInfo['cancelRequested']) {
                        this.buildState = BuildState.Cancelling;
                    }
                    else {
                        this.buildState = BuildState.Running;
                    }
                }
                else {
                    let closeStatus = this.execInfo['closeStatus'];
                    this.buildState = (closeStatus == "COMPLETED") ? BuildState.Completed : BuildState.Terminated;
                }
            }
            else {
                this.activeWorkflowId = "--";
                this.activeExecution = null;
                this.buildState = BuildState.Idle;
            }

            this.fetchingEvents = (this.buildState == BuildState.Running || this.buildState == BuildState.Completed);
            this._computeFarmInterface.statusUpdate();
        });
    }

    ignoreNextStatusUpdate(): void {
        this._ignoreNextStatusUpdate = this._loadingStatus;
    }
}

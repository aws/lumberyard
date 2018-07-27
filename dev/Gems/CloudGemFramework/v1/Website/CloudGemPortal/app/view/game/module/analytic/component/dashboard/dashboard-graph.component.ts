import { Component, OnInit, Input, Output, AfterViewInit, ViewChild, OnDestroy} from '@angular/core';
import { FormBuilder, FormGroup, Validators } from '@angular/forms';
import { ToastsManager } from 'ng2-toastr/ng2-toastr';
import { MetricGraph, MetricQuery, MetricBarData, MapTuple, MapMatrix, MapVector, AllTablesUnion, ChoroplethData } from 'app/view/game/module/shared/class/index';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { DefinitionService } from 'app/shared/service/index';
import { ProjectApi } from 'app/shared/class/index';
import { Subject } from 'rxjs/Subject';
import { ModalComponent } from 'app/shared/component/index';

declare var AMA

enum Modes {
    READ,
    EDIT,
    DELETE 
}

/**
* metricsLineData
* A helper method for parsing out the usful inform from a metrics gem REST endpoint and graphing it.  It takes in an array of data
* and formats it for the ngx-charts graphing library.
* @param series_x_label
* @param series_y_name
* @param dataSeriesLabel
* @param response
*/
export function MetricLineData(series_x_label: string, series_y_name: string, dataSeriesLabel: string, response: any): any {
    let data = response.Result;    
    data = data.slice();

    let header = data[0]
    data.splice(0, 1)
    //At least 2 points required to create a line graph.  If this data doesn't exist generate a 0 value for
    //a single data point.
    let result = <any>[]
    if (header.length == 2) {
        result = MapTuple(header, data)
    } else if (header.length == 3) {
        result = MapVector(header, data)    
    } else if (header.length >= 4) {
        result = MapMatrix(header, data)
    }
    return result
}

@Component({
    selector: 'dashboard-graph',
    template: `
        <div>Charts will become available as data becomes present.  Charts will refresh automatically every 5 minutes. </div>
        <p>Deep dive into your data with <a [href]="quicksighturl" target="_quicksight">AWS QuickSight</a></p>
        <div *ngIf="!isLoadingGraphs; else loading"></div>        
        <ng-container *ngFor="let chart of metricGraphs">
            <graph [ref]="chart" [edit]=edit [delete]=confirmDeletion></graph>                    
        </ng-container>   
        <ng-container *ngIf="!isLoadingGraphs">
            <graph [add]=add></graph>                    
        </ng-container>
        <modal *ngIf="mode == modes.EDIT && currentGraph "
            [title]="modalTitle"
            [autoOpen]="true"
            [hasSubmit]="true"
            [onDismiss]="dismissModal"
            [onClose]="closeModal"
            [submitButtonText]="modalSubmitTitle"
            (modalSubmitted)="submit(currentGraph)">
            <div class="modal-body">
                <form [formGroup]="graphForm">
                    <div class="form-group row" [ngClass]="{'has-danger': control('title').invalid && submitted}">
                        <label class="col-lg-3 col-form-label affix" for="title"> Title </label>
                        <div class="col-lg-8"><input class="form-control" type="string" formControlName="title" id="title" [(ngModel)]="currentGraph.model.title"></div>                        
                        <div *ngIf="control('title').invalid && submitted" class="form-control-danger">
                          <div *ngIf="control('title').errors.required">
                            Title is required.
                          </div>
                        </div>
                    </div>
                    <div class="form-group row" [ngClass]="{'has-danger': control('type').invalid && submitted}">
                        <label class="col-lg-3 col-form-label affix" for="type"> Visualization Type </label>
                        <div class="col-lg-8">
                            <dropdown class="search-dropdown" (dropdownChanged)="updateCurrentGraph($event)" [currentOption]="findChartType(currentGraph.model.type)" [options]="chartTypeOptions" placeholderText="Select type"> </dropdown>
                        </div>                   
                        <div *ngIf="control('type').invalid && submitted" class="form-control-danger">
                          <div *ngIf="control('type').errors.required">
                            Type is required.
                          </div>
                        </div>
                    </div>
                    <div class="form-group row" [ngClass]="{'has-danger': control('xaxislabel').invalid && submitted}">
                        <label class="col-lg-3 col-form-label affix" for="xaxislabel"> X-Axis Label </label>
                        <div class="col-lg-8"><input class="form-control" type="string" formControlName="xaxislabel" [(ngModel)]="currentGraph.model.xaxislabel"></div>                        
                        <div *ngIf="control('xaxislabel').invalid && submitted" class="form-control-danger">
                          <div *ngIf="control('xaxislabel').errors.required">
                            X-Axis is required.
                          </div>
                        </div>
                    </div>
                    <div class="form-group row" [ngClass]="{'has-danger': control('yaxislabel').invalid && submitted}">
                        <label class="col-lg-3 col-form-label affix" for="yaxislabel"> Y-Axis Label </label>
                        <div class="col-lg-8"><input class="form-control" type="string" formControlName="yaxislabel" [(ngModel)]="currentGraph.model.yaxislabel"></div>                        
                        <div *ngIf="control('yaxislabel').invalid && submitted" class="form-control-danger">
                          <div *ngIf="control('yaxislabel').errors.required">
                            Y-Axis is required.
                          </div>
                        </div>
                    </div>                   
                    <div class="form-group row" [ngClass]="{'has-danger': control('sql').invalid && submitted}">
                        <label class="col-lg-3" for="sql"> SQL </label>                        
                        <div *ngIf="control('sql').invalid && submitted" class="form-control-danger">
                          <div *ngIf="control('sql').errors.required">
                            SQL is required.
                          </div>
                        </div>
                        <textarea cols=104 rows=10 formControlName="sql" [(ngModel)]="currentGraph.model.sql[0]"></textarea>                              
                        <p>Edit with <a [href]="getAthenaQueryUrl()" target="_athena">AWS Athena</a>. </p>                                                
                    </div>
                </form>
            </div>
        </modal>
        <modal *ngIf="mode == modes.DELETE"
               title="Delete Graph"
               [autoOpen]="true"
               [hasSubmit]="true"
               [onDismiss]="onDismissModal"
               [onClose]="closeModal"
               submitButtonText="Delete"
               (modalSubmitted)="delete(currentGraph)">               
            <div class="modal-body">
                <p>
                    Are you sure you want to delete this graph?
                </p>
            </div>
        </modal>
        <ng-template #loading>
            <loading-spinner></loading-spinner>
        </ng-template>
      
    `,
    styles: [`textarea{  
        width: 26em; 
        width: 645px; 
    }`]
})
export class DashboardGraphComponent implements OnInit, OnDestroy {
    @Input('facetid') public facetid: string
    private metricGraphs: Array<MetricGraph>;
    private _metricApiHandler: any
    private mode = Modes.READ
    private modes = Modes
    private currentGraph: MetricGraph
    private graphModels = []
    private quicksighturl: string
    private isLoadingGraphs: boolean
    private graphForm: FormGroup    
    private chartTypeOptions = [
        { text: "Line", parser: "lineData", ngx_chart_type: "ngx-charts-line-chart"},
        { text: "Stacked Line", parser: "lineData", ngx_chart_type: "ngx-charts-area-chart-stacked" },
        { text: "Bar", parser: "barData", ngx_chart_type: "ngx-charts-bar-vertical" },
        { text: "Gauge", parser: "barData", ngx_chart_type: "ngx-charts-gauge" }
    ]
    private modalTitle: string
    private submitted: boolean
    private modalSubmitTitle: string
    private _query: MetricQuery

    @ViewChild(ModalComponent) modalRef: ModalComponent;

    constructor(private fb: FormBuilder,
        private http: Http,
        private aws: AwsService,
        private definition: DefinitionService,
        private toastr: ToastsManager) {
        this.metricGraphs = new Array<MetricGraph>()
    }

    ngOnInit() {
        let metric_service = this.definition.getService("CloudGemMetric")
        this._metricApiHandler = new metric_service.constructor(metric_service.serviceUrl, this.http, this.aws)
        this.facetid = this.facetid.toLocaleLowerCase()
        this.quicksighturl = `https://${this.aws.context.region}.quicksight.aws.amazon.com/sn/start?#`
        this.load()
    }

    load = () => {
        this.isLoadingGraphs = true       
        this.graphModels = []
        this.metricGraphs = new Array<MetricGraph>()
        this._metricApiHandler.getDashboard(this.facetid).subscribe(r => {
            this._metricApiHandler.getFilterEvents().subscribe((res) => {
                this.isLoadingGraphs = false
                let distinctEvents = JSON.parse(res.body.text()).result
                this._query = new MetricQuery(this.aws, "", distinctEvents, [new AllTablesUnion(this.aws, distinctEvents)])
                for (let idx in r) {
                    let metric = r[idx]                                           
                    metric.index = idx                   
                    
                    this.metricGraphs.push(
                        new MetricGraph(metric.title,
                            metric.xaxislabel,
                            metric.yaxislabel,
                            metric.dataserieslabels,
                            this.sources(metric),
                            this.parsers(metric),
                            metric.type,
                            [], [],
                            metric.colorscheme,
                            metric
                        ));
                    this.graphModels.push(metric)

                }

            }, err => {
                console.log(err)
            });

        }, err => {
            console.log(err)
        })
    }

    refresh = (graph: MetricGraph) => {
        let metric = graph.model          
        let sources = []
        
        graph.isLoading = true

        for (let idy in metric.sql) {
            let sql = metric.sql[idy]
            sources.push(this._metricApiHandler.postQuery(this._query.toString(sql)))
        }        

        this.graphModels[graph.model.index] = graph.model
        this.metricGraphs[graph.model.index] = new MetricGraph(metric.title,
            metric.xaxislabel,
            metric.yaxislabel,
            metric.dataserieslabels,
            this.sources(metric),
            this.parsers(metric),
            metric.type,
            [], [],
            metric.colorscheme,
            metric
        )
    }

    parsers = (metric) => {
        let parsers = []
        let barData = MetricBarData
        let lineData = MetricLineData
        let choroplethData = ChoroplethData
        
        for (let idy in metric.parsers) {
            let parser = eval(metric.parsers[idy])
            parsers.push(parser)
        }
        return parsers
    }

    sources = (metric) => {
        let sources = []
        for (let idy in metric.sql) {
            let sql = metric.sql[idy]
            sources.push(this._metricApiHandler.postQuery(this._query.toString(sql)))
        }
        return sources
    }

    findChartType = (type) => {        
        for (let idx in this.chartTypeOptions) {
            let chart = this.chartTypeOptions[idx]
            if (type == chart.ngx_chart_type || type == chart.text) {
                return { text: chart.text }
            }
        }
    }

    edit = (graph) => {        
        this.currentGraph = graph        
        this.modalTitle = "Edit SQL"
        this.modalSubmitTitle = "Submit"
        this.setFormGroup()
        this.mode = Modes.EDIT        
    }

    add = () => {            
        this.currentGraph = new MetricGraph("", "", "", [], [], [])
        this.modalTitle = "Add new graph"
        this.modalSubmitTitle = "Save"
        this.currentGraph.model = {
            "dataserieslabels": ["label"],
            "parsers": [],
            "sql": ["SELECT platform_identifier, count(distinct universal_unique_identifier) FROM ${database}.${table_sessionstart} GROUP BY 1 ORDER BY 1"],
            "title": "",
            "type": "",
            "xaxislabel": "",
            "yaxislabel": "",
            "id": AMA.Util.GUID()
        }      
        this.setFormGroup()
        this.mode = Modes.EDIT
    }

    confirmDeletion = (graph: MetricGraph) => {
        this.currentGraph = graph
        this.mode = Modes.DELETE
    }

    delete = (graph: MetricGraph) => {
        let index = graph.model.index    
        let name = graph.title   
        this.graphModels.splice(index, 1);
        this.metricGraphs.splice(index, 1);
        this._metricApiHandler.postDashboard(this.facetid, this.graphModels).subscribe(r => {
            this.toastr.success(`The graph '${name}' has been deleted.`);            
        }, err => {
            this.toastr.success(`The graph '${name}' failed to delete. ${err}`);
            console.log(err)
        })
        this.dismissModal()       
    }

    control = (id) => {
        return this.graphForm.get(id);    
    }

    setFormGroup = () => {
        this.graphForm = this.fb.group({
            title: [this.currentGraph.model.title, Validators.required],
            type: [this.currentGraph.model.type, Validators.required],
            xaxislabel: [this.currentGraph.model.xaxislabel, Validators.required],
            yaxislabel: [this.currentGraph.model.yaxislabel, Validators.required],
            sql: [this.currentGraph.model.sql[0], Validators.required]            
        })       
    }

    updateCurrentGraph = (val) => {
        this.currentGraph.model.type = val.text;
    }

    valid = (graph) => {        
        for (let idx in this.graphForm.controls) {            
            if (idx == 'type')
                continue
            let control = this.graphForm.get(idx)
            if (!control.valid)
                return false
        }

        let valid = false
        for (let idx in this.chartTypeOptions) {
            let chart = this.chartTypeOptions[idx]
            if (graph.model.type == chart.text || graph.model.type == chart.ngx_chart_type) {
                valid = true
            }
        }
        return valid
    }

    submit = (graph: MetricGraph) => {  
        this.submitted = true      
        if (!this.valid(graph))
            return

        let name = graph.title ? graph.title : graph.model.title        
        let found = false
        for (let idx in this.graphModels) {
            let graph_entry = this.graphModels[idx]
            if (graph.model.id == graph_entry.id) {
                found = true
                break;
            }
        }

        for (let idx in this.chartTypeOptions) {
            let chart = this.chartTypeOptions[idx]
            if (graph.model.type == chart.text || graph.model.type == chart.ngx_chart_type) {
                graph.model.parsers = []
                graph.model.parsers.push(chart.parser)
                graph.model.type = chart.ngx_chart_type
            }
        }

        if (!found) {
            this.graphModels.push(graph.model)
        } else {
            graph.isLoading = true
        }
        
        this._metricApiHandler.postDashboard(this.facetid, this.graphModels).subscribe(r => {
            this.toastr.success(`The graph '${name}' has been update successfully.`);
            if(found)            
                this.refresh(graph)
            else
                this.load()
        }, err => {
            graph.isLoading = false
            this.toastr.success(`The graph '${name}' failed to update. ${err}`);
            console.log(err)
            })        
        this.submitted = false 
        this.dismissModal()        
    }

    closeModal = () => {
        this.mode = Modes.READ;        
        this.currentGraph = undefined
    }

    dismissModal = () => {        
        this.modalRef.close();        
    }

    getAthenaQueryUrl = () => {
        let query_id = this.currentGraph.response ? this.currentGraph.response.QueryExecutionId : ""
        return `https://console.aws.amazon.com/athena/home?force&region=${this.aws.context.region}#query/history/${query_id}`
    }

    ngOnDestroy() {
        // When navigating to another gem remove all the interval timers to stop making requests.
        this.metricGraphs.forEach(graph => {
            graph.clearInterval();
        });
    }
    
}
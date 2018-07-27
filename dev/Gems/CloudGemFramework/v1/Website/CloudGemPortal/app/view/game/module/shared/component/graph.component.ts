import { Component, Input, ViewEncapsulation, OnInit, trigger, OnDestroy, ViewChild, AfterViewInit, ComponentFactoryResolver } from '@angular/core';
import { AnchorDirective} from "app/shared/directive/anchor.directive";
import { MetricGraph } from 'app/view/game/module/shared/class/index';
import { ActionStubItemsComponent } from 'app/view/game/module/shared/component/action-stub-items.component'
import Ajv from "ajv"

@Component({
    selector: 'graph',
    template: `        
                    <div class="graph">                                                                
                        <div class="title row no-gutter no-gutters justify-content-end"> 
                            <div class="col-9"> {{ ref ? ref.title : "Add New Graph" }} </div>                      
                            <div class="col-3">                                    
                                <ng-container *ngIf="ref && !ref.isLoading">
                                    <span class="float-right" >
                                        <action-stub-items  [model]="ref" [edit]="onEdit" [delete]="onDelete"></action-stub-items>
                                    </span>
                                </ng-container>
                            </div>
                        </div>                     
                        <div [ngClass]="{'chartarea': ref == undefined || ref.type != 'ngx-charts-gauge'}">                        
                            <div style="padding-top: 80px" *ngIf="ref && ref.isLoading == true">
                                <loading-spinner></loading-spinner>
                            </div>  
                            <div style="padding-top: 50px" *ngIf="ref && !ref.isLoading && ref.response && ref.response.Status && (ref.response.Status.State == 'FAILED' || ref.response.Status.State == 'CANCELLED') ">
                                <p>{{ref.response.Status.StateChangeReason}}</p>
                            </div>  
                            <div style="padding-top: 50px" *ngIf="errorMessage || (ref && ref.errorMessage)">
                                <p>{{errorMessage}}</p>
                                <p *ngIf="ref">{{ref.errorMessage}}</p>
                            </div>                              
                            <ng-container *ngIf="ref && ref.isLoading == false && validateDataSeries(ref.type, ref.dataSeries)">                             
                                <ngx-charts-line-chart *ngIf="ref.type == 'ngx-charts-line-chart'"
                                    [view]="[400, 300]"
                                    [scheme]="ref.colorScheme"
                                    [results]="ref.dataSeries"                                
                                    xAxis="true"
                                    yAxis="true"
                                    showXAxisLabel="true"
                                    showYAxisLabel="true"
                                    [xAxisLabel]="ref.xAxisLabel"
                                    [yAxisLabel]="ref.yAxisLabel"
                                    [xAxisTickFormatting]="xAxisFormatting"     
                                    autoScale="true"
                                    yScaleMin=0
                                    >                        
                                </ngx-charts-line-chart>
                                 <ngx-charts-area-chart-stacked *ngIf="ref.type == 'ngx-charts-area-chart-stacked'"
                                    [view]="[400, 300]"
                                    [scheme]="ref.colorScheme"
                                    [results]="ref.dataSeries"                                
                                    xAxis="true"
                                    yAxis="true"
                                    showXAxisLabel="true"
                                    showYAxisLabel="true"
                                    [xAxisLabel]="ref.xAxisLabel"
                                    [yAxisLabel]="ref.yAxisLabel"
                                    [xAxisTickFormatting]="xAxisFormatting"     
                                    autoScale="true"
                                    yScaleMin=0
                                    >                        
                                </ngx-charts-area-chart-stacked>
                                <ngx-charts-bar-vertical *ngIf="ref.type == 'ngx-charts-bar-vertical'"
                                    [view]="[400, 300]"
                                    [scheme]="ref.colorScheme"
                                    [results]="ref.dataSeries"
                                    [gradient]="gradient"                            
                                    xAxis="true"
                                    yAxis="true"
                                    showXAxisLabel="true"
                                    showYAxisLabel="true"
                                    [xAxisLabel]="ref.xAxisLabel"
                                    [yAxisLabel]="ref.yAxisLabel"               
                                    [xAxisTickFormatting]="xAxisFormatting"                          
                                    >
                                </ngx-charts-bar-vertical>
                                <ngx-charts-gauge *ngIf="ref.type == 'ngx-charts-gauge' "
                                  [view]="[450, 300]"
                                  [scheme]="ref.colorScheme"
                                  [results]="ref.dataSeries"
                                  [min]="0"
                                  [max]="100"
                                  [angleSpan]="240"
                                  [startAngle]="-120"
                                  [units]="ref.yAxisLabel"
                                  [bigSegments]="10"
                                  [smallSegments]="5"
                                  >
                                </ngx-charts-gauge>
                            </ng-container>
                            <ng-container *ngIf="!ref">                             
                                <div (click)="onAdd()" class="clickable" ><i class="fa fa-plus fa-5x new-graph"></i></div>
                            </ng-container>
                        </div>                        
                    </div>
    `,    
    styles: [`.new-graph { margin-top: 94px; color: gainsboro; } .clickable { cursor: pointer; } `]
})
export class GraphComponent implements AfterViewInit, OnDestroy {
    @Input('ref') public ref: MetricGraph;  
    @Input('edit') public onEdit: Function; 
    @Input('add') public onAdd: Function; 
    @Input('delete') public onDelete: Function; 

    private _ajv: Ajv.Ajv = new Ajv({ allErrors: true });

    private _series = {
        "type": "array",
        "items": {
            "type": "object",
            "properties": {
                "name": { "type": 'string' },
                "series": {
                    "type": "array",
                    "items": {
                        "type": "object",
                        "properties": {
                            "name": { "type": "string", "default": "NA" },
                            "value": { "type": "number", "default": 0 }
                        },
                        "additionalProperties": false,
                        "required": ["name", "value"]
                    },
                }                
            },
            "required": ["name", "series"]
        }
    }

    private _pair = {        
        "type": "array",
        "items": {
            "type": "object",
            "properties": {
                "name": { "type": 'string', "default": "NA" },
                "value": { "type": 'number', "default": 0 }
            },
            "additionalProperties": false,
            "required": ["name", "value"]                     
        }
    }

    private chartSchemas = new Map<string, any>([
        ["ngx-charts-line-chart", new Ajv().compile(this._series)],
        ["ngx-charts-area-chart-stacked", new Ajv().compile(this._series)],
        ["ngx-charts-bar-vertical", new Ajv().compile(this._pair)],
        ["ngx-charts-gauge", new Ajv().compile(this._pair)]
    ])   

    private errorMessage = undefined
    private isvalid = false
    private validated = false

    constructor(private componentFactoryResolver: ComponentFactoryResolver) {   
        
    }

    ngOnInit() {        
    }

    ngAfterViewInit() {                
    }

    ngOnDestroy() {                
    }    

    validateDataSeries = (type, dataSeries) => {
        if (this.validated)
            return this.isvalid
                 
        if (!this.chartSchemas.has(type))
            this.errorMessage = `The graph type '${type}' does not have an associated schema to validate against.`

        if (dataSeries == null || dataSeries== undefined)
            return false
        this.validated = true
        let test = this.chartSchemas.get(type)
        let isvalid = test(dataSeries)
        if(isvalid){            
            this.errorMessage = undefined
            this.isvalid = true
            return true
        } else {            
            console.log("Validation errors:")
            console.log(test.errors)   
            console.log("Data source:")
            console.log(dataSeries)         
            this.errorMessage = `The datasource schema was malformed for the graph plotter.  For more details view your browser JavaScript console. Message: ${test.errors[0].message}.`
        }
        return false
    }

    xAxisFormatting = (label) => {
        let dataseries = <any>(this.ref.dataSeries)        
        if (dataseries != null
            && dataseries != undefined            
            && (dataseries.length == 0 || dataseries.length == 2))
            return label 

        if (dataseries[0].series == undefined || dataseries[0].series == null)
            return label

        let idx = -1        
        let length = dataseries[0].series.length
        for (var i = 0; i < length; i++) {
            let entry = dataseries[0].series[i]
            if (label == entry.name) {
                idx = i
                break;
            }
        }

        if (length <= 6) {            
            return label
        } else {                        
            if (idx % 4 == 0) {                         
                return label
            } else {
                return ""
            }
        }
    }


}
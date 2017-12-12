import { Component, Input, ViewEncapsulation, OnInit, trigger, ViewChild } from '@angular/core';
import { AwsService } from "app/aws/aws.service";
import { AwsContext } from "app/aws/context.class";
import { ApiHandler, StringUtil } from 'app/shared/class/index';
import { Http } from '@angular/http';
import { Schema } from 'app/aws/schema.class';
import { Facetable } from '../../class/index';

@Component({
    selector: 'facet-cloud-watch-logs',
    template: `        
            <div *ngIf="model.groups.length == 0 && model.loading">
                <loading-spinner></loading-spinner>
            </div>
            <div *ngIf="model.err && model.groups.length == 0">
                {{model.err}}
            </div>
            <div *ngIf="model.groups.length > 0">
                <div id="top" col="row">
                    <h4>**Logs can be delayed a few minutes.  Displaying the last 20 minutes of CloudWatch Logs.</h4>
                </div>               
                <div class="row table-of-contents" *ngIf="model.groupNames.length>1">
                    <div class="col-12">
                        <div class="d-inline-block dropdown-outer"  ngbDropdown>
                            <button type="button" class="btn l-dropdown" id="path-dropdown" ngbDropdownToggle>
                                <span class="dropdown-inner"> Cloud Watch Groups </span>
                                <i class="fa fa-caret-down" aria-hidden="true"></i>
                            </button>
                            <div class="dropdown-menu dropdown-menu-center" aria-labelledby="path-dropdown">
                                <button *ngFor="let group of model.groupNames; let i = index" (click)="goToLog(group)" type="button" class="dropdown-item"> {{group}} </button>
                            </div>            
                        </div>        
                    </div>
                </div>                
                <div col="row">
                    <div *ngFor="let group of model.groups">                   
                        <div id="{{group.name}}" class="row">  
                            <div class="col-1"><h2>Log Group</h2></div>                         
                            <div class="col-11"><p class="text-info">{{group.name}} (<a (click)="goToLog('top')"> Top </a>) <a><i class="fa fa-external-link" (click)="openAwsConsoleLog(group.name)"></i></a></p></div>
                        </div>
                        <table class="table table-hover">
                            <thead>
                                <tr>                            
                                    <th width="10%"> START </th>
                                    <th width="60%"> MESSAGE </th>
                                    <th width="20%"> LOG STREAM </th>
                                    <th width="10%"> TIMESTAMP </th>
                                </tr>
                            </thead>
                            <tbody>
                                <tr *ngFor="let entry of group.events" (click)="openAwsConsoleLog(group.name, entry.logStreamName)">                            
                                    <td >  {{entry.ingestionTime | toDateFromEpochInMilli | date:'medium'}} </td>
                                    <td >  {{entry.message}} </td>
                                    <td >  {{entry.logStreamName}} </td>                                            
                                    <td >  {{entry.timestamp | toDateFromEpochInMilli | date:'medium'}} </td>
                                </tr>
                            </tbody>
                        </table>
                    </div>
                </div>
            </div>        
    `,
    styles: [
        `
       .table-of-contents {
            margin-bottom: 20px;
        }
        .dropdown-inner {
            width: 360px;
            display: inline-flex;
        }
        .dropdown-outer {
            width: 400px;            
        }
        `
    ]
})
export class CloudWatchLogComponent implements Facetable {
    @Input() data: any;    
        
    private model = {
        groups: [],  
        groupNames: [],
        err: undefined,
        loading: false
    }

    constructor(private aws: AwsService, private http: Http) {        
        
    }

    ngOnInit() {
        this.getCloudWatchLog = this.getCloudWatchLog.bind(this)
        this.getCloudWatchLog(this.data["physicalResourceId"], this.aws.context, this.model)        
    }  

    openAwsConsoleLog(groupName: string, stream: string): void {
        let url = "https://console.aws.amazon.com/cloudwatch/home?region=" + this.aws.context.region + "#logEventViewer:group=" + groupName
        if (stream)
            url += ";stream=" + stream
        window.open(encodeURI(url), 'awsConsoleLogViewer')
    }

    getCloudWatchLog(stackId: string, context: AwsContext, model: any): void {        
        var params = {
            StackName: stackId
        };
        this.model.loading = true;
        context.cloudFormation.describeStackResources(params, function (err, data) {            
            if (err) {
                model.loading = false;
                model.err = err.message
            } else {                             
                let now = new Date()
                now.setMinutes(-20)                
                let startingTime = now.getTime();                          
                for (var i = 0; i < data.StackResources.length; i++) {
                    let spec = data.StackResources[i];                    
                    if (spec[Schema.CloudFormation.RESOURCE_TYPE.NAME] == Schema.CloudFormation.RESOURCE_TYPE.TYPES.LAMBDA) {                                                
                        let params = {
                            logGroupName: '/aws/lambda/' + spec.PhysicalResourceId,
                            interleaved: true,
                            limit: 500,                                                       
                            startTime: startingTime
                        };
                        model.groupNames.push(params.logGroupName)                                      
                        context.cloudWatchLogs.filterLogEvents(params, function (err, data) {
                            model.loading = false;                            
                            if (err) {                                
                                model.err = "No CloudWatch logs found."  
                            } else {      
                                model.groups.push({
                                    name: params.logGroupName,
                                    events: data.events.reverse()
                                })
                            }
                            
                        });
                    }

                }


            }
        });
    }

    goToLog(fragment: string): void {
        window.location.hash = 'blank'
        window.location.hash = fragment;
    }

}


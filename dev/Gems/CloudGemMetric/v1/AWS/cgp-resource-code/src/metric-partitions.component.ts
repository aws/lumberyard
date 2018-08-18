import { Component, OnInit, Input } from '@angular/core';
import { CloudGemMetricApi } from './api-handler.class';
import { AwsService } from 'app/aws/aws.service';
import { Http } from '@angular/http';
import 'rxjs/add/operator/do';
import { Observable } from 'rxjs/Observable';
import { debounceTime, distinctUntilChanged } from 'rxjs/operators';
import { ToastsManager } from 'ng2-toastr/ng2-toastr';

@Component({
    selector: 'metric-partitions',
    template: `
        <h2> Required </h2>
        <ng-container [ngSwitch]="isLoadingPredefinedPartitions">
            <p> Partitions are used by Athena as indexes for your data.  They are effectively <a target="s3" href="https://s3.console.aws.amazon.com/s3/home?region=us-east-1#">S3 key</a> paths.  Querying based on partitions will increase performance and decrease costs.  When Athena uses partitions it only has to scan a subset of the entire dataset. The Cloud Gem Metric gives you some default partitions out of the box.   </p>
            <div *ngSwitchCase="true">
                <loading-spinner> </loading-spinner>
            </div>
            <div *ngSwitchCase="false">                                  
                <div class="row">
                    <div class="col-lg-3">
                        <label> ATTRIBUTE NAME </label>
                    </div>
                    <div class="col-lg-3">
                        <label> TYPE </label>
                    </div>
                    <div class="col-lg-3">
                        <label> PARTS </label>
                    </div>
                </div>                
                <div class="row partition" *ngFor="let partition of predefinedPartitions">
                    <div class="col-lg-3">
                        <label >{{partition.key}}</label>        
                        <tooltip placement="right" [tooltip]="partition.description"> </tooltip>
                    </div>
                    <div class="col-lg-3">
                        {{partition.type}}
                    </div>
                    <div class="col-lg-3">
                        {{partition.parts}}
                    </div>                    
                </div>
            </div>
        </ng-container>
        <h2> Custom </h2>
        <ng-container [ngSwitch]="isLoadingCustomPartitions">
            <div *ngSwitchCase="true">
                <loading-spinner> </loading-spinner>
            </div>
            <div *ngSwitchCase="false">
                <p>ADVANCED USER AREA</p>
                <p>Create your own custom partitions.  Remember that the more partitions you have the smaller the partition files will be.  Too many partitions can decrease performance.<br />Changing the order after data has been written can cause random tables to appear in Athena.</p>
                <div class="row">
                    <div class="col-lg-3">
                        <label> ATTRIBUTE NAME <tooltip placement="right" tooltip="The short event attribute name being sent from your game."> </tooltip></label>
                    </div>
                    <div class="col-lg-3">
                        <label> TYPE <tooltip placement="right" tooltip="The attribute value type.  Currently there is only support int, str, float, datetime.datetime.utcfromtimestamp, and dictionary"> </tooltip> </label>
                    </div>
                    <div class="col-lg-3">
                        <label> PARTS <tooltip placement="right" tooltip="A type of function to call on the casted type.  Example. My type is datetime.datetime.utcfromtimestamp so I can call .year, .month, .day, or .hour."> </tooltip></label>
                    </div>
                </div>                
                <div class="row partition" *ngFor="let customPartition of customPartitions">
                    <div class="col-lg-3">
                        <input type="text" class="form-control" [(ngModel)]="customPartition.key" [ngbTypeahead]="searchKeys" />
                    </div>
                    <div class="col-lg-3">
                        <input type="text" class="form-control" [(ngModel)]="customPartition.type" [ngbTypeahead]="searchTypes" />
                    </div>
                    <div class="col-lg-3">
                        <input type="text" class="form-control" [(ngModel)]="customPartition.parts" [ngbTypeahead]="searchParts" />
                    </div>
                    <i (click)="removeCustomPartition(customPartition)" class="fa fa-trash-o"></i>
                </div>
                <div class="row partition">
                    <div class="col-lg-3">
                        <input type="text" class="custom-partition form-control"  [(ngModel)]="newCustomPartition.key" [ngbTypeahead]="searchKeys" />
                    </div>
                    <div class="col-lg-3">                        
                        <dropdown class="custom-partition" (dropdownChanged)="changeDropdown($event)" [options]="chartTypeOptions" placeholderText="String"> </dropdown>
                    </div>
                    <div class="col-lg-3">
                        <input type="text" class="custom-partition form-control"  [(ngModel)]="newCustomPartition.parts" [ngbTypeahead]="searchParts" />
                    </div>
                </div>
                <div class="add-partition">
                    <button (click)="addCustomPartition(newCustomPartition)" class="btn btn-outline-primary"> + Add Option </button>
                </div>
                <div class="update-partitions">
                    <button (click)="updateCustomPartitions(customPartitions)" class="form-control l-primary btn"> Update Custom Partitions </button>
                </div>
            </div>
        </ng-container>

    `,
    styles: [`
        th {
            width: 20%;
        }
        .row.partition {
            margin-bottom: 15px;
        }

        .partition i {
            line-height: 35px;
        }

        .add-partition {
            margin-bottom: 30px;
        }

        .update-partitions {
            margin-bottom: 15px;
        }
`]
})

export class MetricPartitionsComponent implements OnInit {
    @Input() context: any;
    private _apiHandler: CloudGemMetricApi;
    private predefinedPartitions: Array<Partition>;
    private customPartitions: Array<Partition>;
    private newCustomPartition: Partition;

    private isLoadingPredefinedPartitions: boolean = true;
    private isLoadingCustomPartitions: boolean = true;
    private chartTypeOptions = [
        { text: "Dictionary", type: "map" },
        { text: "Float", type: "float" },
        { text: "Integer", type: "int" },
        { text: "String", type: "str" }, 
        { text: "UTCDatetime", type: "datetime.datetime.utcfromtimestamp" },
    ]

    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager) { }

    // TODO: This is an example list - needs updated and perhaps better filtering prior to release
    keys = [];
    types = ['string', 'int', 'datetime', 'float'];
    parts = ['split("/")[0]']    

    ngOnInit() {
        this._apiHandler = new CloudGemMetricApi(this.context.ServiceUrl, this.http, this.aws);
        this.initPartitionsFacet();
    }

    initPartitionsFacet = () => {
        this.predefinedPartitions = new Array<Partition>();
        this.newCustomPartition = Partition.default();

        this._apiHandler.getPredefinedPartitions().subscribe( (res) => {
            let partitions = JSON.parse(res.body.text()).result;
            this.isLoadingPredefinedPartitions = false;
            if (partitions) {
                for (let partition of partitions) {
                    this.predefinedPartitions.push(new Partition(partition.parts, partition.type, partition.key, partition.description));
                }
            }
        });
        this.initCustomPartitions();
    }

    changeDropdown = (current) => {
        this.newCustomPartition.type = current.type;
    }

    initCustomPartitions = () => {
        this.customPartitions = new Array<Partition>();
        this._apiHandler.getCustomPartitions().subscribe( (res) => {
            let partitions = JSON.parse(res.body.text()).result;
            this.isLoadingCustomPartitions = false;
            if (partitions) {
                for (let partition of partitions) {
                    this.customPartitions.push(new Partition(partition.parts, partition.type, partition.key, partition.description));
                }
            }
        });
    }

    searchKeys = (text$: Observable<string>) =>
        text$.debounceTime(200)
        .distinctUntilChanged()
        .map(term =>
        this.keys.filter(v => v.toLocaleLowerCase().indexOf(term.toLocaleLowerCase()) > -1).slice(0, 10));

    searchTypes = (text$: Observable<string>) =>
        text$.debounceTime(200)
        .distinctUntilChanged()
        .map(term =>
        this.types.filter(v => v.toLocaleLowerCase().indexOf(term.toLocaleLowerCase()) > -1).slice(0, 10));

    searchParts = (text$: Observable<string>) =>
        text$.debounceTime(200)
        .distinctUntilChanged()
        .map(term =>
        this.parts.filter(v => v.toLocaleLowerCase().indexOf(term.toLocaleLowerCase()) > -1).slice(0, 10));


    /** Add a partition to the list of custom partitions.  This is only locally shown until the button is clicked to send the update to the server */
    addCustomPartition = () => {
        this.customPartitions.push(this.newCustomPartition)
        this.newCustomPartition = Partition.default();
    }

    /** Remove a partition from the list of custom partitions */
    removeCustomPartition = partition => this.customPartitions.splice(this.customPartitions.indexOf(partition), 1);

    /** Update the current partition list on the server and then reload the response from the server */
    updateCustomPartitions = (partitions: Array<Partition>) => {
        this.isLoadingCustomPartitions = true;
        
        var partitionsObj = {
            "partitions": partitions
        }

        if (this.newCustomPartition.key && this.newCustomPartition.key !== "") {
            partitionsObj['partitions'].push(this.newCustomPartition);
            this.newCustomPartition = Partition.default();
        }

        for (let partition in partitions) {
            console.log(partitions[partition].parts)
            if (!partitions[partition].parts || partitions[partition].parts.length == 0)
                delete partitions[partition].parts
            delete partitions[partition].description
        }        
        
        this._apiHandler.updatePartitions(partitionsObj).subscribe( (res) => {
            this.toastr.success("Updated custom partitions");
            this.initCustomPartitions();
        }, (err) => {
            this.toastr.error("Unable to update custom partitions", err);
            this.initCustomPartitions();
        });
    }
}

/** Partition model */
export class Partition {
    parts: string[];
    type: string;
    key: string;
    description: string;

    constructor( parts: string[], type: string, key: string, description: string) {
        this.parts = parts;
        this.type = type;
        this.key = key;
        this.description = description;
    }

    /** Default partition impl */
    static default = () => new Partition([], "string", "", "");
}
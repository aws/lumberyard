import { Component, Input, ViewEncapsulation, OnInit, trigger, ViewChild } from '@angular/core';
import { AwsService } from "app/aws/aws.service";
import { AwsContext } from "app/aws/context.class";
import { ApiHandler, StringUtil } from 'app/shared/class/index';
import { Http } from '@angular/http';
import { Schema } from 'app/aws/schema.class';
import { Facetable } from '../../class/index';

@Component({
    selector: 'facet-metric',
    template: `                   
           <div class="row"><barchart [data]="bar1" [title]="'My Test Bar Chart'" [x-axis-label]="'ages'" [y-axis-label]="'Number of People'" [width]="400" [height]="300"></barchart></div>
           <div class="row"><piechart [data]="bar1" [title]="'My Test Bar Chart'" [width]="400" [height]="300"></piechart></div>

           <div class="row"><barchart [data]="bar2" [title]="'My Test Bar Chart'" [x-axis-label]="'ages'" [y-axis-label]="'Number of People'" [width]="900" [height]="600"></barchart></div>
           <div class="row"><piechart [data]="pie2" [title]="'My Test Bar Chart'" [width]="600" [height]="600"></piechart></div>

           <div class="row"><barchart [data]="bar2" [no-yaxis-labels]="noYAxisLabels" [width]="150" [height]="100"></barchart></div>
           <div class="row"><piechart [data]="pie2" [width]="150" [height]="150"></piechart></div>
    `,
    styleUrls: []    
})
export class MetricComponent implements Facetable {
    @Input() data: any;    

    private noYAxisLabels: boolean = true;

    private bar1 = [
        { label: "<5", value: 1203 },
        { label: "5-13", value: 50 },
        { label: "14-17", value: 123 },
        { label: "18-24", value: 119 },
        { label: "25-44", value: 5 },
        { label: "45-64", value: 785 },
        { label: "≥65", value: 964 }
    ];

    private bar2 = [
        { label: "<5", value: 1233, color: "#98abc5" },
        { label: "5-13", value: 50, color: "#8a89a6" },
        { label: "14-17", value: 123, color: "#7b6888" },
        { label: "18-24", value: 119, color: "#6b486b" },
        { label: "25-44", value: 5, color: "#a05d56" },
        { label: "45-64", value: 785, color: "#d0743c" },
        { label: "≥65", value: 964, color: "#ff8c00" }
    ];

    private pie1 = [
        { label: "<5", value: 2704659 },
        { label: "5-13", value: 4499890 },
        { label: "14-17", value: 2159981 },
        { label: "18-24", value: 3853788 },
        { label: "25-44", value: 14106543 },
        { label: "45-64", value: 8819342 },
        { label: "≥65", value: 612463 }
    ];

    private pie2 = [
        { label: "<5", value: 2704659, color: "#98abc5" },
        { label: "5-13", value: 4499890, color: "#8a89a6" },
        { label: "14-17", value: 2159981, color: "#7b6888" },
        { label: "18-24", value: 3853788, color: "#6b486b" },
        { label: "25-44", value: 14106543, color: "#a05d56" },
        { label: "45-64", value: 8819342, color: "#d0743c" },
        { label: "≥65", value: 612463, color: "#ff8c00" }
    ];

    private model = {

    }

    constructor(private aws: AwsService, private http: Http) {        
        
    }

   
}


import { Input, Component, ViewChild, ElementRef } from '@angular/core';
import { AbstractCloudGemIndexComponent } from 'app/view/game/module/cloudgems/class/index';
import { DefinitionService, LyMetricService } from 'app/shared/service/index';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { CloudGemDefectReporterApi } from './index';

// System variable scopes functions/variables found in Systemjs for use without polluting the global scope
declare var System: any

export enum Pages {
    DefectList = 0,
    DefectDetails = 1,
}

@Component({
    selector: 'cloudgemdefectreporter-index',
    templateUrl: 'node_modules/@cloud-gems/cloudgemdefectreporter/index.component.html'
})

export class CloudGemDefectReporterIndexComponent extends AbstractCloudGemIndexComponent {
    @Input() context: any;  //REQUIRED

    private _metricApiHandler: any;
    private showingPage: Pages;
    private showingDefect: Object;

    @ViewChild('DefectListPage') defectListPage;
  
    constructor(private http: Http, private aws: AwsService, private definition: DefinitionService, private metric: LyMetricService) {
        super();
    }
  
    ngOnInit() {                    
        let metric_service = this.definition.getService("CloudGemMetric");
        this._metricApiHandler = new metric_service.constructor(metric_service.serviceUrl, this.http, this.aws, this.metric, this.context.identifier);
        this.showingPage = Pages.DefectList;
    }


    public getMetricApiHandler(): any {
    // Getter for Metric Api Handler

        return this._metricApiHandler;
    }


    private toDefectDetailsPage = (defect: any) => {
    // Set defect detail page as the showing page

        if (!defect) {
            return;
        }

        this.showingDefect = defect;
        this.showingPage = Pages.DefectDetails;
    }

    private toDefectListPage = () => {
    // Set defect list page as the showing page

        this.showingPage = Pages.DefectList;
        this.showingDefect = {};

        if (this.defectListPage.tabIndex === 0) {
            this.defectListPage.datatable.updateFilteredRows();
        }
        else {
            this.defectListPage.bookmarktable.updateFilteredRows();
        }
    }      
}
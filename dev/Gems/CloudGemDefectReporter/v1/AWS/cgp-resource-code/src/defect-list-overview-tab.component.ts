import { ViewChild, Input, Component, Output, EventEmitter } from '@angular/core';
import { LyMetricService } from 'app/shared/service/index';
import { AwsService } from "app/aws/aws.service";
import { ToastsManager } from 'ng2-toastr';
import { ModalComponent } from 'app/shared/component/index';


@Component({
    selector: 'defect-list-overview-tab',
    templateUrl: 'node_modules/@cloud-gems/cloudgemdefectreporter/defect-list-overview-tab.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemdefectreporter/defect-list-page.component.css']
})

export class CloudGemDefectReporterDefectListOverviewTabComponent {
    @Input() metricApiHandler: any;
    @Input() defectReporterApiHandler: any;
    @Input() isJiraIntegrationEnabled: any;
    @Input() partialInputQuery: string;
    @Input() toDefectDetailsPageCallback: Function;
    @Output() updateJiraMappings = new EventEmitter<any>();

    public limit: string = '50';

    @ViewChild('datatable') datatable;
    @ViewChild('searchButton') searchButton;

    constructor(private aws: AwsService, private toastr: ToastsManager, private metric: LyMetricService) {
    }

    /**
    * Setter for limit to be used in query.
    * @param limit  String limit to be sent along with ANSI SQL query.
    **/
    public setLimit(limit: string): void {
        this.limit = limit;
    }
}
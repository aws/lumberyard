import { Input, Component } from '@angular/core';
import { LyMetricService } from 'app/shared/service/index';

@Component({
    selector: 'raw-data-tab',
    templateUrl: 'node_modules/@cloud-gems/cloudgemdefectreporter/raw-data-tab.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemdefectreporter/defect-details-page.component.css']
})


export class CloudGemDefectReporterRawDataTabComponent {
    @Input() rawDataKeys: any;
    @Input() defect: any;
    @Input() isLoading: any;

    constructor(private metric: LyMetricService) {
    }

}
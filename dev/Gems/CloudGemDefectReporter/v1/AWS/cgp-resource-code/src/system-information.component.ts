import { Input, Component } from '@angular/core';
import { LyMetricService } from 'app/shared/service/index';

@Component({
    selector: 'system-information',
    templateUrl: 'node_modules/@cloud-gems/cloudgemdefectreporter/system-information.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemdefectreporter/defect-details-page.component.css']
})


export class CloudGemDefectReporterSystemInformationComponent {
    @Input() systemInformation: any;
    @Input() configurationMappings: any;

    constructor(private metric: LyMetricService) {
    }
}
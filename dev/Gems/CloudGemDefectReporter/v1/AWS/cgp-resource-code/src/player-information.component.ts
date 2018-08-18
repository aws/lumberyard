import { Input, Component } from '@angular/core';
import { LyMetricService } from 'app/shared/service/index';

@Component({
    selector: 'player-information',
    templateUrl: 'node_modules/@cloud-gems/cloudgemdefectreporter/player-information.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemdefectreporter/defect-details-page.component.css']
})


export class CloudGemDefectReporterPlayerInformationComponent {
    @Input() playerInformation: any;
    @Input() configurationMappings: any;

    constructor(private metric: LyMetricService) {
    }
}
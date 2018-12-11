import { Input, Component, Output, EventEmitter } from '@angular/core';
import { LyMetricService } from 'app/shared/service/index';

@Component({
    selector: 'client-configuration-field',
    templateUrl: 'node_modules/@cloud-gems/cloudgemdefectreporter/client-configuration-field.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemdefectreporter/client-configuration.component.css']
})


export class CloudGemDefectReporterClientConfigurationFieldComponent {
    @Input() field: any;
    @Input() fieldIdPrefix: any;
    @Input() isObjectProperty: any;
    @Output() showModifyField = new EventEmitter<any>();
    @Output() showDeleteField = new EventEmitter<any>();
    @Output() addNewProperty = new EventEmitter<any>();

    constructor(private metric: LyMetricService) {
    }

    ngOnInit() {
        let type = this.field['type'];
        if (this.field['defaultValue'] === undefined) {
            if (type === 'text') {
                this.field['defaultValue'] = '';
            }
            else if (type === 'object') {
                this.field['defaultValue'] = {};
            }
            else if (type === 'predefined') {
                if (this.field['multipleSelect'] === true) {
                    this.field['defaultValue'] = [];
                    for (let i = 0; i < this.field['predefines'].length; ++i) {
                        this.field['defaultValue'].push(false);
                    }             
                }
                else {
                    this.field['defaultValue'] = '';
                }
            }
        }
    }
}
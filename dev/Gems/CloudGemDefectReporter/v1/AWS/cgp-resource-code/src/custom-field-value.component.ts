import { Input, Component, Output, EventEmitter } from '@angular/core';
import { LyMetricService } from 'app/shared/service/index';
import { ToastsManager } from 'ng2-toastr';
import { Observable } from 'rxjs/Observable';

@Component({
    selector: 'custom-field-value',
    templateUrl: 'node_modules/@cloud-gems/cloudgemdefectreporter/custom-field-value.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemdefectreporter/custom-field-value.component.css']
})


export class CloudGemDefectReporterCustomFieldValueComponent {
    @Input() jiraSchema: any;
    @Input() reportValue: any;
    @Input() uniqueId: any;
    @Input() possibleFields: any;
    @Output() reportValueChange = new EventEmitter<any>();

    constructor(private toastr: ToastsManager, private metric: LyMetricService) {
        if (this.reportValue === '' || this.reportValue === undefined || this.reportValue === null) {
            this.reportValue = {};
        }
    }

    private updateObjectProperty(prop, newValue): void {
        if (this.reportValue === '' || this.reportValue === undefined || this.reportValue === null) {
            this.reportValue = {};
        }

        if (typeof this.reportValue[prop] === "string" || typeof this.reportValue[prop] === "number") {
            this.reportValue[prop] = newValue.trim() === '' ? null : newValue.trim()
        } else {
            this.reportValue[prop] = newValue
        }

        this.reportValueChange.emit(this.reportValue);
    }

    private updateStandardValue(newValue): void{
        this.reportValue = newValue;
        this.reportValueChange.emit(this.reportValue);

    }

}
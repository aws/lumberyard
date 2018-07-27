import { Component, Input, Output, EventEmitter } from '@angular/core'
import { LyMetricService } from 'app/shared/service/index';

@Component({
    selector: 'report-configuration',
    templateUrl: 'node_modules/@cloud-gems/cloudgemdefectreporter/report-configuration.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemdefectreporter/report-configuration.component.scss']
})

export class CloudGemDefectReporterReportConfigurationComponent {
    @Input() mappings: any;
    @Output() mappingsChange = new EventEmitter();

    private tableDropdownOptions: any[];
    private displayCategories = ['Report Information', 'Player Information', 'System Information'];

    constructor(private metric: LyMetricService) {
    }

    ngOnInit() {
        this.tableDropdownOptions = new Array<any>();
        for (let category of this.displayCategories) {
            this.tableDropdownOptions.push({ text: category })
        }
    }

    /**
    * Add a new configuration mapping for the detail page
    **/
    private addNewMapping(): void {
        let newMapping = { 'key': '', 'displayName': '', 'category': '' };
        this.mappings.unshift(newMapping);
        this.mappingsChange.emit(this.mappings);
        this.updateMappings();
    }

    /**
    * Delete configuration mapping for the detail page
    * @param index The index of the mapping to delete
    **/
    private deleteMapping(index: number): void {
        this.mappings.splice(index, 1);
        this.updateMappings();
    }

    /**
    * Update the category of the data
    * @param option current option of the dropdown
    * @param mapping the mapping to change
    **/
    private updateCategory(option, mapping): void {
        mapping['category'] = option.text;
        this.updateMappings();
    }

    /**
    * Update the configuration mappings and store it in the local cache
    **/
    private updateMappings(): void {
        this.mappingsChange.emit(this.mappings);
        window.localStorage.setItem("configurationMappings", JSON.stringify(this.mappings));
    }
}
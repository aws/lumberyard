import { Input, Component, Output, EventEmitter } from '@angular/core';
import { LyMetricService } from 'app/shared/service/index';
import { CloudGemDefectReporterApi } from './index';
import { ToastsManager } from 'ng2-toastr';
import { Observable } from 'rxjs/Observable';

@Component({
    selector: 'create-jira-issue-window',
    templateUrl: 'node_modules/@cloud-gems/cloudgemdefectreporter/create-jira-issue-window.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemdefectreporter/create-jira-issue-window.component.css']
})


export class CloudGemDefectReporterCreateJiraIssueWindowComponent {        
    @Input() defectReporterApiHandler: any;
    @Input() defectGroupMapping: any;
    @Output() defectChange = new EventEmitter<any>();
    @Output() updateJiraMappings = new EventEmitter<any>();
    @Output() validationFailed = new EventEmitter<any>();

    private isLoadingJiraFieldMappings = false;
    private isLoadingIssueTypes = false;
    private jiraFieldMappings: Object;
    private originalJiraFieldMappings: string;    
    private projectDefault: string;    
    private defectMapping: any;
    private grouped: any;
    private searchFields: string[];
    private defectEvent: any;
    private projectKeys = [];
    private issueTypes = [];

    constructor(private toastr: ToastsManager, private metric: LyMetricService) {

    }

    ngOnInit() {
        this.defectEvent = JSON.parse(JSON.stringify(this.defectMapping));
        this.jiraFieldMappings = {
            project: "",
            issuetype: "",
            fields: []
        };
    }

    @Input()
    set defectReport(value) {
        this.defectMapping = value;      

        this.searchFields = []
        for (var prop in this.defectMapping) {
            this.searchFields.push(prop)
        }

        if (this.jiraFieldMappings !== undefined) {
            this.jiraFieldMappings['fields'] = JSON.parse(this.originalJiraFieldMappings).result;
            this.validateJiraFieldMappings();
        }      
    }

    @Input()
    set groupMode(value) {
        this.grouped = value;
        this.update();
    }

    private get defect(): any {
        return this.grouped ? this.defectGroupMapping : this.defectMapping;        
    }

    private update(): void {        
        if (this.jiraFieldMappings === undefined) {            
            this.initializeJiraFieldContent();
        } else {
            this.jiraFieldMappings['fields'] = JSON.parse(this.originalJiraFieldMappings).result;
            this.validateJiraFieldMappings();
        }
    }

    /**
    * Retrieve the Jira integration settings and get the field mappings
    **/
    private initializeJiraFieldContent(): void {
        this.isLoadingJiraFieldMappings = true;
        this.defectReporterApiHandler.getJiraIntegrationSettings().subscribe(
            response => {
                let obj = JSON.parse(response.body.text());
                this.projectDefault = obj.result.projectDefault;
                this.jiraFieldMappings['project'] = obj.result.projectDefault === null || obj.result.projectDefault === undefined ? obj.result.project : this.projectDefault
                this.jiraFieldMappings['issuetype'] = obj.result.issuetype
                this.getProjectKeys();
                this.getIssueTypes(this.jiraFieldMappings['project']);
                this.getJiraFieldMappings(this.jiraFieldMappings['project'], this.jiraFieldMappings['issuetype']);
            },
            err => {                
                this.toastr.error(`Failed to get the Jira integration settings. The received error was '${err.message}'`);
                this.isLoadingJiraFieldMappings = false;
            }
        );
    }

    /**
    * Retrieve the Jira field mappings based on project key and issue type
    **/
    private getJiraFieldMappings(projectKey, issueType): void {        
        this.defectReporterApiHandler.getFieldMappings(projectKey, issueType).subscribe(
            response => {
                this.originalJiraFieldMappings = response.body.text()                 
                this.jiraFieldMappings['fields'] = JSON.parse(this.originalJiraFieldMappings).result;
                this.validateJiraFieldMappings();
                this.isLoadingJiraFieldMappings = false;
            },
            err => {                
                this.isLoadingJiraFieldMappings = false;
            }
        );
    }

    /**
   * Get all the project keys
   * @returns Observable of the method to subscribe to.
   **/
    private getProjectKeys(): Observable<any> {        
        return this.defectReporterApiHandler.getProjectKeys().subscribe(
            response => {                
                this.projectKeys = JSON.parse(response.body.text()).result.projectKeys;                
            },
            err => {               
            });
    }

    /**
    * Get all the issue types of the current project
    * @returns Observable of the method to subscribe to.
    **/
    private getIssueTypes(projectKey: string): Observable<any> {        
        this.issueTypes = [];
        this.isLoadingIssueTypes = true;
        return this.defectReporterApiHandler.getIssueTypes(projectKey).subscribe(
            response => {
                this.issueTypes = JSON.parse(response.body.text()).result.issueTypes;                
                this.isLoadingIssueTypes = false;
            },
            err => {               
                this.isLoadingIssueTypes = false;
            });;
    }


    private replaceAllNested(object, defect) {
        if (Object.keys(defect).length == 0)
            return object

        if (object instanceof Object || object instanceof Array) {
            for (var prop in object) {
                var result = this.replaceAllNested(object[prop], defect);
                object[prop] = result;
            }
        } else {
            var result = this.replace(object, defect);
            return result;
        }  
        return object      
    }

    private replace(value, defect) {        
        if (!(typeof value == "string"))
            return value
        
        var re = /\[\s*\w*\s*\]/gi;
        var m;
        var result = value;
        while ((m = re.exec(value)) !== null) {
            var found = value.substring(m.index).match(re)[0];         
            var embedded_key = found.substring(1, found.length - 1)
            if (embedded_key in defect) {
                var entry = defect[embedded_key];
                if (entry) {
                    var replace = entry.constructor == Object && 'value' in entry ? entry["value"] : entry;
                    result = result.replace(found, replace);
                }
            }
            value = value.replace(found, "");            
        }
        return result
    }

    /**
    * Validate the mappings according to the Jira field schema
    **/
    private validateJiraFieldMappings(): void {
        let summaryIndex = 0;
        let simplifiedDefect = this.grouped ? {} : {            
            attachment_id: this.defect['attachment_id'],
            universal_unique_identifier: this.defect['universal_unique_identifier'],
        };
        
        for (let i = 0; i < this.jiraFieldMappings['fields'].length; i++) {
            let item = this.jiraFieldMappings['fields'][i];
            let entry = item['mapping'];
            let isArrayType = item['isArrayType'] = item['schema']['type'] == 'array'
            let isArrayOfPrimitives = item['isArrayOfPrimitives'] = isArrayType && item['schema']['items']['type'] !== "object"
            let isArrayOfObjects = item['isArrayOfObjects'] = isArrayType && item['schema']['items']['type'] === "object"
            let isObjectType = item['isObjectType'] = item['schema']['type'] == 'object' && item['schema']['properties'] !== undefined
            
                
            if (isArrayOfObjects) {
                delete item['schema']['items']['properties']['self']
            } else if (isObjectType) {
                delete item['schema']['properties']['self']
            }            

            if (entry !== '' && Object.keys(entry).length > 0) {
                //custom object passed by the client stored as a string
                if (this.defect[entry]) {
                    if (!this.validateReportDataFormat(item['schema'], this.defect[entry]['value'])) {
                        this.toastr.error(`Defect reporter property '${item['mapping']}' doesn't match the schema of Jira field '${item['name']}'.`);
                        this.validationFailed.emit();
                    }
                    
                    simplifiedDefect[item["id"]] = this.defect[entry];

                    /**Set the 'Summary' field to the first element in the array**/
                    if (item['id'] === 'summary') {
                        this.jiraFieldMappings['fields'].splice(i, 1);
                        this.jiraFieldMappings['fields'].splice(0, 0, item);
                    }
                } else {
                    if (!this.grouped)
                        entry = this.replaceAllNested(entry, this.defect);                    
                    simplifiedDefect[item["id"]] = {
                        value: entry,
                        valid: true
                    }                    
                }
            } else if (item['required']) {                
                if (isArrayOfObjects || isObjectType) {

                    simplifiedDefect[item["id"]] = { valid: true, value: isArrayType ? [] : {} };

                    let properties = isArrayOfObjects ? item['schema']['items']['properties'] : item['schema']['properties']
                    let obj = {}
                    for (const prop of Object.getOwnPropertyNames(properties)) {
                        if (properties[prop]['type'] == "boolean") {
                            obj[prop] = false
                        } else {
                            obj[prop] = null
                        }
                    }

                    if (isArrayOfObjects) {
                        simplifiedDefect[item["id"]]['value'].push(obj)
                    } else {
                        simplifiedDefect[item["id"]]['value'] = obj
                    }
                } else {
                    simplifiedDefect[item["id"]] = {
                        value: entry,
                        valid: true
                    }    
                }
            }
            item["mapping"] = item["id"];
        }

        // Remove the report fields which are not mapped to the Jira fields. This helps to reduce the playload sent to the service lambda
        if (this.grouped) {
            this.defectGroupMapping = simplifiedDefect;
        } else {
            this.defectMapping = simplifiedDefect;            
        }        
        this.defect['project'] = this.jiraFieldMappings['project'];
        this.defect['issuetype'] = this.jiraFieldMappings['issuetype'];
        this.defectChange.emit(this.defect);
    }

    /**
    * Check the format of the report data
    * @param schema the schema of the Jira field
    * @param reportData the report data
    **/
    private validateReportDataFormat(schema, reportData): boolean {
        if (reportData !== '') {
            let reportDataType = typeof reportData;
            let standard_data_type = ['string', 'number', 'boolean', 'object'];

            if (standard_data_type.indexOf(schema['type']) > -1 && schema['type'] !== reportDataType) {
                return false;
            }

            if (schema['type'] === 'array') {
                if (Array.isArray(reportData) && reportData.length > 0) {
                    return this.validateReportDataFormat(schema['items'], reportData[0]);
                }
                else {
                    return false;
                }
            }
        }

        return true;
    }
   
    /**
    * Add a new element to the array
    * @param reportField the report field
    * @param type the type of the element to add
    **/
    private addNewArrayElement(reportField, type): void {
        if (reportField.value === '') {
            reportField.value = [];
        }

        if (type === 'object') {
            reportField.value.push({})
        }
        else if (type === 'number') {
            reportField.value.push(0)
        }
        else if (type === 'boolean') {
            reportField.value.push(false)
        } else {
            reportField.value.push('')
        }
    }

    /**
    * Remove an element to the array
    * @param reportField the report field
    * @param index the index of the element to remove
    **/
    private removeArrayElement(reportField, index): void {
        reportField.value.splice(index, 1);
    }

    /**
    * Validate the required fields
    **/
    private validateJiraFields(): boolean {
        let valid = true;
        for (let item of this.jiraFieldMappings['fields']) {
            let mapping = item['mapping'];
            if (mapping) {
                let defect_info = this.defect[mapping]
                if (!defect_info) {
                    continue;
                }
                var required = item['required']
                delete defect_info.valid;
                let type = item['schema']['type']
                var isempty = this.emptyReportField(defect_info);

                defect_info.valid = required ? required && !isempty : true
                if (!defect_info.valid) {
                    valid = false;
                }
            }
        }

        return valid;
    }

    /**
    * Check whether the report field is empty
    * @param reportData the report data to validate
    **/
    private emptyReportField(reportData): boolean {        
        let result = true;
        for (var key in reportData) {            
            let value = reportData[key]           

            if (value === null) {
                result = result && true;             
            } else if (typeof value === 'string' && value.trim() !== '') {
                return false;
            } else if (typeof value === 'object') {                
                return this.emptyReportField(value);                
            }
        }        
        return result;
    }


    private isArray(value): boolean {
        let result = Array.isArray(value);        
        return result;
    }

    /**
        * Retrieve the actual report data after validation
        **/
    retriveReportData = (report, source_row=null): any => {
        let result = {}
        source_row = source_row ? source_row : this.defectEvent;
        if (!Object.is(report, source_row ) && !this.grouped )
            report = this.replaceAllNested(report, source_row);    

        for (let key of Object.keys(report)) {
            let valueType = typeof report[key]['value'];
            result[key] = (valueType === 'string' || valueType === 'number' || valueType === 'boolean') ? report[key]['value'] : report[key]['value'];
        }       
        result['project'] = report['project'];
        result['issuetype'] = report['issuetype'];
        return result
    }

}
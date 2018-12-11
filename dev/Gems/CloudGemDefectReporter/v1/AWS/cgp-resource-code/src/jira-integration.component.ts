import { Component, Input, OnInit, ViewChild, ViewContainerRef } from '@angular/core'
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { ToastsManager } from 'ng2-toastr';
import { LyMetricService } from 'app/shared/service/index';
import { CloudGemDefectReporterApi } from './index';
import { Observable } from 'rxjs/Observable';
import 'rxjs/add/operator/distinctUntilChanged'

@Component({
    selector: 'jira-integration',
    templateUrl: 'node_modules/@cloud-gems/cloudgemdefectreporter/jira-integration.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemdefectreporter/jira-integration.component.css']
})

export class CloudGemDefectReporterJiraIntegrationComponent {
    @Input() context: any;
    @Input() isLoadingReportFields: any;
    @Input() reportFields: any;

    private _apiHandler: CloudGemDefectReporterApi;
    private _reportFields: any;

    private jiraIntegrationSettings: Object;
    private projectKeys = [];
    private issueTypes = [];
    private submitModes = ['manual', 'auto'];
    private fieldMappings = [];

    private isLoadingjiraIntegrationSettings = false;
    private isLoadingProjectList = false;
    private isLoadingIssueTypeList = false;
    private isLoadingfieldMappings = false;

    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager, private metric: LyMetricService) {
    }

    ngOnInit() {
        this._apiHandler = new CloudGemDefectReporterApi(this.context.ServiceUrl, this.http, this.aws, this.metric, this.context.identifier);

        this.jiraIntegrationSettings = {};
        this.getjiraIntegrationSettings();

        this.getJiraProperty(this.getProjectKeys(), 'projectKeys', 'project keys', 'isLoadingProjectList');
    }

    /**
    * Get the Jira Integration settings including submit mode, project key and issue type
    **/
    private getjiraIntegrationSettings(): void{
        this.fieldMappings = [];
        this.isLoadingjiraIntegrationSettings = true;
        this._apiHandler.getJiraIntegrationSettings().subscribe(
            response => {
                let obj = JSON.parse(response.body.text());
                for (let key of Object.keys(obj.result)) {
                    this.jiraIntegrationSettings[key] = { 'value': obj.result[key], 'valid': true, 'message': `Invalid ${key}`};
                }

                if (this.jiraIntegrationSettings['project']['value'] !== '') {
                    this.getJiraProperty(this.getIssueTypes(), 'issueTypes', 'issue types', 'isLoadingIssueTypeList');

                    if (this.jiraIntegrationSettings['issuetype']['value'] !== '') {
                        this.getFieldMappings();
                    }
                }

                this.isLoadingjiraIntegrationSettings = false;
            },
            err => {
                this.toastr.error(`Failed to get the field mappings. The received error was '${err.message}'`);
                this.isLoadingjiraIntegrationSettings = false;
            }
        );
    }

    /**
    * Get all the project keys
    * @returns Observable of the method to subscribe to.
    **/
    private getProjectKeys(): Observable<any> {
        return this._apiHandler.getProjectKeys();
    }

    /**
    * Get all the issue types of the current project
    * @returns Observable of the method to subscribe to.
    **/
    private getIssueTypes(): Observable<any> {
        this.issueTypes = [];
        return this._apiHandler.getIssueTypes(this.jiraIntegrationSettings['project']['value']);
    }

    /**
    * Get a property of the Jira ticket
    * @param method  The method to get the property
    * @param propertyName  The property name
    * @param displayName  The property name shown in the toastr message
    * @param flag  The flag to indicate whether the property value is available
    **/
    private getJiraProperty(method, propertyName, displayName, flag): void {
        this[flag] = true;
        method.subscribe(
            response => {
                let obj = JSON.parse(response.body.text());
                this[propertyName] = obj.result[propertyName];
                this[flag] = false;
            },
            err => {
                this.toastr.error(`Failed to get ${displayName}. ` + `The received error was '${err.message}'`);
                this[flag] = false;
            }
        );
    }

    /**
    * Get the field mappings for the current issue type
    **/
    private getFieldMappings(): void {
        this.fieldMappings = [];

        this.isLoadingfieldMappings = true;
        this._apiHandler.getFieldMappings(this.jiraIntegrationSettings['project']['value'], this.jiraIntegrationSettings['issuetype']['value']).subscribe(
            response => {
                let obj = JSON.parse(response.body.text());
                this.fieldMappings = [];
                for (let fieldMapping of obj.result) {
                    fieldMapping['valid'] = true;
                    fieldMapping['message'] = 'Invalid defect report field.';
                    this.fieldMappings.push(fieldMapping);
                }
                this.isLoadingfieldMappings = false;
                this.validateJiraFieldMappings();

            },
            err => {
                this.toastr.error(`Failed to get the field mappings. The received error was '${err.message}'`);
                this.isLoadingfieldMappings = false;
            }
        );
    }

    validateJiraFieldMappings = () => {
      this.fieldMappings.forEach((item) => {
        let isArrayType = item['isArrayType'] = item['schema']['type'] === 'array'
        let isArrayOfPrimitives = item['isArrayOfPrimitives'] = isArrayType && item['schema']['items']['type'] !== "object"
        let isArrayOfObjects = item['isArrayOfObjects'] = isArrayType && item['schema']['items']['type'] === "object"
        let isObjectType = item['isObjectType'] = item['schema']['type'] === 'object' && item['schema']['properties'] !== undefined

        // convert any array types to be an array if not already
        if (isArrayType && item.mapping === '') {
            item.mapping = [];
        }

        if (isObjectType && item.mapping === '') {
            item.mapping = {};
        }

        if (isArrayOfObjects && item.mapping === '') {
            item.mapping = [{}];
        }
        if (isArrayOfPrimitives && item.mapping === '') {
            item.mapping = [];
        }

        // convert any number types to string types so autocomplete works as intended
        if(item.schema.type === 'number') {
            item.schema.type = 'string';
        }
      });
    }

    /**
    * Save the Jira integration settings and field mappings
    * @returns Observable of the method to subscribe to.
    **/
    private saveSettings(): Observable<any> {
        let settings = JSON.parse(JSON.stringify(this.fieldMappings));
        for (let setting of settings) {
            delete setting.valid;
            delete setting.message;
            if (settings.mappings && (settings.mappings === "" || settings.mapping.length === 0 || settings.mapping[0] === "")) {
                delete settings.mappings;
            }
        }

        settings.push(this.jiraIntegrationSetting('submitMode', 'Submit Mode', this.jiraIntegrationSettings['submitMode']['value'], true));
        settings.push(this.jiraIntegrationSetting('project', 'Project', this.jiraIntegrationSettings['project']['value'], true));
        settings.push(this.jiraIntegrationSetting('issuetype', 'Issue Type', this.jiraIntegrationSettings['issuetype']['value'], true));

        return this._apiHandler.updateFieldMappings(settings);
    }

    /**
    * Generate an object that holds the Jira integration setting
    * @param id  The id of the property
    * @param name  The name of the property
    * @param value  The value of the property
    * @param required  Whether the preperty is required or not
    * @returns an object that holds the Jira integration setting
    **/
    private jiraIntegrationSetting(id, name, value, required): Object {
        let item = {
            'id': id,
            'name': name,
            'mapping': value,
            'required': required
        }

        return item;
    }


    /**
    * Update a property of the Jira ticket
    * @param method  The method to get the property
    * @param displayName  The property name shown in the toastr message
    **/
    updateJiraProperty(method, displayName): void {
        method.subscribe(
            response => {
                this.toastr.success(`Succeeded to update ${displayName}.`);
            },
            err => {
                this.toastr.error(`Failed to update ${displayName}. ` + `The received error was '${err.message}'`);
            }
        );
    }

     /**
    * Add a new element to the array
    * @param reportField the report field
    * @param type the type of the element to add
    **/
   private addNewArrayElement(reportField, type): void {
    if (reportField === '') {
        reportField = [];
    }

    if (type === 'object') {
        reportField.push({})
    }
    else if (type === 'number') {
        reportField.push(0)
    }
    else if (type === 'boolean') {
        reportField.push(false)
    } else {
        reportField.push('')
    }
  }

  /**
  * Remove an element to the array
  * @param reportField the report field
  * @param index the index of the element to remove
  **/
  private removeArrayElement(reportField, index): void {
      reportField.splice(index, 1);;
  }

  private isArray = (value): boolean => Array.isArray(value);

  private trackByFn(index: any, item: any) {
      return index;
  }
}
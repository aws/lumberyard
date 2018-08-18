import { Component, Input, OnInit, ViewChild, ViewContainerRef } from '@angular/core'
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { DateTimeUtil } from 'app/shared/class/index';
import { ModalComponent } from 'app/shared/component/index';
import { ToastsManager } from 'ng2-toastr';
import { ActionItem } from "app/view/game/module/shared/class/index";
import { LyMetricService } from 'app/shared/service/index';
import { FormBuilder, FormGroup, FormArray, Validators } from '@angular/forms';
import { CloudGemDefectReporterApi } from './index';

enum EditMode {
    List,
    CreateField,
    EditField,
    DeleteField
}

@Component({
    selector: 'client-configuration',
    templateUrl: 'node_modules/@cloud-gems/cloudgemdefectreporter/client-configuration.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemdefectreporter/client-configuration.component.css']
})

export class CloudGemDefectReporterClientConfigurationComponent {
    @Input() context: any;

    private EditMode = EditMode;
    private editMode: EditMode;
    private _apiHandler: CloudGemDefectReporterApi;

    private customFields = [];
    private isLoadingClientConfiguration: boolean;
    private curEditingField: Object;
    private curFieldIndex: number;
    private fieldTitleForm: FormGroup;
    private predefinedFieldForm: FormGroup;
    private textFieldForm: FormGroup;

    private fieldTypes = [
        { 'typeinfo': { 'type': 'predefined', 'multipleSelect': true }, 'displayText': 'Multiple Choice (Checkboxes)' },
        { 'typeinfo': { 'type': 'predefined', 'multipleSelect': false }, 'displayText': 'Single Choice (Radio Buttons)' },
        { 'typeinfo': { 'type': 'text' }, 'displayText': 'Text' }
    ]

    private dummyRadioButtonForm: FormGroup;
    private dummyTextAreaModels: string[];

    @ViewChild(ModalComponent) modalRef: ModalComponent;

    constructor(private fb: FormBuilder, private http: Http, private aws: AwsService, private toastr: ToastsManager, private vcr: ViewContainerRef, private metric: LyMetricService) {
        // dummy radio button form group for the radio buttons to work in preview
        this.dummyRadioButtonForm = fb.group({
            'dummy': 'dummy'
        })
    }

    ngOnInit() {
        this._apiHandler = new CloudGemDefectReporterApi(this.context.ServiceUrl, this.http, this.aws, this.metric, this.context.identifier);

        this.getClientConfiguration();

        this.editMode = EditMode.List;
    }

    /**
    * Get the existing client configuration
    **/
    private getClientConfiguration(): void {
        this.isLoadingClientConfiguration = true;
        this._apiHandler.getClientConfiguration().subscribe(
            response => {
                let obj = JSON.parse(response.body.text());
                this.customFields = obj.result.clientConfiguration;
                this.dummyTextAreaModels = [];

                for (let customField of this.customFields) {
                    this.dummyTextAreaModels.push("");
                }

                this.isLoadingClientConfiguration = false;
            },
            err => {
                this.toastr.error("Failed to load the custom client configuration ", err);
                this.isLoadingClientConfiguration = false;
            }
        );
    }

    /**
    * Create custom field forms
    **/
    private createFieldForms(): void {
        this.createFieldTitleForm();
        this.createPredefinedFieldForm();
        this.createTextFieldForm();
    }

    /**
    * Create the field title form
    **/
    private createFieldTitleForm(): void {
        this.fieldTitleForm = this.fb.group({
            'title': [this.curEditingField["title"], Validators.compose([Validators.required])]
        });
    }

    /**
    * Create the check box or radio button options form
    **/
    private createPredefinedFieldForm(): void {
        this.predefinedFieldForm = this.fb.group({
            'options': this.fb.array([])
        });

        if (this.curEditingField["predefines"]) {
            let options = <FormArray>this.predefinedFieldForm.controls["options"];
            for (let option of this.curEditingField["predefines"]) {
                let optionForm = this.fb.group({
                    'value': [option, Validators.compose([Validators.required])]
                });

                options.push(optionForm);
            }
        }
    }

    /**
    * Create the text field form
    **/
    private createTextFieldForm(): void {
        this.textFieldForm = this.fb.group({
            'maxChars': [this.curEditingField["maxChars"], Validators.compose([Validators.required])]
        });
    }

    /**
    * Create a default custom field
    **/
    private createDefaultField(): Object {
        let defaultField = {};
        defaultField["type"] = "predefined";
        defaultField["multipleSelect"] = true;
        defaultField["predefines"] = [""];
        return defaultField;
    }

    /**
    * Update the type of the custom field
    * @param type the new type of the custom field
    **/
    private onChangeFieldType(fieldType: any): void {
        this.curEditingField["type"] = fieldType.typeinfo.type;
        if (this.curEditingField["type"] == 'predefined') {
            this.curEditingField["multipleSelect"] = fieldType.typeinfo.multipleSelect;
        }
    }

    /**
    * Check whether the required form field is empty
    * @param form the form to check
    * @param item the form field to check
    * @returns {boolean} whether the required form field is empty
    **/
    private isFormFieldRequiredEmpty(form: any, item: string): boolean {
        return (form.controls[item].hasError('required') && form.controls[item].touched)
    }

    /**
    * Check whether the form field is valid
    * @param form the form to check
    * @param item the form field to check
    * @returns {boolean} whether the required form field is valid
    **/
    private isFormFieldNotValid(form: any, item: string): boolean {
        return !form.controls[item].valid && form.controls[item].touched
    }

    /**
    * add a new option for the check box or radio button group
    **/
    private onAddOption(): void {
        let options = <FormArray>this.predefinedFieldForm.controls["options"];
        options.push(
            this.fb.group({
                'value': [null, Validators.compose([Validators.required])]
            })
        );
    }

    /**
    * add a new option of the check box or radio button group
    * @param optionIndex the index of the option
    **/
    private onDeleteOption(optionIndex: number): void {
        let options = <FormArray>this.predefinedFieldForm.controls["options"];
        options.removeAt(optionIndex);
    }

    /**
    * Modify a custom field
    **/
    private onModifyField(): void {
        if (!this.validateFieldForms()) {
            return;
        }

        this.extractFieldForm();
        this.customFields[this.curFieldIndex] = JSON.parse(JSON.stringify(this.curEditingField));
        this.updateClientConfiguration();

        this.modalRef.close();
    }

    /**
    * Add a new custom field
    **/
    private onAddField(): void {
        if (!this.validateFieldForms()) {
            return;
        }

        this.extractFieldForm();

        this.customFields.push(this.curEditingField);
        this.dummyTextAreaModels.push("");
        this.updateClientConfiguration();

        this.modalRef.close();
    }

    /**
    * Save the client configuration
    **/
    private updateClientConfiguration(): void{
        let body = { "clientConfiguration": this.customFields };
        this._apiHandler.updateClientConfiguration(body).subscribe(
            response => {
                this.toastr.success("The client configuration was saved successfully.")
            },
            err => {
                this.toastr.error("Failed to update the client configuration. ", err);
            }
        );
    }

    /**
    * validate the field forms
    **/
    private validateFieldForms(): boolean {
        let success = true;

        success = this.validateTitleFieldForm() && success;

        switch (this.curEditingField["type"]) {
            case "predefined":
                success = this.validatePredefinedFieldForm() && success;
                break;
            case "text":
                success = this.validateTextFieldForm() && success;
                break;
        }

        return success;
    }

    /**
    * validate the title field forms
    **/
    private validateTitleFieldForm(): boolean {
        this.fieldTitleForm.controls["title"].markAsTouched();
        return this.fieldTitleForm.controls["title"].valid;
    }

    /**
    * validate the text field forms
    **/
    private validateTextFieldForm(): boolean {
        this.textFieldForm.controls["maxChars"].markAsTouched();
        return this.textFieldForm.controls["maxChars"].valid;
    }

    /**
    * validate the check box or radio button field forms
    **/
    private validatePredefinedFieldForm(): boolean {
        let success = true;

        let options = <FormArray>this.predefinedFieldForm.controls["options"];
        for (let option of options.controls) {
            let optionForm = <FormGroup>option;
            optionForm.controls["value"].markAsTouched();

            success = success && optionForm.controls["value"].valid;
        }

        return success;
    }

    /**
    * extract the custom field form
    **/
    private extractFieldForm(): void {
        this.curEditingField["title"] = this.fieldTitleForm.value.title;

        switch (this.curEditingField["type"]) {
            case "predefined":
                let predefines = [];
                let options = <FormArray>this.predefinedFieldForm.controls["options"];
                for (let optionForm of options.controls) {
                    predefines.push(optionForm.value.value);
                }

                this.curEditingField["predefines"] = predefines;
                break;
            case "text":
                this.curEditingField["maxChars"] = this.textFieldForm.value.maxChars;
                break;
        }
    }

    /**
    * get the display name of each type
    **/
    private getFieldTypeDisplayText() {
        switch (this.curEditingField["type"]) {
            case "text":
                return 'Text';
            case "predefined":
                if (this.curEditingField["multipleSelect"]) {
                    return "Multiple Choice (Checkboxes)";
                } else {
                    return "Single Choice (Radio Buttons)";
                }
            default:
                return "unknown";
        }
    }

    /**
    * Delete the current custom field
    **/
    private onDeleteField(): void {
        this.customFields.splice(this.curFieldIndex, 1);
        this.dummyTextAreaModels.splice(this.curFieldIndex, 1);
        this.updateClientConfiguration();

        this.modalRef.close();
    }

    /**
    * Define all the modals
    **/
    private onShowModifyFieldModal(field: Object, fieldIndex: number): void {
        this.curEditingField = JSON.parse(JSON.stringify(field));
        if (!this.curEditingField["predefines"] || this.curEditingField["predefines"].length == 0) {
            this.curEditingField["predefines"] = [''];
        }

        this.createFieldForms();
        this.editMode = EditMode.EditField;
        this.curFieldIndex = fieldIndex;
    }

    private onShowAddNewFieldModal = () => {
        this.curEditingField = this.createDefaultField();
        this.createFieldForms();

        this.editMode = EditMode.CreateField;
    }

    private onShowDeleteFieldModal = (field: any, index: number) => {
        this.curEditingField = field;
        this.curFieldIndex = index;
        this.editMode = EditMode.DeleteField;
    }

    private onDismissModal = () => {
        this.editMode = EditMode.List;
    }
}
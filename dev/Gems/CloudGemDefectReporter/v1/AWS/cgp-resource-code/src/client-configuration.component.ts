import { Component, Input, OnInit, ViewChild, ViewContainerRef } from '@angular/core'
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { DateTimeUtil } from 'app/shared/class/index';
import { ModalComponent } from 'app/shared/component/index';
import { ToastsManager } from 'ng2-toastr';
import { ActionItem } from "app/view/game/module/shared/class/index";
import { LyMetricService } from 'app/shared/service/index';
import { FormBuilder, FormGroup, FormArray, Validators, FormControl } from '@angular/forms';
import { CloudGemDefectReporterApi, ValidationUtil } from './index';

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
    private isFormFieldNotPositiveNum = ValidationUtil.isFormFieldNotPositiveNum;

    private customFields = [];
    private curFields = [];
    private isLoadingClientConfiguration: boolean;
    private curEditingField: Object;
    private currentField: Object;
    private curFieldIndex: number;
    private fieldTitleForm: FormGroup;
    private predefinedFieldForm: FormGroup;
    private textFieldForm: FormGroup;
    private objectFieldForm: FormGroup;

    private fieldTypes = [
        { 'typeinfo': { 'type': 'predefined', 'multipleSelect': true }, 'displayText': 'Multiple Choice (Checkboxes)' },
        { 'typeinfo': { 'type': 'predefined', 'multipleSelect': false }, 'displayText': 'Single Choice (Radio Buttons)' },
        { 'typeinfo': { 'type': 'text' }, 'displayText': 'Text' },
        { 'typeinfo': { 'type': 'object' }, 'displayText': 'Object' }
    ]

    @ViewChild(ModalComponent) modalRef: ModalComponent;

    constructor(private fb: FormBuilder, private http: Http, private aws: AwsService, private toastr: ToastsManager, private vcr: ViewContainerRef, private metric: LyMetricService) {
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
                for (let customField of this.customFields) {
                    this.deserializeCustomFieldDefaultValue(customField);
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
    * Deserialize the default value of the custom field
    * @param customField the customField to deserialize
    **/
    private deserializeCustomFieldDefaultValue(customField): void {
        if (customField['type'] === "predefined" && customField["multipleSelect"]) {
            customField["defaultValue"] = JSON.parse(customField["defaultValue"]);
        }
        else if (customField['type'] === "object") {
            for (let property of customField['properties']) {
                this.deserializeCustomFieldDefaultValue(property);
            }
            customField["defaultValue"] = JSON.parse(customField["defaultValue"]);
        }
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
            'title': [this.curEditingField["title"], Validators.compose([Validators.required, this.duplicate])]
        });
    }

    /**
    * Check if the title already exists
    **/
    private duplicate = (control: FormControl) =>{
        for (let field of this.curFields) {
            if (this.curEditingField["title"] !== field['title'] && field['title'] === control.value) {
                return { duplicate: true };
            }
        }
        return null;
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
            'maxChars': [this.curEditingField["maxChars"], Validators.compose([Validators.required, ValidationUtil.positiveNumberValidator])]
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
    * Check whether the required form field is duplicate
    * @param form the form to check
    * @param item the form field to check
    * @returns {boolean} whether the required form field is empty
    **/
    private isFormFieldTitleDuplicate(form: any, item: string): boolean {
        return (form.controls[item].hasError('duplicate') && form.controls[item].touched)
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
        if (this.curEditingField['type'] === 'object') {
            this.curEditingField['properties'] = [];
        }
        else if (this.curEditingField['properties']) {
            delete this.curEditingField['properties'];
        }

        delete this.curEditingField['defaultValue'];

        this.curFields[this.curFieldIndex] = this.curEditingField;

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
        if (this.curEditingField['type'] === 'object') {
            this.curEditingField['properties'] = [];
        }
        this.curFields.push(this.curEditingField);

        this.modalRef.close();
    }

    /**
    * Save the client configuration
    **/
    private updateClientConfiguration(): void{
        let customFieldsCopy = JSON.parse(JSON.stringify(this.customFields));
        for (let customField of customFieldsCopy) {
            this.serializeCustomFieldDefaultValue(customField);
        }

        let body = { "clientConfiguration": customFieldsCopy };

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
    * Serialize the default value of the custom field
    * @param customField the customField to serialize
    **/
    private serializeCustomFieldDefaultValue(customField): void {
        if (customField['type'] === "predefined" && customField["multipleSelect"]) {
            let selections = [];
            for (let i = 0; i < customField["defaultValue"].length; i++) {
                if (customField["defaultValue"][i]) {
                    selections.push(customField["predefines"][i])
                }
            }
            customField["defaultValue"] = JSON.stringify(selections);
        }
        else if (customField['type'] === "object") {
            for (let property of customField['properties']) {
                this.serializeCustomFieldDefaultValue(property);
                if (property['type'] === "predefined" && property["multipleSelect"]) {
                    customField["defaultValue"][property['title']] = JSON.parse(property["defaultValue"]);
                }
                else {
                    customField["defaultValue"][property['title']] = property["defaultValue"];
                }
            }
            customField["defaultValue"] = JSON.stringify(customField["defaultValue"]);
        }
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
                this.curEditingField["maxChars"] = +this.textFieldForm.value.maxChars;
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
            case "object":
                return 'Object';
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
        this.curFields.splice(this.curFieldIndex, 1);

        this.modalRef.close();
    }

    /**
    * Define all the modals
    **/
    private onModifyFieldModal(fields: Object[], field: Object, fieldIndex: number): void {
        this.curFields = fields;
        this.curEditingField = JSON.parse(JSON.stringify(field))
        this.createFieldForms();
        this.editMode = EditMode.EditField;
        this.curFieldIndex = fieldIndex;
    }

    private onDeleteFieldModal = (fields: Object[], field: Object, index: number) => {
        this.curFields = fields;
        this.curFieldIndex = index;
        this.curEditingField = field;
        this.editMode = EditMode.DeleteField;
    }

    private onDismissModal = () => {
        this.editMode = EditMode.List;
    }

    private onAddNewFieldModal = (fields: Object[]) => {
        this.curEditingField = this.createDefaultField();
        this.curFields = fields;
        this.createFieldForms();

        this.editMode = EditMode.CreateField;
    }

    private getModalName = () => {
        if (this.editMode == EditMode.EditField) {
            return 'Edit Field';
        }
        else if (this.editMode == EditMode.CreateField) {
            return 'Add New Field';
        }
        else if (this.editMode == EditMode.DeleteField) {
            return 'Delete Field';
        }
    }

    private getSubmitButtonText = () => {
        if (this.editMode == EditMode.EditField) {
            return 'Save Changes';
        }
        else if (this.editMode == EditMode.CreateField) {
            return 'Add Field';
        }
        else if (this.editMode == EditMode.DeleteField) {
            return 'Delete Field';
        }
    }

    private onModalSubmit = () => {
        if (this.editMode == EditMode.EditField) {
            this.onModifyField();
        }
        else if (this.editMode == EditMode.CreateField) {
            this.onAddField();
        }
        else if (this.editMode == EditMode.DeleteField) {
            this.onDeleteField();
        }
    }
}
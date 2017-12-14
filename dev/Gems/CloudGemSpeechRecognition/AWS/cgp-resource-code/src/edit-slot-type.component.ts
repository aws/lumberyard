import { Component, Input, Output, OnInit, EventEmitter, ViewChild, ViewContainerRef } from '@angular/core'
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { ModalComponent } from 'app/shared/component/index';
import { ToastsManager } from 'ng2-toastr';
import { LyMetricService } from 'app/shared/service/index';
import { BotEntry, IntentEntry, SlotTypeEntry, SpeechToTextApi } from './index';

@Component({
    selector: 'edit-slot-type',
    templateUrl: 'node_modules/@cloud-gems/cloudgemspeechrecognition/edit-slot-type.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemspeechrecognition/edit-slot-type.component.css']
})

export class SpeechToTextEditSlotTypeComponent {
    @Input() originalSlotType: SlotTypeEntry;
    @Input() context: any;
    @Input() oldVersionWarning: string;
    @ViewChild(ModalComponent) modalRef: ModalComponent;

    private _apiHandler: SpeechToTextApi;
    private isLoadingSlotType: boolean;
    private currentSlotType: SlotTypeEntry;
    private currentSlotTypeVersions: SlotTypeEntry[];

    private newSlotTypeValue: string;

    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager, private vcr: ViewContainerRef, private metric: LyMetricService) {
    }

    /**
     * Initialize values
	**/
    ngOnInit() {
        this._apiHandler = new SpeechToTextApi(this.context.ServiceUrl, this.http, this.aws, this.metric, this.context.identifier);
        this.getSlotType(this.originalSlotType);
    }

    /**
     * Update the slot type on display when switch to a new tab
     * @param changes any change of the component
	**/
    ngOnChanges(changes: any) {
        if (!changes.originalSlotType.previousValue || (changes.originalSlotType.previousValue.name == changes.originalSlotType.currentValue.name)) {
            return;
        }
        this.getSlotType(this.originalSlotType);
    }

    /**
     * Add a new slot type value
	**/
    public addNewSlotTypeValue(): void {
        this.currentSlotType["enumerationValues"].push({ value: this.newSlotTypeValue });
        this.newSlotTypeValue = "";
    }

    /**
     * Change the slot type value. Delete the entry if it is empty
     * @param $event the new content of the slot type value
     * @param index the index of the slot type value in the list
	**/
    public slotTypeValueChange($event, index): void {
        this.currentSlotType['enumerationValues'][index]['value'] = $event;
        if ($event == "") {
            this.currentSlotType['enumerationValues'].splice(index, 1);
        }
    }

    /**
     * Remove the selected slot type value
     * @param index the index of the slot type value in the list
	**/
    public removeSlotTypeValue(index): void {
        this.currentSlotType.enumerationValues.splice(index, 1);
    }

    /**
     * Save the current slot type
    **/
    public saveSlotType(): void {
        this.currentSlotType.get("$LATEST").then(function (slotType) {
            this.currentSlotType.checksum = slotType.checksum;
            this.currentSlotType.save();
        }.bind(this))
    }

    /**
     * Get the information of the selected slot type
     * @param slotType the slot type to get
    **/
    private getSlotType(slotType): void {
        this.isLoadingSlotType = true;
        slotType.get(slotType.version).then(function (response) {
            this.currentSlotType = response;
            this.getSlotTypeVersions(slotType);
            this.isLoadingSlotType = false;
        }.bind(this), function () {
            this.isLoadingSlotType = false;
        }.bind(this));
    }

    /**
     * Get the all the versions of the selected slot type
     * @param slotType the slot type to get all the versions
    **/
    private getSlotTypeVersions(slotType: SlotTypeEntry): void {
        this.isLoadingSlotType = true;
        this.currentSlotType.getVersions("t").then(function () {
            this.isLoadingSlotType = false;
        }.bind(this), function () {
            this.isLoadingSlotType = false;
        }.bind(this))
    }
}
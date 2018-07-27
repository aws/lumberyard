import { Component, Output, EventEmitter, Input } from '@angular/core';
import { NgbDateStruct } from '@ng-bootstrap/ng-bootstrap';
import { DateTimeUtil } from 'app/shared/class/datetime.class';

@Component({
    selector: 'datetime-range-picker',
    templateUrl: './app/shared/component/datetime-range-picker.component.html',
    styleUrls: ['./app/shared/component/datetime-range-picker.component.css']
})
export class DateTimeRangePickerComponent {
    @Output() dateTimeRange: any = new EventEmitter();
    @Input() hasStart: boolean;
    @Input() hasEnd: boolean;
    @Input() startDateModel: NgbDateStruct;
    @Input() endDateModel: NgbDateStruct;
    @Input() startTime: string;    
    @Input() endTime: string;    
    @Input() isStartRequired?: boolean = false;
    @Input() isEndRequired?: boolean = false;

    private model: any;

    constructor() {
        this.model = {
            isNoEnd: true,
            isNoStart: true,
            hasRequiredStartDateTime: false,
            hasRequiredEndDateTime: false
        }
    }

    ngOnChanges(changes: any) {
        this.init();
    }


    protected change() {

        let result = {
            start: {
                date: this.model.dpStart,
                time: this.model.stime                
            },
            end: {
                date: this.model.dpEnd,
                time: this.model.etime                
            },
            hasStart: !this.model.isNoStart,
            hasEnd: !this.model.isNoEnd,
            hasRequiredStartDateTime: this.model.isStartRequired,
            hasRequiredEndDateTime: this.model.isEndRequired,

        }
        this.validateDate(result);
        this.dateTimeRange.emit(result);
    }

    ngOnInit() {
        this.init();
    }

    private init() {
        if (this.hasStart == null)
            this.model.hasStart = false;

        if (this.hasEnd == null)
            this.model.hasEnd = false;

        if (this.startDateModel) {
            this.model.dpStart = this.startDateModel
        }

        if (this.endDateModel) {
            this.model.dpEnd = this.endDateModel;
        }

        if (this.startTime)
            this.model.stime = this.startTime;

        if (this.endTime)
            this.model.etime = this.endTime;

        if (this.isStartRequired == null) {
            this.model.isStartRequired = false;
        } else {
            this.model.isStartRequired = this.isStartRequired;
        }

        if (this.isEndRequired == null) {
            this.model.isEndRequired = false;
        } else {
            this.model.isEndRequired = this.isEndRequired;
        }

        if (this.model.isStartRequired) {
            this.model.isNoStart = false;
        } else {
            this.model.isNoStart = !this.hasStart;
        }

        if (this.model.isEndRequired) {
            this.model.isNoEnd = false;
        } else {
            this.model.isNoEnd = !this.hasEnd;
        }

        this.change();
    }

    public static default(initializingdate: Date): {
        date: {
            year?: number,
            month?: number,
            day?: number
        },
        time?: {
            hour: number,
            minute: number
        },        
        valid: boolean
    }{
        var today = new Date();

        return {
            date: {
                year: DateTimeUtil.isValid(initializingdate) ? initializingdate.getFullYear() : today.getFullYear(),
                month: DateTimeUtil.isValid(initializingdate) ? initializingdate.getMonth() + 1 : today.getMonth() + 1,
                day: DateTimeUtil.isValid(initializingdate) ? initializingdate.getDate() : today.getDate(),
            },
            time: DateTimeUtil.isValid(initializingdate) ? { hour: initializingdate.getHours(), minute: initializingdate.getMinutes() } :
            { hour: 12, minute: 0 },
            valid: true
        }
    }

    private validateDate(model): void {
        let isValid: boolean = true;

        let start = DateTimeUtil.toObjDate(model.start);
        let end = DateTimeUtil.toObjDate(model.end);
        model.start.valid = model.end.valid = true
        model.date = {}
        if (this.hasEnd) {
            model.end.valid = end ? true : false
            if (!model.end.valid)
                model.date = { message: "The end date is not a valid date.  A date must have a date, hour and minute." }

            isValid = isValid && model.end.valid;
        }
        if (this.hasStart) {
            model.start.valid = start ? true : false
            if (!model.start.valid)
                model.date = { message: "The start date is not a valid date.  A date must have a date, hour and minute." }
            isValid = isValid && model.start.valid;
        }

        if (this.hasEnd && this.hasStart) {
            let isValidDateRange = (start < end);
            isValid = isValid && isValidDateRange
            if (!isValidDateRange) {
                model.date = { message: "The start date must be less than the end date." }
                model.start.valid = false;
            }
        }
        model.valid = isValid;
    }


}

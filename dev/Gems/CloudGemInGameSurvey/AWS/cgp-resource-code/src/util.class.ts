import { DateTimeUtil } from 'app/shared/class/index';

export class TimeUtil {
    static epochToString(epoch: number): string {        
        return DateTimeUtil.toString(DateTimeUtil.fromEpoch(epoch), "MMM dd yyyy HH:mm");
    }
}

export class ValidationUtil {   
    static isFormFieldRequiredEmpty(form: any, item: string): boolean {
        if (!form)
            return true
        return (form.controls[item].hasError('required') && form.controls[item].touched)
    }

    static isFormFieldNotValid(form: any, item: string): boolean {
        if (!form)
            return true
        return !form.controls[item].valid && form.controls[item].touched
    }
}
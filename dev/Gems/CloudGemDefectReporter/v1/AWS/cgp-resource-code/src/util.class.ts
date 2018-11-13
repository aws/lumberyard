import { DateTimeUtil } from 'app/shared/class/index';
import { FormControl } from '@angular/forms';

export class RandomUtil {
// Utilities that use Math.random to generate random values

    static getRandomInt(min, max): number {
    // Quick random number generator (RNG) to test list

        min = Math.ceil(min);
        max = Math.floor(max);
        return Math.floor(Math.random() * (max - min)) + min;
    }
}

export class ValidationUtil {
    static isFormFieldNotPositiveNum(form: any, item: string): boolean {
        if (!form)
            return true
        return (form.controls[item].hasError('positiveNum') && form.controls[item].touched)
    }

    static positiveNumberValidator(control: FormControl) {
        let num = Number(control.value);
        if (!isNaN(num) && num > 0) {
            return null;
        }

        return { 'positiveNum': true };
    }
}
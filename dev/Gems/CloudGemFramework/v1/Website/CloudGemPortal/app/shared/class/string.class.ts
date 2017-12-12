import { DatePipe } from '@angular/common';

export class StringUtil {
    public static toEtcetera(input: string, maxlength: number, suffix: string = "..."): string {
        let output = input;
        if (output && output.length > maxlength) {
            output = output.substr(0, maxlength) + suffix
        }
        return output;
    }
}
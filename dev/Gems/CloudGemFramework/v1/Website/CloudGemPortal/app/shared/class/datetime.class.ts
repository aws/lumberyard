import { DatePipe } from '@angular/common';

// Moment.js for dates.  Angular 2 datepipe is not supported by IE or Safari
declare var moment: any;
declare var System: any;

export class DateTimeUtil {
    public static toObjDate(obj: any) {
        let time = null

        if (this.isNotNullOrUndefined(obj.time)
            && this.isNotNullOrUndefined(obj.time.hour)
            && this.isNotNullOrUndefined(obj.time.minute)
            && this.isNotNullOrUndefined(obj.date)
            && this.isNotNullOrUndefined(obj.date.month)
            && this.isNotNullOrUndefined(obj.date.year)
            && this.isNotNullOrUndefined(obj.date.day)
        ){
            time = {
                hour: obj.time.hour,
                minute: obj.time.minute
            }
        }else{
            return undefined            
        }        
        
        return new Date(obj.date.year, obj.date.month - 1, obj.date.day, time.hour, time.minute, null);
    }

    // toDate: use moment js to convert a date object to a date string for serialization.
    public static toDate(obj: any, format: string = 'MMM DD YYYY HH:mm'): string {       
        let newDate = moment(DateTimeUtil.toObjDate(obj)).format(format);
        return newDate;
    }   

    public static toString(date: Date, format: string): string {
        return new DatePipe("en-US").transform(date, format);
    }

    public static isValid(date: Date): boolean {
        return !(date == null || isNaN(date.valueOf()));
    }

    public static getUTCDate(): Date {
        let date: Date = new Date();
        return new Date(date.getUTCFullYear(), date.getUTCMonth(), date.getUTCDate(), date.getUTCHours(), date.getUTCMinutes(), date.getUTCSeconds());
    }
    
    public static fromEpoch(timestamp: number) {     
        var d = new Date(timestamp * 1000);        
        return d;    
    }

    public static isNotNullOrUndefined(obj: any): boolean {
        return !(obj === undefined || obj === null)
    }
}
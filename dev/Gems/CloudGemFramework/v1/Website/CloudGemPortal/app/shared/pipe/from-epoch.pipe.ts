import { Injectable, Pipe, PipeTransform } from '@angular/core';
import { DateTimeUtil } from "../class/index"

@Pipe({ name: 'toDateFromEpochInMilli' })
export class FromEpochPipe implements PipeTransform {
    transform(value: string, args: string[]): any {
        return DateTimeUtil.fromEpoch(Number.parseInt(value)/1000);
    }
}
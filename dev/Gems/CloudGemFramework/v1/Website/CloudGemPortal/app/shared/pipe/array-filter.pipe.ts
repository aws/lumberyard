import { Injectable, Pipe, PipeTransform } from '@angular/core';

@Pipe({ name: 'arrayFilter' })
export class FilterArrayPipe implements PipeTransform {
    transform(value: any[], args: string[]): any[] {
        return value.filter(e=>e!==args[0])
    }
}
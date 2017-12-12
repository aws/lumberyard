import { Injectable, Pipe, PipeTransform } from '@angular/core';

@Pipe({ name: 'objKeys' })
export class ObjectKeysPipe implements PipeTransform {
    transform(value: any, args: any[] = null): any {
        return Object.keys(value)
    }
}
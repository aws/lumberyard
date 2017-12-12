import { Injectable, Pipe, PipeTransform } from '@angular/core';
import { DefinitionService } from "../service/index"

@Pipe({ name: 'devonly' })
export class ModelDebugPipe implements PipeTransform {
    constructor(private definition: DefinitionService) {

    }

    transform(value: string, args: string[]): any {
        if (this.definition.isProd)
            return;

        if (this.definition.defines.debugModel == true)
            return value;

        return;
    }
}
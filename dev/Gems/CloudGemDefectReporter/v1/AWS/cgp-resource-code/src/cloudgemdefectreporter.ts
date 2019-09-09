import { NgModule } from '@angular/core';
import { CloudGemDefectReporterModule } from './index';

/*
*  Entry point for the cloud gem factory
*/
export function definition(context: any): any {
    return CloudGemDefectReporterModule;
}

import { CloudGemDefectReporterApi } from './index'
/*
*  Expose the API Handler to the directory service
*/
export function serviceApiType(): Function {
    return CloudGemDefectReporterApi
}

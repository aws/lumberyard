import { CloudGemMetricModule } from './index'
import { NgModule } from '@angular/core';
/*
*  Entry point for the cloud gem factory
*/
export function definition(context: any): any {
    return CloudGemMetricModule;
}

import { CloudGemMetricApi } from './index'
/*
*  Expose the API Handler to the directory service
*/
export function serviceApiType(): Function {
    return CloudGemMetricApi
}
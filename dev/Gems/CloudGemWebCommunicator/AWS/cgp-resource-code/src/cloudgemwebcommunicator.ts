import { CloudGemWebCommunicatorModule } from './index'
import { NgModule } from '@angular/core';

/*
*  Entry point for the cloud gem factory
*/
export function definition() {
    return CloudGemWebCommunicatorModule;
}
/// <reference path="../typings/index.d.ts" />

import { platformBrowserDynamic } from '@angular/platform-browser-dynamic';
import { AppModule } from './app.module';
import { enableProdMode } from '@angular/core';

//enableProdMode();

const platform = platformBrowserDynamic();
platform.bootstrapModule(AppModule);

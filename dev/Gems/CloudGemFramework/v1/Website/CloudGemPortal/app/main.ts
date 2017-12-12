import 'reflect-metadata';
import 'zone.js';

import { platformBrowserDynamic } from '@angular/platform-browser-dynamic';
import { AppModule } from './app.module';
import { enableProdMode } from '@angular/core';
import * as environment from 'app/shared/class/environment.class';

if (environment.isProd) {
    enableProdMode();
}

const platform = platformBrowserDynamic();
platform.bootstrapModule(AppModule);

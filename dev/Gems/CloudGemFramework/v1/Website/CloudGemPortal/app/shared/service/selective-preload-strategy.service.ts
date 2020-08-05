import 'rxjs/add/observable/of';
import { Observable } from 'rxjs/Observable';
import { Injectable } from '@angular/core';
import { PreloadingStrategy, Route } from '@angular/router';
import { DefinitionService } from './definition.service'

@Injectable()
export class PreloadingService implements PreloadingStrategy {
    preloadedModules: string[] = [];

    constructor(private definitionService: DefinitionService) {

    }

    preload(route: Route, load: Function): Observable<any> {
        if (route.data && route.data['preload']) {
            // add the route path to our preloaded module array
            this.preloadedModules.push(route.path);

            // log the route path to the console
            if (!this.definitionService.isProd)
                console.log('Preloaded: ' + route.path);

            return load();
        } else {
            return Observable.of(null);
        }
    }
}







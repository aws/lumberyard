import { Injectable } from '@angular/core';
import { Observable } from 'rxjs/Observable'
import { BehaviorSubject } from 'rxjs/BehaviorSubject';
import { AwsService } from 'app/aws/aws.service';
import { ReadSwaggerAction } from './action/read-swagger.class';

//const HEROES = [
//    new Hero('Windstorm', 'Weather mastery'),
//    new Hero('Mr. Nice', 'Killing them with kindness'),
//    new Hero('Magneta', 'Manipulates metalic objects')
//];

@Injectable()
export class CloudGemDirectoryService {
    constructor() { }

    //getAll(type: Type): PromiseLike<any[]> {
    //    if (type === Hero) {
    //        // TODO get from the database
    //        return Promise.resolve<Hero[]>(HEROES);
    //    }
    //    let err = new Error('Cannot get object of this type');
    //    this.logger.error(err);
    //    throw err;
    //}
}
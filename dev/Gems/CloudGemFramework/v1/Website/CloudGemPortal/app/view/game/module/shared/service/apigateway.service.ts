import { Injectable } from '@angular/core';
import { Observable } from 'rxjs/Observable'
import { BehaviorSubject } from 'rxjs/BehaviorSubject';
import { AwsService } from 'app/aws/aws.service';
import { ReadSwaggerAction } from './action/read-swagger.class';

export enum EnumApiGatewayState {
    INITIALIZED,
    READ_SWAGGER_SUCCESS,
    READ_SWAGGER_FAILURE
}

export enum EnumApiGatewayAction {
    READ_SWAGGER
}

export interface ActionContext {
    state: EnumApiGatewayState,
    output?: any[]
}

export interface ApiGatewayAction {
    handle(subject: BehaviorSubject<ActionContext>, ...args: any[]): void
}

@Injectable()
export class ApiGatewayService {

    private _actions = [];
    private _observable: BehaviorSubject<any>;
    
    get onChange(): Observable<any> {
        return this._observable.asObservable();
    }   

    constructor(aws: AwsService) {
        this._observable = new BehaviorSubject<ActionContext>(<ActionContext>{});

        this._actions[EnumApiGatewayAction.READ_SWAGGER] = new ReadSwaggerAction(aws.context)

        this.onChange.subscribe(context => {
            console.log(EnumApiGatewayState[context.state] + ":" + context.output[0] )
        })

    }   

    public change(state: any) {
        this._observable.next(state);
    }

    public getSwaggerAPI(apiid: string): void {
        this.execute(EnumApiGatewayAction.READ_SWAGGER, apiid);
    }

    private execute(transition: EnumApiGatewayAction, ...args: any[]): void {
        this._actions[transition].handle(this._observable, ...args);
    }

}
import { Observable } from 'rxjs/Observable'
import { BehaviorSubject } from 'rxjs/BehaviorSubject';
import { Gemifiable, Tackable, TackableStatus } from './gem-interfaces';
import { ApiHandler, Restifiable } from 'app/shared/class/index'

//TODO: Seperate tackable class from Gemifiable class???
export abstract class DynamicGem implements Gemifiable {
    public context: any = {};
    public abstract identifier: string;
    public abstract tackable: Tackable;
    public abstract classType(): any;
    protected htmlTemplateUrl: string;
    protected styleTemplateUrl: string;
    protected rawTemplate: string
    protected rawStyle: string
    protected isActiveLink: string = "Overview";    

    //the services are provided when then factory generates the componenet.
    //this is done when you click a gem thumbnail
    //adding parameters to this contructor will force angular to resolve the parameter as a service provider when the factory generates the component.
    constructor() {        
    }

    public initialize(context: any) {
        this.context = context;        
    }

    protected getSignedUrl(path: string): string {
        return this.context.provider.aws.context.s3.getSignedUrl('getObject', { Bucket: this.context.meta.bucketId, Key: path })
    }

    public get hasStyle(): boolean {
        return this.styleTemplateUrl || this.rawStyle ? true : false
    }
   
    public get template(): Observable<string> {
        let bs = new BehaviorSubject<string>(this.rawTemplate);

        if (!this.rawTemplate)
            this.context.provider.http.get(this.htmlTemplateUrl).subscribe(resp => {
                bs.next(resp.text())
            })

        return bs.asObservable();
    }

    public get styles(): Observable<string> {
        let bs = new BehaviorSubject<string>(this.rawStyle);

        if (this.hasStyle && !this.rawStyle)
            this.context.provider.http.get(this.styleTemplateUrl).subscribe(resp => {
                bs.next(resp.text())
            })

        return bs.asObservable();
    }

    protected route(model, link): void {
        this.isActiveLink = link;
    }
}

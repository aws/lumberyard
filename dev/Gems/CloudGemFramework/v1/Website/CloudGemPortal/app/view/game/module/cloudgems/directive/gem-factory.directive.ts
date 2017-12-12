import 'reflect-metadata';
import {
    Component,
    Directive,
    Input,
    ViewContainerRef,
    Compiler,
    ModuleWithComponentFactories,
    ComponentRef,
    ReflectiveInjector,
    ComponentFactory,
    Type,
    OnInit
} from '@angular/core';
import { Gemifiable, Tackable } from '../class/index';
import { UrlService, GemService } from "app/shared/service/index";

export function createComponentFactory(compiler: Compiler, cloudgem: Gemifiable, thumbnailonly: boolean): Promise<ComponentFactory<any>> {
    let component = thumbnailonly ? cloudgem.thumbnail : cloudgem.index;

    return compiler.compileModuleAndAllComponentsAsync(cloudgem.module)
        .then((moduleWithComponentFactory: ModuleWithComponentFactories<any>) => {
            return moduleWithComponentFactory.componentFactories.find(x => x.componentType === component);
        });
}

@Directive({ selector: 'gem-factory' })
export class GemFactory {
    @Input() cloudGem: Gemifiable;
    @Input() thumbnailOnly: boolean = false;
    @Input() refreshContent: boolean;
    private cmpRef: ComponentRef<any>;
    private _loading: boolean = false;
    public static DynamicGemFactories: Map<string, ComponentFactory<any>> = new Map<string, ComponentFactory<any>>();

    constructor(private vcRef: ViewContainerRef,
        private compiler: Compiler,
        private urlService: UrlService,
        private gemService: GemService
    ) {

    }

    ngOnInit() {
        if (!this.cloudGem || this._loading) return;

        if (this.cmpRef) {
            this.cmpRef.destroy();
        }
        this._loading = true;
        let component = Reflect.getMetadata('annotations', this.thumbnailOnly ? this.cloudGem.thumbnail : this.cloudGem.index)[0];
        this.urlService.signUrls(component, this.cloudGem.context.bucketId, 60)

        this.createComponent(this.cloudGem, this.thumbnailOnly);
    }

    ngOnChanges() {
        this.ngOnInit();
    }

    ngOnDestroy() {
        if (this.cmpRef) {
            this.cmpRef.destroy();
        }
    }

    createComponent(cloudgem: Gemifiable, thumbnailonly: boolean) {
        createComponentFactory(this.compiler, cloudgem, thumbnailonly)
            .then(factory => {
                this.addToView(cloudgem, factory, thumbnailonly);
               // GemFactory.DynamicGemFactories[cloudgem.identifier] = factory;
                this._loading = false;
            });
    }

    addToView(gem: Gemifiable, factory: ComponentFactory<any>, thumbnailonly: boolean) {
        const injector = ReflectiveInjector.fromResolvedProviders([], this.vcRef.parentInjector);
        //create the component in the vcRef DOM
        this.cmpRef = this.vcRef.createComponent(factory, 0, injector, []);
        (<any>this.cmpRef.instance).context = gem.context;

        if (!thumbnailonly)
            return;

        gem.displayName = (<Tackable>this.cmpRef.instance).displayName
        gem.srcIcon = (<Tackable>this.cmpRef.instance).srcIcon;

        // store in the gem service for keeping track of the current gems
        this.gemService.addGem(gem);
    }
}

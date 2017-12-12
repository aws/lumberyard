import {
    Component,
    Directive,
    NgModule,
    Input,
    ViewContainerRef,
    ContentChild,
    Compiler,        
    ModuleWithComponentFactories,
    ComponentRef,
    ReflectiveInjector,
    ResolvedReflectiveProvider,
    TemplateRef,
    EmbeddedViewRef,
    ComponentFactory,
    Type,
    ViewEncapsulation,
    OnInit
} from '@angular/core';
import { CompileMetadataResolver } from '@angular/compiler'
import { Observable } from 'rxjs/Observable'
import { RouterModule } from '@angular/router';
import { GameSharedModule } from 'app/view/game/module/shared/shared.module';
import { Gemifiable } from '../class/gem-interfaces';

export function createComponentFactory(compiler: Compiler, gem: Gemifiable, template: string, style?: string): Promise<ComponentFactory<any>> {
        
    let componentDefinition = new Component({
        selector: 'dcg-' + gem.identifier,
        template: template        
    });
    
    if (style) {
        componentDefinition.styles = [style];
        componentDefinition.encapsulation = ViewEncapsulation.None;
    }
    
    const decoratedCmp = Component(componentDefinition)(gem.classType()); 

    //No factory exists....create one
    @NgModule({
        imports: [GameSharedModule],        
        declarations: [decoratedCmp]        
    })
    class DynamicCloudGemModule { }    
    
    return compiler.compileModuleAndAllComponentsAsync(DynamicCloudGemModule)
        .then((moduleWithComponentFactory: ModuleWithComponentFactories<any>) => {
            return moduleWithComponentFactory.componentFactories.find(x => x.componentType === decoratedCmp);
        });
}

@Directive({ selector: 'gem-factory' })
export class GemFactory {
    @Input() gem: Gemifiable;
    @Input() refreshContent: boolean;
    private cmpRef: ComponentRef<any>;
    private _loading: boolean = false;
    public static DynamicGemFactories: Map<string, ComponentFactory<any>> = new Map<string, ComponentFactory<any>>();
   
    constructor(private vcRef: ViewContainerRef,
        private compiler: Compiler
    ) {     
        
    }

    ngOnInit() {
     
    }

    ngOnChanges() {        
        const gem = this.gem;
        if (!gem || this._loading) return;
        
        if (this.cmpRef) {
            this.cmpRef.destroy();
        }
        
        let dynamicGemFactory = GemFactory.DynamicGemFactories[gem.identifier];
        if (dynamicGemFactory) {
            this.addToView(gem, dynamicGemFactory);         
        } else {
            //Wait for both the style url and template url to load before creating the component.
            //can't use the component system to do this because it escapes the presigned urls which causes a problem
            this._loading = true;
            gem.template.subscribe(template => {
                if (template) {
                    if (gem.hasStyle) {
                        gem.styles.subscribe(style => {
                            if(style)
                                this.createComponent(gem, template, style);
                        });
                    } else {                        
                        this.createComponent(gem, template);
                    }
                }
            });            
        }
    }

    ngOnDestroy() {
        if (this.cmpRef) {
            this.cmpRef.destroy();
        }
    }

    createComponent(gem: Gemifiable, template: string, style?: string) {
        createComponentFactory(this.compiler, gem, template, style)
            .then(factory => {
                this.addToView(gem, factory);
                GemFactory.DynamicGemFactories[gem.identifier] = factory;
                this._loading = false;
            });
    }

    addToView(gem, factory) {
        const injector = ReflectiveInjector.fromResolvedProviders([], this.vcRef.parentInjector);
        this.cmpRef = this.vcRef.createComponent(factory, 0, injector, [[{ model: { id: "factory" } }]]);
        this.cmpRef.instance.initialize(gem.context);
    }
}

import { NgModule } from '@angular/core';
import { PageNotFoundComponent } from 'app/view/error/component/page-not-found.component'

@NgModule({
    imports: [],
    exports: [
        PageNotFoundComponent   
    ],
    declarations: [PageNotFoundComponent]
})
export class ErrorModule { }

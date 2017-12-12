import { NgModule } from '@angular/core';
import { RouterModule } from '@angular/router';
import { PageNotFoundComponent } from 'app/view/error/component/page-not-found.component'

export const GAME_ROUTES = [
    {
        path: '**',
        redirectTo: 'login',
        pathMatch: 'full'
    }  
];

@NgModule({
    imports: [
        RouterModule.forRoot(GAME_ROUTES) 
    ],
    exports: [
        RouterModule
    ]   
})
export class AppRoutingModule { }
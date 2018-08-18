import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { APP_BASE_HREF } from '@angular/common';
import { LOCALE_ID } from '@angular/core';
import { AppRoutingModule } from 'app/app.routes';
import { AppComponent } from 'app/app.component';
import { ErrorModule } from 'app/view/error/error.module';
import { AuthModule } from 'app/view/authentication/auth.module';
import { GameModule } from 'app/view/game/game.module';
import { AppSharedModule } from 'app/shared/shared.module';
import { BrowserAnimationsModule } from '@angular/platform-browser/animations';
import { ToastModule, ToastOptions} from 'ng2-toastr/ng2-toastr';

export class CustomToastrOptions extends ToastOptions {
    animate = "flyRight";
    showCloseButton = true;
    maxShown = 3;
    positionClass = 'toast-bottom-right';
    messageClass = 'cgp-toastr-message';
    titleClass = 'cgp-toastr-title';

}

@NgModule({
    imports: [
        BrowserModule,
        BrowserAnimationsModule,
        NgbModule.forRoot(),
        AppSharedModule.forRoot(),
        ToastModule.forRoot(),
        ErrorModule,
        AuthModule,
        GameModule,
        AppRoutingModule,  //This must be defined last as the order of the route defintions matters and the app defines '**' which would capute all routes
        ],
    declarations: [
        AppComponent
    ],
    providers: [
        { provide: APP_BASE_HREF, useValue: '/' },
        { provide: LOCALE_ID, useValue: "en-US" },
        { provide: ToastOptions, useClass: CustomToastrOptions }
    ],
    bootstrap: [AppComponent]
})

export class AppModule { }
!function(a){function b(a,b,c){a in i||(i[a]={name:a,declarative:!0,deps:b,declare:c,normalizedDeps:b})}function c(a){return m[a]||(m[a]={name:a,dependencies:[],exports:{},importers:[]})}function d(b){if(!b.module){var e=b.module=c(b.name),f=b.module.exports,g=b.declare.call(a,function(a,b){if(e.locked=!0,"object"==typeof a)for(var c in a)f[c]=a[c];else f[a]=b;for(var d=0,g=e.importers.length;g>d;d++){var h=e.importers[d];if(!h.locked)for(var i=0;i<h.dependencies.length;++i)h.dependencies[i]===e&&h.setters[i](f)}return e.locked=!1,b},b.name);e.setters=g.setters,e.execute=g.execute;for(var j=0,k=b.normalizedDeps.length;k>j;j++){var l,n=b.normalizedDeps[j],o=i[n],p=m[n];p?l=p.exports:o&&!o.declarative?l=o.esModule:o?(d(o),p=o.module,l=p.exports):l=h(n),p&&p.importers?(p.importers.push(e),e.dependencies.push(p)):e.dependencies.push(null),e.setters[j]&&e.setters[j](l)}}}function e(b){var c={};if(("object"==typeof b||"function"==typeof b)&&b!==a)if(k)for(var d in b)"default"!==d&&f(c,b,d);else{var e=b&&b.hasOwnProperty;for(var d in b)"default"===d||e&&!b.hasOwnProperty(d)||(c[d]=b[d])}return c.default=b,l(c,"__useDefault",{value:!0}),c}function f(a,b,c){try{var d;(d=Object.getOwnPropertyDescriptor(b,c))&&l(a,c,d)}catch(d){return a[c]=b[c],!1}}function g(b,c){var d=i[b];if(d&&!d.evaluated&&d.declarative){c.push(b);for(var e=0,f=d.normalizedDeps.length;f>e;e++){var k=d.normalizedDeps[e];-1==j.call(c,k)&&(i[k]?g(k,c):h(k))}d.evaluated||(d.evaluated=!0,d.module.execute.call(a))}}function h(a){if(o[a])return o[a];if("@node/"==a.substr(0,6))return o[a]=e(n(a.substr(6)));var b=i[a];if(!b)throw"Module "+a+" not present.";return d(i[a]),g(a,[]),i[a]=void 0,b.declarative&&l(b.module.exports,"__esModule",{value:!0}),o[a]=b.declarative?b.module.exports:b.esModule}var i={},j=Array.prototype.indexOf||function(a){for(var b=0,c=this.length;c>b;b++)if(this[b]===a)return b;return-1},k=!0;try{Object.getOwnPropertyDescriptor({a:0},"a")}catch(a){k=!1}var l;!function(){try{Object.defineProperty({},"a",{})&&(l=Object.defineProperty)}catch(a){l=function(a,b,c){try{a[b]=c.value||c.get.call(a)}catch(a){}}}}();var m={},n="undefined"!=typeof System&&System._nodeRequire||"undefined"!=typeof require&&require.resolve&&"undefined"!=typeof process&&require,o={"@empty":{}};return function(a,c,d,f){return function(g){g(function(g){for(var i=0;i<c.length;i++)!function(a,b){b&&b.__esModule?o[a]=b:o[a]=e(b)}(c[i],arguments[i]);f({register:b});var j=h(a[0]);if(a.length>1)for(var i=1;i<a.length;i++)h(a[i]);return d?j.default:j})}}}("undefined"!=typeof self?self:global)(["2b"],["3","8","6","7","9","a","d","31","c","10","11"],!1,function(a){this.require,this.exports,this.module;a.register("2c",["3"],function(a,b){"use strict";var c,d,e=this&&this.__extends||function(){var a=Object.setPrototypeOf||{__proto__:[]}instanceof Array&&function(a,b){a.__proto__=b}||function(a,b){for(var c in b)b.hasOwnProperty(c)&&(a[c]=b[c])};return function(b,c){function d(){this.constructor=b}a(b,c),b.prototype=null===c?Object.create(c):(d.prototype=c.prototype,new d)}}();b&&b.id;return{setters:[function(a){c=a}],execute:function(){d=function(a){function b(b,c,d,e,f){return void 0===e&&(e=null),void 0===f&&(f=void 0),a.call(this,b,c,d,e,f)||this}return e(b,a),b.prototype.delete=function(b){return a.prototype.delete.call(this,"admin/messages/"+b)},b.prototype.put=function(b,c){return a.prototype.put.call(this,"admin/messages/"+b,c)},b.prototype.post=function(b){return a.prototype.post.call(this,"admin/messages/",b)},b.prototype.getMessages=function(b,c,d){return a.prototype.get.call(this,"admin/messages?count="+d+"&filter="+b+"&index="+c)},b}(c.ApiHandler),a("MessageOfTheDayApi",d)}}}),a.register("2d",["8","6","2e","7","9","a"],function(a,b){"use strict";var c,d,e,f,g,h,i,j=this&&this.__extends||function(){var a=Object.setPrototypeOf||{__proto__:[]}instanceof Array&&function(a,b){a.__proto__=b}||function(a,b){for(var c in b)b.hasOwnProperty(c)&&(a[c]=b[c])};return function(b,c){function d(){this.constructor=b}a(b,c),b.prototype=null===c?Object.create(c):(d.prototype=c.prototype,new d)}}(),k=this&&this.__decorate||function(a,b,c,d){var e,f=arguments.length,g=f<3?b:null===d?d=Object.getOwnPropertyDescriptor(b,c):d;if("object"==typeof Reflect&&"function"==typeof Reflect.decorate)g=Reflect.decorate(a,b,c,d);else for(var h=a.length-1;h>=0;h--)(e=a[h])&&(g=(f<3?e(g):f>3?e(b,c,g):e(b,c))||g);return f>3&&g&&Object.defineProperty(b,c,g),g},l=this&&this.__metadata||function(a,b){if("object"==typeof Reflect&&"function"==typeof Reflect.metadata)return Reflect.metadata(a,b)};b&&b.id;return{setters:[function(a){c=a},function(a){d=a},function(a){e=a},function(a){f=a},function(a){g=a},function(a){h=a}],execute:function(){i=function(a){function b(b,d,e){var f=a.call(this)||this;return f.http=b,f.aws=d,f.metricservice=e,f.displayName="Message of the Day",f.srcIcon="https://m.media-amazon.com/images/G/01/cloudcanvas/images/message_of_the_day_optimized._V518452894_.png",f.state=new c.TackableStatus,f.metric=new c.TackableMeasure,f}return j(b,a),b.prototype.ngOnInit=function(){this._apiHandler=new e.MessageOfTheDayApi(this.context.ServiceUrl,this.http,this.aws,this.metricservice,this.context.identifier),this.report(this.metric),this.assign(this.state)},b.prototype.report=function(a){a.name="Total Active Messages",a.value="Loading...",this._apiHandler.get("admin/messages?count=500&filter=active&index=0").subscribe(function(b){var c=JSON.parse(b.body.text());a.value=c.result.list.length})},b.prototype.assign=function(a){a.label="Loading",a.styleType="Loading",this._apiHandler.get("service/status").subscribe(function(b){var c=JSON.parse(b.body.text());a.label="online"==c.result.status?"Online":"Offline",a.styleType="online"==c.result.status?"Enabled":"Offline"},function(b){a.label="Offline",a.styleType="Offline"})},k([d.Input(),l("design:type",Object)],b.prototype,"context",void 0),k([d.Input(),l("design:type",String)],b.prototype,"displayName",void 0),k([d.Input(),l("design:type",String)],b.prototype,"srcIcon",void 0),b=k([d.Component({selector:"cloudgemmessageoftheday-thumbnail",template:'\n    <thumbnail-gem \n        [title]="displayName" \n        [cost]="\'Low\'" \n        [srcIcon]="srcIcon" \n        [metric]="metric" \n        [state]="state" \n        >\n    </thumbnail-gem>'}),l("design:paramtypes",[f.Http,g.AwsService,h.LyMetricService])],b)}(c.AbstractCloudGemThumbnailComponent),a("MessageOfTheDayThumbnailComponent",i)}}}),a.register("2f",[],function(a,b){"use strict";var c;b&&b.id;return{setters:[],execute:function(){c=function(){function a(a){this.UniqueMsgID=a.UniqueMsgID,this.start=a.start,this.end=a.end,this.message=a.message,this.priority=a.priority,this.hasStart=a.hasStart,this.hasEnd=a.hasEnd}return a.prototype.clearStartDate=function(){this.start={date:{}}},a}(),a("MotdForm",c)}}}),a.register("30",["6","3","d","2e","7","9","31","8","c","a"],function(a,b){"use strict";var c,d,e,f,g,h,i,j,k,l,m,n,o=this&&this.__extends||function(){var a=Object.setPrototypeOf||{__proto__:[]}instanceof Array&&function(a,b){a.__proto__=b}||function(a,b){for(var c in b)b.hasOwnProperty(c)&&(a[c]=b[c])};return function(b,c){function d(){this.constructor=b}a(b,c),b.prototype=null===c?Object.create(c):(d.prototype=c.prototype,new d)}}(),p=this&&this.__decorate||function(a,b,c,d){var e,f=arguments.length,g=f<3?b:null===d?d=Object.getOwnPropertyDescriptor(b,c):d;if("object"==typeof Reflect&&"function"==typeof Reflect.decorate)g=Reflect.decorate(a,b,c,d);else for(var h=a.length-1;h>=0;h--)(e=a[h])&&(g=(f<3?e(g):f>3?e(b,c,g):e(b,c))||g);return f>3&&g&&Object.defineProperty(b,c,g),g},q=this&&this.__metadata||function(a,b){if("object"==typeof Reflect&&"function"==typeof Reflect.metadata)return Reflect.metadata(a,b)};b&&b.id;return{setters:[function(a){c=a},function(a){d=a},function(a){e=a},function(a){f=a},function(a){g=a},function(a){h=a},function(a){i=a},function(a){j=a},function(a){k=a},function(a){l=a}],execute:function(){!function(a){a[a.List=0]="List",a[a.Edit=1]="Edit",a[a.Delete=2]="Delete",a[a.Add=3]="Add"}(m||(m={})),a("MotdMode",m),n=function(a){function b(b,c,d,e,f){var g=a.call(this)||this;return g.http=b,g.aws=c,g.toastr=d,g.metric=f,g.subNavActiveIndex=0,g.pageSize=5,g.toastr.setRootViewContainerRef(e),g}return o(b,a),b.prototype.ngOnInit=function(){this._apiHandler=new f.MessageOfTheDayApi(this.context.ServiceUrl,this.http,this.aws,this.metric,this.context.identifier),this.updateMessages=this.updateMessages.bind(this),this.updateActiveMessages=this.updateActiveMessages.bind(this),this.updatePlannedMessages=this.updatePlannedMessages.bind(this),this.updateExpiredMessages=this.updateExpiredMessages.bind(this),this.addModal=this.addModal.bind(this),this.editModal=this.editModal.bind(this),this.deleteModal=this.deleteModal.bind(this),this.dismissModal=this.dismissModal.bind(this),this.validate=this.validate.bind(this),this.delete=this.delete.bind(this),this.onSubmit=this.onSubmit.bind(this),this.motdModes=m,this.updateMessages(),this.currentMessage=this.default()},b.prototype.validate=function(a){var b=!0;return a.priority.valid=Number(a.priority.value)>=0,b=b&&a.priority.valid,a.message.valid=!1,a.message.value&&/\S/.test(a.message.value)?a.message.value.length<1?a.message.message="The message was not long enough.":a.message.value.length>700?a.message.message="The message was too long. We counted "+a.message.value.length+" characters.  The limit is 700 characters.":a.message.valid=!0:a.message.message="The message was invalid",b=b&&a.message.valid&&a.datetime.valid},b.prototype.onSubmit=function(a){var b=this;if(this.validate(a)){var c={message:a.message.value,priority:a.priority.value},e=null,f=null;a.hasStart&&a.start.date&&a.start.date.year&&(e=d.DateTimeUtil.toDate(a.start),c.startTime=e),a.hasEnd&&a.end.valid&&(f=d.DateTimeUtil.toDate(a.end),c.endTime=f),this.modalRef.close(),a.UniqueMsgID?this._apiHandler.put(a.UniqueMsgID,c).subscribe(function(){b.toastr.success("The message '"+c.message+"' has been updated."),b.updateMessages()},function(a){b.toastr.error("The message did not update correctly. "+a.message)}):this._apiHandler.post(c).subscribe(function(){b.toastr.success("The message '"+c.message+"' has been created."),b.updateMessages()},function(a){b.toastr.error("The message did not update correctly. "+a.message)}),this.mode=m.List}},b.prototype.updateActiveMessages=function(a){var b=this;this._apiHandler.getMessages("active",0,1e3).subscribe(function(a){var c=JSON.parse(a.body.text()),d=c.result.list.length;b.activeMessagePages=Math.ceil(d/b.pageSize)}),this.isLoadingActiveMessages=!0,this._apiHandler.getMessages("active",(a-1)*this.pageSize,this.pageSize).subscribe(function(a){var c=JSON.parse(a.body.text());b.activeMessages=c.result.list.sort(function(a,b){return a.priority>b.Priority?1:a.priority<b.priority?-1:0}),b.isLoadingActiveMessages=!1},function(a){b.toastr.error("The active messages of the day did not refresh properly. "+a),b.isLoadingActiveMessages=!1})},b.prototype.updatePlannedMessages=function(a){var b=this;this._apiHandler.getMessages("planned",0,1e3).subscribe(function(a){var c=JSON.parse(a.body.text()),d=c.result.list.length;b.plannedMessagePages=Math.ceil(d/b.pageSize)}),this.isLoadingPlannedMessages=!0,this._apiHandler.getMessages("planned",(a-1)*this.pageSize,this.pageSize).subscribe(function(a){var c=JSON.parse(a.body.text());b.plannedMessages=c.result.list,b.isLoadingPlannedMessages=!1},function(a){b.toastr.error("The planned messages of the day did not refresh properly. "+a),b.isLoadingPlannedMessages=!1})},b.prototype.updateExpiredMessages=function(a){var b=this,c=2*this.pageSize;this._apiHandler.getMessages("expired",0,1e3).subscribe(function(a){var d=JSON.parse(a.body.text()),e=d.result.list.length;b.expiredMessagePages=Math.ceil(e/c)}),this.isLoadingExpiredMessages=!0,this._apiHandler.getMessages("expired",(a-1)*c,c).subscribe(function(a){var c=JSON.parse(a.body.text());b.expiredMessages=c.result.list,b.isLoadingExpiredMessages=!1},function(a){b.toastr.error("The expired messages of the day did not refresh properly. "+a),b.isLoadingExpiredMessages=!1})},b.prototype.updateMessages=function(){0==this.subNavActiveIndex?(this.updateActiveMessages(1),this.updatePlannedMessages(1)):1==this.subNavActiveIndex&&this.updateExpiredMessages(1)},b.prototype.addModal=function(){this.currentMessage=this.default(),this.mode=m.Add},b.prototype.editModal=function(a){var b=null,c=null,d=!0,e=!0;a.startTime?b=new Date(a.startTime):(b=new Date,d=!1),a.endTime?c=new Date(a.endTime):(c=new Date,e=!1);var g=new f.MotdForm({UniqueMsgID:a.UniqueMsgID,start:{date:{year:b.getFullYear(),month:b.getMonth()+1,day:b.getDate()},time:d?{hour:b.getHours(),minute:b.getMinutes()}:{hour:12,minute:0},valid:!0,isScheduled:b.getFullYear()>0},end:{date:{year:c.getFullYear(),month:c.getMonth()+1,day:c.getDate()},time:e?{hour:c.getHours(),minute:c.getMinutes()}:{hour:12,minute:0},valid:!0,isScheduled:b.getFullYear()>0},priority:{valid:!0,value:a.priority},message:{valid:!0,value:a.message},hasStart:d,hasEnd:e});this.currentMessage=g,this.mode=m.Edit},b.prototype.deleteModal=function(a){a.end={},a.start={},this.mode=m.Delete,this.currentMessage=a},b.prototype.dismissModal=function(){this.mode=m.List},b.prototype.getSubNavItem=function(a){this.subNavActiveIndex=a,this.updateMessages()},b.prototype.delete=function(){var a=this;this.modalRef.close(),this.mode=m.List,this._apiHandler.delete(this.currentMessage.UniqueMsgID).subscribe(function(b){a.toastr.success("The message '"+a.currentMessage.message+"'was deleted"),a.paginationRef.reset(),a.updateMessages()},function(b){a.toastr.error("The message '"+a.currentMessage.message+"' did not delete. "+b)})},b.prototype.dprModelUpdated=function(a){this.currentMessage.hasStart=a.hasStart,this.currentMessage.hasEnd=a.hasEnd,this.currentMessage.start=a.start,this.currentMessage.start.valid=a.start.valid,this.currentMessage.end=a.end,this.currentMessage.end.valid=a.end.valid,this.currentMessage.datetime={valid:a.valid,message:a.date.message}},b.prototype.default=function(){var a=this.getDefaultStartDateTime(),b=this.getDefaultEndDateTime();return new f.MotdForm({start:a,end:b,priority:{valid:!0,value:0},message:{valid:!0},hasStart:!1,hasEnd:!1})},b.prototype.getDefaultStartDateTime=function(){var a=new Date;return{date:{year:a.getFullYear(),month:a.getMonth()+1,day:a.getDate()},time:{hour:12,minute:0},valid:!0}},b.prototype.getDefaultEndDateTime=function(){var a=new Date;return{date:{year:a.getFullYear(),month:a.getMonth()+1,day:a.getDate()+1},time:{hour:12,minute:0},valid:!0}},b.prototype.substring=function(a,b){return d.StringUtil.toEtcetera(a,b)},p([c.Input(),q("design:type",Object)],b.prototype,"context",void 0),p([c.ViewChild(e.ModalComponent),q("design:type",e.ModalComponent)],b.prototype,"modalRef",void 0),p([c.ViewChild(i.PaginationComponent),q("design:type",i.PaginationComponent)],b.prototype,"paginationRef",void 0),b=p([c.Component({selector:"message-of-the-day-index",template:'<facet-generator [context]="context"          [tabs]="[\'Overview\', \'History\']"         (tabClicked)="getSubNavItem($event)"         [metricIdentifier]="context.identifier"></facet-generator> <div *ngIf="subNavActiveIndex == 0">     <button class="btn l-primary add-motd-button" (click)="addModal()">         Add Message of the Day     </button>     <h2> Active messages</h2>     <div [ngSwitch]="isLoadingActiveMessages">         <div *ngSwitchCase="true">              <loading-spinner [size]="\'sm\'"></loading-spinner>         </div>         <div class="messages-container" *ngSwitchCase="false">             <div *ngIf="!activeMessages || activeMessages.length == 0">                 No active messages             </div>             <div *ngIf="activeMessages.length > 0">                 <table class="table table-hover">                     <thead>                         <tr>                             <th class="messages-column"> MESSAGE </th>                             <th class="number-column"> PRIORITY </th>                             <th class="date-column"> START </th>                             <th class="date-column"> END <tooltip placement="top" tooltip="Messages remain active for up to 12 hours after the end date, relative to the player\'s time zone."> </tooltip> </th>                         </tr>                     </thead>                     <tbody>                         <tr *ngFor="let message of activeMessages">                             <td class="messages-column" (click)="editModal(message)">  {{substring(message.message, 200)}} </td>                             <td class="number-column" (click)="editModal(message)">  {{message.priority}} </td>                             <td class="date-column" (click)="editModal(message)">  {{message.startTime || \'Now\'}} </td>                             <td class="date-column" (click)="editModal(message)">  {{message.endTime || \'Never Expires\'}} </td>                             <td>                                 <div class="float-right">                                     <i (click)="editModal(message)" class="fa fa-cog edit" data-toggle="tooltip" data-placement="top" title="Edit {{substring(message.message, 50)}}"></i>                                     <i (click)="deleteModal(message)" class="fa fa-trash-o del" data-toggle="tooltip" data-placement="top" title="Delete {{substring(message.message, 50)}}"></i>                                 </div>                             </td>                         </tr>                     </tbody>                 </table>             </div>         </div>         <pagination [pages]="activeMessagePages"                     (pageChanged)="updateActiveMessages($event)"></pagination>     </div>     <h2> Planned messages</h2>     <div [ngSwitch]="isLoadingPlannedMessages">         <div *ngSwitchCase="true">             <loading-spinner [size]="\'sm\'"></loading-spinner>         </div>         <div class="messages-container" *ngSwitchCase="false">             <div *ngIf="!plannedMessages || plannedMessages.length == 0">                 No planned messages             </div>             <div *ngIf="plannedMessages.length > 0">                 <table class="table motd-table table-hover">                     <thead>                         <tr>                             <th class="messages-column"> MESSAGE </th>                             <th class="number-column"> PRIORITY </th>                             <th class="date-column"> START </th>                             <th class="date-column"> END </th>                         </tr>                     </thead>                     <tbody>                         <tr *ngFor="let message of plannedMessages">                             <td class="messages-column" (click)="editModal(message)">  {{substring(message.message, 200)}} </td>                             <td class="number-column" (click)="editModal(message)">  {{message.priority}} </td>                             <td class="date-column" (click)="editModal(message)">  {{message.startTime  ||\'Now\'}}</td>                             <td class="date-column" (click)="editModal(message)">  {{message.endTime || \'Never Expires\'}} </td>                             <td>                                 <div class="float-right">                                     <i (click)="editModal(message)" class="fa fa-cog" data-toggle="tooltip" data-placement="top" title="Edit {{substring(message.message, 50)}}"></i>                                     <i (click)="deleteModal(message)" class="fa fa-trash-o" data-toggle="tooltip" data-placement="top" title="Delete {{substring(message.message, 50)}}"></i>                                 </div>                             </td>                         </tr>                     </tbody>                 </table>             </div>         </div>         <pagination [pages]="plannedMessagePages"                     (pageChanged)="updatePlannedMessages($event)"></pagination>     </div> </div> <div *ngIf="subNavActiveIndex == 1">     \x3c!-- display loading icon if the leaderboards are still being loaded --\x3e     <h2> Expired messages</h2>     <div [ngSwitch]="isLoadingExpiredMessages">         <div *ngSwitchCase="true">             <loading-spinner></loading-spinner>         </div>         <div class="messages-container" *ngSwitchCase="false">             <div *ngIf="!expiredMessages || expiredMessages.length == 0">                 No expired messages             </div>             <div *ngIf="expiredMessages.length > 0">                 <table class="table motd-table table-hover">                     <thead>                         <tr>                             <th class="messages-column"> MESSAGE </th>                             <th class="number-column"> PRIORITY </th>                             <th class="date-column"> START </th>                             <th class="date-column"> END </th>                         </tr>                     </thead>                     <tbody>                         <tr *ngFor="let message of expiredMessages">                             <td class="messages-column" (click)="editModal(message)"> {{substring(message.message, 200)}} </td>                             <td class="number-column" (click)="editModal(message)">  {{message.priority}} </td>                             <td class="date-column" (click)="editModal(message)"> {{message.startTime || \'Now\'}} </td>                             <td class="date-column" (click)="editModal(message)"> {{message.endTime || \'Never Expires\'}} </td>                             <td>                                 <div class="float-right">                                     <i (click)="editModal(message)" class="fa fa-cog" data-toggle="tooltip" data-placement="top" title="Edit {{substring(message.message, 50)}}"></i>                                     <i (click)="deleteModal(message)" class="fa fa-trash-o"></i>                                 </div>                             </td>                         </tr>                     </tbody>                 </table>             </div>         </div>         <pagination [pages]="expiredMessagePages"                     (pageChanged)="updateExpiredMessages($event)"></pagination>     </div> </div> \x3c!-- Add/Edit message modal --\x3e <modal *ngIf="mode == motdModes.Edit || mode == motdModes.Add"        [title]="mode == motdModes.Edit ? \'Edit message\' : \'Add message\'"        [metricIdentifier]="context.identifier"        [autoOpen]="true"        [hasSubmit]="true"        [submitButtonText]="mode == motdModes.Edit ? \'Save Changes\' : \'Create message\'"        [onDismiss]="dismissModal"        [onClose]="dismissModal"        (modalSubmitted)="onSubmit(currentMessage)">     <div class="modal-body">         <form>             <div class="form-group row" [class.has-danger]="!currentMessage.message.valid">                 <label class="col-3 col-form-label affix">Message Content</label>                 <div class="col-9">                     <textarea class="form-control" [(ngModel)]="currentMessage.message.value" name="new-motd-message" placeholder="Enter your new message of the day here" cols="100" rows="5"></textarea>                     <div *ngIf="!currentMessage.message.valid" class="form-control-feedback">{{currentMessage.message.message}}</div>                 </div>             </div>             <div class="form-group row" [class.has-danger]="!(currentMessage.start.valid && currentMessage.end.valid)">                 <label for="schedule" class="col-3 col-form-label affix"> Scheduling </label>                 <div class="col-9">                     <datetime-range-picker                          (dateTimeRange)="dprModelUpdated($event)"                         [startDateModel]="currentMessage.start.date"                         [endDateModel]="currentMessage.end.date"                         [startTime]="currentMessage.start.time"                         [endTime]="currentMessage.end.time"                         [hasStart]="currentMessage.hasStart"                         [hasEnd]="currentMessage.hasEnd"                        ></datetime-range-picker>                     <div *ngIf="!(currentMessage.start.valid && currentMessage.end.valid)" class="form-control-feedback">{{currentMessage.datetime ? currentMessage.datetime.message : ""}}</div>                 </div>             </div>             <div class="form-group row">                 <label for="priority" class="col-3 col-form-label affix"> Priority </label>                 <div class="d-inline-block col-3" ngbDropdown>                     <button type="button" class="btn l-dropdown" id="priority-dropdown" ngbDropdownToggle>                         <span> {{currentMessage.priority.value}} </span>                         <i class="fa fa-caret-down" aria-hidden="true"></i>                     </button>                     <div class="dropdown-menu dropdown-menu-right" aria-labelledby="priority-dropdown">                         <button *ngFor="let priorityNumber of [0,1,2,3,4,5,6]" (click)="currentMessage.priority.value = priorityNumber" type="button" class="dropdown-item"> {{priorityNumber == 0 ? \'0 (Highest)\': priorityNumber}} </button>                     </div>                 </div>             </div>         </form>         \x3c!--<pre>{{currentMessage | json | devonly}}</pre>--\x3e     </div> </modal> \x3c!-- Delete message modal --\x3e <modal *ngIf="mode == motdModes.Delete"        title="Delete Message"        [autoOpen]="true"        [hasSubmit]="true"        [metricIdentifier]="context.identifier"        [onDismiss]="dismissModal"        [onClose]="dismissModal"        submitButtonText="Delete message"        (modalSubmitted)="delete(currentMessage)">     <div class="modal-body">         <p> Are you sure you want to delete this message?</p>         \x3c!--<pre>{{currentMessage | json | devonly}}</pre>--\x3e     </div> </modal>',styles:[".add-motd-button{margin-bottom:20px}table.table tr .messages-column{width:50%}table.table tr .number-column{width:10%;text-align:right}table.table tr .date-column{width:15%;text-align:right}table.table{margin-bottom:30px}.messages-container{margin-bottom:30px}"]}),q("design:paramtypes",[g.Http,h.AwsService,k.ToastsManager,c.ViewContainerRef,l.LyMetricService])],b)}(j.AbstractCloudGemIndexComponent),a("MessageOfTheDayIndexComponent",n)}}}),a.register("32",["10","2e","11","6"],function(a,b){"use strict";var c,d,e,f,g,h=this&&this.__decorate||function(a,b,c,d){var e,f=arguments.length,g=f<3?b:null===d?d=Object.getOwnPropertyDescriptor(b,c):d;if("object"==typeof Reflect&&"function"==typeof Reflect.decorate)g=Reflect.decorate(a,b,c,d);else for(var h=a.length-1;h>=0;h--)(e=a[h])&&(g=(f<3?e(g):f>3?e(b,c,g):e(b,c))||g);return f>3&&g&&Object.defineProperty(b,c,g),g};b&&b.id;return{setters:[function(a){c=a},function(a){d=a},function(a){e=a},function(a){f=a}],execute:function(){g=function(){function a(){}return a=h([f.NgModule({imports:[e.GameSharedModule,c.GemModule],declarations:[d.MessageOfTheDayIndexComponent,d.MessageOfTheDayThumbnailComponent],providers:[],bootstrap:[d.MessageOfTheDayThumbnailComponent,d.MessageOfTheDayIndexComponent]})],a)}(),a("CloudGemMessageOfTheDayModule",g)}}}),a.register("2e",["2c","2d","2f","30","32"],function(a,b){"use strict";function c(b){var c={};for(var d in b)"default"!==d&&(c[d]=b[d]);a(c)}b&&b.id;return{setters:[function(a){c(a)},function(a){c(a)},function(a){c(a)},function(a){c(a)},function(a){c(a)}],execute:function(){}}}),a.register("2b",["2e"],function(a,b){"use strict";function c(){return d.CloudGemMessageOfTheDayModule}b&&b.id;a("definition",c);var d;return{setters:[function(a){d=a}],execute:function(){}}})})(function(a){if("function"==typeof define&&define.amd)define(["app/shared/class/index.js","app/view/game/module/cloudgems/class/index.js","@angular/core","@angular/http","app/aws/aws.service.js","app/shared/service/index.js","app/shared/component/index.js","app/view/game/module/shared/component/index.js","ng2-toastr","app/view/game/module/cloudgems/gem.module.js","app/view/game/module/shared/shared.module.js"],a);else{if("object"!=typeof module||!module.exports||"function"!=typeof require)throw new Error("Module must be loaded as AMD or CommonJS");module.exports=a(require("app/shared/class/index.js"),require("app/view/game/module/cloudgems/class/index.js"),require("@angular/core"),require("@angular/http"),require("app/aws/aws.service.js"),require("app/shared/service/index.js"),require("app/shared/component/index.js"),require("app/view/game/module/shared/component/index.js"),require("ng2-toastr"),require("app/view/game/module/cloudgems/gem.module.js"),require("app/view/game/module/shared/shared.module.js"))}});
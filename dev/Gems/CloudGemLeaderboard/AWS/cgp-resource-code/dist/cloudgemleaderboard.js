!function(a){function b(a,b,c){a in i||(i[a]={name:a,declarative:!0,deps:b,declare:c,normalizedDeps:b})}function c(a){return m[a]||(m[a]={name:a,dependencies:[],exports:{},importers:[]})}function d(b){if(!b.module){var e=b.module=c(b.name),f=b.module.exports,g=b.declare.call(a,function(a,b){if(e.locked=!0,"object"==typeof a)for(var c in a)f[c]=a[c];else f[a]=b;for(var d=0,g=e.importers.length;g>d;d++){var h=e.importers[d];if(!h.locked)for(var i=0;i<h.dependencies.length;++i)h.dependencies[i]===e&&h.setters[i](f)}return e.locked=!1,b},b.name);e.setters=g.setters,e.execute=g.execute;for(var j=0,k=b.normalizedDeps.length;k>j;j++){var l,n=b.normalizedDeps[j],o=i[n],p=m[n];p?l=p.exports:o&&!o.declarative?l=o.esModule:o?(d(o),p=o.module,l=p.exports):l=h(n),p&&p.importers?(p.importers.push(e),e.dependencies.push(p)):e.dependencies.push(null),e.setters[j]&&e.setters[j](l)}}}function e(b){var c={};if(("object"==typeof b||"function"==typeof b)&&b!==a)if(k)for(var d in b)"default"!==d&&f(c,b,d);else{var e=b&&b.hasOwnProperty;for(var d in b)"default"===d||e&&!b.hasOwnProperty(d)||(c[d]=b[d])}return c.default=b,l(c,"__useDefault",{value:!0}),c}function f(a,b,c){try{var d;(d=Object.getOwnPropertyDescriptor(b,c))&&l(a,c,d)}catch(d){return a[c]=b[c],!1}}function g(b,c){var d=i[b];if(d&&!d.evaluated&&d.declarative){c.push(b);for(var e=0,f=d.normalizedDeps.length;f>e;e++){var k=d.normalizedDeps[e];-1==j.call(c,k)&&(i[k]?g(k,c):h(k))}d.evaluated||(d.evaluated=!0,d.module.execute.call(a))}}function h(a){if(o[a])return o[a];if("@node/"==a.substr(0,6))return o[a]=e(n(a.substr(6)));var b=i[a];if(!b)throw"Module "+a+" not present.";return d(i[a]),g(a,[]),i[a]=void 0,b.declarative&&l(b.module.exports,"__esModule",{value:!0}),o[a]=b.declarative?b.module.exports:b.esModule}var i={},j=Array.prototype.indexOf||function(a){for(var b=0,c=this.length;c>b;b++)if(this[b]===a)return b;return-1},k=!0;try{Object.getOwnPropertyDescriptor({a:0},"a")}catch(a){k=!1}var l;!function(){try{Object.defineProperty({},"a",{})&&(l=Object.defineProperty)}catch(a){l=function(a,b,c){try{a[b]=c.value||c.get.call(a)}catch(a){}}}}();var m={},n="undefined"!=typeof System&&System._nodeRequire||"undefined"!=typeof require&&require.resolve&&"undefined"!=typeof process&&require,o={"@empty":{}};return function(a,c,d,f){return function(g){g(function(g){for(var i=0;i<c.length;i++)!function(a,b){b&&b.__esModule?o[a]=b:o[a]=e(b)}(c[i],arguments[i]);f({register:b});var j=h(a[0]);if(a.length>1)for(var i=1;i<a.length;i++)h(a[i]);return d?j.default:j})}}}("undefined"!=typeof self?self:global)(["23"],["3","8","6","7","9","a","1b","d","29","c","10","11"],!1,function(a){this.require,this.exports,this.module;a.register("24",["3"],function(a,b){"use strict";var c,d,e=this&&this.__extends||function(){var a=Object.setPrototypeOf||{__proto__:[]}instanceof Array&&function(a,b){a.__proto__=b}||function(a,b){for(var c in b)b.hasOwnProperty(c)&&(a[c]=b[c])};return function(b,c){function d(){this.constructor=b}a(b,c),b.prototype=null===c?Object.create(c):(d.prototype=c.prototype,new d)}}();b&&b.id;return{setters:[function(a){c=a}],execute:function(){d=function(a){function b(b,c,d,e,f){return void 0===e&&(e=null),void 0===f&&(f=void 0),a.call(this,b,c,d,e,f)||this}return e(b,a),b.prototype.getEntries=function(b,c){return a.prototype.post.call(this,"leaderboard/"+b,c)},b.prototype.deleteLeaderboard=function(b){return a.prototype.delete.call(this,"stats/"+b)},b.prototype.addLeaderboard=function(b){return a.prototype.post.call(this,"stats",b)},b.prototype.editLeaderboard=function(b){return a.prototype.post.call(this,"stats",b)},b.prototype.searchLeaderboard=function(b,c){return a.prototype.get.call(this,"score/"+b+"/"+c)},b.prototype.deleteScore=function(b,c){return a.prototype.delete.call(this,"score/"+b+"/"+c)},b.prototype.banPlayer=function(b){return a.prototype.post.call(this,"player/ban/"+b)},b.prototype.unbanPlayer=function(b){return a.prototype.delete.call(this,"player/ban/"+encodeURIComponent(b))},b.prototype.processRecords=function(){return a.prototype.get.call(this,"process_records")},b}(c.ApiHandler),a("LeaderboardApi",d)}}}),a.register("25",["8","6","26","7","9","a"],function(a,b){"use strict";var c,d,e,f,g,h,i,j=this&&this.__extends||function(){var a=Object.setPrototypeOf||{__proto__:[]}instanceof Array&&function(a,b){a.__proto__=b}||function(a,b){for(var c in b)b.hasOwnProperty(c)&&(a[c]=b[c])};return function(b,c){function d(){this.constructor=b}a(b,c),b.prototype=null===c?Object.create(c):(d.prototype=c.prototype,new d)}}(),k=this&&this.__decorate||function(a,b,c,d){var e,f=arguments.length,g=f<3?b:null===d?d=Object.getOwnPropertyDescriptor(b,c):d;if("object"==typeof Reflect&&"function"==typeof Reflect.decorate)g=Reflect.decorate(a,b,c,d);else for(var h=a.length-1;h>=0;h--)(e=a[h])&&(g=(f<3?e(g):f>3?e(b,c,g):e(b,c))||g);return f>3&&g&&Object.defineProperty(b,c,g),g},l=this&&this.__metadata||function(a,b){if("object"==typeof Reflect&&"function"==typeof Reflect.metadata)return Reflect.metadata(a,b)};b&&b.id;return{setters:[function(a){c=a},function(a){d=a},function(a){e=a},function(a){f=a},function(a){g=a},function(a){h=a}],execute:function(){i=function(a){function b(b,d,e){var f=a.call(this)||this;return f.http=b,f.aws=d,f.metricservice=e,f.displayName="Leaderboard",f.srcIcon="https://m.media-amazon.com/images/G/01/cloudcanvas/images/Leadboard_icon_optimized._V518452895_.png",f.state=new c.TackableStatus,f.metric=new c.TackableMeasure,f}return j(b,a),b.prototype.ngOnInit=function(){this._apiHandler=new e.LeaderboardApi(this.context.ServiceUrl,this.http,this.aws,this.metricservice,this.context.identifier),this.report(this.metric),this.assign(this.state)},b.prototype.report=function(a){a.name="Total Leaderboards",a.value="Loading...",this._apiHandler.get("stats").subscribe(function(b){var c=JSON.parse(b.body.text());a.value=c.result.stats.length},function(b){a.value="Offline"})},b.prototype.assign=function(a){a.label="Loading",a.styleType="Loading",this._apiHandler.get("service/status").subscribe(function(b){var c=JSON.parse(b.body.text());a.label="online"==c.result.status?"Online":"Offline",a.styleType="online"==c.result.status?"Enabled":"Offline"},function(b){a.label="Offline",a.styleType="Offline"})},k([d.Input(),l("design:type",Object)],b.prototype,"context",void 0),k([d.Input(),l("design:type",String)],b.prototype,"displayName",void 0),k([d.Input(),l("design:type",String)],b.prototype,"srcIcon",void 0),b=k([d.Component({selector:"cloudgemleaderboard-thumbnail",template:'\n    <thumbnail-gem \n        [title]="displayName" \n        [cost]="\'High\'" \n        [srcIcon]="srcIcon" \n        [metric]="metric" \n        [state]="state" \n        >\n    </thumbnail-gem>'}),l("design:paramtypes",[f.Http,g.AwsService,h.LyMetricService])],b)}(c.AbstractCloudGemThumbnailComponent),a("LeaderboardThumbnailComponent",i)}}}),a.register("27",[],function(a,b){"use strict";var c,d,e;b&&b.id;return{setters:[],execute:function(){c=function(){function a(){}return a}(),a("PageModel",c),d=function(){function a(a,b,c,d,e,f,g){this.name=a,this.mode=b,this.min=""==c||void 0===c||null===c?void 0:Number(c),this.max=""==d||void 0===d||null===d?void 0:Number(d),this.leaderboards=e,this.state=f,this.sample_size=""==g||void 0===g||null===g?void 0:Number(g)}return a}(),a("LeaderboardForm",d),e=function(){function a(){}return a}(),a("PlayerMeasure",e)}}}),a.register("28",["6","1b","d","26","7","9","29","8","c","a"],function(a,b){"use strict";var c,d,e,f,g,h,i,j,k,l,m,n,o,p,q=this&&this.__extends||function(){var a=Object.setPrototypeOf||{__proto__:[]}instanceof Array&&function(a,b){a.__proto__=b}||function(a,b){for(var c in b)b.hasOwnProperty(c)&&(a[c]=b[c])};return function(b,c){function d(){this.constructor=b}a(b,c),b.prototype=null===c?Object.create(c):(d.prototype=c.prototype,new d)}}(),r=this&&this.__decorate||function(a,b,c,d){var e,f=arguments.length,g=f<3?b:null===d?d=Object.getOwnPropertyDescriptor(b,c):d;if("object"==typeof Reflect&&"function"==typeof Reflect.decorate)g=Reflect.decorate(a,b,c,d);else for(var h=a.length-1;h>=0;h--)(e=a[h])&&(g=(f<3?e(g):f>3?e(b,c,g):e(b,c))||g);return f>3&&g&&Object.defineProperty(b,c,g),g},s=this&&this.__metadata||function(a,b){if("object"==typeof Reflect&&"function"==typeof Reflect.metadata)return Reflect.metadata(a,b)};b&&b.id;return{setters:[function(a){c=a},function(a){d=a},function(a){e=a},function(a){f=a},function(a){g=a},function(a){h=a},function(a){i=a},function(a){j=a},function(a){k=a},function(a){l=a}],execute:function(){m=300,function(a){a[a.Add=0]="Add",a[a.List=1]="List",a[a.Show=2]="Show",a[a.Edit=3]="Edit",a[a.Delete=4]="Delete",a[a.Unban=5]="Unban",a[a.ProcessQueue=6]="ProcessQueue"}(n||(n={})),a("LeaderboardMode",n),function(a){a[a.List=0]="List",a[a.BanUser=1]="BanUser",a[a.DeleteScore=2]="DeleteScore"}(o||(o={})),a("PlayerMode",o),p=function(a){function b(b,c,e,f,g,h,i){var j=a.call(this)||this;return j.http=b,j.aws=c,j.cache=e,j.toastr=f,j.metric=h,j.breadcrumbs=i,j.pageSize=25,j.subNavActiveIndex=0,j.searchScoresControl=new d.FormControl,j.toastr.setRootViewContainerRef(g),j}return q(b,a),b.prototype.ngOnInit=function(){this.aggregateModes=[{display:"Overwrite",server:"OVERWRITE"},{display:"Increment",server:"INCREMENT"}],this._apiHandler=new f.LeaderboardApi(this.context.ServiceUrl,this.http,this.aws,this.metric,this.context.identifier),this.addModal=this.addModal.bind(this),this.editModal=this.editModal.bind(this),this.deleteModal=this.deleteModal.bind(this),this.dismissModal=this.dismissModal.bind(this),this.list=this.list.bind(this),this.addLeaderboard=this.addLeaderboard.bind(this),this.editLeaderboard=this.editLeaderboard.bind(this),this.show=this.show.bind(this),this.deleteLeaderboard=this.deleteLeaderboard.bind(this),this.deleteScoreModal=this.deleteScoreModal.bind(this),this.deleteScore=this.deleteScore.bind(this),this.getBannedPlayers=this.getBannedPlayers.bind(this),this.banPlayerModal=this.banPlayerModal.bind(this),this.banPlayer=this.banPlayer.bind(this),this.dismissPlayerModal=this.dismissPlayerModal.bind(this),this.unbanPlayerModal=this.unbanPlayerModal.bind(this),this.unbanPlayer=this.unbanPlayer.bind(this),this.processLeaderboardQueue=this.processLeaderboardQueue.bind(this),this.checkLocalScoreCacheForChanges=this.checkLocalScoreCacheForChanges.bind(this),this.leaderboardModes=n,this.leaderboards=[],this.list(),this.playerModes=o,this.playerMode=o.List,this.currentPlayerSearch="",this.currentLeaderboard=new f.LeaderboardForm("",null,null,null)},b.prototype.playerSearchUpdated=function(a){var b=this;a.value||a.value.length>0?(this.currentLeaderboard.numberOfPages=0,this._apiHandler.searchLeaderboard(this.currentLeaderboard.name,a.value).subscribe(function(a){var c=JSON.parse(a.body.text()),d=c.result.scores;b.currentScores=d,b.currentScores=b.checkLocalScoreCacheForChanges(),b.isLoadingScores=!1},function(a){b.toastr.error(a),b.isLoadingScores=!1})):this.show(this.currentLeaderboard)},b.prototype.validate=function(a){var b=!0,c=a.name.split(" ");if(a.validation={id:{valid:!0},min:{valid:!0},max:{valid:!0},sample_size:{valid:!0}},c.length>1?(b=!1,a.validation.id={valid:!1,message:"The leaderboard identifier can not have any spaces in it."}):0==c.length||0==c[0].length?(b=!1,a.validation.id={valid:!1,message:"The leaderboard identifier can not be empty."}):c[0].toString().match(/^[0-9a-zA-Z]+$/)?c[0]=c[0].trim():(b=!1,a.validation.id={valid:!1,message:"The leaderboard identifier can not contains non-alphanumeric characters"}),this.setNumberValidationObject(a,"max"),this.setNumberValidationObject(a,"min"),this.setNumberValidationObject(a,"sample_size"),a.validation.max.valid&&a.validation.min.valid){var d=Number(a.max),e=Number(a.min);e>=0||""===a.min||void 0===a.min||null===a.min||(a.validation.min.valid=!1,a.validation.min.message="The minimum reportable value must be numeric."),d>=0||""===a.max||void 0===a.max||null===a.max||(a.validation.max.valid=!1,a.validation.max.message="The maximum reportable value must be numeric."),e>d&&(a.validation.max.valid=!1,a.validation.min.valid=!1,a.validation.min.message="The minimum reportable value must be a greater than the maximum reportable value.")}if(a.validation.sample_size.valid){Number(a.sample_size)>=0||""===a.sample_size||void 0===a.sample_size||null===a.sample_size||(a.validation.sample_size.valid=!1,a.validation.sample_size.message="The reservoir size value must be numeric.")}b=a.validation.max.valid&&a.validation.min.valid&&a.validation.sample_size.valid&&a.validation.id.valid;var f=n.Add;if(Number(a.state)==f)for(var g=0;g<a.leaderboards.length;g++){var h=a.leaderboards[g].name;if(h==c[0]){b=!1,a.validation.id={valid:!1,message:"That leaderboard identifier is already in use in your project.  Please choose another name."};break}}return b&&(a.validation=null),b},b.prototype.addModal=function(a){this.mode=n.Add,this.currentLeaderboard=new f.LeaderboardForm("",this.aggregateModes[0].display),this.currentLeaderboard.leaderboards=this.leaderboards,this.currentLeaderboard.state=this.mode.toString()},b.prototype.editModal=function(a){this.mode=n.Edit,this.currentLeaderboard=new f.LeaderboardForm(a.name,a.mode,a.min,a.max,this.leaderboards,this.mode.toString(),a.sample_size)},b.prototype.deleteModal=function(a){this.mode=n.Delete,this.currentLeaderboard=a},b.prototype.dismissModal=function(){this.mode=n.List},b.prototype.show=function(a,b){var c=this;void 0===b&&(b=1),this.currentLeaderboard=a,this.mode=n.Show,this.breadcrumbs.addBreadcrumb(this.currentLeaderboard.name,null),this.isLoadingScores=!0,void 0===a.additional_data&&(a.additional_data={users:[],page:b,page_size:this.pageSize},a.numberOfPages=0),a.additional_data.page=b-1,this.getScoresSub&&this.getScoresSub.unsubscribe(),this.getScoresSub=this._apiHandler.getEntries(a.name,a.additional_data).subscribe(function(b){var d=JSON.parse(b.body.text());c.currentScores=d.result.scores,a.numberOfPages=d.result.total_pages,c.currentScores=c.checkLocalScoreCacheForChanges(),c.isLoadingScores=!1},function(b){c.toastr.error("The leaderboard with id '"+a.name+"' failed to retrieve its data: "+b),c.isLoadingScores=!1})},b.prototype.list=function(){var a=this;this.isLoadingLeaderboards=!0,this.mode=n.List,this.currentScores=null,this._apiHandler.get("stats").subscribe(function(b){var c=JSON.parse(b.body.text());a.leaderboards=c.result.stats,a.isLoadingLeaderboards=!1},function(b){a.toastr.error("The leaderboards did not refresh properly: "+b),a.isLoadingLeaderboards=!1})},b.prototype.addLeaderboard=function(a){var b=this;if(this.validate(a)){var c=this.getPostObject();this.modalRef.close(),this._apiHandler.addLeaderboard(c).subscribe(function(a){b.toastr.success("The leaderboard with id '"+b.currentLeaderboard.name+"' was created."),b.currentLeaderboard=new f.LeaderboardForm(""),b.list(),b.mode=n.List},function(a){b.toastr.error("The leaderboard could not be created: "+a),b.list()})}},b.prototype.editLeaderboard=function(a){var b=this;if(this.validate(a)){var c=this.getPostObject();this.modalRef.close(),this._apiHandler.editLeaderboard(c).subscribe(function(a){b.toastr.success("The leaderboard with id '"+b.currentLeaderboard.name+"' was updated."),b.list(),b.mode=n.List},function(a){b.toastr.error("The leaderboard could not be updated: "+a),b.list()})}},b.prototype.deleteLeaderboard=function(){var a=this;this.modalRef.close(),this._apiHandler.deleteLeaderboard(this.currentLeaderboard.name).subscribe(function(b){a.toastr.success("The leaderboard with id '"+a.currentLeaderboard.name+"' has been deleted."),a.list(),a.mode=n.List},function(b){a.toastr.error("The leaderboard could not be deleted: "+b)})},b.prototype.subNavUpdated=function(a){this.subNavActiveIndex=a,0==this.subNavActiveIndex?this.list():1==this.subNavActiveIndex&&this.getBannedPlayers()},b.prototype.deleteScoreModal=function(a){this.playerMode=o.DeleteScore,this.currentScore=a},b.prototype.deleteScore=function(){var a=this;this.isLoadingScores=!0,this.modalRef.close(),this._apiHandler.deleteScore(this.currentLeaderboard.name,this.currentScore.user).subscribe(function(b){a.toastr.success("The score for player '"+a.currentScore.user+"' was deleted successfully."),a.cache.setObject(a.currentScore,a.currentScore),a.currentScores=a.checkLocalScoreCacheForChanges(),a.isLoadingScores=!1,a.playerMode=o.List},function(b){a.playerMode=o.List,a.toastr.error("The score for could not be deleted: "+b)})},b.prototype.getBannedPlayers=function(){var a=this;this.isLoadingBannedPlayers=!0,this._apiHandler.get("player/ban_list").subscribe(function(b){a.isLoadingBannedPlayers=!1;var c=JSON.parse(b.body.text());a.bannedPlayers=c.result.players},function(b){a.isLoadingBannedPlayers=!1,a.toastr.error("Unable to get the list of banned players: "+b)})},b.prototype.banPlayerModal=function(a){this.playerMode=o.BanUser,this.currentScore=a},b.prototype.banPlayer=function(){var a=this;this.isLoadingScores=!0,this.modalRef.close(),this._apiHandler.banPlayer(this.currentScore.user).subscribe(function(b){JSON.parse(b.body.text());a.toastr.success("The player '"+a.currentScore.user+"' has been banned.  The score will remain until deleted."),a.isLoadingScores=!1,a.playerMode=o.List},function(b){a.playerMode=o.List,a.toastr.error("The player could not be banned: "+b)})},b.prototype.unbanPlayerModal=function(a){this.currentPlayer=a,this.mode=n.Unban},b.prototype.unbanPlayer=function(){var a=this;this.modalRef.close(),this._apiHandler.unbanPlayer(this.currentPlayer).subscribe(function(b){JSON.parse(b.body.text());a.toastr.success("The player '"+a.currentPlayer+"' has had their ban lifted"),a.cache.remove(a.currentPlayer),a.getBannedPlayers(),a.mode=n.List},function(b){a.mode=n.List,a.toastr.error("The ban could not be removed: "+b)})},b.prototype.dismissPlayerModal=function(){this.playerMode=o.List},b.prototype.promptProcessLeaderboard=function(){this.mode=n.ProcessQueue},b.prototype.processLeaderboardQueue=function(){var a=this;this.modalRef.close(),this.mode=n.List,this._apiHandler.processRecords().subscribe(function(b){a.toastr.success("The leaderboard processor has been started. Processing should only take a few moments.")},function(b){a.toastr.error("The leaderboard process failed to start"+b)})},b.prototype.checkLocalScoreCacheForChanges=function(){for(var a=this.currentScores,b=Array(),c=0;c<this.currentScores.length;c++){var d=this.cache.objectExists(this.currentScores[c]),e=this.cache.exists(this.currentScores[c].user);(d||e)&&b.push(c)}var f=0;return b.forEach(function(b){a.splice(b-f,1),f++}),a},b.prototype.getPostObject=function(){return new f.LeaderboardForm(this.currentLeaderboard.name,this.currentLeaderboard.mode.toUpperCase(),null===this.currentLeaderboard.min||void 0===this.currentLeaderboard.min?void 0:this.currentLeaderboard.min.toString(),null===this.currentLeaderboard.max||void 0===this.currentLeaderboard.max?void 0:this.currentLeaderboard.max.toString(),void 0,void 0,null===this.currentLeaderboard.sample_size||void 0===this.currentLeaderboard.sample_size?void 0:this.currentLeaderboard.sample_size.toString())},b.prototype.isValidNumber=function(a,b,c){if(void 0===c&&(c=void 0),""===a||null===a||void 0===a)return!0;var d=Number(a);return NaN!==d&&(b&&c?d>=b&&d<=c:d>=b)},b.prototype.setNumberValidationObject=function(a,b){a[b]=""===a[b]||null===a[b]?void 0:a[b],a.validation[b]=this.isValidNumber(a[b],0),a.validation[b]={message:a.validation[b]?void 0:"This value must be numeric and greater than zero.",valid:a.validation[b]}},r([c.Input(),s("design:type",Object)],b.prototype,"context",void 0),r([c.ViewChild(e.ModalComponent),s("design:type",e.ModalComponent)],b.prototype,"modalRef",void 0),b=r([c.Component({selector:"leaderboard-index",template:'<facet-generator [context]="context"         [tabs]="[\'Overview\', \'Banned Players\', \'Settings\']"         (tabClicked)="subNavUpdated($event)"         [metricIdentifier]="context.identifier"></facet-generator> <div *ngIf="subNavActiveIndex == 0">     <div [ngSwitch]="mode == leaderboardModes.Show">         <div *ngSwitchCase="false">             <button (click)="addModal()" class="btn l-primary add-leaderboard-button">                 Add Leaderboard             </button>             </div>         <div *ngSwitchCase="true">             <search class="float-right"              searchInputPlaceholder="Search Leaderboard"              (searchUpdated)="playerSearchUpdated($event)"> </search>         </div>         <div *ngIf="mode != leaderboardModes.Show" [ngSwitch]="isLoadingLeaderboards">             \x3c!-- display loading icon if the leaderboards are still being loaded --\x3e             <div *ngSwitchCase="true">                 <loading-spinner></loading-spinner>             </div>             <div *ngSwitchCase="false">                 <table class="table table-hover">                     <thead>                         <tr>                             <th > LEADERBOARD ID </th>                             <th > MODE </th>                             <th class="number-column"> MINIMUM ALLOWABLE VALUE </th>                             <th class="number-column"> MAXIMUM ALLOWABLE VALUE </th>                                                                                     <th class="number-column"> RESEVOIR SAMPLE SIZE </th>                         </tr>                     </thead>                     <tbody>                         <tr *ngFor="let leaderboard of leaderboards">                             <td width="20%" (click)="show(leaderboard)">  {{leaderboard.name}} </td>                             <td width="20%" (click)="show(leaderboard)">  {{leaderboard.mode}} </td>                             <td width="15%" class="number-column" (click)="show(leaderboard)">  {{leaderboard.min}} </td>                             <td width="15%" class="number-column" (click)="show(leaderboard)">  {{leaderboard.max}} </td>                             <td width="15%" class="number-column" (click)="show(leaderboard)">  {{leaderboard.sample_size}} </td>                             <td width="15%">                                 <div class="float-right">                                     <i (click)="editModal(leaderboard)" class="fa fa-cog" data-toggle="tooltip" data-placement="top" title="Edit {{leaderboard.name}}"></i>                                     <i (click)="deleteModal(leaderboard)" class="fa fa-trash-o" data-toggle="tooltip" data-placement="top" title="Delete {{leaderboard.name}}"></i>                                 </div>                             </td>                         </tr>                     </tbody>                   </table>             </div>         </div>     </div>     \x3c!-- Template for viewing individual leaderboard --\x3e     <div *ngIf="mode == leaderboardModes.Show" class="show-players-container">         <div class="show-players-heading">             <h2 class="current-leaderboard-title"> {{currentLeaderboard.name}} - <small class="text-muted">Processing of new entries could take up to 5 minutes before they appear here.</small></h2>          </div>         <div [ngSwitch]="isLoadingScores">             <div *ngSwitchCase="true">                 <loading-spinner></loading-spinner>             </div>             <div *ngSwitchCase="false">                 <div *ngIf="currentScores?.length == 0 && !this.searchScoresControl.value" >                     No scores for this leaderboard                  </div>                 <div *ngIf="currentScores?.length == 0 && this.searchScoresControl.value?.length > 0" >                     No scores for search: {{searchScoresControl.value}}                 </div>                 <table class="table" *ngIf="currentScores?.length > 0">                     <thead>                         <tr>                             <th class="player-name-column"> PLAYER </th>                             <th class="score-column"> SCORE </th>                         </tr>                     </thead>                     <tbody>                         <tr *ngFor="let score of currentScores">                             <td width="35%" class="player-name-column">  {{score.user}} </td>                             <td width="25%" class="score-column">  {{score.value}} </td>                             <td width="40%">                                 <div class="float-right">                                     <i (click)="banPlayerModal(score)" class="fa fa-ban" data-toggle="tooltip" data-placement="top" title="Ban user \'{{score.user}}\'"></i>                                     <i (click)="deleteScoreModal(score)" class="fa fa-trash-o" data-toggle="tooltip" data-placement="top" title="Delete score \'{{score.value}}\' for player \'{{score.user}}\'"></i>                                 </div>                             </td>                         </tr>                     </tbody>                   </table>             </div>         </div>         <pagination [pages]="currentLeaderboard.numberOfPages"                     (pageChanged)="show(currentLeaderboard, $event)">         </pagination>          </div> </div> <div *ngIf="subNavActiveIndex == 1">      <div [ngSwitch]="isLoadingBannedPlayers">         <div *ngSwitchCase="true">             <loading-spinner></loading-spinner>         </div>         <div *ngSwitchCase="false">             <p *ngIf="bannedPlayers?.length == 0">                 No banned players             </p>             <table class="table table-hover" *ngIf="bannedPlayers?.length > 0">                 <thead>                     <tr>                         <th > NAME </th>                     </tr>                 </thead>                 <tbody>                     <tr *ngFor="let playerName of bannedPlayers">                         <td width="70%" (click)="unbanPlayerModal(playerName)">  {{playerName}} </td>                         <td width="30%">                             <div class="float-right">                                 <i (click)="unbanPlayerModal(playerName)" class="fa fa-ban"                                 data-toggle="tooltip" data-placement="top" title="Remove Ban on {{playerName}}"></i>                             </div>                         </td>                     </tr>                 </tbody>               </table>          </div>         \x3c!--<pre>{{bannedPlayers | json | devonly}}</pre>--\x3e     </div> </div>  <div *ngIf="subNavActiveIndex == 2">     <div class="form-group row">         <p class="col-3">Leaderboard queue processor</p>         <div class="col-9">                         <button type="button" class="btn l-primary" (click)="promptProcessLeaderboard()"> Process Now </button>         </div>     </div> </div>  \x3c!-- Modals --\x3e \x3c!-- Edit leaderboard modal --\x3e <modal *ngIf="mode == leaderboardModes.Edit || mode == leaderboardModes.Add"    [title]="mode == leaderboardModes.Edit ? \'Edit Leaderboard\' : \'Add Leaderboard\'"    [autoOpen]="true"    [hasSubmit]="true"    [metricIdentifier]="context.identifier"    [onClose]="dismissModal"    [onDismiss]="dismissModal"    [submitButtonText]="mode == leaderboardModes.Edit ? \'Save Changes\' : \'Create Leaderboard\'"    (modalSubmitted)="mode == leaderboardModes.Edit ? editLeaderboard(currentLeaderboard) : addLeaderboard(currentLeaderboard)">     <div class="modal-body">         <form>             <div class="form-group row" [class.has-danger]="currentLeaderboard.validation && currentLeaderboard.validation.id.valid == false">                 <label class="col-5 col-form-label affix ">Leaderboard Id</label>                 <div class="col-7 form-control-danger">                                         <input class="form-control full-width" [(ngModel)]="currentLeaderboard.name" name="currentLeaderboard.name" type="text" [disabled]="mode == leaderboardModes.Edit" />                     <div *ngIf="currentLeaderboard.validation && currentLeaderboard.validation.id.valid == false" class="form-control-feedback">{{currentLeaderboard.validation.id.message}}</div>                 </div>             </div>             <div class="form-group row">                 <label class="col-5 col-form-label affix ">Mode</label>                 <div class="col-7 form-control-danger">                                         <div class="d-inline-block" ngbDropdown>                         <button type="button" class="btn l-dropdown" id="priority-dropdown" ngbDropdownToggle>                             <span>{{currentLeaderboard.mode }}</span>                             <i class="fa fa-caret-down" aria-hidden="true"></i>                         </button>                         <div class="dropdown-menu dropdown-menu-right" aria-labelledby="priority-dropdown">                             <button *ngFor="let mode of aggregateModes" (click)="currentLeaderboard.mode = mode.display" type="button" class="dropdown-item"> {{mode.display}} </button>                         </div>                     </div>                 </div>             </div>             <div class="form-group row" [class.has-danger]="currentLeaderboard.validation && currentLeaderboard.validation.min.valid == false">                 <label class="col-5 col-form-label affix ">Minimum Reportable Value Allowed</label>                 <div class="col-7 form-control-danger">                     <input class="form-control full-width" [(ngModel)]="currentLeaderboard.min" name="currentLeaderboard.min" type="text" />                     <div *ngIf="currentLeaderboard.validation && currentLeaderboard.validation.min.valid == false" class="form-control-feedback">{{currentLeaderboard.validation.min.message}}</div>                 </div>             </div>             <div class="form-group row" [class.has-danger]="currentLeaderboard.validation && currentLeaderboard.validation.max.valid == false" >                 <label class="col-5 col-form-label affix ">Maximum Reportable Value Allowed</label>                 <div class="col-7 form-control-danger">                                                         <input class="form-control full-width" [(ngModel)]="currentLeaderboard.max" name="currentLeaderboard.max" type="text" />                     <div *ngIf="currentLeaderboard.validation && currentLeaderboard.validation.max.valid == false" class="form-control-feedback">{{currentLeaderboard.validation.max.message}}</div>                 </div>             </div>                 <div class="form-group row" [class.has-danger]="currentLeaderboard.validation && currentLeaderboard.validation.sample_size.valid == false">                 <label class="col-5 col-form-label affix ">Reservoir Sample Size <i class="fa fa-question-circle-o" placement="right"                     ngbTooltip="The leaderboard sample resevoir size. The recommended range is between 200 to 1000. Example: 300"></i></label>                 <div class="col-7 form-control-danger">                     <input class="form-control full-width" [(ngModel)]="currentLeaderboard.sample_size" name="currentLeaderboard.sample_size" type="text" />                     <div *ngIf="currentLeaderboard.validation && currentLeaderboard.validation.sample_size.valid == false" class="form-control-feedback">{{currentLeaderboard.validation.sample_size.message}}</div>                 </div>             </div>                     </form>         \x3c!--<pre>{{currentLeaderboard | json | devonly}}</pre>--\x3e     </div> </modal>  \x3c!-- Delete leaderboard modal --\x3e <modal *ngIf="mode == leaderboardModes.Delete"        title="Delete Leaderboard"        [autoOpen]="true"        [metricIdentifier]="context.identifier"        [hasSubmit]="true"        [onClose]="dismissModal"        [onDismiss]="dismissModal"        (modalSubmitted)="deleteLeaderboard(currentLeaderboard)"        submitButtonText ="Delete Leaderboard">     <div class="modal-body">         <p> Are you sure you want to delete leaderboard: {{currentLeaderboard.name}}?</p>         <p> All data in {{currentLeaderboard.name}} will be lost. </p>         \x3c!--<pre>{{currentLeaderboard | json | devonly}}</pre>--\x3e     </div> </modal>  \x3c!-- Player modals --\x3e \x3c!-- Ban user modal --\x3e <modal *ngIf="playerMode == playerModes.BanUser"        title="Ban User"        [autoOpen]="true"        [metricIdentifier]="context.identifier"        [onClose]="dismissPlayerModal"        [onDismiss]="dismissPlayerModal"        [hasSubmit]="true"        (modalSubmitted)="banPlayer(currentPlayer)"        submitButtonText ="Ban Player">     <div class="modal-body">         <p> Are you sure you want to ban the player \'{{currentScore.user}}\'?</p>         <p> \'{{currentScore.user}}\' will have their future scores rejected.  All their current scores will remain.  The scores can be deleted manually. </p>                 <p> These changes will be reflected locally in the CGP for now and may take a few minutes to propagate to the server. </p>         \x3c!--<pre>{{currentScore | json | devonly}}</pre>--\x3e     </div> </modal>  \x3c!-- Delete score modal --\x3e <modal *ngIf="playerMode == playerModes.DeleteScore"        title="Delete Score"        [autoOpen]="true"        [metricIdentifier]="context.identifier"        submitButtonText="Delete Score"        [onClose]="dismissPlayerModal"        [onDismiss]="dismissPlayerModal"        [hasSubmit]="true"        (modalSubmitted)="deleteScore(currentScore)">     <div class="modal-body">         <p> Are you sure you want to delete the player \'{{currentScore.user}}\' with score \'{{currentScore.value}}\'? </p>         <p> These changes will be reflected locally in the CGP for now and may take a few minutes to propagate to the server. </p>         \x3c!--<pre>{{currentScore | json | devonly}}</pre>--\x3e     </div> </modal>  \x3c!-- Un-ban user modal --\x3e <modal *ngIf="mode == leaderboardModes.Unban"        title="Remove Ban"        [autoOpen]="true"        [metricIdentifier]="context.identifier"        [hasSubmit]="true"        [onClose]="dismissModal"        [onDismiss]="dismissModal"        (modalSubmitted)="unbanPlayer(currentPlayer)"        submitButtonText="Remove Ban">     <div class="modal-body">         Are you sure you want to remove the ban on {{currentPlayer}}?  This will restore all their scores to existing leaderboards and allow their future scores to be posted.         <p> These changes will be reflected locally in the CGP for now and may take a few minutes to propagate to the server. </p>         \x3c!--<pre>{{currentPlayer | json | devonly}}</pre>--\x3e     </div> </modal>   \x3c!-- Process leaderboard modal --\x3e <modal *ngIf="mode == leaderboardModes.ProcessQueue"        title="Process Leaderboard Queue"        [autoOpen]="true"        [metricIdentifier]="context.identifier"        (modalSubmitted)="processLeaderboardQueue()"        submitButtonText="Process Queue"        [hasSubmit]="true"        [onClose]="dismissModal"        [onDismiss]="dismissModal">     <div class="modal-body">         <p> Processing the leaderboard queue manually is a <b>development only</b> feature.</p>         <p> This feature could lead to duplicate entries.  It is highly recommended that you do not to use this feature in an production environment.</p>         <p> By default the leaderboard queue processor executes on a scheduled cycle.  The default is every 2 minutes.</p>         <p> Are you sure you want to manually call the processor?</p>                     </div> </modal>',styles:[".add-leaderboard-button{margin-bottom:20px}.score-column{text-align:right;width:15%}table.table tr .number-column{text-align:right}table.table tr .date-column{width:15%;text-align:right}.controls:hover{cursor:pointer}"]}),s("design:paramtypes",[g.Http,h.AwsService,i.CacheHandlerService,k.ToastsManager,c.ViewContainerRef,l.LyMetricService,l.BreadcrumbService])],b)}(j.AbstractCloudGemIndexComponent),a("LeaderboardIndexComponent",p)}}}),a.register("2a",["6","10","26","11"],function(a,b){"use strict";var c,d,e,f,g,h=this&&this.__decorate||function(a,b,c,d){var e,f=arguments.length,g=f<3?b:null===d?d=Object.getOwnPropertyDescriptor(b,c):d;if("object"==typeof Reflect&&"function"==typeof Reflect.decorate)g=Reflect.decorate(a,b,c,d);else for(var h=a.length-1;h>=0;h--)(e=a[h])&&(g=(f<3?e(g):f>3?e(b,c,g):e(b,c))||g);return f>3&&g&&Object.defineProperty(b,c,g),g};b&&b.id;return{setters:[function(a){c=a},function(a){d=a},function(a){e=a},function(a){f=a}],execute:function(){g=function(){function a(){}return a=h([c.NgModule({imports:[f.GameSharedModule,d.GemModule],declarations:[e.LeaderboardIndexComponent,e.LeaderboardThumbnailComponent],providers:[],bootstrap:[e.LeaderboardThumbnailComponent,e.LeaderboardIndexComponent]})],a)}(),a("CloudGemLeaderboardModule",g)}}}),a.register("26",["24","25","27","28","2a"],function(a,b){"use strict";function c(b){var c={};for(var d in b)"default"!==d&&(c[d]=b[d]);a(c)}b&&b.id;return{setters:[function(a){c(a)},function(a){c(a)},function(a){c(a)},function(a){c(a)},function(a){c(a)}],execute:function(){}}}),a.register("23",["26"],function(a,b){"use strict";function c(){return d.CloudGemLeaderboardModule}b&&b.id;a("definition",c);var d;return{setters:[function(a){d=a}],execute:function(){}}})})(function(a){if("function"==typeof define&&define.amd)define(["app/shared/class/index.js","app/view/game/module/cloudgems/class/index.js","@angular/core","@angular/http","app/aws/aws.service.js","app/shared/service/index.js","@angular/forms","app/shared/component/index.js","app/view/game/module/cloudgems/service/cachehandler.service.js","ng2-toastr","app/view/game/module/cloudgems/gem.module.js","app/view/game/module/shared/shared.module.js"],a);else{if("object"!=typeof module||!module.exports||"function"!=typeof require)throw new Error("Module must be loaded as AMD or CommonJS");module.exports=a(require("app/shared/class/index.js"),require("app/view/game/module/cloudgems/class/index.js"),require("@angular/core"),require("@angular/http"),require("app/aws/aws.service.js"),require("app/shared/service/index.js"),require("@angular/forms"),require("app/shared/component/index.js"),require("app/view/game/module/cloudgems/service/cachehandler.service.js"),require("ng2-toastr"),require("app/view/game/module/cloudgems/gem.module.js"),require("app/view/game/module/shared/shared.module.js"))}});
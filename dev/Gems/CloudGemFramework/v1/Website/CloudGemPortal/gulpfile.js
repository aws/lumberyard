var gulp = require('gulp'),
  typescript = require('gulp-typescript'),
  Builder = require('systemjs-builder'),
  inlineNg2Template = require('gulp-inline-ng2-template'),
  del = require('del'),
  tscConfig_dist = require('./tsconfig-dist.json'),
  tscConfig_dev = require('./tsconfig.json'),
  rename = require('gulp-rename'),
  replace = require('gulp-replace'),
  sass = require('gulp-sass'),
  sourcemaps = require('gulp-sourcemaps'),
  prefix = require('gulp-autoprefixer'),
  chmod = require('gulp-chmod'),
  browserSync = require('browser-sync').create(),
  concat = require('gulp-concat'),
  util = require('gulp-util'),
  nodemon = require('gulp-nodemon'),
  uglify = require('gulp-uglify');

dist_path = "../../AWS/www/"
systemjs_config = "systemjs.config-dist.js"
index_dist = "index-dist.html"
app_bundle = "bundles/app.bundle.js"
app_bundle_dependencies = "bundles/dependencies.bundle.js"
app_min_bundle = "bundles/app.bundle.min.js"
app_min_bundle_dependencies = "bundles/dependencies.bundle.min.js"

var paths = {
    styles: {
        files: ['./app/**/*.scss', './styles/**/*.scss'],
        dest: '.'
    },
    gems: {
        src: './external',
        files: './external/**/*.ts',
        dest: './external/js'
    }
}


// A display error function, to format and make custom errors more uniform
// Could be combined with gulp-util or npm colors for nicer output
var displayError = function (error) {
    // Initial building up of the error
    var errorString = '[' + error.plugin + ']';
    errorString += ' ' + error.message.replace("\n", ''); // Removes new line at the end
    // If the error contains the filename or line number add it to the string
    if (error.fileName)
        errorString += ' in ' + error.fileName;
    if (error.lineNumber)
        errorString += ' on line ' + error.lineNumber;
    // This will output an error like the following:
    // [gulp-sass] error message in file_name on line 1
    console.error(errorString);
}
gulp.task('sass', function () {
    // Taking the path from the above object
    gulp.src(paths.styles.files, { base: "./" } )
    // Sass options - make the output compressed and add the source map
    // Also pull the include path from the paths object
    .pipe(sass({ outputStyle: 'compressed' }))
    // If there is an error, don't stop compiling but use the custom displayError function
    .on('error', function (err) {
        displayError(err);
    })
    // Pass the compiled sass through the prefixer with defined 
    .pipe(prefix(
        'last 2 version', 'safari 5', 'ie 8', 'ie 9', 'opera 12.1', 'ios 6', 'android 4'
    ))
    .pipe(chmod(666))
    .pipe(gulp.dest("./", { overwrite: true, force: true }));
});
// This is the default task - which is run when `gulp` is run
gulp.task('serve-watch', ['sass', 'gem-watch', 'gem-ts'], function () {

    // Start a server 
    nodemon({
        // the script to run the app
        script: 'server.js',
        // this listens to changes in any of these files/routes and restarts the application
        watch: ["server.js"],
        ext: 'js ts'
        // Below i'm using es6 arrow functions but you can remove the arrow and have it a normal .on('restart', function() { // then place your stuff in here }
    }).on('restart', () => {
        gulp.src('server.js')
    });

    // Watch the files in the paths object, and when there is a change, fun the functions in the array
    gulp.watch(paths.styles.files, ['sass'])
    .on('change', function (evt) {
        console.log(
            '[watcher] File ' + evt.path.replace(/.*(?=sass)/, '') + ' was ' + evt.type + ', compiling...'
        );
    });
});

gulp.task("serve", ['gem-watch', 'gem-ts', 'sass'], function() {
    nodemon({
        script: 'server.js',
        ext: 'js ts'
    }).on('restart', () => {
        gulp.src('server.js')
    })
})

gulp.task('gem-watch', function() {
    gulp.watch(paths.gems.files, ['gem-ts'])
    .on('change', function (evt) {
        console.log(
            '[watcher] File ' + evt.path.replace(/.*(?=ts)/, '') + ' was ' + evt.type + ', compiling...'
        )
    })
});


// Typescript compilation for gems in development
gulp.task('gem-ts', function() {
    return gulp.src(paths.gems.files, { base: "./"})
      .pipe(sourcemaps.init())
      .pipe(typescript(tscConfig_dev.compilerOptions))
      .pipe(gulp.dest("./"));
})

gulp.task('default', ['serve']);

gulp.task('build_deploy', ['bundle-app', 'bundle-dependencies'], function () {
    gulp.run('copy:assets');
    //gulp.run('copy:asset:systemjs:config');
    gulp.run('copy:asset:index');
    gulp.run('set:dev_define');
});

gulp.task('set:prod_define', function () {
    gulp.src(['app/shared/service/definition.service.ts'], { base: './' })
      .pipe(replace("public isProd: boolean = false", "public isProd: boolean = true"))
      .pipe(chmod(666))
      .pipe(gulp.dest('./', { overwrite: true, force: true }));

    gulp.src(['app/main.ts'], { base: './' })
      .pipe(replace("//enableProdMode();", "enableProdMode();"))
      .pipe(chmod(666))
      .pipe(gulp.dest('./', { overwrite: true, force: true }));

});


gulp.task('set:dev_define', function () {
    gulp.src(['app/shared/service/definition.service.ts'], { base: './' })
      .pipe(replace("public isProd: boolean = true", "public isProd: boolean = false"))
      .pipe(chmod(666))
      .pipe(gulp.dest('./', { overwrite: true, force: true }));

    gulp.src(['app/main.ts'], { base: './' })
      .pipe(replace("enableProdMode();", "//enableProdMode();"))
      .pipe(chmod(666))
      .pipe(gulp.dest('./', { overwrite: true, force: true }));
});


gulp.task('inline-templates', ['clean:bundles', 'clean:dist', 'clean', 'set:prod_define', 'sass'], function () {
    return gulp.src('app/**/*.ts')
      .pipe(inlineNg2Template({ UseRelativePaths: true, indent: 0, removeLineBreaks: true, }))
      .pipe(typescript(tscConfig_dist.compilerOptions))
      .pipe(gulp.dest('./dist/app/'))
});

gulp.task('bundle-app', ['inline-templates'], function () {
    // optional constructor options
    // sets the baseURL and loads the configuration file
    var builder = new Builder('', systemjs_config);
    builder.loader.defaultJSExtensions = true;
    return builder
      .bundle('[dist/app/**/*.js]', app_bundle, {
          minify: true,
          uglify: {
              beautify: {
                  comments: require('uglify-save-license')
              }
          }
      })
      .then(function () {
          console.log('Build complete');
      })
      .catch(function (err) {
          console.log('Build error');
          console.log(err);
      });
});

gulp.task('bundle-dependencies', ['inline-templates'], function () {
    // optional constructor options
    // sets the baseURL and loads the configuration file
    var builder = new Builder('', systemjs_config);
    return builder
      .bundle('dist/app/**/* - [dist/app/**/*]', app_bundle_dependencies, {
          minify: true,
          uglify: {
              beautify: {
                  comments: require('uglify-save-license')
              }
          }
      })
      .then(function () {
          console.log('Build complete');
      })
      .catch(function (err) {
          console.log('Build error');
          console.log(err);
      });
});


// minify the app bundle
gulp.task('bundle:minify:app', ['bundle-app'], function () {
    var builder = new Builder('', systemjs_config);
    return builder.bundle(app_bundle, app_min_bundle, {
        minify: true,
        uglify: {
            beautify: {
                comments: require('uglify-save-license')
            }
        }
    }).then(function () {
        console.log('Build complete');
    })
      .catch(function (err) {
          console.log('Build error');
          console.log(err);
      });
});

// clean the contents of the distribution directory
gulp.task('clean:bundles', function () {
    return del('bundles/**/*', { force: true });
});

gulp.task('clean:dist', function () {
    return del('dist/**/*', { force: true });
});



// clean the contents of the distribution directory
gulp.task('clean', function () {
    return del(dist_path + '**/*', { force: true });
});

// copy dependencies
gulp.task('copy:libs', function () {
    return gulp.src([
   "node_modules/es6-shim/es6-shim.min.js",
   "node_modules/systemjs/dist/system-polyfills.js",
   "node_modules/zone.js/dist/zone.min.js",
   "node_modules/reflect-metadata/Reflect.js",
   "node_modules/systemjs/dist/system-csp-production.js",
   "node_modules/aws-sdk/dist/aws-sdk.min.js",
   "node_modules/crypto-js/crypto-js.js"
    ])
    .pipe(gulp.dest(dist_path + 'lib'))
});
// copy static assets - i.e. non TypeScript compiled source
gulp.task('copy:assets', function () {
    return gulp.src([
        './bundles/**/*',
        './lib/dist/*.js'
    ], { base: './' })
      .pipe(gulp.dest(dist_path))
});

gulp.task('copy:asset:index', function () {
    return gulp.src([
        index_dist,
    ], { base: './' })
      .pipe(rename('index.html'))
      .pipe(gulp.dest(dist_path))
});
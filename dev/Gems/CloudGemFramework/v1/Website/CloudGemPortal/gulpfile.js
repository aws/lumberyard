var gulp = require('gulp');
var typescript = require('gulp-typescript');
var Builder = require('systemjs-builder');
var merge = require('merge-stream');
var inlineNg2Template = require('gulp-inline-ng2-template');
var fs = require('fs');
var path = require('path');
var del = require('del');
var tsconfig = require('./tsconfig.json');
var rename = require('gulp-rename');
var replace = require('gulp-replace');
var sass = require('gulp-sass');
var sourcemaps = require('gulp-sourcemaps');
var prefix = require('gulp-autoprefixer');
var chmod = require('gulp-chmod');
var browserSync = require('browser-sync').create();
var concat = require('gulp-concat');
var util = require('gulp-util');
var nodemon = require('gulp-nodemon');
var uglify = require('gulp-uglify');
var protractor = require("gulp-protractor").protractor;
var webdriver_standalone = require("gulp-protractor").webdriver_standalone;
var webdriver_update = require("gulp-protractor").webdriver_update_specific;
var finder = require('process-finder');
var exec = require('gulp-exec');
var exec_process = require('child_process').exec;
var argv = require('yargs').argv
var through = require('through2'),

dist_path = "../../AWS/www/"
cgp_resource_code_folder = "cgp-resource-code"
cgp_resource_code_src_folder = "src"
cgp_package_name = "cloud-gem-portal"
package_scope_name = "@cloud-gems"
aws_folder = "AWS"
systemjs_config = "config-dist.js"
cgp_bootstrap = "cgp_bootstrap.js"
index_dist = "index-dist.html"
app_bundle = "bundles/app.bundle.js"
app_bundle_dependencies = "bundles/dependencies.bundle.js"
app_min_bundle = "bundles/app.bundle.min.js"
app_min_bundle_dependencies = "bundles/dependencies.bundle.min.js"
environment_file = "app/shared/class/environment.class.ts"
localhost_port = 3000

var paths = {
    styles: {
        files: ['./app/**/*.scss', './styles/**/*.scss', './node_modules/@cloud-gems/**/*.scss'],
        dest: '.'
    },
    gems: {
        src: './node_modules/@cloud-gems',
        files: './node_modules/@cloud-gems/**/*.ts',
        relativeToGemsFolder: path.join(path.join(path.join("..", ".."), ".."), "..")
    }
}

//gulp-exec options
var gulp_exec = {
    options: {
        continueOnError: false, // default = false, true means don't emit error event 
        pipeStdout: true // default = false, true means stdout is written to file.contents    
    },
    reportOptions: {
        err: true, // default = true, false means don't write err 
        stderr: true, // default = true, false means don't write stderr 
        stdout: true // default = true, false means don't write stdout 
    }
};
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
    
    util.log(new Error(errorString));
}

// clean the contents of the distribution directory
gulp.task('clean:bundles', function () {
    return del('bundles/**/*', { force: true }).then(function (paths) {
        console.log('Deleted files and folders:\n', paths.join('\n'));
    });
});

gulp.task('clean:dist', function () {
    return del('dist/**/*', { force: true }).then(function (paths) {
        console.log('Deleted files and folders:\n', paths.join('\n'));
    });
});

gulp.task('clean:gems', function () {
    return del(paths.gems.src + '/**/*.js', { force: true }).then(function (paths) {
        console.log('Deleted files and folders:\n', paths.join('\n'));
    });
})

gulp.task('clean:www', function (cb) {
    del(dist_path + "**/*.js", { force: true }).then(function (paths) {
        console.log('Deleted files and folders:\n', paths.join('\n'));
    });
    del(dist_path + "**/*.css", { force: true }).then(function (paths) {
        console.log('Deleted files and folders:\n', paths.join('\n'));
    });
    del(dist_path + "**/*.html", { force: true }).then(function (paths) {
        console.log('Deleted files and folders:\n', paths.join('\n'));
    });
    del(dist_path + "**/*.json", { force: true }).then(function (paths) {
        console.log('Deleted files and folders:\n', paths.join('\n'));
    });
	del(dist_path + "**/*.map", { force: true }).then(function (paths) {
        console.log('Deleted files and folders:\n', paths.join('\n'));
    });
    cb()
})

gulp.task('sass', function () {
    // Taking the path from the above object
    return gulp.src(paths.styles.files, { base: "./" })
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

//Link the Cloud Gem Portal cloud gem packages
gulp.task('npm:link', function () {
    var cwd = process.cwd();    	
    return cloudGems()	
    .pipe(through.obj(function (file, encoding, done) {		
		var gem_path = file.path.substring(0, file.path.lastIndexOf("\\") + 1)
        var names = packageName(file.path);
        var package_name = names[0]
        var gem_name = names[1]
		util.log("")
		util.log(util.colors.blue("GLOBAL LINKING"));
        util.log("Package Name:\t\t" + package_name);
        util.log("Gem Name:\t\t\t" + gem_name);
        util.log("Path:\t\t\t" + gem_path);
        util.log(gem_path)
        process.chdir(gem_path)   
		var stream = this.push
		exec_process('npm link').on("close", function (code) {
			util.log("LINKING global package complete for '" + package_name + "'")						
			util.log("")
			process.chdir(cwd)		
			if (fs.existsSync("node_modules/@cloud-gems/" + gem_name)) {                			
				done()			
				return;
			}
			util.log(util.colors.yellow("LOCAL LINKING"))
			util.log("LINKING package " + cgp_package_name + " to package " + package_name + ".");
			exec_process('npm link @cloud-gems/' + gem_name).on("close", function (code) {
				util.log("LINKING CGP Cloud gem package to '" + package_name + "' is complete")		
				util.log("")
				done()
			}) 			
		})			
	}))
})

var verifyEnvironmentFile = function () {
    var exists = fs.existsSync(environment_file)
    if (!exists) {
        writeFile(environment_file, "")
    }
}

gulp.task('set:prod-define', function (cb) {
    verifyEnvironmentFile()
    globalDefines()    
    replaceOrAppend(environment_file, /export const isProd: boolean = (true|false)/g, "export const isProd: boolean = true")
    cb()
});

gulp.task('set:test-define-on', function (cb) {
    verifyEnvironmentFile()
    globalDefines()
    replaceOrAppend(environment_file, /export const isTest: boolean = (true|false)/g, "export const isTest: boolean = true")
    cb()
});

gulp.task('set:test-define-off', function (cb) {
    verifyEnvironmentFile()
    globalDefines()
    replaceOrAppend(environment_file, /export const isTest: boolean = (true|false)/g, "export const isTest: boolean = false")
    cb()
});

gulp.task('set:dev-define', function (cb) {
    verifyEnvironmentFile()
    globalDefines()    
    replaceOrAppend(environment_file, /export const isProd: boolean = (true|false)/g, "export const isProd: boolean = false")
    cb()
});

// Typescript compilation for gems in development
gulp.task('gem-ts', function () {
    util.log("COMPILING files in directory '" + process.cwd() + path.sep + paths.gems.files + "'");
    return gulp.src(paths.gems.files, { base: "./" })
        .pipe(sourcemaps.init())
        .pipe(typescript(tsconfig.compilerOptions))
        .pipe(gulp.dest("./"));
});

gulp.task('gem-watch', function () {    
    return cloudGems()     
        .pipe(through.obj(function (file, encoding, done) {                    
            var path = file.path
            var gem_path = path.substring(0, path.lastIndexOf("\\") + 1)
            util.log("WATCHING [*.ts, *.scss] files in directory '" + gem_path + "'");
            gulp.watch(gem_path + "/**/*.ts", {
                depth: 10,
                delay: 100,
                followSymlinks: true,
                ignoreInitial: true
            }, gulp.series('gem-ts'))
                .on('change', function (path, evt) {
                util.log(
                    '[watcher] File ' + path.replace(/.*(?=ts)/, '') + ' was changed, compiling...'
                )
            })
            gulp.watch(gem_path + "/**/*.scss", {
                depth: 10,
                delay: 100,
                followSymlinks: true,
                ignoreInitial: true
            }, gulp.series('sass'))
                .on('change', function (path, evt) {
                util.log(
                    '[watcher] File ' + path.replace(/.*(?=scss)/, '') + ' was changed, compiling...'
                )
            })
			done()
        }))
    
});


// This is the default task - which is run when `gulp` is run
gulp.task('serve-watch', gulp.series('set:test-define-off', 'npm:link', 'set:dev-define', 'sass', 'gem-ts', 'gem-watch', function (cb) {
    // Start a server 
    nodemon({
        // the script to run the app
        script: 'server.js',
        // this listens to changes in any of these files/routes and restarts the application
        watch: ["server.js"]
        // Below i'm using es6 arrow functions but you can remove the arrow and have it a normal .on('restart', function() { // then place your stuff in here }
    }).on('restart', function () {
        gulp.src('server.js')
    });
    
    gulp.watch(["styles/**/*.scss", "app/**/*.scss"], {
        depth: 10,
        delay: 100,
        followSymlinks: true,
        ignoreInitial: false
    }, gulp.series('sass'))
        .on('change', function (path, evt) {
        util.log(
            '[watcher] File ' + path.replace(/.*(?=scss)/, '') + ' was changed, compiling...'
        )
    })
    cb()
}));

gulp.task("serve", gulp.series('set:test-define-off', 'npm:link', 'set:dev-define', 'sass', 'gem-ts', function (cb) {
    nodemon({
        script: 'server.js',
        watch: ["server.js"]
        // Below i'm using es6 arrow functions but you can remove the arrow and have it a normal .on('restart', function() { // then place your stuff in here }
    }).on('restart', function () {
        gulp.src('server.js')
    });
    cb()
}))

gulp.task('default', gulp.series('serve'));


gulp.task('inline-template-app', function (cb) {
    return gulp.src('app/**/*.ts')
        .pipe(inlineNg2Template({ UseRelativePaths: true, indent: 0, removeLineBreaks: true, }))
        .pipe(typescript(tsconfig.compilerOptions))
        .pipe(gulp.dest('dist/app'))

});

gulp.task('inline-template-gem', function () {
    return gulp.src(paths.gems.files)
        .pipe(inlineNg2Template({ UseRelativePaths: true, indent: 0, removeLineBreaks: true, }))
        .pipe(typescript(tsconfig.compilerOptions))
        .pipe(gulp.dest('dist/external'))
});

gulp.task('bundle-gems', gulp.series('npm:link', 'inline-template-gem', 'gem-ts', function (cb) {    
    var builder = new Builder('', systemjs_config);    
	return cloudGems()	
    .pipe(through.obj(function bundle(file, encoding, done) {	
		var gem_path = file.path.substring(0, file.path.lastIndexOf("\\") + 1)
		var names = packageName(file.path);
        var package_name = names[0]
        var gem_name = names[1]
		var source = path.join('dist/external', gem_name, "/**/*.js");		
        var destination = path.join(gem_path, "/../dist/", gem_name.toLowerCase() + ".js");
		util.log(util.colors.yellow("GEM BUNDLING"), "for", gem_name )
        del(destination, { force: true }).then(function (paths) {
            util.log('Deleted path ', paths.join('\n'));
        });
        return builder.buildStatic('[' + source + ']', destination, {
            minify: true,
            uglify: {
                beautify: {
                    comments: require('uglify-save-license')
                }
            }
        }).then(function () {            
            util.log("Bundled gem '" + gem_name + "' and copied to " + destination);
            done()
        })	
	}));	
}))

// copy static assets - i.e. non TypeScript compiled source
gulp.task('copy:assets', function (cb) {
    gulp.src(['./bundles/**/*',], { base: './' })
        .pipe(gulp.dest(dist_path))
    
    gulp.src([index_dist], { base: './' })
        .pipe(rename('index.html'))
        .pipe(gulp.dest(dist_path))
    
    gulp.src([systemjs_config], { base: './' })
        .pipe(rename('config.js'))
        .pipe(gulp.dest(dist_path))
    cb()
});


var bundleApp = function () {
    // optional constructor options
    // sets the baseURL and loads the configuration file
    var builder = new Builder('', systemjs_config);
    builder.loader.defaultJSExtensions = true;
    return builder.bundle('[dist/app/**/*.js]', app_bundle, {
        minify: true,
        uglify: {
            beautify: {
                comments: require('uglify-save-license')
            }
        }
    }).then(function () {
        util.log('Build complete');
    }).catch(function (err) {
        util.log(util.colors.red('Build error'));
        util.log(err);
    });
};

var bundleDependencies = function () {
    // optional constructor options
    // sets the baseURL and loads the configuration file
    //  sourceMaps: true,
    //  lowResSourceMaps: true,
    var builder = new Builder('', systemjs_config);
    return builder.bundle('dist/app/**/* - [dist/app/**/*]', app_bundle_dependencies, {
        minify: true,      
        uglify: {
            beautify: {
                comments: require('uglify-save-license')
            }
        }
    }).then(function () {
        util.log('Build complete');
    }).catch(function (err) {
        util.log('Build error');
        util.log(err);
    });
};

gulp.task('build-deploy',
    gulp.series('clean:www',
        'clean:dist',
        'set:test-define-off',        
        'set:prod-define', 
        'inline-template-app',        
		'bundle-gems',
        gulp.parallel(bundleDependencies, bundleApp),
        'copy:assets',
        'set:dev-define'));

/*
*   E2E TESTING TASKS
*
*/

gulp.task('test:config-path-for-drivers', function (cb) {
    //add the drivers path to the shell path environment variable
    //this is need for firefox and edge
    var cwd = process.cwd();
    var driver = cwd + "\\e2e\\driver\\"
    console.log(driver)
    process.env.PATH = process.env.PATH + ';' + driver
    cb()
});

gulp.task('test:webdriver-update', webdriver_update({
    browsers: ['ignore_ssl']
}));

gulp.task('test:webdriver-standalone', webdriver_standalone);

gulp.task('test:webdriver', gulp.series('test:config-path-for-drivers', 'serve', 'test:webdriver-update', 'set:test-define-on', function (cb) {
    cb()
}));

var testIntegrationWelcomeModal = function (cb) {
    return runTest({ specs: './e2e/authentication-welcome-page.e2e-spec.js' }, 
        [{ pattern: /"firstTimeUse": false/g, value: '"firstTimeUse": true' }], cb)
};

var testIntegrationUserAdministrator = function (cb) {
    return runTest({ specs: './e2e/user-administration.e2e-spec.js' }, 
        [{ pattern: /"firstTimeUse": true/g, value: '"firstTimeUse": false' }], cb);
};

var testIntegrationNoBootstrap = function (cb) {
    var cwd = process.cwd();
    var driver = cwd + "\e2e\driver\geckodriver-v0.18.0-win64.exe"
    return runTest({ specs: './e2e/authentication-no-bootstrap.e2e-spec.js' }, 
        [{ pattern: /var bootstrap = {.*}/g, value: 'var bootstrap = {}' }], cb);
};

var testIntegrationMessageOfTheDay = function (cb) {
    return runTest({ specs: './e2e/message-of-the-day.e2e-spec.js' }, 
        [{ pattern: /"firstTimeUse": true/g, value: '"firstTimeUse": false' }], cb);
};

var killLocalHost = function (cb) {
    exec_process('Taskkill /IM node.exe /F', function (err, stdout, stderr) { });
    exec_process('Taskkill /IM geckodriver.exe /F', function (err, stdout, stderr) { });
    exec_process('Taskkill /IM MicrosoftWebDriver.exe /F', function (err, stdout, stderr) { });
    exec_process('Taskkill /IM chromedriver_2.31.exe /F', function (err, stdout, stderr) { });
    finder.find({
        port: localhost_port,
        info: true
    }, function (err, pids) {
        process.exit();
        pids.forEach(function (pid) {
            try {
                process.kill(pid)
            } catch (err) {
                util.l(err)
         
            } finally {
                cb()
            }
        });
    });
}

gulp.task('test:integration:message-of-the-day', gulp.series('test:webdriver', testIntegrationMessageOfTheDay, killLocalHost));

gulp.task('test:integration:authentication-no-bootstrap', gulp.series('test:webdriver', testIntegrationNoBootstrap, killLocalHost));

gulp.task('test:integration:authentication-welcome-page', gulp.series('test:webdriver', testIntegrationWelcomeModal, killLocalHost));

gulp.task('test:integration:user-administrator', gulp.series('test:webdriver', testIntegrationUserAdministrator, killLocalHost));

gulp.task('test:integration:runall', gulp.series('test:webdriver', function Integration(cb) {
    gulp.series(
        testIntegrationNoBootstrap,
        testIntegrationWelcomeModal,
        gulp.parallel(
            testIntegrationUserAdministrator,
            testIntegrationMessageOfTheDay
        ), killLocalHost, cb)()
}));

/*
*  END TESTING
*/

gulp.task('package:install', function (cb) {
    if (Object.keys(argv).length <= 2) {
        util.log("")
        util.log("usage: package:install --[COMMAND]=[value]")
        util.log("Description:\t\t\tUsed to install a new package or install the currently defined packages.")
        util.log("")
        util.log("COMMAND")
        util.log("")
        util.log("package:\t\t\tName of the package to install.")
        util.log("repos:\t\t\tName of the packages respository.  Example [npm | github]")
        util.log("version:\t\t\tThe version you wish to install.")
        util.log("devonly:\t\t\tThe package will appear in your devDependencies.")
        util.log("")
    }
    util.log("INSTALLING package(s)")
    var npmcommand = 'npm i'
    var jspmcommand = 'jspm install'
    if (argv.package && argv.repos) {
        util.log("\tPackage: " + argv.package)
        util.log("\tRepository: " + argv.repos)
        util.log("\tDevonly: " + argv.devonly)
        util.log("\tVersion: " + argv.version)
        npmcommand = 'npm i ' + argv.package
        jspmcommand = 'jspm install ' + argv.repos + ":" + argv.package
        
        if (argv.version) {
            jspmcommand += "@" + argv.version
            npmcommand += "@" + argv.version
        }
        npmcommand += ' --save '
        
        if (argv.devonly)
            npmcommand += " --save-dev"
    }
    return gulp.src('./')
        .pipe(exec(npmcommand, gulp_exec.options))
        .pipe(exec.reporter(gulp_exec.reportOptions))
        .pipe(exec(jspmcommand, gulp_exec.options))
        .pipe(exec.reporter(gulp_exec.reportOptions))
})

gulp.task('package:uninstall', function () {
    if (Object.keys(argv).length <= 2) {
        util.log("")
        util.log("usage: package:uninstall --[COMMAND]=[value]")
        util.log("Description:\t\t\tUsed to uninstall a package.")
        util.log("")
        util.log("COMMAND")
        util.log("")
        util.log("package:\t\t\tName of the package to install.")
        util.log("repos:\t\t\tName of the packages respository.  Example [npm | github]")
        util.log("")
        return;
    }
    util.log("UNINSTALLING package(s)")
    if (argv.package && argv.repos) {
        util.log("\tPackage: " + argv.package)
        util.log("\tRepository: " + argv.repos)
        var npmcommand = 'npm uninstall ' + argv.package + ' --save'
        var jspmcommand = 'jspm uninstall ' + argv.repos + ":" + argv.package
        return gulp.src('./')
            .pipe(exec(npmcommand, gulp_exec.options))
            .pipe(exec.reporter(gulp_exec.reportOptions))
            .pipe(exec(jspmcommand, gulp_exec.options))
            .pipe(exec.reporter(gulp_exec.reportOptions))
    } else {
        util.log(new util.PluginError('GULP', "No package or repos parameters were found.  Command usage:  gulp package:uninstall --package=<packagename> --repos=<repository['npm'|'github']> ", {
            showStack: true
        }));
        return;
    }
})

function globalDefines() {
    replaceOrAppend(environment_file, /export const metricWhiteListedCloudGem = \[.*\]/g, "export const metricWhiteListedCloudGem = ['CloudGemAWSScriptBehaviors','CloudGemDynamicContent','CloudGemFramework','CloudGemInGameSurvey','CloudGemLeaderboard','CloudGemLoadTest','CloudGemMessageOfTheDay','CloudGemPlayerAccount','CloudGemPolly','CloudGemTextToSpeech']")
    replaceOrAppend(environment_file, /export const metricWhiteListedFeature = \[.*\]/g, "export const metricWhiteListedFeature = ['Cloud Gems', 'Support', 'Analytics', 'Admin']")
    replaceOrAppend(environment_file, /export const metricAdminIndex = 3/g, "export const metricAdminIndex = 3")
}

function packageName(gem_path) {
    var parts = gem_path.split(path.sep)
    var gem_name = parts[parts.length - 1].split('.')[0].toLowerCase()
    var package_name = package_scope_name + "/" + gem_name
    return [package_name, gem_name]
}

// Utility function to get all folders in a path
function getFolders(dir) {
    return fs.readdirSync(dir)
        .filter(function (file) {
        return fs.statSync(path.join(dir, file)).isDirectory();
    });
}

function cloudGems(addquotes, asrelative) {
    //We are looking for any gem with a node project.  There could potentially be other Cloud Gems without node projects but they are not loaded by the CGP.
    var searchPath = paths.gems.relativeToGemsFolder + path.sep + "**" + path.sep + "*.njsproj"
    return gulp.src([searchPath, '!*CloudGemFramework/**/*', '!*node_modules/**/*', '!../../**/*'], { base: "." })
}

function writeFile(filename, content) {
    fs.writeFileSync(filename, content, {}, function (err) {
        if (err)
            util.log("ERROR::writeFile::" + err)
    });
}

function replaceOrAppend(path, regex, content) {
    var file_content = fs.readFileSync(path, 'utf-8').toString();
    
    var match = file_content.match(regex)
    if (match) {
        file_content = file_content.replace(new RegExp(regex, 'g'), content)
    } else {
        file_content = file_content + '\r\n' + content
    }
    writeFile(path, file_content)
}


function replaceFileContent(path, regex, content) {
    var file_content = fs.readFileSync(path, 'utf-8').toString();
    var match = file_content.match(regex)
    if (match) {
        var result = file_content.replace(new RegExp(regex, 'g'), content)
        writeFile(path, result)
    }
}



var runTestExit = function (original_file_content) {
    gulp.series('set:test-define-off')()
    writeFile(cgp_bootstrap, original_file_content);
}

var runTest = function (context, bootstrap_setting, cb, onend, onerr) {
    var protractor_args = []
    var source_file = "./e2e/*.e2e-spec.js"
    
    var original_file_content = fs.readFileSync(cgp_bootstrap, 'utf-8').toString();
    
    if (bootstrap_setting) {
        var new_file_content = original_file_content
        for (var i = 0; i < bootstrap_setting.length; i++) {
            var setting = bootstrap_setting[i]
            util.log(setting)
            replaceFileContent(cgp_bootstrap, setting.pattern, setting.value)
        }
    }
    
    if (context && context.specs) {
        source_file = context.specs
    }
    util.log("Running test with args: ")
    util.log("\t\tSource: \t\t" + source_file)
    util.log("\t\tArgs: \t\t\t\t" + protractor_args)
    util.log("\t\tWorking Directory: \t" + process.cwd())
    util.log("\t\tBootstrap: \t\t" + fs.readFileSync(cgp_bootstrap, 'utf-8').toString())
    
    return gulp.src([source_file])
        .pipe(protractor({
        configFile: "e2e/protractor.config.js",
        args: protractor_args
    }))
        .on('error', function (e) {
        util.log(new util.PluginError('GULP', e.message, {
            showStack: true
        }));
        if (onerr)
            onerr();
        runTestExit(original_file_content);
        if (cb)
            cb()
            
    })
        .on('end', function () {
        if (onend)
            onend();
        runTestExit(original_file_content);
        util.log("The test '" + source_file + "' is done.");
        if (cb)
            cb()
    })
}
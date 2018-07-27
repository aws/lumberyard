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
var argv = require('yargs').argv;
var through = require('through2');
var open = require('gulp-open');

dist_path = "../../AWS/www/"
cgp_resource_code_folder = "cgp-resource-code"
cgp_resource_code_src_folder = "src"
cgp_package_name = "cloud-gem-portal"
package_scope_name = "@cloud-gems"
aws_folder = "AWS"
systemjs_config = "config-dist.js"
systemjs_config_dev = "config.js"
bootstrap_pattern = /{(?=.*"identityPoolId":\s"(\S*)")(?=.*"projectConfigBucketId":\s"(\S*)")(?=.*"userPoolId":\s"(\S*)")(?=.*"region":\s"(\S*)")(?=.*"clientId":\s"(\S*)").*}/g
index_dist = "index-dist.html"
index = "index.html"
app_bundle = "bundles/app.bundle.js"
app_bundle_dependencies = "bundles/dependencies.bundle.js"
app_min_bundle = "bundles/app.bundle.min.js"
app_min_bundle_dependencies = "bundles/dependencies.bundle.min.js"
environment_file = "./app/shared/class/environment.class.ts"
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

gulp.task('clean:assets', function () {
    return del(systemjs_config, { force: true }).then(function (paths) {
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
        if (gem_name == "cloudgemportal")
            return
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

var initializeBootstrap = function (cb) {
    //is the bootstrap set
    var cwd = process.cwd();
    var bootstrap = fs.readFileSync(index, 'utf-8').toString();
    util.log(util.colors.yellow("Looking for a local Cloud Gem Portal bootstrap variable in location '", cwd + path.sep + index, "'."))
    var match = bootstrap.match(bootstrap_pattern)
    if (match) {
        //a local bootstrap has been defined
        util.log(util.colors.yellow("Bootstrap found at path ", cwd + index, "."))
        startServer()
        cb()
    } else {
        //no bootstrap has been set.   Attempting to stand up a project stack

        process.chdir(enginePath())
        exec_process('lmbr_aws project list-resources', function (err, stdout, stderr) {
            util.log(stdout);
            process.chdir(cwd)
            var match = stdout.match(/CloudGemPortal\s*AWS::S3::Bucket\s*[CREATE_COMPLETE|UPDATE_COMPLETE]/i)
            //is there a project stack created?
            if (match) {
                // yes a project stack exists, create the local bootstrap
                writeBootstrapAndStart(cb)
            } else {
                // no
                util.log(util.colors.yellow("No project stack or boostrap is defined."))
                createProjectStack(function (stdout, stderr, err) {
                    var match = stdout.match(/Stack\s'CGPProjStk\S*'\supdate\scomplete/i)
                    //is there a project stack created?
                    if (match) {
                        writeBootstrapAndStart(cb)
                    } else {
                        util.log(util.colors.red("The attempt to automatically create a Cloud Gem Portal development project stack failed.  Please review the logs above."))
                        util.log(util.colors.red("Verify you have the Cloud Gem Framework enabled for your project.  ") + util.colors.yellow("\n\tRun the Project Configurator: " + enginePath() + "<bin folder>" + path.sep + "ProjectConfigurator.exe"))
                        util.log(util.colors.red("Verify you have default AWS credentials set with command: ") + util.colors.yellow("\n\tcd " + enginePath() + "\n\tlmbr_aws profile list"))
                        util.log(util.colors.red("Verify you have sufficient AWS resource capacity.  Ie. You can create more DynamoDb instances."))
                        cb(err);
                    }
                })
            }
        });
    }
}

// This is the default task - which is run when `gulp` is run
gulp.task('serve-watch', gulp.series('set:test-define-off', 'npm:link', 'set:dev-define', 'sass', 'gem-ts', 'gem-watch', initializeBootstrap, function (cb) {
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

gulp.task("serve", gulp.series('set:test-define-off', 'npm:link', 'set:dev-define', 'sass', 'gem-ts', initializeBootstrap))

var launchSite = function () {
    return gulp.src('./index.html')
        .pipe(open({ uri: 'http://localhost:3000' }));
}

gulp.task('default', gulp.series('serve', launchSite))

gulp.task('open', gulp.series('default'))
gulp.task('launch', gulp.series('default'))
gulp.task('start', gulp.series('default'))

gulp.task('inline-template-app', gulp.series('sass', function (cb) {
    return gulp.src('app/**/*.ts')
        .pipe(inlineNg2Template({ UseRelativePaths: true, indent: 0, removeLineBreaks: true, }))
        .pipe(typescript(tsconfig.compilerOptions))
        .pipe(gulp.dest('dist/app'))

}));

gulp.task('inline-template-gem', gulp.series('sass', function () {
    return gulp.src(paths.gems.files)
        .pipe(inlineNg2Template({ UseRelativePaths: true, indent: 0, removeLineBreaks: true, }))
        .pipe(typescript(tsconfig.compilerOptions))
        .pipe(gulp.dest('dist/external'))
}));

var writeIndexDist = function (cb){
    var systemjs_config_file_content = fs.readFileSync(systemjs_config_dev, 'utf-8').toString();
    var regex_config_js = /System\.config\(([\s\S]*?)\);/gm
    var match = regex_config_js.exec(systemjs_config_file_content)
    var match_group = match[1]
    var regex_unquoted_attr = /  ([a-zA-Z].*):/gm

    //replace all non-quoted attributes with their quoted equivalents so we can parse this as JSON
    while (unquoted_attr = regex_unquoted_attr.exec(match_group)) {
        match_group = match_group.replace(unquoted_attr[1]+":", "\""+unquoted_attr[1]+"\":")
    }
    match_group = match_group.replace(/\r\n|\r|\n/gm, '')

    //parse the object into a javascript object
    var config = JSON.parse(match_group)

    //append the required distro alterations
    config.map["app"] = "./dist/app"
    config.defaultJSExtensions = true
    config.packages = {
        "app": {
            "defaultExtension": "js",
            "main": "./main.js"
        }
    }
    var stringified_config = JSON.stringify(config)
    //get the dev index
    var index_file_content = fs.readFileSync(index, 'utf-8').toString();
    //write the empty bootstrap
    index_file_content = index_file_content.replace(/<script id=['|"]bootstrap['|"]>([\s\S])*?<\/script>/i, "<script>var bootstrap= {}</script>")
    //replace the distro config.js with an updated version from the dev config.js
    index_file_content = index_file_content.replace(/<script src=['|"]config.js['|"]>([\s\S])*?<\/script>/i, "<script>System.config(" + stringified_config + ");</script>")
    //write systemjs load script
    var systemjs_loader = 'let normalizeFn = System.normalize; \n' +
                    '\tlet customNormalize = function (name, parentName) { \n' +
                    '\t\tif ((name[0] != \'.\' || (!!name[1] && name[1] != \'/\' && name[1] != \'.\')) && name[0] != \'/\' && !name.match(System.absURLRegEx)) \n' +
                        '\t\t\treturn normalizeFn(name, parentName) \n' +
                    '\t\treturn name \n' +
                    '\t} \n' +
                    '\tSystem.normalize = customNormalize; \n' +
                    '\tSystem.import(dependencies).then(function () { \n' +
                        '\t\tSystem.normalize = customNormalize; \n ' +
                        '\t\tSystem.import(app).then(function () { \n' +
                            '\t\tSystem.import(\'app\'); \n' +
                    '\t\t}); \n' +
                    '\t\tSystem.normalize = normalizeFn; \n' +
                    '\t}); \n' +
                    '\tSystem.normalize = normalizeFn; \n'
    writeFile(index_dist, index_file_content.replace(/<script id=['|"]systemjs_load['|"]>([\s\S])*?<\/script>/i, "<script>" + systemjs_loader + "</script>"))

    //replace the distro config.js with an updated version from the dev config.js
    //gulp uses this file to transpile ts to js.
    writeFile(systemjs_config, "System.config(" + stringified_config + ");")
    cb()
}

gulp.task('js', gulp.series(writeIndexDist))

gulp.task('bundle-gems', gulp.series('inline-template-gem', 'gem-ts', function (cb) {
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

    cb()
});


var bundleApp = function () {
    // optional constructor options
    // sets the baseURL and loads the configuration file
    var builder = new Builder('', systemjs_config);
    builder.loader.defaultJSExtensions = true;
    return builder.bundle('[dist/app/**/*.js]', app_bundle, {
        minify: false,
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
    return builder.bundle('dist/**/* - [dist/**/*]', app_bundle_dependencies, {
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
    gulp.series(gulp.parallel('clean:www','clean:dist','clean:assets'),
        gulp.parallel(gulp.series('set:test-define-off', 'set:prod-define'), writeIndexDist),
        'npm:link',
        'inline-template-app',
        'bundle-gems',
        gulp.parallel(bundleDependencies, bundleApp),
        gulp.parallel('copy:assets', 'set:dev-define'))
);

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

var testIntegrationWelcomeModal = function (cb, username, password, index_page) {
    return runTest({ specs: './e2e/sequential/authentication-welcome-page.e2e-spec.js' },
        [{ pattern: /"firstTimeUse": false/g, value: '"firstTimeUse": true' }], cb, username, password, index_page)
};

var testIntegrationUserAdministrator = function (cb) {
    return runTest({ specs: './e2e/user-administration.e2e-spec.js' },
        [{ pattern: /"firstTimeUse": true/g, value: '"firstTimeUse": false' }], cb);
};

var testIntegrationNoBootstrap = function (cb, username, password, index_page) {
    return runTest({ specs: './e2e/sequential/authentication-no-bootstrap.e2e-spec.js' },
        [{ pattern: /var bootstrap = {.*}/g, value: 'var bootstrap = {}' }], cb, username, password, index_page);
};

var testIntegrationMessageOfTheDay = function (cb) {
    return runTest({ specs: './e2e/message-of-the-day.e2e-spec.js' },
        [{ pattern: /"firstTimeUse": true/g, value: '"firstTimeUse": false' }], cb);
};

var testIntegrationDynamicContent = function (cb) {
    return runTest({ specs: './e2e/dynamic-content.e2e-spec.js' },
        [{ pattern: /"firstTimeUse": true/g, value: '"firstTimeUse": false' }], cb);
};


var testAllParallel = function (cb) {
    return runTest({},
        [{ pattern: /"firstTimeUse": true/g, value: '"firstTimeUse": false' }], cb);
};


var killLocalHost = function (cb) {
    exec_process('Taskkill /IM node.exe /F', function (err, stdout, stderr) { });
    exec_process('Taskkill /IM geckodriver.exe /F', function (err, stdout, stderr) { });
    exec_process('Taskkill /IM MicrosoftWebDriver.exe /F', function (err, stdout, stderr) { });
    exec_process('Taskkill /IM chromedriver_2.32.exe /F', function (err, stdout, stderr) { });
    exec_process('Taskkill /IM chromedriver_2.33.exe /F', function (err, stdout, stderr) { });
    finder.find({
        port: localhost_port,
        info: true
    }, function (err, pids) {
        pids.forEach(function (pid) {
            try {
                process.kill(pid)
            } catch (err) {
                util.log(err)
            } finally {
                cb()
            }
        });
        if (!pids) {
            cb()
        }
    });
}

var createTestRegion = function (gulp_callback, region, out) {
    createProjectStack(function (stdout, stderr, err, username, password) {
        var match = stdout.match(/Stack\s'CGPProjStk\S*'\supdate\scomplete/i)
        //is there a project stack created?
        if (match) {
            //create a deployment
            out.username = username
            out.password = password
            //createDevelopmentStack(function (stdout, stderr, err) {
            //    if (stderr) {
            //        writeBootstrapAndStart(gulp_callback)
            //    }
                gulp_callback();
            //})
        } else {
            util.log(util.colors.red("The attempt to automatically create a Cloud Gem Portal development project stack failed.  Please review the logs above."))
            util.log(util.colors.red("Verify you have the Cloud Gem Framework enabled for your project.  ") + util.colors.yellow("\n\tRun the Project Configurator: " + enginePath() + "<bin folder>" + path.sep + "ProjectConfigurator.exe"))
            util.log(util.colors.red("Verify you have default AWS credentials set with command: ") + util.colors.yellow("\n\tcd " + enginePath() + "\n\tlmbr_aws profile list"))
            util.log(util.colors.red("Verify you have sufficient AWS resource capacity.  Ie. You can create more DynamoDb instances."))
            gulp_callback(err);
        }
    }, region)
}

gulp.task('test:integration:message-of-the-day', gulp.series('test:webdriver', testIntegrationMessageOfTheDay, killLocalHost));

gulp.task('test:integration:authentication-no-bootstrap', gulp.series('test:webdriver', testIntegrationNoBootstrap, killLocalHost));

gulp.task('test:integration:authentication-welcome-page', gulp.series('test:webdriver', testIntegrationWelcomeModal, killLocalHost));

gulp.task('test:integration:dynamic-content', gulp.series('test:webdriver', testIntegrationDynamicContent, killLocalHost));

gulp.task('test:integration:user-administrator', gulp.series('test:webdriver', testIntegrationUserAdministrator, killLocalHost));

gulp.task('test:integration:runall', gulp.series('test:webdriver', function Integration(cb) {
    gulp.series(
        testIntegrationNoBootstrap,
        testIntegrationWelcomeModal,
        gulp.parallel(
            testIntegrationDynamicContent,
            testIntegrationMessageOfTheDay
        ),
        testIntegrationUserAdministrator,
        killLocalHost,
        cb)()
}));

// sleep time expects milliseconds
function sleep(time) {
    return new Promise(function (resolve) { setTimeout(resolve, time) });
}

gulp.task('test:integration:runallregions', gulp.series(function createRegion(region_all_cb) {
    var bootstrap_file = fs.readFileSync(index, 'utf-8').toString();
    //var bootstrap_information = bootstrap_file.match(bootstrap_pattern)
    //util.log(util.colors.yellow("The original bootstrap is \n" + bootstrap_information))
    var regions = ['us-east-1']
    //var bootstrap = bootstrap_information ?  JSON.parse(bootstrap_information) : {}
    readLocalProjectSettingsPath(function (settings_path) {
        var region_creation = function (cb) { cb() };
        var project_settings_path = settings_path
        var project_settings = fs.readFileSync(settings_path, 'utf-8').toString();
        let project_settings_obj = JSON.parse(project_settings)
        util.log("Here is your local-project-settings.json file in case something goes wrong.")
        util.log(util.colors.green(project_settings))
        var filename = project_settings_path + "_bak"
        util.log(util.colors.yellow("Saving the local_project_settings.json as " + filename))
        writeFile(filename, project_settings)

        for (var i = 0; i < regions.length; i++) {
            let region = regions[i]
            let multiplier = i
            console.log(region)
            region_creation = gulp.parallel(region_creation,
                 gulp.series(
                    function createRegionProjectStack(cb1) {
                        var project_region = undefined
                        if (project_settings_obj[region] && project_settings_obj[region].ProjectStackId) {
                            var parts = project_settings_obj[region].ProjectStackId.split(":")
                            project_region = parts[3]
                        }
                        util.log("Region to test: \t\t\t " + region)
                        util.log("Project settings region object: \t " + JSON.stringify(project_settings_obj[region]))
                        if (project_region === undefined || project_region != region) {
                            //create the test region, we offset the starts to avoid colisions in the CGF as it was not designed to create project stacks in parallel
                            project_settings_obj[region]= {}
                        sleep(multiplier*30000).then(function () {
                            createTestRegion(function () {
                                util.log(util.colors.yellow("Project stack out parameters"))
                                util.log(project_settings_obj[region])
                                cb1()
                            }, region, project_settings_obj[region])
                        })
                        } else {
                            cb1()
                        }
                    },
                    function createRegionIndex(cb1) {
                        util.log("Region to test: \t\t\t " + region)
                        var filename = region + "_" + index
                        project_settings_obj[region].url = filename
                        util.log(util.colors.yellow("Saving the index.html with region '" + region + "' as " + filename))
                        util.log(util.colors.yellow("Project stack out parameters"))
                        util.log(project_settings_obj)
                        writeBootstrap(cb1, filename, region)
                    },
                    'test:webdriver',
                    //function (cb1) {
                    //    testIntegrationNoBootstrap(cb1, project_settings_obj[region].username, project_settings_obj[region].password, project_settings_obj[region].url )
                    //},
                    function (cb1) {
                        testIntegrationWelcomeModal(cb1, project_settings_obj[region].username, project_settings_obj[region].password, project_settings_obj[region].url)
                    },
                    //gulp.parallel(
                    //        testIntegrationUserAdministrator,
                    //        testIntegrationMessageOfTheDay
                    //    ),
                    function revertBootstrapAndProjectSettings(cb3) {
                            writeFile(index, bootstrap_file)
                            writeFile(project_settings_path, project_settings)
                            cb3()
                        }
                    //function deleteDeploymentStk(cb4) {
                    //    deleteDeploymentStack(cb4)
                    //},
                    //deleteProjectStack,
                )
            )
        }
        console.log("Done...main event now")
        //create all the regions in parallel
        gulp.series(
            region_creation,
            killLocalHost,
            function (main_callback) {
                main_callback()
                region_all_cb()
        })()
    })


}));

/*
 * Generate an ID to append to user names so multiple browsers running in parallel don't use the same user name.
*/
var makeId = function() {
    var text = "";
    var possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    for (var i = 0; i < 5; i++)
        text += possible.charAt(Math.floor(Math.random() * possible.length));

    return text;
}

var runTestExit = function (original_file_content) {
    gulp.series('set:test-define-off')()
    writeFile(index, original_file_content);
}

var runTest = function (context, bootstrap_setting, cb, username, password, index_page, onerr) {
    util.log(process.cwd())
    var protractor_args = [
        '--params.user.username', "zzztestUser1" + makeId(),
        '--params.user.email', "cgp-integ-test" + makeId(),
        '--params.user.password', "Test01)!",
        '--params.user.newPassword', "Test02)@",
        '--params.admin.username', username ? username : "administrator",
        '--params.admin.password', password ? password : "T3mp1@34",
        '--params.url', index_page ? "http://localhost:3000/" + index_page : "http://localhost:3000/index.html"
    ]
    var source_file = "./e2e/*.e2e-spec.js"

    var original_file_content = fs.readFileSync(index_page ? index_page : index, 'utf-8').toString();

    if (bootstrap_setting) {
        for (var i = 0; i < bootstrap_setting.length; i++) {
            var setting = bootstrap_setting[i]
            util.log(util.colors.yellow(JSON.stringify(setting)))
            replaceFileContent(index_page ? index_page : index, setting.pattern, setting.value)
        }
    }

    if (context && context.specs) {
        source_file = context.specs
    }
    util.log("Running test with args: ")
    util.log("\t\tSource: \t\t" + source_file)
    util.log("\t\tArgs: \t\t\t\t" + JSON.stringify(protractor_args))
    util.log("\t\tWorking Directory: \t" + process.cwd())
    util.log("\t\tBootstrap: \t\t" + fs.readFileSync(index_page ? index_page : index, 'utf-8').toString().match(bootstrap_pattern))
    util.log("\t\Protractor Config Path: \t" +'./e2e/protractor.config.js')

    return gulp.src([source_file])
        .pipe(protractor({
            configFile: './e2e/protractor.config.js',
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
        runTestExit(original_file_content);
        util.log("The test '" + source_file + "' is done.");
        if (cb)
            cb()
    })
}
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
        util.log("\tVersion: " + argv.pkgversion)
        npmcommand = 'npm i ' + argv.package
        jspmcommand = 'jspm install ' + argv.repos + ":" + argv.package

        if (argv.pkgversion) {
            jspmcommand += "@" + argv.pkgversion
            npmcommand += "@" + argv.pkgversion
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
    enginePath()
    process.chdir(cgp_path)
    append(environment_file, /export const metricWhiteListedCloudGem = \[.*\]/g, "export const metricWhiteListedCloudGem = []")
    if (argv.builtByAmazon) {
	console.log("Whitelisting for metrics: " + getCloudGemFolderNames(true))
        replaceOrAppend(environment_file, /export const metricWhiteListedCloudGem = \[.*\]/g, "export const metricWhiteListedCloudGem = [" + getCloudGemFolderNames(true) + "]")
    }
    replaceOrAppend(environment_file, /export const metricWhiteListedFeature = \[.*\]/g, "export const metricWhiteListedFeature = ['Cloud Gems', 'Support', 'Analytics', 'Admin', 'User Administration']")
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

function getCloudGemFolderNames(addquotes, asrelative) {
    var relativePathPrefix = path.join(path.join(path.join("..", ".."), ".."), "..");
    var folders = getFolders(relativePathPrefix)
    var cloudGems = []
    folders.map(function (folder) {
        if (folder.toLowerCase().startsWith("cloudgem")) {
            if (addquotes)
                cloudGems.push("\"" + folder + "\"")
            else if (asrelative)
                cloudGems.push(path.join(relativePathPrefix, folder))
            else
                cloudGems.push(folder)
        }
    })
    return cloudGems
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

function append(path, regex, content) {
    var file_content = fs.readFileSync(path, 'utf-8').toString();
    var match = file_content.match(regex)
    if (!match) {
        file_content = file_content + '\r\n' + content
        writeFile(path, file_content)
    }
}


function replaceFileContent(path, regex, content, ouput_path) {
    var file_content = fs.readFileSync(path, 'utf-8').toString();
    var match = file_content.match(regex)
    if (match) {
        var result = file_content.replace(new RegExp(regex, 'g'), content)
        writeFile(ouput_path ? ouput_path : path, result)
    }
}

var startServer = function () {
    var config = fs.readFileSync('server.js', 'utf-8').toString();
    var port = config.match(/server.listen(.*)/i)[0]
    util.log(util.colors.yellow("Starting the local web server."))
    util.log(util.colors.magenta("Launch your browser and enter the url http://localhost:" + port.replace('server.listen(', '').replace(')', '')))
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
}

var writeBootstrapAndStart = function (cb) {
    util.log(util.colors.yellow("A project stack exists.  Creating the local bootstrap at path ", cgp_path + path.sep + index, "."))
    writeBootstrap(function (err) {
        startServer()
        cb(err)
    })
}

var writeBootstrap = function (cb, output_path, region){
    return executeLmbrAwsCommand('lmbr_aws cloud-gem-framework cloud-gem-portal --show-url-only --show-configuration' + (region ? ' --region-override ' + region : '' ) , function (stdout, stderr, err) {
        var bootstrap_information = stdout.match(bootstrap_pattern)
        //is there a bootstrap configuration present?
        if (bootstrap_information) {
            replaceFileContent(index, /var\s*bootstrap\s*=\s*{.*}/i, "var bootstrap = " + bootstrap_information[0], output_path)
        } else {
            util.log(util.colors.red("The attempt to automatically write the Cloud Gem Portal development bootstrap failed.  Please review the logs above."))
        }
        cb(err)
    })
}

var cgp_path = undefined
var enginePath = function () {
    if (!cgp_path) {
        cgp_path = process.cwd()
    }
    return cgp_path + path.sep + ".." + path.sep + ".." + path.sep + ".." + path.sep + ".." + path.sep + ".." + path.sep
}

var createDevelopmentStack = function (handler_callback, name) {
    if (!name)
        name = deploymentStackName()
    util.log(util.colors.yellow("Attempting to create a deploymment stack. Please wait...this could take up to 15 minutes."))
    executeLmbrAwsCommand('lmbr_aws deployment create  --confirm-aws-usage --confirm-security-change --deployment ' + name, function (stdout, stderr, err) {
        handler_callback(stdout, stderr, err)
    })
}

var createProjectStack = function (handler_callback, region) {
    if (!region)
        region = 'us-east-1'
    var stackName = projectStackName()
    util.log(util.colors.yellow("Attempting to create a project stack with name '" + stackName + "' and region '" + region + "'. Please wait...this could take up to 10 minutes."))
    executeLmbrAwsCommand('lmbr_aws project create --stack-name ' + stackName + ' --confirm-aws-usage --confirm-security-change --region ' + region, function (stdout, stderr, err) {
        var username = stdout.match(/Username:\s(\S*)/i)
        var password = stdout.match(/Password:\s(\S*)/i)
        if (username) {
            util.log(util.colors.blue("Your administrator account credentials are:"))
            util.log(util.colors.blue(username[0]))
            util.log(util.colors.blue(password[0]))
        }
        handler_callback(stdout, stderr, err, username, password)
    })
}

var deleteProjectStack = function (gulp_callback) {
    util.log(util.colors.yellow("Attempting to delete a project stack. Please wait...this could take up to 5 minutes."))
    executeLmbrAwsCommand('lmbr_aws project delete -D', function (response) {
        gulp_callback()
    })
}

var deleteDeploymentStack = function (handler_callback, stackname) {
    if (!stackname)
        stackname = 'Stack1'
    util.log(util.colors.yellow("Attempting to delete a deployment stack. Please wait...this could take up to 10 minutes."))
    executeLmbrAwsCommand('lmbr_aws deployment delete -D -d', function (response) {
        handler_callback()
    })
}

var readLocalProjectSettingsPath = function (handler_callback) {
    util.log(util.colors.yellow("Reading your current local_project_settings.json file."))
    executeLmbrAwsCommand('lmbr_aws cloud-gem-framework paths', function (response) {
        var regex = /Local Project Settings\s*(\S*)\s*/i
        var match = regex.exec(response)
        var path = match[1]
        util.log(util.colors.yellow("Found at location '"+path+"'"))
        handler_callback(path)
    })
}

var deploymentStackName = function (){
    return 'CGPDepStk'+ Date.now().toString().substring(5, Date.now().length)
}

var projectStackName = function () {
    return 'CGPProjStk' + Date.now().toString().substring(5, Date.now().length)
}

var executeLmbrAwsCommand = function (command, callback) {
    util.log("changing to " + enginePath() + " from " + process.cwd())
    process.chdir(enginePath())
    util.log(util.colors.yellow(command))
    exec_process(command, function (err, stdout, stderr) {
        util.log(util.colors.red("changing back to " + cgp_path))
        process.chdir(cgp_path)
        util.log(stdout);
        util.log(util.colors.red(stderr));
        callback(stdout, stderr, err)
    });
}


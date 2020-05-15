/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

namespace Projects
{
    constexpr const char* DefaultProjectTemplatesRoot = "ProjectTemplates";
    constexpr const char* DefaultProjectTemplate = "EmptyTemplate";
    constexpr const char* DefaultProjectTemplateDefinition = "templatedefinition.json";
    constexpr const char* DefaultProjectGemTemplateProperty = "GemTemplate";

    constexpr const char* ProjectSettingsFilename = "project.json";

    // External Projects Support
    constexpr const char* ExternalProjectsJsonFilename = "LyzardExternalProjects.json";
    constexpr const char* ProjectNameKey = "project_name";
    constexpr const char* ProjectIdKey = "project_id";
    constexpr const char* ProjectPathKey = "project_path";
    constexpr const char* ExternalProjectsArrayKey = "external_projects";

} // namespace Projects

#define DEFAULT_PROJECT_TEMPLATES_ROOT Projects::DefaultProjectTemplatesRoot
#define DEFAULT_PROJECT_TEMPLATE Projects::DefaultProjectTemplate

// Filename of a project template definition json config file
#define DEFAULT_PROJECT_TEMPLATE_DEFINITION Projects::DefaultProjectTemplateDefinition

/*
 * In templatedefinition.json:
 *
 * {
 * "TemplateDefinitionVersion": 1,
 * "Name": "Default",
 * "Description":  "...",
 * "IconPath": "${ProjectName}/preview.png",
 * "GemTemplate": "default_project_gem_template"  // <--- DEFAULT_PROJECT_GEM_TEMPLATE_DEFINITION_PROPERTY
 * }
 */
#define DEFAULT_PROJECT_GEM_TEMPLATE_DEFINITION_PROPERTY Projects::DefaultProjectGemTemplateProperty


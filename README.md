Making a test change here!
![lmbr](https://s3-us-west-2.amazonaws.com/git-release/ReadmeResources/readme_header.jpg)

# Amazon Lumberyard
Amazon Lumberyard is a free AAA game engine that gives developers the tools they need to create the highest-quality games. Deeply integrated with the AWS Cloud and Twitch, Amazon Lumberyard also includes full source, allowing you to customize your project at any level.

For more information, visit: https://aws.amazon.com/lumberyard/

## Acquiring Lumberyard source
Each release of Lumberyard exists as a separate branch in GitHub. You can get Lumberyard from GitHub using the following steps:

### Fork the repository
Forking creates a copy of the Lumberyard repository in your GitHub account. Your fork becomes the remote repository into which you can push changes.

### Clone the repository
Cloning the repository will copy your fork onto your local computer. To clone the repository, click the "Clone or download" button in the GitHub web site, then copy the resultant URL to the clipboard. Then, from a command prompt, type '''git clone [URL]''', where [URL] is the URL you copied in the previous step.

See the GitHub documentation for more information about cloning a repository.

### Downloading additive files
Once the repository exists locally on your machine, manually execute ```git_bootstrap.exe``` found at the root of the repository. This application will perform a download operation for __Lumberyard binaries required prior to using or building the engine__. 

### Running SetupAssistant
```git_bootstrap.exe``` will launch the Setup Assistant when it completes. Setup Assistant lets you configure your environment and launch the Lumberyard editor.

## Contributing code to Lumberyard
You can submit changes/fixes to Lumberyard using pull requests. When you submit a pull request, our support team will be notified and will evaluate the code you submitted. You may be contacted to provide further detail or clarification while support evaluates your submitted code. 

See the GitHub documentation for more information on how to work with pull requests.

## Lumberyard Documentation
Full Lumberyard documentation can be found here:
https://aws.amazon.com/documentation/lumberyard/
We also have tutorials available at https://gamedev.amazon.com/forums/tutorials 

## License
For complete copyright and license terms please see the LICENSE.txt file at the root of this distribution (the "License").  As a reminder, here are some key pieces to keep in mind when submitting changes/fixes and creating your own forks:
-	If you submit a change/fix, we can use it without restriction, and other Lumberyard users can use it under the License. 
-	Only share forks in this GitHub repo.
-	Your forks are governed by the License, and you must include the License.txt file with your fork.  Please also add a note at the top explaining your modifications.
-	If you use someone else’s fork from this repo, your use is subject to the License.    
-	Your fork may not enable the use of third-party compute, storage or database services.  
-	It's fine to connect to third-party platform services like Steamworks, Apple GameCenter, console platform services, etc.  
To learn more, please see our FAQs https://aws.amazon.com/lumberyard/faq/#licensing. 


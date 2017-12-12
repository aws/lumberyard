The following locations need to be updated when adding a new version:

    {root}\Tools\build\waf-1.7.13\lmbrwaflib\lumberyard_sdks.py
    {root}\Tools\Python\python.cmd
    {root}\Tools\build\waf-1.7.13\lmbrwaflib\compile_rules_win_x64_host.py
    {root}\Tools\build\waf-1.7.13\lmbrwaflib\compile_rules_linux_x64_host.py
    {root}\Code\Tools\AzCodeGenerator\AZCodeGenerator\wscript

If a new version of Python is installed, the following packages should be installed in the new directory using pip install <packagename>

funcsigs
mock
pbr
requests
requests_aws4auth
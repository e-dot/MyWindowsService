# MyWindowsService (MWS)
Binary utility to easily deploy windows services - no coding WIN32 API (e.g. for node.js programs)

## What is a windows service?

A Windows Service is a program that starts automatically at boot. It can be stopped, paused and restarted via a Windows API (https://learn.microsoft.com/en-us/windows/win32/services/service-functions). Writing a service requires the use a C/C++ API (or .NET) : it may be hard for scripting developers to write such code. MWS is a very simple wrapper for any Windows Service, e.g. programs written in NodeJS, Python, PHP...

# Install

* Execute as administrator :
```
_setup.bat
```

You may also call `_setup.bat` via command line and provide parameters to automate the installation process:
```
CMD /C _setup.bat MYSERVICEPASSWORD MYSERVICELOGIN MYSERVICENAME MYSERVICELABEL
```

This batch script is registering the binary utility as a service with [sc.exe](https://learn.microsoft.com/en-us/windows/win32/services/controlling-a-service-using-sc) :
1. stop service (if running)
2. delete service (if already installed)
3. create service (register its binary utility)
4. configure service to start automatically at boot
5. configure service to restart twice in case of unexpected crash
6. start service

# FAQ

# How do I run my service under the system account ?

You need to provide a `NULL` login and password during setup, i.e. when calling `_setup.bat` :
```
CMD /C _setup.bat NULL NULL MYSERVICENAME MYSERVICELABEL
```

# How do I run my service under a specific account ?

You need to provide this specific login and password during setup, i.e. when calling `_setup.bat` :
```
CMD /C _setup.bat MYSERVICEPASSWORD MYSERVICELOGIN MYSERVICENAME MYSERVICELABEL
```

# References

* [Service Functions](https://learn.microsoft.com/en-us/windows/win32/services/service-functions)
* [Controlling a service using sc](https://learn.microsoft.com/en-us/windows/win32/services/controlling-a-service-using-sc)
* [sc commands](https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2012-r2-and-2012/cc754599(v=ws.11))
* [sc.exe query](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/sc-query)
* [sc.exe create](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/sc-create)
* [sc.exe config](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/sc-config)
* [se.exe start](https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2012-r2-and-2012/cc742126(v=ws.11))
* [se.exe stop](https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2012-r2-and-2012/cc742107(v=ws.11))

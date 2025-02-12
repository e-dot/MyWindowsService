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

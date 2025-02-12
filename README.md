# MyWindowsService (MWS)
Binary utility to easily deploy windows services - no coding WIN32 API (e.g. for node.js programs)

# Install

Execute as administrator :
_setup.bat

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

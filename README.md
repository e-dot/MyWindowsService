# MyWindowsService (MWS)
Binary utility to deploy windows services without coding WIN32 API (e.g. for node.js programs)

# Install

Execute as administrator :
MWS_setup.bat

# FAQ

# How do I run my service under the system account ?

You need to provide a `NULL` login and password during setup, i.e. when calling `MWS_setup.bat` :
```
CMD /C MWS_setup.bat NULL NULL MYSERVICENAME MYSERVICELABEL
```

# How do I run my service under a specific account ?

You need to provide this specific login and password during setup, i.e. when calling `MWS_setup.bat` :
```
CMD /C MWS_setup.bat MYSERVICEPASSWORD MYSERVICELOGIN MYSERVICENAME MYSERVICELABEL
```

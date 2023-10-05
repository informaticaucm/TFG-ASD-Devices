deactivate the active proving on windows
add a registry value to the following key: Computer\HKEY_LOCAL_MACHINE\SOFTWARE\Policies\Microsoft\Windows\NetworkConnectivityStatusIndicator:
    Name: NoActiveProbe
    Type: DWORD (32bit)
    Value: 1

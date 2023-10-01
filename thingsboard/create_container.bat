docker run -it -p 9090:9090 -p 1883:1883 -p 7070:7070 -p 5683-5688:5683-5688/udp -v C:/Users/Jaime/Escritorio/sharedProjects/4to/TFG/TFG-ASD-Devices/thingsboard/.mytb-data:/data -v C:/Users/Jaime/Escritorio/sharedProjects/4to/TFG/TFG-ASD-Devices/thingsboard/.mytb-logs:/var/log/thingsboard --name mytb --restart always thingsboard/tb-postgres


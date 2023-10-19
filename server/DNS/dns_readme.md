1. Stop the Host Network Service service // disable it or will come back on
2. Stop the Internet Connection Sharing service // disable it or will come back on
    * netsh wlan stop hostednetwork 
    * netsh wlan set hostednetwork mode= disallow 
3. __Start your service/process that uses port 53__
4. Start the Host Network Service service


netstat -ano | findstr :53
taskkill /PID <PID> /F 
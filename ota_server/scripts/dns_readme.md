    Stop the Host Network Service service // disable it or will come back on
    Stop the Internet Connection Sharing service
        netsh wlan stop hostednetwork 
        netsh wlan set hostednetwork mode= disallow 
    Start your service/process that uses port 53
    Start the Host Network Service service

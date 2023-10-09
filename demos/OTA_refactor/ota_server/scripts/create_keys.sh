openssl req -x509 -newkey rsa:2048 -keyout ../ca_key.pem -out ../ca_cert.pem -days 365 -nodes

cp ../ca_cert.pem ../../common/server_certs/

# When prompted for the Common Name (CN), enter the name of the server that the 
# "ESP-Dev-Board" will connect to. When running this example from a development 
# machine, this is probably the IP address. The HTTPS client will check that 
# the CN matches the address given in the HTTPS URL.

# cn: 192.168.43.209
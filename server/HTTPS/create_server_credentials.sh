mkdir ../keys/https

# create private key
openssl genrsa -out ../keys/https/server_key.pem 2048 

# create csr

openssl req -new -key ../keys/https/server_key.pem -out ../keys/https/server_request.csr

# sign csr
mkcert -csr ../keys/https/server_request.csr

# cn: otaserver
cp ./otaserver.asd.pem ../keys/https/server_cert.pem
rm ./otaserver.asd.pem 
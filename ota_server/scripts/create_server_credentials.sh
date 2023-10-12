mkdir ../keys/server

# create private key
openssl genrsa -out ../keys/server/server_key.pem 2048 

# create csr

openssl req -new -key ../keys/server/server_key.pem -out ../keys/server/server_request.csr

# sign csr
mkcert -csr ../keys/server/server_request.csr

# cn: otaserver
cp ./otaserver.pem ../keys/server/server_cert.pem
rm ./otaserver.pem 
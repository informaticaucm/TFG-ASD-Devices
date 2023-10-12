mkdir ../keys/server

# create private key
openssl genrsa -out ../keys/server/server_key.pem 2048 

# create csr

openssl req -new -key ../keys/server/server_key.pem -out ../keys/server/server_request.csr

# sign csr
mkcert -csr ../keys/server/server_request.csr

# cn: 192.168.43.209
cp ./192.168.43.209.pem ../keys/server/server_cert.pem
rm ./192.168.43.209.pem 
mkcert -install

rm -rf ../keys/CA
cp -r $(mkcert -CAROOT) ../keys/CA
rm -rf ../../binary_source_code/main/server_certs/
mkdir ../../binary_source_code/main/server_certs/
cp -r $(mkcert -CAROOT)/rootCA-key.pem ../../binary_source_code/main/server_certs/ca_cert.pem

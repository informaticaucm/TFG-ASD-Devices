mkcert -install

rm -rf ../keys/CA
cp -r $(mkcert -CAROOT) ../keys/CA
cp -r $(mkcert -CAROOT)/rootCA-key.pem ../../common/server_certs/ca_cert.pem
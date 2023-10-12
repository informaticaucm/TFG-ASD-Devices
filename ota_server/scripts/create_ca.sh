mkcert -install

rm -rf ../keys/CA
cp -r $(mkcert -CAROOT) ../keys/CA
rm -rf ../../demos/OTA/common/server_certs/
mkdir ../../demos/OTA/common/server_certs/
cp -r $(mkcert -CAROOT)/rootCA-key.pem ../../demos/OTA/common/server_certs/ca_cert.pem
rm -rf ../../demos/pinger/main/server_certs/
mkdir ../../demos/pinger/main/server_certs/
cp -r $(mkcert -CAROOT)/rootCA-key.pem ../../demos/pinger/main/server_certs/ca_cert.pem
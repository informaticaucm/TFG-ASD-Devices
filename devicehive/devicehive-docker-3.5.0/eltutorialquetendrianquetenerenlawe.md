el content-type tiene que ser application/json
los parametros del body van en el json

ejemplo para lo descrito [aqui](https://docs.devicehive.com/docs/login)
```
curl --location 'localhost/auth/rest/token' \
--header 'Content-Type: application/json' \
--data '{"login":"kkkk","password":"kkkkkk"}'
```

ahora que tienes el churro web access piticlin puedes proceder a [esto](https://docs.devicehive.com/docs/register) asi:

```
curl --location --request PUT 'localhost/api/rest/device/001' \
--header 'Content-Type: application/json' \
--header 'Authorization: Bearer eyJhbGciOiJIUzI1NiJ9.eyJwYXlsb2FkIjp7ImEiOlsyLDMsNCw1LDYsNyw4LDksMTAsMTEsMTUsMTYsMTddLCJlIjoxNjk2Njg5OTk0OTU1LCJ0IjoxLCJ1IjoyLCJuIjpbIjIiXSwiZHQiOlsiKiJdfX0.LciJ9Wgf7LTqKRRybomGQjlqbwUVBEyZl3a1vIUgHgo' \
--data '{name:"dev001","networkid":1,"isBlocked":false}'
```

/!\ ten cuidado a que network lo metes pillín
* esto tambien se puede hacer fresquisimamente desde el /admin/admin/devices, pero te quedas sin saber el id

Y ya lo ultimo, las (notificaciones)[https://docs.devicehive.com/docs/poll], usando el mismo id en la url manda:

curl --location 'localhost/api/rest/device/001/command/poll?waitTimeout=0' \
--header 'Authorization: Bearer eyJhbGciOiJIUzI1NiJ9.eyJwYXlsb2FkIjp7ImEiOlsyLDMsNCw1LDYsNyw4LDksMTAsMTEsMTUsMTYsMTddLCJlIjoxNjk2Njg5OTk0OTU1LCJ0IjoxLCJ1IjoyLCJuIjpbIjIiXSwiZHQiOlsiKiJdfX0.LciJ9Wgf7LTqKRRybomGQjlqbwUVBEyZl3a1vIUgHgo'

y te llega:

[
    {
        "id": 1646552271,
        "command": "fasfddasf",
        "timestamp": "2023-10-07T14:32:19.729",
        "lastUpdated": "2023-10-07T14:32:19.729",
        "userId": 1,
        "deviceId": "001",
        "networkId": 2,
        "deviceTypeId": 1,
        "parameters": {},
        "lifetime": null,
        "status": null,
        "result": null
    }
]


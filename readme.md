# Automatización de Seguimiento Docente (Hardware)

## Hitos del primer sprint 9-10-2023

### Selección de hub de IOT
Se han considerado e investigado 2 posibilidaded
* Things Board
* Device Hive + Grafana

Más detalles son necesarios para la decisión
### Selección de framework
Entre Arduino y Expressif he elegido el segund
### Prototipado de OTA
El prototipo no funciona con https y usa pooling para detectar la necesidad de un OTA. 
### Control de la cámara
Se ha comprobado que las librerías funcionan. El desarrollo de código personalizado para las funciones del asd se hará más adelante.
### HTTP ping pong
Se ha experimentado con la forma de extraer la MAC del equipo para identificarse ante un servidor y experimentado con peticiones POST.

## Objetivos del segundo sprint 
* Integración de notificaciones mqtt en el proceso de OTA, usando el broker del hub IOT que se decida usar en adelante.
* Esbozar el enrolment de máquinas recién falseadas usando el hub IOT
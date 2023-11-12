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

## Hitos del segundo sprint 23-10-2023
Se ha selecionado ThingsBoard
Se ha diseñado la red, con DNS y DHCP
Se han resuelto las comunicaciones SMQTT con thingsboard
Se ha completado el primer prototipo completo de OTA
Se han comenzado los intentos de lectura de QR con el 

## Objetivos del tercer sprint
Aprovisionamiento
## Hitos del tercer sprint 06-11-2023
- Se ha diseñado el sistema de tasks para permitir el aprovisionamiento
	- el razonamiento es que las tasks se hablen entre ellas, de esa forma la task de qr se puede hablar con la de aprovisionamiento o la de telecomunicaciones segun lea un qr o otro
- Se ha solucionado el prblema de la memoria moviendo los staks a la psram
- Se han movido las alocaciones de la libreria de qr a la psram
- Se han movido algunas las alocaciones de la libreria vlgl port a la psram
- Se ha implementado un proceso de gestion gráfica básico, que muestra mensajes y el progreso del ota 
- Se ha implementado la lectura de qr, que se usará para el aprovisionamiento y lectura de las tui
	- De momento los qr se muestran en la pantalla, como demo
- Se ha modificado el sistema MQTT para iniciarse tras un comando desde el starter (responsable de guardar las credenciales y de decodificar el mensaje de Aprovisionamiento)

# Cheat sheat

cuando quitas y pones un firmware desde thingsboard a un perfil, un device con ese perfil recive esto
- quitar:
'''
I (26415) mtqq_plugin: MQTT_EVENT_DATA
TOPIC=v1/devices/me/attributes
DATA={"deleted":["fw_checksum_algorithm","fw_version","fw_ts","fw_tag","fw_checksum","fw_title","fw_state","fw_size","fw_url"]}
'''
- poner:
'''
I (56855) mtqq_plugin: MQTT_EVENT_DATA
TOPIC=v1/devices/me/attributes
DATA={"deleted":["fw_checksum_algorithm","fw_checksum","fw_size"]}

I (56915) mtqq_plugin: MQTT_EVENT_DATA
TOPIC=v1/devices/me/attributes
DATA={"fw_title":"first ota","fw_version":"v1","fw_tag":"first ota v1","fw_url":"otaserver.asd/payload.bin"}
'''
# Automatización de seguimiento docente
En este repositorio se encuentra el codigo fuente en relación al nodo IoT del TFG Automatización de seguimiento docente.

Los proyectos que generan la imagen binaria para el ESP32 principal y el ESP32 auxiliar se encuentran en source y rfid source respectivamente.
El resto de carpetas contienen experimentos y codigo para crear un entorno de pruebas. 

Para más información sobre el proyecto, consultar la [memoria del TFG](https://www.overleaf.com/read/nkvdnsnwcnfc#e1403b) en overleaf.

## Contenidos del repositorio

- __/source__ y __/rfid_source__ son los proyectos de ESP-IDF que generan los binarios para los 2 microcontroladores del Nodo IoT.
- __/dev_server__ y __/dev_thingsboard__ guardan los archivos necesarios para lanzar el entrorno de desarroyo. Trás desplegar el entorno en produción no se continuaron manteniendo estos repositorios.
- __/demos__ es un registro de los proyectos de ESP-IDF con los que experimenté al principio del TFG para familiarizarme con cada funcionalidad del hardware por separado.
- __/blueprints__ contiene imágenes explicativas de la primera iteración del diseño físico. 
- __/provisioning_app__ es el proyecto que genera la web estática de reprogramación de dispositivos.
- __/rtc_test__ código de evaluación para la precisión del reloj de tiempo real para microcontroladore ESP-IDF junto a los resultados de el mismo experimento para un ESP32-s3-eye.
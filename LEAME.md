# ESP32-CAM-car-server

Proyecto de vehiculo a control remoto mediante WIFI, si decide realizar este proyecto obtendrá un entretenido prototipo de juguete que concentra varias ramas tecnológicas, entre ellas electricidad, electrónica, programación de microcontroladores, informática, telecomunicaciones y servicios de internet.

Este repositorio incluye el código fuente que se instalará en el microcontrolador, en mi caso he utilizado el microcontrolador ESP32-CAM, sin embargo puede utilizar otros modelos (idealmente placas compatibles con el IDE de Arduino) ya que la mayoría del código esta escrito en leguage C, en caso qe desee usar otra placa debe considerar los cambios necesarios en la programación para el correcto funcionamiento.

[Click here for english readme](https://github.com/pablotoledom/ESP32-CAM-car-server/blob/main/README.md)

## Comenzando 🚀

Ejemplo del resultado que podría obtener:


Este es un entretenido proyecto para hacer usted mismo, con su familia o con amigos, aprenderá mucho sobre como funcionan tecnologías actuales, es increible como un proyecto tan pequeño lleva en su interior décadas de investigaciones y evolución tecnológica. El siguiente diagrama representa todo lo que digo:

![alt Diagrama](https://raw.githubusercontent.com/pablotoledom/ESP32-CAM-car-server/main/images/Remote_WIFI_Car_main_diagram_es.jpg)

### Pre-requisitos 📋

Considere contar con las siguientes herramientas:

- Soldador o cautín
- Alicate cortante, pela cables
- Cuchillo o cortador
- Destornillador de precisión (cruz)
- Pistola de silicona caliente o fria para pegar las placas en el chasis
- Estaño
- Cinta aislante
- Pegamento acrílico para pegar las ruedas (o cualquier pegamento que sea muy firme)

Materiales:

- Kit de vehiculo de dos motores (o pueden ser 4), en caso de no disponer del kit, puede armarlo usted mismo usando motores DC de 5v y agregando ruedas que le acomoden.
- Puente H doble (), en caso de no disponer de puente H, puede crearlo usted mismo mediante transistores de potencia o relés.
- Micro controlador ESP32-CAM
- Switch de encendido
- Piezo Buzzer activo, puede usar un Piezo Buzzer pasivo pero debe contemplar cambiar la salida digital GPIO 12 a PWM, o tambien puede agregar un generador de pulsos mediante un circuito de carga y descarga o o bien un circuito LM555
- Cables hembra para conexiones de pines (mínimo 20 cables)
- 2 Baterias 18650 3.7v sobre 5000mAh
- Porta baterias 18650
- Cargador de baterias 18650
- Antena WIFI 3dbi mínimo (opcional) el módulo ESP32-CAM incluye una antena, es débil pero puede servir de igual forma si no pretende alejarse del vehículo o del punto de acceso WIFI, en lo personal yo si recomiendo agregar la antena

Computador

- Adaptador UART, USB a Serial TTL
- Computador que permita ejecutar Arduino IDE
- Controladores del adaptador USB a Serial
- Instalar esptool.py

### Armado del circuito 🔧

El siguiente diagrama electrónico muestra las conexiones que debe realizar entre los difrentes componentes:

![alt Diagrama Electrico](https://raw.githubusercontent.com/pablotoledom/ESP32-CAM-car-server/main/images/electric_main_diagram.jpg | width=100)

### Instalación del software 🔧

#### 1) clonar el proyecto desde Github

Ejecuta el siguiente comando en tu consola

```console
git clone https://github.com/pablotoledom/ESP32-CAM-car-server.git
```

#### 2) Abrir el proyecto con el IDE de Arduino

Una vez que abra el proyecto en el IDE de Arduino, verá tres archivos principales:

- ESP32-CAM-car-server.ino   Archivo principal del proyecto, incluye los métodos que inicializan al microcontrolador y manejan el hardware WIFI.
- app_httpd.cpp   Archivo del servidor web, este archivo es el encargado de levantar el sitio web, compartir en tiempo real la cámara y de recibir los comando http para controlar los movimientos del vehículo.
- web_index.h   Archivo del sitio web HTML, este es una copia del archivo frontend.html, solo que se encuentra comprimido por gzip y declarado el binario dentro de un archivo de cabecera en exadecimal.


#### 3) Agregar dependencia

La dependencia de Espressif agrega compatibilidad del modulo ESP32 al IDE de Arduino, este software permite que el código fuente escrito en lenguaje C pueda ser compilado correctamente para el microcontrolador, si no instala esta dependencia no podrá cargar el programa dentro del hardware.

Primero bara la configuración del IDE de Arduino y agregue el siguiente texto:

https://dl.espressif.com/dl/package_esp32_index.json: con esta dirección el gestor de placas tendrá acceso a un conjunto elevado de placas y módulos ESP32 de varios fabricantes.
https://resource.heltec.cn/download/package_heltec_esp32_index.json: con esta otra el gestor de placas tendrá acceso a las placas de desarrollo ESP32 comercializadas por Heltec:

![alt Agregar dependencia](https://raw.githubusercontent.com/pablotoledom/ESP32-CAM-car-server/main/images/0_instalar_espressif.png)

Luego de agregar la ruta, ahora debe dirigirse al gestor de tarjetas y buscar la dependencia ESP32:

![alt Preferencias Arduino](https://raw.githubusercontent.com/pablotoledom/ESP32-CAM-car-server/main/images/1_preferencias.png)

![alt Diagrama Gestor tarjetas](https://raw.githubusercontent.com/pablotoledom/ESP32-CAM-car-server/main/images/2_gestor_tarjetas.png)

Una vez encontrado instale la dependencia:

![alt Diagrama Instalar Dependencia](https://raw.githubusercontent.com/pablotoledom/ESP32-CAM-car-server/main/images/3_esp32.png)

## Despliegue 📦

Para desplegar el proyecto primero debe compilarlo para verificar que se instalará correctamente en el micro-controlador ESP32-CAM, para ello presione el botón "Verificar"

![alt Compilar](https://raw.githubusercontent.com/pablotoledom/ESP32-CAM-car-server/main/images/4_compile.png)

Si la consola se muestra limpia tal como en la imagen anterior, ya puede cargar el programa en el micro-controlador, para ello debe conectar la UART del ESP32 a su computadora, la forma más comun es usar un adaptador USB a Serial.

![alt Conexion UART](https://raw.githubusercontent.com/pablotoledom/ESP32-CAM-car-server/main/images/esp32_uart.png)

Una vez conectado a el adaptador USB a Serial a su computadora debe configurar los parámetros de comunicación en el IDE de Arduino:

![alt Configuracion Arduino IDE](https://raw.githubusercontent.com/pablotoledom/ESP32-CAM-car-server/main/images/5_arduino_configuracion.png)

Luego que configure la comunicaión entre el IDE y la UART del ESP32, ya puede subir el código a su micro-controlador, simplemente presione el boton "Subir Usando Programador" y también debe presionar el único botón que posee la placa ESP32, esto habilitará el modo de escritura.

Una vez cargado el programa ya puede quitar el puente entre IO0 y GND. Y reiniciar el ESP32.


## Conectarse al Vehículo 🎮

El vehículo levantará una red WIFI abierta llamada "Remote WIFI Car"

![alt Connectar WIFI](https://raw.githubusercontent.com/pablotoledom/ESP32-CAM-car-server/main/images/6_connect_wifi.png)

Una vez que se conecte a dicha red, deberá ingresar a la siguiente dirección IP en su navegador de internet (ya sea teléfono, tablet o computadora): http://192.168.4.1 y verá el siguiente sitio de control:

![alt Ingresar IP](https://raw.githubusercontent.com/pablotoledom/ESP32-CAM-car-server/main/images/7_enter_ip.png)

Una vez dentro del sitio web del vehículo, podra controlarlo directamente desde aqui o también podrá vincularlo a una red WIFI. En caso de conectarlo a una red WIFI, considere que deberá localizar manualmente la IP que le ha sido asignada (ya sea ingresando al router o usando comandos de red) para poder controlar el vehículo.

![alt Configurar WIFI](https://raw.githubusercontent.com/pablotoledom/ESP32-CAM-car-server/main/images/8_configure_wifi.png)

## Autor

Pablo Toledo


## Licencia 📄

Este proyecto está bajo la Licencia Apache, Versión 2.0 - mira el archivo [LICENCIA.md](LICENSE.md) para detalles.

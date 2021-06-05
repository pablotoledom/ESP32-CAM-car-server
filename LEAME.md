# ESP32-CAM-car-server

Proyecto de vehiculo a control remoto mediante WIFI, si deide continuar con este proyecto obtendrá un entretenido prototipo de juguete que concentra varias ramas tecnológicas, entre ellas electricidad, electrónica, programación de microcontroladores, informática, telecomunicaciones y servicios de internet.

Este repositorio incluye el código fuente que se instalará en el microcontrolador, en mi caso he utilizado el microcontrolador ESP32-CAM, sin embargo puede utilizar otros modelos (idealmente placas compatibles con el IDE de Arduino) ya que la mayoría del código esta escrito en leguage C, en caso qe desee usar otra placa debe considerar los cambios necesarios en la programación para el correcto funcionamiento.

[Click here for english readme](https://github.com/pablotoledom/ESP32-CAM-car-server/blob/main/README.md)

## Comenzando 🚀

Ejemplo del resultado que podría obtener:


Este es un entretenido proyecto para hacer usted mismo, con su familia o con amigos, aprenderá mucho sobre como funcionan tecnologías actuales, es increible como un proyecto tan pequeño lleva en su interior décadas de investigaciones y evolución tecnológica. El siguiente diagrama representa todo lo que digo:



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
- Buzer activo, puede usar un buzer pasivo pero debe contemplar cambiar la salida digital GPIO 12 a PWM
- Cables hembra para conexiones de pines (mínimo 20 cables)

Computador

- Adaptador UART, USB a Serial TTL
- Computador que permita ejecutar Arduino IDE
- Controladores del adaptador USB a Serial
- Instalar esptool.py

### Armado del circuito 🔧

El siguiente diagrama electrónico muestra las conexiones que debe realizar entre los difrentes componentes:



### Instalación del software 🔧

#### 1) clonar el proyecto desde Github

Ejecuta el siguiente comando en tu consola

```console
git clone https://github.com/pablotoledom/ESP32-CAM-car-server.git
```

#### 2) Abrir el proyecto con el IDE de Arduino

Una vez que abra el proyecto en el IDE de Arduino, verá tres archivos principales:

- ESP32-CAM-car-server.ino   Archivo principal del proyecto, incluye los methodos que inicializan al microcontrolador y manejan el hardware WIFI.
- app_httpd.cpp   Archivo del servidor web, este archivo es el encargado de levantar el sitio web, compartir en tiempo real la cámara y de recibir los comando http para controlar los movimientos del vehículo.
- web_index.h   Archivo del sitio web HTML, este es una copia del archivo frontend.html, solo que se encuentra comprimido por gzip y declarado el binario dentro de un archivo de cabecera en exadecimal.

#### 3) Agregar los datos de configuración




## Despliegue 📦


## Conectarse al Vehículo 📦

El vehículo levantará una red WIFI abierta llamada "Remote WIFI Car"

Una vez que se conecte a dicha red, deberá ingresar a la siguiente dirección IP en su navegador de internet (ya sea teléfono, tablet o computadora): http://192.168.4.1


Una vez dentro del sitio web del vehículo, podra controlarlo directamente aqui o también podrá vincularlo a una red WIFI. En caso de conectarlo a una red WIFI, considere que deberá localizar manualmente la IP que le ha sido asignada (ya sea ingresando al router o usando comandos de red) para poder controlar el vehículo.


## Autor

Pablo Toledo


## Licencia 📄

Este proyecto está bajo la Licencia Apache, Versión 2.0 - mira el archivo [LICENCIA.md](LICENSE.md) para detalles.
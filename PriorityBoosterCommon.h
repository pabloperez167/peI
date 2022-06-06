#pragma once

/*Este arachivo sera utilzado mas tarde por el cliente en modo usuario, dentro del archivo necesitamos definir dos cosas:
*	La estructura de datso que el driver espera de los clientes
*	El codigo de control para cambiar la propiedad de un hilo 
* Empezamos declarando la estructura que captura la info que el driver necesita para un cliente
*/
struct ThreadData {
	ULONG ThreadId;//necesitamos el ID unico del hilo, tipo UNLOG son hilos de 32bits enteros sin signos
	int Priority;//la prioridad objetivo
};

/*Definimos el codigo de control con el CLT_CODE macro que acepta parametros para crear el control code:
*	tipo de device: identifica el tipo de device
*	funcion : un nuero ascendente que indica una operacion es especifica
*	metodo : indica como los input y output buffers provenientes del cliente pasen al driver 
*	acceso : indica si esta operacion es del driver, con FILE_ANY_ACCESS y tratan con la solicitud IRP_DEVICE_CONTROL
*/
#define PRIORITY_BOOSTER_DEVICE 0x8000

#define IOCTL_PRIORITY_BOOSTER_SET_PRIORITY CTL_CODE(PRIORITY_BOOSTER_DEVICE, \
	0x800, METHOD_NEITHER, FILE_ANY_ACCESS)

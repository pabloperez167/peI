#include <ntifs.h>
#include <ntddk.h>
#include "PriorityBoosterCommon.h"

/*
• Driver Initialization
• Client Code
• The Create and Close Dispatch Routines
• The DeviceIoControl Dispatch Routine*/

// prototypes

void PriorityBoosterUnload(_In_ PDRIVER_OBJECT DriverObject);// Rutina de descarga y lo apuntamos al objeto del driver
NTSTATUS PriorityBoosterCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);/*La funcion debe devolver NTStatus y acepta un puntero a un objeto del dispo
											y un puntero a una E/S(IRP)*/
																					 esto es el objeto principal donde se almacena la info de la solicitud*/
NTSTATUS PriorityBoosterDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
/*La funcion debe retornar NTSTATUS y aceptar un puntero a un objeto dispositivo y
	un puntero a I/O Request Packet (IRP),donde se almacena la informacion, para todos las solicitudes
*/

/* DriverEntry
* Establecer una rutina de descarga 
* establecer rutinas de mensaje que admita el soporte del driver
* crear un objeto del dipositivo
* crear un enlace simbolico al objeto del device
*/

//Inicializacion del driver
extern "C" NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);

	KdPrint(("PriorityBooster DriverEntry started\n"));
	//Inicializacion de nuestras rutinas de envio(dispatch)

	DriverObject->DriverUnload = PriorityBoosterUnload;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = PriorityBoosterCreateClose; /*Todos los drivers deben admitir IRP_MJ_CREATE y CLOSE, 
										 de lo contrario no habria forma de abrir un identificador
										 para cualquier dispo para este controlador */	
																			
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = PriorityBoosterCreateClose;
	//Pasar info al driver
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PriorityBoosterDeviceControl;/*DeviceIOControl es una forma flexible de comunicarse 
											    con un driver-> corresponde a la funcion mayor IRP_MJ_DEVICE_CONTROL*/

	//Nombre del dispositivo
	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\PriorityBooster");
	//RtlInitUnicodeString(&devName, L"\\Device\\ThreadBoost");
	PDEVICE_OBJECT DeviceObject;
	/*Creando el Objeto Device : IoCreateDevice argumentos:
	* DriverObject: el cual el objeto device pertenece
	* DeviceExtensionSize: extra bytes que estaran colocados en adicion al sizeof(DEVICE_OBJECT)
	* DeviceName
	* DeviceType
	* DeviceCharacteristics: un set de flags
	* Exclusive: deberia mas de un file object estar permitido para abrir en el mismo device? false
	* DeviceObject: retorna el puntero, 
	*/
	NTSTATUS status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create device (0x%08X)\n", status));
		return status;
	}
	//Si todo va bien, ahora tendremos un puntero hacia nuestro device object.
	//El siguiente paso es hacer nuestro driver mas accesible a las llmadas del user mode mediante un link simbolico 

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\PriorityBooster");
	status = IoCreateSymbolicLink(&symLink, &devName);//Acepta el link simbolico y el objetivo del link
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create symbolic link (0x%08X)\n", status));
		IoDeleteDevice(DeviceObject);// si la creacion falla debemos elminar el objeto device
		return status;
	}

	KdPrint(("PriorityBooster DriverEntry completed successfully\n"));

	return STATUS_SUCCESS;
}

//Una vez que el link simboico y el device object estan preparados, el DriverEntry puede devolver exito y el driver esta ahora listo para aceptar las solicitudes
//Antes de continuar debemos crear la rutina de descarga que debe deshacer todo lo hecho por DriverEntry:

void PriorityBoosterUnload(_In_ PDRIVER_OBJECT DriverObject) {
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\PriorityBooster");
	// delete symbolic link
	IoDeleteSymbolicLink(&symLink);

	// delete device object
	IoDeleteDevice(DriverObject->DeviceObject);

	KdPrint(("PriorityBooster unloaded\n"));
}


//Rutinas de dispatch (mensaje),los dispatch routines aceptan el objetivo device y el IRP  dispatch CREATE/CLOSE:
/*El propisito del driver es manejar el IRP
* Nosotros necesitamos establecer el estatus del IRP en el miembro IOStatus que tiene 2 miembros:
* status: indica el status en el que se completaran las solicitudes
* info
*/
_Use_decl_annotations_
NTSTATUS PriorityBoosterCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT); //Basicamente propaga el IRP de vuelta al creador y el manager notifica al cliente que la operacion ha sido completada
	return STATUS_SUCCESS;
}

/*Este es dispatch que hace el trabajo real de configurar un subproceso dado una prioridad solicitada.
* Lo primero que comprobar es el codigo de control
* 
*/
_Use_decl_annotations_
NTSTATUS PriorityBoosterDeviceControl(PDEVICE_OBJECT, PIRP Irp) {
	// get our IO_STACK_LOCATION
	auto stack = IoGetCurrentIrpStackLocation(Irp);//retrona un puntero al correcto IO_STACK_LOCATION
	auto status = STATUS_SUCCESS;

	switch (stack->Parameters.DeviceIoControl.IoControlCode) {//determina si nosotros comprendemos el control del codigo o no
		case IOCTL_PRIORITY_BOOSTER_SET_PRIORITY:
		{
			// do the work
			if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ThreadData)) {/*Comprobar si el buffer recibido es lo suficientemente grande 
													para contener ThreatData*/
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			auto data = (ThreadData*)stack->Parameters.DeviceIoControl.Type3InputBuffer; /* Asumimos que le buffer es lo suficientemente grande 
													para tratarlo como Threatdata*/
			if (data == nullptr) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			if (data->Priority < 1 || data->Priority > 31) { // Vemos si la prioridad es legal entre 1 y 31
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			//Transformamos nuestro ID del hilo en un puntero
			PETHREAD Thread;//puntero rsultante PETHREAD
			status = PsLookupThreadByThreadId(ULongToHandle(data->ThreadId), &Thread);//ULongToHandle macro provee el casteo necesario para el compilador funcione
			if (!NT_SUCCESS(status))
				break;

			KeSetPriorityThread((PKTHREAD)Thread, data->Priority);//llmamada para cambiar la prioridad
			ObDereferenceObject(Thread);//Decrementar la referencia del objeto hilo
			KdPrint(("Thread Priority change for %d to %d succeeded!\n",
				data->ThreadId, data->Priority));
			break;
		}

		default:
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;
	}

	//Completar el IRP, respuesta completa al cliente

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

/*Para instalar el driver:
1.Primero lo instalamos con sc.exe introducioendo: sc create booster type= kernel binPath= c:\Test\PriorityBooster.sys
2.Lo iniciamos con sc start booster
3.Tanto en Win ¡Obj como ProcessExplorer nos aparece el nombre del device y el symbolic link

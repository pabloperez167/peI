// Booster.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include "..\PriorityBooster\PriorityBoosterCommon.h" //incluimos el header comun creado por el driver y lo compartimos al codigo cliente

//En este punto escribimos el codigo cliente del user mode

int Error(const char* message) {
	printf("%s (error=%d)\n", message, GetLastError());
	return 1;
}
//Aceptaremos un ID del hilo y una prioridad
int main(int argc, const char* argv[]) {
	if (argc < 3) {
		printf("Usage: Booster <threadid> <priority>\n");
		return 0;
	}
	/*Abriremos un handle para nuestro device,
	* CreateFile alcanzara el driver en IRP_MJ_CREATE dispatch routine. 
	Tendremos un handle valido para nuestro device, sera momento de llamr al deviceControl pero
	primero tendremos que crear el ThreadData
	*/
	HANDLE hDevice = CreateFile(L"\\\\.\\PriorityBooster", GENERIC_WRITE, FILE_SHARE_WRITE,	
		nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE)
		return Error("Failed to open device");

	ThreadData data;
	data.ThreadId = atoi(argv[1]);
	data.Priority = atoi(argv[2]);

	DWORD returned;
	//											control code					input buffer and length, output buffer
	BOOL success = DeviceIoControl(hDevice, IOCTL_PRIORITY_BOOSTER_SET_PRIORITY, &data, sizeof(data), nullptr, 0, &returned, nullptr);
	if (success)
		printf("Priority change succeeded!\n");
	else
		Error("Priority change failed!");

	CloseHandle(hDevice);
}
//DeviceIoControl invoca la funcion mayor IRP_MJ_DEVICE_CONTROL, YA ESTARIA EL client code
//Quedaria implementar las rutinas de mensaje declaradas en el lado del driver


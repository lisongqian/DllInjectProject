#include "DllHook.h"
void WINAPI DllHook::SetHook()
{
	std::wstring szPath;
	LPSETHOOK lproc;
	HINSTANCE hDll;
	BOOL bRet;
	PROCESS_INFORMATION info;
	STARTUPINFO start;
	memset(&start, 0, sizeof(start));
	// ȡ��Ҫ�����DLL�ļ���
	szPath = _T("C://Users//lisongqian//source//repos//Ldll//x64//Debug//Ldll.dll");
	// ����DLL
	hDll = LoadLibraryW(szPath.c_str());
	if (hDll != NULL)
	{
		lproc = (LPSETHOOK)GetProcAddress(hDll, "SetHook");
		if (lproc != NULL)
		{
			// if ((*lproc)(0) == false) //ȫ������
			// {
			// std::wstring message = L"Sethook";
			// ShowError(GetLastError(), const_cast<LPTSTR>(message.c_str()));
			// }
			// ��Ϊû���ʵ��Ĺ��߿���ȡ���̣߳ɣģ�ҲΪ�˼���������������´�����һ�����±����̣��Ա�ȡ�������̣߳ɣģ����䰲װ���ӣ������ǵ�DLLע�뵽���±������С�
			std::wstring str = _T("C://Windows//System32//notepad.exe");
			bRet = CreateProcess(NULL,
				const_cast<wchar_t*>(str.c_str()),
				NULL,
				NULL,
				TRUE,
				0,
				NULL,
				NULL,
				&start,
				&info);
			if (bRet != 0)
			{
				if (((*lproc)(info.dwThreadId)) == false)
				{
					std::wstring message = L"Sethook";
					DllHook::ShowError(GetLastError(), const_cast<LPTSTR>(message.c_str()));
				}
			}
			else
			{
				std::wstring message = L"CreateProcess";
				DllHook::ShowError(GetLastError(), const_cast<LPTSTR>(message.c_str()));
			}
		}
	}
}
void WINAPI DllHook::UnSetHook()
{
	 std::wstring szPath;
	 LPSETHOOK lproc;
	 HINSTANCE hDll;

	 szPath = _T("C://Users//lisongqian//source//repos//Ldll//x64//Debug//Ldll.dll");
	 hDll = LoadLibrary(szPath.c_str());
	 if (hDll != NULL)
	 {
		 lproc = (LPSETHOOK)GetProcAddress(hDll, "SetHook");
		 if (lproc != NULL)
			 (*lproc)(0); //���ý��̺�0��Ϊ����
	 }
}
// DLLע�뺯��
BOOL WINAPI DllHook::LoadLib(DWORD dwProcessId, LPWSTR lpszLibName)
{
	HANDLE hProcess = NULL;
	HANDLE hThread = NULL;
	LPWSTR lpszRemoteFile = NULL;
	// ��Զ�̽���
	hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, dwProcessId);
	if (hProcess == NULL)
	{
		ShowError(GetLastError(), const_cast<LPTSTR>(L"OpenProcess"));
		return FALSE;
	}

	// ��Զ�̽����з������DLL�ļ����Ŀռ�
	lpszRemoteFile = (LPWSTR)VirtualAllocEx(hProcess, NULL, sizeof(WCHAR) * lstrlenW(lpszLibName) + 1, MEM_COMMIT, PAGE_READWRITE);
	if (lpszRemoteFile == NULL)
	{
		ShowError(GetLastError(), const_cast<LPTSTR>(L"VirtualAllocEx"));
		return FALSE;
	}
	// ����DLL�ļ�����Զ�̸շ���Ľ��̿ռ�
	if (!WriteProcessMemory(hProcess, lpszRemoteFile, (PVOID)lpszLibName, sizeof(WCHAR) * lstrlenW(lpszLibName) + 1, NULL))
	{
		ShowError(GetLastError(), const_cast<LPTSTR>(L"WriteProcessMemory"));
		return FALSE;
	}
	// ȡ��LoadLibrary������Kennel32.dll�еĵ�ַ
	PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(L"Kernel32.dll"), "LoadLibraryW");
	if (pfnThreadRtn == NULL)
	{
		ShowError(GetLastError(), const_cast<LPTSTR>(L"GetProcAddress"));
		return FALSE;
	}
	// ����Զ���߳� 
	hThread = CreateRemoteThread(hProcess,
		NULL,
		0,
		pfnThreadRtn, // LoadLibrary��ַ
		lpszRemoteFile, // Ҫ���ص�DLL��
		0,
		NULL);
	if (hThread == NULL)
	{
		ShowError(GetLastError(), const_cast<LPTSTR>(L"CreateRemoteThread"));
		return FALSE;
	}
	// �ȴ��̷߳��� 
	WaitForSingleObject(hThread, INFINITE);
	// �ͷŽ��̿ռ��е��ڴ�
	VirtualFreeEx(hProcess, lpszRemoteFile, 0, MEM_RELEASE);
	// �رվ�� 
	CloseHandle(hThread);
	CloseHandle(hProcess);
	return TRUE;
}
// �ڽ��̿ռ��ͷ�ע���DLL
BOOL WINAPI DllHook::FreeLib(DWORD dwProcessId, LPTSTR lpszLibName)
{
	HANDLE hProcess = NULL,
	       hThread = NULL,
	       hthSnapshot = NULL;
	MODULEENTRY32 hMod = {sizeof(hMod)};
	BOOL bFound = FALSE;
	// ȡ��ָ�����̵�����ģ��ӳ��
	hthSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,
	                                       dwProcessId);
	if (hthSnapshot == NULL)
	{
		ShowError(GetLastError(), const_cast<LPTSTR>(L"CreateToolhelp32Snapshot"));
		return FALSE;
	}
	// ȡ������ģ���б��е�ָ����ģ��
	BOOL bMoreMods = Module32First(hthSnapshot, &hMod);
	if (bMoreMods == FALSE)
	{
		ShowError(GetLastError(), const_cast<LPTSTR>(L"Module32First"));
		return FALSE;
	}
	// ѭ��ȡ����Ҫ��ģ��
	for (; bMoreMods; bMoreMods = Module32Next(hthSnapshot, &hMod))
	{
		//ShowMessage(String(hMod.szExePath) + " | " + String(lpszLibName));
		if ((wcscmp(hMod.szExePath, lpszLibName) == 0) ||
			(wcscmp(hMod.szModule, lpszLibName) == 0))
		{
			bFound = TRUE;
			break;
		}
	}
	if (!bFound)
	{
		MessageBox(nullptr, L"ģ�鲻����", L"", MB_OK);
		CloseHandle(hthSnapshot);
		return FALSE;
	}
	// �򿪽���
	hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE,
	                       FALSE, dwProcessId);
	if (hProcess == NULL)
	{
		ShowError(GetLastError(), const_cast<LPTSTR>(L"OpenProcess"));
		return FALSE;
	}
	// ȡ��FreeLibrary������Kernel32.dll�еĵ�ַ
	PTHREAD_START_ROUTINE pfnThreadRtn =
		(PTHREAD_START_ROUTINE)GetProcAddress(
			GetModuleHandle(L"Kernel32.dll"), "FreeLibrary");
	if (pfnThreadRtn == NULL)
	{
		ShowError(GetLastError(), const_cast<LPTSTR>(L"GetProcAddress"));
		return FALSE;
	}
	// ����Զ���߳���ִ��FreeLibrary����
	hThread = CreateRemoteThread(hProcess,
	                             NULL,
	                             0,
	                             pfnThreadRtn,
	                             hMod.modBaseAddr,
	                             0,
	                             NULL);
	if (hThread == NULL)
	{
		ShowError(GetLastError(), const_cast<LPTSTR>(L"CreateRemoteThread"));
		return FALSE;
	}
	// �ȴ��̷߳���
	WaitForSingleObject(hThread, INFINITE);
	// �رվ��
	CloseHandle(hThread);
	CloseHandle(hthSnapshot);
	CloseHandle(hProcess);
	return TRUE;
}
// ��ȡ����ID
DWORD DllHook::GetProcessId(LPCWSTR lpName, std::wstring& errMsg)
{
	DWORD dwPid = 0;
	HANDLE hProcess = NULL;
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;

	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		errMsg = L"Error: CreateToolhelp32Snapshot (of processes)";
		return 0;
	}

	// Set the size of the structure before using it.
	pe32.dwSize = sizeof(PROCESSENTRY32);
	// Retrieve information about the first process,
	// and exit if unsuccessful
	if (!Process32First(hProcessSnap, &pe32))
	{
		errMsg = L"Error: Process32First";
		//printf("Error: Process32First\r\n"); // show cause of failure
		CloseHandle(hProcessSnap); // clean the snapshot object
		return 0;
	}

	// Now walk the snapshot of processes, and
	// display information about each process in turn
	int namelen = 200;
	char name[201] = { 0 };
	do
	{
		if (!wcscmp(pe32.szExeFile, lpName))
		{
			dwPid = pe32.th32ProcessID;
			break;
		}
	} while (Process32Next(hProcessSnap, &pe32));
	CloseHandle(hProcessSnap);
	return dwPid;
}
// ��ʾ������Ϣ
void DllHook::ShowError(DWORD dwErrNo, LPTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwErrNo,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and exit the process

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(
			TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dwErrNo, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	//    ExitProcess(dwErrNo);
}
#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <psapi.h>
#include <vector>
#include <time.h>

MEMORY_BASIC_INFORMATION mbi;
HMODULE gmodule;

HANDLE GetProcessHandle() {
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (Process32First(snapshot, &entry) == TRUE) {
		while (Process32Next(snapshot, &entry) == TRUE) {
			if (_stricmp(entry.szExeFile, "SCPSL.exe") == 0) {
				CloseHandle(snapshot);
				return OpenProcess(PROCESS_ALL_ACCESS, 0, entry.th32ProcessID);
			}
		}
	}
	CloseHandle(snapshot);
	return 0;
}


MEMORY_BASIC_INFORMATION LastScan;

LPVOID PatternScanSingle(HANDLE handle, PUCHAR pattern, PCHAR mask) {
	LPVOID result = NULL;
	int pattern_length = strlen(mask);

	int pIndex = 0;
	MEMORY_BASIC_INFORMATION tmbi;
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	for (DWORD64 i = (DWORD64)si.lpMinimumApplicationAddress; i < (DWORD64)si.lpMaximumApplicationAddress;) {
		VirtualQueryEx(handle, (LPCVOID)i, &tmbi, sizeof(tmbi));
		LastScan = tmbi;

		if (tmbi.State == MEM_COMMIT && tmbi.Type == MEM_PRIVATE)
		{
			byte* tbuffer = new byte[tmbi.RegionSize];

			ReadProcessMemory(handle, tmbi.BaseAddress, tbuffer, tmbi.RegionSize, 0);
			for (int j = 0; j < tmbi.RegionSize; j++) {
				if (tbuffer[j] == pattern[pIndex] || mask[pIndex] == '?')
					pIndex++;
				else
					pIndex = 0;

				if (pIndex == pattern_length) {
					result = (LPVOID)((DWORD64)tmbi.BaseAddress + j - pIndex + 1);
					pIndex = 0;
					//printf("This region size is %i, %i\n", tmbi.RegionSize, tmbi.Type);
					break;
				}
			}
			delete[]tbuffer;
		}

		i += tmbi.RegionSize;
	}

	return result;
}


LPVOID PatternScanSingle(HANDLE handle, PUCHAR pattern, PCHAR mask, int rSize) {
	LPVOID result = NULL;
	int pattern_length = strlen(mask);

	int pIndex = 0;
	MEMORY_BASIC_INFORMATION tmbi;
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	for (DWORD64 i = (DWORD64)si.lpMinimumApplicationAddress; i < (DWORD64)si.lpMaximumApplicationAddress;) {
		VirtualQueryEx(handle, (LPCVOID)i, &tmbi, sizeof(tmbi));
		LastScan = tmbi;

		//if (tmbi.State == MEM_COMMIT && tmbi.Type == MEM_PRIVATE)
		if (tmbi.RegionSize == rSize)
		{
			byte* tbuffer = new byte[tmbi.RegionSize];

			ReadProcessMemory(handle, tmbi.BaseAddress, tbuffer, tmbi.RegionSize, 0);
			for (int j = 0; j < tmbi.RegionSize; j++) {
				if (tbuffer[j] == pattern[pIndex] || mask[pIndex] == '?')
					pIndex++;
				else
					pIndex = 0;

				if (pIndex == pattern_length) {
					result = (LPVOID)((DWORD64)tmbi.BaseAddress + j - pIndex + 1);
					pIndex = 0;
					//printf("This region size is %i, %i\n", tmbi.RegionSize, tmbi.Type);
					break;
				}
			}
			delete[]tbuffer;
		}

		i += tmbi.RegionSize;
	}

	return result;
}


LPVOID PatternScanSingle(HANDLE handle, PUCHAR pattern, PCHAR mask, DWORD64 start, DWORD64 end) {
	LPVOID result = NULL;
	int pattern_length = strlen(mask);

	int pIndex = 0;

	for (DWORD64 i = (DWORD64)LastScan.BaseAddress; i < end;) {
		//VirtualQueryEx(handle, (LPCVOID)i, &tmbi, sizeof(tmbi));

		byte* tbuffer = new byte[LastScan.RegionSize];

		ReadProcessMemory(handle, LastScan.BaseAddress, tbuffer, LastScan.RegionSize, 0);
		for (int j = 0; j < LastScan.RegionSize; j++) {
			if (tbuffer[j] == pattern[pIndex] || mask[pIndex] == '?')
				pIndex++;
			else
				pIndex = 0;

			if (pIndex == pattern_length) {
				result = (LPVOID)((DWORD64)LastScan.BaseAddress + j - pIndex + 1);
				pIndex = 0;
			}
		}
		delete[]tbuffer;

		// i += LastScan.RegionSize;
	}

	return result;
}


std::vector<LPVOID> PatternScan(HANDLE handle, LPVOID base, DWORD64 size, PUCHAR pattern, PCHAR mask) {
	std::vector<LPVOID> result;
	PBYTE buffer = new BYTE[size];
	int pattern_length = strlen(mask);

	if (!ReadProcessMemory(handle, base, buffer, size, nullptr))
		return result;

	int pIndex = 0;

	size -= pattern_length;

	for (int i = 0; i < size; i++) {
		if (buffer[i] == pattern[pIndex] || mask[pIndex] == '?')
			pIndex++;
		else
			pIndex = 0;

		if (pIndex == pattern_length) {
			result.push_back((LPVOID)((DWORD64)base + i - pIndex + 1));
			pIndex = 0;
		}
	}

	buffer = 0;

	if (result.size() > 0)
		printf("Pattern found! \n");

	return result;
}

bool PatchWallHack(HANDLE handle) {

	std::cout << std::endl << "Patching Wallhack .." << std::endl;

	UCHAR pattern[] = {
		/*// 00 00 07 00 00 00 15 02 00 00 00 00 00 00 00 00 00 00
		// new -> 00 00 42 00 00 00 66 02 00 00 00 00 00 00 00 00 00 00
		// new -> 00 00 0A 00 00 00 CA 02 00 00 00 00 00 00 00 00 00 00
		0x00,
		0x00,
		0x0A,
		0x00,
		0x00,
		0x00,
		0xCA,
		0x02,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,*/

		// 40 88 47 51 48 8B 46 10
		0x40,
		0x88,
		0x47,
		0x51,
		0x48,
		0x8B,
		0x46,
		0x10
	};
	//CHAR mask[] = "xxxxxxxxxxxxxxxxx";
	CHAR mask[] = "xxxxxxx";

	/*UCHAR patch[] = {
		// call r11
		// 0x55, 0x48, 0x8B, 0xEC, 0x56, 0x57, 0x41, 0x57, 0x48, 0x83, 0xEC, 0x38, 0x48, 0x8B, 0xF1, 0x48, 0xB8, 0xE0, 0xCE, 0xD6, 0x41, 0xFA, 0x01, 0x00, 0x00, 0x48, 0x8B, 0x00, 0x48, 0x8B, 0xC8, 0x48, 0x8B, 0xD6, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0x0E, 0xB9, 0x1C, 0x54, 0xFA, 0x01, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8B, 0x7E, 0x38, 0x45, 0x33, 0xFF, 0xE9, 0x39, 0x00, 0x00, 0x00, 0x66, 0x90, 0x49, 0x63, 0xC7, 0x39, 0x47, 0x18, 0x0F, 0x86, 0x9D, 0x01, 0x00, 0x00, 0x48, 0x8D, 0x44, 0xC7, 0x20, 0x48, 0x8B, 0x00, 0x48, 0x8B, 0xC8, 0xBA, 0x01, 0x00, 0x00, 0x00, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0x8A, 0xA0, 0x2A, 0x42, 0xFA, 0x01, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x41, 0xFF, 0xC7, 0x48, 0x63, 0x47, 0x18, 0x44, 0x3B, 0xF8, 0x7C, 0xC0, 0x48, 0x8B, 0x46, 0x48, 0x48, 0x8B, 0xC8, 0xBA, 0x03, 0x00, 0x00, 0x00, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0x00, 0xB9, 0x1C, 0x54, 0xFA, 0x01, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8B, 0x46, 0x48, 0x48, 0x89, 0x45, 0xB0, 0x48, 0x8D, 0x46, 0x58, 0x48, 0x63, 0x00, 0x89, 0x45, 0xD8, 0x48, 0x83, 0xEC, 0x08, 0x48, 0x83, 0xEC, 0x08, 0x48, 0x63, 0x45, 0xD8, 0x89, 0x04, 0x24, 0x48, 0x83, 0xEC, 0x20, 0x49, 0xBB, 0xCC, 0x00, 0x1C, 0x54, 0xFA, 0x01, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x30, 0x48, 0x8B, 0xD0, 0x48, 0x8B, 0x45, 0xB0, 0x48, 0x8B, 0xC8, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0xF2, 0xB8, 0x1C, 0x54, 0xFA, 0x01, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8B, 0x46, 0x30, 0x48, 0x8B, 0xC8, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0xD7, 0x9D, 0x29, 0x42, 0xFA, 0x01, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8B, 0xC8, 0xBA, 0x01, 0x00, 0x00, 0x00, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0x81, 0xC9, 0x32, 0x42, 0xFA, 0x01, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8B, 0x46, 0x30, 0x48, 0x89, 0x45, 0xB8, 0x48, 0x8B, 0x46, 0x48, 0x48, 0x8B, 0xC8, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0xFE, 0xF9, 0x19, 0x54, 0xFA, 0x01, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0xF3, 0x0F, 0x5A, 0xC0, 0x48, 0x8B, 0x45, 0xB8, 0x48, 0x8B, 0xC8, 0xF2, 0x0F, 0x10, 0xC8, 0xF2, 0x0F, 0x5A, 0xC9, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0xE4, 0xB8, 0x1C, 0x54, 0xFA, 0x01, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8B, 0x46, 0x30, 0x48, 0x89, 0x45, 0xC0, 0x48, 0x8B, 0x46, 0x48, 0x48, 0x8B, 0xC8, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0x84, 0xBA, 0x32, 0x42, 0xFA, 0x01, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0xF3, 0x0F, 0x5A, 0xC0, 0x48, 0x8B, 0x45, 0xC0, 0x48, 0x8B, 0xC8, 0xF2, 0x0F, 0x10, 0xC8, 0xF2, 0x0F, 0x5A, 0xC9, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0xD6, 0xB8, 0x1C, 0x54, 0xFA, 0x01, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8D, 0x65, 0xE8, 0x41, 0x5F, 0x5F, 0x5E, 0xC9, 0xC3, 0xBA, 0xBA, 0x01, 0x00, 0x00, 0xB9, 0x43, 0x01, 0x00, 0x02, 0x49, 0xBB, 0xD0, 0x1C, 0xC4, 0x41, 0xFA, 0x01, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x00, 0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x00, 0x14, 0x02, 0x00, 0x00, 0x01, 0x04, 0x03, 0x05, 0x00, 0x00, 0x04, 0x53, 0x01, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x25, 0x47, 0xA7, 0xED, 0x08, 0x78, 0xC2, 0xD0, 0x49, 0xFA, 0x01, 0x00, 0x00, 0xE8, 0x17, 0x47, 0xA7, 0xED, 0x08, 0xE8, 0xC2, 0xD0, 0x49, 0xFA, 0x01, 0x00, 0x00, 0xE8, 0x09, 0x47, 0xA7, 0xED, 0x08, 0x60, 0xC9, 0xD0, 0x49, 0xFA, 0x01, 0x00, 0x00, 0xE8, 0xFB, 0x46, 0xA7, 0xED, 0x08, 0x58, 0xC3, 0xD0, 0x49, 0xFA, 0x01, 0x00, 0x00, 0xE8, 0xED, 0x46, 0xA7, 0xED, 0x08, 0xF0, 0xE7, 0xDB, 0x4A, 0xFA, 0x01
		// 0x55, 0x48, 0x8B, 0xEC, 0x56, 0x57, 0x41, 0x57, 0x48, 0x83, 0xEC, 0x38, 0x48, 0x8B, 0xF1, 0x48, 0xB8, 0xE0, 0xCE, 0x07, 0x0A, 0x8A, 0x02, 0x00, 0x00, 0x48, 0x8B, 0x00, 0x48, 0x8B, 0xC8, 0x48, 0x8B, 0xD6, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0x30, 0xB8, 0x59, 0x0A, 0x8A, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8B, 0x7E, 0x38, 0x45, 0x33, 0xFF, 0xE9, 0x39, 0x00, 0x00, 0x00, 0x66, 0x90, 0x49, 0x63, 0xC7, 0x39, 0x47, 0x18, 0x0F, 0x86, 0x9A, 0x01, 0x00, 0x00, 0x48, 0x8D, 0x44, 0xC7, 0x20, 0x48, 0x8B, 0x00, 0x48, 0x8B, 0xC8, 0xBA, 0x01, 0x00, 0x00, 0x00, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0x60, 0x63, 0x31, 0x1C, 0x8A, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x41, 0xFF, 0xC7, 0x48, 0x63, 0x47, 0x18, 0x44, 0x3B, 0xF8, 0x7C, 0xC0, 0x48, 0x8B, 0x46, 0x48, 0x48, 0x8B, 0xC8, 0x33, 0xD2, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0x10, 0x99, 0x04, 0x9E, 0x8A, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8B, 0x46, 0x48, 0x48, 0x89, 0x45, 0xB0, 0x48, 0x8D, 0x46, 0x58, 0x48, 0x63, 0x00, 0x89, 0x45, 0xD8, 0x48, 0x83, 0xEC, 0x08, 0x48, 0x83, 0xEC, 0x08, 0x48, 0x63, 0x45, 0xD8, 0x89, 0x04, 0x24, 0x48, 0x83, 0xEC, 0x20, 0x49, 0xBB, 0xF0, 0xE8, 0xAD, 0x1C, 0x8A, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x30, 0x48, 0x8B, 0xD0, 0x48, 0x8B, 0x45, 0xB0, 0x48, 0x8B, 0xC8, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0x00, 0x9A, 0x04, 0x9E, 0x8A, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8B, 0x46, 0x30, 0x48, 0x8B, 0xC8, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0xF0, 0x9D, 0x5A, 0x0A, 0x8A, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8B, 0xC8, 0xBA, 0x01, 0x00, 0x00, 0x00, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0xC0, 0x75, 0x2D, 0x1C, 0x8A, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8B, 0x46, 0x30, 0x48, 0x89, 0x45, 0xB8, 0x48, 0x8B, 0x46, 0x48, 0x48, 0x8B, 0xC8, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0x00, 0xFA, 0x32, 0x1C, 0x8A, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0xF3, 0x0F, 0x5A, 0xC0, 0x48, 0x8B, 0x45, 0xB8, 0x48, 0x8B, 0xC8, 0xF2, 0x0F, 0x10, 0xC8, 0xF2, 0x0F, 0x5A, 0xC9, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0xB0, 0xBF, 0xAD, 0x1C, 0x8A, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8B, 0x46, 0x30, 0x48, 0x89, 0x45, 0xC0, 0x48, 0x8B, 0x46, 0x48, 0x48, 0x8B, 0xC8, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0x80, 0x71, 0x31, 0x1C, 0x8A, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0xF3, 0x0F, 0x5A, 0xC0, 0x48, 0x8B, 0x45, 0xC0, 0x48, 0x8B, 0xC8, 0xF2, 0x0F, 0x10, 0xC8, 0xF2, 0x0F, 0x5A, 0xC9, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0x70, 0xDB, 0xAD, 0x1C, 0x8A, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8D, 0x65, 0xE8, 0x41, 0x5F, 0x5F, 0x5E, 0xC9, 0xC3, 0xBA, 0xB7, 0x01, 0x00, 0x00, 0xB9, 0x43, 0x01, 0x00, 0x02, 0x49, 0xBB, 0xD0, 0x1C, 0xF5, 0x09, 0x8A, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3
		//0x55, 0x48, 0x8B, 0xEC, 0x56, 0x57, 0x41, 0x57, 0x48, 0x83, 0xEC, 0x38, 0x48, 0x8B, 0xF1, 0x48, 0x83, 0xEC, 0x20, 0x49, 0xBB, 0x40, 0x37, 0xD5, 0x14, 0x63, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x85, 0xC0, 0x0F, 0x84, 0xD8, 0x01, 0x00, 0x00, 0x48, 0xB8, 0xE0, 0xCE, 0x52, 0x7E, 0x63, 0x02, 0x00, 0x00, 0x48, 0x8B, 0x00, 0x48, 0x8B, 0xC8, 0x48, 0x8B, 0xD6, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0x30, 0xB8, 0xA4, 0x7E, 0x63, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8B, 0x7E, 0x38, 0x45, 0x33, 0xFF, 0xE9, 0x3C, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x64, 0x24, 0x00, 0x49, 0x63, 0xC7, 0x39, 0x47, 0x18, 0x0F, 0x86, 0x9A, 0x01, 0x00, 0x00, 0x48, 0x8D, 0x44, 0xC7, 0x20, 0x48, 0x8B, 0x00, 0x48, 0x8B, 0xC8, 0xBA, 0x01, 0x00, 0x00, 0x00, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0x70, 0x63, 0xD4, 0x10, 0x63, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x41, 0xFF, 0xC7, 0x48, 0x63, 0x47, 0x18, 0x44, 0x3B, 0xF8, 0x7C, 0xC0, 0x48, 0x8B, 0x46, 0x48, 0x48, 0x8B, 0xC8, 0x33, 0xD2, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0x10, 0xB3, 0xDB, 0x14, 0x63, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8B, 0x46, 0x48, 0x48, 0x89, 0x45, 0xB0, 0x48, 0x8D, 0x46, 0x58, 0x48, 0x63, 0x00, 0x89, 0x45, 0xD8, 0x48, 0x83, 0xEC, 0x08, 0x48, 0x83, 0xEC, 0x08, 0x48, 0x63, 0x45, 0xD8, 0x89, 0x04, 0x24, 0x48, 0x83, 0xEC, 0x20, 0x49, 0xBB, 0xA0, 0xDE, 0xD9, 0x14, 0x63, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x30, 0x48, 0x8B, 0xD0, 0x48, 0x8B, 0x45, 0xB0, 0x48, 0x8B, 0xC8, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0x00, 0xB4, 0xDB, 0x14, 0x63, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8B, 0x46, 0x30, 0x48, 0x8B, 0xC8, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0xF0, 0x9D, 0xA5, 0x7E, 0x63, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8B, 0xC8, 0xBA, 0x01, 0x00, 0x00, 0x00, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0xD0, 0x75, 0xD0, 0x10, 0x63, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8B, 0x46, 0x30, 0x48, 0x89, 0x45, 0xB8, 0x48, 0x8B, 0x46, 0x48, 0x48, 0x8B, 0xC8, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0x10, 0xFA, 0xD5, 0x10, 0x63, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0xF3, 0x0F, 0x5A, 0xC0, 0x48, 0x8B, 0x45, 0xB8, 0x48, 0x8B, 0xC8, 0xF2, 0x0F, 0x10, 0xC8, 0xF2, 0x0F, 0x5A, 0xC9, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0x00, 0xC1, 0xD9, 0x14, 0x63, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8B, 0x46, 0x30, 0x48, 0x89, 0x45, 0xC0, 0x48, 0x8B, 0x46, 0x48, 0x48, 0x8B, 0xC8, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0x90, 0x71, 0xD4, 0x10, 0x63, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0xF3, 0x0F, 0x5A, 0xC0, 0x48, 0x8B, 0x45, 0xC0, 0x48, 0x8B, 0xC8, 0xF2, 0x0F, 0x10, 0xC8, 0xF2, 0x0F, 0x5A, 0xC9, 0x48, 0x83, 0xEC, 0x20, 0x83, 0x38, 0x00, 0x49, 0xBB, 0xB0, 0xDC, 0xD9, 0x14, 0x63, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3, 0x48, 0x83, 0xC4, 0x20, 0x48, 0x8D, 0x65, 0xE8, 0x41, 0x5F, 0x5F, 0x5E, 0xC9, 0xC3, 0xBA, 0xB7, 0x01, 0x00, 0x00, 0xB9, 0x43, 0x01, 0x00, 0x02, 0x49, 0xBB, 0xD0, 0x1C, 0x2C, 0x7E, 0x63, 0x02, 0x00, 0x00, 0x41, 0xFF, 0xD3
		0x90, 0x90, 0x90
	};


	auto methodAllocator = PatternScanSingle(handle, pattern, mask);

	if (methodAllocator != NULL) {
		DWORD64 pointer;
		DWORD64 methodAllocatorPointer = reinterpret_cast<DWORD64>(methodAllocator);
		ReadProcessMemory(handle, (void*)(methodAllocatorPointer - 0x6), &pointer, sizeof(pointer), 0);
		printf("Fount at %p\n", pointer);

		SIZE_T bytes_written;
		WriteProcessMemory(handle, (LPVOID)(pointer + 0x4a), patch, sizeof(patch), &bytes_written);
		printf("Patch applied!\n");

		return true;
	}*/

	UCHAR patch[] = {
		// mov rcx,r15
		0x90, 0x90, 0x90
	};

	UCHAR patch2[] = {
		// je 1E8239A8F6B
		0x90, 0x90, 0x90, 0x90, 0x90, 0x90
	};

	UCHAR patch3[] = {
		// jne 1E8239A8F6B
		0x90, 0x90
	};

	auto tpointer = PatternScanSingle(handle, pattern, mask, 1048576);

	if (tpointer != NULL) {
		/*DWORD64 pointer;
		DWORD64 methodAllocatorPointer = reinterpret_cast<DWORD64>(methodAllocator);
		ReadProcessMemory(handle, (void*)(methodAllocatorPointer - 0x6), &pointer, sizeof(pointer), 0);
		printf("Found at %p\n", pointer);*/
		DWORD64 pointer = (DWORD64)tpointer - 0x24;
		printf("Found at %p\n", pointer);

		SIZE_T bytes_written;

		WriteProcessMemory(handle, (LPVOID)(pointer + 0x4a), patch, sizeof(patch), &bytes_written);
		//printf("Patch applied!\n");

		WriteProcessMemory(handle, (LPVOID)(pointer + 0x5b), patch2, sizeof(patch2), &bytes_written);
		printf("Patch applied!\n");

		return true;
	}

	printf("You must be spawned and alive to apply patch!");

	return false;
}

bool PatchCameraFilter(HANDLE handle) {
	
	std::cout << std::endl << "Patching Camera .." << std::endl;

	UCHAR pattern[] = {
		/*// 00 00 07 00 00 00 44 03 00 00 00 00 00 00 00 00 00 00
		0x00,
		0x00,
		0x07,
		0x00,
		0x00,
		0x00,
		0x44,
		0x03,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,*/

		/*// 00 00 1D 00 00 00 7B 00 00 00 00 00 00 00 00 00 00 -> +0x12
		0x00,
		0x00,
		0x1D,
		0x00,
		0x00,
		0x00,
		0x7B,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00*/

		// C3 00 00 C8 42 00 00 00 00 00 00 00 00 44 03
		0xC3,
		0x00,
		0x00,
		0xC8,
		0x42,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x44,
		0x03
	};
	//CHAR mask[] = "xxxxxxxxxxxxxxxxx";
	CHAR mask[] = "xxxxxxxxxxxxxx";

	UCHAR patch[] = {
		// mov rcx,r15
		0x90, 0x90, 0x90
	};

	auto tpointer = PatternScanSingle(handle, pattern, mask, 1048576);// , 1048576);

	if (tpointer != NULL) {

		// check if double..

		/*DWORD64 pointer;
		DWORD64 methodAllocatorPointer = reinterpret_cast<DWORD64>(methodAllocator);
		ReadProcessMemory(handle, (void*)(methodAllocatorPointer - 0x33f), &pointer, sizeof(pointer), 0);
		printf("Found at %p\n", pointer);*/

		/*WriteProcessMemory(handle, (LPVOID)(pointer + 0x8e), patch3, sizeof(patch), &bytes_written);
		printf("Patch applied!\n");*/

		DWORD64 pointer = (DWORD64)tpointer - 0x33f;
		printf("Found at %p\n", pointer);

		SIZE_T bytes_written;
		WriteProcessMemory(handle, (LPVOID)(pointer + 0x2e0), patch, sizeof(patch), &bytes_written);
		printf("Patch applied!\n");

		return true;
	}

	printf("You must be spawned and alive to apply patch!");

	return false;
}

bool PatchFootsteps(HANDLE handle) {

	// 00 00 07 00 00 00 54 01 00 00 00 00 00 00 00 00 00 00
	std::cout << std::endl << "Patching Footsteps .." << std::endl;
	UCHAR pattern[] = {
		// 00 00 07 00 00 00 54 01 00 00 00 00
		0x00,
		0x00,
		0x07,
		0x00,
		0x00,
		0x00,
		0x54,
		0x01,
		0x00,
		0x00,
		0x00,
		0x00,
	};
	CHAR mask[] = "xxxxxxxxxxx";
	//auto methodAllocator = SearchEveryWhere(handle, gmodule, mbi.RegionSize, pattern, mask);
	auto methodAllocator = PatternScanSingle(handle, pattern, mask);// , 614400);
	//printf("methodAllocator returned %p", methodAllocator);
	if (methodAllocator != NULL) {
		DWORD64 footstepPointer;
		DWORD64 methodAllocatorPointer = reinterpret_cast<DWORD64>(methodAllocator);
		ReadProcessMemory(handle, (void*)(methodAllocatorPointer - 0x6), &footstepPointer, sizeof(footstepPointer), 0);

		//printf("methodAllocator found at %p!\n", methodAllocator);
		//printf("footstep pointer found at %p!\n", footstepPointer);

		/*UCHAR pattern[] = {
		// C9 C3
		0xC3,
		};
		CHAR mask[] = "x";

		//auto footstepEnd = PatternScanSingle(handle, pattern, mask, footstepPointer, footstepPointer + 0x1F7);
		//printf("footstep end at %p\n", footstepEnd);*/

		UCHAR new_bytes[] = {
			// test eax, eax (85 C0)
			//0x90,
			//0x90,
			
			// je 1294012940
			0xE9,
			0xE5,
			0x00,
			0x00,
			0x00
		};

		printf("found at %p\n", footstepPointer);

		SIZE_T bytes_written;
		WriteProcessMemory(handle, (LPVOID)(footstepPointer + 0x29), new_bytes, sizeof(new_bytes), &bytes_written);
		printf("Patch applied!\n");
	}
	else {
		printf("LocalPlayer never loaded any match or the game was updated!\n");
	}
	return true;




	/*std::cout << std::endl << "Patching Footsteps .." << std::endl;
	UCHAR pattern[] = {
		// 00 00 07 00 00 00 F8 01 00 00 00 00 00 00 00 00 00 00 33 33 33 3F 00 00 00 00
		0x00,
		0x00,
		0x07,
		0x00,
		0x00,
		0x00,
		0xF8,
		0x01,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x33,
		0x33,
		0x33,
		0x3F,
		0x00,
		0x00,
		0x00,
		0x00
	};
	CHAR mask[] = "xxxxxxxxxxxxxxxxxxxxxxxxx";
	//auto methodAllocator = SearchEveryWhere(handle, gmodule, mbi.RegionSize, pattern, mask);
	auto methodAllocator = PatternScanSingle(handle, pattern, mask);
	//printf("methodAllocator returned %p", methodAllocator);
	if (methodAllocator != NULL) {
		DWORD64 footstepPointer;
		DWORD64 methodAllocatorPointer = reinterpret_cast<DWORD64>(methodAllocator);
		ReadProcessMemory(handle, (void*)(methodAllocatorPointer - 0x6), &footstepPointer, sizeof(footstepPointer), 0);

		printf("methodAllocator found at %p!\n", methodAllocator);
		printf("footstep pointer found at %p!\n", footstepPointer);

		UCHAR new_bytes[] = {
			// call r11 (41 FF D3)
			0x90,
			0x90,
			0x90,
		};
		SIZE_T bytes_written;
		WriteProcessMemory(handle, (LPVOID)(footstepPointer + 0x1E7), new_bytes, sizeof(new_bytes), &bytes_written);

	}
	else {
		printf("LocalPlayer never loaded any match or the game was updated!\n");
	}
	return true;*/
}

bool PatchWallhackAttack(HANDLE handle) {

	std::cout << std::endl << "Patching wallhack self-attack bug .." << std::endl;
	UCHAR pattern[] = {
		/*// 00 00 80 3F 85 01 00 00 00 00 40 40 85 01 00
		// 00 00 80 3F ?? 00 00 00 00 00 40 40 ?? ?? 00
		0x00,
		0x00,
		0x80,
		0x3F,
		0x85,
		0x01,
		0x00,
		0x00,
		0x00,
		0x00,
		0x40,
		0x40,
		0x85,
		0x01,
		0x00*/

		// 00 00 07 00 00 00 12 01 00 00 00 00 00 00 00 00 00 00 00 00
		0x00,
		0x00,
		0x07,
		0x00,
		0x00,
		0x00,
		0x12,
		0x01,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00

	};
	CHAR mask[] = "xxxxxxxxxxxxxxxxxxx";

	auto methodAllocator = PatternScanSingle(handle, pattern, mask);
	if (methodAllocator != NULL) {
		DWORD64 pointer;
		DWORD64 methodAllocatorPointer = reinterpret_cast<DWORD64>(methodAllocator);
		ReadProcessMemory(handle, (void*)(methodAllocatorPointer - 0x6), &pointer, sizeof(pointer), 0);

		//printf("methodAllocator found at %p!\n", methodAllocator);
		//printf("patchwallhack pointer found at %p!\n", pointer);

		/*UCHAR pattern[] = {
		// C9 C3
		0xC3,
		};
		CHAR mask[] = "x";

		//auto footstepEnd = PatternScanSingle(handle, pattern, mask, footstepPointer, footstepPointer + 0x1F7);
		//printf("footstep end at %p\n", footstepEnd);*/

		UCHAR new_bytes[] = {
			// test eax, eax (85 C0)
			0x90,
			0x90,
		};

		UCHAR new_bytes2[] = {
			// test eax, eax (85 C0)
			0x90,
			0x90,
			0x90,
		};

		printf("found at %p\n", pointer);

		SIZE_T bytes_written;
		WriteProcessMemory(handle, (LPVOID)(pointer + 0x24), new_bytes, sizeof(new_bytes), &bytes_written);
		WriteProcessMemory(handle, (LPVOID)(pointer + 0x6f), new_bytes, sizeof(new_bytes), &bytes_written);
		WriteProcessMemory(handle, (LPVOID)(pointer + 0x84), new_bytes2, sizeof(new_bytes), &bytes_written);
		printf("Patch applied!\n");
	}
	else {
		printf("LocalPlayer never loaded any match or the game was updated!\n");
	}
	return true;
}

int PrintModules(HANDLE handle)
{
	HMODULE hMods[1024];
	DWORD cbNeeded;
	char module_name[MAX_PATH];
	unsigned int i;
	MODULEINFO modinfo;

	// Get a list of all the modules in this process.

	if (EnumProcessModules(handle, hMods, sizeof(hMods), &cbNeeded))
	{
		for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
		{
			TCHAR szModName[MAX_PATH];

			// Get the full path to the module's file.

			if (GetModuleFileNameEx(handle, hMods[i], szModName,
				sizeof(szModName) / sizeof(TCHAR)))
			{
				// Print the module name and handle value.
				GetModuleBaseName(handle, hMods[i], module_name, sizeof(module_name));

				//printf(TEXT("\t%s (0x%08X) %s\n"), szModName, hMods[i], module_name);
				if (_stricmp(module_name, "SCPSL.exe") == 0) {
				//if (module_name == "mono.dll") {
					//printf(TEXT("\t%s (0x%08X) %s\n"), szModName, hMods[i], module_name);
					gmodule = hMods[i];
					GetModuleInformation(handle, gmodule, &modinfo, sizeof(MODULEINFO));
					VirtualQueryEx(handle, (PVOID)gmodule, &mbi, sizeof(mbi));
					BYTE* buffer = nullptr; 
					buffer = new BYTE[mbi.RegionSize];
					printf("SCP:SL module base: %p\n", (DWORD64)gmodule);
					printf("SCP:SL module size: %p\n", mbi.RegionSize);
				}
			}
		}
	}

	return 0;
}


int main() {

	HANDLE handle = GetProcessHandle();
	std::cout << "SCP:SL handle: " << (int)handle << std::endl;

	if (handle) {

		PrintModules(handle);

		if (PatchWallhackAttack(handle)) {

			// We patch wallhack only if patched attack.
			// killing yourself isn't cool, right?

			if (PatchWallHack(handle)) {

				// Camera isn't enabled instantly..
				Sleep(500);

				PatchCameraFilter(handle);
			}
		}
		PatchFootsteps(handle);

		CloseHandle(handle);
	}

	system("pause");

	return 0;

}
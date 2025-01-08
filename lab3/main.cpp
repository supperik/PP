#include <iostream>
#include <string>
#include <windows.h>
#include <fstream>

std::ofstream firstLogs("logs1.txt");
std::ofstream secondLogs("logs2.txt");
DWORD timeOfStart;
SYSTEMTIME sysTime;


void PayLoad()
{
	int result;
	for (int j = 0; j < 1'000'000; j++)
	{
		result = 100 / sqrt(100);
	}
}

DWORD WINAPI ThreadProc(CONST LPVOID lpParam)
{
	int payLoad;


	for (int i = 0; i < 20; i++)
	{
		PayLoad();
		SYSTEMTIME curTime;
		GetSystemTime(&curTime);

		DWORD currentTime = GetTickCount();

		int firstTime = sysTime.wSecond * 1000 + sysTime.wMilliseconds;
		int secondTime = curTime.wSecond * 1000 + curTime.wMilliseconds;

		std::string dur = std::to_string(secondTime - firstTime);


		(int(lpParam) == 2) ? (secondLogs  /* << std::to_string((int)lpParam) << "|"*/ << dur << std::endl)
			: (firstLogs /* << std::to_string((int)lpParam) << "|"*/ << dur << std::endl);
	}

	ExitThread(0);
}

int main()
{
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);
	UINT countProcessor = (1 << 8 - 1);
	int countThreads = 2;
	SetProcessAffinityMask(GetCurrentProcess(), countProcessor);

	system("pause");

	timeOfStart = GetTickCount();
	GetSystemTime(&sysTime);

	HANDLE* handles = new HANDLE[countThreads];

	for (int i = 0; i < countThreads; i++)
	{
		int ThreadCount = i + 1;
		handles[i] = CreateThread(NULL, 0, &ThreadProc, (LPVOID)ThreadCount, CREATE_SUSPENDED, NULL);

		SetThreadPriority(handles[i], i);


	}

	for (int i = 0; i < countThreads; i++)
	{
		ResumeThread(handles[i]);
	}

	WaitForMultipleObjects(countThreads, handles, true, INFINITE);
	return 0;

}
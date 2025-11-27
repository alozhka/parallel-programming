#include "tchar.h"

#include <fstream>
#include <iostream>
#include <string>
#include <format>
#include <windows.h>

// constexpr LPCSTR MUTEX_NAME = R"(Lw5PPGlobalMutex)";
// HANDLE GlobalMutex = nullptr;
CRITICAL_SECTION GlobalCriticalSection;

struct threadData
{
	int money;
};

int ReadFromFile()
{
	std::fstream myfile("balance.txt", std::ios_base::in);
	int result;
	myfile >> result;
	myfile.close();

	return result;
}

void WriteToFile(int data)
{
	std::fstream myfile("balance.txt", std::ios_base::out);
	myfile << data << std::endl;
	myfile.close();
}

void PrintWithTime(int processId, const std::string& message, int balance = -1)
{
	SYSTEMTIME st;
	GetLocalTime(&st);

	std::cout << std::format("[{:02d}:{:02d}:{:02d}.{:03d}] ",
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	if (balance != -1)
	{
		std::cout << std::format("({}) {}: {}\n", processId, message, balance);
	}
	else
	{
		std::cout << std::format("({}) {}\n", processId, message);
	}
}

int GetBalance()
{
	int balance = ReadFromFile();
	return balance;
}

void Deposit(int money)
{
	// WaitForSingleObject(GlobalMutex, INFINITE);
	// EnterCriticalSection(&GlobalCriticalSection);

	int balance = GetBalance();
	balance += money;

	WriteToFile(balance);
	PrintWithTime(GetCurrentProcessId(), "Balance after deposit", balance);

	// ReleaseMutex(GlobalMutex);
	// LeaveCriticalSection(&GlobalCriticalSection);
}

void Withdraw(int money)
{
	// WaitForSingleObject(GlobalMutex, INFINITE);
	// EnterCriticalSection(&GlobalCriticalSection);

	if (GetBalance() < money)
	{
		PrintWithTime(GetCurrentProcessId(), std::format("Cannot withdraw money, balance lower than {}", money));
	}
	else
	{
		Sleep(20);
		int balance = GetBalance();
		balance -= money;
		WriteToFile(balance);
		PrintWithTime(GetCurrentProcessId(), "Balance after withdraw", balance);
	}

	// ReleaseMutex(GlobalMutex);
	// LeaveCriticalSection(&GlobalCriticalSection);
}

DWORD WINAPI DoDeposit(CONST LPVOID lpParameter)
{
	auto* data = static_cast<threadData*>(lpParameter);
	Deposit(data->money);
	ExitThread(0);
}

DWORD WINAPI DoWithdraw(CONST LPVOID lpParameter)
{
	auto* data = static_cast<threadData*>(lpParameter);
	Withdraw(data->money);
	ExitThread(0);
}

int main()
{
	auto* handles = new HANDLE[50];

	// GlobalMutex = CreateMutex(nullptr, false, MUTEX_NAME);
	InitializeCriticalSection(&GlobalCriticalSection);
	PrintWithTime(GetCurrentProcessId(), "Started");

	// WaitForSingleObject(GlobalMutex, INFINITE);
	// EnterCriticalSection(&GlobalCriticalSection);
	WriteToFile(0);
	PrintWithTime(GetCurrentProcessId(), "Balance is set to 0");
	// ReleaseMutex(GlobalMutex);
	// LeaveCriticalSection(&GlobalCriticalSection);

	SetProcessAffinityMask(GetCurrentProcess(), 1);
	for (int i = 0; i < 50; i++)
	{
		handles[i] = i % 2 == 0
			? CreateThread(nullptr, 0, &DoDeposit, new threadData{ 230 }, CREATE_SUSPENDED, nullptr)
			: CreateThread(nullptr, 0, &DoWithdraw, new threadData{ 1000 }, CREATE_SUSPENDED, nullptr);
		ResumeThread(handles[i]);
	}

	WaitForMultipleObjects(50, handles, true, INFINITE);
	PrintWithTime(GetCurrentProcessId(), "Final Balance", GetBalance());

	// CloseHandle(GlobalMutex);
	DeleteCriticalSection(&GlobalCriticalSection);

	getchar();
	return 0;
}

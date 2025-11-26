#include "tchar.h"

#include <fstream>
#include <iostream>
#include <string>
#include <windows.h>

HANDLE GlobalMutex = nullptr;
// CRITICAL_SECTION GlobalCriticalSection;

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

int GetBalance()
{
	int balance = ReadFromFile();
	return balance;
}

void Deposit(int money)
{
	WaitForSingleObject(GlobalMutex, INFINITE);
	// EnterCriticalSection(&GlobalCriticalSection);

	int balance = GetBalance();
	balance += money;

	WriteToFile(balance);
	printf("(%d) Balance after deposit: %d\n", GetCurrentProcessId(), balance);

	ReleaseMutex(GlobalMutex);
	// LeaveCriticalSection(&GlobalCriticalSection);
}

void Withdraw(int money)
{
	WaitForSingleObject(GlobalMutex, INFINITE);
	// EnterCriticalSection(&GlobalCriticalSection);

	if (GetBalance() < money)
	{
		printf("(%d) Cannot withdraw money, balance lower than %d\n", GetCurrentProcessId(), money);
	}
	else
	{
		Sleep(20);
		int balance = GetBalance();
		balance -= money;
		WriteToFile(balance);
		printf("(%d) Balance after withdraw: %d\n", GetCurrentProcessId(), balance);
	}

	ReleaseMutex(GlobalMutex);
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
	auto* handles = new HANDLE[49];

	GlobalMutex = CreateMutex(nullptr, false, "Lw5PPGlobalMutex");
	// InitializeCriticalSection(&GlobalCriticalSection);

	WaitForSingleObject(GlobalMutex, INFINITE);
	WriteToFile(0);
	printf("(%d) Balance is set to 0\n", GetCurrentProcessId());
	ReleaseMutex(GlobalMutex);

	SetProcessAffinityMask(GetCurrentProcess(), 1);
	for (int i = 0; i < 50; i++)
	{
		handles[i] = i % 2 == 0
			? CreateThread(nullptr, 0, &DoDeposit, new threadData{ 230 }, CREATE_SUSPENDED, nullptr)
			: CreateThread(nullptr, 0, &DoWithdraw, new threadData{ 1000 }, CREATE_SUSPENDED, nullptr);
		ResumeThread(handles[i]);
	}

	// ожидание окончания работы двух потоков
	WaitForMultipleObjects(50, handles, true, INFINITE);
	printf("(%d) Final Balance: %d\n", GetCurrentProcessId(), GetBalance());

	CloseHandle(GlobalMutex);
	// DeleteCriticalSection(&GlobalCriticalSection);

	return 0;
}

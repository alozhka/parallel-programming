#include "tchar.h"
#include <fstream>
#include <iostream>
#include <string>
#include <windows.h>

HANDLE GlobalMutex = nullptr;
CRITICAL_SECTION FileLockingCriticalSection;

struct threadData
{
	int money;
};

int ReadFromFile()
{
	EnterCriticalSection(&FileLockingCriticalSection);
	std::fstream myfile("balance.txt", std::ios_base::in);
	int result;
	myfile >> result;
	myfile.close();
	LeaveCriticalSection(&FileLockingCriticalSection);

	return result;
}

void WriteToFile(int data)
{
	EnterCriticalSection(&FileLockingCriticalSection);
	std::fstream myfile("balance.txt", std::ios_base::out);
	myfile << data << std::endl;
	myfile.close();
	LeaveCriticalSection(&FileLockingCriticalSection);
}

int GetBalance()
{
	int balance = ReadFromFile();
	return balance;
}

void Deposit(int money)
{
	WaitForSingleObject(GlobalMutex, INFINITE);
	int balance = GetBalance();
	balance += money;

	WriteToFile(balance);
	printf("Balance after deposit: %d\n", balance);
	ReleaseMutex(GlobalMutex);
}

void Withdraw(int money)
{
	WaitForSingleObject(GlobalMutex, INFINITE);
	if (GetBalance() < money)
	{
		printf("Cannot withdraw money, balance lower than %d\n", money);
		return;
	}

	Sleep(20);
	int balance = GetBalance();
	balance -= money;
	WriteToFile(balance);
	printf("Balance after withdraw: %d\n", balance);
	ReleaseMutex(GlobalMutex);
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

int _tmain()
{
	GlobalMutex = CreateMutex(nullptr, false, "GlobalBalanceMutex");
	auto* handles = new HANDLE[49];

	InitializeCriticalSection(&FileLockingCriticalSection);

	WriteToFile(0);

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
	printf("Final Balance: %d\n", GetBalance());

	getchar();

	DeleteCriticalSection(&FileLockingCriticalSection);

	return 0;
}

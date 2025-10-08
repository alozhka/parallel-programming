#include <cstdint>
#include <iostream>
#include <vector>
#include <windows.h>

static short ThreadAmountFromParams(int argc, char* argv[])
{
	if (argc < 2)
	{
		throw std::invalid_argument("Threads amount is not specified");
	}

	int amount = 0;
	try
	{
		amount = std::atoi(argv[1]);
	}
	catch (const std::exception&)
	{
		throw std::invalid_argument("Amount must be a number");
	}

	if (amount < 1 || amount > MAXIMUM_WAIT_OBJECTS)
	{
		throw std::invalid_argument("Amount must be greater than zero and lower than maximum wait objects");
	}

	return amount;
}

DWORD WINAPI ThreadFunction(CONST LPVOID lpParameter)
{
	short threadNumber = *static_cast<short*>(lpParameter);
	std::string threadInfo = "Thread number " + std::to_string(threadNumber) + "\n";
	std::cout << threadInfo;

	Sleep(500 + std::rand() % 1000);
	ExitThread(0);
}

void ProcessThreads(uint8_t amount)
{

	std::vector<HANDLE> handles;
	handles.resize(amount);
	std::vector<short> threadsParams;
	threadsParams.resize(amount);

	for (int i = 0; i < amount; ++i)
	{
		threadsParams[i] = i + 1;
		handles[i] = CreateThread(
			nullptr,
			0,
			&ThreadFunction,
			&threadsParams[i],
			CREATE_SUSPENDED,
			nullptr);
	}

	for (HANDLE& handle : handles)
	{
		ResumeThread(handle);
	}

	WaitForMultipleObjects(amount, handles.data(), TRUE, INFINITE);

	for (HANDLE& handle : handles)
	{
		CloseHandle(handle);
	}
}

int main(int argc, char* argv[])
{
	try
	{
		uint8_t amount = ThreadAmountFromParams(argc, argv);
		ProcessThreads(amount);
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}
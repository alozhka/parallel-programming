#include <cmath>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <windows.h>

constexpr int OPERATIONS_COUNT = 20;
constexpr int THREADS_COUNT = 2;
constexpr DWORD SLEEP_TIME = 10;
constexpr std::string OUTPUT_FILE = "thread_log.txt";

struct ThreadData
{
	int threadNumber;
	DWORD startTime;
	HANDLE hFile, mutex;
};
DWORD WINAPI ThreadFunction(LPVOID lpParam)
{
	const auto* data = static_cast<ThreadData*>(lpParam);

	for (int i = 0; i < OPERATIONS_COUNT; ++i)
	{
		DWORD currentTime = timeGetTime() - data->startTime;
		std::string buffer = std::to_string(data->threadNumber) + '|' + std::to_string(currentTime) + '\n';

		WaitForSingleObject(data->mutex, INFINITE);
		WriteFile(data->hFile, buffer.c_str(), buffer.size(), nullptr, nullptr);
		ReleaseMutex(data->mutex);

		volatile double x = 0.0;
		for (int j = 0; j < 100000; j++)
		{
			x += std::sin(std::cos(std::sin(static_cast<double>(j))));
		}
	}

	return 0;
}

void ClearFile(const std::string& fileName)
{
	std::ofstream outFile(fileName, std::ios::trunc);
	if (!outFile.is_open())
	{
		throw std::invalid_argument("Ошибка: не удалось создать файл для записи!");
	}
}

int main()
{
	SetProcessPriorityBoost(GetCurrentProcess(), true);
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
	ClearFile(OUTPUT_FILE);
	std::cout << "Запуск потоков..." << std::endl;
	DWORD startTime = timeGetTime();

	SetProcessAffinityMask(GetCurrentProcess(), 0b1);

	HANDLE mutex = CreateMutex(nullptr, false, nullptr);
	HANDLE file = CreateFile(
		OUTPUT_FILE.c_str(),
		GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	ThreadData threadData[THREADS_COUNT];
	HANDLE handles[THREADS_COUNT];

	for (int i = 0; i < THREADS_COUNT; ++i)
	{
		threadData[i] = {
			i + 1,
			startTime,
			file,
			mutex
		};
		handles[i] = CreateThread(
			nullptr,
			0,
			ThreadFunction,
			&threadData[i],
			0,
			nullptr);
	}

	SetThreadPriority(handles[1], THREAD_PRIORITY_HIGHEST);
	WaitForMultipleObjects(THREADS_COUNT, handles, TRUE, INFINITE);

	for (HANDLE& handle : handles)
	{
		CloseHandle(handle);
	}

	return 0;
}
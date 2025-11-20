#include "src/BmpProcessor.h"

#include <chrono>
#include <functional>
#include <iostream>
#include <random>
#include <vector>
#include <windows.h>

struct InputData
{
	std::string inputFile, outputFile, statsFile;
	int numCores, numThreads;
	std::vector<int> threadPriorities;
};

bool ParseCommandLine(int argc, char* argv[], InputData& input)
{
	if (argc != 6)
	{
		std::cerr << "Usage: " << argv[0] << " <input.bmp> <output.bmp> <stats.txt> <num_cores> <num_threads>\n";
		return false;
	}
	input.inputFile = argv[1];
	input.outputFile = argv[2];
	input.statsFile = argv[3];
	input.numCores = std::atoi(argv[4]);
	input.numThreads = std::atoi(argv[5]);

	if (input.numThreads < 1 || input.numThreads > 16 || input.numCores < 1 || input.numCores > 4)
	{
		std::cerr << "Invalid parameters: threads in [1,16], cores in [1,4]\n";
		return false;
	}
	return true;
}

bool SetCpuAffinity(int coresAmount)
{
	DWORD_PTR mask = (1ULL << coresAmount) - 1;
	return SetProcessAffinityMask(GetCurrentProcess(), mask) != 0;
}


void WriteTimings(const std::string& filename)
{
	std::ofstream out{ filename };
	if (!out.is_open())
	{
		throw std::invalid_argument("Cannot open log file");
	}
	for (auto [threadId, time] : g_timings)
	{
		out << threadId << "|" << time << std::endl;
	}
}

int main(int argc, char* argv[])
{
	SetProcessPriorityBoost(GetCurrentProcess(), true);
	DWORD start = timeGetTime();

	InputData input;

	if (!ParseCommandLine(argc, argv, input))
	{
		return 1;
	}

	if (!SetCpuAffinity(input.numCores))
	{
		std::cerr << "Failed to set CPU affinity\n";
		return 1;
	}

	FileData fileData = BmpProcessor::Read(input.inputFile);

	std::cout << "Image size: " << fileData.GetWidth() << "x" << fileData.GetHeight() << "\n";
	std::cout << "Row stride: " << fileData.GetRowStride() << "\n";
	std::cout << "Pixels size: " << fileData.pixels.size() << "\n";
	std::cout << "Threads: " << input.numThreads << ", Cores: " << input.numCores << "\n";

	BmpProcessor::BlurImage(fileData, input.numThreads, input.statsFile);

	BmpProcessor::Write(input.outputFile, fileData);
	std::cout << "Output saved to: " << input.outputFile << "\n";

	auto time = timeGetTime() - start;
	std::cout << "Spent time: " << time << "\n";

	WriteTimings(input.statsFile);
	return 0;
}
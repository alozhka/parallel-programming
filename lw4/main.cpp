#include "src/BmpProcessor.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>
#include <windows.h>

struct InputData
{
	std::string inputFile, outputFile;
	int numCores, numThreads;
};

bool ParseCommandLine(int argc, char* argv[], InputData& input)
{
	if (argc != 5)
	{
		std::cerr << "Usage: " << argv[0] << " <input.bmp> <output.bmp> <num_threads> <num_cores>\n";
		return false;
	}
	input.inputFile = argv[1];
	input.outputFile = argv[2];
	input.numCores = std::atoi(argv[3]);
	input.numThreads = std::atoi(argv[4]);

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

std::vector<std::vector<Square>> DivideIntoSquares(uint32_t width, uint32_t height, int numThreads);
void ApplyBoxBlurToSquare(const std::vector<uint8_t>& src, std::vector<uint8_t>& dst,
	const Square& square, uint32_t width, uint32_t height, uint32_t rowStride);
DWORD WINAPI ThreadProc(LPVOID lpParam);

int main(int argc, char* argv[])
{
	LARGE_INTEGER freq, start, end;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&start);

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

	BmpProcessor::BlurImage(fileData, input.numThreads);

	BmpProcessor::Write(input.outputFile, fileData);
	std::cout << "Output saved to: " << input.outputFile << "\n";

	QueryPerformanceCounter(&end);
	auto time = (end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
	std::cout << "Spent time: " << time << "\n";

	return 0;
}
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

struct Square
{
	int startX, startY;
	int endX, endY;
};

struct ThreadData
{
	std::vector<uint8_t>* srcPixels;
	std::vector<uint8_t>* dstPixels;
	uint32_t width;
	uint32_t height;
	uint32_t rowStride;
	std::vector<Square> squares;
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
	input.numCores = std::atoi(argv[4]);
	input.numThreads = std::atoi(argv[3]);

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
double MeasureTime(const std::function<void()>& func);

int main(int argc, char* argv[])
{
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

	constexpr int iterations = 10;
	auto threadSquares = DivideIntoSquares(fileData.GetWidth(), fileData.GetHeight(), input.numThreads);

	double TN = MeasureTime([&]() {
		for (int iter = 0; iter < iterations; ++iter)
		{
			auto temp = fileData.pixels;

			std::vector<HANDLE> threads(input.numThreads);
			std::vector<ThreadData> threadData(input.numThreads);

			for (int i = 0; i < input.numThreads; ++i)
			{
				threadData[i].srcPixels = &fileData.pixels;
				threadData[i].dstPixels = &temp;
				threadData[i].width = fileData.GetWidth();
				threadData[i].height = fileData.GetHeight();
				threadData[i].rowStride = fileData.GetRowStride();
				threadData[i].squares = threadSquares[i];

				threads[i] = CreateThread(nullptr, 0, ThreadProc, &threadData[i], 0, nullptr);
			}

			WaitForMultipleObjects(input.numThreads, threads.data(), TRUE, INFINITE);

			for (int i = 0; i < input.numThreads; ++i)
			{
				CloseHandle(threads[i]);
			}

			fileData.pixels = temp;
		}
	});

	std::cout << "Spent time: " << TN << "\n";


	BmpProcessor::Write(input.outputFile, fileData);
	std::cout << "Output saved to: " << input.outputFile << "\n";

	return 0;
}

std::vector<std::vector<Square>> DivideIntoSquares(uint32_t width, uint32_t height, int numThreads)
{
	// Всего квадратов: N*N, где N = numThreads
	int totalSquares = numThreads * numThreads;
	int squaresPerSide = numThreads;

	// Размеры одного квадрата
	int squareWidth = (width + squaresPerSide - 1) / squaresPerSide;
	int squareHeight = (height + squaresPerSide - 1) / squaresPerSide;

	// Создаем все квадраты
	std::vector<Square> allSquares;
	for (int row = 0; row < squaresPerSide; ++row)
	{
		for (int col = 0; col < squaresPerSide; ++col)
		{
			Square sq;
			sq.startX = col * squareWidth;
			sq.startY = row * squareHeight;
			sq.endX = std::min(sq.startX + squareWidth, static_cast<int>(width));
			sq.endY = std::min(sq.startY + squareHeight, static_cast<int>(height));
			allSquares.push_back(sq);
		}
	}

	// Перемешиваем квадраты для равномерного распределения
	std::vector<int> indices(totalSquares);
	std::iota(indices.begin(), indices.end(), 0);
	std::shuffle(indices.begin(), indices.end(), std::default_random_engine{});

	// Распределяем квадраты по потокам (каждый поток получает N квадратов)
	std::vector<std::vector<Square>> result(numThreads);
	for (int i = 0; i < totalSquares; ++i)
	{
		int threadIdx = i % numThreads;
		result[threadIdx].push_back(allSquares[indices[i]]);
	}

	return result;
}

void ApplyBoxBlurToSquare(const std::vector<uint8_t>& src, std::vector<uint8_t>& dst,
	const Square& square, uint32_t width, uint32_t height, uint32_t rowStride)
{
	for (int y = square.startY; y < square.endY; ++y)
	{
		for (int x = square.startX; x < square.endX; ++x)
		{
			int r = 0, g = 0, b = 0, count = 0;

			// Box blur 3x3
			for (int dy = -1; dy <= 1; ++dy)
			{
				for (int dx = -1; dx <= 1; ++dx)
				{
					int ny = y + dy;
					int nx = x + dx;

					if (ny >= 0 && ny < static_cast<int>(height) && nx >= 0 && nx < static_cast<int>(width))
					{
						int offset = ny * rowStride + nx * 3;
						b += src[offset + 0];
						g += src[offset + 1];
						r += src[offset + 2];
						++count;
					}
				}
			}

			int offset = y * rowStride + x * 3;
			dst[offset + 0] = static_cast<uint8_t>(b / count);
			dst[offset + 1] = static_cast<uint8_t>(g / count);
			dst[offset + 2] = static_cast<uint8_t>(r / count);
		}
	}
}

DWORD WINAPI ThreadProc(LPVOID lpParam)
{
	ThreadData* data = static_cast<ThreadData*>(lpParam);

	for (const auto& square : data->squares)
	{
		ApplyBoxBlurToSquare(*data->srcPixels, *data->dstPixels, square,
			data->width, data->height, data->rowStride);
	}

	return 0;
}

double MeasureTime(const std::function<void()>& func)
{
	LARGE_INTEGER freq, start, end;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&start);
	func();
	QueryPerformanceCounter(&end);
	return (end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
}
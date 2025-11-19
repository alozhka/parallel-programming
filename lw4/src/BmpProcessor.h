#pragma once
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
#include <windows.h>

#pragma pack(push, 1)
struct FileHeader
{
	uint16_t file_type{ 0x4D42 };
	uint32_t file_size{ 0 };
	uint16_t reserved1{ 0 };
	uint16_t reserved2{ 0 };
	uint32_t offset_data{ 0 };
};

struct BitmapHeader
{
	uint32_t size{ 40 };
	int32_t width{ 0 };
	int32_t height{ 0 };
	uint16_t planes{ 1 };
	uint16_t bit_count{ 24 };
	uint32_t compression{ 0 };
	uint32_t size_image{ 0 };
	int32_t x_pixels_per_meter{ 0 };
	int32_t y_pixels_per_meter{ 0 };
	uint32_t colors_used{ 0 };
	uint32_t colors_important{ 0 };
};
#pragma pack(pop)

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
	int threadId;
	DWORD startTime;
	HANDLE statsFile, mutex;
};

struct FileData
{
	FileHeader header;
	BitmapHeader bitmapHeader;
	std::vector<uint8_t> pixels;

	uint32_t GetWidth() const { return bitmapHeader.width; }
	uint32_t GetHeight() const { return bitmapHeader.height; }
	uint32_t GetRowStride() const { return (bitmapHeader.width * 3 + 3) & (~3); }
};

class BmpProcessor
{
public:
	static FileData Read(const std::string& filename)
	{
		std::ifstream in(filename, std::ios::binary);
		if (!in)
		{
			throw std::runtime_error("Cannot open file");
		}

		FileData data;
		FileHeader header;
		BitmapHeader bitmapHeader;

		in.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));
		in.read(reinterpret_cast<char*>(&bitmapHeader), sizeof(BitmapHeader));
		data.header = header;
		data.bitmapHeader = bitmapHeader;

		in.seekg(header.offset_data, std::ios::beg);

		// Вычисляем размер строки с учетом выравнивания до 4 байт
		// Каждый пиксель занимает 3 байта (BGR)
		uint32_t row_stride = (bitmapHeader.width * 3 + 3) & (~3);
		uint32_t pixelsSize = row_stride * bitmapHeader.height;
		data.pixels.resize(pixelsSize);

		// Читаем все строки
		for (int y = 0; y < bitmapHeader.height; ++y)
		{
			in.read(reinterpret_cast<char*>(data.pixels.data() + y * row_stride), row_stride);
		}

		in.close();
		return data;
	}

	static void Write(const std::string& filename, const FileData& data)
	{
		std::ofstream out(filename, std::ios::binary);
		if (!out)
		{
			throw std::runtime_error("Cannot create file");
		}

		out.write(reinterpret_cast<const char*>(&data.header), sizeof(FileHeader));
		out.write(reinterpret_cast<const char*>(&data.bitmapHeader), sizeof(BitmapHeader));

		// Записываем пиксели
		uint32_t row_stride = data.GetRowStride();
		for (int y = 0; y < data.bitmapHeader.height; ++y)
		{
			out.write(reinterpret_cast<const char*>(data.pixels.data() + y * row_stride), row_stride);
		}

		out.close();
	}

	static void BlurImage(FileData& fileData, int numThreads, const std::string& statsFile)
	{
		HANDLE hStatsFile = CreateFile(
			statsFile.c_str(),
			GENERIC_WRITE,
			0,
			nullptr,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			nullptr);
		HANDLE mutex = CreateMutex(nullptr, false, nullptr);

		DWORD globalStart = timeGetTime();

		auto threadSquares = DivideIntoSquares(fileData.GetWidth(), fileData.GetHeight(), numThreads);

		for (int iter = 0; iter < ITERATIONS; ++iter)
		{
			auto temp = fileData.pixels;

			std::vector<HANDLE> threads(numThreads);
			std::vector<ThreadData> threadData(numThreads);

			for (int i = 0; i < numThreads; ++i)
			{
				threadData[i].srcPixels = &fileData.pixels;
				threadData[i].dstPixels = &temp;
				threadData[i].width = fileData.GetWidth();
				threadData[i].height = fileData.GetHeight();
				threadData[i].rowStride = fileData.GetRowStride();
				threadData[i].squares = threadSquares[i];
				threadData[i].threadId = i + 1;
				threadData[i].startTime = globalStart;
				threadData[i].statsFile = hStatsFile;
				threadData[i].mutex = mutex;

				threads[i] = CreateThread(nullptr, 0, BlurFunction, &threadData[i], 0, nullptr);
			}

			SetThreadPriority(threads[0], ABOVE_NORMAL_PRIORITY_CLASS);
			WaitForMultipleObjects(numThreads, threads.data(), TRUE, INFINITE);

			for (HANDLE& thread : threads)
			{
				CloseHandle(thread);
			}

			fileData.pixels = temp;
		}
	}

private:
	static constexpr int ITERATIONS = 17;

	static DWORD WINAPI BlurFunction(LPVOID lpParam)
	{
		auto* data = static_cast<ThreadData*>(lpParam);

		for (const Square& square : data->squares)
		{
			ApplyBoxBlurToSquare(*data->srcPixels, *data->dstPixels, square,
				data->width, data->height, data->rowStride, data->threadId,
				data->startTime, data->statsFile, data->mutex);
		}

		return 0;
	}

	static void ApplyBoxBlurToSquare(const std::vector<uint8_t>& src, std::vector<uint8_t>& dst,
		const Square& square, uint32_t width, uint32_t height, uint32_t rowStride, int threadId, DWORD globalStart, HANDLE statsFile, HANDLE mutex)
	{
		volatile int pixels = 0;
		for (int y = square.startY; y < square.endY; ++y)
		{
			for (int x = square.startX; x < square.endX; ++x)
			{
				DWORD currentTime = timeGetTime();

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

				pixels = pixels + 1;
				if (pixels > 20000)
				{
					pixels = 0;
					DWORD elapsedMs = currentTime - globalStart;
					std::string statsLine = std::to_string(threadId) + "|" + std::to_string(elapsedMs) + "\n";
					WaitForSingleObject(mutex, INFINITE);
					WriteFile(statsFile, statsLine.c_str(), statsLine.length(), nullptr, nullptr);
					ReleaseMutex(mutex);

					volatile double temp = 0;
					for (int j = 0; j < 1000; j++)
					{
						temp += std::sin(std::cos(std::sin(static_cast<double>(j))));
					}
				}
			}
		}
	}

	static std::vector<std::vector<Square>> DivideIntoSquares(uint32_t width, uint32_t height, int numThreads)
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
				Square sq{};
				sq.startX = col * squareWidth;
				sq.startY = row * squareHeight;
				sq.endX = std::min(sq.startX + squareWidth, static_cast<int>(width));
				sq.endY = std::min(sq.startY + squareHeight, static_cast<int>(height));
				allSquares.push_back(sq);
			}
		}

		std::vector<int> indices(totalSquares);
		std::iota(indices.begin(), indices.end(), 0);
		std::ranges::shuffle(indices, std::default_random_engine{ 321 });

		// Распределяем квадраты по потокам (каждый поток получает N квадратов)
		std::vector<std::vector<Square>> result(numThreads);
		for (int i = 0; i < totalSquares; ++i)
		{
			int threadIdx = i % numThreads;
			result[threadIdx].push_back(allSquares[indices[i]]);
		}

		return result;
	}
};
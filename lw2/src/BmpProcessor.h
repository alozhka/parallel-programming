#pragma once
#include <cstdint>
#include <fstream>
#include <vector>

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
};
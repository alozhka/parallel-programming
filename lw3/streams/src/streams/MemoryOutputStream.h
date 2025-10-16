#pragma once
#include "IOutputDataStream.h"
#include <vector>

class MemoryOutputStream : public IOutputDataStream
{
public:
	MemoryOutputStream();

	void WriteByte(uint8_t data) override;
	void WriteBlock(const void* srcData, std::streamsize size) override;
	void Close() override;

	// Метод для получения данных (для тестирования)
	const std::vector<uint8_t>& GetData() const;

private:
	void EnsureStreamIsOpened() const;

	std::vector<uint8_t> m_data;
	bool m_isClosed = false;
};
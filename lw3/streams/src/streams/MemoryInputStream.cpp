#include "MemoryInputStream.h"
#include <stdexcept>
#include <cstring>

MemoryInputStream::MemoryInputStream(const std::vector<uint8_t>& data)
	: m_data(data)
	, m_position(0)
{
}

bool MemoryInputStream::IsEOF() const
{
	return m_position >= m_data.size();
}

uint8_t MemoryInputStream::ReadByte()
{
	EnsureIsNotEOF();
	return m_data[m_position++];
}

std::streamsize MemoryInputStream::ReadBlock(void* dstBuffer, std::streamsize size)
{
	if (size == 0)
	{
		return 0;
	}

	std::streamsize bytesToRead = std::min<std::streamsize>(size, m_data.size() - m_position);
	if (bytesToRead > 0)
	{
		std::memcpy(dstBuffer, m_data.data() + m_position, bytesToRead);
		m_position += bytesToRead;
	}

	return bytesToRead;
}

void MemoryInputStream::EnsureIsNotEOF() const
{
	if (IsEOF())
	{
		throw std::ios_base::failure("Cannot read from stream: end of stream reached");
	}
}
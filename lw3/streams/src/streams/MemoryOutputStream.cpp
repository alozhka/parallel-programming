#include "MemoryOutputStream.h"
#include <stdexcept>
#include <cstring>

MemoryOutputStream::MemoryOutputStream()
	: m_isClosed(false)
{
}

void MemoryOutputStream::WriteByte(uint8_t data)
{
	EnsureStreamIsOpened();
	m_data.push_back(data);
}

void MemoryOutputStream::WriteBlock(const void* srcData, std::streamsize size)
{
	EnsureStreamIsOpened();

	if (size == 0)
	{
		return;
	}

	const auto* bytes = static_cast<const uint8_t*>(srcData);
	m_data.insert(m_data.end(), bytes, bytes + size);
}

void MemoryOutputStream::Close()
{
	m_isClosed = true;
}

const std::vector<uint8_t>& MemoryOutputStream::GetData() const
{
	return m_data;
}

void MemoryOutputStream::EnsureStreamIsOpened() const
{
	if (m_isClosed)
	{
		throw std::logic_error("Cannot write to a closed stream");
	}
}
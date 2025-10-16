#pragma once
#include "../streams/IOutputDataStream.h"
#include <memory>

// Абстрактный базовый класс для декораторов выходного потока
class OutputStreamDecorator : public IOutputDataStream
{
public:
	explicit OutputStreamDecorator(std::unique_ptr<IOutputDataStream>&& stream)
		: m_stream(std::move(stream))
	{
	}

	void Close() override
	{
		m_stream->Close();
	}

protected:
	std::unique_ptr<IOutputDataStream> m_stream;
};
#include "../parse/CommandLineParser.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace
{
uint32_t ParseKey(const std::string& keyStr)
{
	if (keyStr.empty())
	{
		throw std::invalid_argument("Key cannot be empty");
	}

	if (!std::ranges::all_of(keyStr, [](unsigned char c) { return std::isdigit(c); }))
	{
		throw std::invalid_argument("Key must be a valid unsigned integer: " + keyStr);
	}

	try
	{
		unsigned long long value = std::stoull(keyStr);
		if (value > UINT32_MAX)
		{
			throw std::invalid_argument("Key value is too large: " + keyStr);
		}
		return static_cast<uint32_t>(value);
	}
	catch (const std::exception&)
	{
		throw std::invalid_argument("Invalid key format: " + keyStr);
	}
}

void ParseOption(const std::string& option, int& index, int argc, char* argv[], CommandLineOptions& options)
{
	if (option == "--encrypt")
	{
		++index;
		if (index >= argc)
		{
			throw std::invalid_argument("--encrypt requires a key argument");
		}
		uint32_t key = ParseKey(argv[index]);
		options.outputOperations.push_back({ OperationType::ENCRYPT, key });
		++index;
	}
	else if (option == "--decrypt")
	{
		++index;
		if (index >= argc)
		{
			throw std::invalid_argument("--decrypt requires a key argument");
		}
		uint32_t key = ParseKey(argv[index]);
		options.inputOperations.push_back({ OperationType::DECRYPT, key });
		++index;
	}
	else if (option == "--compress")
	{
		options.outputOperations.push_back({ OperationType::COMPRESS, 0 });
		++index;
	}
	else if (option == "--decompress")
	{
		options.inputOperations.push_back({ OperationType::DECOMPRESS, 0 });
		++index;
	}
	else
	{
		throw std::invalid_argument("Unknown option: " + option);
	}
}

void ParseOptions(CommandLineOptions& options, int& index, int argc, char* argv[])
{
	while (index < argc)
	{
		std::string arg = argv[index];

		if (arg[0] == '-' && arg[1] == '-')
		{
			ParseOption(arg, index, argc, argv, options);
		}
		else
		{
			// Достигли файлов
			break;
		}
	}
}
} // namespace

CommandLineOptions CommandLineParser::Parse(int argc, char* argv[])
{
	if (argc < 3)
	{
		throw std::invalid_argument("Usage: transform [options] <input-file> <output-file>");
	}

	CommandLineOptions options;
	int i = 1;

	ParseOptions(options, i, argc, argv);

	if (i >= argc)
	{
		throw std::invalid_argument("Input file is missing");
	}
	options.inputFile = argv[i++];

	if (i >= argc)
	{
		throw std::invalid_argument("Output file is missing");
	}
	options.outputFile = argv[i];

	return options;
}
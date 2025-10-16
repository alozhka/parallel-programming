#pragma once
#include <string>
#include <vector>
#include <cstdint>

enum class OperationType
{
	ENCRYPT,
	DECRYPT,
	COMPRESS,
	DECOMPRESS
};

struct Operation
{
	OperationType type;
	uint32_t key = 0;
};

struct CommandLineOptions
{
	std::string inputFile;
	std::string outputFile;
	std::vector<Operation> inputOperations;
	std::vector<Operation> outputOperations;
};

class CommandLineParser
{
public:
	static CommandLineOptions Parse(int argc, char* argv[]);
};
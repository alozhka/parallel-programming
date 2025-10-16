#pragma once
#include <array>
#include <cstdint>
#include <algorithm>
#include <random>

namespace EncryptionUtils
{
	// Генерирует таблицу шифрования на основе ключа
	inline std::array<uint8_t, 256> GenerateEncryptionTable(uint32_t key)
	{
		std::array<uint8_t, 256> table;

		// Инициализируем таблицу значениями 0-255
		for (int i = 0; i < 256; ++i)
		{
			table[i] = static_cast<uint8_t>(i);
		}

		// Перемешиваем используя mt19937 с заданным ключом
		std::mt19937 generator(key);
		std::ranges::shuffle(table, generator);

		return table;
	}

	// Генерирует таблицу дешифрования (обратную к таблице шифрования)
	inline std::array<uint8_t, 256> GenerateDecryptionTable(uint32_t key)
	{
		auto encryptionTable = GenerateEncryptionTable(key);
		std::array<uint8_t, 256> decryptionTable;

		// Создаём обратную таблицу
		for (int i = 0; i < 256; ++i)
		{
			decryptionTable[encryptionTable[i]] = static_cast<uint8_t>(i);
		}

		return decryptionTable;
	}
}
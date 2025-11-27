#include <chrono>
#include <functional>
#include <iostream>
#include <omp.h>

constexpr long long ITERATIONS = 1'00'000'000;

double CalculatePiParallelSequential(long long iterations)
{
	double sum = 0.0;
	for (long long i = 0; i < iterations; i++)
	{
		sum += (i % 2 == 0 ? 1.0 : -1.0) / (2.0 * i + 1.0);
	}

	return 4.0 * sum;
}

double CalculatePiParallelIncorrect(long long iterations)
{
	double sum = 0.0;
#pragma omp parallel for
	for (long long i = 0; i < iterations; i++)
	{
		double term = (i % 2 == 0 ? 1.0 : -1.0) / (2.0 * i + 1.0);
		sum += term;
	}

	return 4.0 * sum;
}

double CalculatePiParallelAtomic(long long iterations)
{
	double sum = 0.0;
#pragma omp parallel for
	for (long long i = 0; i < iterations; i++)
	{
		double term = (i % 2 == 0 ? 1.0 : -1.0) / (2.0 * i + 1.0);
#pragma omp atomic
		sum += term;
	}

	return 4.0 * sum;
}

double CalculatePiParallelReduction(long long iterations)
{
	double sum = 0.0;
#pragma omp parallel for reduction(+ : sum)
	for (long long i = 0; i < iterations; i++)
	{
		double term = (i % 2 == 0 ? 1.0 : -1.0) / (2.0 * i + 1.0);
#pragma omp atomic
		sum += term;
	}

	return 4.0 * sum;
}

void Measure(const std::function<double(long long)>& func)
{
	auto start = omp_get_wtime();

	double pi = func(ITERATIONS);

	auto duration = omp_get_wtime() - start;

	std::cout << "Pi: " << pi << "; time: " << duration << " microseconds" << std::endl;
}

int main()
{
	std::cout << "Sequential\n";
	Measure(CalculatePiParallelSequential);
	std::cout << "Parallel incorrect\n";
	Measure(CalculatePiParallelIncorrect);
	std::cout << "Parallel atomic\n";
	Measure(CalculatePiParallelAtomic);
	std::cout << "Parallel reduction\n";
	Measure(CalculatePiParallelReduction);

	return 0;
}
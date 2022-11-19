#include "pch.h"
#include "CppUnitTest.h"
#include "../ThirdLab/ThirdLab.cpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTestSort
{
	template<typename T>
	T* generateArray(int size, T (*getGeneratedValue)(int)) {
		T* array = new T[size];
		for (int i = 0; i < size; i++) {
			array[i] = getGeneratedValue(i);
		}
		return array;
	}
	template<typename T>
	bool checkSortedArray(int size, T* mass) {
		for (int i = 0; i < size - 1; i++) {
			if (mass[i] > mass[i + 1]) {
				return false;
			}
		}
		return true;
	}

	TEST_CLASS(UnitTestSort)
	{
	public:
		//Сортировка массива INT с 1 элементом
		TEST_METHOD(SortOneIntTest)
		{
			int mass[1] = { 1 };
			QuickSort(mass, mass, [](int a, int b) { return a < b; });
			Assert::IsTrue(mass[0] == 1);
		}
		//Сортировка массива INT с 2 элементами
		TEST_METHOD(SortTwoIntTest)
		{
			int size = 2;
			int* mass = generateArray<int>(size, [](int value) { return value; });
			QuickSort(mass, mass + (size - 1), [](int a, int b) { return a < b; });
			Assert::IsTrue(checkSortedArray<int>(size, mass) == true);
			delete[] mass;
		}
		//Сортировка массива INT со 100 элементами
		TEST_METHOD(SortMoreIntTest)
		{
			int size = 100;
			int* mass0 = generateArray<int>(size, [](int value) { return value; });
			int* mass1 = generateArray<int>(size, [](int value) { return -value; });
			int* mass2 = generateArray<int>(size, [](int value) { return 0; });
			int* mass3 = generateArray<int>(size, [](int value) { return value % 2; });
			int* mass4 = generateArray<int>(size, [](int value) { return rand() % 100; });
			QuickSort(mass0, mass0 + (size - 1), [](int a, int b) { return a < b; });
			QuickSort(mass1, mass1 + (size - 1), [](int a, int b) { return a < b; });
			QuickSort(mass2, mass2 + (size - 1), [](int a, int b) { return a < b; });
			QuickSort(mass3, mass3 + (size - 1), [](int a, int b) { return a < b; });
			QuickSort(mass4, mass4 + (size - 1), [](int a, int b) { return a < b; });
			Assert::IsTrue(checkSortedArray<int>(size, mass0) == true);
			Assert::IsTrue(checkSortedArray<int>(size, mass1) == true);
			Assert::IsTrue(checkSortedArray<int>(size, mass2) == true);
			Assert::IsTrue(checkSortedArray<int>(size, mass3) == true);
			Assert::IsTrue(checkSortedArray<int>(size, mass4) == true);
			delete[] mass0;
			delete[] mass1;
			delete[] mass2;
			delete[] mass3;
			delete[] mass4;
		}
		//Сортировка массива STRING с 1 элементом
		TEST_METHOD(SortOneStringTest)
		{
			std::string mass[1] = { "1" };
			QuickSort(mass, mass, [](std::string a, std::string b) { return a < b; });
			Assert::IsTrue(mass[0] == "1");
		}
		//Сортировка массива STRING с 2 элементами
		TEST_METHOD(SortTwoStringTest)
		{
			int size = 2;
			std::string* mass = generateArray<std::string>(size, [](int value) { return std::string("" + (char)value); });
			QuickSort(mass, mass + size - 1, [](std::string a, std::string b) { return a < b; });
			Assert::IsTrue(checkSortedArray(size,mass) == true);
			delete[] mass;
		}
		//Сортировка массива STRING со 100 элементами
		TEST_METHOD(SortMoreStringTest)
		{
			int size = 100;
			std::string* mass0 = generateArray<std::string>(size, [](int value) { return std::string("" + (char)value); });
			std::string* mass1 = generateArray<std::string>(size, [](int value) { return std::string("1"); });
			std::string* mass2 = generateArray<std::string>(size, [](int value) { return std::string("" + (char)(rand() % 100)); });
			QuickSort(mass0, mass0 + size - 1, [](std::string a, std::string b) { return a < b; });
			QuickSort(mass1, mass1 + size - 1, [](std::string a, std::string b) { return a < b; });
			QuickSort(mass2, mass2 + size - 1, [](std::string a, std::string b) { return a < b; });
			Assert::IsTrue(checkSortedArray(size, mass0) == true);
			Assert::IsTrue(checkSortedArray(size, mass1) == true);
			Assert::IsTrue(checkSortedArray(size, mass2) == true);
			delete[] mass0;
			delete[] mass1;
			delete[] mass2;
		}
	};
}

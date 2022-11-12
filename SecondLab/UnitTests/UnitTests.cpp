#include "pch.h"
#include "CppUnitTest.h"
#include "../SecondLab/SecondLab.cpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTests
{
	TEST_CLASS(UnitTests)
	{
	public:
		//Проверка конструтора по-умолчанию
		TEST_METHOD(DefaultConstructorTest)
		{
			Array<int> obj;
			Assert::IsTrue(obj.pointer() != nullptr);
			Assert::IsTrue(obj.size() == 0);
			Assert::IsTrue(obj.capacity() == obj.defaultCapacity());
		}
		//Проверка конструтора с параметрами
		TEST_METHOD(ParameterConstructorTest)
		{
			int capacity = 5;
			Array<std::string> obj(capacity);
			Assert::IsTrue(obj.pointer() != nullptr);
			Assert::IsTrue(obj.size() == 0);
			Assert::IsTrue(obj.capacity() == capacity);
		}
		//Проверка копирующего конструтора
		TEST_METHOD(CopyConstructorTest)
		{
			Array<int> obj1;
			obj1.insert(10);
			obj1.insert(11);
			obj1.insert(12);
			Array<int> obj2 = obj1;
			Assert::IsTrue(obj2.pointer() != obj1.pointer());
			Assert::IsTrue(obj2.size() == obj1.size());
			Assert::IsTrue(obj2.capacity() == obj1.capacity());
			for (int i = 0; i < obj2.size(); i++) {
				Assert::IsTrue(obj2.pointer()[i] == obj1.pointer()[i]);
			}
		}
		//Проверка копирующего оператора присваивания
		TEST_METHOD(CopyOperatorTest)
		{
			Array<std::string> obj1;
			obj1.insert("A");
			obj1.insert("B");
			obj1.insert("C");
			Array<std::string> obj2;
			obj2 = obj1;
			Assert::IsTrue(obj2.pointer() != obj1.pointer());
			Assert::IsTrue(obj2.size() == obj1.size());
			Assert::IsTrue(obj2.capacity() == obj1.capacity());
			for (int i = 0; i < obj2.size(); i++) {
				Assert::IsTrue(obj2.pointer()[i] == obj1.pointer()[i]);
			}
		}
		//Проверка перемещающего конструктора
		TEST_METHOD(MoveConstructorTest)
		{
			Array<int> obj1, obj2;
			obj1.insert(10);
			obj1.insert(11);
			obj2.insert(20);
			obj2.insert(21);
			Array<int> obj3 = obj1 + obj2;
			Assert::IsTrue(obj3.pointer() != obj1.pointer() && obj3.pointer() != obj2.pointer());
			Assert::IsTrue(obj3.size() == obj1.size() + obj2.size() && obj3.capacity() == obj1.size() + obj2.size());
			for (int i = 0; i < obj1.size(); i++) {
				Assert::IsTrue(obj3.pointer()[i] == obj1.pointer()[i]);
			}
			for (int i = 0; i < obj2.size(); i++) {
				Assert::IsTrue(obj3.pointer()[i + obj1.size()] == obj2.pointer()[i]);
			}
		}
		//Проверка перемещающего оператора
		TEST_METHOD(MoveOperatorTest)
		{
			Array<std::string> obj1, obj2;
			obj1.insert("A");
			obj1.insert("B");
			obj2.insert("C");
			obj2.insert("D");
			Array<std::string> obj3;
			obj3 = obj1 + obj2;
			Assert::IsTrue(obj3.pointer() != obj1.pointer() && obj3.pointer() != obj2.pointer());
			Assert::IsTrue(obj3.size() == obj1.size() + obj2.size() && obj3.capacity() == obj1.size() + obj2.size());
			for (int i = 0; i < obj1.size(); i++) {
				Assert::IsTrue(obj3.pointer()[i] == obj1.pointer()[i]);
			}
			for (int i = 0; i < obj2.size(); i++) {
				Assert::IsTrue(obj3.pointer()[i + obj1.size()] == obj2.pointer()[i]);
			}
		}
		//Проверка работы метода insert
		TEST_METHOD(InsertTest)
		{
			Array<int> obj;
			int a = 10;
			Assert::IsTrue(obj[obj.insert(a)] == a);
			Assert::IsTrue(obj.size() == 1);
		}
		//Проверка вставки по индексу
		TEST_METHOD(InsertMiddleTest)
		{
			Array<int> obj(3);
			obj.insert(0);
			obj.insert(1);
			obj.insert(2);
			obj.insert(1,10);
			Assert::IsTrue(obj[0] == 0);
			Assert::IsTrue(obj[1] == 10);
			Assert::IsTrue(obj[2] == 1);
			Assert::IsTrue(obj[3] == 2);
			Assert::IsTrue(obj.size() == 4);
		}
		//Проверка работы метода remove и класса итератора
		TEST_METHOD(RemoveAndIteratorTest)
		{
			Array<std::string> obj;
			obj.insert("A");
			obj.insert("B");
			obj.insert("C");
			obj.insert("D");
			obj.insert("E");
			obj.remove(1);
			obj.remove(3);
			int i = 0;
			for (auto it = obj.iterator(); it.hasCurrent(); it.next()) {
				Assert::IsTrue(it.get() == obj[i]);
				i++;
			}
			Assert::IsTrue(obj.size() == 3);
		}
		//Проверка работы с "большими" string'ами
		TEST_METHOD(RemoveStringTest)
		{
			Array<std::string> obj;
			obj.insert("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
			obj.insert("BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
			obj.insert("CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
			obj.remove(1);
			Assert::IsTrue(obj[0] == "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
			Assert::IsTrue(obj[1] == "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
			Assert::IsTrue(obj.size() == 2);
		}
	};
}

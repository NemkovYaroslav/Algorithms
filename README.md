Описание методов:
1.  Array() - конструктор по-умолчанию
2.  Array(int capacity) - конструктор с параметрами
3.  Array(const Array& other) - копирующий конструктор
4.  Array& operator = (const Array& other) + void swap(Array& other) - копирующий оператор присваивания, реализован через идиому Copy-Swap Idiom
5.  Array(Array&& other) - перемещающий конструтор, реализует move-семантику
6.  Array& operator=(Array&& other) - перемещающий оператор присваивания
7.  Array operator+(const Array& other) - оператор конкатенации
8.  ~Array() - деструктор
9.  const T& operator[] (int index) const, T& operator[] (int index) - операторы индексирования
10. int insert(int index, const T& element) - вставка некоторого элемента по индексу c сдвигом элементов вправо
11. int insert(const T& element) - вставка некоторого элемента в конец массива
11. void remove(int index) - удаление элемента массива по индексу с сдвигом элементов влево
12. class ConstIterator - класс константного итератора
13. class Iterator : public ConstIterator - класс стандартного итератора

Модульные тесты сделаны на базе CppUnitTestFramework и содержат проверку всех методов

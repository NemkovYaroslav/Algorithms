#include <iostream>
#include <string>

template <typename T>
class Array final {
private:
    T* pi_;
    int size_;
    int capacity_;
    static constexpr int defaultCapacity_ = 8;
    static constexpr int resizeMultiplier = 2;
public:
    //Получение стандартного запаса массива
    int defaultCapacity() const {
        return defaultCapacity_;
    }
    //Получение адреса массива
    T* pointer() const {
        return pi_;
    }
    //Получение размера массива
    int size() const {
        return size_;
    }
    //Получение объема массива
    int capacity() const {
        return capacity_;
    }
    //Конструктор по-умолчанию
    Array() {
        pi_ = (T*)malloc(sizeof(T) * defaultCapacity_);
        size_ = 0;
        capacity_ = defaultCapacity_;
    }
    //Конструктор с параметром
    Array(int capacity) {
        pi_ = (T*)malloc(sizeof(T) * capacity);
        size_ = 0;
        capacity_ = capacity;
    }
    //Копирующий конструктор
    Array(const Array& other) {
        pi_ = (T*)malloc(sizeof(T) * other.capacity_);
        for (int i = 0; i < other.size_; i++) {
            new(pi_ + i) T(other.pi_[i]);
        }
        size_ = other.size_;
        capacity_ = other.capacity_;
    }
    //Копирующий оператор присваивания (Copy-Swap Idiom)
    void swap(Array& other) {
        std::swap(pi_, other.pi_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }
    Array& operator = (const Array& other) {
        if (this != &other) {
            Array<T> temp = other;
            swap(temp);
        }
        return *this;
    }
    //Перемещающий конструктор
    Array(Array&& other) {
        pi_ = other.pi_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        other.pi_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }
    //Перемещающий оператор присваивания
    Array& operator=(Array&& other) {
        if (this != &other) {
            free(pi_);
            pi_ = other.pi_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            other.pi_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }
    Array operator+(const Array& other) {
        Array<T> temp(size_ + other.size_);
        for (int i = 0; i < size_; i++) {
            new (temp.pi_ + i) T(pi_[i]);
            temp.size_++;
        }
        for (int i = 0; i < other.size_; i++) {
            new (temp.pi_ + i + size_) T(other.pi_[i]);
            temp.size_++;
        }
        return temp;
    }
    //Деструктор
    ~Array() {
        for (int i = 0; i < size_; i++) {
            pi_[i].~T();
        }
        free(pi_);
    }
    //Констанстный оператор индексирования
    const T& operator[] (int index) const {
        return pi_[index];
    }
    //Оператор индексирования
    T& operator[] (int index) {
        return pi_[index];
    }
    //Вставка элемента в конец массива
    int insert(const T& element) {
        if (size_ == capacity_) {
            capacity_ *= resizeMultiplier;
            T* temp = (T*)malloc(sizeof(T) * capacity_);
            for (int i = 0; i < size_; i++) {
                new (temp + i) T(std::move(pi_[i]));
                pi_[i].~T();
            }
            free(pi_);
            pi_ = temp;
        }
        new (pi_ + size_) T(element);
        size_++;
        return (size_ - 1);
    }
    //Вставка элемента по индексу в массив
    int insert(int index, const T& element) {
        if (size_ == capacity_) {
            capacity_ *= resizeMultiplier;
            T* temp = (T*)malloc(sizeof(T) * capacity_);
            for (int i = 0; i < index; i++) {
                new (temp + i) T(std::move(pi_[i]));
            }
            new (temp + index) T(element);
            for (int i = index + 1; i < size_ + 1; i++) {
                new (temp + i) T(std::move(pi_[i - 1]));
            }
            for (int i = 0; i < size_; i++) {
                pi_[i].~T();
            }
            free(pi_);
            pi_ = temp;
        }
        else {
            for (int i = index + 1; i < size_ + 1; i++) {
                new (pi_ + i) T(std::move(pi_[i - 1]));
            }
            new (pi_ + index) T(element);
        }
        size_++;
        return index;
    }
    //Удаление элемента массива по индексу
    void remove(int index) {
        pi_[index].~T();
        for (int i = index; i < size_ - 1; i++) {
            new (pi_ + i) T(std::move(pi_[i + 1]));
        }
    }
    //База итераторов
    class BaseIterator {
    protected:
        Array<T> *arrayPtr_;
        T* curPtr_;
        bool isReversed_;
        int direction_;
    public:
        BaseIterator(Array<T> *other, bool isReversed) {
            arrayPtr_ = other;
            isReversed_ = isReversed;
            if (isReversed) {
                direction_ = -1;
                curPtr_ = other->pi_ + (other->size_ - 1);
            }
            else {
                direction_ = 1;
                curPtr_ = other->pi_;
            }
        }
        const T& get() const {
            return *curPtr_;
        }
        void next() {
            curPtr_ += direction_;
        }
        bool hasNext() const {
            return ((curPtr_ >= arrayPtr_->pi_) && (curPtr_ <= arrayPtr_->pi_ + (arrayPtr_->size_ - 1)));
        }
        virtual void set(const T& value) = 0;
    };
    //Итератор
    class Iterator final : public BaseIterator {
    public:
        Iterator(Array<T>* other, bool isReversed) : BaseIterator(other, isReversed) {}
        void set(const T& value) override {
            BaseIterator::curPtr_->~T();
            new (BaseIterator::curPtr_) T(value);
        }
    };
    //Константный итератор
    class ConstIterator final : public BaseIterator {
    public:
        ConstIterator(Array<T>* other, bool isReversed) : BaseIterator(other, isReversed) {}
        void set(const T& value) override = delete;
    };
    //Функции создания итераторов
    Iterator iterator() {
        return Iterator(this,false);
    }
    ConstIterator iterator() const {
        return ConstIterator(this, false);
    }
    Iterator reverseIterator() {
        return Iterator(this, true);
    }
    ConstIterator reverseIterator() const {
        return ConstIterator(this, true);
    }
};

int main()
{
    return EXIT_SUCCESS;
}

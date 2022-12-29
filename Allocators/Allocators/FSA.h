#pragma once
#include <iostream>
#include <windows.h>
#include <vector>
#include <cassert>

class FSA {

private:

#ifdef _DEBUG
	bool isInitialized; // флаг проинициализированного аллокатора
	bool isDestroyed;   // флаг разрушенного аллокатора
	int nFreedBlocks;   // число освобожденных блоков
#endif

	struct Page {
		Page* pNextPage;      // указатель на следующую страницу
		int iFLH;             // индекс головы FreeList'а
		void* pBlocksStart;   // указатель на начало блоков страницы
		int nAllocatedBlocks; // фиксирует число аллоцированных блоков страницы
	};
	Page* pPage;        // текущая страница
	int blockSize;      // задает размерность блоков страницы
	int nLimitBlocks;   // фиксирует число блоков страницы

public:

	FSA () {
	#ifdef _DEBUG
		isInitialized = false;
		isDestroyed = false;
		nFreedBlocks = 0;
	#endif
	}
	~FSA() {
	#ifdef _DEBUG
		assert(isDestroyed && "FSA was not destroyed before deletion");
	#endif
	}

	// выполняет инициализацию аллокатора, запрашивая необходимые страницы памяти у ОС
	// дополнительно резервирует адресное пространство под будущие запросы
	virtual void init(int blockSize, int nLimitBlocks) {
	#ifdef _DEBUG
		assert(!isDestroyed && "FSA was destroyed before initialization");
		isInitialized = true;
		isDestroyed = false;
	#endif
		// задаем инициализируемому аллокатору характеристики
		this->blockSize = blockSize;
		this->nLimitBlocks = nLimitBlocks;
		allocNewPage(pPage);
	}
	void allocNewPage(Page*& pPage) {
		// настраиваем указатель страницы на буфер
		pPage = static_cast<Page*>(VirtualAlloc(NULL, blockSize * nLimitBlocks + sizeof(Page), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
		// следующей страницы после выделенного буфера не существует
		pPage->pNextPage = nullptr;
		// говорим "голове" FreeList'а, что новосозданная страница есть последняя страница
		pPage->iFLH = -1;
		// указатель на начало блоков Page побайтово сдвигаем на размер sizeof(Page)
		pPage->pBlocksStart = static_cast<byte*>(static_cast<void*>(pPage)) + sizeof(Page);
		// число инициализированных блоков ставим 0
		pPage->nAllocatedBlocks = 0;
	}

	// выделяет блок памяти размером this->blockSize байт (то есть, size уже выровнен по границе в 8 байт)
	virtual void* alloc() {
	#ifdef _DEBUG
		assert(isInitialized && "FSA was not initialized before allocation");
	#endif
		Page* pAllocPage = pPage;
		// если это последний свободный блок в текущей странице
		while (pAllocPage->iFLH == -1) {
			// по сути смотрим, можем ли мы создать ещё один блок
			// если число инициализированных блоков < числа блоков страницы
			if (pAllocPage->nAllocatedBlocks < nLimitBlocks) {
				// по сути "нарезаем" адресное пространство блоков
				// создаем указатель на следующую свободную область страницы,
				// побайтово шагая в конец области инициализированных блоков
				void* p = static_cast<byte*>(pAllocPage->pBlocksStart) + (pAllocPage->nAllocatedBlocks * blockSize);
				//std::cout << "STEP_IN: " << (pAllocPage->nAllocatedBlocks * blockSize) << std::endl;
				// увеличиваем число инициализированных блоков страницы
				pAllocPage->nAllocatedBlocks++;
				// возвращаем указатель на свободную область страницы				
				return p;
			}
			// если число уже инициализированных блоков = числу блоков блоков страницы
			// и если указатель на следующую страницу не существует, то:
			if ((pAllocPage->nAllocatedBlocks == nLimitBlocks) && (pAllocPage->pNextPage == nullptr)) {
				// создаем новую страницу
				allocNewPage(pAllocPage->pNextPage);
			}
			// следующая страница становится текущей
			pAllocPage = pAllocPage->pNextPage;
		}
		// возвращает указатель на первый свободный блок из FreeList'а
		void* p = static_cast<byte*>(pAllocPage->pBlocksStart) + pAllocPage->iFLH * blockSize;
		//std::cout << "FreeList block before: " << pAllocPage->iFLH << std::endl;
		pAllocPage->iFLH = *static_cast<int*>(p);
		//std::cout << "FreeList block after: " << pAllocPage->iFLH << std::endl;
		return p;
	}

	// освобождает занимаемую память, на которую указывает указатель p
	virtual bool free(void *p) {
	#ifdef _DEBUG
		assert(isInitialized && "Memory allocator was not initialized before free function");
	#endif
		Page* allocPage = pPage;
		// пока не просмотрим все страницы
		while (allocPage != nullptr) {
			// проверяем не выходит ли переданный для освобождения указатель за пределы выделенных блоков памяти
			if ((static_cast<void*>(allocPage->pBlocksStart) <= p) && (static_cast<void*>(static_cast<byte*>(allocPage->pBlocksStart) + nLimitBlocks * blockSize) > p)) {
				// записываем внутрь свободного блока индекс следующего свободного блока
				*static_cast<int*>(p) = allocPage->iFLH;
				// перенастраиваем голову FreeList'а на текущий свободный блок
				allocPage->iFLH = static_cast<int>((static_cast<byte*>(p) - static_cast<byte*>(allocPage->pBlocksStart)) / blockSize);
				//std::cout << "CURRENT_FREE_BLOCK: " << allocPage->iFLH << std::endl;
			#ifdef _DEBUG
				nFreedBlocks++;
			#endif
				return true;
			}
			// если искомого указателя в данном блоке нет, переходим к поиску внутри следующего блока
			allocPage = allocPage->pNextPage;
		}
		return false;
	}

	// выполнят деинициализацию аллокатора, возвращая всю запрошенную память ОС
	virtual void destroy() {
	#ifdef _DEBUG
		assert(isInitialized && "FSA was not initialized before destruction");
		isDestroyed = true;
		isInitialized = false;
	#endif
		// вызываем метод разбора страниц
		destroyPage(pPage);
		// перенастраиваем указатель после освобождения
		pPage = nullptr;
	}
	void destroyPage(Page*& pPage) {
		// если текущая страница была освобождена - выходим
		if (pPage == nullptr) {
			return;
		}
		// шагаем вглубь до последней страницы
		destroyPage(pPage->pNextPage);
		// освобождаем страницы в обратном порядке
		VirtualFree(static_cast<void*>(pPage), 0, MEM_RELEASE);
	}

#ifdef _DEBUG
	// вывод в стандартный поток вывода список всех занятых блоков, их адреса и размеры
	void dumpBlocks() const {
		assert(isInitialized && "FSA was not initialized before dumpBlocks function");
		std::cout << "  [FSA_" << blockSize << "]:" << std::endl;
		Page* pDBPage = pPage;
		int nDBPage = 0;
		while (pDBPage != nullptr) {
			std::cout << "   Page: " << nDBPage++ << std::endl;
			// здесь осуществляется операция проверки каждого блока страницы на занятость
			for (int nBlock = 0; nBlock < pDBPage->nAllocatedBlocks; nBlock++) {
				bool wasInitialized = true;
				int nIFLH = pDBPage->iFLH;
				// пока не дошли до последнего свободного блока на странице
				while (nIFLH != -1) {
					// содержится ли текущий блок во FreeList'е текущей страницы?
					if (nBlock == nIFLH) {
						// если да, то выходим из цикла и переходим к следующему блоку
						wasInitialized = false;
						break;
					}
					// если нет, то меняем свободный блок на следующий во FreeList'е
					nIFLH = *static_cast<int*>(static_cast<void*>(static_cast<byte*>(pDBPage->pBlocksStart) + nIFLH * blockSize));
				}
				// если блок не является свободным, то выводим подробную информацию о нём
				if (wasInitialized) {
					std::cout << "    Index: " << nBlock << "  Status: Initialized  Adress: " << static_cast<void*>(static_cast<byte*>(pDBPage->pBlocksStart) + nBlock * blockSize) << "  Size: " << blockSize << std::endl;
				}
			}
			// если блоки страницы закончились, то переходим к следующей странице
			pDBPage = pDBPage->pNextPage;
		}
	}
	void dumpStat() const {
		assert(isInitialized && "FSA was not initialized before dumpBlocks function");
		std::cout << "  [FSA_" << blockSize << "]:" << std::endl;
		Page* pDSPage = pPage;
		int nDSPage = 0;
		int nFreeBlocks = 0;
		int nInitializedBlocks = 0;
		while (pDSPage != nullptr) {
			std::cout << "   Page: " << nDSPage++ << std::endl;
			// здесь осуществляется операция проверки каждого блока страницы на занятость
			for (int nBlock = 0; nBlock < pPage->nAllocatedBlocks; nBlock++) {
				bool wasInitialized = true;
				int nIFLH = pDSPage->iFLH;
				// пока не дошли до последнего свободного блока на странице
				while (nIFLH != -1) {
					// содержится ли текущий блок во FreeList'е текущей страницы?
					if (nBlock == nIFLH) {
						// если да, то выходим из цикла и переходим к следующему блоку
						wasInitialized = false;
						// увеличиваем число свободных блоков страницы на единицу
						nFreeBlocks++;
						break;
					}
					// если нет, то меняем свободный блок на следующий во FreeList'е
					nIFLH = *static_cast<int*>(static_cast<void*>(static_cast<byte*>(pDSPage->pBlocksStart) + nIFLH * blockSize));
					// увеличиваем количество занятых блоков страницы на единицу
					nInitializedBlocks++;
				}
				// выводим подробную информацию о блоке
				std::cout << "    Index: " << nBlock << "  Status:";
				if (wasInitialized) {
					std::cout << " Initialized ";
				}
				else {
					std::cout << "    Freed    ";
				}
				std::cout << " Adress: " << static_cast<void*>(static_cast<byte*>(pDSPage->pBlocksStart) + nBlock * blockSize) << "  Size : " << blockSize << std::endl;
			}
			// если блоки страницы закончились, то переходим к следующей странице
			pDSPage = pDSPage->pNextPage;
		}
	}
#endif

};
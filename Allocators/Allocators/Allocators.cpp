#include <iostream>
#include <windows.h>
#include <vector>
#include <cassert>

#include "FSA.h";
#include "CA.h";

constexpr int kOSLimitedSize = 10 * 1048576; // предел в 10МБ, если буфер больше, то запрос передаётся ОС

class MemoryAllocator {

private:

#ifdef _DEBUG
	bool isInitialized; // флаг проинициализированного аллокатора
	bool isDestroyed;   // флаг разрушенного аллокатора
	int nAllocated;     // число аллоцированных блоков
	int nFreedBlocks;         // число освобожденных блоков
#endif

	struct Block {
		void* pData;
		int blockSize;
	};
	// создаем хранилище для блоков, передаваемых под управление ОС
	std::vector<Block> OSBlockStorage;
	// создаем объекты типа FSA для различных размерностей блока
	FSA FSA_16, FSA_32, FSA_64, FSA_128, FSA_256, FSA_512;
	CA CA;
public:

	MemoryAllocator() {
	#ifdef _DEBUG
		isInitialized = false;
		isDestroyed = false;
		nAllocated = 0;
		nFreedBlocks = 0;
	#endif
	}
	virtual ~MemoryAllocator() {
	#ifdef _DEBUG
		assert(isDestroyed && "Memory allocator was not destroyed before deletion");
	#endif
	}

	// выполняет инициализацию аллокатора, запрашивая необходимые страницы памяти у ОС
	virtual void init() {
	#ifdef _DEBUG
		assert(!isDestroyed && "Memory allocator was destroyed before initialization");
		isInitialized = true;
		isDestroyed = false;
	#endif
		// вызываем инициализацию всех доступных FSA'ов и CA
		FSA_16.init(16, 5);
		FSA_32.init(32, 5);
		FSA_64.init(64, 5);
		FSA_128.init(128, 5);
		FSA_256.init(256, 5);
		FSA_512.init(512, 5);
		CA.init(kOSLimitedSize);
	}

	// выполнят деинициализацию аллокатора, возвращая всю запрошенную память ОС 
	virtual void destroy() {
	#ifdef _DEBUG
		assert(isInitialized && "Memory allocator was not initialized before destruction");
		isDestroyed = true;
		isInitialized = false;
	#endif
		// вызываем разбор всех доступных FSA'ов и CA
		FSA_16.destroy();
		FSA_32.destroy();
		FSA_64.destroy();
		FSA_128.destroy();
		FSA_256.destroy();
		FSA_512.destroy();
		CA.destroy();
		// очищаем хранилище блоков под управлением ОС (если такие есть)
		for (int i = 0; i < OSBlockStorage.size(); i++) {
			VirtualFree(OSBlockStorage[i].pData, 0, MEM_RELEASE); // т.к. MEM_RELEASE, то размер ставим 0
		}
	}

	// выделяет блок памяти размером this->blockSize байт (то есть, size уже выровнен по границе в 8 байт) 
	virtual void* alloc(int blockSize) {
	#ifdef _DEBUG
		assert(isInitialized && "Memory allocator was not initialized before allocation");
		nAllocated++;
	#endif
		// по запросу вызываем выделение блоков FSA
		if (blockSize < 16) { return FSA_16.alloc(); }
		if (blockSize < 32) { return FSA_32.alloc(); }
		if (blockSize < 64) { return FSA_64.alloc(); }
		if (blockSize < 128) { return FSA_128.alloc(); }
		if (blockSize < 256) { return FSA_256.alloc(); }
		if (blockSize < 512) { return FSA_512.alloc(); }
		if ((blockSize >= 512) && (blockSize <= kOSLimitedSize)) { return CA.alloc(blockSize); }
		if (blockSize > kOSLimitedSize) {
			void* p = VirtualAlloc(NULL, blockSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			Block OSBlock{};
			OSBlock.pData = p;
			OSBlock.blockSize = blockSize;
			OSBlockStorage.push_back(OSBlock);
			return p;
		}
	}

	// освобождает занимаемую память, на которую указывает указатель p 
	virtual void free(void* p) {
	#ifdef _DEBUG
		assert(isInitialized && "Memory allocator was not initialized before free function");
		nFreedBlocks++;
	#endif
		// проходимся по всем блокам всех аллокаторов и ищем нужный нам указатель для освобождения
		if (FSA_16.free(p)) { return; }
		if (FSA_32.free(p)) { return; }
		if (FSA_64.free(p)) { return; }
		if (FSA_128.free(p)) { return; }
		if (FSA_256.free(p)) { return; }
		if (FSA_512.free(p)) { return; }
		if (CA.free(p)) { return; }
		// проверяем корректность указателя на блок под управлением ОС
		bool VFRes = VirtualFree(p, 0, MEM_RELEASE);
		assert(VFRes && "Memory release error");
		// проходимся по всему хранилищу блоков ОС и вынимаем нужный указатель
		for (auto elem = OSBlockStorage.begin(); elem < OSBlockStorage.end(); elem++) {
			if (static_cast<Block>(*elem).pData == p) {
				OSBlockStorage.erase(elem);
				break;
			}
		}
	}

	#ifdef _DEBUG
	virtual void dumpBlocks() const {
		assert(isInitialized && "Memory allocator was not initialized before dumpBlocks function");
		std::cout << " --------------------------------------------------------------------------- " << std::endl;
		std::cout << "                        INFO ABOUT BUSY BLOCKS STATUS:" << std::endl;
		std::cout << " --------------------------------------------------------------------------- " << std::endl;
		FSA_16.dumpBlocks();
		FSA_32.dumpBlocks();
		FSA_64.dumpBlocks();
		FSA_128.dumpBlocks();
		FSA_256.dumpBlocks();
		FSA_512.dumpBlocks();
		CA.dumpBlocks();
		std::cout << "  [OSA]:" << std::endl;
		for (int i = 0; i < OSBlockStorage.size(); i++) {
			std::cout << "   Index: " << i << " Adress: " << OSBlockStorage[i].pData << " Size: " << OSBlockStorage[i].blockSize << "  Size: " << OSBlockStorage[i].blockSize << std::endl;
		}
		std::cout << " --------------------------------------------------------------------------- " << std::endl;
	}
	virtual void dumpStat() const {
		assert(isInitialized && "Memory allocator was not initialized before dumpStat function");
		std::cout << " --------------------------------------------------------------------------- " << std::endl;
		std::cout << "                         INFO ABOUT BLOCKS STATUS:                       " << std::endl;
		std::cout << " --------------------------------------------------------------------------- " << std::endl;
		std::cout << "                          Initialized " << nAllocated << "  Freed " << nFreedBlocks << std::endl;
		std::cout << " --------------------------------------------------------------------------- " << std::endl;
		FSA_16.dumpStat();
		FSA_32.dumpStat();
		FSA_64.dumpStat();
		FSA_128.dumpStat();
		FSA_256.dumpStat();
		FSA_512.dumpStat();
		CA.dumpStat();
		std::cout << "  [OSA]:" << std::endl;
		for (int i = 0; i < OSBlockStorage.size(); i++) {
			std::cout << "  Block_" << i << " Adress: " << OSBlockStorage[i].pData << " Size: " << OSBlockStorage[i].blockSize << std::endl;
		}
		std::cout << " --------------------------------------------------------------------------- " << std::endl;
	}
	#endif

};

int main()
{
	MemoryAllocator allocator;
	allocator.init();

	///*
	int* pFSA_0 = (int*)allocator.alloc(sizeof(int));
	int* pFSA_1 = (int*)allocator.alloc(sizeof(int));
	int* pFSA_2 = (int*)allocator.alloc(sizeof(int));
	int* pFSA_3 = (int*)allocator.alloc(sizeof(int));
	int* pFSA_4 = (int*)allocator.alloc(sizeof(int));

	int* pFSA_5 = (int*)allocator.alloc(sizeof(int));
	int* pFSA_6 = (int*)allocator.alloc(sizeof(int));
	int* pFSA_7 = (int*)allocator.alloc(sizeof(int));
	int* pFSA_8 = (int*)allocator.alloc(sizeof(int));
	int* pFSA_9 = (int*)allocator.alloc(sizeof(int));

	int* pFSA_10 = (int*)allocator.alloc(sizeof(int) * 20);
	int* pFSA_11 = (int*)allocator.alloc(sizeof(int) * 20);
	int* pFSA_12 = (int*)allocator.alloc(sizeof(int) * 20);
	int* pFSA_13 = (int*)allocator.alloc(sizeof(int) * 20);
	int* pFSA_14 = (int*)allocator.alloc(sizeof(int) * 20);

	allocator.free(pFSA_2);
	allocator.free(pFSA_6);
	allocator.free(pFSA_13);
	allocator.free(pFSA_11);

#ifdef _DEBUG
	allocator.dumpBlocks();
	allocator.dumpStat();
#endif
	//*/

	/*
	int* pCA_0 = (int*)allocator.alloc(kOSLimitedSize/1);
	int* pCA_1 = (int*)allocator.alloc(kOSLimitedSize/2);
	int* pCA_2 = (int*)allocator.alloc(kOSLimitedSize/4);

	//allocator.free(pCA_0);

	int* pCA_3 = (int*)allocator.alloc(kOSLimitedSize/8);
	int* pCA_4 = (int*)allocator.alloc(kOSLimitedSize/10);

	allocator.dumpBlocks();
	allocator.dumpStat();

	allocator.free(pCA_0);
	allocator.free(pCA_1);
	allocator.free(pCA_2);
	allocator.free(pCA_3);
	allocator.free(pCA_4);
	*/

	allocator.destroy();

	return EXIT_SUCCESS;
}
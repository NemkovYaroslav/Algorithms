#pragma once
#include <iostream>
#include <windows.h>
#include <vector>
#include <cassert>

class CA {

private:

#ifdef _DEBUG
	bool isInitialized;     // флаг проинициализированного аллокатора
	bool isDestroyed;       // флаг разрушенного аллокатора
	int nFreedBlocks;       // число освобожденных блоков
#endif

	struct Buffer {
		Buffer* pNextBuffer; // указатель на следующую страницу
		int iFLH;            // индекс головы FreeList'а
		void* pBlocksStart;  // указатель на начало блоков страницы
	};
	Buffer* pBuffer;
	int bufferDataSize;

	struct Block {
		void* pBlockData;
		int blockDataSize;
		bool isFreed;
	};
	int nInitializedBlocks;

public:

	CA() {
	#ifdef _DEBUG
		isInitialized = false;
		isDestroyed = false;
		nFreedBlocks = 0;
	#endif
	}
	~CA() {
	#ifdef _DEBUG
		assert(isDestroyed && "CA was not destroyed before deletion");
	#endif
	}

	virtual void init(int bufferDataSize) {
	#ifdef _DEBUG
		assert(!isDestroyed && "CA was destroyed before initialization");
		isInitialized = true;
		isDestroyed = false;
	#endif
		// размер области данных с точки зрения буфера
		this->bufferDataSize = bufferDataSize;
		allocNewBuffer(this->pBuffer);
	}
	void allocNewBuffer(Buffer*& pBuffer) {
		pBuffer = static_cast<Buffer*>(VirtualAlloc(NULL, bufferDataSize + sizeof(Buffer) + sizeof(Block), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
		pBuffer->pNextBuffer = nullptr;
		pBuffer->iFLH = 0;
		pBuffer->pBlocksStart = static_cast<byte*>(static_cast<void*>(pBuffer)) + sizeof(Buffer);
		Block* pBlock = static_cast<Block*>(pBuffer->pBlocksStart);
		pBlock->pBlockData = static_cast<byte*>(static_cast<void*>(pBlock)) + sizeof(Block);
		pBlock->blockDataSize = bufferDataSize;
		pBlock->isFreed = true;
		*(static_cast<int*>(pBlock->pBlockData)) = -1;
		nInitializedBlocks = 0;
	}

	virtual void* alloc(int requestDataSize) {
	#ifdef _DEBUG
		assert(isInitialized && "FSA was not initialized before allocation");
	#endif
		nInitializedBlocks++;
		// берем имеющийся буфер (голова буфера + голова блока + данные блока)
		Buffer* pAllocBuffer = this->pBuffer;
		while (true) {
			// проверка: есть ли в данном буфере свободный блок?
			if (pAllocBuffer->iFLH != -1) {
				// если в данном буфере есть свободный блок, то:
				
				// шагаем по-байтово в свободный блок
				Block* pBlock = static_cast<Block*>(static_cast<void*>(static_cast<byte*>(pAllocBuffer->pBlocksStart) + pAllocBuffer->iFLH));
				// создаем указатель на предыдущий блок
				Block* pPrevBlock = nullptr;
				while (true) {
					// в шаг FreeList'а записываем байтовый путь до следующего свободного блока
					int stepToFB = *(static_cast<int*>(pBlock->pBlockData));
					// проверка: вместимость свободного блока БОЛЬШЕ или РАВНО запрашиваемой памяти?
					if (pBlock->blockDataSize >= requestDataSize) {
						// если вместимость свободного блока больше или равно запрашиваемой памяти, то:

						// проверка: вместимость свободного блока РАВНО запрашиваемой памяти?
						if (pBlock->blockDataSize == requestDataSize) {
							// если вместимость свободного блока равно запрашиваемой памяти, то

							// проверка: предыдущий блок существует?
							if (pPrevBlock == nullptr) {
								// если предыдущий блок не существует, то:
								
								// в FLH текущего буфера записываем путь до следующего свободного блока
								pAllocBuffer->iFLH = stepToFB;
							}
							else {
								// если предыдущий блок существует, то:
								
								// говорим что предыдущий блок является свободным
								*(static_cast<int*>(pPrevBlock->pBlockData)) = -1;
							}
							// объявляем текущий блок занятым
							pBlock->isFreed = false;
							// возвращаем область данных текущего блока
							return pBlock->pBlockData;
						}
						// если вместимость свободного блока БОЛЬШЕ запрашиваемой памяти, то

						// записываем размер текущего блока
						int oldSize = pBlock->blockDataSize;
						// размер текущего блока заменяем на размер запрашиваемой памяти
						pBlock->blockDataSize = requestDataSize;
						// создаем новый блок с началом в конце области запрашиваемой памяти
						Block* pNewBlock = static_cast<Block*>(static_cast<void*>(static_cast<byte*>(pBlock->pBlockData) + requestDataSize));
						// выделяем блок данных для нового блока с отступом для головы блока
						pNewBlock->pBlockData = static_cast<byte*>(static_cast<void*>(pNewBlock)) + sizeof(Block);
						// размерность нового блока определяем как: старый больший размер - запрашиваемый размер - размер головы
						pNewBlock->blockDataSize = oldSize - requestDataSize - sizeof(Block);
						// говорил что новый блок абсолютно свободен
						*static_cast<int*>(pNewBlock->pBlockData) = -1;
						// объявляем новый блок свободным
						pNewBlock->isFreed = true;

						// проверка: предыдущий блок существует?
						if (pPrevBlock == nullptr) {
							// если предыдущий блок не существует, то:

							// FL буфера настраиваем на шаг от начала блоков текущего буфера до нового свободного блока
							pAllocBuffer->iFLH = static_cast<byte*>(static_cast<void*>(pNewBlock)) - static_cast<byte*>(pAllocBuffer->pBlocksStart);
						}
						else {
							// если предыдущий блок существует, то:

							// в предыдущий блок записываем шаг от начала блоков текущего буфера до нового свободного блока
							*static_cast<int*>(pPrevBlock->pBlockData) = static_cast<byte*>(static_cast<void*>(pNewBlock)) - static_cast<byte*>(pAllocBuffer->pBlocksStart);
						}
						// объявляем текущий блок занятым
						pBlock->isFreed = false;
						// возвращаем область данных текущего блока
						return pBlock->pBlockData;
					}
					// если вместимость свободного блока меньше запрашиваемой памяти, то:
					
					// проверка: следующий свободный блок существует в текущем буфере?
					if (stepToFB == -1) {
						// если следующего свободного блока не существует в текущем буфере

						// то выходим из цикла и переходим к проверке следующего буфера
						break;
					}
					// если следующий свободный блок существует в текущем буфере (просто он находится дальше)

					// записываем текущий блок в качестве предыдущего блока
					pPrevBlock = pBlock;
					// по-байтово шагаем в сторону свободного блока
					pBlock = static_cast<Block*>(static_cast<void*>(static_cast<byte*>(pAllocBuffer->pBlocksStart) + stepToFB));
				}
			}
			// в данном буфере нет свободного блока, то:
			
			// проверка: есть ли следующий буфер?
			if (pAllocBuffer->pNextBuffer == nullptr) {
				// если его нет, то:

				// аллоцируем новых буфер
				allocNewBuffer(pAllocBuffer->pNextBuffer);
			}
			// если он есть, то:

			// переходи в следующий буфер
			pAllocBuffer = pAllocBuffer->pNextBuffer;
		}
	}

	virtual bool free(void* p) {
	#ifdef _DEBUG
		assert(isInitialized && "Memory allocator was not initialized before free function");
	#endif
		Buffer* pAllocBuffer = this->pBuffer;
		// пока не дойдем до последнего буфера
		while (pAllocBuffer != nullptr) {
			// проверка: не выходит ли переданный для освобождения указатель за пределы выделенного буфера?
			if ((static_cast<void*>(pAllocBuffer->pBlocksStart) <= p) && (static_cast<void*>(static_cast<byte*>(pAllocBuffer->pBlocksStart) + this->bufferDataSize + sizeof(Block)) > p)) {
				// если переданный для освобождения указатель не выходит за пределы выделенного буфера, то:
				
				// берем первый блок из буфера
				Block* pAllocBlock = static_cast<Block*>(static_cast<void*>(static_cast<byte*>(pAllocBuffer->pBlocksStart)));
				// создаем указатель на предыдущий блок
				Block* pPrevBlock = nullptr;
				// пока не добрались до нужного блока памяти
				while (!(pAllocBlock->pBlockData == p)) { //////
					// записываем прошедший блок в качестве предыдущего
					pPrevBlock = pAllocBlock;
					// шагаем по текущим блокам данных на величину их размера
					pAllocBlock = static_cast<Block*>(static_cast<void*>(static_cast<byte*>(pAllocBlock->pBlockData) + pAllocBlock->blockDataSize));
				}
				// создаем указатель на следующий блок памяти
				Block* pNextBlock = static_cast<Block*>(static_cast<void*>(static_cast<byte*>(pAllocBlock->pBlockData) + pAllocBlock->blockDataSize));
				
				// проверка: следующий блок находится на границе буфера?
				if (static_cast<void*>(pNextBlock) == (byte*)pAllocBuffer->pBlocksStart + this->bufferDataSize + sizeof(Block)) { //////
					// если следующий блок находится на границе буфера, то:
																																  
					// принимаем следующий блок за nullptr
					pNextBlock = nullptr;
				}
				// если следующий блок не находится на границе буфера, то:
				
				// проверка: следующий блок существует и он свободен?
				if ((pNextBlock != nullptr) && (pNextBlock->isFreed)) {
					// если следующий блок существует и он свободен, то:

					// вводим шаг до следующего свободного блока буфера
					int nextBlockStep = pAllocBuffer->iFLH;
					// вводим предыдущий шаг до свободного блока
					int prevStep = -1;
					// цикл: пока не доберемся до свободного блока справа от текущего блока
					while (static_cast<Block*>(static_cast<void*>(static_cast<byte*>(pAllocBuffer->pBlocksStart) + nextBlockStep)) != pNextBlock) {
						// если всё ещё идем до свободного блока справа от текущего, то:

						// предыдущий шаг становится текущим
						prevStep = nextBlockStep;
						// текущий шаг берет информацию о следующем шаге из поля данных свободного блока справа
						nextBlockStep = *static_cast<int*>(static_cast<Block*>(static_cast<void*>(static_cast<byte*>(pAllocBuffer->pBlocksStart) + nextBlockStep))->pBlockData);
					}
					// если мы дошли до свободного блока справа от текущего, то:

					// проверка: существует ли свободный блок за 1 свободный шаг до свободного блока pNext
					if (prevStep == -1) {
						// если такого свободного блока не существует, то:

						// записываем в текущий свободный блок буфера информацию из pNext
						pAllocBuffer->iFLH = *static_cast<int*>(pNextBlock->pBlockData);
					}
					else {
						// если такой свободного блок существует, то:

						// записываем внутрь этого свободного блока данные о следующем свободном блоке из свободного блока pNext
						*static_cast<int*>(static_cast<Block*>(static_cast<void*>(static_cast<byte*>(pAllocBuffer->pBlocksStart) + prevStep))->pBlockData) = *static_cast<int*>(pNextBlock->pBlockData);
					}
					// объединяем размеры текущего блока и блока pNext
					pAllocBlock->blockDataSize += pNextBlock->blockDataSize + sizeof(Block);
				}
				// если следующий блок либо не существует, либо он не свободен, то:
				
				// проверка: предыдущий блок существует и он свободен?
				if ((pPrevBlock != nullptr) && (pPrevBlock->isFreed)) {
					// если предыдущий блок существует и он свободен, то:

					// объединяем размеры предыдущего свободного блока и текущего свободного блока
					pPrevBlock->blockDataSize += pAllocBlock->blockDataSize + sizeof(Block);
					#ifdef _DEBUG
					nFreedBlocks++;
					#endif
					return true;
				}
				// если предыдущий блок либо не существует, либо он не свободен, то:

				// освобождаем текущий блок
				pAllocBlock->isFreed = true;
				// внутрь текущего блока записываем путь до следующего свободного блока буфера
				*static_cast<int*>(pAllocBlock->pBlockData) = pAllocBuffer->iFLH;
				// путь до следующего свободного блока буфера принимаем как расстояние до текущего блока
				pAllocBuffer->iFLH = static_cast<byte*>(static_cast<void*>(pAllocBlock)) - static_cast<byte*>(pAllocBuffer->pBlocksStart);
				#ifdef _DEBUG
				nFreedBlocks++;
				#endif
				return true;
			}
			// если переданный для освобождения указатель выходит за пределы выделенного буфера, то:

			// переходим в следующий буфер
			pAllocBuffer = pAllocBuffer->pNextBuffer;
		}
		return false;
	}

	virtual void destroy() {
	#ifdef _DEBUG
		assert(isInitialized && "CA was not initialized before destruction");
		isDestroyed = true;
		isInitialized = false;
	#endif
		destroyBuffer(pBuffer);
	}
	void destroyBuffer(Buffer*& pBuffer)
	{
		if (pBuffer == nullptr) {
			return;
		}
		destroyBuffer(pBuffer->pNextBuffer);
		VirtualFree(static_cast<void*>(pBuffer), 0, MEM_RELEASE);
	}

#ifdef _DEBUG
	virtual void dumpBlocks() const {
		assert(isInitialized && "CA was not initialized before dumpBlocks function");
		std::cout << "  [CA]:" << std::endl;
		Buffer* pDBBuffer = pBuffer;
		int nDBPage = 0;
		while (pDBBuffer != nullptr) {
			std::cout << "   Page: " << nDBPage++ << std::endl;
			Block* pBlock = static_cast<Block*>(pDBBuffer->pBlocksStart);
			int nDBBlock = 0;
			if (nInitializedBlocks > 0) {
				while (static_cast<byte*>(static_cast<void*>(pBlock)) - static_cast<byte*>(pDBBuffer->pBlocksStart) < bufferDataSize + sizeof(Block)) {
					if (!(pBlock->isFreed)) {
						std::cout << "    Index: " << nDBBlock << "  Status: Initialized  Adress: " << static_cast<void*>(pBlock) << "  Size: " << pBlock->blockDataSize << std::endl;
					}
					nDBBlock++;
					pBlock = static_cast<Block*>(static_cast<void*>(static_cast<byte*>(pBlock->pBlockData) + pBlock->blockDataSize));
				}
			}
			pDBBuffer = pDBBuffer->pNextBuffer;
		}
	}
	virtual void dumpStat() const {
		assert(isInitialized && "CA was not initialized before dumpStat function");

		std::cout << "  [CA]:" << std::endl;
		Buffer* pDBBuffer = pBuffer;
		int nDBPage = 0;

		while (pDBBuffer != nullptr) {
			std::cout << "   Page: " << nDBPage++ << std::endl;
			Block* pBlock = static_cast<Block*>(pDBBuffer->pBlocksStart);
			int nDBBlock = 0;
			if (nInitializedBlocks > 0) {
				while (static_cast<byte*>(static_cast<void*>(pBlock)) - static_cast<byte*>(pDBBuffer->pBlocksStart) < bufferDataSize + sizeof(Block)) {
					if (!(pBlock->isFreed)) {
						std::cout << "    Index: " << nDBBlock << "  Status: Initialized  Adress: " << static_cast<void*>(pBlock) << "  Size: " << pBlock->blockDataSize << std::endl;
					}
					else {
						std::cout << "    Index: " << nDBBlock << "  Status:    Freed     Adress: " << static_cast<void*>(pBlock) << "  Size: " << pBlock->blockDataSize << std::endl;
					}
					nDBBlock++;
					pBlock = static_cast<Block*>(static_cast<void*>(static_cast<byte*>(pBlock->pBlockData) + pBlock->blockDataSize));
				}
			}
			pDBBuffer = pDBBuffer->pNextBuffer;
		}

	}
#endif

};
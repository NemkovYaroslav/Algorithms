#pragma once
#include <iostream>
#include <windows.h>
#include <vector>
#include <cassert>

class CA {

private:

#ifdef _DEBUG
	bool isInitialized;     // ���� ���������������������� ����������
	bool isDestroyed;       // ���� ������������ ����������
	int nFreedBlocks;       // ����� ������������� ������
#endif

	struct Buffer {
		Buffer* pNextBuffer; // ��������� �� ��������� ��������
		int iFLH;            // ������ ������ FreeList'�
		void* pBlocksStart;  // ��������� �� ������ ������ ��������
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
		// ������ ������� ������ � ����� ������ ������
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
		// ����� ��������� ����� (������ ������ + ������ ����� + ������ �����)
		Buffer* pAllocBuffer = this->pBuffer;
		while (true) {
			// ��������: ���� �� � ������ ������ ��������� ����?
			if (pAllocBuffer->iFLH != -1) {
				// ���� � ������ ������ ���� ��������� ����, ��:
				
				// ������ ��-������� � ��������� ����
				Block* pBlock = static_cast<Block*>(static_cast<void*>(static_cast<byte*>(pAllocBuffer->pBlocksStart) + pAllocBuffer->iFLH));
				// ������� ��������� �� ���������� ����
				Block* pPrevBlock = nullptr;
				while (true) {
					// � ��� FreeList'� ���������� �������� ���� �� ���������� ���������� �����
					int stepToFB = *(static_cast<int*>(pBlock->pBlockData));
					// ��������: ����������� ���������� ����� ������ ��� ����� ������������� ������?
					if (pBlock->blockDataSize >= requestDataSize) {
						// ���� ����������� ���������� ����� ������ ��� ����� ������������� ������, ��:

						// ��������: ����������� ���������� ����� ����� ������������� ������?
						if (pBlock->blockDataSize == requestDataSize) {
							// ���� ����������� ���������� ����� ����� ������������� ������, ��

							// ��������: ���������� ���� ����������?
							if (pPrevBlock == nullptr) {
								// ���� ���������� ���� �� ����������, ��:
								
								// � FLH �������� ������ ���������� ���� �� ���������� ���������� �����
								pAllocBuffer->iFLH = stepToFB;
							}
							else {
								// ���� ���������� ���� ����������, ��:
								
								// ������� ��� ���������� ���� �������� ���������
								*(static_cast<int*>(pPrevBlock->pBlockData)) = -1;
							}
							// ��������� ������� ���� �������
							pBlock->isFreed = false;
							// ���������� ������� ������ �������� �����
							return pBlock->pBlockData;
						}
						// ���� ����������� ���������� ����� ������ ������������� ������, ��

						// ���������� ������ �������� �����
						int oldSize = pBlock->blockDataSize;
						// ������ �������� ����� �������� �� ������ ������������� ������
						pBlock->blockDataSize = requestDataSize;
						// ������� ����� ���� � ������� � ����� ������� ������������� ������
						Block* pNewBlock = static_cast<Block*>(static_cast<void*>(static_cast<byte*>(pBlock->pBlockData) + requestDataSize));
						// �������� ���� ������ ��� ������ ����� � �������� ��� ������ �����
						pNewBlock->pBlockData = static_cast<byte*>(static_cast<void*>(pNewBlock)) + sizeof(Block);
						// ����������� ������ ����� ���������� ���: ������ ������� ������ - ������������� ������ - ������ ������
						pNewBlock->blockDataSize = oldSize - requestDataSize - sizeof(Block);
						// ������� ��� ����� ���� ��������� ��������
						*static_cast<int*>(pNewBlock->pBlockData) = -1;
						// ��������� ����� ���� ���������
						pNewBlock->isFreed = true;

						// ��������: ���������� ���� ����������?
						if (pPrevBlock == nullptr) {
							// ���� ���������� ���� �� ����������, ��:

							// FL ������ ����������� �� ��� �� ������ ������ �������� ������ �� ������ ���������� �����
							pAllocBuffer->iFLH = static_cast<byte*>(static_cast<void*>(pNewBlock)) - static_cast<byte*>(pAllocBuffer->pBlocksStart);
						}
						else {
							// ���� ���������� ���� ����������, ��:

							// � ���������� ���� ���������� ��� �� ������ ������ �������� ������ �� ������ ���������� �����
							*static_cast<int*>(pPrevBlock->pBlockData) = static_cast<byte*>(static_cast<void*>(pNewBlock)) - static_cast<byte*>(pAllocBuffer->pBlocksStart);
						}
						// ��������� ������� ���� �������
						pBlock->isFreed = false;
						// ���������� ������� ������ �������� �����
						return pBlock->pBlockData;
					}
					// ���� ����������� ���������� ����� ������ ������������� ������, ��:
					
					// ��������: ��������� ��������� ���� ���������� � ������� ������?
					if (stepToFB == -1) {
						// ���� ���������� ���������� ����� �� ���������� � ������� ������

						// �� ������� �� ����� � ��������� � �������� ���������� ������
						break;
					}
					// ���� ��������� ��������� ���� ���������� � ������� ������ (������ �� ��������� ������)

					// ���������� ������� ���� � �������� ����������� �����
					pPrevBlock = pBlock;
					// ��-������� ������ � ������� ���������� �����
					pBlock = static_cast<Block*>(static_cast<void*>(static_cast<byte*>(pAllocBuffer->pBlocksStart) + stepToFB));
				}
			}
			// � ������ ������ ��� ���������� �����, ��:
			
			// ��������: ���� �� ��������� �����?
			if (pAllocBuffer->pNextBuffer == nullptr) {
				// ���� ��� ���, ��:

				// ���������� ����� �����
				allocNewBuffer(pAllocBuffer->pNextBuffer);
			}
			// ���� �� ����, ��:

			// �������� � ��������� �����
			pAllocBuffer = pAllocBuffer->pNextBuffer;
		}
	}

	virtual bool free(void* p) {
	#ifdef _DEBUG
		assert(isInitialized && "Memory allocator was not initialized before free function");
	#endif
		Buffer* pAllocBuffer = this->pBuffer;
		// ���� �� ������ �� ���������� ������
		while (pAllocBuffer != nullptr) {
			// ��������: �� ������� �� ���������� ��� ������������ ��������� �� ������� ����������� ������?
			if ((static_cast<void*>(pAllocBuffer->pBlocksStart) <= p) && (static_cast<void*>(static_cast<byte*>(pAllocBuffer->pBlocksStart) + this->bufferDataSize + sizeof(Block)) > p)) {
				// ���� ���������� ��� ������������ ��������� �� ������� �� ������� ����������� ������, ��:
				
				// ����� ������ ���� �� ������
				Block* pAllocBlock = static_cast<Block*>(static_cast<void*>(static_cast<byte*>(pAllocBuffer->pBlocksStart)));
				// ������� ��������� �� ���������� ����
				Block* pPrevBlock = nullptr;
				// ���� �� ��������� �� ������� ����� ������
				while (!(pAllocBlock->pBlockData == p)) { //////
					// ���������� ��������� ���� � �������� �����������
					pPrevBlock = pAllocBlock;
					// ������ �� ������� ������ ������ �� �������� �� �������
					pAllocBlock = static_cast<Block*>(static_cast<void*>(static_cast<byte*>(pAllocBlock->pBlockData) + pAllocBlock->blockDataSize));
				}
				// ������� ��������� �� ��������� ���� ������
				Block* pNextBlock = static_cast<Block*>(static_cast<void*>(static_cast<byte*>(pAllocBlock->pBlockData) + pAllocBlock->blockDataSize));
				
				// ��������: ��������� ���� ��������� �� ������� ������?
				if (static_cast<void*>(pNextBlock) == (byte*)pAllocBuffer->pBlocksStart + this->bufferDataSize + sizeof(Block)) { //////
					// ���� ��������� ���� ��������� �� ������� ������, ��:
																																  
					// ��������� ��������� ���� �� nullptr
					pNextBlock = nullptr;
				}
				// ���� ��������� ���� �� ��������� �� ������� ������, ��:
				
				// ��������: ��������� ���� ���������� � �� ��������?
				if ((pNextBlock != nullptr) && (pNextBlock->isFreed)) {
					// ���� ��������� ���� ���������� � �� ��������, ��:

					// ������ ��� �� ���������� ���������� ����� ������
					int nextBlockStep = pAllocBuffer->iFLH;
					// ������ ���������� ��� �� ���������� �����
					int prevStep = -1;
					// ����: ���� �� ��������� �� ���������� ����� ������ �� �������� �����
					while (static_cast<Block*>(static_cast<void*>(static_cast<byte*>(pAllocBuffer->pBlocksStart) + nextBlockStep)) != pNextBlock) {
						// ���� �� ��� ���� �� ���������� ����� ������ �� ��������, ��:

						// ���������� ��� ���������� �������
						prevStep = nextBlockStep;
						// ������� ��� ����� ���������� � ��������� ���� �� ���� ������ ���������� ����� ������
						nextBlockStep = *static_cast<int*>(static_cast<Block*>(static_cast<void*>(static_cast<byte*>(pAllocBuffer->pBlocksStart) + nextBlockStep))->pBlockData);
					}
					// ���� �� ����� �� ���������� ����� ������ �� ��������, ��:

					// ��������: ���������� �� ��������� ���� �� 1 ��������� ��� �� ���������� ����� pNext
					if (prevStep == -1) {
						// ���� ������ ���������� ����� �� ����������, ��:

						// ���������� � ������� ��������� ���� ������ ���������� �� pNext
						pAllocBuffer->iFLH = *static_cast<int*>(pNextBlock->pBlockData);
					}
					else {
						// ���� ����� ���������� ���� ����������, ��:

						// ���������� ������ ����� ���������� ����� ������ � ��������� ��������� ����� �� ���������� ����� pNext
						*static_cast<int*>(static_cast<Block*>(static_cast<void*>(static_cast<byte*>(pAllocBuffer->pBlocksStart) + prevStep))->pBlockData) = *static_cast<int*>(pNextBlock->pBlockData);
					}
					// ���������� ������� �������� ����� � ����� pNext
					pAllocBlock->blockDataSize += pNextBlock->blockDataSize + sizeof(Block);
				}
				// ���� ��������� ���� ���� �� ����������, ���� �� �� ��������, ��:
				
				// ��������: ���������� ���� ���������� � �� ��������?
				if ((pPrevBlock != nullptr) && (pPrevBlock->isFreed)) {
					// ���� ���������� ���� ���������� � �� ��������, ��:

					// ���������� ������� ����������� ���������� ����� � �������� ���������� �����
					pPrevBlock->blockDataSize += pAllocBlock->blockDataSize + sizeof(Block);
					#ifdef _DEBUG
					nFreedBlocks++;
					#endif
					return true;
				}
				// ���� ���������� ���� ���� �� ����������, ���� �� �� ��������, ��:

				// ����������� ������� ����
				pAllocBlock->isFreed = true;
				// ������ �������� ����� ���������� ���� �� ���������� ���������� ����� ������
				*static_cast<int*>(pAllocBlock->pBlockData) = pAllocBuffer->iFLH;
				// ���� �� ���������� ���������� ����� ������ ��������� ��� ���������� �� �������� �����
				pAllocBuffer->iFLH = static_cast<byte*>(static_cast<void*>(pAllocBlock)) - static_cast<byte*>(pAllocBuffer->pBlocksStart);
				#ifdef _DEBUG
				nFreedBlocks++;
				#endif
				return true;
			}
			// ���� ���������� ��� ������������ ��������� ������� �� ������� ����������� ������, ��:

			// ��������� � ��������� �����
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
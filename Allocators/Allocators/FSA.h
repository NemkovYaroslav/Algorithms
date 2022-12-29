#pragma once
#include <iostream>
#include <windows.h>
#include <vector>
#include <cassert>

class FSA {

private:

#ifdef _DEBUG
	bool isInitialized; // ���� ���������������������� ����������
	bool isDestroyed;   // ���� ������������ ����������
	int nFreedBlocks;   // ����� ������������� ������
#endif

	struct Page {
		Page* pNextPage;      // ��������� �� ��������� ��������
		int iFLH;             // ������ ������ FreeList'�
		void* pBlocksStart;   // ��������� �� ������ ������ ��������
		int nAllocatedBlocks; // ��������� ����� �������������� ������ ��������
	};
	Page* pPage;        // ������� ��������
	int blockSize;      // ������ ����������� ������ ��������
	int nLimitBlocks;   // ��������� ����� ������ ��������

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

	// ��������� ������������� ����������, ���������� ����������� �������� ������ � ��
	// ������������� ����������� �������� ������������ ��� ������� �������
	virtual void init(int blockSize, int nLimitBlocks) {
	#ifdef _DEBUG
		assert(!isDestroyed && "FSA was destroyed before initialization");
		isInitialized = true;
		isDestroyed = false;
	#endif
		// ������ ����������������� ���������� ��������������
		this->blockSize = blockSize;
		this->nLimitBlocks = nLimitBlocks;
		allocNewPage(pPage);
	}
	void allocNewPage(Page*& pPage) {
		// ����������� ��������� �������� �� �����
		pPage = static_cast<Page*>(VirtualAlloc(NULL, blockSize * nLimitBlocks + sizeof(Page), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
		// ��������� �������� ����� ����������� ������ �� ����������
		pPage->pNextPage = nullptr;
		// ������� "������" FreeList'�, ��� ������������� �������� ���� ��������� ��������
		pPage->iFLH = -1;
		// ��������� �� ������ ������ Page ��������� �������� �� ������ sizeof(Page)
		pPage->pBlocksStart = static_cast<byte*>(static_cast<void*>(pPage)) + sizeof(Page);
		// ����� ������������������ ������ ������ 0
		pPage->nAllocatedBlocks = 0;
	}

	// �������� ���� ������ �������� this->blockSize ���� (�� ����, size ��� �������� �� ������� � 8 ����)
	virtual void* alloc() {
	#ifdef _DEBUG
		assert(isInitialized && "FSA was not initialized before allocation");
	#endif
		Page* pAllocPage = pPage;
		// ���� ��� ��������� ��������� ���� � ������� ��������
		while (pAllocPage->iFLH == -1) {
			// �� ���� �������, ����� �� �� ������� ��� ���� ����
			// ���� ����� ������������������ ������ < ����� ������ ��������
			if (pAllocPage->nAllocatedBlocks < nLimitBlocks) {
				// �� ���� "��������" �������� ������������ ������
				// ������� ��������� �� ��������� ��������� ������� ��������,
				// ��������� ����� � ����� ������� ������������������ ������
				void* p = static_cast<byte*>(pAllocPage->pBlocksStart) + (pAllocPage->nAllocatedBlocks * blockSize);
				//std::cout << "STEP_IN: " << (pAllocPage->nAllocatedBlocks * blockSize) << std::endl;
				// ����������� ����� ������������������ ������ ��������
				pAllocPage->nAllocatedBlocks++;
				// ���������� ��������� �� ��������� ������� ��������				
				return p;
			}
			// ���� ����� ��� ������������������ ������ = ����� ������ ������ ��������
			// � ���� ��������� �� ��������� �������� �� ����������, ��:
			if ((pAllocPage->nAllocatedBlocks == nLimitBlocks) && (pAllocPage->pNextPage == nullptr)) {
				// ������� ����� ��������
				allocNewPage(pAllocPage->pNextPage);
			}
			// ��������� �������� ���������� �������
			pAllocPage = pAllocPage->pNextPage;
		}
		// ���������� ��������� �� ������ ��������� ���� �� FreeList'�
		void* p = static_cast<byte*>(pAllocPage->pBlocksStart) + pAllocPage->iFLH * blockSize;
		//std::cout << "FreeList block before: " << pAllocPage->iFLH << std::endl;
		pAllocPage->iFLH = *static_cast<int*>(p);
		//std::cout << "FreeList block after: " << pAllocPage->iFLH << std::endl;
		return p;
	}

	// ����������� ���������� ������, �� ������� ��������� ��������� p
	virtual bool free(void *p) {
	#ifdef _DEBUG
		assert(isInitialized && "Memory allocator was not initialized before free function");
	#endif
		Page* allocPage = pPage;
		// ���� �� ���������� ��� ��������
		while (allocPage != nullptr) {
			// ��������� �� ������� �� ���������� ��� ������������ ��������� �� ������� ���������� ������ ������
			if ((static_cast<void*>(allocPage->pBlocksStart) <= p) && (static_cast<void*>(static_cast<byte*>(allocPage->pBlocksStart) + nLimitBlocks * blockSize) > p)) {
				// ���������� ������ ���������� ����� ������ ���������� ���������� �����
				*static_cast<int*>(p) = allocPage->iFLH;
				// ��������������� ������ FreeList'� �� ������� ��������� ����
				allocPage->iFLH = static_cast<int>((static_cast<byte*>(p) - static_cast<byte*>(allocPage->pBlocksStart)) / blockSize);
				//std::cout << "CURRENT_FREE_BLOCK: " << allocPage->iFLH << std::endl;
			#ifdef _DEBUG
				nFreedBlocks++;
			#endif
				return true;
			}
			// ���� �������� ��������� � ������ ����� ���, ��������� � ������ ������ ���������� �����
			allocPage = allocPage->pNextPage;
		}
		return false;
	}

	// �������� ��������������� ����������, ��������� ��� ����������� ������ ��
	virtual void destroy() {
	#ifdef _DEBUG
		assert(isInitialized && "FSA was not initialized before destruction");
		isDestroyed = true;
		isInitialized = false;
	#endif
		// �������� ����� ������� �������
		destroyPage(pPage);
		// ��������������� ��������� ����� ������������
		pPage = nullptr;
	}
	void destroyPage(Page*& pPage) {
		// ���� ������� �������� ���� ����������� - �������
		if (pPage == nullptr) {
			return;
		}
		// ������ ������ �� ��������� ��������
		destroyPage(pPage->pNextPage);
		// ����������� �������� � �������� �������
		VirtualFree(static_cast<void*>(pPage), 0, MEM_RELEASE);
	}

#ifdef _DEBUG
	// ����� � ����������� ����� ������ ������ ���� ������� ������, �� ������ � �������
	void dumpBlocks() const {
		assert(isInitialized && "FSA was not initialized before dumpBlocks function");
		std::cout << "  [FSA_" << blockSize << "]:" << std::endl;
		Page* pDBPage = pPage;
		int nDBPage = 0;
		while (pDBPage != nullptr) {
			std::cout << "   Page: " << nDBPage++ << std::endl;
			// ����� �������������� �������� �������� ������� ����� �������� �� ���������
			for (int nBlock = 0; nBlock < pDBPage->nAllocatedBlocks; nBlock++) {
				bool wasInitialized = true;
				int nIFLH = pDBPage->iFLH;
				// ���� �� ����� �� ���������� ���������� ����� �� ��������
				while (nIFLH != -1) {
					// ���������� �� ������� ���� �� FreeList'� ������� ��������?
					if (nBlock == nIFLH) {
						// ���� ��, �� ������� �� ����� � ��������� � ���������� �����
						wasInitialized = false;
						break;
					}
					// ���� ���, �� ������ ��������� ���� �� ��������� �� FreeList'�
					nIFLH = *static_cast<int*>(static_cast<void*>(static_cast<byte*>(pDBPage->pBlocksStart) + nIFLH * blockSize));
				}
				// ���� ���� �� �������� ���������, �� ������� ��������� ���������� � ��
				if (wasInitialized) {
					std::cout << "    Index: " << nBlock << "  Status: Initialized  Adress: " << static_cast<void*>(static_cast<byte*>(pDBPage->pBlocksStart) + nBlock * blockSize) << "  Size: " << blockSize << std::endl;
				}
			}
			// ���� ����� �������� �����������, �� ��������� � ��������� ��������
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
			// ����� �������������� �������� �������� ������� ����� �������� �� ���������
			for (int nBlock = 0; nBlock < pPage->nAllocatedBlocks; nBlock++) {
				bool wasInitialized = true;
				int nIFLH = pDSPage->iFLH;
				// ���� �� ����� �� ���������� ���������� ����� �� ��������
				while (nIFLH != -1) {
					// ���������� �� ������� ���� �� FreeList'� ������� ��������?
					if (nBlock == nIFLH) {
						// ���� ��, �� ������� �� ����� � ��������� � ���������� �����
						wasInitialized = false;
						// ����������� ����� ��������� ������ �������� �� �������
						nFreeBlocks++;
						break;
					}
					// ���� ���, �� ������ ��������� ���� �� ��������� �� FreeList'�
					nIFLH = *static_cast<int*>(static_cast<void*>(static_cast<byte*>(pDSPage->pBlocksStart) + nIFLH * blockSize));
					// ����������� ���������� ������� ������ �������� �� �������
					nInitializedBlocks++;
				}
				// ������� ��������� ���������� � �����
				std::cout << "    Index: " << nBlock << "  Status:";
				if (wasInitialized) {
					std::cout << " Initialized ";
				}
				else {
					std::cout << "    Freed    ";
				}
				std::cout << " Adress: " << static_cast<void*>(static_cast<byte*>(pDSPage->pBlocksStart) + nBlock * blockSize) << "  Size : " << blockSize << std::endl;
			}
			// ���� ����� �������� �����������, �� ��������� � ��������� ��������
			pDSPage = pDSPage->pNextPage;
		}
	}
#endif

};
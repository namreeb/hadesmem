// AsmJit - Complete JIT Assembler for C++ Language.

// Copyright (c) 2008-2010, Petr Kobalicek <kobalicek.petr@gmail.com>
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// [Dependencies]
#include "Build.h"
#include "MemoryManager.h"
#include "Platform.h"

#include <stdio.h>
#include <string.h>

#include <new>

// [Api-Begin]
#include "ApiBegin.h"

// This file contains implementation of virtual memory management for AsmJit
// library. The initial concept is to keep this implementation simple but 
// efficient. There are several goals I decided to write implementation myself.
//
// Goals:
// - We need usually to allocate blocks of 64 bytes long and more.
// - Alignment of allocated blocks is large - 32 bytes or 64 bytes.
// - Keep memory manager information outside allocated virtual memory pages
//   (these pages allows execution of code).
// - Keep implementation small.
//
// I think that implementation is not small and probably not too much readable,
// so there is small know how.
//
// - Implementation is based on bit arrays and binary trees. Bit arrays 
//   contains information about allocated and unused blocks of memory. Each
//   block size describes M_Node::density member. Count of blocks are
//   stored in M_Node::blocks member. For example if density is 64 and 
//   count of blocks is 20, memory node contains 64*20 bytes of memory and
//   smallest possible allocation (and also alignment) is 64 bytes. So density
//   describes also memory alignment. Binary trees are used to enable fast
//   lookup into all addresses allocated by memory manager instance. This is
//   used mainly in MemoryManagerPrivate::free().
//
//   Bit array looks like this (empty = unused, X = used) - Size of block 64
//   -------------------------------------------------------------------------
//   | |X|X| | | | | |X|X|X|X|X|X| | | | | | | | | | | | |X| | | | |X|X|X| | |
//   -------------------------------------------------------------------------
//   Bits array shows that there are 12 allocated blocks of 64 bytes, so total 
//   allocated size is 768 bytes. Maximum count of continuous blocks is 12
//   (see largest gap).

namespace AsmJit {

// ============================================================================
// [Bits Manipulation]
// ============================================================================

#define BITS_PER_ENTITY (sizeof(sysuint_t) * 8)

static void _SetBit(sysuint_t* buf, sysuint_t index) ASMJIT_NOTHROW
{
  sysuint_t i = index / BITS_PER_ENTITY; // sysuint_t[]
  sysuint_t j = index % BITS_PER_ENTITY; // sysuint_t[][] bit index

  buf += i;
  *buf |= (sysuint_t)1 << j;
}

static void _ClearBit(sysuint_t* buf, sysuint_t index) ASMJIT_NOTHROW
{
  sysuint_t i = index / BITS_PER_ENTITY; // sysuint_t[]
  sysuint_t j = index % BITS_PER_ENTITY; // sysuint_t[][] bit index

  buf += i;
  *buf &= ~((sysuint_t)1 << j);
}

#if 0
static void _SetBits(sysuint_t* buf, sysuint_t index, sysuint_t len) ASMJIT_NOTHROW
{
  if (len == 0) return;

  sysuint_t i = index / BITS_PER_ENTITY; // sysuint_t[]
  sysuint_t j = index % BITS_PER_ENTITY; // sysuint_t[][] bit index

  // How many bytes process in the first group.
  sysuint_t c = BITS_PER_ENTITY - j;
  if (c > len) c = len;

  // Offset.
  buf += i;

  if (c >= len) 
  {
    *buf |= (((sysuint_t)-1) >> (BITS_PER_ENTITY - len)) << j;
    return;
  }
  else
  {
    //*buf++ |= (((sysuint_t)-1) >> (BITS_PER_ENTITY - c)) << j;
    *buf++ |= ((sysuint_t)-1) << j;
    len -= c;
  }

  while (len >= BITS_PER_ENTITY)
  {
    *buf++ = (sysuint_t)-1;
    len -= BITS_PER_ENTITY;
  }

  if (len)
  {
    *buf |= (((sysuint_t)-1) >> (BITS_PER_ENTITY - len));
  }
}

static void _ClearBits(sysuint_t* buf, sysuint_t index, sysuint_t len) ASMJIT_NOTHROW
{
  if (len == 0) return;

  sysuint_t i = index / BITS_PER_ENTITY; // sysuint_t[]
  sysuint_t j = index % BITS_PER_ENTITY; // sysuint_t[][] bit index

  // How many bytes process in the first group.
  sysuint_t c = BITS_PER_ENTITY - j;

  // Offset.
  buf += i;

  if (c > len)
  {
    *buf &= ~((((sysuint_t)-1) >> (BITS_PER_ENTITY - len)) << j);
    return;
  }
  else
  {
    //*buf++ &= ~((((sysuint_t)-1) >> (BITS_PER_ENTITY - c)) << j);
    *buf++ &= ~(((sysuint_t)-1) << j);
    len -= c;
  }

  while (len >= BITS_PER_ENTITY)
  {
    *buf++ = 0;
    len -= BITS_PER_ENTITY;
  }

  if (len)
  {
    *buf &= ((sysuint_t)-1) << len;
  }
}
#endif

static void _SetBits(sysuint_t* buf, sysuint_t index, sysuint_t len) ASMJIT_NOTHROW
{
  if (len == 0) return;

  sysuint_t i = index / BITS_PER_ENTITY; // sysuint_t[]
  sysuint_t j = index % BITS_PER_ENTITY; // sysuint_t[][] bit index

  // How many bytes process in the first group.
  sysuint_t c = BITS_PER_ENTITY - j;
  if (c > len) c = len;

  // Offset.
  buf += i;

  *buf++ |= (((sysuint_t)-1) >> (BITS_PER_ENTITY - c)) << j;
  len -= c;

  while (len >= BITS_PER_ENTITY)
  {
    *buf++ = (sysuint_t)-1;
    len -= BITS_PER_ENTITY;
  }

  if (len)
  {
    *buf |= (((sysuint_t)-1) >> (BITS_PER_ENTITY - len));
  }
}

static void _ClearBits(sysuint_t* buf, sysuint_t index, sysuint_t len) ASMJIT_NOTHROW
{
  if (len == 0) return;

  sysuint_t i = index / BITS_PER_ENTITY; // sysuint_t[]
  sysuint_t j = index % BITS_PER_ENTITY; // sysuint_t[][] bit index

  // How many bytes process in the first group.
  sysuint_t c = BITS_PER_ENTITY - j;
  if (c > len) c = len;

  // Offset.
  buf += i;

  *buf++ &= ~((((sysuint_t)-1) >> (BITS_PER_ENTITY - c)) << j);
  len -= c;

  while (len >= BITS_PER_ENTITY)
  {
    *buf++ = 0;
    len -= BITS_PER_ENTITY;
  }

  if (len)
  {
    *buf &= ((sysuint_t)-1) << len;
  }
}

// ============================================================================
// [AsmJit::M_Node]
// ============================================================================

#define M_DIV(x, y) ((x) / (y))
#define M_MOD(x, y) ((x) % (y))

struct ASMJIT_HIDDEN M_Node
{
  // --------------------------------------------------------------------------
  // [Node double-linked list]
  // --------------------------------------------------------------------------

  M_Node* prev;            // Prev node in list.
  M_Node* next;            // Next node in list.

  // --------------------------------------------------------------------------
  // [Node LLRB (left leaning red-black) tree, KEY is mem].
  // --------------------------------------------------------------------------

  // Implementation is based on:
  //   Left-leaning Red-Black Trees by Robert Sedgewick.

  M_Node* nlLeft;          // Left node.
  M_Node* nlRight;         // Right node.
  uint32_t nlColor;        // Color (RED or BLACK).

  // --------------------------------------------------------------------------
  // [Chunk Memory]
  // --------------------------------------------------------------------------

  uint8_t* mem;            // Virtual memory address.

  // --------------------------------------------------------------------------
  // [Chunk Data]
  // --------------------------------------------------------------------------

  sysuint_t size;          // How many bytes contain this node.
  sysuint_t blocks;        // How many blocks are here.
  sysuint_t density;       // Minimum count of allocated bytes in this node (also alignment).
  sysuint_t used;          // How many bytes are used in this node.
  sysuint_t largestBlock;  // Contains largest block that can be allocated.
  sysuint_t* baUsed;       // Contains bits about used blocks.
                           // (0 = unused, 1 = used).
  sysuint_t* baCont;       // Contains bits about continuous blocks.
                           // (0 = stop, 1 = continue).

  // --------------------------------------------------------------------------
  // [Enums]
  // --------------------------------------------------------------------------

  enum NODE_COLOR
  {
    NODE_BLACK = 0,
    NODE_RED = 1
  };

  // --------------------------------------------------------------------------
  // [Methods]
  // --------------------------------------------------------------------------

  // Get available space.
  inline sysuint_t getAvailable() const ASMJIT_NOTHROW { return size - used; }
};

// ============================================================================
// [AsmJit::M_Permanent]
// ============================================================================

//! @brief Permanent node.
struct ASMJIT_HIDDEN M_PermanentNode
{
  uint8_t* mem;            // Base pointer (virtual memory address).
  sysuint_t size;          // Count of bytes allocated.
  sysuint_t used;          // Count of bytes used.
  M_PermanentNode* prev;   // Pointer to prev chunk or NULL.

  // Get available space.
  inline sysuint_t getAvailable() const ASMJIT_NOTHROW { return size - used; }
};

// ============================================================================
// [AsmJit::MemoryManagerPrivate]
// ============================================================================

struct ASMJIT_HIDDEN MemoryManagerPrivate
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

#if !defined(ASMJIT_WINDOWS)
  MemoryManagerPrivate() ASMJIT_NOTHROW;
#else
  MemoryManagerPrivate(HANDLE hProcess) ASMJIT_NOTHROW;
#endif // ASMJIT_WINDOWS
  ~MemoryManagerPrivate() ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Allocation]
  // --------------------------------------------------------------------------

  M_Node* createNode(sysuint_t size, sysuint_t density) ASMJIT_NOTHROW;

  void* allocPermanent(sysuint_t vsize) ASMJIT_NOTHROW;
  void* allocFreeable(sysuint_t vsize) ASMJIT_NOTHROW;

  bool free(void* address) ASMJIT_NOTHROW;
  void freeAll(bool keepVirtualMemory) ASMJIT_NOTHROW;

  // Helpers to avoid ifdefs in the code.
  inline uint8_t* allocVirtualMemory(sysuint_t size, sysuint_t* vsize) ASMJIT_NOTHROW
  {
#if !defined(ASMJIT_WINDOWS)
    return (uint8_t*)VirtualMemory::alloc(size, vsize, true);
#else
    return (uint8_t*)VirtualMemory::allocProcessMemory(_hProcess, size, vsize, true);
#endif
  }

  inline void freeVirtualMemory(void* vmem, sysuint_t vsize) ASMJIT_NOTHROW
  {
#if !defined(ASMJIT_WINDOWS)
    VirtualMemory::free(vmem, vsize);
#else
    VirtualMemory::freeProcessMemory(_hProcess, vmem, vsize);
#endif
  }

  // --------------------------------------------------------------------------
  // [NodeList LLRB-Tree]
  // --------------------------------------------------------------------------

  static bool nlCheckTree(M_Node* node) ASMJIT_NOTHROW;

  static inline bool nlIsRed(M_Node* n) ASMJIT_NOTHROW;
  static M_Node* nlRotateLeft(M_Node* n) ASMJIT_NOTHROW;
  static M_Node* nlRotateRight(M_Node* n) ASMJIT_NOTHROW;
  static inline void nlFlipColor(M_Node* n) ASMJIT_NOTHROW;
  static M_Node* nlMoveRedLeft(M_Node* h) ASMJIT_NOTHROW;
  static M_Node* nlMoveRedRight(M_Node* h) ASMJIT_NOTHROW;
  static inline M_Node* nlFixUp(M_Node* h) ASMJIT_NOTHROW;

  inline void nlInsertNode(M_Node* n) ASMJIT_NOTHROW;
  M_Node* nlInsertNode_(M_Node* h, M_Node* n) ASMJIT_NOTHROW;

  inline void nlRemoveNode(M_Node* n) ASMJIT_NOTHROW;
  M_Node* nlRemoveNode_(M_Node* h, M_Node* n) ASMJIT_NOTHROW;
  M_Node* nlRemoveMin(M_Node* h) ASMJIT_NOTHROW;

  M_Node* nlFindPtr(uint8_t* mem) ASMJIT_NOTHROW;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_WINDOWS)
  HANDLE _hProcess;            // Process where to allocate memory.
#endif // ASMJIT_WINDOWS
  Lock _lock;                  // Lock for thread safety.

  sysuint_t _newChunkSize;     // Default node size.
  sysuint_t _newChunkDensity;  // Default node density.
  sysuint_t _allocated;        // How many bytes are allocated.
  sysuint_t _used;             // How many bytes are used.

  // Memory nodes list.
  M_Node* _first;
  M_Node* _last;
  M_Node* _optimal;

  // Memory nodes tree.
  M_Node* _root;

  // Permanent memory.
  M_PermanentNode* _permanent;

  // Whether to keep virtual memory after destroy.
  bool _keepVirtualMemory;
};

// ============================================================================
// [AsmJit::MemoryManagerPrivate - Construction / Destruction]
// ============================================================================

#if !defined(ASMJIT_WINDOWS)
MemoryManagerPrivate::MemoryManagerPrivate() ASMJIT_NOTHROW :
#else
MemoryManagerPrivate::MemoryManagerPrivate(HANDLE hProcess) ASMJIT_NOTHROW :
  _hProcess(hProcess),
#endif
  _newChunkSize(65536),
  _newChunkDensity(64),
  _allocated(0),
  _used(0),
  _root(NULL),
  _first(NULL),
  _last(NULL),
  _optimal(NULL),
  _permanent(NULL),
  _keepVirtualMemory(false)
{
}

MemoryManagerPrivate::~MemoryManagerPrivate() ASMJIT_NOTHROW
{
  // Freeable memory cleanup - Also frees the virtual memory if configured to.
  freeAll(_keepVirtualMemory);

  // Permanent memory cleanup - Never frees the virtual memory.
  M_PermanentNode* node = _permanent;
  while (node)
  {
    M_PermanentNode* prev = node->prev;
    ASMJIT_FREE(node);
    node = prev;
  }
}

// ============================================================================
// [AsmJit::MemoryManagerPrivate - Allocation]
// ============================================================================

// allocates virtual memory node and M_Node structure.
//
// returns M_Node* if success, otherwise NULL
M_Node* MemoryManagerPrivate::createNode(sysuint_t size, sysuint_t density) ASMJIT_NOTHROW
{
  sysuint_t vsize;
  uint8_t* vmem = allocVirtualMemory(size, &vsize);

  // Out of memory.
  if (vmem == NULL) return NULL;

  sysuint_t blocks = (vsize / density);
  sysuint_t basize = (((blocks + 7) >> 3) + sizeof(sysuint_t) - 1) & ~(sysuint_t)(sizeof(sysuint_t)-1);
  sysuint_t memSize = sizeof(M_Node) + (basize * 2);

  M_Node* node = (M_Node*)ASMJIT_MALLOC(memSize);

  // Out of memory.
  if (node == NULL)
  {
    freeVirtualMemory(vmem, vsize);
    return NULL;
  }

  memset(node, 0, memSize);

  node->nlColor = M_Node::NODE_RED;
  node->mem = vmem;

  node->size = vsize;
  node->blocks = blocks;
  node->density = density;
  node->largestBlock = vsize;
  node->baUsed = (sysuint_t*)( (uint8_t*)node + sizeof(M_Node) );
  node->baCont = (sysuint_t*)( (uint8_t*)node->baUsed + basize );

  return node;
}

void* MemoryManagerPrivate::allocPermanent(sysuint_t vsize) ASMJIT_NOTHROW
{
  static const sysuint_t permanentAlignment = 32;
  static const sysuint_t permanentNodeSize  = 32768;

  sysuint_t over = vsize % permanentAlignment;
  if (over) over = permanentAlignment - over;
  sysuint_t alignedSize = vsize + over;

  AutoLock locked(_lock);

  M_PermanentNode* node = _permanent;

  // Try to find space in allocated chunks.
  while (node && alignedSize > node->getAvailable()) node = node->prev;

  // Or allocate new node.
  if (!node)
  {
    sysuint_t nodeSize = permanentNodeSize;
    if (vsize > nodeSize) nodeSize = vsize;

    node = (M_PermanentNode*)ASMJIT_MALLOC(sizeof(M_PermanentNode));
    // Out of memory.
    if (node == NULL) return NULL;

    node->mem = allocVirtualMemory(nodeSize, &node->size);
    // Out of memory.
    if (node->mem == NULL) 
    {
      ASMJIT_FREE(node);
      return NULL;
    }

    node->used = 0;
    node->prev = _permanent;
    _permanent = node;
  }

  // Finally, copy function code to our space we reserved for.
  uint8_t* result = node->mem + node->used;

  // Update Statistics.
  node->used += alignedSize;
  _used += alignedSize;

  // Code can be null to only reserve space for code.
  return (void*)result;
}

void* MemoryManagerPrivate::allocFreeable(sysuint_t vsize) ASMJIT_NOTHROW
{
  sysuint_t i;               // Current index.
  sysuint_t need;            // How many we need to be freed.
  sysuint_t minVSize;

  // Align to 32 bytes (our default alignment).
  vsize = (vsize + 31) & ~(sysuint_t)31;
  if (vsize == 0) return NULL;

  AutoLock locked(_lock);
  M_Node* node = _optimal;

  minVSize = _newChunkSize;

  // Try to find memory block in existing nodes.
  while (node)
  {
    // Skip this node?
    if ((node->getAvailable() < vsize) || 
        (node->largestBlock < vsize && node->largestBlock != 0))
    {
      M_Node* next = node->next;
      if (node->getAvailable() < minVSize && node == _optimal && next) _optimal = next;
      node = next;
      continue;
    }

    sysuint_t* up = node->baUsed;    // Current ubits address.
    sysuint_t ubits;                 // Current ubits[0] value.
    sysuint_t bit;                   // Current bit mask.
    sysuint_t blocks = node->blocks; // Count of blocks in node.
    sysuint_t cont = 0;              // How many bits are currently freed in find loop.
    sysuint_t maxCont = 0;           // Largest continuous block (bits count).
    sysuint_t j;

    need = M_DIV((vsize + node->density - 1), node->density);
    i = 0;

    // Try to find node that is large enough.
    while (i < blocks)
    {
      ubits = *up++;

      // Fast skip used blocks.
      if (ubits == (sysuint_t)-1)
      { 
        if (cont > maxCont) maxCont = cont;
        cont = 0;

        i += BITS_PER_ENTITY;
        continue;
      }

      sysuint_t max = BITS_PER_ENTITY;
      if (i + max > blocks) max = blocks - i;

      for (j = 0, bit = 1; j < max; bit <<= 1)
      {
        j++;
        if ((ubits & bit) == 0)
        {
          if (++cont == need) { i += j; i -= cont; goto found; }
          continue;
        }

        if (cont > maxCont) maxCont = cont;
        cont = 0;
      }

      i += BITS_PER_ENTITY;
    }

    // Because we traversed entire node, we can set largest node size that
    // will be used to cache next traversing..
    node->largestBlock = maxCont * node->density;

    node = node->next;
  }

  // If we are here, we failed to find existing memory block and we must
  // allocate new.
  {
    sysuint_t chunkSize = _newChunkSize;
    if (chunkSize < vsize) chunkSize = vsize;

    node = createNode(chunkSize, _newChunkDensity);
    if (node == NULL) return NULL;

    // Link with others.
    node->prev = _last;

    if (_first == NULL)
    {
      _first = node;
      _last = node;
      _optimal = node;
    }
    else
    {
      node->prev = _last;
      _last->next = node;
      _last = node;
    }

    // Update binary tree.
    nlInsertNode(node);

    // Alloc first node at start.
    i = 0;
    need = (vsize + node->density - 1) / node->density;

    // Update statistics.
    _allocated += node->size;
  }

found:
  //printf("ALLOCATED BLOCK %p (%d) \n", node->mem + i * node->density, need * node->density);

  // Update bits.
  _SetBits(node->baUsed, i, need);
  _SetBits(node->baCont, i, need - 1);

  // Update statistics.
  {
    sysuint_t u = need * node->density;
    node->used += u;
    node->largestBlock = 0;
    _used += u;
  }

  // And return pointer to allocated memory.
  uint8_t* result = node->mem + i * node->density;
  ASMJIT_ASSERT(result >= node->mem && result < node->mem + node->size);
  return result;
}

bool MemoryManagerPrivate::free(void* address) ASMJIT_NOTHROW
{
  if (address == NULL) return true;

  AutoLock locked(_lock);

  M_Node* node = nlFindPtr((uint8_t*)address);
  if (node == NULL)
    return false;

  sysuint_t offset = (sysuint_t)((uint8_t*)address - (uint8_t*)node->mem);
  sysuint_t bitpos = M_DIV(offset, node->density);
  sysuint_t i = (bitpos / BITS_PER_ENTITY);
  sysuint_t j = (bitpos % BITS_PER_ENTITY);

  sysuint_t* up = node->baUsed + i;  // Current ubits address.
  sysuint_t* cp = node->baCont + i;  // Current cbits address.
  sysuint_t ubits = *up;             // Current ubits[0] value.
  sysuint_t cbits = *cp;             // Current cbits[0] value.
  sysuint_t bit = (sysuint_t)1 << j; // Current bit mask.

  sysuint_t cont = 0;

  bool stop;

  for (;;)
  {
    stop = (cbits & bit) == 0;
    ubits &= ~bit;
    cbits &= ~bit;

    j++;
    bit <<= 1;
    cont++;

    if (stop || j == BITS_PER_ENTITY)
    {
      *up = ubits;
      *cp = cbits;

      if (stop) break;

      ubits = *++up;
      cbits = *++cp;

      j = 0;
      bit = 1;
    }
  }

  // If the freed block is fully allocated node, need to update optimal
  // pointer in memory manager.
  if (node->used == node->size)
  {
    M_Node* cur = _optimal;

    do {
      cur = cur->prev;
      if (cur == node) { _optimal = node; break; }
    } while (cur);
  }

  //printf("FREEING %p (%d)\n", address, cont * node->density);

  // Statistics.
  cont *= node->density;
  if (node->largestBlock < cont) node->largestBlock = cont;
  node->used -= cont;
  _used -= cont;

  // If page is empty, we can free it.
  if (node->used == 0)
  {
    _allocated -= node->size;
    nlRemoveNode(node);
    freeVirtualMemory(node->mem, node->size);

    M_Node* next = node->next;
    M_Node* prev = node->prev;

    if (prev) { prev->next = next; } else { _first = next; }
    if (next) { next->prev = prev; } else { _last  = prev; }
    if (_optimal == node) { _optimal = prev ? prev : next; }

    ASMJIT_FREE(node);
  }

  return true;
}

void MemoryManagerPrivate::freeAll(bool keepVirtualMemory) ASMJIT_NOTHROW
{
  M_Node* node = _first;

  while (node)
  {
    M_Node* next = node->next;
    
    if (!keepVirtualMemory) freeVirtualMemory(node->mem, node->size);
    ASMJIT_FREE(node);

    node = next;
  }

  _allocated = 0;
  _used = 0;

  _root = NULL;
  _first = NULL;
  _last = NULL;
  _optimal = NULL;
}

// ============================================================================
// [AsmJit::MemoryManagerPrivate - NodeList LLRB-Tree]
// ============================================================================

bool MemoryManagerPrivate::nlCheckTree(M_Node* node) ASMJIT_NOTHROW
{
  bool result = true;
  if (node == NULL) return result;

  if (node->nlLeft && node->mem < node->nlLeft->mem)
    return false;
  if (node->nlRight && node->mem > node->nlRight->mem)
    return false;

  if (node->nlLeft) result &= nlCheckTree(node->nlLeft);
  if (node->nlRight) result &= nlCheckTree(node->nlRight);
  return result;
}

inline bool MemoryManagerPrivate::nlIsRed(M_Node* n) ASMJIT_NOTHROW
{
  return n && n->nlColor == M_Node::NODE_RED;
}

inline M_Node* MemoryManagerPrivate::nlRotateLeft(M_Node* n) ASMJIT_NOTHROW
{
  M_Node* x = n->nlRight;

  n->nlRight = x->nlLeft;
  x->nlLeft = n;

  x->nlColor = n->nlColor;
  n->nlColor = M_Node::NODE_RED;

  return x;
}

inline M_Node* MemoryManagerPrivate::nlRotateRight(M_Node* n) ASMJIT_NOTHROW
{
  M_Node* x = n->nlLeft;

  n->nlLeft = x->nlRight;
  x->nlRight = n;

  x->nlColor = n->nlColor;
  n->nlColor = M_Node::NODE_RED;

  return x;
}

inline void MemoryManagerPrivate::nlFlipColor(M_Node* n) ASMJIT_NOTHROW
{
  ASMJIT_ASSERT(n->nlLeft != NULL);
  ASMJIT_ASSERT(n->nlRight != NULL);

  n->nlColor = !n->nlColor;
  n->nlLeft->nlColor = !(n->nlLeft->nlColor);
  n->nlRight->nlColor = !(n->nlRight->nlColor);
}

M_Node* MemoryManagerPrivate::nlMoveRedLeft(M_Node* h) ASMJIT_NOTHROW
{
  nlFlipColor(h);
  if (nlIsRed(h->nlRight->nlLeft))
  {
    h->nlRight = nlRotateRight(h->nlRight);
    h = nlRotateLeft(h);
    nlFlipColor(h);
  }
  return h;
}

M_Node* MemoryManagerPrivate::nlMoveRedRight(M_Node* h) ASMJIT_NOTHROW
{
  nlFlipColor(h);
  if (nlIsRed(h->nlLeft->nlLeft))
  {
    h = nlRotateRight(h);
    nlFlipColor(h);
  }
  return h;
}

inline M_Node* MemoryManagerPrivate::nlFixUp(M_Node* h) ASMJIT_NOTHROW
{
  if (nlIsRed(h->nlRight))
    h = nlRotateLeft(h);
  if (nlIsRed(h->nlLeft) && nlIsRed(h->nlLeft->nlLeft))
    h = nlRotateRight(h);
  if (nlIsRed(h->nlLeft) && nlIsRed(h->nlRight))
    nlFlipColor(h);

  return h;
}

inline void MemoryManagerPrivate::nlInsertNode(M_Node* n) ASMJIT_NOTHROW
{
  _root = nlInsertNode_(_root, n);
  ASMJIT_ASSERT(nlCheckTree(_root));
}

M_Node* MemoryManagerPrivate::nlInsertNode_(M_Node* h, M_Node* n) ASMJIT_NOTHROW
{
  if (h == NULL) return n;

  if (n->mem < h->mem)
    h->nlLeft = nlInsertNode_(h->nlLeft, n);
  else
    h->nlRight = nlInsertNode_(h->nlRight, n);

  if (nlIsRed(h->nlRight) && !nlIsRed(h->nlLeft)) h = nlRotateLeft(h);
  if (nlIsRed(h->nlLeft) && nlIsRed(h->nlLeft->nlLeft)) h = nlRotateRight(h);

  if (nlIsRed(h->nlLeft) && nlIsRed(h->nlRight)) nlFlipColor(h);

  return h;
}

void MemoryManagerPrivate::nlRemoveNode(M_Node* n) ASMJIT_NOTHROW
{
  _root = nlRemoveNode_(_root, n);
  if (_root) _root->nlColor = M_Node::NODE_BLACK;

  ASMJIT_ASSERT(nlFindPtr(n->mem) == NULL);
  ASMJIT_ASSERT(nlCheckTree(_root));
}

static M_Node* findParent(M_Node* root, M_Node* n) ASMJIT_NOTHROW
{
  M_Node* parent = NULL;
  M_Node* cur = root;
  uint8_t* mem = n->mem;
  uint8_t* curMem;
  uint8_t* curEnd;

  while (cur)
  {
    curMem = cur->mem;
    if (mem < curMem)
    {
      parent = cur;
      cur = cur->nlLeft;
      continue;
    }
    else
    {
      curEnd = curMem + cur->size;
      if (mem >= curEnd)
      {
        parent = cur;
        cur = cur->nlRight;
        continue;
      }
      return parent;
    }
  }

  return NULL;
}

M_Node* MemoryManagerPrivate::nlRemoveNode_(M_Node* h, M_Node* n) ASMJIT_NOTHROW
{
  if (n->mem < h->mem)
  {
    if (!nlIsRed(h->nlLeft) && !nlIsRed(h->nlLeft->nlLeft))
      h = nlMoveRedLeft(h);
    h->nlLeft = nlRemoveNode_(h->nlLeft, n);
  }
  else
  {
    if (nlIsRed(h->nlLeft))
      h = nlRotateRight(h);
    if (h == n && (h->nlRight == NULL))
      return NULL;
    if (!nlIsRed(h->nlRight) && !nlIsRed(h->nlRight->nlLeft))
      h = nlMoveRedRight(h);
    if (h == n)
    {
      // Get minimum node.
      h = n->nlRight;
      while (h->nlLeft) h = h->nlLeft;

      M_Node* _l = n->nlLeft;
      M_Node* _r = nlRemoveMin(n->nlRight);

      h->nlLeft = _l;
      h->nlRight = _r;
      h->nlColor = n->nlColor;
    }
    else
    {
      h->nlRight = nlRemoveNode_(h->nlRight, n);
    }
  }

  return nlFixUp(h);
}

M_Node* MemoryManagerPrivate::nlRemoveMin(M_Node* h) ASMJIT_NOTHROW
{
  if (h->nlLeft == NULL) return NULL;
  if (!nlIsRed(h->nlLeft) && !nlIsRed(h->nlLeft->nlLeft))
    h = nlMoveRedLeft(h);
  h->nlLeft = nlRemoveMin(h->nlLeft);
  return nlFixUp(h);
}

M_Node* MemoryManagerPrivate::nlFindPtr(uint8_t* mem) ASMJIT_NOTHROW
{
  M_Node* cur = _root;
  while (cur)
  {
    uint8_t* curMem = cur->mem;
    if (mem < curMem)
    {
      cur = cur->nlLeft;
      continue;
    }
    else
    {
      uint8_t* curEnd = curMem + cur->size;
      if (mem >= curEnd)
      {
        cur = cur->nlRight;
        continue;
      }
      break;
    }
  }
  return cur;
}

// ============================================================================
// [AsmJit::MemoryManager]
// ============================================================================

MemoryManager::MemoryManager() ASMJIT_NOTHROW
{
}

MemoryManager::~MemoryManager() ASMJIT_NOTHROW
{
}

MemoryManager* MemoryManager::getGlobal() ASMJIT_NOTHROW
{
  static VirtualMemoryManager memmgr;
  return &memmgr;
}

// ============================================================================
// [AsmJit::VirtualMemoryManager]
// ============================================================================

#if !defined(ASMJIT_WINDOWS)
VirtualMemoryManager::VirtualMemoryManager() ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = new(std::nothrow) MemoryManagerPrivate();
  _d = (void*)d;
}
#else
VirtualMemoryManager::VirtualMemoryManager() ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = new(std::nothrow) MemoryManagerPrivate(GetCurrentProcess());
  _d = (void*)d;
}

VirtualMemoryManager::VirtualMemoryManager(HANDLE hProcess) ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = new(std::nothrow) MemoryManagerPrivate(hProcess);
  _d = (void*)d;
}
#endif // ASMJIT_WINDOWS

VirtualMemoryManager::~VirtualMemoryManager() ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = reinterpret_cast<MemoryManagerPrivate*>(_d);
  delete d;
}

void* VirtualMemoryManager::alloc(sysuint_t size, uint32_t type) ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = reinterpret_cast<MemoryManagerPrivate*>(_d);

  if (type == MEMORY_ALLOC_PERMANENT) 
    return d->allocPermanent(size);
  else
    return d->allocFreeable(size);
}

bool VirtualMemoryManager::free(void* address) ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = reinterpret_cast<MemoryManagerPrivate*>(_d);
  return d->free(address);
}

void VirtualMemoryManager::freeAll() ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = reinterpret_cast<MemoryManagerPrivate*>(_d);

  // Calling MemoryManager::freeAll() will never keep allocated memory.
  return d->freeAll(false);
}

sysuint_t VirtualMemoryManager::getUsedBytes() ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = reinterpret_cast<MemoryManagerPrivate*>(_d);
  return d->_used;
}

sysuint_t VirtualMemoryManager::getAllocatedBytes() ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = reinterpret_cast<MemoryManagerPrivate*>(_d);
  return d->_allocated;
}

bool VirtualMemoryManager::getKeepVirtualMemory() const ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = reinterpret_cast<MemoryManagerPrivate*>(_d);
  return d->_keepVirtualMemory;
}

void VirtualMemoryManager::setKeepVirtualMemory(bool keepVirtualMemory) ASMJIT_NOTHROW
{
  MemoryManagerPrivate* d = reinterpret_cast<MemoryManagerPrivate*>(_d);
  d->_keepVirtualMemory = keepVirtualMemory;
}

} // AsmJit namespace

// [Api-End]
#include "ApiEnd.h"

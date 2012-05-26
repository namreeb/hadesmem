#pragma once

#include "sehtest.h"

struct RTL_VECTORED_EXCEPTION_HANDLER
{
  RTL_VECTORED_EXCEPTION_HANDLER *Next;
  RTL_VECTORED_EXCEPTION_HANDLER *Prev;
  DWORD Refs;
  PVECTORED_EXCEPTION_HANDLER Handler;
};

struct VectoredHandlerCollection
{
  SRWLOCK Lock;
  RTL_VECTORED_EXCEPTION_HANDLER *Head;
  RTL_VECTORED_EXCEPTION_HANDLER *Tail;
};

LONG CALLBACK VectoredHandler(PEXCEPTION_POINTERS exceptionInfo);
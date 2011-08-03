#pragma once

#include "sehtest.h"

typedef _EXCEPTION_DISPOSITION (__cdecl *PEXCEPTION_HANDLER)(_EXCEPTION_RECORD *exceptionRecord, void *establisherFrame, _CONTEXT *contextRecord, void *dispatcherContext);

struct _EXCEPTION_REGISTRATION_RECORD
{
  _EXCEPTION_REGISTRATION_RECORD *Next;
  PEXCEPTION_HANDLER Handler;
};

struct _EXCEPTION_REGISTRATION_RECORD_EXTENDED
{
	_EXCEPTION_REGISTRATION_RECORD Record;
	_EXCEPTION_REGISTRATION_RECORD *Parent;
};

struct _DISPATCHER_CONTEXT
{
  _EXCEPTION_REGISTRATION_RECORD *RegistrationPointer;
};

#define EXCEPTION_CHAIN_END ((_EXCEPTION_REGISTRATION_RECORD *)-1)

#define EXCEPTION_UNWINDING                                 0x02
#define EXCEPTION_EXIT_UNWIND                               0x04
#define EXCEPTION_STACK_INVALID                             0x08
#define EXCEPTION_NESTED_CALL                               0x10
#define EXCEPTION_TARGET_UNWIND                             0x20
#define EXCEPTION_COLLIDED_UNWIND                           0x20
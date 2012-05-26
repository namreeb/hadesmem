#include "sehtest.h"
#include "veh.h"
#include "seh.h"

// Working:
// Debug mode

// Not working:
// Release mode
// Stack unwinding. Crashing due to invalid handler, even though handlers are 
// valid... Something fishy going on here.

// Notes:
//[3:44:02 AM] MaiN:     if (handler is on a non-executable page) { 
//        if (ExecuteDispatchEnable bit set in the process flags) 
//            return TRUE; 
//        else 
//            raise ACCESS_VIOLATION; // enforce DEP even if we have no hardware NX
//[3:44:06 AM] MaiN: this is the easiest way :P
//[3:44:37 AM] MaiN: if it was non executable
//[3:44:39 AM] MaiN: it would be ez
//[3:46:31 AM] MaiN: you can unwind until you're inside RtlDispatchException
//[3:46:34 AM] MaiN: and set al to 1
//[3:46:58 AM] MaiN: that will make it seem like RtlIsValidHandler returned 1
//[3:47:12 AM] MaiN: it's hookable without hooking it because it raises exceptions
// How would you bypass DEP though?

typedef void (__stdcall *tRtlRaiseException)(PEXCEPTION_RECORD rec);

static tRtlRaiseException RtlRaiseException;
static VectoredHandlerCollection *VhCollection;

bool CallVectoredHandlers(PSRWLOCK lock, RTL_VECTORED_EXCEPTION_HANDLER *firstHandler, RTL_VECTORED_EXCEPTION_HANDLER **head, PEXCEPTION_POINTERS exceptionInfo)
{
	RTL_VECTORED_EXCEPTION_HANDLER *removed = nullptr;
	LONG result = 0;

	AcquireSRWLockExclusive(lock);

	RTL_VECTORED_EXCEPTION_HANDLER *handler = firstHandler;
	while (static_cast<PVOID>(handler) != static_cast<PVOID>(head))
	{
		handler->Refs++;
		ReleaseSRWLockExclusive(lock);
		PVECTORED_EXCEPTION_HANDLER handlerFunc = static_cast<PVECTORED_EXCEPTION_HANDLER>(DecodePointer(handler->Handler));
		result = handlerFunc(exceptionInfo);

		AcquireSRWLockExclusive(lock);

		if (handler->Refs-- == 1)
		{
			// remove this handler from the list
			handler->Prev->Next = handler->Next;
			handler->Next->Prev = handler->Prev;
			
			if (handler->Next == handler->Prev)
				_interlockedbittestandreset(*(volatile LONG **)(__readfsdword(24) + 0x30) + 0x28, 2);

			handler->Next = removed;
			removed = handler;
		}

		if (result == EXCEPTION_CONTINUE_EXECUTION)
			break;

		handler = handler->Next;
	}

	ReleaseSRWLockExclusive(lock);
	while (removed)
	{
		LPVOID toFree = removed;
		removed = removed->Next;
		HeapFree(*(HANDLE *)(*(DWORD *)(__readfsdword(24) + 48) + 24), 0, toFree);
	}

	return result == EXCEPTION_CONTINUE_EXECUTION;
}

bool CallVectoredExceptionHandlers(PEXCEPTION_POINTERS exceptionInfo)
{
	VectoredHandlerCollection* col = &VhCollection[0];

	RTL_VECTORED_EXCEPTION_HANDLER *handler = col->Head;
	while (static_cast<PVOID>(handler) != static_cast<PVOID>(&col->Head))
	{
		if (DecodePointer(handler->Handler) == &VectoredHandler)
		{
			return CallVectoredHandlers(&col->Lock, handler->Next, &col->Head, exceptionInfo);
		}
	}

	return false;
}

void CallVectoredContinueHandlers(PEXCEPTION_POINTERS exceptionInfo)
{
	VectoredHandlerCollection* col = &VhCollection[1];

	CallVectoredHandlers(&col->Lock, col->Head, &col->Head, exceptionInfo);
}

_EXCEPTION_REGISTRATION_RECORD *GetExceptionRegistrationHead()
{
	return (_EXCEPTION_REGISTRATION_RECORD *)__readfsdword(0);
}

_EXCEPTION_DISPOSITION __cdecl ExceptionProtector(_EXCEPTION_RECORD *exceptionRecord, void *establisherFrame, _CONTEXT *contextRecord, void *dispatcherContext)
{
	if (!(exceptionRecord->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND)))
	{
		_DISPATCHER_CONTEXT *dispatcherContextReal = reinterpret_cast<_DISPATCHER_CONTEXT *>(dispatcherContext);
		_EXCEPTION_REGISTRATION_RECORD_EXTENDED *establisherFrameExt = reinterpret_cast<_EXCEPTION_REGISTRATION_RECORD_EXTENDED *>(establisherFrame);
		dispatcherContextReal->RegistrationPointer = establisherFrameExt->Parent;
		return ExceptionNestedException;
	}

	return ExceptionContinueSearch;
}

LONG CALLBACK VectoredHandler(PEXCEPTION_POINTERS exceptionInfo)
{
	LONG result = 1;

	if (CallVectoredExceptionHandlers(exceptionInfo))
	{
		result = EXCEPTION_CONTINUE_EXECUTION;
	}
	else
	{
		_EXCEPTION_REGISTRATION_RECORD *nestedFrame = nullptr;
		_DISPATCHER_CONTEXT dispatcherContext = {0};
		_EXCEPTION_REGISTRATION_RECORD *record = GetExceptionRegistrationHead();
		while (record != EXCEPTION_CHAIN_END)
		{			
			__asm
			{
				push record;
				push offset [ExceptionProtector];
				push fs:[0x0];
				mov fs:[0x0], esp;

				lea eax, dispatcherContext;
				push eax;
				mov eax, exceptionInfo;
				push [eax]_EXCEPTION_POINTERS.ContextRecord;
				push record;
				push [eax]_EXCEPTION_POINTERS.ExceptionRecord;
				mov eax, record;
				call [eax]_EXCEPTION_REGISTRATION_RECORD.Handler;
				add esp, 0x10;

				pop dword ptr fs:[0x0];
				add esp, 8;
			}

			_EXCEPTION_DISPOSITION disposition;
			__asm
			{
				mov disposition, eax;
			}

			if (record == nestedFrame)
			{
				exceptionInfo->ExceptionRecord->ExceptionFlags &= ~EXCEPTION_NESTED_CALL;
				nestedFrame = nullptr;
			}

			switch (disposition)
			{
				case ExceptionContinueExecution:
					if (exceptionInfo->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
					{
						EXCEPTION_RECORD ex = {0};
						ex.ExceptionRecord = exceptionInfo->ExceptionRecord;
						ex.ExceptionCode = STATUS_NONCONTINUABLE_EXCEPTION;
						ex.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
	
						RtlRaiseException(&ex);
					}
					else
					{
						result = EXCEPTION_CONTINUE_EXECUTION;
						goto Done;
					}

					break;
				case ExceptionContinueSearch:
					break;
				case ExceptionNestedException:
					exceptionInfo->ExceptionRecord->ExceptionFlags |= EXCEPTION_NESTED_CALL;

					if (dispatcherContext.RegistrationPointer > nestedFrame)
					{
						nestedFrame = dispatcherContext.RegistrationPointer;
					}
					break;
				default:
					EXCEPTION_RECORD ex = {0};
					ex.ExceptionRecord = exceptionInfo->ExceptionRecord;
					ex.ExceptionCode = STATUS_INVALID_DISPOSITION;
				
					RtlRaiseException(&ex);
					break;
			}

			record = record->Next;
		}
	}

Done:
	if (result == 1) // unhandled
		RaiseFailFastException(exceptionInfo->ExceptionRecord, exceptionInfo->ContextRecord, 0);

	if (result != EXCEPTION_CONTINUE_EXECUTION) // can the result really be anything else here?
	{
		// by returning EXCEPTION_CONTINUE_EXECUTION, ntdll!RtlRaiseException will
		// be calling vectored continue handlers. We shouldn't call them twice.
		CallVectoredContinueHandlers(exceptionInfo);
	}

	return result;
}

LONG CALLBACK VectoredContinueHandler(PEXCEPTION_POINTERS exceptionInfo)
{
	printf("Continuing exception: %x\n", exceptionInfo->ExceptionRecord->ExceptionCode);
	return EXCEPTION_CONTINUE_EXECUTION;
}

void RegisterVEHAndGetVHList()
{
	RTL_VECTORED_EXCEPTION_HANDLER *handler = (RTL_VECTORED_EXCEPTION_HANDLER *)AddVectoredExceptionHandler(1, &VectoredHandler);
	VectoredHandlerCollection *col = reinterpret_cast<VectoredHandlerCollection *>(reinterpret_cast<DWORD_PTR>(handler->Prev) - offsetof(VectoredHandlerCollection, Head));
	VhCollection = col;

	AddVectoredContinueHandler(1, &VectoredContinueHandler);
}

_EXCEPTION_DISPOSITION __cdecl ExceptionProtector2(_EXCEPTION_RECORD *exceptionRecord, void *establisherFrame, _CONTEXT *contextRecord, void *dispatcherContext)
{
	printf("In exception protector 2!");
	return ExceptionContinueExecution;
}

int main()
{
	RegisterVEHAndGetVHList();
	RtlRaiseException = (tRtlRaiseException)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "RtlRaiseException");

	__try
	{
		__try
		{
			__try
			{
				__try
				{
					//__asm
					//{
					//	push offset ExceptionProtector2;
					//	push fs:[0x0];
					//	mov fs:[0x0], esp;
					//}

					RaiseException(STATUS_ACCESS_VIOLATION, 0, 0, nullptr);

					//__asm
					//{
					//	pop dword ptr fs:[0x0];
					//	add esp, 4;
					//}
				}
				__except (/*false && */GetExceptionCode() == STATUS_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
				{
					printf("Handling inner\n");
					RaiseException(1234, 0, 0, nullptr);
				}
			}
			__finally
			{
				printf("Finally inner\n");
			}
		}
		__except (GetExceptionCode() == 1234 /*STATUS_ACCESS_VIOLATION*/ ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
		{
			printf("Handling outer\n");
		}
	}
	__finally
	{
		printf("Finally outer\n");
	}

	printf("Done\n");
	getc(stdin);
}
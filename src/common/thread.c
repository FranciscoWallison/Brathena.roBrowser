/****************************************************************************!
*                _           _   _   _                                       *
*               | |__  _ __ / \ | |_| |__   ___ _ __   __ _                  *
*               | '_ \| '__/ _ \| __| '_ \ / _ \ '_ \ / _` |                 *
*               | |_) | | / ___ \ |_| | | |  __/ | | | (_| |                 *
*               |_.__/|_|/_/   \_\__|_| |_|\___|_| |_|\__,_|                 *
*                                                                            *
*                            www.brathena.org                                *
******************************************************************************
* src/common/thread.c                                                        *
******************************************************************************
* Copyright (c) brAthena Dev Team                                            *
* Copyright (c) Hercules Dev Team                                            *
* Copyright (c) Athena Dev Teams                                             *
*                                                                            *
* Licenciado sob a licen?a GNU GPL                                           *
* Para mais informa??es leia o arquivo LICENSE na ra?z do emulador           *
*****************************************************************************/

#define BRATHENA_CORE

#include "thread.h"

#include "common/cbasetypes.h"
#include "common/memmgr.h"
#include "common/showmsg.h"
#include "common/sysinfo.h" // sysinfo->getpagesize()

#ifdef WIN32
#	include "common/winapi.h"
#	define __thread __declspec( thread )
#else
#	include <pthread.h>
#	include <sched.h>
#	include <signal.h>
#	include <stdlib.h>
#	include <string.h>
#	include <unistd.h>
#endif

// When Compiling using MSC (on win32..) we know we have support in any case!
#ifdef _MSC_VER
#define HAS_TLS
#endif

#define RA_THREADS_MAX 64

struct rAthread {
	unsigned int myID;

	RATHREAD_PRIO  prio;
	rAthreadProc proc;
	void *param;

	#ifdef WIN32
	HANDLE hThread;
	#else
	pthread_t hThread;
	#endif
};

#ifdef HAS_TLS
__thread int g_rathread_ID = -1;
#endif

///
/// Subystem Code
///
static struct rAthread l_threads[RA_THREADS_MAX];

void rathread_init(void) {
	register unsigned int i;
	memset(&l_threads, 0x00, RA_THREADS_MAX * sizeof(struct rAthread) );

	for(i = 0; i < RA_THREADS_MAX; i++){
		l_threads[i].myID = i;
	}

	// now lets init thread id 0, which represents the main thread
#ifdef HAS_TLS
	g_rathread_ID = 0;
#endif
	l_threads[0].prio = RAT_PRIO_NORMAL;
	l_threads[0].proc = (rAthreadProc)0xDEADCAFE;

}//end: rathread_init()

void rathread_final(void) {
	register unsigned int i;

	// Unterminated Threads Left?
	// Shouldn't happen ..
	// Kill 'em all!
	//
	for(i = 1; i < RA_THREADS_MAX; i++){
		if(l_threads[i].proc != NULL){
			ShowWarning("thread_final: thread interminado (tid %u entryPoint %p) - forcando o encerramento (finalizando)\n", i, l_threads[i].proc);
			rathread_destroy(&l_threads[i]);
		}
	}

}//end: rathread_final()

// gets called whenever a thread terminated ..
static void rat_thread_terminated(rAthread *handle) {
	// Preserve handle->myID and handle->hThread, set everything else to its default value
	handle->param = NULL;
	handle->proc = NULL;
	handle->prio = RAT_PRIO_NORMAL;
}//end: rat_thread_terminated()

#ifdef WIN32
DWORD WINAPI raThreadMainRedirector(LPVOID p){
#else
static void *raThreadMainRedirector( void *p ){
	sigset_t set; // on Posix Thread platforms
#endif
	void *ret;

	// Update myID @ TLS to right id.
#ifdef HAS_TLS
	g_rathread_ID = ((rAthread*)p)->myID;
#endif

#ifndef WIN32
	// When using posix threads
	// the threads inherits the Signal mask from the thread which spawned
	// this thread
	// so we've to block everything we don't care about.
	(void)sigemptyset(&set);
	(void)sigaddset(&set, SIGINT);
	(void)sigaddset(&set, SIGTERM);
	(void)sigaddset(&set, SIGPIPE);

	pthread_sigmask(SIG_BLOCK, &set, NULL);

#endif

	ret = ((rAthread*)p)->proc( ((rAthread*)p)->param ) ;

#ifdef WIN32
	CloseHandle( ((rAthread*)p)->hThread );
#endif

	rat_thread_terminated( (rAthread*)p );
#ifdef WIN32
	return (DWORD)ret;
#else
	return ret;
#endif
}//end: raThreadMainRedirector()

///
/// API Level
///
rAthread *rathread_create(rAthreadProc entryPoint, void *param) {
	return rathread_createEx( entryPoint, param,  (1<<23) /*8MB*/,  RAT_PRIO_NORMAL );
}//end: rathread_create()

rAthread *rathread_createEx(rAthreadProc entryPoint, void *param, size_t szStack, RATHREAD_PRIO prio) {
#ifndef WIN32
	pthread_attr_t attr;
#endif
	size_t tmp;
	unsigned int i;
	rAthread *handle = NULL;

	// given stacksize aligned to systems pagesize?
	tmp = szStack % sysinfo->getpagesize();
	if(tmp != 0)
		szStack += tmp;

	// Get a free Thread Slot.
	for(i = 0; i < RA_THREADS_MAX; i++){
		if(l_threads[i].proc == NULL){
			handle = &l_threads[i];
			break;
		}
	}

	if(handle == NULL){
		ShowError("thread: Nao foi possivel criar um novo thread (entryPoint: %p) - sem espaco livre!", entryPoint);
		return NULL;
	}

	handle->proc = entryPoint;
	handle->param = param;

#ifdef WIN32
	handle->hThread = CreateThread(NULL, szStack, raThreadMainRedirector, (void*)handle, 0, NULL);
#else
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, szStack);

	if(pthread_create(&handle->hThread, &attr, raThreadMainRedirector, (void*)handle) != 0){
		handle->proc = NULL;
		handle->param = NULL;
		return NULL;
	}
	pthread_attr_destroy(&attr);
#endif

	rathread_prio_set( handle,  prio );

	return handle;
}//end: rathread_createEx

void rathread_destroy(rAthread *handle) {
#ifdef WIN32
	if( TerminateThread(handle->hThread, 0) != FALSE){
		CloseHandle(handle->hThread);
		rat_thread_terminated(handle);
	}
#else
	if( pthread_cancel( handle->hThread ) == 0){
		// We have to join it, otherwise pthread wont re-cycle its internal resources assoc. with this thread.
		pthread_join( handle->hThread, NULL );

		// Tell our manager to release resources ;)
		rat_thread_terminated(handle);
	}
#endif
}//end: rathread_destroy()

rAthread *rathread_self(void) {
#ifdef HAS_TLS
	rAthread *handle = &l_threads[g_rathread_ID];

	if(handle->proc != NULL) // entry point set, so its used!
		return handle;
#else
	// .. so no tls means we have to search the thread by its api-handle ..
	int i;

	#ifdef WIN32
		HANDLE hSelf;
		hSelf = GetCurrent = GetCurrentThread();
	#else
		pthread_t hSelf;
		hSelf = pthread_self();
	#endif

	for(i = 0; i < RA_THREADS_MAX; i++){
		if(l_threads[i].hThread == hSelf  &&  l_threads[i].proc != NULL)
			return &l_threads[i];
	}
#endif

	return NULL;
}//end: rathread_self()

int rathread_get_tid(void) {

#ifdef HAS_TLS
	return g_rathread_ID;
#else
	// TODO
	#ifdef WIN32
		return (int)GetCurrentThreadId();
	#else
		return (intptr_t)pthread_self();
	#endif
#endif

}//end: rathread_get_tid()

bool rathread_wait(rAthread *handle, void **out_exitCode) {
	// Hint:
	// no thread data cleanup routine call here!
	// its managed by the callProxy itself..
	//
#ifdef WIN32
	WaitForSingleObject(handle->hThread, INFINITE);
	return true;
#else
	if(pthread_join(handle->hThread, out_exitCode) == 0)
		return true;
	return false;
#endif

}//end: rathread_wait()

void rathread_prio_set(rAthread *handle, RATHREAD_PRIO prio) {
	handle->prio = RAT_PRIO_NORMAL;
	//@TODO
}//end: rathread_prio_set()

RATHREAD_PRIO rathread_prio_get(rAthread *handle) {
	return handle->prio;
}//end: rathread_prio_get()

void rathread_yield(void) {
#ifdef WIN32
	SwitchToThread();
#else
	sched_yield();
#endif
}//end: rathread_yield()

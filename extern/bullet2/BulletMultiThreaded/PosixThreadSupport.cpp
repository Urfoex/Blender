/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2007 Erwin Coumans  http://bulletphysics.com

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include <stdio.h>
#include "PosixThreadSupport.h"
#ifdef USE_PTHREADS
#include <errno.h>
#include <unistd.h>

#include "SpuCollisionTaskProcess.h"
#include "SpuNarrowPhaseCollisionTask/SpuGatheringCollisionTask.h"

#define checkPThreadFunction(returnValue) \
    if(0 != returnValue) { \
        printf("PThread problem at line %i in file %s: %i %d\n", __LINE__, __FILE__, returnValue, errno); \
    }

// The number of threads should be equal to the number of available cores
// Todo: each worker should be linked to a single core, using SetThreadIdealProcessor.

// PosixThreadSupport helps to initialize/shutdown libspe2, start/stop SPU tasks and communication
// Setup and initialize SPU/CELL/Libspe2
PosixThreadSupport::PosixThreadSupport(ThreadConstructionInfo& threadConstructionInfo)
{
	startThreads(threadConstructionInfo);
}

// cleanup/shutdown Libspe2
PosixThreadSupport::~PosixThreadSupport()
{
	stopSPU();
}

#if (defined (__APPLE__))
#define NAMED_SEMAPHORES
#endif

// this semaphore will signal, if and how many threads are finished with their work
static sem_t* mainSemaphore;

static sem_t* createSem(const char* baseName)
{
	static int semCount = 0;
#ifdef NAMED_SEMAPHORES
        /// Named semaphore begin
        char name[32];
        snprintf(name, 32, "/%s-%d-%4.4d", baseName, getpid(), semCount++); 
        sem_t* tempSem = sem_open(name, O_CREAT, 0600, 0);
        if (tempSem != reinterpret_cast<sem_t *>(SEM_FAILED))
        {
        	//printf("Created \"%s\" Semaphore %x\n", name, tempSem);
        }
        else
	{
		//printf("Error creating Semaphore %d\n", errno);
		exit(-1);
	}
        /// Named semaphore end
#else
	sem_t* tempSem = new sem_t;
	checkPThreadFunction(sem_init(tempSem, 0, 0));
#endif
	return tempSem;
}

static void destroySem(sem_t* semaphore)
{
#ifdef NAMED_SEMAPHORES
	checkPThreadFunction(sem_close(semaphore));
#else
	checkPThreadFunction(sem_destroy(semaphore));
	delete semaphore;
#endif	
}

static void *threadFunction(void *argument) 
{

	PosixThreadSupport::btSpuStatus* status = (PosixThreadSupport::btSpuStatus*)argument;

	
	while (1)
	{
            checkPThreadFunction(sem_wait(status->startSemaphore));
		
		void* userPtr = status->m_userPtr;

		if (userPtr)
		{
			btAssert(status->m_status);
			status->m_userThreadFunc(userPtr,status->m_lsMemory);
			status->m_status = 2;
			checkPThreadFunction(sem_post(mainSemaphore));
	                status->threadUsed++;
		} else {
			//exit Thread
			status->m_status = 3;
			checkPThreadFunction(sem_post(mainSemaphore));
			printf("Thread with taskId %i exiting\n",status->m_taskId);
			break;
		}
		
	}

	printf("Thread TERMINATED\n");
	return 0;

}

///send messages to SPUs
void PosixThreadSupport::sendRequest(uint32_t uiCommand, ppu_address_t uiArgument0, uint32_t taskId)
{
	///	gMidphaseSPU.sendRequest(CMD_GATHER_AND_PROCESS_PAIRLIST, (uint32_t) &taskDesc);
	
	///we should spawn an SPU task here, and in 'waitForResponse' it should wait for response of the (one of) the first tasks that finished
	


	switch (uiCommand)
	{
	case 	CMD_GATHER_AND_PROCESS_PAIRLIST:
		{
			btSpuStatus&	spuStatus = m_activeSpuStatus[taskId];
			btAssert(taskId >= 0);
			btAssert(taskId < m_activeSpuStatus.size());

			spuStatus.m_commandId = uiCommand;
			spuStatus.m_status = 1;
			spuStatus.m_userPtr = (void*)uiArgument0;

			// fire event to start new task
			checkPThreadFunction(sem_post(spuStatus.startSemaphore));
			break;
		}
	default:
		{
			///not implemented
			btAssert(0);
		}

	};


}


///check for messages from SPUs
void PosixThreadSupport::waitForResponse(unsigned int *puiArgument0, unsigned int *puiArgument1)
{
	///We should wait for (one of) the first tasks to finish (or other SPU messages), and report its response
	
	///A possible response can be 'yes, SPU handled it', or 'no, please do a PPU fallback'


	btAssert(m_activeSpuStatus.size());

        // wait for any of the threads to finish
	checkPThreadFunction(sem_wait(mainSemaphore));
        
	// get at least one thread which has finished
        size_t last = -1;
        
        for(size_t t=0; t < size_t(m_activeSpuStatus.size()); ++t) {
            if(2 == m_activeSpuStatus[t].m_status) {
                last = t;
                break;
            }
        }

	btSpuStatus& spuStatus = m_activeSpuStatus[last];

	btAssert(spuStatus.m_status > 1);
	spuStatus.m_status = 0;

	// need to find an active spu
	btAssert(last >= 0);

	*puiArgument0 = spuStatus.m_taskId;
	*puiArgument1 = spuStatus.m_status;
}



void PosixThreadSupport::startThreads(ThreadConstructionInfo& threadConstructionInfo)
{
        printf("%s creating %i threads.\n", __FUNCTION__, threadConstructionInfo.m_numThreads);
	m_activeSpuStatus.resize(threadConstructionInfo.m_numThreads);
        
	mainSemaphore = createSem("main");                
        
	for (int i=0;i < threadConstructionInfo.m_numThreads;i++)
	{
		printf("starting thread %d\n",i);

		btSpuStatus&	spuStatus = m_activeSpuStatus[i];

		spuStatus.startSemaphore = createSem("threadLocal");                
                
                checkPThreadFunction(pthread_create(&spuStatus.thread, NULL, &threadFunction, (void*)&spuStatus));

		spuStatus.m_userPtr=0;

		spuStatus.m_taskId = i;
		spuStatus.m_commandId = 0;
		spuStatus.m_status = 0;
		spuStatus.m_lsMemory = threadConstructionInfo.m_lsMemoryFunc();
		spuStatus.m_userThreadFunc = threadConstructionInfo.m_userThreadFunc;
        spuStatus.threadUsed = 0;

		printf("started thread %d \n",i);
		
	}

}

void PosixThreadSupport::startSPU()
{
}


///tell the task scheduler we are done with the SPU tasks
void PosixThreadSupport::stopSPU()
{
	for(size_t t=0; t < size_t(m_activeSpuStatus.size()); ++t) {
            btSpuStatus&	spuStatus = m_activeSpuStatus[t];
            printf("%s: Thread %i used: %ld\n", __FUNCTION__, int(t), spuStatus.threadUsed);
        
            destroySem(spuStatus.startSemaphore);
            checkPThreadFunction(pthread_cancel(spuStatus.thread));
        }
        destroySem(mainSemaphore);

	m_activeSpuStatus.clear();
}

#endif // USE_PTHREADS


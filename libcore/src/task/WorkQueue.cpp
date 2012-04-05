/*  Sirikata Kernel -- Task scheduling system
 *  WorkQueue.cpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*  Created on: May 2, 2009 */

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/task/WorkQueue.hpp>
#include <sirikata/core/task/Time.hpp>
#include <sirikata/core/queue/ThreadSafeQueue.hpp>
#include <sirikata/core/queue/LockFreeQueue.hpp>
#include <sirikata/core/util/Thread.hpp>
#include <stdio.h>

using namespace std;

namespace Sirikata {
namespace Task {

bool AbortableWorkItem::doAcquireLock() {
	if ((++mAbortLock)==1) {
		return true;
	}
	return false;
}

AbortableWorkItem::AbortableWorkItem() : mAbortLock(0) {
}
AbortableWorkItem::~AbortableWorkItem() {}

bool AbortableWorkItem::abort() {
	AbortableWorkItemPtr tempReference(shared_from_this());
	if (doAcquireLock()) {
		if (mPreventDeletion) {
			aborted();
			return true;
		}
	}
	return false;
}



namespace {
void workQueueWorkerThread(WorkQueue *queue) {
    try {
        while (queue->dequeueBlocking()) {
        }
    } catch (std::exception &exc) {
        SILOG(task,fatal,"Caught exception '" << exc.what() << "' in WorkQueue thread!");
        throw;
    }
}
}

class WorkQueueThread {
public:
	std::vector<Thread*> mThreads;
};

WorkQueueThread *WorkQueue::createWorkerThreads(int count) {
	WorkQueueThread *threads = new WorkQueueThread;
	for (int i = 0; i < count; ++i) {
		threads->mThreads.push_back(
                    new Thread("WorkQueue Worker", std::tr1::bind(&workQueueWorkerThread, this)));
	}
	return threads;
}

void WorkQueue::destroyWorkerThreads(WorkQueueThread *th) {
	int len = th->mThreads.size();
	for (int i = 0; i < len; ++i) {
		enqueue(NULL); // Make sure each thread gets one of these.
	}
	for (int i = 0; i < len; ++i) {
		th->mThreads[i]->join();
		delete th->mThreads[i];
	}
	delete th;
}

template <class QueueType>
void UnsafeWorkQueueImpl<QueueType>::enqueue(WorkItem *element) {
	mQueue[mWhichQueue].push(element);
}

template <class QueueType>
bool UnsafeWorkQueueImpl<QueueType>::dequeuePoll() {
	if (mQueue[mWhichQueue].empty()) {
		return false;
	}
	WorkItem *element = mQueue[mWhichQueue].front();
	if (element) {
		(*element)();
	}
	mQueue[mWhichQueue].pop();
	return element?true:false;
}

template <class QueueType>
bool UnsafeWorkQueueImpl<QueueType>::dequeueBlocking() {
	if (!dequeuePoll()) {
		throw std::domain_error("Blocking dequeue called on empty thread-unsafe WorkQueue.");
	}
	return true;
}

template <class QueueType>
unsigned int UnsafeWorkQueueImpl<QueueType>::dequeueAll() {
	unsigned int count = 0;

	// std::queue has no swap function.
	QueueType &queue = mQueue[mWhichQueue];
	mWhichQueue = !mWhichQueue;

	while (!queue.empty()) {
		WorkItem *element = queue.front();
		if (element) {
			(*element)();
		}
		queue.pop();
		++count;
	}
	return count;
}

template <class QueueType>
UnsafeWorkQueueImpl<QueueType>::~UnsafeWorkQueueImpl() {
	// std::queue has no swap function.
	QueueType &queue = mQueue[mWhichQueue];
	mWhichQueue = !mWhichQueue;

	while (!queue.empty()) {
		WorkItem *element = queue.front();
		if (element) {
            std::auto_ptr<WorkItem>deleteMe(element);
		}
		queue.pop();
	}
}

template <class QueueType>
bool UnsafeWorkQueueImpl<QueueType>::probablyEmpty() {
	return mQueue[mWhichQueue].empty();
}

template class SIRIKATA_EXPORT UnsafeWorkQueueImpl<std::queue<WorkItem*> >;


}
}

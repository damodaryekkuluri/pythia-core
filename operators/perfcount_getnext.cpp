/*
 * Copyright 2015, Pythia authors (see AUTHORS file).
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "operators.h"
#include "operators_priv.h"

#include "../rdtsc.h"
#include "../util/numaallocate.h"

#include <unistd.h>
#include <fcntl.h>

void PerfCount_Getnext::init(libconfig::Config& root, libconfig::Setting& node)
{
	SingleInputOp::init(root, node);
	schema = nextOp->getOutSchema();

	barrier = PThreadLockCVBarrier();
	libconfig::Setting& partnode = node["threadgroups"];
	barrier.init(partnode.getLength());
	groupleader = partnode[0];

	//to record the path of startfifo and stopfifo, which is used to synchronize
	//perf and pythia process
	startfifo = (const char*) root.getRoot()["path"];
	startfifo += "/";
	startfifo += (const char*) node["startfifo"];
	
	stopfifo = (const char*) root.getRoot()["path"];
	stopfifo += "/";
	stopfifo += (const char*) node["stopfifo"];
}

Operator::ResultCode PerfCount_Getnext::scanStart(unsigned short threadid,
		Page* indexdatapage, Schema& indexdataschema)
{
	ResultCode res;
	
	res = nextOp->scanStart(threadid, indexdatapage, indexdataschema);
	
	barrier.Arrive();
	//start perf after threads finish scanstart for underlying operators
	if (threadid == groupleader)
	{
		startWrid = open(startfifo.c_str(), O_WRONLY);
	}
		
	return res;
}

Operator::GetNextResultT PerfCount_Getnext::getNext(unsigned short threadid)
{
	GetNextResultT res;
	
	res = nextOp->getNext(threadid);
	
	return res;
}

Operator::ResultCode PerfCount_Getnext::scanStop(unsigned short threadid)
{
	ResultCode res;
	
	barrier.Arrive();

	if (threadid == groupleader)
	{
		stopWrid = open(stopfifo.c_str(), O_WRONLY);
	}
		
	res = nextOp->scanStop(threadid);
	
	return res;
}

void PerfCount_Getnext::threadClose(unsigned short threadid)
{
	// The group leader closes the FIFO used for synchronization.
	//
	if (threadid == groupleader)
	{
		close(startWrid);
		close(stopWrid);
	}
}


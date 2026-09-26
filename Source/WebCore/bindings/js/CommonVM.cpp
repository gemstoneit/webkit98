/*
 * Copyright (C) 2016-2022 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "CommonVM.h"

#include "JSDOMWindow.h"
#include "LocalDOMWindow.h"
#include "LocalFrame.h"
#include "OpportunisticTaskScheduler.h"
#include "ScriptController.h"
#include "WebCoreJSClientData.h"
#include <JavaScriptCore/EdenGCActivityCallback.h>
#include <JavaScriptCore/FullGCActivityCallback.h>
#include <JavaScriptCore/HeapInlines.h>
#include <JavaScriptCore/MachineStackMarker.h>
#include <JavaScriptCore/VM.h>
#include <wtf/MainThread.h>
#include <wtf/RunLoop.h>
#include <wtf/text/AtomString.h>

#if PLATFORM(IOS_FAMILY)
#include "WebCoreThreadInternal.h"
#endif

#if defined(WEBKIT_WINDOWS_LEGACY_TARGET) && OS(WINDOWS)
extern "C" void win98Trace(const char*);
#define WIN98_TRACE(message) win98Trace(message)
#else
#define WIN98_TRACE(message) do { } while (0)
#endif

namespace WebCore {

JSC::VM* g_commonVMOrNull;

JSC::VM& commonVMSlow()
{
    WIN98_TRACE("commonVMSlow: enter");
    ASSERT(isMainThread());
    ASSERT(!g_commonVMOrNull);

    // FIXME: Remove this call to ScriptController::initializeMainThread(). The
    // main thread should have been initialized by a WebKit entrypoint already.
    // Also, initializeMainThread() does nothing on iOS.
    WIN98_TRACE("commonVMSlow: initializeMainThread begin");
    ScriptController::initializeMainThread();
    WIN98_TRACE("commonVMSlow: initializeMainThread end");

#if PLATFORM(IOS_FAMILY)
    RunLoop* runLoop = RunLoop::webIfExists();
#else
    RunLoop* runLoop = nullptr;
#endif

    WIN98_TRACE("commonVMSlow: VM create begin");
    auto vmRef = JSC::VM::create(JSC::HeapType::Large, runLoop);
    WIN98_TRACE("commonVMSlow: VM create end");
    auto& vm = vmRef.leakRef();
    WIN98_TRACE("commonVMSlow: VM leakRef end");
#if !PLATFORM(IOS_FAMILY)
#if defined(WEBKIT_WINDOWS_LEGACY_TARGET) && OS(WINDOWS)
    WIN98_TRACE("commonVMSlow: clear activity callbacks begin");
    vm.heap.setFullActivityCallback(nullptr);
    vm.heap.setEdenActivityCallback(nullptr);
    WIN98_TRACE("commonVMSlow: clear activity callbacks end");
#else
    vm.heap.setFullActivityCallback(OpportunisticTaskScheduler::FullGCActivityCallback::create(vm.heap));
    vm.heap.setEdenActivityCallback(OpportunisticTaskScheduler::EdenGCActivityCallback::create(vm.heap));
    vm.heap.disableStopIfNecessaryTimer(); // Because opportunistic task scheduler and GC timer exists, we do not need StopIfNecessaryTimer.
#endif
#endif

    WIN98_TRACE("commonVMSlow: assign global VM begin");
    g_commonVMOrNull = &vm;
    WIN98_TRACE("commonVMSlow: assign global VM end");

    WIN98_TRACE("commonVMSlow: heap acquireAccess begin");
    vm.heap.acquireAccess(); // At any time, we may do things that affect the GC.
    WIN98_TRACE("commonVMSlow: heap acquireAccess end");

#if PLATFORM(IOS_FAMILY)
    if (WebThreadIsEnabled())
        vm.apiLock().makeWebThreadAware();
    vm.heap.machineThreads().addCurrentThread();
#endif

    WIN98_TRACE("commonVMSlow: initNormalWorld begin");
    JSVMClientData::initNormalWorld(&vm, WorkerThreadType::Main);
    WIN98_TRACE("commonVMSlow: initNormalWorld end");

    WIN98_TRACE("commonVMSlow: exit");
    return vm;
}

LocalFrame* lexicalFrameFromCommonVM()
{
    JSC::VM& vm = commonVM();
    if (auto* topCallFrame = vm.topCallFrame) {
        if (auto* globalObject = JSC::jsCast<JSDOMGlobalObject*>(topCallFrame->lexicalGlobalObject(vm))) {
            if (auto* window = JSC::jsDynamicCast<JSDOMWindow*>(globalObject)) {
                if (auto* frame = window->wrapped().frame())
                    return dynamicDowncast<LocalFrame>(frame);
            }
        }
    }
    return nullptr;
}

void addImpureProperty(const AtomString& propertyName)
{
    commonVM().addImpureProperty(propertyName.impl());
}

} // namespace WebCore

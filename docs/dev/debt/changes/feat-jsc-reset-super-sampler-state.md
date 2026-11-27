# libSceJsc

## add Functional

- [JSC::resetSuperSamplerState](../../../core/libs/prx/libSceJsc/src/SuperSampler.cpp) (libSceJsc) - not compared with a console module; the console JavaScriptCore version is unverified. It follows pinned [SuperSampler.cpp](https://github.com/WebKit/WebKit/blob/c55364d53f971f63a709976f168d932d558b8909/Source/JavaScriptCore/bytecode/SuperSampler.cpp#L75-L80): under the sampler lock it zeroes the inside and outside sample counts. libSceJsc exports neither `initializeSuperSampler`, `printSuperSamplerState`, `enableSuperSampler` nor `g_superSamplerCount`, so no sampler thread runs and nothing outside the module reads the counts; only the host test fills them. Closes when: compared with a console module.

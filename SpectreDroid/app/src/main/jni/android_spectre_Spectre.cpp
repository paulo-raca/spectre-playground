#include "android_spectre_Spectre.h"
#include <android/log.h>
/*
extern void spectre_dump(void* target_ptr, size_t target_len);

void Java_android_spectre_Spectre_dump(JNIEnv *env, jclass cls, jbyteArray jdata) {
    __android_log_print(ANDROID_LOG_DEBUG, "Spectre", "Java_android_spectre_Spectre_dump\n");
    jsize len = env->GetArrayLength(jdata);
    jbyte* data = env->GetByteArrayElements(jdata, nullptr);
    if (data != NULL) {
        spectre_dump(data, len);
        env->ReleaseByteArrayElements(jdata, data, JNI_ABORT);
    }
}*/


#include <jni.h>
#include <cctype>
#include <cstring>
#include <alloca.h>

using namespace std;

#define VERSION JNI_VERSION_1_6
#define PRIMITIVE_TYPE_COUNT 7

class NonVirtualMethodCaller {
private:
    class PrimitiveType {
        char shortName;
        jclass wrapperType;
        jmethodID boxMethod;
        jmethodID unboxMethod;
        jclass primitiveType;
    public:
        struct Names {
            const char *name;
            const char shortName;

            Names(const char *name, const char shortName) :
                    name(name), shortName(shortName) {}
        };

        PrimitiveType() = default;

        PrimitiveType(
                const char shortName,
                const jclass &wrapperType,
                const jmethodID &boxMethod,
                const jmethodID &unboxMethod,
                const jclass &primitiveType
        ) : shortName(shortName),
            wrapperType(wrapperType),
            boxMethod(boxMethod),
            unboxMethod(unboxMethod),
            primitiveType(primitiveType) {}

        bool isWrapperType(JNIEnv *env, jclass clazz) {
            return env->IsSameObject(clazz, wrapperType);
        }

        bool isPrimitiveType(JNIEnv *env, jclass clazz) {
            return env->IsSameObject(clazz, primitiveType);
        }

        void unbox(JNIEnv *env, jobject obj, jvalue *out) {
            switch (shortName) {
                case 'Z': {
                    out->z = env->CallBooleanMethod(obj, unboxMethod);
                }
                    break;
                case 'I': {
                    out->i = env->CallIntMethod(obj, unboxMethod);
                }
                    break;
                case 'J': {
                    out->j = env->CallLongMethod(obj, unboxMethod);
                }
                    break;
                case 'S': {
                    out->s = env->CallShortMethod(obj, unboxMethod);
                }
                    break;
                case 'D': {
                    out->d = env->CallDoubleMethod(obj, unboxMethod);
                }
                    break;
                case 'F': {
                    out->f = env->CallFloatMethod(obj, unboxMethod);
                }
                    break;
                case 'B': {
                    out->d = env->CallByteMethod(obj, unboxMethod);
                }
                    break;
            }
        }

        jobject invokeNonVirtualMethod(
                JNIEnv *env,
                jclass clazz,
                jmethodID method,
                jobject obj,
                jvalue *args
        ) {
            jvalue value;
            switch (shortName) {
                case 'Z': {
                    value.z = env->CallNonvirtualBooleanMethodA(obj, clazz, method, args);
                }
                    break;
                case 'I': {
                    value.i = env->CallNonvirtualIntMethodA(obj, clazz, method, args);
                }
                    break;
                case 'J': {
                    value.j = env->CallNonvirtualLongMethodA(obj, clazz, method, args);
                }
                    break;
                case 'S': {
                    value.s = env->CallNonvirtualShortMethodA(obj, clazz, method, args);
                }
                    break;
                case 'D': {
                    value.d = env->CallNonvirtualDoubleMethodA(obj, clazz, method, args);
                }
                    break;
                case 'F': {
                    value.f = env->CallNonvirtualFloatMethodA(obj, clazz, method, args);
                }
                    break;
                case 'B': {
                    value.b = env->CallNonvirtualByteMethodA(obj, clazz, method, args);
                }
                    break;
            }
            if (env->ExceptionCheck()) {
                return nullptr;
            } else {
                return env->CallStaticObjectMethodA(wrapperType, boxMethod, &value);
            }
        }
    };

    PrimitiveType *mappings;

    jclass voidType;

    static jclass getReturnType(
            JNIEnv *env,
            jclass clazz,
            jmethodID methodId
    ) {
        return getReturnType(env, env->ToReflectedMethod(clazz, methodId, JNI_FALSE));
    }

    static jclass getReturnType(
            JNIEnv *env,
            jobject method
    ) {
        static jmethodID getReturnTypeMethod = nullptr;
        if (getReturnTypeMethod == nullptr) {
            getReturnTypeMethod = env->GetMethodID(
                    env->FindClass("java/lang/reflect/Method"),
                    "getReturnType",
                    "()Ljava/lang/Class;"
            );
        }
        return (jclass) env->CallObjectMethod(method, getReturnTypeMethod);
    }

    static jclass getDeclaringClass(JNIEnv *env, jobject method) {
        static jmethodID getDeclaringClassMethod = nullptr;
        if (getDeclaringClassMethod == nullptr) {
            getDeclaringClassMethod = env->GetMethodID(
                    env->FindClass("java/lang/reflect/Method"),
                    "getDeclaringClass",
                    "()Ljava/lang/Class;"
            );
        }
        return (jclass) env->CallObjectMethod(method, getDeclaringClassMethod);
    }

    static jobjectArray getParameterTypes(JNIEnv *env, jobject method) {
        static jmethodID getParameterTypesMethod = nullptr;
        if (getParameterTypesMethod == nullptr) {
            getParameterTypesMethod = env->GetMethodID(
                    env->FindClass("java/lang/reflect/Method"),
                    "getParameterTypes",
                    "()[Ljava/lang/Class;"
            );
        }
        return (jobjectArray) env->CallObjectMethod(method, getParameterTypesMethod);
    }

    void toNativeArgs(
            JNIEnv *env,
            jobjectArray parameterTypes,
            jobjectArray parameters,
            jvalue *out,
            int length
    ) {
        for (int i = 0; i < length; ++i) {
            auto clazz = (jclass) env->GetObjectArrayElement(parameterTypes, i);
            int find = -1;
            for (int j = 0; j < PRIMITIVE_TYPE_COUNT; ++j) {
                auto item = mappings[j];
                if (item.isWrapperType(env, clazz)) {
                    find = j;
                }
            }
            jobject obj = env->GetObjectArrayElement(parameters, i);
            jvalue value;
            if (find != -1) {
                mappings[find].unbox(env, obj, &value);
            } else {
                value.l = obj;
            }
            out[i] = value;
        }
    }

public:
    explicit NonVirtualMethodCaller(JNIEnv *env) {
        PrimitiveType::Names primitiveTypeNames[PRIMITIVE_TYPE_COUNT] = {
                PrimitiveType::Names("Boolean", 'Z'),
                PrimitiveType::Names("Integer", 'I'),
                PrimitiveType::Names("Long", 'J'),
                PrimitiveType::Names("Short", 'S'),
                PrimitiveType::Names("Double", 'D'),
                PrimitiveType::Names("Character", 'C'),
                PrimitiveType::Names("Byte", 'B'),
        };
        mappings = new PrimitiveType[PRIMITIVE_TYPE_COUNT];
        for (int i = 0; i < PRIMITIVE_TYPE_COUNT; ++i) {
            auto typeName = primitiveTypeNames[i];
            char *name = new char[strlen("java/lang/") + strlen(typeName.name) + 1];
            strcpy(name, "java/lang/");
            strcat(name, typeName.name);
            auto wrapperType = (jclass) env->NewGlobalRef(env->FindClass(name));
            delete[] name;
            char *sig = new char[3 + strlen("Ljava/lang/") + strlen(typeName.name)];
            strcpy(sig, "C");
            char shortName[] = {typeName.shortName, '\0'};
            strcat(sig, shortName);
            strcat(sig, "Ljava/lang/");
            strcat(sig, typeName.name);
            jmethodID boxMethod = env->GetStaticMethodID(
                    wrapperType,
                    "valueOf",
                    sig
            );
            delete[] sig;
            name = new char[strlen(typeName.name) + strlen("Value") + 1];
            strcpy(name, typeName.name);
            name[0] = (char) tolower(name[0]);
            strcat(name, "Value");
            sig = new char[strlen("()") + 2];
            strcpy(sig, "()");
            strcat(sig, shortName);
            jmethodID unboxMethod = env->GetMethodID(
                    wrapperType,
                    name,
                    sig
            );
            delete[] name;
            delete[] sig;
            auto primitiveType = (jclass) env->NewGlobalRef(
                    getReturnType(env, wrapperType, unboxMethod)
            );
            mappings[i] = PrimitiveType(
                    typeName.shortName,
                    wrapperType,
                    boxMethod,
                    unboxMethod,
                    primitiveType
            );
        }
        auto voidFakeType = env->FindClass("java/lang/Void");
        auto voidTypeId = env->GetStaticFieldID(voidType, "TYPE", "Ljava/lang/Class;");
        voidType = (jclass) env->NewGlobalRef(env->GetStaticObjectField(voidFakeType, voidTypeId));
    }

    jobject invokeNonVirtual(JNIEnv *env, jobject method, jobject obj, jobjectArray args) {
        auto clazz = getDeclaringClass(env, method);
        auto parameterTypes = getParameterTypes(env, method);
        auto returnType = getReturnType(env, method);
        auto length = env->GetArrayLength(parameterTypes);
        auto nativeArgs = (jvalue *) alloca(sizeof(jvalue) * length);
        toNativeArgs(env, parameterTypes, args, nativeArgs, length);
        auto methodId = env->FromReflectedMethod(method);
        if (env->IsSameObject(returnType, voidType)) {
            env->CallNonvirtualVoidMethodA(obj, clazz, methodId, nativeArgs);
            return nullptr;
        } else {
            int find = -1;
            for (int i = 0; i < PRIMITIVE_TYPE_COUNT; ++i) {
                auto type = mappings[i];
                if (type.isPrimitiveType(env, returnType)) {
                    find = i;
                }
            }
            if (find != -1) {
                return mappings[find].invokeNonVirtualMethod(env, clazz, methodId, obj, nativeArgs);
            } else {
                return env->CallNonvirtualObjectMethodA(obj, clazz, methodId, nativeArgs);
            }
        }
    }
};

static NonVirtualMethodCaller *caller = nullptr;

extern "C"
JNIEXPORT jobject JNICALL
Java_open_source_reflect_NonVirtualMethodCaller_invokeNonVirtual(
        JNIEnv *env,
        jclass _,
        jobject method,
        jobject obj,
        jobjectArray args
) {
    if (caller == nullptr) {
        caller = new NonVirtualMethodCaller(env);
    }
    return caller->invokeNonVirtual(env, method, obj, args);
}

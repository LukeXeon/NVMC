package open.source.reflect;

import java.lang.reflect.Method;

public class NonVirtualMethodCaller {

    // Used to load the 'nvmc' library on application startup.
    static {
        System.loadLibrary("nvmc");
    }

    public synchronized static native Object invokeNonVirtual(Method method, Object obj, Object[] args);

}
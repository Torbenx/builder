#include "util/target.hpp"

bool isWindows(){
    return IS_WINDOWS;
}

bool isAndroid(){
    return IS_ANDROID;
}

bool rawInput(){
    return isWindows();
}

bool usingGLES(){
    return isAndroid();
}

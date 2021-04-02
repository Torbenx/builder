#pragma once

#include "math/Mat4x4.hpp"
#include "util/log.hpp"

class MvpMatrix{
    public:
        Mat4x4 model;
        Mat4x4 view;
        Mat4x4 projection;
        Mat4x4 setupMvp(Mat4x4 objModel){
            return projection*(view*(objModel*model));
        }
        Mat4x4 setupMvp(){
            return projection*(view*model);
        }
        void debug(){
            LOGD("Model:");
            model.debug();
            LOGD("View:");
            view.debug();
            LOGD("Projection:");
            projection.debug();
            LOGD("Mvp:");
            setupMvp().debug();
        }
};

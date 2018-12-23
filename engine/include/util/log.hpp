//
// Created by Torben Thaysen on 03.02.2016.
//

#ifndef UNTITLED1_LOG_HPP
#define UNTITLED1_LOG_HPP

#include<stdio.h>

#define LOGE(msg, ...) printf(msg"\n", ##__VA_ARGS__)
#define LOGW(msg, ...) printf(msg"\n", ##__VA_ARGS__)
#define LOGI(msg, ...) printf(msg"\n", ##__VA_ARGS__)
#define LOGD(msg, ...) printf(msg"\n", ##__VA_ARGS__)

#endif //UNTITLED1_LOG_HPP

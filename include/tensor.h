#ifndef TENSOR_H
#define TENSOR_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Tensor Tensor;

struct Tensor{
    float* data;      // 存储张量的实际数据
    int* dims;        // 存储每个维度的大小，如[batch_size, channels, height, width]
    int ndims;        // 维度数量，如4表示4维张量
    bool is_param;    // 是否为模型参数，用于区分是否需要梯度
};

// 基础操作
Tensor* tensor_create(int* dims, int ndims, bool is_param);
void    tensor_free(Tensor* tensor);

// 基础数学运算
Tensor* tensor_mathmul(Tensor* a, Tensor* b);      // 矩阵乘法
Tensor* tensor_add(Tensor* a, Tensor* b);          // 张量加法
Tensor* tensor_sub(Tensor* a, Tensor* b);          // 张量减法
Tensor* tensor_mul(Tensor* a, Tensor* b);          // 张量逐元素乘法
Tensor* tensor_div(Tensor* a, Tensor* b);          // 张量除法
Tensor* tensor_pow(Tensor* a, float power);        // 张量幂运算

// 统计运算
Tensor* tensor_mean(Tensor* a, int axis);          // 沿指定轴计算平均值
Tensor* tensor_sum(Tensor* a, int axis);           // 沿指定轴求和
Tensor* tensor_max(Tensor* a, int axis);           // 沿指定轴求最大值
Tensor* tensor_min(Tensor* a, int axis);           // 沿指定轴求最小值

// 形状操作
Tensor* tensor_reshape(Tensor* a, int* new_dims, int new_ndims);  // 重塑张量形状
Tensor* tensor_transpose(Tensor* a, int* perm);                   // 转置张量
Tensor* tensor_concat(Tensor* a, Tensor* b, int axis);           // 在指定轴上拼接张量

// 激活函数
Tensor* tensor_relu(Tensor* a);                    // ReLU激活函数
Tensor* tensor_sigmoid(Tensor* a);                 // Sigmoid激活函数
Tensor* tensor_tanh(Tensor* a);                    // Tanh激活函数
Tensor* tensor_softmax(Tensor* a, int axis);       // Softmax函数

// 辅助函数
void    tensor_fill(Tensor* a, float value);                      // 用指定值填充张量
void    tensor_random_normal(Tensor* a, float mean, float std);   // 正态分布初始化
void    tensor_random_uniform(Tensor* a, float min, float max);   // 均匀分布初始化
bool    tensor_equal(Tensor* a, Tensor* b);                       // 判断两个张量是否相等

#endif

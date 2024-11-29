#ifndef MODEL_H
#define MODEL_H

typedef struct ModelConfig ModelConfig;
typedef struct TransformerModel TransformerModel;

// 模型配置结构
struct ModelConfig {
    int model_dim;            // 模型维度，通常是512或768
    int num_heads;            // 注意力头数，通常是8或12
    int vocab_size;           // 词汇表大小，根据数据集确定
    int max_seq_length;       // 最大序列长度，如512或1024
    int num_layers;           // Transformer层数，如6或12层
    float learning_rate;      // 学习率，通常从0.0001到0.01
    float dropout_rate;       // Dropout比率，通常0.1-0.3
    bool use_bias;            // 是否在层中使用偏置项
    int warmup_steps;         // 学习率预热步数
    float weight_decay;       // 权重衰减系数，用于正则化
    bool use_layer_norm;      // 是否使用层归一化
    int seed;                 // 随机数种子，用于复现结果
};

// Transformer 模型结构
struct TransformerModel {
    // 基本参数
    int model_dim;
    int num_heads;
    int vocab_size;
    int max_seq_length;
    float dropout_rate;
    
    // 层数
    int num_encoder_layers;
    int num_decoder_layers;
    
    // 层
    EncoderLayer** encoder_layers;
    DecoderLayer** decoder_layers;
    
    // 配置
    ModelConfig* config;
};


#endif
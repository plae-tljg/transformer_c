// 嵌入层实现, code to implement the embedding layers, 
// first by implementing the token embedding layer, then the positional encoding layer, 
// and finally the transformer embedding layer by adding their outputs together

#include "embeddings.h"
#include "tensor.h"
#include "dropout.h"
#include "model_config.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#pragma region Embedding Creation Functions
// 创建Token嵌入结构
// embedding_matrix tensor shape: [batch_size, vocab_size, embedding_dim], embedding_dim is the same as encoding_dim, d_model
TokenEmbedding* token_embedding_create(int vocab_size, int embedding_dim) {
    TokenEmbedding* token_emb = (TokenEmbedding*)malloc(sizeof(TokenEmbedding));
    if (!token_emb) {
        fprintf(stderr, "Failed to allocate memory for token embedding\n");
        return NULL;
    }

    // 初始化基本参数
    token_emb->vocab_size = vocab_size;
    token_emb->embedding_dim = embedding_dim;

    // 获取batch_size
    const int batch_size = g_model_config.batch_size;

    // 创建嵌入矩阵张量 [batch_size, vocab_size, embedding_dim]
    int shape[] = {batch_size, vocab_size, embedding_dim};
    token_emb->embedding_matrix = tensor_create(shape, 3);
    if (!token_emb->embedding_matrix) {
        free(token_emb);
        return NULL;
    }

    return token_emb;
}


// 创建位置编码结构
// encodings tensor shape: [max_seq_length, encoding_dim]
PositionalEncoding* positional_encoding_create(int max_seq_length, int encoding_dim) {
    PositionalEncoding* pos_enc = (PositionalEncoding*)malloc(sizeof(PositionalEncoding));
    if (!pos_enc) {
        fprintf(stderr, "Failed to allocate memory for positional encoding\n");
        return NULL;
    }

    // 初始化基本参数
    pos_enc->max_seq_length = max_seq_length;
    pos_enc->encoding_dim = encoding_dim;
    const int batch_size = g_model_config.batch_size;

    // 创建位置编码张量 [batch_size, max_seq_length, encoding_dim]
    int shape[] = {batch_size, max_seq_length, encoding_dim};
    pos_enc->encodings = tensor_create(shape, 3);
    if (!pos_enc->encodings) {
        free(pos_enc);
        return NULL;
    }

    // 计算位置编码, need to be optimized later
    for (int batch = 0; batch < batch_size; batch++) {
        for (int pos = 0; pos < max_seq_length; pos++) {
            for (int i = 0; i < encoding_dim; i += 2) {
                float angle = pos / powf(10000.0f, (float)i / encoding_dim);
                int idx = (batch * max_seq_length * encoding_dim) + (pos * encoding_dim) + i;
                pos_enc->encodings->data[idx] = sinf(angle);    // this part modify later to use sin and cos lookup table from model file
                if (i + 1 < encoding_dim) {
                    pos_enc->encodings->data[idx + 1] = cosf(angle);
                }
            }
        }
    }

    return pos_enc;
}

TransformerEmbedding* transformer_embedding_create(
    int vocab_size, 
    int embedding_dim, 
    int max_seq_length
) {
    TransformerEmbedding* trans_emb = (TransformerEmbedding*)malloc(sizeof(TransformerEmbedding));
    if (!trans_emb) {
        return NULL;
    }

    // 创建token嵌入
    trans_emb->token_embedding = token_embedding_create(vocab_size, embedding_dim);
    if (!trans_emb->token_embedding) {
        free(trans_emb);
        return NULL;
    }

    // 创建位置编码
    trans_emb->positional_encoding = positional_encoding_create(max_seq_length, embedding_dim);
    if (!trans_emb->positional_encoding) {
        free_token_embedding(trans_emb->token_embedding);
        free(trans_emb);
        return NULL;
    }

    return trans_emb;
}

#pragma endregion

#pragma region Forward Pass Functions
// token_embedding forward pass
// embedding_matrix tensor shape: [vocab_size, embedding_dim], embedding_dim is the same as encoding_dim, d_model, is dictionary of embedding vectors for each token
// input tokens shape: [batch_size, seq_length], entry is the index of the token in the vocabulary, to be generated by tokenizer
// output tensor shape: [batch_size, seq_length, embedding_dim], entry is the embedding vector of the token
bool token_embedding_forward(const TokenEmbedding* embedding, const Tensor* tokens, Tensor* output) {
    if (tokens->num_dims != 2) {
        fprintf(stderr, "Tokens tensor must be 2-dimensional [batch_size, seq_length]\n");
        return false;
    }

    int batch_size = tokens->shape[0];
    int seq_length = tokens->shape[1];
    int embedding_dim = embedding->embedding_dim;   // embedding_dim is the same as encoding_dim, d_model

    // 检查输出张量维度 [batch_size, seq_length, embedding_dim]
    if (output->num_dims != 3 || 
        output->shape[0] != batch_size ||
        output->shape[1] != seq_length ||
        output->shape[2] != embedding_dim) {
        fprintf(stderr, "Invalid output tensor dimensions\n");
        return false;
    }

    // 对每个batch和序列位置进行嵌入查找
    for (int b = 0; b < batch_size; b++) {
        for (int s = 0; s < seq_length; s++) {
            int token = (int)tokens->data[b * seq_length + s];  // (b,s) entry
            if (token >= embedding->vocab_size) {
                fprintf(stderr, "Token id exceeds vocabulary size\n");  // here may modify to unknown token later
                return false;
            }
            
            // 复制对应的嵌入向量
            memcpy(
                output->data + (b * seq_length + s) * embedding_dim,    // (b,s) entry of output tensor
                embedding->embedding_matrix->data + token * embedding_dim,  // (token,:) entry of embedding_matrix
                embedding_dim * sizeof(float)   // copy embedding_dim elements, so copy a vector
            );
        }
    }
    return true;
}

// positional_encoding forward pass
// input tensor shape: [batch_size, seq_length, encoding_dim], encoding_dim is the same as embedding_dim, d_model, generated by token_embedding_forward
// adds positional encoding directly to input tensor, shape remains the same
bool positional_encoding_forward(const PositionalEncoding* pos_enc, Tensor* input) {
    if (input->num_dims != 3) {
        fprintf(stderr, "Input tensor must be 3-dimensional [batch_size, seq_length, encoding_dim]\n");
        return false;
    }

    int batch_size = input->shape[0];
    int seq_length = input->shape[1];
    int encoding_dim = input->shape[2];

    if (encoding_dim != pos_enc->encoding_dim) {
        fprintf(stderr, "Encoding dimension mismatch\n");
        return false;
    }

    // 确保序列长度不超过最大长度
    if (seq_length > pos_enc->max_seq_length) {
        seq_length = pos_enc->max_seq_length;
    }

    // 对每个batch添加位置编码
    for (int b = 0; b < batch_size; b++) {
        for (int s = 0; s < seq_length; s++) {
            for (int d = 0; d < encoding_dim; d++) {
                input->data[(b * seq_length + s) * encoding_dim + d] +=
                    pos_enc->encodings->data[s * encoding_dim + d];  // (s,:) entry of encodings
            }
        }
    }
    return true;
}

// transformer_embedding forward pass
// input tokens shape: [batch_size, seq_length]
// output tensor shape: [batch_size, seq_length, embedding_dim]
bool transformer_embedding_forward(
    const TransformerEmbedding* trans_emb,
    const Tensor* tokens,
    Tensor* output
) {
    // 执行token嵌入
    if (!token_embedding_forward(trans_emb->token_embedding, tokens, output)) {
        return false;
    }
    
    // 添加位置编码
    if (!positional_encoding_forward(trans_emb->positional_encoding, output)) {
        return false;
    }

    // 应用dropout
    Tensor* dropout_output = dropout_forward(output, g_model_config.dropout_prob, false);   // 测试阶段不进行dropout, change it later to include training flag in model_config
    if (!dropout_output) {
        return false;
    }

    // 复制dropout结果到输出tensor
    memcpy(output->data, dropout_output->data, 
           calculate_total_size(output->shape, output->num_dims) * sizeof(float));
    
    // 释放临时tensor
    tensor_free(dropout_output);
    
    return true;
}
#pragma endregion

#pragma region Memory Management Functions
void free_token_embedding(TokenEmbedding* token_emb) {
    if (token_emb) {
        tensor_free(token_emb->embedding_matrix);
        free(token_emb);
    }
}

void free_positional_encoding(PositionalEncoding* pos_enc) {
    if (pos_enc) {
        tensor_free(pos_enc->encodings);
        free(pos_enc);
    }
}

void free_transformer_embedding(TransformerEmbedding* trans_emb) {
    if (trans_emb) {
        free_token_embedding(trans_emb->token_embedding);
        free_positional_encoding(trans_emb->positional_encoding);
        free(trans_emb);
    }
}
#pragma endregion


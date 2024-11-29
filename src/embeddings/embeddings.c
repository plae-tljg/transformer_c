#include "embeddings.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// 创建位置编码结构
PositionalEncoding* positional_encoding_create(int max_seq_length, int encoding_dim) {
    PositionalEncoding* pos_enc = (PositionalEncoding*)malloc(sizeof(PositionalEncoding));
    if (!pos_enc) {
        fprintf(stderr, "Failed to allocate memory for positional encoding\n");
        exit(EXIT_FAILURE);
    }

    // 初始化基本参数
    pos_enc->max_seq_length = max_seq_length;
    pos_enc->encoding_dim = encoding_dim;

    // 分配位置编码矩阵内存
    pos_enc->encodings = (float*)malloc(max_seq_length * encoding_dim * sizeof(float));
    if (!pos_enc->encodings) {
        fprintf(stderr, "Failed to allocate memory for positional encoding\n");
        free(pos_enc);
        return NULL;
    }
    for (int pos = 0; pos < max_seq_length; pos++) {
        for (int i = 0; i < encoding_dim; i += 2) {
            float angle = pos / powf(10000.0f, (float)i / encoding_dim);
            pos_enc->encodings[pos * encoding_dim + i] = sinf(angle);
            
            if (i + 1 < encoding_dim) {
                pos_enc->encodings[pos * encoding_dim + i + 1] = cosf(angle);
            }
        }
    }

    return pos_enc;
}

// 创建Token嵌入结构
TokenEmbedding* token_embedding_create(int vocab_size, int embedding_dim, bool requires_grad) {
    TokenEmbedding* token_emb = (TokenEmbedding*)malloc(sizeof(TokenEmbedding));
    if (!token_emb) {
        fprintf(stderr, "Failed to allocate memory for token embedding\n");
        exit(EXIT_FAILURE);
    }

    // 初始化基本参数
    token_emb->vocab_size = vocab_size;
    token_emb->embedding_dim = embedding_dim;
    token_emb->requires_grad = requires_grad;

    // 分配嵌入矩阵内存
    token_emb->embedding_matrix = (float*)malloc(vocab_size * embedding_dim * sizeof(float));
    if (!token_emb->embedding_matrix) {
        fprintf(stderr, "Failed to allocate memory for embedding matrix\n");
        free(token_emb);
        return NULL;
    }

    return token_emb;
}

// 创建Transformer嵌入层
TransformerEmbedding* transformer_embedding_create(int vocab_size, int embedding_dim, int max_seq_length, bool requires_grad) {
    TransformerEmbedding* trans_emb = (TransformerEmbedding*)malloc(sizeof(TransformerEmbedding));
    if (!trans_emb) {
        fprintf(stderr, "Failed to allocate memory for transformer embedding\n");
        return NULL;
    }

    // 创建token嵌入
    trans_emb->token_embedding = token_embedding_create(vocab_size, embedding_dim, requires_grad);
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

void free_positional_encoding(PositionalEncoding* pos_enc) {
    if (!pos_enc) return;
    
    if (pos_enc->encodings) {
        free(pos_enc->encodings);
    }
    
    free(pos_enc);
}

void free_token_embedding(TokenEmbedding* token_emb) {
    if (!token_emb) return;
    
    if (token_emb->embedding_matrix) {
        free(token_emb->embedding_matrix);
    }
    
    free(token_emb);
}

void free_transformer_embedding(TransformerEmbedding* trans_emb) {
    if (!trans_emb) return;
    
    free_token_embedding(trans_emb->token_embedding);
    free_positional_encoding(trans_emb->positional_encoding);
    free(trans_emb);
}

// Forward pass of the token embedding layer
void token_embedding_forward(TokenEmbedding* embedding, int* tokens, float* output) {
    int vocab_size = embedding->vocab_size;
    int embedding_dim = embedding->embedding_dim;
    float* embedding_matrix = embedding->embedding_matrix;

    // Loop through each token in the input sequence
    for (int i = 0; i < vocab_size; i++) {
        int token = tokens[i];
        // Look up the embedding for the current token
        float* embedding = embedding_matrix + token * embedding_dim;
        // Copy the embedding to the output array
        memcpy(output + i * embedding_dim, embedding, embedding_dim * sizeof(float));
    }
}

// Forward pass of the positional encoding layer
void positional_encoding_forward(PositionalEncoding* pos_enc, int seq_length, float* output) {
    int encoding_dim = pos_enc->encoding_dim;
    float* encodings = pos_enc->encodings;

    // 确保序列长度不超过最大长度
    if (seq_length > pos_enc->max_seq_length) {
        seq_length = pos_enc->max_seq_length;
    }

    // 复制位置编码到输出
    for (int pos = 0; pos < seq_length; pos++) {
        memcpy(
            output + pos * encoding_dim,
            encodings + pos * encoding_dim,
            encoding_dim * sizeof(float)
        );
    }
}

// Forward pass of transformer embedding layer
void transformer_embedding_forward(TransformerEmbedding* trans_emb, int* tokens, int seq_length, float* output) {
    int embedding_dim = trans_emb->token_embedding->embedding_dim;
    
    // 临时缓冲区用于存储token嵌入结果
    float* token_output = (float*)malloc(seq_length * embedding_dim * sizeof(float));
    
    // 计算token嵌入
    token_embedding_forward(trans_emb->token_embedding, tokens, token_output);
    
    // 复制token嵌入到输出
    memcpy(output, token_output, seq_length * embedding_dim * sizeof(float));
    
    // 添加位置编码
    positional_encoding_forward(trans_emb->positional_encoding, seq_length, output);
    
    // 释放临时缓冲区
    free(token_output);
}

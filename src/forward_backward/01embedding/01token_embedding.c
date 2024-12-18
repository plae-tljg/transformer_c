#include "01token_embedding.h"
#include "lookup.h"
#include "model_config.h"
#include <stdio.h>
#include <stdlib.h>

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

void free_token_embedding(TokenEmbedding* token_emb) {
    if (token_emb) {
        tensor_free(token_emb->embedding_matrix);
        free(token_emb);
    }
}

// token_embedding forward pass
// embedding_matrix tensor shape: [vocab_size, embedding_dim], 
// embedding_dim is the same as encoding_dim, d_model, is dictionary of embedding vectors for each token
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

    return perform_embedding_lookup(embedding->embedding_matrix, tokens, output, batch_size, seq_length, embedding_dim);
}



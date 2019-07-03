#include "wrapper.h"

#include <cstring>
#include <cstdio>

#include "activation.h"
#include "constants.h"
#include "fc.h"
#include "rnn.h"
#include "softmax.h"
#include "types.h"

inline void copy_array(FDATA_T* src, FDATA_T* dst, LDATA_T len) {
  for (LDATA_T i = 0; i < len; i++)
    dst[i] = src[i];
}

inline void init_rnn_state(FDATA_T state[BATCH_SIZE * RNN_STATE_SIZE]) {
  for (LDATA_T i = 0; i < BATCH_SIZE * RNN_STATE_SIZE; i++)
    state[i] = 0;
}

inline void init_fc_state(FDATA_T state[BATCH_SIZE * FC_OUTPUT_SIZE]) {
  for (LDATA_T i = 0; i < BATCH_SIZE * FC_OUTPUT_SIZE; i++)
    state[i] = 0;
}

// finish 1 batch, e.g. 64, of computation, return the probability distribution
void wrapper_rnn_fc(
    FDATA_T word_embeddings[WORD_NUM * WORD_SIZE],
    FDATA_T rnn_kernel[RNN_INPUT_SIZE * RNN_STATE_SIZE], 
    FDATA_T rnn_recurrent_kernel[RNN_STATE_SIZE * RNN_STATE_SIZE], 
    FDATA_T rnn_bias[RNN_STATE_SIZE], 
    FDATA_T fc_kernel[FC_OUTPUT_SIZE * FC_INPUT_SIZE], 
    FDATA_T fc_bias[FC_OUTPUT_SIZE], 
    IDATA_T input_word_idx[BATCH_SIZE],
    FDATA_T rnn_input_state_cache[BATCH_SIZE * RNN_INPUT_SIZE],
    FDATA_T rnn_last_state[BATCH_SIZE * RNN_STATE_SIZE], 
    FDATA_T rnn_output_state[BATCH_SIZE * RNN_STATE_SIZE],
    FDATA_T fc_output_cache[BATCH_SIZE * FC_OUTPUT_SIZE], 
    IDATA_T result_idx[BATCH_SIZE]) {
  // input:
  //  word_embeddings, rnn weights, and fc weights
  //  last state, input word_idx
  // output:
  //  rnn_output_state, current generated word index
  // cache:
  //  fc_output_cache, avoid malloc every time we call this function

  init_rnn_state(rnn_output_state);
  for (LDATA_T i = 0; i < BATCH_SIZE; i++) {
    LDATA_T rnn_input_state_cache_idx = i * RNN_INPUT_SIZE;
    memcpy(rnn_input_state_cache + rnn_input_state_cache_idx, 
           &word_embeddings[input_word_idx[i] * RNN_INPUT_SIZE], 
           sizeof(IDATA_T) * RNN_INPUT_SIZE);
    // printf("%d\t", input_word_idx[i]);
  }

  rnn(rnn_last_state, rnn_input_state_cache,
      rnn_bias, rnn_kernel, rnn_recurrent_kernel, rnn_output_state);
#ifdef DEBUG
	if (i == 24) {
		//printf("step 1 output:\n RNN_STATE_SIZE: %d", RNN_STATE_SIZE);
		for (LDATA_T j = 0; j < 2 * RNN_STATE_SIZE; j++) {
			printf("%f\t", (state1[j]));
		}
		printf("\n");
	}
#endif

  init_fc_state(fc_output_cache);
  // the output state feed to fc layer
  fc(/* input_feature_map = */rnn_output_state, fc_bias, fc_kernel, 
     /* output_feature_map = */fc_output_cache);

  argmax<FDATA_T,LDATA_T> (fc_output_cache, result_idx);
}

/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "Operations"

#include "HalInterfaces.h"
#include "IndexedShapeWrapper.h"
#include "LSTM.h"
#include "OperationResolver.h"
#include "OperationsUtils.h"

namespace android {
namespace nn {
namespace unidirectional_sequence_lstm {

// Inputs
constexpr uint32_t kNumInputs = 28;

// Input tensor of size {max_time, n_batch, n_input}
constexpr uint32_t kInputTensor = 0;

// Input weight tensors of size: {n_cell, n_input}
constexpr uint32_t kInputToInputWeightsTensor = 1;  // Optional
constexpr uint32_t kInputToForgetWeightsTensor = 2;
constexpr uint32_t kInputToCellWeightsTensor = 3;
constexpr uint32_t kInputToOutputWeightsTensor = 4;

// Recurrent weight tensors of size {n_cell, n_output}
constexpr uint32_t kRecurrentToInputWeightsTensor = 5;  // Optional
constexpr uint32_t kRecurrentToForgetWeightsTensor = 6;
constexpr uint32_t kRecurrentToCellWeightsTensor = 7;
constexpr uint32_t kRecurrentToOutputWeightsTensor = 8;

// Peephole weights tensors of size {n_cell}, representing a diagonal matrix.
constexpr uint32_t kCellToInputWeightsTensor = 9;    // Optional
constexpr uint32_t kCellToForgetWeightsTensor = 10;  // Optional
constexpr uint32_t kCellToOutputWeightsTensor = 11;  // Optional

// Gates bias tensors of size {n_cell}
constexpr uint32_t kInputGateBiasTensor = 12;  // Optional
constexpr uint32_t kForgetGateBiasTensor = 13;
constexpr uint32_t kCellGateBiasTensor = 14;
constexpr uint32_t kOutputGateBiasTensor = 15;

// Projection weight tensor of size {n_output, n_cell}
constexpr uint32_t kProjectionWeightsTensor = 16;  // Optional
// Projection bias tensor of size {n_output}
constexpr uint32_t kProjectionBiasTensor = 17;  // Optional

// Input from the output of the previous step, tensor of size {batch_size, n_output}
constexpr uint32_t kOutputStateInTensor = 18;
// Input from the cell state of the previous step, tensor of size {batch_size, n_cell}
constexpr uint32_t kCellStateInTensor = 19;

constexpr uint32_t kActivationParam = 20;
constexpr uint32_t kCellClipParam = 21;
constexpr uint32_t kProjClipParam = 22;
constexpr uint32_t kTimeMajorParam = 23;

// Layer norm weights tensors of size {n_cell}, representing a diagonal matrix.
constexpr uint32_t kInputLayerNormWeightsTensor = 24;   // Optional
constexpr uint32_t kForgetLayerNormWeightsTensor = 25;  // Optional
constexpr uint32_t kCellLayerNormWeightsTensor = 26;    // Optional
constexpr uint32_t kOutputLayerNormWeightsTensor = 27;  // Optional

// Output tensors.
constexpr uint32_t kNumOutputs = 4;

constexpr uint32_t kScratchBufferTensor = 0;
constexpr uint32_t kOutputStateOutTensor = 1;
constexpr uint32_t kCellStateOutTensor = 2;
constexpr uint32_t kOutputTensor = 3;

namespace {

inline bool hasTensor(IOperationExecutionContext* context, const uint32_t tensor) {
    return context->getInputBuffer(tensor) != nullptr;
}

inline bool isTimeMajor(IOperationExecutionContext* context) {
    return context->getInputValue<bool>(kTimeMajorParam);
}

template <typename T>
inline LSTMParams getLSTMParams(IOperationExecutionContext* context) {
    LSTMParams params;
    params.activation =
            static_cast<TfLiteFusedActivation>(context->getInputValue<int32_t>(kActivationParam));
    params.cell_clip = static_cast<float>(context->getInputValue<T>(kCellClipParam));
    params.proj_clip = static_cast<float>(context->getInputValue<T>(kProjClipParam));
    params.use_cifg = !hasTensor(context, kInputToInputWeightsTensor);
    params.use_peephole = hasTensor(context, kCellToOutputWeightsTensor);
    params.use_layer_norm = hasTensor(context, kInputLayerNormWeightsTensor);
    params.use_projection_weight = hasTensor(context, kProjectionWeightsTensor);
    params.use_projection_bias = hasTensor(context, kProjectionBiasTensor);
    return params;
}

}  // namespace

bool validate(const IOperationValidationContext* context) {
    NN_RET_CHECK_EQ(context->getNumInputs(), kNumInputs);
    NN_RET_CHECK_EQ(context->getNumOutputs(), kNumOutputs);
    const OperandType inputType = context->getInputType(kInputTensor);
    std::vector<OperandType> inExpectedTypes;
    std::vector<OperandType> outExpectedTypes;
    if (inputType == OperandType::TENSOR_FLOAT32) {
        inExpectedTypes = {OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32,
                           OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32,
                           OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32,
                           OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32,
                           OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32,
                           OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32,
                           OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32,
                           OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32,
                           OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32,
                           OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32,
                           OperandType::INT32,          OperandType::FLOAT32,
                           OperandType::FLOAT32,        OperandType::BOOL,
                           OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32,
                           OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32};
        outExpectedTypes = {OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32,
                            OperandType::TENSOR_FLOAT32, OperandType::TENSOR_FLOAT32};
    } else if (inputType == OperandType::TENSOR_FLOAT16) {
        inExpectedTypes = {OperandType::TENSOR_FLOAT16, OperandType::TENSOR_FLOAT16,
                           OperandType::TENSOR_FLOAT16, OperandType::TENSOR_FLOAT16,
                           OperandType::TENSOR_FLOAT16, OperandType::TENSOR_FLOAT16,
                           OperandType::TENSOR_FLOAT16, OperandType::TENSOR_FLOAT16,
                           OperandType::TENSOR_FLOAT16, OperandType::TENSOR_FLOAT16,
                           OperandType::TENSOR_FLOAT16, OperandType::TENSOR_FLOAT16,
                           OperandType::TENSOR_FLOAT16, OperandType::TENSOR_FLOAT16,
                           OperandType::TENSOR_FLOAT16, OperandType::TENSOR_FLOAT16,
                           OperandType::TENSOR_FLOAT16, OperandType::TENSOR_FLOAT16,
                           OperandType::TENSOR_FLOAT16, OperandType::TENSOR_FLOAT16,
                           OperandType::INT32,          OperandType::FLOAT16,
                           OperandType::FLOAT16,        OperandType::BOOL,
                           OperandType::TENSOR_FLOAT16, OperandType::TENSOR_FLOAT16,
                           OperandType::TENSOR_FLOAT16, OperandType::TENSOR_FLOAT16};
        outExpectedTypes = {OperandType::TENSOR_FLOAT16, OperandType::TENSOR_FLOAT16,
                            OperandType::TENSOR_FLOAT16, OperandType::TENSOR_FLOAT16};
    } else {
        NN_RET_CHECK_FAIL()
                << "Unsupported input operand type for UNIDIRECTIONAL_SEQUENCE_LSTM op: "
                << toString(inputType);
    }
    NN_RET_CHECK(validateInputTypes(context, inExpectedTypes));
    NN_RET_CHECK(validateOutputTypes(context, outExpectedTypes));
    return validateHalVersion(context, HalVersion::V1_2);
}

bool prepare(IOperationExecutionContext* context) {
    const Shape inputShape = context->getInputShape(kInputTensor);
    const uint32_t inputRank = getNumberOfDimensions(inputShape);
    NN_RET_CHECK_EQ(inputRank, 3) << "Invalid input tensor rank: " << inputRank;

    const uint32_t maxTime = getSizeOfDimension(inputShape, isTimeMajor(context) ? 0 : 1);
    const uint32_t batchSize = getSizeOfDimension(inputShape, isTimeMajor(context) ? 1 : 0);
    const uint32_t inputSize = getSizeOfDimension(inputShape, inputRank - 1);

    const Shape inputToOutputShape = context->getInputShape(kInputToOutputWeightsTensor);
    NN_RET_CHECK_EQ(getNumberOfDimensions(inputToOutputShape), 2);
    NN_RET_CHECK_EQ(getSizeOfDimension(inputToOutputShape, 1), inputSize);
    const uint32_t numCells = getSizeOfDimension(inputToOutputShape, 0);

    const Shape recurrentToOutputShape = context->getInputShape(kRecurrentToOutputWeightsTensor);
    NN_RET_CHECK_EQ(getNumberOfDimensions(recurrentToOutputShape), 2);
    NN_RET_CHECK_EQ(getSizeOfDimension(recurrentToOutputShape, 0), numCells);
    const uint32_t outputSize = getSizeOfDimension(recurrentToOutputShape, 1);

    if (hasTensor(context, kInputToInputWeightsTensor)) {
        const Shape inputToInputShape = context->getInputShape(kInputToInputWeightsTensor);
        NN_RET_CHECK_EQ(getNumberOfDimensions(inputToInputShape), 2);
        NN_RET_CHECK_EQ(getSizeOfDimension(inputToInputShape, 0), numCells);
        NN_RET_CHECK_EQ(getSizeOfDimension(inputToInputShape, 1), inputSize);
    }

    const Shape inputToForgetShape = context->getInputShape(kInputToForgetWeightsTensor);
    NN_RET_CHECK_EQ(getNumberOfDimensions(inputToForgetShape), 2);
    NN_RET_CHECK_EQ(getSizeOfDimension(inputToForgetShape, 0), numCells);
    NN_RET_CHECK_EQ(getSizeOfDimension(inputToForgetShape, 1), inputSize);
    const Shape inputToCellShape = context->getInputShape(kInputToCellWeightsTensor);
    NN_RET_CHECK_EQ(getNumberOfDimensions(inputToCellShape), 2);
    NN_RET_CHECK_EQ(getSizeOfDimension(inputToCellShape, 0), numCells);
    NN_RET_CHECK_EQ(getSizeOfDimension(inputToCellShape, 1), inputSize);

    if (hasTensor(context, kRecurrentToInputWeightsTensor)) {
        const Shape recurrentToInputShape = context->getInputShape(kRecurrentToInputWeightsTensor);
        NN_RET_CHECK_EQ(getNumberOfDimensions(recurrentToInputShape), 2);
        NN_RET_CHECK_EQ(getSizeOfDimension(recurrentToInputShape, 0), numCells);
        NN_RET_CHECK_EQ(getSizeOfDimension(recurrentToInputShape, 1), outputSize);
    }

    const Shape recurrentToForgetShape = context->getInputShape(kRecurrentToForgetWeightsTensor);
    NN_RET_CHECK_EQ(getNumberOfDimensions(recurrentToForgetShape), 2);
    NN_RET_CHECK_EQ(getSizeOfDimension(recurrentToForgetShape, 0), numCells);
    NN_RET_CHECK_EQ(getSizeOfDimension(recurrentToForgetShape, 1), outputSize);
    const Shape recurrentToCellShape = context->getInputShape(kRecurrentToCellWeightsTensor);
    NN_RET_CHECK_EQ(getNumberOfDimensions(recurrentToCellShape), 2);
    NN_RET_CHECK_EQ(getSizeOfDimension(recurrentToCellShape, 0), numCells);
    NN_RET_CHECK_EQ(getSizeOfDimension(recurrentToCellShape, 1), outputSize);

    if (hasTensor(context, kCellToInputWeightsTensor)) {
        const Shape cellToInputShape = context->getInputShape(kCellToInputWeightsTensor);
        NN_RET_CHECK_EQ(getNumberOfDimensions(cellToInputShape), 1);
        NN_RET_CHECK_EQ(getSizeOfDimension(cellToInputShape, 0), numCells);
    }

    if (hasTensor(context, kCellToForgetWeightsTensor)) {
        const Shape cellToForgetShape = context->getInputShape(kCellToForgetWeightsTensor);
        NN_RET_CHECK_EQ(getNumberOfDimensions(cellToForgetShape), 1);
        NN_RET_CHECK_EQ(getSizeOfDimension(cellToForgetShape, 0), numCells);
    }

    if (hasTensor(context, kCellToOutputWeightsTensor)) {
        const Shape cellToOutputShape = context->getInputShape(kCellToOutputWeightsTensor);
        NN_RET_CHECK_EQ(getNumberOfDimensions(cellToOutputShape), 1);
        NN_RET_CHECK_EQ(getSizeOfDimension(cellToOutputShape, 0), numCells);
    }

    if (hasTensor(context, kInputGateBiasTensor)) {
        const Shape inputGateBiasShape = context->getInputShape(kInputGateBiasTensor);
        NN_RET_CHECK_EQ(getNumberOfDimensions(inputGateBiasShape), 1);
        NN_RET_CHECK_EQ(getSizeOfDimension(inputGateBiasShape, 0), numCells);
    }

    const Shape forgetGateBiasShape = context->getInputShape(kForgetGateBiasTensor);
    NN_RET_CHECK_EQ(getNumberOfDimensions(forgetGateBiasShape), 1);
    NN_RET_CHECK_EQ(getSizeOfDimension(forgetGateBiasShape, 0), numCells);
    const Shape cellGateBiasShape = context->getInputShape(kCellGateBiasTensor);
    NN_RET_CHECK_EQ(getNumberOfDimensions(cellGateBiasShape), 1);
    NN_RET_CHECK_EQ(getSizeOfDimension(cellGateBiasShape, 0), numCells);
    const Shape outputGateBiasShape = context->getInputShape(kOutputGateBiasTensor);
    NN_RET_CHECK_EQ(getNumberOfDimensions(outputGateBiasShape), 1);
    NN_RET_CHECK_EQ(getSizeOfDimension(outputGateBiasShape, 0), numCells);

    if (hasTensor(context, kProjectionWeightsTensor)) {
        const Shape projectionShape = context->getInputShape(kProjectionWeightsTensor);
        NN_RET_CHECK_EQ(getNumberOfDimensions(projectionShape), 2);
        NN_RET_CHECK_EQ(getSizeOfDimension(projectionShape, 0), outputSize);
        NN_RET_CHECK_EQ(getSizeOfDimension(projectionShape, 1), numCells);
    }

    if (hasTensor(context, kProjectionBiasTensor)) {
        const Shape projectionBiasShape = context->getInputShape(kProjectionBiasTensor);
        NN_RET_CHECK_EQ(getNumberOfDimensions(projectionBiasShape), 1);
        NN_RET_CHECK_EQ(getSizeOfDimension(projectionBiasShape, 0), outputSize);
    }

    const Shape outputStateShape = context->getInputShape(kOutputStateInTensor);
    NN_RET_CHECK_EQ(getNumberOfDimensions(outputStateShape), 2);
    NN_RET_CHECK_EQ(getSizeOfDimension(outputStateShape, 0), batchSize);
    NN_RET_CHECK_EQ(getSizeOfDimension(outputStateShape, 1), outputSize);
    const Shape cellStateShape = context->getInputShape(kCellStateInTensor);
    NN_RET_CHECK_EQ(getNumberOfDimensions(cellStateShape), 2);
    NN_RET_CHECK_EQ(getSizeOfDimension(cellStateShape, 0), batchSize);
    NN_RET_CHECK_EQ(getSizeOfDimension(cellStateShape, 1), numCells);

    if (hasTensor(context, kInputLayerNormWeightsTensor)) {
        const Shape inputLayerNormShape = context->getInputShape(kInputLayerNormWeightsTensor);
        NN_RET_CHECK_EQ(getNumberOfDimensions(inputLayerNormShape), 1);
        NN_RET_CHECK_EQ(getSizeOfDimension(inputLayerNormShape, 0), numCells);
    }

    if (hasTensor(context, kForgetLayerNormWeightsTensor)) {
        const Shape forgetLayerNormShape = context->getInputShape(kForgetLayerNormWeightsTensor);
        NN_RET_CHECK_EQ(getNumberOfDimensions(forgetLayerNormShape), 1);
        NN_RET_CHECK_EQ(getSizeOfDimension(forgetLayerNormShape, 0), numCells);
    }

    if (hasTensor(context, kCellLayerNormWeightsTensor)) {
        const Shape cellLayerNormShape = context->getInputShape(kCellLayerNormWeightsTensor);
        NN_RET_CHECK_EQ(getNumberOfDimensions(cellLayerNormShape), 1);
        NN_RET_CHECK_EQ(getSizeOfDimension(cellLayerNormShape, 0), numCells);
    }

    if (hasTensor(context, kOutputLayerNormWeightsTensor)) {
        const Shape outputLayerNormShape = context->getInputShape(kOutputLayerNormWeightsTensor);
        NN_RET_CHECK_EQ(getNumberOfDimensions(outputLayerNormShape), 1);
        NN_RET_CHECK_EQ(getSizeOfDimension(outputLayerNormShape, 0), numCells);
    }

    Shape outputShape = context->getInputShape(kInputTensor);
    outputShape.dimensions[2] = outputSize;

    Shape scratchShape = context->getInputShape(kCellStateInTensor);
    const bool use_cifg = !hasTensor(context, kInputToInputWeightsTensor);
    scratchShape.dimensions[1] = use_cifg ? numCells * 3 : numCells * 4;

    return context->setOutputShape(kOutputTensor, outputShape) &&
           context->setOutputShape(kOutputStateOutTensor, outputStateShape) &&
           context->setOutputShape(kCellStateOutTensor, cellStateShape) &&
           context->setOutputShape(kScratchBufferTensor, scratchShape);
}

bool execute(IOperationExecutionContext* context) {
    const OperandType inputType = context->getInputType(kInputTensor);
    switch (inputType) {
        case OperandType::TENSOR_FLOAT32: {
            LSTMCell::LSTMEvalFloat32(
                    getLSTMParams<float>(context), context->getInputBuffer<float>(kInputTensor),
                    context->getInputShape(kInputTensor),
                    context->getInputBuffer<float>(kInputToInputWeightsTensor),
                    context->getInputBuffer<float>(kInputToForgetWeightsTensor),
                    context->getInputBuffer<float>(kInputToCellWeightsTensor),
                    context->getInputBuffer<float>(kInputToOutputWeightsTensor),
                    context->getInputShape(kInputToOutputWeightsTensor),
                    context->getInputBuffer<float>(kRecurrentToInputWeightsTensor),
                    context->getInputBuffer<float>(kRecurrentToForgetWeightsTensor),
                    context->getInputBuffer<float>(kRecurrentToCellWeightsTensor),
                    context->getInputBuffer<float>(kRecurrentToOutputWeightsTensor),
                    context->getInputShape(kRecurrentToOutputWeightsTensor),
                    context->getInputBuffer<float>(kCellToInputWeightsTensor),
                    context->getInputBuffer<float>(kCellToForgetWeightsTensor),
                    context->getInputBuffer<float>(kCellToOutputWeightsTensor),
                    context->getInputBuffer<float>(kInputGateBiasTensor),
                    context->getInputBuffer<float>(kForgetGateBiasTensor),
                    context->getInputBuffer<float>(kCellGateBiasTensor),
                    context->getInputBuffer<float>(kOutputGateBiasTensor),
                    context->getInputBuffer<float>(kProjectionWeightsTensor),
                    context->getInputBuffer<float>(kProjectionBiasTensor),
                    context->getInputBuffer<float>(kOutputStateInTensor),
                    context->getInputBuffer<float>(kCellStateInTensor),
                    context->getInputBuffer<float>(kInputLayerNormWeightsTensor),
                    context->getInputBuffer<float>(kForgetLayerNormWeightsTensor),
                    context->getInputBuffer<float>(kCellLayerNormWeightsTensor),
                    context->getInputBuffer<float>(kOutputLayerNormWeightsTensor),
                    context->getOutputBuffer<float>(kOutputStateOutTensor),
                    context->getOutputBuffer<float>(kCellStateOutTensor),
                    context->getOutputBuffer<float>(kOutputTensor),
                    context->getOutputBuffer<float>(kScratchBufferTensor), isTimeMajor(context));
        } break;
        case OperandType::TENSOR_FLOAT16: {
            LSTMCell::LSTMEvalFloat16(
                    getLSTMParams<_Float16>(context),
                    context->getInputBuffer<_Float16>(kInputTensor),
                    context->getInputShape(kInputTensor),
                    context->getInputBuffer<_Float16>(kInputToInputWeightsTensor),
                    context->getInputBuffer<_Float16>(kInputToForgetWeightsTensor),
                    context->getInputBuffer<_Float16>(kInputToCellWeightsTensor),
                    context->getInputBuffer<_Float16>(kInputToOutputWeightsTensor),
                    context->getInputShape(kInputToOutputWeightsTensor),
                    context->getInputBuffer<_Float16>(kRecurrentToInputWeightsTensor),
                    context->getInputBuffer<_Float16>(kRecurrentToForgetWeightsTensor),
                    context->getInputBuffer<_Float16>(kRecurrentToCellWeightsTensor),
                    context->getInputBuffer<_Float16>(kRecurrentToOutputWeightsTensor),
                    context->getInputShape(kRecurrentToOutputWeightsTensor),
                    context->getInputBuffer<_Float16>(kCellToInputWeightsTensor),
                    context->getInputBuffer<_Float16>(kCellToForgetWeightsTensor),
                    context->getInputBuffer<_Float16>(kCellToOutputWeightsTensor),
                    context->getInputBuffer<_Float16>(kInputGateBiasTensor),
                    context->getInputBuffer<_Float16>(kForgetGateBiasTensor),
                    context->getInputBuffer<_Float16>(kCellGateBiasTensor),
                    context->getInputBuffer<_Float16>(kOutputGateBiasTensor),
                    context->getInputBuffer<_Float16>(kProjectionWeightsTensor),
                    context->getInputBuffer<_Float16>(kProjectionBiasTensor),
                    context->getInputBuffer<_Float16>(kOutputStateInTensor),
                    context->getInputBuffer<_Float16>(kCellStateInTensor),
                    context->getInputBuffer<_Float16>(kInputLayerNormWeightsTensor),
                    context->getInputBuffer<_Float16>(kForgetLayerNormWeightsTensor),
                    context->getInputBuffer<_Float16>(kCellLayerNormWeightsTensor),
                    context->getInputBuffer<_Float16>(kOutputLayerNormWeightsTensor),
                    context->getOutputBuffer<_Float16>(kOutputStateOutTensor),
                    context->getOutputBuffer<_Float16>(kCellStateOutTensor),
                    context->getOutputBuffer<_Float16>(kOutputTensor),
                    context->getOutputBuffer<_Float16>(kScratchBufferTensor), isTimeMajor(context));
        } break;
        default: {
            LOG(ERROR) << "Unsupported data type: " << static_cast<int>(inputType);
            return false;
        }
    }
    return true;
}

}  // namespace unidirectional_sequence_lstm

NN_REGISTER_OPERATION(UNIDIRECTIONAL_SEQUENCE_LSTM, "UNIDIRECTIONAL_SEQUENCE_LSTM",
                      unidirectional_sequence_lstm::validate, unidirectional_sequence_lstm::prepare,
                      unidirectional_sequence_lstm::execute);

}  // namespace nn
}  // namespace android
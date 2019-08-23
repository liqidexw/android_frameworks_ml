// Generated from concat_quant8_3.mod.py
// DO NOT EDIT
// clang-format off
#include "GeneratedTests.h"


namespace generated_tests::concat_quant8_3 {

const ::test_helper::TestModel& get_test_model();

} // namespace generated_tests::concat_quant8_3

namespace android::hardware::neuralnetworks::V1_0::generated_tests::concat_quant8_3 {

TEST_F(GeneratedTest, concat_quant8_3) {
    Execute(device, ::generated_tests::concat_quant8_3::get_test_model());
}

TEST_F(ValidationTest, concat_quant8_3) {
    const Model model = createModel(::generated_tests::concat_quant8_3::get_test_model());
    const Request request = createRequest(::generated_tests::concat_quant8_3::get_test_model());
    validateEverything(model, request);
}

} // namespace android::hardware::neuralnetworks::V1_0::generated_tests::concat_quant8_3


namespace generated_tests::concat_quant8_3 {

const ::test_helper::TestModel& get_test_model_all_inputs_as_internal();

} // namespace generated_tests::concat_quant8_3

namespace android::hardware::neuralnetworks::V1_0::generated_tests::concat_quant8_3 {

TEST_F(GeneratedTest, concat_quant8_3_all_inputs_as_internal) {
    Execute(device, ::generated_tests::concat_quant8_3::get_test_model_all_inputs_as_internal());
}

TEST_F(ValidationTest, concat_quant8_3_all_inputs_as_internal) {
    const Model model = createModel(::generated_tests::concat_quant8_3::get_test_model_all_inputs_as_internal());
    const Request request = createRequest(::generated_tests::concat_quant8_3::get_test_model_all_inputs_as_internal());
    validateEverything(model, request);
}

} // namespace android::hardware::neuralnetworks::V1_0::generated_tests::concat_quant8_3

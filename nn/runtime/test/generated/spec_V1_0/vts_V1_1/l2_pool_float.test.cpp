// Generated from l2_pool_float.mod.py
// DO NOT EDIT
// clang-format off
#include "GeneratedTests.h"


namespace generated_tests::l2_pool_float {

const ::test_helper::TestModel& get_test_model();

} // namespace generated_tests::l2_pool_float

namespace android::hardware::neuralnetworks::V1_1::generated_tests::l2_pool_float {

TEST_F(GeneratedTest, l2_pool_float) {
    Execute(device, ::generated_tests::l2_pool_float::get_test_model());
}

TEST_F(ValidationTest, l2_pool_float) {
    const Model model = createModel(::generated_tests::l2_pool_float::get_test_model());
    const Request request = createRequest(::generated_tests::l2_pool_float::get_test_model());
    validateEverything(model, request);
}

} // namespace android::hardware::neuralnetworks::V1_1::generated_tests::l2_pool_float


namespace generated_tests::l2_pool_float {

const ::test_helper::TestModel& get_test_model_all_inputs_as_internal();

} // namespace generated_tests::l2_pool_float

namespace android::hardware::neuralnetworks::V1_1::generated_tests::l2_pool_float {

TEST_F(GeneratedTest, l2_pool_float_all_inputs_as_internal) {
    Execute(device, ::generated_tests::l2_pool_float::get_test_model_all_inputs_as_internal());
}

TEST_F(ValidationTest, l2_pool_float_all_inputs_as_internal) {
    const Model model = createModel(::generated_tests::l2_pool_float::get_test_model_all_inputs_as_internal());
    const Request request = createRequest(::generated_tests::l2_pool_float::get_test_model_all_inputs_as_internal());
    validateEverything(model, request);
}

} // namespace android::hardware::neuralnetworks::V1_1::generated_tests::l2_pool_float

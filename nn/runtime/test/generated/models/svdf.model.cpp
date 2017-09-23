// Generated file (from: svdf.mod.py). Do not edit
void CreateModel(Model *model) {
  OperandType type5(Type::TENSOR_FLOAT32, {2, 36});
  OperandType type0(Type::TENSOR_FLOAT32, {2, 3});
  OperandType type6(Type::TENSOR_FLOAT32, {2, 4});
  OperandType type2(Type::TENSOR_FLOAT32, {4, 10});
  OperandType type1(Type::TENSOR_FLOAT32, {4, 3});
  OperandType type3(Type::TENSOR_FLOAT32, {4});
  OperandType type4(Type::TENSOR_INT32, {1});
  // Phase 1, operands
  auto input = model->addOperand(&type0);
  auto weights_feature = model->addOperand(&type1);
  auto weights_time = model->addOperand(&type2);
  auto bias = model->addOperand(&type3);
  auto rank_param = model->addOperand(&type4);
  auto activation_param = model->addOperand(&type4);
  auto state = model->addOperand(&type5);
  auto output = model->addOperand(&type6);
  // Phase 2, operations
  model->addOperation(ANEURALNETWORKS_SVDF, {input, weights_feature, weights_time, bias, rank_param, activation_param}, {state, output});
  // Phase 3, inputs and outputs
  model->setInputsAndOutputs(
    {input, weights_feature, weights_time, bias, rank_param, activation_param},
    {state, output});
  assert(model->isValid());
}

bool is_ignored(int i) {
  static std::set<int> ignore = {0};
  return ignore.find(i) != ignore.end();
}
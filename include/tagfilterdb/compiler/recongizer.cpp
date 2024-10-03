#include "recongizer.hpp"

using namespace tagfilterdb;
using namespace tagfilerdb::compiler;

void Recognizer::InitializeInstanceFields() {
    _stateNumber = ATNState::INVALID_STATE_NUMBER;
    _interpreter = nullptr;
}

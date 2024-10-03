#include "ATN.hpp"
#include "decisionState.hpp"

using namespace tagfilterdb;
using namespace tagfilterdb::compiler;
using namespace tagfilterdb::compiler::atn;

ATN::ATN() : ATN(ATNType::LEXER, 0) {}

ATN::ATN(ATNType grammarType_, size_t maxTokenType_)
    : grammarType(grammarType_), maxTokenType(maxTokenType_) {}

ATN::~ATN() {
    for (ATNState *state : states) {
        delete state;
    }
}

void ATN::addState(ATNState *state) {
    if (state != nullptr) {
        state->stateNumber = static_cast<int>(states.size());
    }
    states.push_back(state);
}

void ATN::removeState(ATNState *state) {
    delete states.at(state->stateNumber);
    states.at(state->stateNumber) = nullptr;
}

int ATN::defineDecisionState(DecisionState *s) {
    decisionToState.push_back(s);
    s->decision = static_cast<int>(decisionToState.size() - 1);
    return s->decision;
}

DecisionState *ATN::getDecisionState(size_t decision) const {
    if (!decisionToState.empty()) {
        return decisionToState[decision];
    }
    return nullptr;
}

size_t ATN::getNumberOfDecisions() const { return decisionToState.size(); }

std::string ATN::toString() const {
    std::stringstream ss;
    std::string type;
    switch (grammarType) {
    case ATNType::LEXER:
        type = "LEXER ";
        break;

    case ATNType::PARSER:
        type = "PARSER ";
        break;

    default:
        break;
    }
    ss << "(" << type << "ATN " << std::hex << this << std::dec
       << ") maxTokenType: " << maxTokenType << std::endl;
    ss << "states (" << states.size() << ") {" << std::endl;

    size_t index = 0;
    for (auto *state : states) {
        if (state == nullptr) {
            ss << "  " << index++ << ": nul" << std::endl;
        } else {
            std::string text = state->toString();
            ss << "  " << index++ << ": " << text << std::endl;
        }
    }

    index = 0;
    for (auto *state : decisionToState) {
        if (state == nullptr) {
            ss << "  " << index++ << ": nul" << std::endl;
        } else {
            std::string text = state->toString();
            ss << "  " << index++ << ": " << text << std::endl;
        }
    }

    ss << "}";

    return ss.str();
}
#include "ATNState.hpp"

#include "tagfilterdb/support/rangeSet.hpp"
#include "transition.hpp"

using namespace tagfilterdb;
using namespace tagfilterdb::compiler::atn;

size_t ATNState::hashCode() const { return stateNumber; }

bool ATNState::equals(const ATNState &other) const {
    return stateNumber == other.stateNumber;
}

bool ATNState::isNonGreedyExitState() const { return false; }

std::string ATNState::toString() const { return std::to_string(stateNumber); }

void ATNState::addTransition(ConstTransitionPtr e) {
    addTransition(transitions.size(), std::move(e));
}

void ATNState::addTransition(size_t index, ConstTransitionPtr e) {
    for (const auto &transition : transitions) {
        if (transition->target->stateNumber == e->target->stateNumber) {
            return;
        }
        if (transitions.empty()) {
            epsilonOnlyTransitions = e->isEpsilon();
        } else if (epsilonOnlyTransitions != e->isEpsilon()) {
            std::cerr << "ATN state %d has both epsilon and non-epsilon "
                         "transitions.\n"
                      << stateNumber;
            epsilonOnlyTransitions = false;
        }

        transitions.insert(transitions.begin() + index, std::move(e));
    }
}

ConstTransitionPtr ATNState::removeTransition(size_t index) {
    ConstTransitionPtr result = std::move(transitions[index]);
    transitions.erase(transitions.begin() + index);
    return result;
}
//! @file see/JsonEmit.cpp
//! @brief OPT format JSON emission implementation.

#include "JsonEmit.h"

#include <cstdio>
#include <iomanip>
#include <sstream>

namespace see {

nlohmann::json JsonEmitter::toOPT(const TraceState& state) {
    nlohmann::json result;

    // Include source code
    result["code"] = state.getSourceCode();

    // Build trace array
    nlohmann::json trace = nlohmann::json::array();
    for (const auto& step : state.getSteps()) {
        trace.push_back(stepToJson(step));
    }
    result["trace"] = trace;

    return result;
}

nlohmann::json JsonEmitter::stepToJson(const TraceStep& step) {
    nlohmann::json result;

    result["line"] = step.line;

    // Event type
    switch (step.event) {
    case EventKind::Call:
        result["event"] = "call";
        break;
    case EventKind::Return:
        result["event"] = "return";
        break;
    case EventKind::StepLine:
    default:
        result["event"] = "step_line";
        break;
    }

    result["func_name"] = step.funcName;

    // Stack to render
    nlohmann::json stackToRender = nlohmann::json::array();
    for (const auto& frame : step.stack) {
        stackToRender.push_back(frameToJson(frame));
    }
    result["stack_to_render"] = stackToRender;

    // Globals (empty for now)
    result["globals"] = nlohmann::json::object();
    result["ordered_globals"] = nlohmann::json::array();

    // Heap (empty for now - Tier 3)
    result["heap"] = nlohmann::json::object();

    // Stdout
    result["stdout"] = step.stdout_capture;

    return result;
}

nlohmann::json JsonEmitter::frameToJson(const Frame& frame) {
    nlohmann::json result;

    result["frame_id"] = frame.frameId;
    result["func_name"] = frame.funcName;
    result["is_highlighted"] = frame.isHighlighted;
    result["is_zombie"] = frame.isZombie;

    // Encoded locals
    nlohmann::json encodedLocals = nlohmann::json::object();
    for (const auto& [name, var] : frame.locals) {
        encodedLocals[name] = valueToJson(var.value, var.type);
    }
    result["encoded_locals"] = encodedLocals;

    // Ordered variable names
    result["ordered_varnames"] = frame.orderedLocalNames;

    return result;
}

nlohmann::json JsonEmitter::valueToJson(const EncodedValue& value,
                                         const TypeDescriptor* type) {
    return std::visit(
        [type](auto&& arg) -> nlohmann::json {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, long long>) {
                return arg;
            } else if constexpr (std::is_same_v<T, unsigned long long>) {
                return arg;
            } else if constexpr (std::is_same_v<T, double>) {
                return arg;
            } else if constexpr (std::is_same_v<T, bool>) {
                return arg;
            } else if constexpr (std::is_same_v<T, char>) {
                // Encode as string for display
                return std::string(1, arg);
            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                // Pointer encoding: ["C_ADDRESS", "0x...", "type", "region"]
                nlohmann::json arr = nlohmann::json::array();
                for (const auto& s : arg) {
                    arr.push_back(s);
                }
                return arr;
            } else if constexpr (std::is_same_v<T, std::string>) {
                return arg;
            } else {
                return nullptr;
            }
        },
        value);
}

void JsonEmitter::emit(const TraceState& state) {
    nlohmann::json output = toOPT(state);
    std::fprintf(stderr, "%s\n", output.dump().c_str());
}

} // namespace see

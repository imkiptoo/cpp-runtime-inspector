//! @file inspector/JsonEmit.cpp
//! @brief OPT format JSON emission implementation.

#include "JsonEmit.h"

#include <cstdio>
#include <iomanip>
#include <sstream>

namespace inspector {

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

    // Include memory leaks if any
    const auto& leaks = state.getLeakedAllocations();
    if (!leaks.empty()) {
        nlohmann::json leakedArray = nlohmann::json::array();
        for (const auto& [heapId, typeName] : leaks) {
            nlohmann::json leakInfo = nlohmann::json::array();
            leakInfo.push_back("leaked");
            leakInfo.push_back(heapId);
            leakInfo.push_back(typeName);
            leakedArray.push_back(leakInfo);
        }
        result["memory_leaks"] = leakedArray;
    }

    // Include type metadata (sizeof, memory layout) for encountered types
    const auto& types = state.getEncounteredTypes();
    if (!types.empty()) {
        nlohmann::json typeMeta = nlohmann::json::object();
        for (const auto& [name, type] : types) {
            typeMeta[name] = typeDescriptorToJson(type);
        }
        result["type_metadata"] = typeMeta;
    }

    return result;
}

nlohmann::json JsonEmitter::heapObjectToJson(const HeapObject& obj) {
    nlohmann::json result = nlohmann::json::array();

    if (obj.isArray) {
        // Array: ["HEAP_ARRAY", typeName, [elem1, elem2, ...]]
        result.push_back("HEAP_ARRAY");
        result.push_back(obj.typeName);

        // The value should be an ArrayValue
        if (std::holds_alternative<ArrayValue>(obj.value)) {
            const auto& av = std::get<ArrayValue>(obj.value);
            nlohmann::json elements = nlohmann::json::array();
            for (const auto& elem : av.elements) {
                if (elem) {
                    elements.push_back(valueToJson(elem->value, elem->type));
                }
            }
            result.push_back(elements);
        } else {
            // Fallback: just include the encoded value
            result.push_back(valueToJson(obj.value, nullptr));
        }
    } else if (std::holds_alternative<StructValue>(obj.value)) {
        // Struct: ["HEAP_STRUCT", typeName, [[field1, value1], ...]]
        // For polymorphic types, append the dynamic type as a 4th element.
        result.push_back("HEAP_STRUCT");
        result.push_back(obj.typeName);

        const auto& sv = std::get<StructValue>(obj.value);
        nlohmann::json fields = nlohmann::json::array();
        for (const auto& fieldName : sv.fieldOrder) {
            auto it = sv.fields.find(fieldName);
            if (it != sv.fields.end() && it->second) {
                nlohmann::json fieldPair = nlohmann::json::array();
                fieldPair.push_back(fieldName);
                fieldPair.push_back(valueToJson(it->second->value, it->second->type));
                fields.push_back(fieldPair);
            }
        }
        result.push_back(fields);
        if (!sv.dynamicType.empty()) {
            result.push_back(sv.dynamicType);
        }
    } else {
        // Primitive: ["HEAP_PRIMITIVE", typeName, value]
        result.push_back("HEAP_PRIMITIVE");
        result.push_back(obj.typeName);
        result.push_back(valueToJson(obj.value, nullptr));
    }

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
    case EventKind::Throw:
        result["event"] = "exception";
        break;
    case EventKind::Catch:
        result["event"] = "catch";
        break;
    case EventKind::StepLine:
    default:
        result["event"] = "step_line";
        break;
    }

    result["func_name"] = step.funcName;

    // Emit lifecycle field for Rule-of-5 tracking (only if not None)
    if (step.lifecycle != LifecycleKind::None) {
        const char* lifecycleStr = lifecycleKindToString(step.lifecycle);
        if (lifecycleStr) {
            result["lifecycle"] = lifecycleStr;
        }
    }

    // Stack to render
    nlohmann::json stackToRender = nlohmann::json::array();
    for (const auto& frame : step.stack) {
        stackToRender.push_back(frameToJson(frame));
    }
    result["stack_to_render"] = stackToRender;

    // Globals
    nlohmann::json globalsJson = nlohmann::json::object();
    for (const auto& [name, vs] : step.globals) {
        globalsJson[name] = valueToJson(vs.value, vs.type);
    }
    result["globals"] = globalsJson;

    nlohmann::json orderedGlobals = nlohmann::json::array();
    for (const auto& name : step.orderedGlobals) {
        orderedGlobals.push_back(name);
    }
    result["ordered_globals"] = orderedGlobals;

    // Heap state
    nlohmann::json heapJson = nlohmann::json::object();
    for (const auto& [heapId, obj] : step.heap) {
        heapJson[std::to_string(heapId)] = heapObjectToJson(obj);
    }
    result["heap"] = heapJson;

    // Stdout
    result["stdout"] = step.stdout_capture;

    // Stdin (input read since last step)
    if (!step.stdin_input.empty()) {
        result["stdin_input"] = step.stdin_input;
    }

    // Return value (for return events)
    if (step.return_value.has_value()) {
        result["return_value"] = valueToJson(step.return_value.value(), nullptr);
    }

    return result;
}

nlohmann::json JsonEmitter::frameToJson(const Frame& frame) {
    nlohmann::json result;

    result["frame_id"] = frame.frameId;
    result["func_name"] = frame.funcName;
    result["is_highlighted"] = frame.isHighlighted;
    result["is_zombie"] = frame.isZombie;

    // Emit is_ghost_dtor only when true (for temporary destruction tracking)
    if (frame.isGhostDtor) {
        result["is_ghost_dtor"] = true;
    }

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
            } else if constexpr (std::is_same_v<T, StructValue>) {
                // Struct: ["C_STRUCT", typeName, {field1: value1, ...}]
                // For polymorphic objects whose dynamic type differs from
                // the static type, append the dynamic type as a 4th element:
                //   ["C_STRUCT", "Shape", {fields}, "Circle"]
                nlohmann::json result = nlohmann::json::array();
                result.push_back("C_STRUCT");
                result.push_back(arg.typeName);

                nlohmann::json fields = nlohmann::json::object();
                for (const auto& fieldName : arg.fieldOrder) {
                    auto it = arg.fields.find(fieldName);
                    if (it != arg.fields.end() && it->second) {
                        fields[fieldName] =
                            valueToJson(it->second->value, it->second->type);
                    }
                }
                result.push_back(fields);
                if (!arg.dynamicType.empty()) {
                    result.push_back(arg.dynamicType);
                }
                return result;
            } else if constexpr (std::is_same_v<T, ArrayValue>) {
                // Array: ["C_ARRAY", elementType, [elem1, elem2, ...]]
                nlohmann::json result = nlohmann::json::array();
                result.push_back("C_ARRAY");
                result.push_back(arg.elementTypeName);

                nlohmann::json elements = nlohmann::json::array();
                for (const auto& elem : arg.elements) {
                    if (elem) {
                        elements.push_back(
                            valueToJson(elem->value, elem->type));
                    }
                }
                result.push_back(elements);
                return result;
            } else if constexpr (std::is_same_v<T, EnumValue_>) {
                // Enum: show name if available, otherwise value
                if (arg.hasName) {
                    return arg.enumName;
                } else {
                    return arg.value;
                }
            } else if constexpr (std::is_same_v<T, UnionValue>) {
                // Union: ["C_UNION", typeName, {firstField: value}]
                nlohmann::json result = nlohmann::json::array();
                result.push_back("C_UNION");
                result.push_back(arg.typeName);

                nlohmann::json content = nlohmann::json::object();
                if (!arg.firstFieldName.empty() && arg.firstFieldValue) {
                    content[arg.firstFieldName] = valueToJson(
                        arg.firstFieldValue->value, arg.firstFieldValue->type);
                }
                // Also include raw bytes as hex
                std::ostringstream hexBytes;
                for (uint8_t b : arg.rawBytes) {
                    hexBytes << std::hex << std::setfill('0') << std::setw(2)
                             << static_cast<int>(b);
                }
                content["__raw"] = hexBytes.str();
                result.push_back(content);
                return result;
            } else if constexpr (std::is_same_v<T, HeapRef>) {
                // Heap reference
                nlohmann::json result = nlohmann::json::array();
                if (arg.isDangling) {
                    result.push_back("DANGLING");
                    result.push_back(arg.heapId);
                } else if (arg.offset > 0) {
                    result.push_back("REF_OFFSET");
                    result.push_back(arg.heapId);
                    result.push_back(static_cast<int>(arg.offset));
                } else {
                    result.push_back("REF");
                    result.push_back(arg.heapId);
                }
                return result;
            } else {
                return nullptr;
            }
        },
        value);
}

nlohmann::json JsonEmitter::typeDescriptorToJson(const TypeDescriptor* type) {
    if (!type) return nullptr;

    nlohmann::json result;
    result["sizeof"] = static_cast<int>(type->size);
    result["kind"] = typeKindToString(type->kind);

    if (type->spelling) {
        result["name"] = type->spelling;
    }

    // For struct/class types, include field layout
    if ((type->kind == TypeKind::Struct || type->kind == TypeKind::Union) &&
        type->fields && type->field_count > 0) {
        nlohmann::json fields = nlohmann::json::array();
        for (size_t i = 0; i < type->field_count; ++i) {
            const FieldInfo& field = type->fields[i];
            nlohmann::json fieldInfo;
            fieldInfo["name"] = field.name ? field.name : "<unnamed>";
            fieldInfo["offset"] = static_cast<int>(field.offset);
            if (field.type) {
                fieldInfo["sizeof"] = static_cast<int>(field.type->size);
                if (field.type->spelling) {
                    fieldInfo["type"] = field.type->spelling;
                }
            }
            if (field.is_bitfield) {
                fieldInfo["bitfield"] = true;
                fieldInfo["bit_offset"] = static_cast<int>(field.bit_offset);
                fieldInfo["bit_width"] = static_cast<int>(field.bit_width);
            }
            fields.push_back(fieldInfo);
        }
        result["fields"] = fields;
    }

    // For array types, include element info
    if (type->kind == TypeKind::Array && type->element_type) {
        result["element_type"] = type->element_type->spelling ? type->element_type->spelling : "unknown";
        result["element_count"] = static_cast<int>(type->element_count);
        result["element_sizeof"] = static_cast<int>(type->element_type->size);
    }

    // For enum types, include values
    if (type->kind == TypeKind::Enum && type->enum_values && type->enum_value_count > 0) {
        nlohmann::json values = nlohmann::json::object();
        for (size_t i = 0; i < type->enum_value_count; ++i) {
            const EnumValue& ev = type->enum_values[i];
            if (ev.name) {
                values[ev.name] = ev.value;
            }
        }
        result["enum_values"] = values;
        result["is_scoped"] = type->is_scoped_enum;
    }

    // Inheritance info
    if (type->bases && type->base_count > 0) {
        nlohmann::json bases = nlohmann::json::array();
        for (size_t i = 0; i < type->base_count; ++i) {
            const BaseInfo& base = type->bases[i];
            nlohmann::json baseInfo;
            if (base.type && base.type->spelling) {
                baseInfo["type"] = base.type->spelling;
            }
            baseInfo["offset"] = static_cast<int>(base.offset);
            baseInfo["virtual"] = base.is_virtual;
            bases.push_back(baseInfo);
        }
        result["bases"] = bases;
    }

    // Flags
    if (type->is_polymorphic) {
        result["polymorphic"] = true;
    }
    if (type->is_union) {
        result["union"] = true;
    }

    return result;
}

size_t JsonEmitter::estimateOutputSize(const TraceState& state) {
    // Quick estimation based on step count and average step size
    // Average step is roughly 500-2000 bytes depending on variable count
    const size_t avgStepSize = 1000;
    return state.getSteps().size() * avgStepSize + state.getSourceCode().size();
}

bool JsonEmitter::emit(const TraceState& state, size_t maxOutputSize) {
    nlohmann::json output = toOPT(state);
    std::string jsonStr = output.dump();

    // Check size limit
    if (maxOutputSize > 0 && jsonStr.size() > maxOutputSize) {
        // Output is too large - emit truncated version
        // We need to reduce the number of trace steps until we fit
        nlohmann::json truncated;
        truncated["code"] = state.getSourceCode();
        truncated["truncated"] = true;
        truncated["truncation_reason"] = "output_size_exceeded";
        truncated["original_event_count"] = state.getSteps().size();

        // Try to include as many steps as possible
        nlohmann::json trace = nlohmann::json::array();
        const auto& steps = state.getSteps();

        // Start with roughly half the steps and adjust
        size_t targetSteps = steps.size();
        while (targetSteps > 0) {
            trace.clear();
            for (size_t i = 0; i < targetSteps && i < steps.size(); ++i) {
                trace.push_back(stepToJson(steps[i]));
            }
            truncated["trace"] = trace;
            truncated["included_event_count"] = targetSteps;

            jsonStr = truncated.dump();
            if (jsonStr.size() <= maxOutputSize) {
                break;
            }
            // Reduce by 25% each iteration
            targetSteps = targetSteps * 3 / 4;
            if (targetSteps == 0) {
                // Can't fit even minimal output - emit error only
                truncated["trace"] = nlohmann::json::array();
                truncated["included_event_count"] = 0;
                truncated["error"] = "output_too_large_to_fit";
                jsonStr = truncated.dump();
                break;
            }
        }

        std::fprintf(stderr, "%s\n", jsonStr.c_str());
        return true;  // Was truncated
    }

    std::fprintf(stderr, "%s\n", jsonStr.c_str());
    return false;  // Not truncated
}

} // namespace inspector

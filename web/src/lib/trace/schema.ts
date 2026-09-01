/**
 * Zod schemas for validating OPT-format trace data from the backend.
 * Based on the C++ Runtime Inspector trace format specification.
 */
import { z } from 'zod';

// Memory regions where pointers can point
export const MemoryRegionSchema = z.enum(['stack', 'heap', 'global', 'unknown', 'null']);
export type MemoryRegion = z.infer<typeof MemoryRegionSchema>;

// Event types for trace steps
export const TraceEventSchema = z.enum(['call', 'return', 'step_line', 'exception', 'catch']);
export type TraceEvent = z.infer<typeof TraceEventSchema>;

// Lifecycle events (Rule of 5)
export const LifecycleKindSchema = z.enum([
	'DefaultCtor',
	'CopyCtor',
	'MoveCtor',
	'CopyAssign',
	'MoveAssign',
	'Destructor'
]);
export type LifecycleKind = z.infer<typeof LifecycleKindSchema>;

// Tagged value types (discriminated by first array element)

// C_ADDRESS: Pointer with address, type, and region
export const CAddressSchema = z.tuple([
	z.literal('C_ADDRESS'),
	z.string(), // hex address like "0x7ffd1234"
	z.string(), // type like "int*"
	MemoryRegionSchema
]);
export type CAddress = z.infer<typeof CAddressSchema>;

// REF: Reference to heap object
export const CRefSchema = z.tuple([z.literal('REF'), z.number()]);
export type CRef = z.infer<typeof CRefSchema>;

// REF_OFFSET: Reference to heap object with byte offset
export const CRefOffsetSchema = z.tuple([z.literal('REF_OFFSET'), z.number(), z.number()]);
export type CRefOffset = z.infer<typeof CRefOffsetSchema>;

// DANGLING: Dangling pointer (freed heap)
export const CDanglingSchema = z.tuple([z.literal('DANGLING'), z.number()]);
export type CDangling = z.infer<typeof CDanglingSchema>;

// Forward declare EncodedValue type for recursive types
export type EncodedValue =
	| number
	| boolean
	| string
	| null
	| CAddress
	| CRef
	| CRefOffset
	| CDangling
	| CStruct
	| CArray
	| CUnion;

// C_STRUCT: Struct/class instance
// ["C_STRUCT", "TypeName", {field: value, ...}, "DynamicType"?]
export type CStruct = ['C_STRUCT', string, Record<string, EncodedValue>, string?];

// C_ARRAY: Fixed-size array
// ["C_ARRAY", "elementType", [elem1, elem2, ...]]
export type CArray = ['C_ARRAY', string, EncodedValue[]];

// C_UNION: Union type
// ["C_UNION", "TypeName", {activeField: val, "__raw": "hexbytes"}]
export type CUnion = ['C_UNION', string, Record<string, EncodedValue>];

// Primitive values
const PrimitiveValueSchema = z.union([z.number(), z.boolean(), z.string(), z.null()]);

// C_STRUCT schema - handle both 3 and 4 element variants
const CStructSchema = z.union([
	z.tuple([
		z.literal('C_STRUCT'),
		z.string(),
		z.record(z.string(), z.lazy(() => EncodedValueSchema))
	]),
	z.tuple([
		z.literal('C_STRUCT'),
		z.string(),
		z.record(z.string(), z.lazy(() => EncodedValueSchema)),
		z.string()
	])
]);

// C_ARRAY schema
const CArraySchema = z.tuple([
	z.literal('C_ARRAY'),
	z.string(),
	z.array(z.lazy(() => EncodedValueSchema))
]);

// C_UNION schema
const CUnionSchema = z.tuple([
	z.literal('C_UNION'),
	z.string(),
	z.record(z.string(), z.lazy(() => EncodedValueSchema))
]);

// All possible encoded values (Zod schema)
export const EncodedValueSchema: z.ZodType<EncodedValue> = z.lazy(() =>
	z.union([
		PrimitiveValueSchema,
		CAddressSchema,
		CRefSchema,
		CRefOffsetSchema,
		CDanglingSchema,
		CStructSchema,
		CArraySchema,
		CUnionSchema
	])
);

// Heap object types - handle both 3 and 4 element variants for HEAP_STRUCT
export const HeapPrimitiveSchema = z.tuple([
	z.literal('HEAP_PRIMITIVE'),
	z.string(),
	EncodedValueSchema
]);
export type HeapPrimitive = z.infer<typeof HeapPrimitiveSchema>;

export const HeapArraySchema = z.tuple([
	z.literal('HEAP_ARRAY'),
	z.string(),
	z.array(EncodedValueSchema)
]);
export type HeapArray = z.infer<typeof HeapArraySchema>;

// HEAP_STRUCT can have 3 or 4 elements (with optional dynamic type)
export const HeapStructSchema = z.union([
	z.tuple([
		z.literal('HEAP_STRUCT'),
		z.string(),
		z.array(z.tuple([z.string(), EncodedValueSchema]))
	]),
	z.tuple([
		z.literal('HEAP_STRUCT'),
		z.string(),
		z.array(z.tuple([z.string(), EncodedValueSchema])),
		z.string()
	])
]);
export type HeapStruct = z.infer<typeof HeapStructSchema>;

export const HeapObjectSchema = z.union([HeapPrimitiveSchema, HeapArraySchema, HeapStructSchema]);
export type HeapObject = z.infer<typeof HeapObjectSchema>;

// Stack frame
export const StackFrameSchema = z.object({
	frame_id: z.number(),
	func_name: z.string(),
	encoded_locals: z.record(z.string(), EncodedValueSchema),
	local_sizes: z.record(z.string(), z.number()).optional(),
	// Declared type spelling and storage address per local, straight from the
	// runtime's type descriptors. Optional for older backends.
	local_types: z.record(z.string(), z.string()).optional(),
	local_addresses: z.record(z.string(), z.string()).optional(),
	ordered_varnames: z.array(z.string()),
	is_highlighted: z.boolean(),
	is_zombie: z.boolean(),
	is_ghost_dtor: z.boolean().optional(),
	stack_size_bytes: z.number().optional()
});
export type StackFrame = z.infer<typeof StackFrameSchema>;

// Trace step
export const TraceStepSchema = z.object({
	line: z.number(),
	event: TraceEventSchema,
	func_name: z.string(),
	lifecycle: LifecycleKindSchema.optional(),
	stack_to_render: z.array(StackFrameSchema),
	globals: z.record(z.string(), EncodedValueSchema),
	global_sizes: z.record(z.string(), z.number()).optional(),
	global_types: z.record(z.string(), z.string()).optional(),
	ordered_globals: z.array(z.string()),
	heap: z.record(z.string(), HeapObjectSchema),
	heap_sizes: z.record(z.string(), z.number()).optional(),
	heap_addresses: z.record(z.string(), z.string()).optional(),
	stdout: z.string(),
	stdin_input: z.string().optional(),
	return_value: EncodedValueSchema.optional(),
	exception_msg: z.string().optional(),
	stack_total_bytes: z.number().optional(),
	heap_total_bytes: z.number().optional()
});
export type TraceStep = z.infer<typeof TraceStepSchema>;

// Memory leak info - backend sends as ["leaked", heapId, typeName]
export const MemoryLeakSchema = z.tuple([z.literal('leaked'), z.number(), z.string()]);
export type MemoryLeak = z.infer<typeof MemoryLeakSchema>;

// Helper to extract leak info
export function getLeakHeapId(leak: MemoryLeak): number {
	return leak[1];
}

export function getLeakTypeName(leak: MemoryLeak): string {
	return leak[2];
}

// Full trace output
export const TraceOutputSchema = z.object({
	code: z.string(),
	trace: z.array(TraceStepSchema),
	memory_leaks: z.array(MemoryLeakSchema).optional(),
	type_metadata: z.record(z.string(), z.unknown()).optional(),
	truncated: z.boolean().optional(),
	truncation_reason: z.string().nullable().optional(),
	// The plugin-rewritten C++ source. Added by the API layer, not the runtime,
	// so it is optional for compatibility with older backends.
	instrumented_source: z.string().optional()
});
export type TraceOutput = z.infer<typeof TraceOutputSchema>;

// API error response from backend
// Backend sends: { error: "phase", message: "error message", details?: "compiler output" }
export const TraceErrorSchema = z.object({
	error: z.string(),           // Phase: "instrumentation", "compilation", "execution", etc.
	message: z.string(),         // Human-readable error message
	details: z.string().optional(),  // Compiler stderr or additional details
	instrumented_source: z.string().optional()  // Rewritten source, if the plugin got that far
});
export type TraceError = z.infer<typeof TraceErrorSchema>;

// Helper type guards
export function isCAddress(value: EncodedValue): value is CAddress {
	return Array.isArray(value) && value[0] === 'C_ADDRESS';
}

export function isCRef(value: EncodedValue): value is CRef {
	return Array.isArray(value) && value[0] === 'REF';
}

export function isCRefOffset(value: EncodedValue): value is CRefOffset {
	return Array.isArray(value) && value[0] === 'REF_OFFSET';
}

export function isCDangling(value: EncodedValue): value is CDangling {
	return Array.isArray(value) && value[0] === 'DANGLING';
}

export function isCStruct(value: EncodedValue): value is CStruct {
	return Array.isArray(value) && value[0] === 'C_STRUCT';
}

export function isCArray(value: EncodedValue): value is CArray {
	return Array.isArray(value) && value[0] === 'C_ARRAY';
}

export function isCUnion(value: EncodedValue): value is CUnion {
	return Array.isArray(value) && value[0] === 'C_UNION';
}

export function isHeapRef(value: EncodedValue): value is CRef | CRefOffset | CDangling {
	return isCRef(value) || isCRefOffset(value) || isCDangling(value);
}

export function isPointerLike(value: EncodedValue): value is CAddress | CRef | CRefOffset | CDangling {
	return isCAddress(value) || isHeapRef(value);
}

export function getHeapId(value: CRef | CRefOffset | CDangling): number {
	return value[1];
}

export function getAddressHex(value: CAddress): string {
	return value[1];
}

export function getPointerType(value: CAddress): string {
	return value[2];
}

export function getMemoryRegion(value: CAddress): MemoryRegion {
	return value[3];
}

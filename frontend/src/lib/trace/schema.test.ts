import { describe, it, expect } from 'vitest';
import {
	TraceOutputSchema,
	TraceStepSchema,
	StackFrameSchema,
	EncodedValueSchema,
	isCAddress,
	isCRef,
	isCRefOffset,
	isCDangling,
	isCStruct,
	isCArray,
	getHeapId,
	getAddressHex,
	getPointerType,
	getMemoryRegion
} from './schema';
import type { EncodedValue } from './schema';

describe('trace/schema', () => {
	describe('EncodedValue validation', () => {
		it('validates primitive values', () => {
			expect(EncodedValueSchema.safeParse(42).success).toBe(true);
			expect(EncodedValueSchema.safeParse(3.14).success).toBe(true);
			expect(EncodedValueSchema.safeParse(true).success).toBe(true);
			expect(EncodedValueSchema.safeParse('hello').success).toBe(true);
			expect(EncodedValueSchema.safeParse(null).success).toBe(true);
		});

		it('validates C_ADDRESS', () => {
			const addr = ['C_ADDRESS', '0x7ffd1234', 'int*', 'stack'];
			expect(EncodedValueSchema.safeParse(addr).success).toBe(true);
		});

		it('validates REF', () => {
			const ref = ['REF', 1];
			expect(EncodedValueSchema.safeParse(ref).success).toBe(true);
		});

		it('validates REF_OFFSET', () => {
			const refOffset = ['REF_OFFSET', 1, 8];
			expect(EncodedValueSchema.safeParse(refOffset).success).toBe(true);
		});

		it('validates DANGLING', () => {
			const dangling = ['DANGLING', 1];
			expect(EncodedValueSchema.safeParse(dangling).success).toBe(true);
		});

		it('validates C_STRUCT without dynamic type', () => {
			const struct = ['C_STRUCT', 'Point', { x: 10, y: 20 }];
			expect(EncodedValueSchema.safeParse(struct).success).toBe(true);
		});

		it('validates C_STRUCT with dynamic type', () => {
			const struct = ['C_STRUCT', 'Point', { x: 10, y: 20 }, 'DerivedPoint'];
			expect(EncodedValueSchema.safeParse(struct).success).toBe(true);
		});

		it('validates C_ARRAY', () => {
			const arr = ['C_ARRAY', 'int', [1, 2, 3]];
			expect(EncodedValueSchema.safeParse(arr).success).toBe(true);
		});
	});

	describe('type guards', () => {
		it('isCAddress', () => {
			expect(isCAddress(['C_ADDRESS', '0x123', 'int*', 'stack'])).toBe(true);
			expect(isCAddress(['REF', 1])).toBe(false);
			expect(isCAddress(42)).toBe(false);
		});

		it('isCRef', () => {
			expect(isCRef(['REF', 1])).toBe(true);
			expect(isCRef(['REF_OFFSET', 1, 8])).toBe(false);
			expect(isCRef(['C_ADDRESS', '0x123', 'int*', 'stack'])).toBe(false);
		});

		it('isCRefOffset', () => {
			expect(isCRefOffset(['REF_OFFSET', 1, 8])).toBe(true);
			expect(isCRefOffset(['REF', 1])).toBe(false);
		});

		it('isCDangling', () => {
			expect(isCDangling(['DANGLING', 1])).toBe(true);
			expect(isCDangling(['REF', 1])).toBe(false);
		});

		it('isCStruct', () => {
			expect(isCStruct(['C_STRUCT', 'Point', { x: 1 }])).toBe(true);
			expect(isCStruct(['C_ARRAY', 'int', [1]])).toBe(false);
		});

		it('isCArray', () => {
			expect(isCArray(['C_ARRAY', 'int', [1, 2, 3]])).toBe(true);
			expect(isCArray(['C_STRUCT', 'Point', {}])).toBe(false);
		});
	});

	describe('helper functions', () => {
		it('getHeapId', () => {
			expect(getHeapId(['REF', 42])).toBe(42);
			expect(getHeapId(['REF_OFFSET', 10, 8])).toBe(10);
			expect(getHeapId(['DANGLING', 5])).toBe(5);
		});

		it('getAddressHex', () => {
			expect(getAddressHex(['C_ADDRESS', '0x7ffd1234', 'int*', 'stack'])).toBe('0x7ffd1234');
		});

		it('getPointerType', () => {
			expect(getPointerType(['C_ADDRESS', '0x123', 'int*', 'heap'])).toBe('int*');
		});

		it('getMemoryRegion', () => {
			expect(getMemoryRegion(['C_ADDRESS', '0x123', 'int*', 'heap'])).toBe('heap');
			expect(getMemoryRegion(['C_ADDRESS', '0x123', 'int*', 'stack'])).toBe('stack');
		});
	});

	describe('StackFrame validation', () => {
		it('validates a complete stack frame', () => {
			const frame = {
				frame_id: 0,
				func_name: 'main',
				encoded_locals: { x: 42, ptr: ['REF', 1] },
				local_sizes: { x: 4, ptr: 8 },
				ordered_varnames: ['x', 'ptr'],
				is_highlighted: true,
				is_zombie: false,
				stack_size_bytes: 12
			};

			const result = StackFrameSchema.safeParse(frame);
			expect(result.success).toBe(true);
		});
	});

	describe('TraceStep validation', () => {
		it('validates a call event', () => {
			const step = {
				line: 10,
				event: 'call',
				func_name: 'main',
				stack_to_render: [],
				globals: {},
				ordered_globals: [],
				heap: {},
				stdout: ''
			};

			const result = TraceStepSchema.safeParse(step);
			expect(result.success).toBe(true);
		});

		it('validates a step with heap objects', () => {
			const step = {
				line: 15,
				event: 'step_line',
				func_name: 'main',
				stack_to_render: [
					{
						frame_id: 0,
						func_name: 'main',
						encoded_locals: { p: ['REF', 1] },
						ordered_varnames: ['p'],
						is_highlighted: true,
						is_zombie: false
					}
				],
				globals: {},
				ordered_globals: [],
				heap: {
					'1': ['HEAP_PRIMITIVE', 'int', 42]
				},
				stdout: ''
			};

			const result = TraceStepSchema.safeParse(step);
			expect(result.success).toBe(true);
		});
	});

	describe('TraceOutput validation', () => {
		it('validates a complete trace', () => {
			const trace = {
				code: 'int main() { return 0; }',
				trace: [
					{
						line: 1,
						event: 'call',
						func_name: 'main',
						stack_to_render: [],
						globals: {},
						ordered_globals: [],
						heap: {},
						stdout: ''
					},
					{
						line: 1,
						event: 'return',
						func_name: 'main',
						stack_to_render: [],
						globals: {},
						ordered_globals: [],
						heap: {},
						stdout: '',
						return_value: 0
					}
				]
			};

			const result = TraceOutputSchema.safeParse(trace);
			expect(result.success).toBe(true);
		});

		it('validates trace with memory leaks', () => {
			const trace = {
				code: 'int main() { int* p = new int(5); return 0; }',
				trace: [],
				memory_leaks: [['leaked', 1, 'int']]
			};

			const result = TraceOutputSchema.safeParse(trace);
			expect(result.success).toBe(true);
		});
	});
});

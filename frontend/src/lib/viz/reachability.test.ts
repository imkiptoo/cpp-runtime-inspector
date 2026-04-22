import { describe, it, expect } from 'vitest';
import { findOrphans, isOrphan, getReachableFrom } from './reachability';
import type { TraceStep, HeapObject } from '$lib/trace/schema';

describe('viz/reachability', () => {
	describe('findOrphans', () => {
		it('returns empty set when no heap objects', () => {
			const step: TraceStep = {
				line: 1,
				event: 'step_line',
				func_name: 'main',
				stack_to_render: [],
				globals: {},
				ordered_globals: [],
				heap: {},
				stdout: ''
			};

			expect(findOrphans(step).size).toBe(0);
		});

		it('finds orphan when heap object has no references', () => {
			const step: TraceStep = {
				line: 1,
				event: 'step_line',
				func_name: 'main',
				stack_to_render: [
					{
						frame_id: 0,
						func_name: 'main',
						encoded_locals: {},
						ordered_varnames: [],
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

			const orphans = findOrphans(step);
			expect(orphans.size).toBe(1);
			expect(orphans.has(1)).toBe(true);
		});

		it('does not mark reachable objects as orphans', () => {
			const step: TraceStep = {
				line: 1,
				event: 'step_line',
				func_name: 'main',
				stack_to_render: [
					{
						frame_id: 0,
						func_name: 'main',
						encoded_locals: {
							ptr: ['REF', 1]
						},
						ordered_varnames: ['ptr'],
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

			const orphans = findOrphans(step);
			expect(orphans.size).toBe(0);
		});

		it('finds transitive reachability through heap chain', () => {
			const heap: Record<string, HeapObject> = {
				'1': ['HEAP_STRUCT', 'Node', [['value', 1], ['next', ['REF', 2]]]],
				'2': ['HEAP_STRUCT', 'Node', [['value', 2], ['next', ['REF', 3]]]],
				'3': ['HEAP_STRUCT', 'Node', [['value', 3], ['next', null]]]
			};

			const step: TraceStep = {
				line: 1,
				event: 'step_line',
				func_name: 'main',
				stack_to_render: [
					{
						frame_id: 0,
						func_name: 'main',
						encoded_locals: {
							head: ['REF', 1]
						},
						ordered_varnames: ['head'],
						is_highlighted: true,
						is_zombie: false
					}
				],
				globals: {},
				ordered_globals: [],
				heap,
				stdout: ''
			};

			const orphans = findOrphans(step);
			expect(orphans.size).toBe(0);
		});

		it('finds orphans in a chain when root reference is lost', () => {
			const heap: Record<string, HeapObject> = {
				'1': ['HEAP_STRUCT', 'Node', [['value', 1], ['next', ['REF', 2]]]],
				'2': ['HEAP_STRUCT', 'Node', [['value', 2], ['next', null]]]
			};

			const step: TraceStep = {
				line: 1,
				event: 'step_line',
				func_name: 'main',
				stack_to_render: [
					{
						frame_id: 0,
						func_name: 'main',
						encoded_locals: {},
						ordered_varnames: [],
						is_highlighted: true,
						is_zombie: false
					}
				],
				globals: {},
				ordered_globals: [],
				heap,
				stdout: ''
			};

			const orphans = findOrphans(step);
			expect(orphans.size).toBe(2);
			expect(orphans.has(1)).toBe(true);
			expect(orphans.has(2)).toBe(true);
		});

		it('handles globals as roots', () => {
			const step: TraceStep = {
				line: 1,
				event: 'step_line',
				func_name: 'main',
				stack_to_render: [],
				globals: {
					globalPtr: ['REF', 1]
				},
				ordered_globals: ['globalPtr'],
				heap: {
					'1': ['HEAP_PRIMITIVE', 'int', 42]
				},
				stdout: ''
			};

			const orphans = findOrphans(step);
			expect(orphans.size).toBe(0);
		});

		it('handles REF_OFFSET references', () => {
			const step: TraceStep = {
				line: 1,
				event: 'step_line',
				func_name: 'main',
				stack_to_render: [
					{
						frame_id: 0,
						func_name: 'main',
						encoded_locals: {
							ptr: ['REF_OFFSET', 1, 4]
						},
						ordered_varnames: ['ptr'],
						is_highlighted: true,
						is_zombie: false
					}
				],
				globals: {},
				ordered_globals: [],
				heap: {
					'1': ['HEAP_ARRAY', 'int', [1, 2, 3]]
				},
				stdout: ''
			};

			const orphans = findOrphans(step);
			expect(orphans.size).toBe(0);
		});

		it('does not count DANGLING as reachable', () => {
			const step: TraceStep = {
				line: 1,
				event: 'step_line',
				func_name: 'main',
				stack_to_render: [
					{
						frame_id: 0,
						func_name: 'main',
						encoded_locals: {
							ptr: ['DANGLING', 1]
						},
						ordered_varnames: ['ptr'],
						is_highlighted: true,
						is_zombie: false
					}
				],
				globals: {},
				ordered_globals: [],
				heap: {},
				stdout: ''
			};

			// No heap objects, so no orphans
			const orphans = findOrphans(step);
			expect(orphans.size).toBe(0);
		});
	});

	describe('isOrphan', () => {
		it('returns true for orphan heap ID', () => {
			const step: TraceStep = {
				line: 1,
				event: 'step_line',
				func_name: 'main',
				stack_to_render: [],
				globals: {},
				ordered_globals: [],
				heap: {
					'1': ['HEAP_PRIMITIVE', 'int', 42]
				},
				stdout: ''
			};

			expect(isOrphan(step, 1)).toBe(true);
		});

		it('returns false for reachable heap ID', () => {
			const step: TraceStep = {
				line: 1,
				event: 'step_line',
				func_name: 'main',
				stack_to_render: [
					{
						frame_id: 0,
						func_name: 'main',
						encoded_locals: {
							ptr: ['REF', 1]
						},
						ordered_varnames: ['ptr'],
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

			expect(isOrphan(step, 1)).toBe(false);
		});
	});

	describe('getReachableFrom', () => {
		it('returns empty set for primitive value', () => {
			const heap: Record<string, HeapObject> = {};
			const reachable = getReachableFrom(42, heap);
			expect(reachable.size).toBe(0);
		});

		it('returns single ID for simple REF', () => {
			const heap: Record<string, HeapObject> = {
				'1': ['HEAP_PRIMITIVE', 'int', 42]
			};
			const reachable = getReachableFrom(['REF', 1], heap);
			expect(reachable.size).toBe(1);
			expect(reachable.has(1)).toBe(true);
		});

		it('follows chain of references', () => {
			const heap: Record<string, HeapObject> = {
				'1': ['HEAP_STRUCT', 'Node', [['next', ['REF', 2]]]],
				'2': ['HEAP_STRUCT', 'Node', [['next', ['REF', 3]]]],
				'3': ['HEAP_STRUCT', 'Node', [['next', null]]]
			};

			const reachable = getReachableFrom(['REF', 1], heap);
			expect(reachable.size).toBe(3);
			expect(reachable.has(1)).toBe(true);
			expect(reachable.has(2)).toBe(true);
			expect(reachable.has(3)).toBe(true);
		});
	});
});

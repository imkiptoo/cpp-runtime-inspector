import { describe, it, expect, vi, beforeEach } from 'vitest';
import { layoutHeap, layoutHeapIncremental, type HeapLayout } from './layoutHeap';
import type { HeapObject } from '$lib/trace/schema';

// Mock ELK
vi.mock('elkjs', () => {
	return {
		default: class MockELK {
			async layout(graph: {
				id: string;
				children?: Array<{ id: string; width?: number; height?: number }>;
				edges?: Array<{ id: string; sources: string[]; targets: string[] }>;
			}) {
				// Return mock layout with positions
				const children = (graph.children ?? []).map((node, idx) => ({
					...node,
					x: idx * 200 + 20,
					y: Math.floor(idx / 3) * 100 + 20,
					width: node.width ?? 180,
					height: node.height ?? 80
				}));

				const edges = (graph.edges ?? []).map((edge) => ({
					...edge,
					sections: [
						{
							startPoint: { x: 0, y: 0 },
							endPoint: { x: 100, y: 100 }
						}
					]
				}));

				return {
					...graph,
					children,
					edges,
					width: children.length * 200 + 40,
					height: Math.ceil(children.length / 3) * 100 + 40
				};
			}
		}
	};
});

describe('viz/layoutHeap', () => {
	describe('layoutHeap', () => {
		it('returns empty layout for empty heap', async () => {
			const heap: Record<string, HeapObject> = {};
			const layout = await layoutHeap(heap);

			expect(layout.nodes.size).toBe(0);
			expect(layout.edges).toEqual([]);
			expect(layout.width).toBe(0);
			expect(layout.height).toBe(0);
		});

		it('layouts single heap primitive', async () => {
			const heap: Record<string, HeapObject> = {
				'1': ['HEAP_PRIMITIVE', 'int', 42]
			};

			const layout = await layoutHeap(heap);

			expect(layout.nodes.size).toBe(1);
			expect(layout.nodes.has('1')).toBe(true);

			const node = layout.nodes.get('1')!;
			expect(node.id).toBe('1');
			expect(typeof node.x).toBe('number');
			expect(typeof node.y).toBe('number');
			expect(node.width).toBeGreaterThan(0);
			expect(node.height).toBeGreaterThan(0);
		});

		it('layouts heap array', async () => {
			const heap: Record<string, HeapObject> = {
				'1': ['HEAP_ARRAY', 'int', [1, 2, 3]]
			};

			const layout = await layoutHeap(heap);

			expect(layout.nodes.size).toBe(1);
			expect(layout.nodes.has('1')).toBe(true);
		});

		it('layouts heap struct', async () => {
			const heap: Record<string, HeapObject> = {
				'1': ['HEAP_STRUCT', 'Point', [['x', 10], ['y', 20]]]
			};

			const layout = await layoutHeap(heap);

			expect(layout.nodes.size).toBe(1);
			expect(layout.nodes.has('1')).toBe(true);
		});

		it('layouts multiple heap objects', async () => {
			const heap: Record<string, HeapObject> = {
				'1': ['HEAP_PRIMITIVE', 'int', 42],
				'2': ['HEAP_PRIMITIVE', 'int', 100],
				'3': ['HEAP_ARRAY', 'int', [1, 2, 3]]
			};

			const layout = await layoutHeap(heap);

			expect(layout.nodes.size).toBe(3);
			expect(layout.nodes.has('1')).toBe(true);
			expect(layout.nodes.has('2')).toBe(true);
			expect(layout.nodes.has('3')).toBe(true);
		});

		it('creates edges for REF pointers', async () => {
			const heap: Record<string, HeapObject> = {
				'1': ['HEAP_STRUCT', 'Node', [['next', ['REF', 2]]]],
				'2': ['HEAP_STRUCT', 'Node', [['next', null]]]
			};

			const layout = await layoutHeap(heap);

			expect(layout.edges.length).toBe(1);
			expect(layout.edges[0].sourceId).toBe('1');
			expect(layout.edges[0].targetId).toBe('2');
		});

		it('creates edges for REF_OFFSET pointers', async () => {
			const heap: Record<string, HeapObject> = {
				'1': ['HEAP_STRUCT', 'Node', [['ptr', ['REF_OFFSET', 2, 8]]]],
				'2': ['HEAP_ARRAY', 'int', [1, 2, 3]]
			};

			const layout = await layoutHeap(heap);

			expect(layout.edges.length).toBe(1);
			expect(layout.edges[0].sourceId).toBe('1');
			expect(layout.edges[0].targetId).toBe('2');
		});

		it('uses custom node sizes when provided', async () => {
			const heap: Record<string, HeapObject> = {
				'1': ['HEAP_PRIMITIVE', 'int', 42]
			};

			const nodeSizes = new Map([['1', { width: 300, height: 150 }]]);

			const layout = await layoutHeap(heap, nodeSizes);

			const node = layout.nodes.get('1')!;
			expect(node.width).toBe(300);
			expect(node.height).toBe(150);
		});

		it('handles different density settings', async () => {
			const heap: Record<string, HeapObject> = {
				'1': ['HEAP_PRIMITIVE', 'int', 42],
				'2': ['HEAP_PRIMITIVE', 'int', 100]
			};

			const compactLayout = await layoutHeap(heap, undefined, 'compact');
			const spreadLayout = await layoutHeap(heap, undefined, 'spread');

			// Both should have the same number of nodes
			expect(compactLayout.nodes.size).toBe(2);
			expect(spreadLayout.nodes.size).toBe(2);
		});

		it('handles linked list structure', async () => {
			const heap: Record<string, HeapObject> = {
				'1': ['HEAP_STRUCT', 'Node', [['value', 1], ['next', ['REF', 2]]]],
				'2': ['HEAP_STRUCT', 'Node', [['value', 2], ['next', ['REF', 3]]]],
				'3': ['HEAP_STRUCT', 'Node', [['value', 3], ['next', null]]]
			};

			const layout = await layoutHeap(heap);

			expect(layout.nodes.size).toBe(3);
			expect(layout.edges.length).toBe(2);
		});

		it('filters edges to non-existent nodes', async () => {
			const heap: Record<string, HeapObject> = {
				'1': ['HEAP_STRUCT', 'Node', [['next', ['REF', 99]]]] // REF to non-existent node
			};

			const layout = await layoutHeap(heap);

			expect(layout.nodes.size).toBe(1);
			expect(layout.edges.length).toBe(0); // Edge filtered out
		});
	});

	describe('layoutHeapIncremental', () => {
		it('returns previous layout when no changes', async () => {
			const heap: Record<string, HeapObject> = {
				'1': ['HEAP_PRIMITIVE', 'int', 42]
			};

			const initial = await layoutHeap(heap);
			const incremental = await layoutHeapIncremental(heap, initial);

			expect(incremental).toBe(initial);
		});

		it('relayouts when nodes added', async () => {
			const heap1: Record<string, HeapObject> = {
				'1': ['HEAP_PRIMITIVE', 'int', 42]
			};

			const heap2: Record<string, HeapObject> = {
				'1': ['HEAP_PRIMITIVE', 'int', 42],
				'2': ['HEAP_PRIMITIVE', 'int', 100]
			};

			const initial = await layoutHeap(heap1);
			const incremental = await layoutHeapIncremental(heap2, initial);

			expect(incremental.nodes.size).toBe(2);
			expect(incremental).not.toBe(initial);
		});

		it('relayouts when nodes removed', async () => {
			const heap1: Record<string, HeapObject> = {
				'1': ['HEAP_PRIMITIVE', 'int', 42],
				'2': ['HEAP_PRIMITIVE', 'int', 100]
			};

			const heap2: Record<string, HeapObject> = {
				'1': ['HEAP_PRIMITIVE', 'int', 42]
			};

			const initial = await layoutHeap(heap1);
			const incremental = await layoutHeapIncremental(heap2, initial);

			expect(incremental.nodes.size).toBe(1);
			expect(incremental).not.toBe(initial);
		});
	});
});

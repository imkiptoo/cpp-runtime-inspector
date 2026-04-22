/**
 * BFS-based reachability analysis for detecting orphan heap objects.
 * An orphan is a heap object not reachable from any stack frame or global.
 */
import type { TraceStep, EncodedValue, HeapObject, StackFrame } from '$lib/trace/schema';
import { isCRef, isCRefOffset, isCDangling, isCStruct, isCArray, isCUnion } from '$lib/trace/schema';

/**
 * Extract all heap IDs referenced by an encoded value (recursively).
 */
function collectHeapRefs(value: EncodedValue, refs: Set<number>): void {
	if (isCRef(value)) {
		refs.add(value[1]);
	} else if (isCRefOffset(value)) {
		refs.add(value[1]);
	} else if (isCDangling(value)) {
		// Dangling refs don't count as reachable
		return;
	} else if (isCStruct(value)) {
		const fields = value[2];
		for (const fieldValue of Object.values(fields)) {
			collectHeapRefs(fieldValue, refs);
		}
	} else if (isCArray(value)) {
		const elements = value[2];
		for (const elem of elements) {
			collectHeapRefs(elem, refs);
		}
	} else if (isCUnion(value)) {
		const fields = value[2];
		for (const fieldValue of Object.values(fields)) {
			collectHeapRefs(fieldValue, refs);
		}
	}
}

/**
 * Extract heap refs from a heap object.
 */
function collectHeapObjectRefs(obj: HeapObject, refs: Set<number>): void {
	const tag = obj[0];
	if (tag === 'HEAP_PRIMITIVE') {
		collectHeapRefs(obj[2], refs);
	} else if (tag === 'HEAP_ARRAY') {
		for (const elem of obj[2]) {
			collectHeapRefs(elem, refs);
		}
	} else if (tag === 'HEAP_STRUCT') {
		const fields = obj[2];
		for (const [, fieldValue] of fields) {
			collectHeapRefs(fieldValue, refs);
		}
	}
}

/**
 * Collect all heap IDs directly referenced from stack frames.
 */
function collectStackRoots(frames: StackFrame[]): Set<number> {
	const roots = new Set<number>();
	for (const frame of frames) {
		for (const value of Object.values(frame.encoded_locals)) {
			collectHeapRefs(value, roots);
		}
	}
	return roots;
}

/**
 * Collect all heap IDs directly referenced from globals.
 */
function collectGlobalRoots(globals: Record<string, EncodedValue>): Set<number> {
	const roots = new Set<number>();
	for (const value of Object.values(globals)) {
		collectHeapRefs(value, roots);
	}
	return roots;
}

/**
 * Perform BFS to find all reachable heap IDs starting from roots.
 */
function bfsReachable(
	roots: Set<number>,
	heap: Record<string, HeapObject>
): Set<number> {
	const reachable = new Set<number>();
	const queue = [...roots];

	while (queue.length > 0) {
		const id = queue.shift()!;
		if (reachable.has(id)) continue;

		// Check if this heap ID exists
		const heapKey = String(id);
		if (!(heapKey in heap)) continue;

		reachable.add(id);

		// Find refs from this heap object
		const obj = heap[heapKey];
		const refs = new Set<number>();
		collectHeapObjectRefs(obj, refs);

		for (const refId of refs) {
			if (!reachable.has(refId)) {
				queue.push(refId);
			}
		}
	}

	return reachable;
}

/**
 * Find orphan heap IDs in a trace step.
 * Orphans are heap objects not reachable from any stack/global root.
 */
export function findOrphans(step: TraceStep): Set<number> {
	// Collect all roots from stack and globals
	const stackRoots = collectStackRoots(step.stack_to_render);
	const globalRoots = collectGlobalRoots(step.globals);
	const allRoots = new Set([...stackRoots, ...globalRoots]);

	// Find all reachable heap objects via BFS
	const reachable = bfsReachable(allRoots, step.heap);

	// Orphans are heap objects not in reachable set
	const orphans = new Set<number>();
	for (const heapKey of Object.keys(step.heap)) {
		const id = parseInt(heapKey, 10);
		if (!reachable.has(id)) {
			orphans.add(id);
		}
	}

	return orphans;
}

/**
 * Check if a specific heap ID is an orphan.
 */
export function isOrphan(step: TraceStep, heapId: number): boolean {
	return findOrphans(step).has(heapId);
}

/**
 * Get all heap IDs reachable from a specific value.
 */
export function getReachableFrom(value: EncodedValue, heap: Record<string, HeapObject>): Set<number> {
	const roots = new Set<number>();
	collectHeapRefs(value, roots);
	return bfsReachable(roots, heap);
}

/**
 * Shared state for heap node drag offsets.
 * Tracks user-applied position adjustments per node.
 */

export interface NodeOffset {
	x: number;
	y: number;
}

// Map of heapId -> drag offset
const offsets = new Map<string, NodeOffset>();

// Version counter for change detection
let version = 0;

export const nodePositions = {
	get version() {
		return version;
	},

	getOffset(heapId: string): NodeOffset {
		return offsets.get(heapId) ?? { x: 0, y: 0 };
	},

	setOffset(heapId: string, x: number, y: number): void {
		offsets.set(heapId, { x, y });
		version++;
	},

	clearOffset(heapId: string): void {
		offsets.delete(heapId);
		version++;
	},

	clearAll(): void {
		offsets.clear();
		version++;
	},

	hasOffset(heapId: string): boolean {
		return offsets.has(heapId);
	}
};

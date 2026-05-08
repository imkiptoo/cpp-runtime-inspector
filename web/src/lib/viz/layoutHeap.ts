/**
 * ELK-based heap graph layout for positioning heap nodes.
 * Uses elkjs to compute optimal positions for heap allocations.
 */
import ELK from 'elkjs';
import type { ElkNode, ElkExtendedEdge, LayoutOptions } from 'elkjs';
import type { HeapObject, EncodedValue } from '$lib/trace/schema';
import { isCRef, isCRefOffset } from '$lib/trace/schema';

const elk = new ELK();

export interface HeapNodeLayout {
	id: string;
	x: number;
	y: number;
	width: number;
	height: number;
}

export interface HeapEdgeLayout {
	id: string;
	sourceId: string;
	targetId: string;
	sections: Array<{
		startPoint: { x: number; y: number };
		endPoint: { x: number; y: number };
		bendPoints?: Array<{ x: number; y: number }>;
	}>;
}

export interface HeapLayout {
	nodes: Map<string, HeapNodeLayout>;
	edges: HeapEdgeLayout[];
	width: number;
	height: number;
}

export type HeapDensity = 'compact' | 'normal' | 'spread';

const DENSITY_SPACING: Record<HeapDensity, { nodeSpacing: number; layerSpacing: number }> = {
	compact: { nodeSpacing: 20, layerSpacing: 40 },
	normal: { nodeSpacing: 40, layerSpacing: 80 },
	spread: { nodeSpacing: 60, layerSpacing: 120 }
};

// Default node dimensions (can be overridden by measured sizes)
const DEFAULT_NODE_WIDTH = 180;
const DEFAULT_NODE_HEIGHT = 80;

/**
 * Extract edges (references) from heap objects.
 */
function extractEdges(
	heap: Record<string, HeapObject>
): Array<{ source: string; target: string; sourcePort?: string }> {
	const edges: Array<{ source: string; target: string; sourcePort?: string }> = [];

	for (const [heapId, obj] of Object.entries(heap)) {
		const tag = obj[0];

		if (tag === 'HEAP_PRIMITIVE') {
			const value = obj[2];
			addValueEdges(heapId, value, undefined, edges);
		} else if (tag === 'HEAP_ARRAY') {
			const elements = obj[2];
			elements.forEach((elem, idx) => {
				addValueEdges(heapId, elem, `[${idx}]`, edges);
			});
		} else if (tag === 'HEAP_STRUCT') {
			const fields = obj[2];
			for (const [fieldName, fieldValue] of fields) {
				addValueEdges(heapId, fieldValue, fieldName, edges);
			}
		}
	}

	return edges;
}

function addValueEdges(
	sourceId: string,
	value: EncodedValue,
	port: string | undefined,
	edges: Array<{ source: string; target: string; sourcePort?: string }>
): void {
	if (isCRef(value)) {
		edges.push({ source: sourceId, target: String(value[1]), sourcePort: port });
	} else if (isCRefOffset(value)) {
		edges.push({ source: sourceId, target: String(value[1]), sourcePort: port });
	}
	// Recursively check nested values
	if (Array.isArray(value) && value[0] === 'C_STRUCT') {
		const fields = value[2] as Record<string, EncodedValue>;
		for (const [fieldName, fieldValue] of Object.entries(fields)) {
			addValueEdges(sourceId, fieldValue, port ? `${port}.${fieldName}` : fieldName, edges);
		}
	} else if (Array.isArray(value) && value[0] === 'C_ARRAY') {
		const elements = value[2] as EncodedValue[];
		elements.forEach((elem, idx) => {
			addValueEdges(sourceId, elem, port ? `${port}[${idx}]` : `[${idx}]`, edges);
		});
	}
}

/**
 * Compute layout for heap objects using ELK.
 */
export async function layoutHeap(
	heap: Record<string, HeapObject>,
	nodeSizes?: Map<string, { width: number; height: number }>,
	density: HeapDensity = 'normal'
): Promise<HeapLayout> {
	const heapIds = Object.keys(heap);
	if (heapIds.length === 0) {
		return { nodes: new Map(), edges: [], width: 0, height: 0 };
	}

	const spacing = DENSITY_SPACING[density];

	// Build ELK graph
	const nodes: ElkNode[] = heapIds.map((id) => {
		const size = nodeSizes?.get(id);
		return {
			id,
			width: size?.width ?? DEFAULT_NODE_WIDTH,
			height: size?.height ?? DEFAULT_NODE_HEIGHT
		};
	});

	const edges = extractEdges(heap);
	const elkEdges: ElkExtendedEdge[] = edges
		.filter((e) => heap[e.target]) // Only edges to existing nodes
		.map((e, idx) => ({
			id: `e${idx}`,
			sources: [e.source],
			targets: [e.target]
		}));

	const layoutOptions: LayoutOptions = {
		'elk.algorithm': 'layered',
		'elk.direction': 'RIGHT',
		'elk.spacing.nodeNode': String(spacing.nodeSpacing),
		'elk.layered.spacing.nodeNodeBetweenLayers': String(spacing.layerSpacing),
		'elk.layered.nodePlacement.strategy': 'NETWORK_SIMPLEX',
		'elk.layered.crossingMinimization.strategy': 'LAYER_SWEEP',
		'elk.edgeRouting': 'ORTHOGONAL',
		'elk.padding': '[top=20,left=20,bottom=20,right=20]'
	};

	const graph: ElkNode = {
		id: 'root',
		layoutOptions,
		children: nodes,
		edges: elkEdges
	};

	try {
		const laid = await elk.layout(graph);

		const resultNodes = new Map<string, HeapNodeLayout>();
		for (const child of laid.children ?? []) {
			resultNodes.set(child.id, {
				id: child.id,
				x: child.x ?? 0,
				y: child.y ?? 0,
				width: child.width ?? DEFAULT_NODE_WIDTH,
				height: child.height ?? DEFAULT_NODE_HEIGHT
			});
		}

		const resultEdges: HeapEdgeLayout[] = (laid.edges ?? []).map((edge) => {
			const elkEdge = edge as ElkExtendedEdge;
			return {
				id: elkEdge.id,
				sourceId: elkEdge.sources[0],
				targetId: elkEdge.targets[0],
				sections: elkEdge.sections ?? []
			};
		});

		return {
			nodes: resultNodes,
			edges: resultEdges,
			width: laid.width ?? 0,
			height: laid.height ?? 0
		};
	} catch (err) {
		console.error('ELK layout failed:', err);
		// Fallback: simple grid layout
		return fallbackLayout(heapIds, nodeSizes);
	}
}

/**
 * Simple grid layout fallback if ELK fails.
 */
function fallbackLayout(
	heapIds: string[],
	nodeSizes?: Map<string, { width: number; height: number }>
): HeapLayout {
	const nodes = new Map<string, HeapNodeLayout>();
	const cols = Math.ceil(Math.sqrt(heapIds.length));
	let maxWidth = 0;
	let maxHeight = 0;

	heapIds.forEach((id, idx) => {
		const size = nodeSizes?.get(id);
		const width = size?.width ?? DEFAULT_NODE_WIDTH;
		const height = size?.height ?? DEFAULT_NODE_HEIGHT;
		const col = idx % cols;
		const row = Math.floor(idx / cols);
		const x = col * (DEFAULT_NODE_WIDTH + 40) + 20;
		const y = row * (DEFAULT_NODE_HEIGHT + 40) + 20;

		nodes.set(id, { id, x, y, width, height });
		maxWidth = Math.max(maxWidth, x + width);
		maxHeight = Math.max(maxHeight, y + height);
	});

	return {
		nodes,
		edges: [],
		width: maxWidth + 20,
		height: maxHeight + 20
	};
}

/**
 * Layout only changed nodes incrementally (preserve existing positions).
 */
export async function layoutHeapIncremental(
	heap: Record<string, HeapObject>,
	previousLayout: HeapLayout,
	nodeSizes?: Map<string, { width: number; height: number }>,
	density: HeapDensity = 'normal'
): Promise<HeapLayout> {
	const newIds = new Set(Object.keys(heap));
	const prevIds = new Set(previousLayout.nodes.keys());

	// If no new nodes and no removed nodes, keep previous layout
	const added = [...newIds].filter((id) => !prevIds.has(id));
	const removed = [...prevIds].filter((id) => !newIds.has(id));

	if (added.length === 0 && removed.length === 0) {
		return previousLayout;
	}

	// For now, do full relayout on structural changes
	// A more sophisticated approach would fix existing nodes and only layout new ones
	return layoutHeap(heap, nodeSizes, density);
}

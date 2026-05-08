/**
 * Pointer-arrow routing based on SeePlusPlus implementation.
 * Takes measured chip + target rects and returns edge endpoints and sides.
 *
 * Decisions:
 * 1. Source side (chip.left or chip.right) and target side (any of four)
 *    chosen by geometry with obstacle-aware scoring.
 * 2. Endpoint distribution when multiple edges converge on same target side.
 */

export type Side = 'left' | 'right' | 'top' | 'bottom';

export interface MeasuredRect {
	left: number;
	right: number;
	top: number;
	bottom: number;
	width: number;
	height: number;
}

export type EdgeKind = 'pointer' | 'ref';

export interface EdgeSample {
	/** Stable key for the rendered path. */
	key: string;
	kind: EdgeKind;
	/** Target heap address — used to group arrows that share an endpoint. */
	target: string;
	/** Address of the enclosing source card, or null if from stack. */
	sourceAddr: string | null;
	/** The chip rect — visible source of the arrow. */
	chip: MeasuredRect;
	/** The card rect that contains the chip (heap node). null for stack chips. */
	sourceCard: MeasuredRect | null;
	/** The target element rect (heap card, stack local, or stack frame). */
	targetEl: MeasuredRect;
}

export interface CardRect extends MeasuredRect {
	/** Stable identifier for the card. Used to skip source/target in scoring. */
	id: string;
}

export interface RoutedEdge {
	key: string;
	kind: EdgeKind;
	target: string;
	x1: number;
	y1: number;
	x2: number;
	y2: number;
	sourceSide: 'left' | 'right';
	targetSide: Side;
}

export interface RouteOptions {
	/** Per-card center positions from layout. Stabilizes side selection during FLIP. */
	layoutCenters?: ReadonlyMap<string, { x: number; y: number }>;
	/** Card rectangles arrows should avoid crossing. */
	obstacles?: ReadonlyArray<CardRect>;
}

export type RoutingMode = 'curved' | 'straight' | 'orthogonal';

interface InternalEdge extends EdgeSample {
	sourceSide: 'left' | 'right';
	targetSide: Side;
	x1: number;
	y1: number;
	x2: number;
	y2: number;
}

interface Anchor {
	x: number;
	y: number;
}

const SOURCE_SIDES: Array<'left' | 'right'> = ['left', 'right'];
const TARGET_SIDES: Side[] = ['left', 'right', 'top', 'bottom'];

function chipAnchor(chip: MeasuredRect, side: 'left' | 'right'): Anchor {
	return {
		x: side === 'left' ? chip.left : chip.right,
		y: (chip.top + chip.bottom) / 2,
	};
}

function rectAnchor(r: MeasuredRect, side: Side): Anchor {
	switch (side) {
		case 'left':
			return { x: r.left, y: (r.top + r.bottom) / 2 };
		case 'right':
			return { x: r.right, y: (r.top + r.bottom) / 2 };
		case 'top':
			return { x: (r.left + r.right) / 2, y: r.top };
		case 'bottom':
			return { x: (r.left + r.right) / 2, y: r.bottom };
	}
}

/**
 * Liang–Barsky line-clipping test. Returns true if segment intersects rect.
 */
function segmentIntersectsRect(p0: Anchor, p1: Anchor, r: MeasuredRect): boolean {
	const dx = p1.x - p0.x;
	const dy = p1.y - p0.y;
	let t0 = 0;
	let t1 = 1;
	const checks: ReadonlyArray<readonly [number, number]> = [
		[-dx, p0.x - r.left],
		[dx, r.right - p0.x],
		[-dy, p0.y - r.top],
		[dy, r.bottom - p0.y],
	];
	for (const [p, q] of checks) {
		if (p === 0) {
			if (q < 0) return false;
		} else if (p < 0) {
			const t = q / p;
			if (t > t1) return false;
			if (t > t0) t0 = t;
		} else {
			const t = q / p;
			if (t < t0) return false;
			if (t < t1) t1 = t;
		}
	}
	return t0 <= t1;
}

/** Chip side facing the target. Ties default to right. */
function naturalSourceSide(sourceCenterX: number, targetCenterX: number): 'left' | 'right' {
	if (Math.abs(targetCenterX - sourceCenterX) < 1) return 'right';
	return targetCenterX < sourceCenterX ? 'left' : 'right';
}

/** Target side on the dominant axis. */
function naturalTargetSide(
	sourceCenter: { x: number; y: number },
	targetCenter: { x: number; y: number }
): Side {
	const dx = sourceCenter.x - targetCenter.x;
	const dy = sourceCenter.y - targetCenter.y;
	if (Math.abs(dx) >= Math.abs(dy)) {
		return dx < 0 ? 'left' : 'right';
	}
	return dy < 0 ? 'top' : 'bottom';
}

/** Score a side pair: crosses * 100 + naturalDistance. Lower is better. */
function scoreSides(
	sample: EdgeSample,
	sourceSide: 'left' | 'right',
	targetSide: Side,
	obstacles: ReadonlyArray<CardRect>,
	excludeIds: ReadonlySet<string>,
	naturalSrc: 'left' | 'right',
	naturalTgt: Side
): number {
	const src = chipAnchor(sample.chip, sourceSide);
	const tgt = rectAnchor(sample.targetEl, targetSide);
	let crosses = 0;
	for (const r of obstacles) {
		if (excludeIds.has(r.id)) continue;
		if (segmentIntersectsRect(src, tgt, r)) crosses++;
	}
	let naturalDistance = 0;
	if (sourceSide !== naturalSrc) naturalDistance += 1;
	if (targetSide !== naturalTgt) naturalDistance += 1;
	return crosses * 100 + naturalDistance;
}

export function routeEdges(
	samples: ReadonlyArray<EdgeSample>,
	options: RouteOptions = {}
): RoutedEdge[] {
	const obstacles = options.obstacles ?? [];

	const initial: InternalEdge[] = samples.map((s) => {
		// Prefer layout-time centers for stable direction during FLIP
		const layoutSrc = s.sourceAddr !== null ? options.layoutCenters?.get(s.sourceAddr) : undefined;
		const layoutTgt = options.layoutCenters?.get(s.target);

		const stableSrc = layoutSrc ?? (s.sourceCard
			? {
				x: (s.sourceCard.left + s.sourceCard.right) / 2,
				y: (s.sourceCard.top + s.sourceCard.bottom) / 2,
			}
			: {
				x: (s.chip.left + s.chip.right) / 2,
				y: (s.chip.top + s.chip.bottom) / 2,
			});

		const stableTgt = layoutTgt ?? {
			x: (s.targetEl.left + s.targetEl.right) / 2,
			y: (s.targetEl.top + s.targetEl.bottom) / 2,
		};

		const naturalSrc = naturalSourceSide(stableSrc.x, stableTgt.x);
		const naturalTgt = naturalTargetSide(stableSrc, stableTgt);

		const excludeIds = new Set<string>();
		if (s.sourceAddr !== null) excludeIds.add(s.sourceAddr);
		excludeIds.add(s.target);

		// Stack chips pinned to right (heap is always to the right of stack)
		const sourceSidesToTry: Array<'left' | 'right'> = s.sourceCard
			? SOURCE_SIDES
			: ['right'];

		let bestSrc: 'left' | 'right' = naturalSrc;
		let bestTgt: Side = naturalTgt;
		let bestScore = Infinity;

		for (const src of sourceSidesToTry) {
			for (const tgt of TARGET_SIDES) {
				const score = scoreSides(s, src, tgt, obstacles, excludeIds, naturalSrc, naturalTgt);
				if (score < bestScore) {
					bestScore = score;
					bestSrc = src;
					bestTgt = tgt;
				}
			}
		}

		const srcA = chipAnchor(s.chip, bestSrc);
		const tgtA = rectAnchor(s.targetEl, bestTgt);

		return {
			...s,
			sourceSide: bestSrc,
			targetSide: bestTgt,
			x1: srcA.x,
			y1: srcA.y,
			x2: tgtA.x,
			y2: tgtA.y,
		};
	});

	// Spread arrows that converge on the same (target, side)
	const targetGroups = new Map<string, InternalEdge[]>();
	for (const e of initial) {
		const key = `${e.target}::${e.targetSide}`;
		let group = targetGroups.get(key);
		if (!group) {
			group = [];
			targetGroups.set(key, group);
		}
		group.push(e);
	}

	for (const group of targetGroups.values()) {
		if (group.length < 2) continue;
		const tgt = group[0]!.targetEl;
		const side = group[0]!.targetSide;

		if (side === 'left' || side === 'right') {
			// Sort by source y to minimize crossings
			group.sort((a, b) => a.y1 - b.y1);
			const margin = Math.min(8, tgt.height / 4);
			const yMin = tgt.top + margin;
			const yMax = tgt.bottom - margin;
			for (let i = 0; i < group.length; i++) {
				const t = group.length > 1 ? i / (group.length - 1) : 0.5;
				group[i]!.y2 = yMin + t * (yMax - yMin);
			}
		} else {
			group.sort((a, b) => a.x1 - b.x1);
			const margin = Math.min(8, tgt.width / 4);
			const xMin = tgt.left + margin;
			const xMax = tgt.right - margin;
			for (let i = 0; i < group.length; i++) {
				const t = group.length > 1 ? i / (group.length - 1) : 0.5;
				group[i]!.x2 = xMin + t * (xMax - xMin);
			}
		}
	}

	return initial.map((e) => ({
		key: e.key,
		kind: e.kind,
		target: e.target,
		x1: e.x1,
		y1: e.y1,
		x2: e.x2,
		y2: e.y2,
		sourceSide: e.sourceSide,
		targetSide: e.targetSide,
	}));
}

/**
 * Generate SVG path for an edge based on routing mode.
 */
export function edgePath(
	e: {
		x1: number;
		y1: number;
		x2: number;
		y2: number;
		sourceSide: 'left' | 'right';
		targetSide: Side;
	},
	routing: RoutingMode
): string {
	if (routing === 'straight') {
		return `M ${e.x1},${e.y1} L ${e.x2},${e.y2}`;
	}

	if (routing === 'orthogonal') {
		if (e.targetSide === 'top' || e.targetSide === 'bottom') {
			const horizOff = e.sourceSide === 'right' ? 24 : -24;
			const cornerX = e.x1 + horizOff;
			const midY = (e.y1 + e.y2) / 2;
			return `M ${e.x1},${e.y1} L ${cornerX},${e.y1} L ${cornerX},${midY} L ${e.x2},${midY} L ${e.x2},${e.y2}`;
		}
		const midX = e.x1 + (e.x2 - e.x1) / 2;
		return `M ${e.x1},${e.y1} L ${midX},${e.y1} L ${midX},${e.y2} L ${e.x2},${e.y2}`;
	}

	// Curved (cubic bezier) - default
	const dx = Math.max(24, Math.abs(e.x2 - e.x1) * 0.5);
	const dy = Math.max(24, Math.abs(e.y2 - e.y1) * 0.5);
	const c1x = e.sourceSide === 'right' ? e.x1 + dx : e.x1 - dx;
	const c1y = e.y1;
	let c2x = e.x2;
	let c2y = e.y2;

	switch (e.targetSide) {
		case 'left':
			c2x = e.x2 - dx;
			break;
		case 'right':
			c2x = e.x2 + dx;
			break;
		case 'top':
			c2y = e.y2 - dy;
			break;
		case 'bottom':
			c2y = e.y2 + dy;
			break;
	}

	return `M ${e.x1},${e.y1} C ${c1x},${c1y} ${c2x},${c2y} ${e.x2},${e.y2}`;
}

/**
 * Convert a DOMRect to MeasuredRect.
 */
export function toMeasured(r: DOMRect): MeasuredRect {
	return {
		left: r.left,
		right: r.right,
		top: r.top,
		bottom: r.bottom,
		width: r.width,
		height: r.height,
	};
}

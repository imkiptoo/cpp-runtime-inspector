import { describe, it, expect } from 'vitest';
import {
	routeEdges,
	edgePath,
	toMeasured,
	type EdgeSample,
	type MeasuredRect,
	type CardRect
} from './routeEdges';

describe('viz/routeEdges', () => {
	// Helper to create a MeasuredRect
	function makeRect(left: number, top: number, width: number, height: number): MeasuredRect {
		return {
			left,
			top,
			right: left + width,
			bottom: top + height,
			width,
			height
		};
	}

	// Helper to create an EdgeSample
	function makeSample(
		key: string,
		chip: MeasuredRect,
		targetEl: MeasuredRect,
		sourceCard: MeasuredRect | null = null,
		sourceAddr: string | null = null
	): EdgeSample {
		return {
			key,
			kind: 'pointer',
			target: 'heap-1',
			sourceAddr,
			chip,
			sourceCard,
			targetEl
		};
	}

	describe('routeEdges', () => {
		it('routes a single edge', () => {
			const chip = makeRect(50, 50, 40, 20);
			const target = makeRect(200, 50, 100, 80);
			const samples: EdgeSample[] = [makeSample('e1', chip, target)];

			const routed = routeEdges(samples);

			expect(routed).toHaveLength(1);
			expect(routed[0].key).toBe('e1');
			expect(routed[0].x1).toBeDefined();
			expect(routed[0].y1).toBeDefined();
			expect(routed[0].x2).toBeDefined();
			expect(routed[0].y2).toBeDefined();
		});

		it('chooses source side based on target position', () => {
			// Target to the right - should exit from chip.right
			const chip = makeRect(50, 50, 40, 20);
			const targetRight = makeRect(200, 50, 100, 80);
			const routedRight = routeEdges([makeSample('e1', chip, targetRight)]);
			expect(routedRight[0].sourceSide).toBe('right');

			// Target to the left - should exit from chip.left (if from heap)
			const targetLeft = makeRect(-100, 50, 80, 60);
			const sourceCard = makeRect(40, 40, 60, 60);
			const routedLeft = routeEdges([makeSample('e2', chip, targetLeft, sourceCard, 'heap-2')]);
			expect(routedLeft[0].sourceSide).toBe('left');
		});

		it('chooses target side based on source position', () => {
			const chip = makeRect(50, 50, 40, 20);

			// Source to the left of target - target side should be left
			const targetRight = makeRect(200, 50, 100, 80);
			const routed = routeEdges([makeSample('e1', chip, targetRight)]);
			expect(routed[0].targetSide).toBe('left');
		});

		it('spreads multiple arrows on the same target side', () => {
			const chip1 = makeRect(50, 30, 40, 20);
			const chip2 = makeRect(50, 80, 40, 20);
			const target = makeRect(200, 50, 100, 80);

			const samples: EdgeSample[] = [
				{ ...makeSample('e1', chip1, target), target: 'heap-1' },
				{ ...makeSample('e2', chip2, target), target: 'heap-1' }
			];

			const routed = routeEdges(samples);

			expect(routed).toHaveLength(2);
			// Y endpoints should be different (spread along target side)
			expect(routed[0].y2).not.toBe(routed[1].y2);
		});

		it('avoids obstacles when scoring sides', () => {
			const chip = makeRect(50, 100, 40, 20);
			const target = makeRect(250, 100, 100, 80);

			// Obstacle blocking direct path
			const obstacles: CardRect[] = [
				{ id: 'obs', ...makeRect(150, 80, 60, 60) }
			];

			const samples: EdgeSample[] = [makeSample('e1', chip, target)];
			const routed = routeEdges(samples, { obstacles });

			// Should still produce a valid route
			expect(routed).toHaveLength(1);
			expect(routed[0].x1).toBeDefined();
		});

		it('skips source and target rects when scoring obstacles', () => {
			const chip = makeRect(50, 50, 40, 20);
			const sourceCard = makeRect(40, 40, 60, 60);
			const target = makeRect(200, 50, 100, 80);

			// Include source and target as obstacles - they should be ignored
			const obstacles: CardRect[] = [
				{ id: 'heap-src', ...sourceCard },
				{ id: 'heap-1', ...target }
			];

			const samples: EdgeSample[] = [
				{
					...makeSample('e1', chip, target, sourceCard, 'heap-src'),
					target: 'heap-1'
				}
			];

			const routed = routeEdges(samples, { obstacles });
			expect(routed).toHaveLength(1);
		});
	});

	describe('edgePath', () => {
		const edge = {
			x1: 100,
			y1: 50,
			x2: 300,
			y2: 50,
			sourceSide: 'right' as const,
			targetSide: 'left' as const
		};

		it('generates straight path', () => {
			const path = edgePath(edge, 'straight');
			expect(path).toContain('M 100,50');
			expect(path).toContain('L 300,50');
		});

		it('generates curved (bezier) path', () => {
			const path = edgePath(edge, 'curved');
			expect(path).toContain('M 100,50');
			expect(path).toContain('C'); // Cubic bezier command
		});

		it('generates orthogonal path for horizontal layout', () => {
			const path = edgePath(edge, 'orthogonal');
			expect(path).toContain('M 100,50');
			expect(path).toContain('L'); // Multiple line segments
			// Should have midpoint at X
			expect(path).toContain('200'); // midX between 100 and 300
		});

		it('generates orthogonal path for vertical target', () => {
			const verticalEdge = {
				x1: 100,
				y1: 50,
				x2: 150,
				y2: 200,
				sourceSide: 'right' as const,
				targetSide: 'top' as const
			};
			const path = edgePath(verticalEdge, 'orthogonal');
			expect(path).toContain('M 100,50');
		});
	});

	describe('toMeasured', () => {
		it('converts DOMRect to MeasuredRect', () => {
			const domRect = new DOMRect(10, 20, 100, 50);
			const measured = toMeasured(domRect);

			expect(measured.left).toBe(10);
			expect(measured.top).toBe(20);
			expect(measured.right).toBe(110);
			expect(measured.bottom).toBe(70);
			expect(measured.width).toBe(100);
			expect(measured.height).toBe(50);
		});
	});

	describe('edge cases', () => {
		it('handles empty samples array', () => {
			const routed = routeEdges([]);
			expect(routed).toHaveLength(0);
		});

		it('handles stack chips (no source card)', () => {
			const chip = makeRect(50, 50, 40, 20);
			const target = makeRect(200, 50, 100, 80);

			// Stack chip: no sourceCard, sourceAddr is null
			const samples: EdgeSample[] = [makeSample('e1', chip, target, null, null)];

			const routed = routeEdges(samples);
			expect(routed).toHaveLength(1);
			// Stack chips should always exit from right
			expect(routed[0].sourceSide).toBe('right');
		});

		it('handles ref edges', () => {
			const chip = makeRect(50, 50, 40, 20);
			const target = makeRect(200, 50, 100, 80);

			const samples: EdgeSample[] = [{
				key: 'e1',
				kind: 'ref',
				target: 'heap-1',
				sourceAddr: null,
				chip,
				sourceCard: null,
				targetEl: target
			}];

			const routed = routeEdges(samples);
			expect(routed[0].kind).toBe('ref');
		});
	});
});

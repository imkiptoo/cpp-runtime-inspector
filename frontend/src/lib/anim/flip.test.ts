import { describe, it, expect, vi, beforeEach } from 'vitest';
import { calculateTransitions, type FlipState, type FlipRect } from './flip';

describe('anim/flip', () => {
	describe('calculateTransitions', () => {
		const rect1: FlipRect = { x: 0, y: 0, width: 100, height: 50 };
		const rect2: FlipRect = { x: 100, y: 0, width: 100, height: 50 };

		it('detects moved elements', () => {
			const first = new Map<string, FlipState>([
				['a', { id: 'a', rect: rect1, opacity: 1 }]
			]);
			const last = new Map<string, FlipState>([
				['a', { id: 'a', rect: rect2, opacity: 1 }]
			]);

			const { moved, entered, exited } = calculateTransitions(first, last);

			expect(moved.length).toBe(1);
			expect(moved[0].id).toBe('a');
			expect(moved[0].deltaX).toBe(-100);
			expect(moved[0].deltaY).toBe(0);
			expect(entered.length).toBe(0);
			expect(exited.length).toBe(0);
		});

		it('detects entered elements', () => {
			const first = new Map<string, FlipState>();
			const last = new Map<string, FlipState>([
				['a', { id: 'a', rect: rect1, opacity: 1 }]
			]);

			const { moved, entered, exited } = calculateTransitions(first, last);

			expect(moved.length).toBe(0);
			expect(entered.length).toBe(1);
			expect(entered[0]).toBe('a');
			expect(exited.length).toBe(0);
		});

		it('detects exited elements', () => {
			const first = new Map<string, FlipState>([
				['a', { id: 'a', rect: rect1, opacity: 1 }]
			]);
			const last = new Map<string, FlipState>();

			const { moved, entered, exited } = calculateTransitions(first, last);

			expect(moved.length).toBe(0);
			expect(entered.length).toBe(0);
			expect(exited.length).toBe(1);
			expect(exited[0]).toBe('a');
		});

		it('handles all three cases simultaneously', () => {
			const first = new Map<string, FlipState>([
				['a', { id: 'a', rect: rect1, opacity: 1 }],
				['b', { id: 'b', rect: rect1, opacity: 1 }]
			]);
			const last = new Map<string, FlipState>([
				['a', { id: 'a', rect: rect2, opacity: 1 }],
				['c', { id: 'c', rect: rect1, opacity: 1 }]
			]);

			const { moved, entered, exited } = calculateTransitions(first, last);

			expect(moved.length).toBe(1);
			expect(moved[0].id).toBe('a');
			expect(entered.length).toBe(1);
			expect(entered[0]).toBe('c');
			expect(exited.length).toBe(1);
			expect(exited[0]).toBe('b');
		});

		it('calculates correct scale factors', () => {
			const smallRect: FlipRect = { x: 0, y: 0, width: 50, height: 25 };
			const largeRect: FlipRect = { x: 0, y: 0, width: 100, height: 50 };

			const first = new Map<string, FlipState>([
				['a', { id: 'a', rect: smallRect, opacity: 1 }]
			]);
			const last = new Map<string, FlipState>([
				['a', { id: 'a', rect: largeRect, opacity: 1 }]
			]);

			const { moved } = calculateTransitions(first, last);

			expect(moved.length).toBe(1);
			expect(moved[0].scaleX).toBe(0.5);
			expect(moved[0].scaleY).toBe(0.5);
		});

		it('ignores tiny movements', () => {
			const almostSame: FlipRect = { x: 0.1, y: 0.1, width: 100, height: 50 };

			const first = new Map<string, FlipState>([
				['a', { id: 'a', rect: rect1, opacity: 1 }]
			]);
			const last = new Map<string, FlipState>([
				['a', { id: 'a', rect: almostSame, opacity: 1 }]
			]);

			const { moved } = calculateTransitions(first, last);

			// Movement less than 0.5px should be ignored
			expect(moved.length).toBe(0);
		});

		it('ignores tiny scale changes', () => {
			const almostSame: FlipRect = { x: 0, y: 0, width: 100.5, height: 50.5 };

			const first = new Map<string, FlipState>([
				['a', { id: 'a', rect: rect1, opacity: 1 }]
			]);
			const last = new Map<string, FlipState>([
				['a', { id: 'a', rect: almostSame, opacity: 1 }]
			]);

			const { moved } = calculateTransitions(first, last);

			// Scale change less than 1% should be ignored
			expect(moved.length).toBe(0);
		});

		it('handles empty maps', () => {
			const first = new Map<string, FlipState>();
			const last = new Map<string, FlipState>();

			const { moved, entered, exited } = calculateTransitions(first, last);

			expect(moved.length).toBe(0);
			expect(entered.length).toBe(0);
			expect(exited.length).toBe(0);
		});
	});
});

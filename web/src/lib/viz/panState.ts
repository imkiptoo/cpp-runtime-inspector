/**
 * Shared mutable pan state for instant edge synchronization.
 * Uses a simple object reference for zero-overhead updates.
 */

export interface PanState {
	x: number;
	y: number;
	version: number; // Increment to trigger updates
}

// Singleton pan state
export const panState: PanState = { x: 0, y: 0, version: 0 };

// Update pan state
export function setPan(x: number, y: number): void {
	panState.x = x;
	panState.y = y;
	panState.version++;
}

// Reset pan state
export function resetPan(): void {
	panState.x = 0;
	panState.y = 0;
	panState.version++;
}

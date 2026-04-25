/**
 * Hover context for tracking which address/heap ID is being hovered.
 */
import { setContext, getContext } from 'svelte';

const HOVER_KEY = Symbol('hover-context');

export interface HoverState {
	hoveredHeapId: number | null;
	hoveredAddress: string | null;
	setHoveredHeapId: (id: number | null) => void;
	setHoveredAddress: (addr: string | null) => void;
}

/**
 * Create and set hover context.
 */
export function setHoverContext(): HoverState {
	let hoveredHeapId = $state<number | null>(null);
	let hoveredAddress = $state<string | null>(null);

	const state: HoverState = {
		get hoveredHeapId() { return hoveredHeapId; },
		get hoveredAddress() { return hoveredAddress; },
		setHoveredHeapId: (id) => { hoveredHeapId = id; },
		setHoveredAddress: (addr) => { hoveredAddress = addr; }
	};

	setContext(HOVER_KEY, state);
	return state;
}

/**
 * Get hover context.
 */
export function getHoverContext(): HoverState {
	return getContext<HoverState>(HOVER_KEY);
}

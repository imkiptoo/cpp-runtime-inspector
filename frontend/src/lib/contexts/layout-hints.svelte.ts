/**
 * Layout hints context for passing ELK layout results to components.
 */
import { setContext, getContext } from 'svelte';
import type { HeapLayout } from '$lib/viz/layoutHeap';

const LAYOUT_KEY = Symbol('layout-hints-context');

export interface LayoutHints {
	heapLayout: HeapLayout | null;
	setHeapLayout: (layout: HeapLayout | null) => void;
}

/**
 * Create and set layout hints context.
 */
export function setLayoutContext(): LayoutHints {
	let heapLayout = $state<HeapLayout | null>(null);

	const state: LayoutHints = {
		get heapLayout() { return heapLayout; },
		setHeapLayout: (layout) => { heapLayout = layout; }
	};

	setContext(LAYOUT_KEY, state);
	return state;
}

/**
 * Get layout hints context.
 */
export function getLayoutContext(): LayoutHints {
	return getContext<LayoutHints>(LAYOUT_KEY);
}

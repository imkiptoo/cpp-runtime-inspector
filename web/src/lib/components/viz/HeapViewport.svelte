<script lang="ts">
	import { onMount, tick } from 'svelte';
	import type { HeapObject } from '$lib/trace/schema';
	import { appState } from '$lib/state/app.svelte';
	import { layoutHeap, type HeapLayout } from '$lib/viz/layoutHeap';
	import { findOrphans } from '$lib/viz/reachability';
	import { capturePositions, calculateTransitions, animateFlip, animateEnter } from '$lib/anim/flip';
	import { setPan, resetPan } from '$lib/viz/panState';
	import HeapGraph from './HeapGraph.svelte';

	interface Props {
		heap: Record<string, HeapObject>;
		addresses?: Record<string, string>;
	}

	let { heap, addresses = {} }: Props = $props();

	let containerRef: HTMLDivElement | undefined = $state();
	let heapContentRef: HTMLDivElement | undefined = $state();
	let layout = $state<HeapLayout | null>(null);
	let orphans = $state<Set<number>>(new Set());

	// Pan state
	let panX = $state(0);
	let panY = $state(0);
	let isPanning = $state(false);
	let startPanX = $state(0);
	let startPanY = $state(0);

	// FLIP animation state
	let previousPositions = $state<Map<string, { id: string; rect: { x: number; y: number; width: number; height: number }; opacity: number }>>(new Map());
	let previousStepIndex = $state(-1);

	const hasHeap = $derived(Object.keys(heap).length > 0);

	// Capture positions before step changes using $effect.pre
	$effect.pre(() => {
		const stepIndex = appState.stepIndex;
		if (heapContentRef && stepIndex !== previousStepIndex) {
			previousPositions = capturePositions(heapContentRef);
		}
	});

	// Recompute layout when heap changes
	$effect(() => {
		if (Object.keys(heap).length > 0) {
			layoutHeap(heap, undefined, appState.heapDensity).then((l) => {
				layout = l;
			});
		} else {
			layout = null;
		}
	});

	// Recompute orphans when step changes
	$effect(() => {
		if (appState.currentStep) {
			orphans = findOrphans(appState.currentStep);
		} else {
			orphans = new Set();
		}
	});

	// Animate after layout changes
	$effect(() => {
		const stepIndex = appState.stepIndex;
		const currentLayout = layout;

		if (!heapContentRef || !currentLayout || stepIndex === previousStepIndex) {
			if (stepIndex !== previousStepIndex) {
				previousStepIndex = stepIndex;
			}
			return;
		}

		tick().then(() => {
			if (!heapContentRef) return;

			const currentPositions = capturePositions(heapContentRef);
			const { moved, entered } = calculateTransitions(previousPositions, currentPositions);

			for (const transition of moved) {
				const element = heapContentRef.querySelector(`[data-flip-id="${transition.id}"]`);
				if (element instanceof HTMLElement) {
					animateFlip(element, transition, 250);
				}
			}

			for (const id of entered) {
				const element = heapContentRef.querySelector(`[data-flip-id="${id}"]`);
				if (element instanceof HTMLElement) {
					animateEnter(element, 200);
				}
			}

			previousStepIndex = stepIndex;
			previousPositions = currentPositions;
		});
	});

	function handleMouseDown(event: MouseEvent) {
		if (event.button !== 0) return;
		isPanning = true;
		startPanX = event.clientX - panX;
		startPanY = event.clientY - panY;
	}

	function handleMouseMove(event: MouseEvent) {
		if (!isPanning) return;
		panX = event.clientX - startPanX;
		panY = event.clientY - startPanY;
		// Update shared pan state and dispatch event for instant edge sync
		setPan(panX, panY);
		containerRef?.dispatchEvent(new CustomEvent('heappan', { bubbles: true }));
	}

	function handleMouseUp() {
		isPanning = false;
	}

	function handleWheel(event: WheelEvent) {
		panX -= event.deltaX;
		panY -= event.deltaY;
		// Update shared pan state and dispatch event for instant edge sync
		setPan(panX, panY);
		containerRef?.dispatchEvent(new CustomEvent('heappan', { bubbles: true }));
	}

	export function resetView() {
		panX = 0;
		panY = 0;
		resetPan();
	}

	onMount(() => {
		window.addEventListener('mouseup', handleMouseUp);
		return () => window.removeEventListener('mouseup', handleMouseUp);
	});
</script>

<!-- svelte-ignore a11y_no_noninteractive_element_interactions -->
<div
	bind:this={containerRef}
	class="h-full w-full overflow-hidden relative {isPanning ? 'cursor-grabbing' : 'cursor-grab'}"
	style="--heap-pan-x: {panX}px; --heap-pan-y: {panY}px;"
	onmousedown={handleMouseDown}
	onmousemove={handleMouseMove}
	onwheel={handleWheel}
	role="application"
	aria-label="Heap visualization viewport"
>
	{#if hasHeap && layout}
		<!-- Heap nodes (edges are rendered at VizPane level) -->
		<div
			bind:this={heapContentRef}
			class="absolute"
			style="transform: translate({panX}px, {panY}px); padding: 24px;"
		>
			<HeapGraph {heap} {layout} {orphans} {addresses} />
		</div>

		{#if panX !== 0 || panY !== 0}
			<div class="absolute bottom-4 right-4 z-10">
				<button
					class="btn-chonky btn-chonky-secondary btn-chonky-sm"
					onclick={resetView}
				><span>Reset View</span></button>
			</div>
		{/if}
	{:else}
		<div class="h-full flex items-center justify-center text-islands-400 dark:text-islands-500">
			<div class="text-center">
				<svg class="w-12 h-12 mx-auto mb-2 opacity-50" fill="none" stroke="currentColor" viewBox="0 0 24 24">
					<path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.5" d="M20 7l-8-4-8 4m16 0l-8 4m8-4v10l-8 4m0-10L4 7m8 4v10M4 7v10l8 4" />
				</svg>
				<p class="text-[13px]">No heap allocations</p>
			</div>
		</div>
	{/if}
</div>

<script lang="ts">
	import { onMount, tick } from 'svelte';
	import { appState } from '$lib/state/app.svelte';
	import { getHoverContext } from '$lib/contexts/hover.svelte';
	import { panState } from '$lib/viz/panState';
	import {
		routeEdges,
		edgePath,
		toMeasured,
		type EdgeSample,
		type CardRect,
		type RoutedEdge,
		type MeasuredRect
	} from '$lib/viz/routeEdges';
	import { FLIP_DURATION } from '$lib/anim/flip';

	interface Props {
		/** Container whose descendants carry [data-ptr-target] / [data-heap-addr] attributes. */
		containerRef: HTMLElement | null;
	}

	let { containerRef }: Props = $props();

	const hover = getHoverContext();

	// Base edges computed at pan (0,0) - we store extra info about which ends are heap-side
	interface BaseEdge extends RoutedEdge {
		sourceInHeap: boolean;
		targetInHeap: boolean;
	}
	let baseEdges = $state<BaseEdge[]>([]);
	let basePanX = $state(0);
	let basePanY = $state(0);
	let containerRect = $state<DOMRect | null>(null);

	// Live pan tracking - poll every frame for instant response
	let livePanX = $state(0);
	let livePanY = $state(0);

	// Derived edges with pan offset applied
	const edges = $derived.by(() => {
		const dx = livePanX - basePanX;
		const dy = livePanY - basePanY;

		return baseEdges.map((e) => ({
			...e,
			x1: e.x1 + (e.sourceInHeap ? dx : 0),
			y1: e.y1 + (e.sourceInHeap ? dy : 0),
			x2: e.x2 + (e.targetInHeap ? dx : 0),
			y2: e.y2 + (e.targetInHeap ? dy : 0)
		}));
	});

	// Build target map: address -> element
	function buildTargetMap(container: HTMLElement): Map<string, HTMLElement> {
		const map = new Map<string, HTMLElement>();

		// Heap nodes
		for (const el of container.querySelectorAll<HTMLElement>('[data-heap-addr]')) {
			const k = el.getAttribute('data-heap-addr');
			if (k && !map.has(k)) map.set(k, el);
		}

		// Stack locals (if we add data-stack-addr later)
		for (const el of container.querySelectorAll<HTMLElement>('[data-stack-addr]')) {
			const k = el.getAttribute('data-stack-addr');
			if (k && !map.has(k)) map.set(k, el);
		}

		return map;
	}

	// Extended edge sample with heap flags
	interface ExtendedSample extends EdgeSample {
		sourceInHeap: boolean;
		targetInHeap: boolean;
	}

	// Compute all edges from DOM
	function computeEdges(container: HTMLElement): BaseEdge[] {
		const cRect = container.getBoundingClientRect();
		containerRect = cRect;

		const ptrs = Array.from(container.querySelectorAll<HTMLElement>('[data-ptr-target]'));
		const targetMap = buildTargetMap(container);

		// Collect edge samples with heap flags
		const samples: ExtendedSample[] = [];
		for (let i = 0; i < ptrs.length; i++) {
			const p = ptrs[i]!;
			const target = p.getAttribute('data-ptr-target');
			if (!target || target === 'null') continue;

			const targetEl = targetMap.get(target);
			if (!targetEl) continue;

			const kind = p.getAttribute('data-ptr-kind') === 'ref' ? 'ref' : 'pointer';
			const sourceCardEl = p.closest<HTMLElement>('[data-heap-addr]');
			const sourceAddr = sourceCardEl?.getAttribute('data-heap-addr') ?? null;

			// Check if source/target are in heap (have data-heap-addr or are descendants)
			const sourceInHeap = sourceCardEl !== null;
			const targetInHeap = targetEl.closest('[data-heap-addr]') !== null || targetEl.hasAttribute('data-heap-addr');

			samples.push({
				key: `${i}:${target}`,
				kind,
				target,
				sourceAddr,
				chip: toMeasured(p.getBoundingClientRect()),
				sourceCard: sourceCardEl ? toMeasured(sourceCardEl.getBoundingClientRect()) : null,
				targetEl: toMeasured(targetEl.getBoundingClientRect()),
				sourceInHeap,
				targetInHeap
			});
		}

		// Collect obstacles (heap cards and stack frames)
		const obstacles: CardRect[] = [];
		for (const el of container.querySelectorAll<HTMLElement>('[data-heap-addr]')) {
			const id = el.getAttribute('data-heap-addr');
			if (!id) continue;
			obstacles.push({ id, ...toMeasured(el.getBoundingClientRect()) });
		}
		for (const el of container.querySelectorAll<HTMLElement>('[data-stack-frame-id]')) {
			const id = el.getAttribute('data-stack-frame-id');
			if (!id) continue;
			obstacles.push({ id, ...toMeasured(el.getBoundingClientRect()) });
		}

		const routed = routeEdges(samples, { obstacles });

		// Convert to container-relative coordinates and preserve heap flags
		return routed.map((r, idx) => ({
			...r,
			x1: r.x1 - cRect.left,
			y1: r.y1 - cRect.top,
			x2: r.x2 - cRect.left,
			y2: r.y2 - cRect.top,
			sourceInHeap: samples[idx]?.sourceInHeap ?? false,
			targetInHeap: samples[idx]?.targetInHeap ?? true
		}));
	}

	// Recompute base edges and store current pan offset
	function recompute() {
		if (!containerRef) {
			baseEdges = [];
			return;
		}
		basePanX = panState.x;
		basePanY = panState.y;
		livePanX = panState.x;
		livePanY = panState.y;
		baseEdges = computeEdges(containerRef);
	}

	// Track step changes and recompute
	$effect(() => {
		const _step = appState.stepIndex;
		const _trace = appState.trace;
		const _routing = appState.pointerRouting;
		const _container = containerRef;

		if (!_container) {
			baseEdges = [];
			return;
		}

		// Wait for DOM to update
		tick().then(() => {
			recompute();

			// Follow FLIP animations
			const start = performance.now();
			const follow = () => {
				if (performance.now() - start < FLIP_DURATION + 50) {
					recompute();
					requestAnimationFrame(follow);
				}
			};
			requestAnimationFrame(follow);
		});
	});

	// Set up observers for DOM changes and pan events
	onMount(() => {
		if (!containerRef) return;

		let rafId: number | null = null;

		const schedule = () => {
			if (rafId !== null) cancelAnimationFrame(rafId);
			rafId = requestAnimationFrame(() => {
				rafId = null;
				recompute();
			});
		};

		// Handle pan events - update instantly (no rAF delay)
		const handlePan = () => {
			livePanX = panState.x;
			livePanY = panState.y;
		};

		const ro = new ResizeObserver(schedule);
		ro.observe(containerRef);

		const mo = new MutationObserver(schedule);
		mo.observe(containerRef, {
			childList: true,
			subtree: true,
			attributes: true,
			attributeFilter: [
				'data-heap-addr',
				'data-ptr-target',
				'data-stack-addr',
				'data-stack-frame-id',
				'data-stack-expanded',
			],
		});

		// Listen for pan events from HeapViewport
		containerRef.addEventListener('heappan', handlePan);

		// Listen for node drag events - recompute edges after DOM update
		const handleNodeDrag = () => {
			tick().then(recompute);
		};
		document.addEventListener('heapnodedrag', handleNodeDrag);

		return () => {
			ro.disconnect();
			mo.disconnect();
			containerRef?.removeEventListener('heappan', handlePan);
			document.removeEventListener('heapnodedrag', handleNodeDrag);
			if (rafId !== null) cancelAnimationFrame(rafId);
		};
	});

	function isHighlighted(edge: RoutedEdge): boolean {
		const hoveredId = hover?.hoveredHeapId;
		if (hoveredId === null || hoveredId === undefined) return false;
		return edge.target === String(hoveredId);
	}

	function handleMouseEnter(target: string) {
		const id = parseInt(target, 10);
		if (!isNaN(id)) {
			hover?.setHoveredHeapId(id);
		}
	}

	function handleMouseLeave() {
		hover?.setHoveredHeapId(null);
	}
</script>

<svg
	aria-hidden="true"
	class="absolute inset-0 h-full w-full pointer-events-none z-10"
	data-testid="edge-layer"
>
	<defs>
		<marker
			id="spp-arrow"
			viewBox="0 0 10 10"
			refX="9"
			refY="5"
			markerWidth="6"
			markerHeight="6"
			orient="auto-start-reverse"
		>
			<path d="M 0 0 L 10 5 L 0 10 z" class="fill-brand-500 dark:fill-brand-400" />
		</marker>
		<marker
			id="spp-arrow-hot"
			viewBox="0 0 10 10"
			refX="9"
			refY="5"
			markerWidth="7"
			markerHeight="7"
			orient="auto-start-reverse"
		>
			<path d="M 0 0 L 10 5 L 0 10 z" class="fill-brand-600 dark:fill-brand-300" />
		</marker>
	</defs>

	{#each edges as edge (edge.key)}
		{@const hot = isHighlighted(edge)}
		{@const d = edgePath(edge, appState.pointerRouting)}
		<g style="pointer-events: auto">
			<!-- Wide invisible hit region for easier hovering -->
			<path
				{d}
				fill="none"
				stroke="transparent"
				stroke-width="12"
				onmouseenter={() => handleMouseEnter(edge.target)}
				onmouseleave={handleMouseLeave}
			/>
			<!-- Visible edge path -->
			<path
				{d}
				data-ptr-kind={edge.kind}
				data-highlighted={hot || undefined}
				fill="none"
				class="{hot
					? 'stroke-brand-600 dark:stroke-brand-300'
					: 'stroke-brand-400 dark:stroke-brand-500'}"
				stroke-width={hot ? 2 : 1.5}
				stroke-dasharray={edge.kind === 'ref' ? '4 3' : undefined}
				marker-end={hot ? 'url(#spp-arrow-hot)' : 'url(#spp-arrow)'}
				style="pointer-events: none"
			/>
		</g>
	{/each}
</svg>

<script lang="ts">
	import { onMount } from 'svelte';
	import type { HeapObject } from '$lib/trace/schema';
	import type { HeapLayout } from '$lib/viz/layoutHeap';
	import { nodePositions } from '$lib/viz/nodePositions';
	import HeapNode from './HeapNode.svelte';

	interface Props {
		heap: Record<string, HeapObject>;
		layout: HeapLayout;
		orphans: Set<number>;
		addresses?: Record<string, string>;
	}

	let { heap, layout, orphans, addresses = {} }: Props = $props();

	// Drag state
	let draggingId: string | null = $state(null);
	let dragStartX = 0;
	let dragStartY = 0;
	let dragOffsetX = 0;
	let dragOffsetY = 0;

	// Track offsets reactively
	let offsetVersion = $state(0);

	function getNodeOffset(heapId: string) {
		// Access version to trigger reactivity
		const _ = offsetVersion;
		return nodePositions.getOffset(heapId);
	}

	function handleMouseDown(event: MouseEvent, heapId: string) {
		// Only drag with left mouse button
		if (event.button !== 0) return;

		// Prevent viewport panning
		event.stopPropagation();

		draggingId = heapId;
		const offset = nodePositions.getOffset(heapId);
		dragStartX = event.clientX;
		dragStartY = event.clientY;
		dragOffsetX = offset.x;
		dragOffsetY = offset.y;
	}

	function handleMouseMove(event: MouseEvent) {
		if (!draggingId) return;

		const dx = event.clientX - dragStartX;
		const dy = event.clientY - dragStartY;

		nodePositions.setOffset(draggingId, dragOffsetX + dx, dragOffsetY + dy);
		offsetVersion = nodePositions.version;

		// Dispatch event for edge updates
		document.dispatchEvent(new CustomEvent('heapnodedrag', { bubbles: true }));
	}

	function handleMouseUp() {
		draggingId = null;
	}

	onMount(() => {
		window.addEventListener('mousemove', handleMouseMove);
		window.addEventListener('mouseup', handleMouseUp);
		return () => {
			window.removeEventListener('mousemove', handleMouseMove);
			window.removeEventListener('mouseup', handleMouseUp);
		};
	});
</script>

<div class="relative" style="width: {layout.width}px; height: {layout.height}px;">
	{#each Object.entries(heap) as [heapId, obj] (heapId)}
		{@const nodeLayout = layout.nodes.get(heapId)}
		{@const offset = getNodeOffset(heapId)}
		{#if nodeLayout}
			<!-- svelte-ignore a11y_no_static_element_interactions -->
			<div
				class="absolute {draggingId === heapId ? 'cursor-grabbing z-10' : 'cursor-grab'}"
				style="left: {nodeLayout.x + offset.x}px; top: {nodeLayout.y + offset.y}px;"
				data-flip-id="heap-{heapId}"
				onmousedown={(e) => handleMouseDown(e, heapId)}
			>
				<HeapNode
					id={parseInt(heapId)}
					{obj}
					address={addresses[heapId]}
					isOrphan={orphans.has(parseInt(heapId))}
				/>
			</div>
		{/if}
	{/each}
</div>

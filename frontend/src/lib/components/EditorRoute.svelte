<script lang="ts">
	import { appState } from '$lib/state/app.svelte';
	import EditorPane from './editor/EditorPane.svelte';
	import VizPane from './viz/VizPane.svelte';
	import ConsolePane from './console/ConsolePane.svelte';
	import ExecutionBar from './exec/ExecutionBar.svelte';

	let containerWidth = $state(0);
	let containerRef: HTMLDivElement | undefined = $state();

	// Calculate editor width from fraction
	const editorWidth = $derived(containerWidth * appState.editorFraction);

	// Handle resize via drag on the edge between panels
	let isResizing = $state(false);
	let resizeStartX = $state(0);
	let resizeStartFraction = $state(0);

	function handleResizeStart(event: MouseEvent) {
		isResizing = true;
		resizeStartX = event.clientX;
		resizeStartFraction = appState.editorFraction;
		window.addEventListener('mousemove', handleResizeMove);
		window.addEventListener('mouseup', handleResizeEnd);
	}

	function handleResizeMove(event: MouseEvent) {
		if (!isResizing || containerWidth === 0) return;
		const delta = event.clientX - resizeStartX;
		const newFraction = resizeStartFraction + (delta / containerWidth);
		appState.setEditorFraction(newFraction);
	}

	function handleResizeEnd() {
		isResizing = false;
		window.removeEventListener('mousemove', handleResizeMove);
		window.removeEventListener('mouseup', handleResizeEnd);
	}

	// Vertical resize for console
	let isResizingConsole = $state(false);
	let resizeStartY = $state(0);
	let resizeStartHeight = $state(0);

	function handleConsoleResizeStart(event: MouseEvent) {
		isResizingConsole = true;
		resizeStartY = event.clientY;
		resizeStartHeight = appState.consoleHeightPx;
		window.addEventListener('mousemove', handleConsoleResizeMove);
		window.addEventListener('mouseup', handleConsoleResizeEnd);
	}

	function handleConsoleResizeMove(event: MouseEvent) {
		if (!isResizingConsole) return;
		const delta = resizeStartY - event.clientY;
		appState.setConsoleHeight(resizeStartHeight + delta);
	}

	function handleConsoleResizeEnd() {
		isResizingConsole = false;
		window.removeEventListener('mousemove', handleConsoleResizeMove);
		window.removeEventListener('mouseup', handleConsoleResizeEnd);
	}

	$effect(() => {
		if (!containerRef) return;

		const observer = new ResizeObserver((entries) => {
			for (const entry of entries) {
				containerWidth = entry.contentRect.width;
			}
		});

		observer.observe(containerRef);
		return () => observer.disconnect();
	});
</script>

<div class="h-full flex flex-col gap-1.5" bind:this={containerRef}>
	<!-- Main content area with Islands panels -->
	<div class="flex-1 min-h-0 flex gap-1.5">
		<!-- Editor Pane Island -->
		<div class="h-full overflow-hidden islands-panel relative" style="width: {editorWidth}px; min-width: 200px;">
			<EditorPane />
			<!-- Resize handle on right edge -->
			<div
				class="absolute top-0 right-0 w-1 h-full cursor-col-resize hover:bg-brand-400 transition-colors z-10 {isResizing ? 'bg-brand-500' : ''}"
				onmousedown={handleResizeStart}
				role="separator"
				aria-orientation="horizontal"
			></div>
		</div>

		<!-- Visualization area -->
		<div class="flex-1 min-w-0 h-full overflow-hidden flex flex-col gap-1.5">
			<!-- Viz Pane Island -->
			<div class="flex-1 min-h-0 overflow-hidden islands-panel relative">
				<VizPane />
				<!-- Console resize handle on bottom edge -->
				{#if appState.consoleOpen}
					<div
						class="absolute bottom-0 left-0 right-0 h-1 cursor-row-resize hover:bg-brand-400 transition-colors z-10 {isResizingConsole ? 'bg-brand-500' : ''}"
						onmousedown={handleConsoleResizeStart}
						role="separator"
						aria-orientation="vertical"
					></div>
				{/if}
			</div>

			<!-- Console Island (collapsible) -->
			{#if appState.consoleOpen}
				<div class="islands-panel overflow-hidden" style="height: {appState.consoleHeightPx}px; min-height: 100px;">
					<ConsolePane />
				</div>
			{/if}
		</div>
	</div>

	<!-- Execution Bar Island -->
	<div class="islands-panel">
		<ExecutionBar />
	</div>
</div>

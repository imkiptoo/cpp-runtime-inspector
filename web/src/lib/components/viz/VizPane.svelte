<script lang="ts">
	import { appState } from '$lib/state/app.svelte';
	import StackFrames from './StackFrames.svelte';
	import HeapViewport from './HeapViewport.svelte';
	import EdgeLayer from './EdgeLayer.svelte';
	import { setHoverContext, getHoverContext } from '$lib/contexts/hover.svelte';
	import { setHeapAddressesContext } from '$lib/contexts/heapAddresses.svelte';
	import { shortcuts } from '$lib/platform/kbd';

	// Set up hover context for this viz pane
	setHoverContext();
	const hover = getHoverContext();

	// Set up heap addresses context - updates when step changes
	$effect(() => {
		setHeapAddressesContext(appState.currentStep?.heap_addresses);
	});

	// Reference to heap viewport for recenter
	let heapViewportRef: { resetView: () => void } | undefined = $state();

	// Container ref for EdgeLayer (covers both stack and heap)
	let vizContainerRef: HTMLDivElement | undefined = $state();

	const hasTrace = $derived(appState.trace !== null && appState.trace.trace.length > 0);
	const step = $derived(appState.currentStep);
	const buildFailed = $derived(!!appState.error && !appState.running);

	// Counts for section labels
	const stackCount = $derived(step?.stack_to_render.length ?? 0);
	const heapCount = $derived(step ? Object.keys(step.heap).length : 0);
</script>

<section class="h-full flex flex-col bg-white dark:bg-islands-900" data-testid="viz-pane">
	<!-- Header - Islands panel style -->
	<header class="flex items-center justify-between h-9 px-3 border-b border-line">
		<div class="flex items-center gap-3">
			<span class="font-medium text-sm text-islands-700 dark:text-islands-200">Memory</span>
			{#if step}
				<span class="text-sm text-islands-400 dark:text-islands-500">
					{heapCount} on heap
				</span>
			{/if}
		</div>
		{#if hasTrace}
			<button
				type="button"
				onclick={() => heapViewportRef?.resetView()}
				class="btn-chonky btn-chonky-secondary btn-chonky-sm"
			><span>Recenter</span></button>
		{/if}
	</header>

	<!-- Build Failed Banner -->
	{#if buildFailed}
		<div class="flex items-start justify-between gap-3 border-b border-red-200 dark:border-red-800 bg-red-50 dark:bg-red-900/20 px-3 min-h-9 py-1.5 text-sm text-red-700 dark:text-red-300">
			<span class="flex items-start gap-2">
				<svg class="w-4 h-4 mt-0.5 shrink-0" fill="none" stroke="currentColor" viewBox="0 0 24 24">
					<path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 8v4m0 4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
				</svg>
				<span>
					<strong class="font-semibold">Build failed</strong>
					<span class="text-red-600 dark:text-red-400"> — {appState.error}</span>
				</span>
			</span>
			<button
				type="button"
				onclick={() => appState.run()}
				class="btn-chonky btn-chonky-danger btn-chonky-sm shrink-0"
			><span>Re-run</span></button>
		</div>
	{:else if appState.stale && hasTrace}
		<!-- Stale Trace Banner -->
		<div class="flex items-center justify-between gap-3 border-b border-amber-200 dark:border-amber-800 bg-amber-50 dark:bg-amber-900/20 px-3 min-h-9 py-1.5 text-sm text-amber-700 dark:text-amber-300">
			<span class="flex items-center gap-2">
				<span aria-hidden="true" class="inline-block h-2 w-2 rounded-full bg-amber-500"></span>
				<span>
					<strong class="font-semibold">Trace is stale</strong>
					<span class="text-amber-600 dark:text-amber-400"> — code has been edited since the last run.</span>
				</span>
			</span>
			<button
				type="button"
				onclick={() => appState.run()}
				class="btn-chonky btn-chonky-warning btn-chonky-sm shrink-0"
			><span>Re-run</span></button>
		</div>
	{/if}

	<!-- Memory Leaks Banner -->
	{#if appState.hasMemoryLeaks && !buildFailed}
		<div class="flex items-center gap-2 border-b border-red-200 dark:border-red-800 bg-red-50 dark:bg-red-900/20 px-3 min-h-9 py-1.5 text-sm text-red-700 dark:text-red-300">
			<svg class="w-4 h-4 shrink-0" fill="none" stroke="currentColor" viewBox="0 0 24 24">
				<path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z" />
			</svg>
			<span>
				<strong class="font-semibold">Memory leaks detected</strong>
				<span class="text-red-600 dark:text-red-400"> — {appState.trace?.memory_leaks?.length} allocation(s) not freed</span>
			</span>
		</div>
	{/if}

	<!-- Main visualization area -->
	<div
		bind:this={vizContainerRef}
		class="flex-1 min-h-0 flex overflow-hidden relative transition-opacity duration-150 {appState.stale && hasTrace ? 'opacity-60' : ''}"
		data-stale={appState.stale && hasTrace ? true : undefined}
	>
		{#if appState.running && !hasTrace}
			<!-- Running state -->
			<div class="flex-1 flex items-center justify-center">
				<div class="flex items-center gap-3 text-islands-500 dark:text-islands-400">
					<svg class="w-5 h-5 animate-spin" fill="none" viewBox="0 0 24 24">
						<circle class="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" stroke-width="4"></circle>
						<path class="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
					</svg>
					<span class="text-sm">Compiling and running...</span>
				</div>
			</div>
		{:else if hasTrace && step}
			<!-- Stack and Heap panels (z-0 so arrows render on top) -->
			<div class="grid h-full min-h-0 w-full z-0" style="grid-template-columns: 260px 1fr">
				<!-- Stack panel -->
				<div class="flex min-h-0 min-w-0 flex-col overflow-auto border-r border-line bg-white dark:bg-islands-800/50">
					<div class="flex items-center justify-between px-3 py-2 text-[11px] font-medium uppercase tracking-wider text-islands-500 dark:text-islands-400 border-b border-line bg-islands-50 dark:bg-islands-800">
						<span>Stack</span>
						<span class="text-islands-600 dark:text-islands-300">{stackCount}</span>
					</div>
					<div class="pt-2 flex-1 overflow-auto">
						<StackFrames frames={step.stack_to_render} globals={step.globals} orderedGlobals={step.ordered_globals} />
					</div>
				</div>

				<!-- Heap panel -->
				<div class="relative flex min-h-0 min-w-0 flex-col overflow-hidden bg-white dark:bg-islands-900">
					<div class="flex items-center justify-between px-3 py-2 text-[11px] font-medium uppercase tracking-wider text-islands-500 dark:text-islands-400 border-b border-line bg-islands-50 dark:bg-islands-800">
						<span>Heap</span>
						<span class="text-islands-600 dark:text-islands-300">{heapCount}</span>
					</div>
					<div class="flex-1 min-h-0">
						<HeapViewport heap={step.heap} addresses={step.heap_addresses} bind:this={heapViewportRef} />
					</div>
				</div>
			</div>

			<!-- Edge layer - renders ALL arrows (stack-to-heap and heap-to-heap) -->
			<!-- Placed after panels with z-10 so arrows render on top -->
			<EdgeLayer containerRef={vizContainerRef ?? null} />
		{:else if buildFailed}
			<!-- Build failed empty state -->
			<div class="flex-1 flex items-center justify-center">
				<p class="text-sm text-islands-400 dark:text-islands-500">
					No trace — fix the errors above and re-run.
				</p>
			</div>
		{:else}
			<!-- Empty state -->
			<div class="flex-1 flex items-center justify-center">
				<div class="text-center">
					<svg class="w-12 h-12 mx-auto mb-3 text-islands-300 dark:text-islands-600" fill="none" stroke="currentColor" viewBox="0 0 24 24">
						<path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.5" d="M14.752 11.168l-3.197-2.132A1 1 0 0010 9.87v4.263a1 1 0 001.555.832l3.197-2.132a1 1 0 000-1.664z" />
						<path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.5" d="M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
					</svg>
					<p class="text-sm text-islands-400 dark:text-islands-500">
						Click <span class="text-islands-600 dark:text-islands-300 font-medium">Run</span>
						<span> or press </span>
						<kbd class="px-1.5 py-0.5 text-[11px] font-mono bg-islands-100 dark:bg-islands-800 text-islands-600 dark:text-islands-300 rounded-sm border border-line">{shortcuts.run()}</kbd>
						<span> to visualize.</span>
					</p>
				</div>
			</div>
		{/if}
	</div>
</section>

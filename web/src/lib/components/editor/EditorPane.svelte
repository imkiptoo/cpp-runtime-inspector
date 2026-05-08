<script lang="ts">
	import { onMount } from 'svelte';
	import { appState } from '$lib/state/app.svelte';
	import { createEditor } from './codemirror.action';

	let editorContainer: HTMLDivElement | undefined = $state();
	let editorView: ReturnType<typeof createEditor> | undefined = $state();

	onMount(() => {
		if (!editorContainer) return;

		editorView = createEditor(editorContainer, {
			initialCode: appState.code,
			onChange: (code) => appState.setCode(code),
			onRun: () => appState.run(),
			onLineClick: (line) => {
				// Jump to step with this line
				if (!appState.trace) return;
				const idx = appState.trace.trace.findIndex((s) => s.line === line);
				if (idx !== -1) {
					appState.stepTo(idx);
				}
			}
		});

		return () => editorView?.destroy();
	});

	// Update trace line highlight when step changes
	$effect(() => {
		if (editorView && appState.currentStep) {
			editorView.setTraceLine(appState.currentStep.line);
		} else if (editorView) {
			editorView.setTraceLine(null);
		}
	});

	// Update editor content when code changes externally (e.g., loading example)
	$effect(() => {
		if (editorView && appState.code !== editorView.getCode()) {
			editorView.setCode(appState.code);
		}
	});

	// Update theme when it changes
	$effect(() => {
		if (editorView) {
			editorView.setTheme(appState.resolvedTheme);
		}
	});
</script>

<div class="h-full flex flex-col bg-white dark:bg-islands-900">
	<!-- Tab bar - Islands style -->
	<div class="flex items-end h-9 px-2 border-b border-line bg-islands-50 dark:bg-islands-800/50">
		<div class="flex items-center gap-2 px-3 py-1.25 rounded-t-md -mb-px text-sm font-medium text-islands-700 dark:text-islands-200 bg-white dark:bg-islands-900 border border-line">
			<svg class="w-4 h-4 text-islands-400 dark:text-islands-500" fill="none" stroke="currentColor" viewBox="0 0 24 24">
				<path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.5" d="M10 20l4-16m4 4l4 4-4 4M6 16l-4-4 4-4" />
			</svg>
			<span>main.cpp</span>
			{#if appState.stale}
				<span class="stale-indicator" title="Code changed since last run"></span>
			{/if}
		</div>
	</div>

	<!-- Editor -->
	<div class="flex-1 min-h-0 overflow-hidden" bind:this={editorContainer}></div>
</div>

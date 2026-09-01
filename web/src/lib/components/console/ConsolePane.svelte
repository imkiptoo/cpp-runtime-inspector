<script lang="ts">
	import { untrack } from 'svelte';
	import { appState } from '$lib/state/app.svelte';
	import { getLeakHeapId, getLeakTypeName } from '$lib/trace/schema';
	import CodeViewer from './CodeViewer.svelte';

	const stdout = $derived(appState.currentStep?.stdout ?? '');
	const hasError = $derived(appState.error !== null);
	const hasBuildOutput = $derived(appState.buildOutput !== null);
	const hasStdout = $derived(stdout.length > 0);
	const exceptionMsg = $derived(appState.currentStep?.exception_msg);

	const instrumented = $derived(appState.instrumentedSource ?? '');
	const traceJson = $derived(appState.traceJson);

	type Tab = 'output' | 'build' | 'instrumented' | 'json' | 'leaks';
	let activeTab = $state<Tab>('output');

	// Auto-switch tab based on content
	$effect(() => {
		if (hasBuildOutput || hasError) {
			// Build failures always win — the user needs to see them.
			activeTab = 'build';
		} else if (hasStdout || exceptionMsg) {
			// Don't yank the user out of a tab they deliberately opened.
			// untrack keeps this effect from re-running on their tab clicks.
			const current = untrack(() => activeTab);
			if (current === 'output' || current === 'build') {
				activeTab = 'output';
			}
		}
	});

	const TAB_BASE = 'px-3 py-0.75 cursor-pointer font-medium rounded-sm transition-colors';
	const TAB_ACTIVE = 'bg-islands-200 text-islands-900 dark:bg-islands-700 dark:text-islands-100';
	const TAB_IDLE =
		'text-islands-500 hover:text-islands-800 hover:bg-islands-200/60 dark:text-islands-400 dark:hover:text-islands-200 dark:hover:bg-islands-700/50';

	function tabClass(tab: Tab): string {
		return `${TAB_BASE} ${activeTab === tab ? TAB_ACTIVE : TAB_IDLE}`;
	}

	// The code tabs render in CodeMirror, which is viewport-virtualized and so
	// copes with big documents; this cap only guards against pathological
	// traces (parsing for highlighting is still O(doc)). Copy yields the full
	// text either way.
	const MAX_RENDER_CHARS = 1_000_000;

	function clip(text: string): string {
		return text.length > MAX_RENDER_CHARS ? text.slice(0, MAX_RENDER_CHARS) : text;
	}

	const isCodeTab = $derived(activeTab === 'instrumented' || activeTab === 'json');

	let copied = $state(false);
	let copyTimer: ReturnType<typeof setTimeout> | null = null;

	async function copy(text: string) {
		try {
			await navigator.clipboard.writeText(text);
			copied = true;
			if (copyTimer) clearTimeout(copyTimer);
			copyTimer = setTimeout(() => (copied = false), 1500);
		} catch {
			// Clipboard unavailable (insecure origin / denied permission).
		}
	}

	$effect(() => () => {
		if (copyTimer) clearTimeout(copyTimer);
	});
</script>

<div class="h-full flex text-[14px] flex-col bg-white text-islands-700 dark:bg-islands-900 dark:text-islands-200">
	<!-- Header with tabs -->
	<div class="flex items-center justify-between h-9 px-2 border-b border-line bg-islands-50 dark:bg-islands-800">
		<div class="flex items-center gap-1">
			<button class={tabClass('output')} onclick={() => activeTab = 'output'}>
				Output
			</button>
			<button
				class="{tabClass('build')} {hasError ? 'text-red-600 dark:text-red-400' : ''}"
				onclick={() => activeTab = 'build'}
			>
				Build
				{#if hasError}
					<span class="ml-1 w-2 h-2 inline-block rounded-full bg-red-500"></span>
				{/if}
			</button>
			<button
				class={tabClass('instrumented')}
				onclick={() => activeTab = 'instrumented'}
				title="C++ source after the Clang plugin injected instrumentation"
			>
				Instrumented
			</button>
			<button
				class={tabClass('json')}
				onclick={() => activeTab = 'json'}
				title="Raw OPT-format trace returned by the runtime"
			>
				JSON
			</button>
			{#if appState.hasMemoryLeaks}
				<button class={tabClass('leaks')} onclick={() => activeTab = 'leaks'}>
					Leaks
					<span class="ml-1 px-1.5 py-0.5 bg-amber-500/20 text-amber-600 dark:text-amber-400 rounded-sm">
						{appState.trace?.memory_leaks?.length}
					</span>
				</button>
			{/if}
		</div>

		<div class="flex items-center gap-1">
			{#if (activeTab === 'instrumented' && instrumented) || (activeTab === 'json' && traceJson)}
				<button
					class="px-2 py-1 font-medium rounded-sm text-islands-500 hover:text-islands-800 hover:bg-islands-200 dark:text-islands-400 dark:hover:text-islands-200 dark:hover:bg-islands-700 transition-colors"
					onclick={() => copy(activeTab === 'instrumented' ? instrumented : traceJson)}
				>
					{copied ? 'Copied' : 'Copy'}
				</button>
			{/if}

			<button
				class="p-1.5 text-islands-500 hover:text-islands-800 hover:bg-islands-200 dark:text-islands-400 dark:hover:text-islands-200 dark:hover:bg-islands-700 rounded-sm transition-colors"
				onclick={() => appState.toggleConsole()}
				aria-label="Close console"
				>
					<svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
						<path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M6 18L18 6M6 6l12 12" />
					</svg>
				</button>
			</div>
		</div>

	<!-- Content. Code tabs manage their own scrolling and padding. -->
	<div class="flex-1 min-h-0 {isCodeTab ? 'overflow-hidden' : 'overflow-auto p-3'}">
		{#if activeTab === 'output'}
			<div class="console-output font-mono">
				{#if exceptionMsg}
					<div class="text-red-600 dark:text-red-400 mb-2">
						<span class="font-semibold">Exception:</span> {exceptionMsg}
					</div>
				{/if}

				{#if hasStdout}
					<pre class="text-islands-700 dark:text-islands-300 whitespace-pre-wrap">{stdout}</pre>
				{:else if !exceptionMsg}
					<span class="text-islands-400 dark:text-islands-500 italic">No output yet</span>
				{/if}
			</div>
		{:else if activeTab === 'build'}
			<div class="console-output font-mono">
				{#if hasError}
					<div class="text-red-600 dark:text-red-400 mb-2">
						<span class="font-semibold">Error:</span> {appState.error}
					</div>
				{/if}

				{#if hasBuildOutput}
					<pre class="text-islands-600 dark:text-islands-400 whitespace-pre-wrap">{appState.buildOutput}</pre>
				{:else if !hasError}
					<span class="text-islands-400 dark:text-islands-500 italic">No build output</span>
				{/if}
			</div>
		{:else if activeTab === 'instrumented'}
			{#if instrumented}
				<div class="h-full flex flex-col">
					<div class="flex-1 min-h-0">
						<CodeViewer doc={clip(instrumented)} language="cpp" />
					</div>
					{#if instrumented.length > MAX_RENDER_CHARS}
						<div class="shrink-0 px-3 py-1.5 border-t border-line italic text-islands-400 dark:text-islands-500">
							Showing the first {MAX_RENDER_CHARS.toLocaleString()} of
							{instrumented.length.toLocaleString()} characters — use Copy for the full source.
						</div>
					{/if}
				</div>
			{:else}
				<div class="p-3">
					<span class="text-islands-400 dark:text-islands-500 italic">
						Run your code to see the instrumented source
					</span>
				</div>
			{/if}
		{:else if activeTab === 'json'}
			{#if traceJson}
				<div class="h-full flex flex-col">
					<div class="flex-1 min-h-0">
						<CodeViewer doc={clip(traceJson)} language="json" />
					</div>
					{#if traceJson.length > MAX_RENDER_CHARS}
						<div class="shrink-0 px-3 py-1.5 border-t border-line italic text-islands-400 dark:text-islands-500">
							Showing the first {MAX_RENDER_CHARS.toLocaleString()} of
							{traceJson.length.toLocaleString()} characters — use Copy for the full trace.
						</div>
					{/if}
				</div>
			{:else}
				<div class="p-3">
					<span class="text-islands-400 dark:text-islands-500 italic">
						Run your code to see the trace JSON
					</span>
				</div>
			{/if}
		{:else if activeTab === 'leaks'}
			<div class="space-y-2">
				{#if appState.trace?.memory_leaks}
					{#each appState.trace.memory_leaks as leak}
						<div class="p-2.5 bg-amber-50 border border-amber-200 dark:bg-amber-900/20 dark:border-amber-700/30 rounded-sm">
							<div class="flex items-center justify-between">
								<span class="font-mono text-amber-700 dark:text-amber-300">heap[{getLeakHeapId(leak)}]</span>
								<span class="text-islands-500 dark:text-islands-400">leaked</span>
							</div>
							<div class="text-islands-500 dark:text-islands-400 mt-1">
								Type: <span class="text-islands-700 dark:text-islands-300">{getLeakTypeName(leak)}</span>
							</div>
						</div>
					{/each}
				{/if}
			</div>
		{/if}
	</div>
</div>

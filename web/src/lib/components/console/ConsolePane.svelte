<script lang="ts">
	import { appState } from '$lib/state/app.svelte';
	import { getLeakHeapId, getLeakTypeName } from '$lib/trace/schema';

	const stdout = $derived(appState.currentStep?.stdout ?? '');
	const hasError = $derived(appState.error !== null);
	const hasBuildOutput = $derived(appState.buildOutput !== null);
	const hasStdout = $derived(stdout.length > 0);
	const exceptionMsg = $derived(appState.currentStep?.exception_msg);

	type Tab = 'output' | 'build' | 'leaks';
	let activeTab = $state<Tab>('output');

	// Auto-switch tab based on content
	$effect(() => {
		if (hasBuildOutput || hasError) {
			activeTab = 'build';
		} else if (hasStdout || exceptionMsg) {
			activeTab = 'output';
		}
	});
</script>

<div class="h-full flex flex-col bg-white text-islands-700 dark:bg-islands-900 dark:text-islands-200">
	<!-- Header with tabs -->
	<div class="flex items-center justify-between h-9 px-2 border-b border-line bg-islands-50 dark:bg-islands-800">
		<div class="flex items-center gap-1">
			<button
				class="px-3 py-1.5 text-[11px] font-medium rounded-sm transition-colors {activeTab === 'output' ? 'bg-islands-200 text-islands-900 dark:bg-islands-700 dark:text-islands-100' : 'text-islands-500 hover:text-islands-800 hover:bg-islands-200/60 dark:text-islands-400 dark:hover:text-islands-200 dark:hover:bg-islands-700/50'}"
				onclick={() => activeTab = 'output'}
			>
				Output
			</button>
			<button
				class="px-3 py-1.5 text-[11px] font-medium rounded-sm transition-colors {activeTab === 'build' ? 'bg-islands-200 text-islands-900 dark:bg-islands-700 dark:text-islands-100' : 'text-islands-500 hover:text-islands-800 hover:bg-islands-200/60 dark:text-islands-400 dark:hover:text-islands-200 dark:hover:bg-islands-700/50'} {hasError ? 'text-red-600 dark:text-red-400' : ''}"
				onclick={() => activeTab = 'build'}
			>
				Build
				{#if hasError}
					<span class="ml-1 w-2 h-2 inline-block rounded-full bg-red-500"></span>
				{/if}
			</button>
			{#if appState.hasMemoryLeaks}
				<button
					class="px-3 py-1.5 text-[11px] font-medium rounded-sm transition-colors {activeTab === 'leaks' ? 'bg-islands-200 text-islands-900 dark:bg-islands-700 dark:text-islands-100' : 'text-islands-500 hover:text-islands-800 hover:bg-islands-200/60 dark:text-islands-400 dark:hover:text-islands-200 dark:hover:bg-islands-700/50'}"
					onclick={() => activeTab = 'leaks'}
				>
					Leaks
					<span class="ml-1 px-1.5 py-0.5 text-[10px] bg-amber-500/20 text-amber-600 dark:text-amber-400 rounded-sm">
						{appState.trace?.memory_leaks?.length}
					</span>
				</button>
			{/if}
		</div>

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

	<!-- Content -->
	<div class="flex-1 min-h-0 overflow-auto p-3">
		{#if activeTab === 'output'}
			<div class="console-output font-mono text-[12px]">
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
			<div class="console-output font-mono text-[12px]">
				{#if hasError}
					<div class="text-red-600 dark:text-red-400 mb-2">
						<span class="font-semibold">Error:</span> {appState.error}
					</div>
				{/if}

				{#if hasBuildOutput}
					<pre class="text-islands-600 dark:text-islands-400 text-[11px] whitespace-pre-wrap">{appState.buildOutput}</pre>
				{:else if !hasError}
					<span class="text-islands-400 dark:text-islands-500 italic">No build output</span>
				{/if}
			</div>
		{:else if activeTab === 'leaks'}
			<div class="space-y-2">
				{#if appState.trace?.memory_leaks}
					{#each appState.trace.memory_leaks as leak}
						<div class="p-2.5 bg-amber-50 border border-amber-200 dark:bg-amber-900/20 dark:border-amber-700/30 rounded-sm text-[12px]">
							<div class="flex items-center justify-between">
								<span class="font-mono text-amber-700 dark:text-amber-300">heap[{getLeakHeapId(leak)}]</span>
								<span class="text-islands-500 dark:text-islands-400">leaked</span>
							</div>
							<div class="text-islands-500 dark:text-islands-400 text-[11px] mt-1">
								Type: <span class="text-islands-700 dark:text-islands-300">{getLeakTypeName(leak)}</span>
							</div>
						</div>
					{/each}
				{/if}
			</div>
		{/if}
	</div>
</div>

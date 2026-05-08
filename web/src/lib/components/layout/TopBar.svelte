<script lang="ts">
	import { appState } from '$lib/state/app.svelte';
	import Brand from './Brand.svelte';
	import SettingsMenu from '$lib/components/settings/SettingsMenu.svelte';
	import VideoModal from '$lib/components/modals/VideoModal.svelte';
	import { shortcuts } from '$lib/platform/kbd';

	interface Props {
		onOpenExamples: () => void;
	}

	let { onOpenExamples }: Props = $props();
	let settingsOpen = $state(false);
	let videoModalOpen = $state(false);

	// Status computation
	const statusKind = $derived(() => {
		if (appState.running) return 'compiling';  // Waiting for backend
		if (appState.playing) return 'running';    // Animation playing
		if (appState.error) return 'error';
		if (appState.stale && appState.trace) return 'stale';
		if (appState.trace) return 'ready';
		return 'idle';
	});

	const statusLabel = $derived(() => {
		const kind = statusKind();
		switch (kind) {
			case 'compiling': return 'Compiling';
			case 'running': return 'Running';
			case 'error': return 'Error';
			case 'stale': return 'Stale';
			case 'ready': return 'Ready';
			default: return 'Idle';
		}
	});

	const statusDotClass = $derived(() => {
		const kind = statusKind();
		switch (kind) {
			case 'compiling': return 'bg-amber-500 animate-pulse';
			case 'running': return 'bg-brand-500 animate-pulse';
			case 'error': return 'bg-red-500';
			case 'stale': return 'bg-amber-500';
			case 'ready': return 'bg-green-500';
			default: return 'bg-islands-400';
		}
	});

	// Stats
	const stepsCount = $derived(appState.trace?.trace.length ?? 0);

	// Estimate heap bytes (rough estimate based on trace data)
	const heapBytes = $derived(() => {
		if (!appState.trace) return 0;
		// Sum up memory from all heap objects in the last step
		const lastStep = appState.trace.trace[appState.trace.trace.length - 1];
		if (!lastStep) return 0;
		// Rough estimate: count objects * average size
		return Object.keys(lastStep.heap).length * 64; // Rough estimate
	});

	function formatBytes(bytes: number): string {
		if (bytes < 1024) return `${bytes}B`;
		if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)}KB`;
		return `${(bytes / (1024 * 1024)).toFixed(1)}MB`;
	}
</script>

<header class="flex h-10 min-h-10 px-3 shrink-0 items-stretch" data-testid="topbar">
	<Brand />

	<!-- Left section: Examples -->
	<div class="flex items-center border-l border-line px-3">
		<button
			onclick={onOpenExamples}
			class="btn-chonky btn-chonky-secondary mt-px"
			data-testid="examples-button"
		><span>
			<!-- ph:code-fill -->
			<svg xmlns="http://www.w3.org/2000/svg" class="size-5" viewBox="0 0 16 16"><path fill="currentColor" d="M9.905 2.815a.75.75 0 0 1 .38.99l-4 9a.75.75 0 1 1-1.37-.61l4-9a.75.75 0 0 1 .99-.38M4.498 5.19a.75.75 0 0 1 .063 1.058L3.003 8l1.558 1.752a.75.75 0 1 1-1.122.996l-2-2.25a.75.75 0 0 1 0-.996l2-2.25A.75.75 0 0 1 4.5 5.19m7.004 0a.75.75 0 0 1 1.059.062l2 2.25a.75.75 0 0 1 0 .996l-2 2.25a.75.75 0 0 1-1.122-.996L12.996 8L11.44 6.248a.75.75 0 0 1 .063-1.058"/></svg>
			Examples
			<kbd class="hidden sm:inline px-1 font-mono opacity-50">
				{shortcuts.examples()}
			</kbd>
		</span></button>
	</div>

	<!-- Social links & What/Why -->
	<div class="flex items-center border-l border-line px-3">
		<div class="btn-bar mt-px">
			<a
				href="https://github.com/imkiptoo/cpp-runtime-inspector"
				target="_blank"
				rel="noopener noreferrer"
				class="btn-chonky btn-chonky-secondary btn-chonky-square w-9"
				aria-label="GitHub"
			>
				<span>
					<!-- ph:github-logo-fill -->
					<svg xmlns="http://www.w3.org/2000/svg" class="size-5" viewBox="0 0 256 256"><path fill="currentColor" d="M208.31 75.68A59.78 59.78 0 0 0 202.93 28a8 8 0 0 0-6.93-4a59.75 59.75 0 0 0-48 24h-24a59.75 59.75 0 0 0-48-24a8 8 0 0 0-6.93 4a59.78 59.78 0 0 0-5.38 47.68A58.14 58.14 0 0 0 56 104v8a56.06 56.06 0 0 0 48.44 55.47A39.8 39.8 0 0 0 96 192v8H72a24 24 0 0 1-24-24a40 40 0 0 0-40-40a8 8 0 0 0 0 16a24 24 0 0 1 24 24a40 40 0 0 0 40 40h24v16a8 8 0 0 0 16 0v-40a24 24 0 0 1 48 0v40a8 8 0 0 0 16 0v-40a39.8 39.8 0 0 0-8.44-24.53A56.06 56.06 0 0 0 216 112v-8a58.14 58.14 0 0 0-7.69-28.32"/></svg>
				</span>
			</a>
			<a
				href="https://imkiptoo.dev"
				target="_blank"
				rel="noopener noreferrer"
				class="btn-chonky btn-chonky-secondary btn-chonky-square w-9"
				aria-label="Website"
			>
				<span>
					<svg xmlns="http://www.w3.org/2000/svg" class="size-5" viewBox="0 0 16 16"><path fill="currentColor" d="M6 8c0-.703.044-1.375.125-2h3.75c.08.625.125 1.297.125 2s-.044 1.375-.125 2h-3.75A16 16 0 0 1 6 8m-.883 2A17 17 0 0 1 5 8c0-.693.04-1.365.117-2H2.34A6 6 0 0 0 2 8c0 .701.12 1.374.341 2zm-2.314 1h2.47c.125.655.292 1.254.493 1.776c.134.349.286.672.457.957A6.02 6.02 0 0 1 2.803 11m3.489 0h3.416a9 9 0 0 1-.407 1.417c-.213.554-.455.969-.698 1.236S8.156 14 8 14s-.36-.08-.603-.347s-.485-.682-.698-1.236A9 9 0 0 1 6.292 11m4.436 0a10.5 10.5 0 0 1-.494 1.776a6 6 0 0 1-.457.957A6.02 6.02 0 0 0 13.197 11zm2.93-1A6 6 0 0 0 14 8a6 6 0 0 0-.341-2h-2.776c.076.635.117 1.307.117 2s-.04 1.365-.117 2zM9.302 3.583c.159.414.297.89.407 1.417H6.292c.11-.527.248-1.003.407-1.417c.213-.554.455-.969.698-1.236S7.844 2 8 2s.36.08.603.347s.485.682.698 1.236M10.728 5h2.47a6.02 6.02 0 0 0-3.421-2.733c.17.285.323.608.457.957c.201.522.368 1.12.494 1.776M2.803 5h2.47a10.5 10.5 0 0 1 .493-1.776c.134-.349.286-.672.457-.957A6.02 6.02 0 0 0 2.803 5"/></svg>
				</span>
			</a>
			<button
				onclick={() => videoModalOpen = true}
				class="btn-chonky btn-chonky-secondary"
				aria-label="What & Why"
			>
				<span>
					<!-- ph:play-circle-fill -->
					<svg xmlns="http://www.w3.org/2000/svg" class="size-5" viewBox="0 0 256 256"><path fill="currentColor" d="M128 24a104 104 0 1 0 104 104A104.11 104.11 0 0 0 128 24m40.55 110.58l-52 36A8 8 0 0 1 104 164v-72a8 8 0 0 1 12.55-6.58l52 36a8 8 0 0 1 0 13.16"/></svg>
					What & Why
				</span>
			</button>
		</div>
	</div>

	<!-- Spacer -->
	<div class="flex-1"></div>

	<!-- Status pill -->
	<div class="flex items-center gap-3 border-line px-3 font-mono text-sm text-islands-500 dark:text-islands-400" data-testid="status-pill">
		<span class="inline-block h-2 w-2 rounded-full {statusDotClass()}"></span>
		<span class="font-medium">
			{statusLabel()}
		</span>
		{#if appState.trace && !appState.error && !appState.running}
			<span class="text-islands-300 dark:text-islands-600">|</span>
			<span>{stepsCount} steps</span>
			<span class="text-islands-300 dark:text-islands-600">|</span>
			<span>{formatBytes(heapBytes())} heap</span>
		{/if}
	</div>

	<!-- Run button -->
	<div class="flex items-center border-l border-line px-3">
		<button
			type="button"
			onclick={() => appState.run()}
			disabled={appState.running}
			data-testid="run-button"
			class="btn-chonky btn-chonky-primary {appState.running ? 'btn-chonky-busy' : 'mt-px'}"
		>
			<span>
				{#if appState.running}
					<svg class="w-4 h-4 animate-spin" fill="none" viewBox="0 0 24 24">
						<circle class="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" stroke-width="4"></circle>
						<path class="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
					</svg>
					Compiling...
				{:else}
					<svg xmlns="http://www.w3.org/2000/svg" class="size-4" viewBox="0 0 24 24"><path fill="currentColor" d="M5 5.274c0-1.707 1.826-2.792 3.325-1.977l12.362 6.727c1.566.852 1.566 3.1 0 3.952L8.325 20.702C6.826 21.518 5 20.432 5 18.726z"/></svg>
					<div>
						Run
					</div>
					<kbd class="hidden sm:inline opacity-50">
						{shortcuts.run()}
					</kbd>
				{/if}
			</span>
		</button>
	</div>

	<!-- Settings -->
	<div class="flex items-center border-line">
		<div class="relative">
			<button
				onclick={() => settingsOpen = !settingsOpen}
				class="btn-chonky btn-chonky-secondary btn-chonky-square w-8 mt-px"
				aria-expanded={settingsOpen}
				aria-label="Settings"
			>
				<span>
					<svg xmlns="http://www.w3.org/2000/svg" class="size-5" viewBox="0 0 16 16"><path fill="currentColor" d="M2.267 6.153A6 6 0 0 1 3.53 3.98a.36.36 0 0 1 .382-.095l1.36.484a.71.71 0 0 0 .935-.538l.26-1.416a.35.35 0 0 1 .274-.282a6.1 6.1 0 0 1 2.52 0c.14.03.248.141.274.282l.26 1.416a.708.708 0 0 0 .935.538l1.36-.484a.36.36 0 0 1 .382.095a6 6 0 0 1 1.262 2.173a.35.35 0 0 1-.108.378l-1.102.931a.703.703 0 0 0 0 1.076l1.102.931c.11.093.152.242.108.378a6 6 0 0 1-1.262 2.173a.36.36 0 0 1-.382.095l-1.36-.484a.71.71 0 0 0-.935.538l-.26 1.416a.35.35 0 0 1-.275.282a6.1 6.1 0 0 1-2.519 0a.35.35 0 0 1-.275-.282l-.259-1.416a.708.708 0 0 0-.935-.538l-1.36.484a.36.36 0 0 1-.382-.095a6 6 0 0 1-1.262-2.173a.35.35 0 0 1 .108-.378l1.102-.931a.704.704 0 0 0 0-1.076l-1.102-.931a.35.35 0 0 1-.108-.378M6.25 8a1.75 1.75 0 1 0 3.5 0a1.75 1.75 0 0 0-3.5 0"/></svg>
				</span>
			</button>

			{#if settingsOpen}
				<SettingsMenu onClose={() => settingsOpen = false} />
			{/if}
		</div>
	</div>
</header>

<VideoModal open={videoModalOpen} onClose={() => videoModalOpen = false} />

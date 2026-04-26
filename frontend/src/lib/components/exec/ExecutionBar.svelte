<script lang="ts">
	import { appState } from '$lib/state/app.svelte';
	import Scrubbar from './Scrubbar.svelte';
	import { shortcuts } from '$lib/platform/kbd';

	const hasTrace = $derived(appState.trace !== null);
	const totalSteps = $derived(appState.totalSteps);
	const stepIndex = $derived(appState.stepIndex);
	const step = $derived(appState.currentStep);

	const disabled = $derived(!hasTrace || totalSteps === 0);
	const atStart = $derived(disabled || stepIndex <= 0);
	const atEnd = $derived(disabled || stepIndex >= totalSteps - 1);

	// Compute whether step-into/step-out are possible from current position
	const canStepInto = $derived(() => {
		if (!appState.trace || atEnd) return false;
		const hereDepth = appState.trace.trace[stepIndex]?.stack_to_render.length ?? 0;
		for (let i = stepIndex + 1; i < appState.trace.trace.length; i++) {
			const d = appState.trace.trace[i].stack_to_render.length;
			if (d > hereDepth) return true;
		}
		return false;
	});

	const canStepOut = $derived(() => {
		if (!appState.trace || atEnd) return false;
		const hereDepth = appState.trace.trace[stepIndex]?.stack_to_render.length ?? 0;
		for (let i = stepIndex + 1; i < appState.trace.trace.length; i++) {
			const d = appState.trace.trace[i].stack_to_render.length;
			if (d < hereDepth) return true;
		}
		return false;
	});

</script>

<section class="flex h-10 shrink-0 items-center gap-3 px-3" data-testid="exec-bar">
	<!-- Primary transport group -->
	<div class="btn-bar mt-px">
		<button
			type="button"
			onclick={() => appState.goToStart()}
			disabled={atStart}
			aria-label="Restart"
			title={atStart ? (disabled ? 'Run first to load a trace' : 'Already at the first step') : `Restart (${shortcuts.goToStart()})`}
			class="btn-chonky h-7.25 w-9 btn-chonky-secondary btn-chonky-square"
		><span>
			<!-- ph:skip-back-fill -->
			<svg xmlns="http://www.w3.org/2000/svg" class="size-4" viewBox="0 0 16 16"><path fill="currentColor" d="M6.97 3.22a.75.75 0 1 1 1.06 1.06L4.31 8l3.72 3.72a.75.75 0 1 1-1.06 1.06L2.72 8.53a.75.75 0 0 1 0-1.06zm4.5 0a.75.75 0 1 1 1.06 1.06L8.81 8l3.72 3.72a.75.75 0 1 1-1.06 1.06L7.22 8.53a.75.75 0 0 1 0-1.06z"/></svg>
		</span></button>

		<button
			type="button"
			onclick={() => appState.stepBackward()}
			disabled={atStart}
			aria-label="Step backward"
			title={atStart ? (disabled ? 'Run first to load a trace' : 'Already at the first step') : `Step backward (${shortcuts.stepBackward()})`}
			class="btn-chonky h-7.25 w-9 btn-chonky-secondary btn-chonky-square"
		><span>
			<!-- ph:caret-left-fill -->
			<svg xmlns="http://www.w3.org/2000/svg" class="size-4" viewBox="0 0 16 16"><path fill="currentColor" d="M14 8a.75.75 0 0 1-.75.75H4.463l3.287 2.941a.75.75 0 1 1-1 1.118L2 8.559A.75.75 0 0 1 2 7.44l4.75-4.25a.75.75 0 1 1 1 1.118L4.463 7.25h8.787A.75.75 0 0 1 14 8"/></svg>
		</span></button>

		<button
			type="button"
			onclick={() => appState.togglePlay()}
			disabled={disabled}
			aria-label={appState.playing ? 'Pause' : 'Play'}
			title={disabled ? 'Run first to load a trace' : `${appState.playing ? 'Pause' : 'Play'} (${shortcuts.playPause()})`}
			class="btn-chonky h-7.25 w-9 btn-chonky-primary btn-chonky-square"
		><span>
			{#if appState.playing}
				<!-- ph:pause-fill -->
				<svg xmlns="http://www.w3.org/2000/svg" class="size-4" viewBox="0 0 16 16"><path fill="currentColor" d="M3.75 2A1.75 1.75 0 0 0 2 3.75v8.5c0 .966.784 1.75 1.75 1.75h1.5A1.75 1.75 0 0 0 7 12.25v-8.5A1.75 1.75 0 0 0 5.25 2zm7 0A1.75 1.75 0 0 0 9 3.75v8.5c0 .966.784 1.75 1.75 1.75h1.5A1.75 1.75 0 0 0 14 12.25v-8.5A1.75 1.75 0 0 0 12.25 2z"/></svg>
			{:else}
				<!-- ph:play-fill -->
				<svg xmlns="http://www.w3.org/2000/svg" class="size-4" viewBox="0 0 16 16"><path fill="currentColor" d="M14.222 6.687a1.5 1.5 0 0 1 0 2.629l-10 5.499A1.5 1.5 0 0 1 2 13.5V2.502a1.5 1.5 0 0 1 2.223-1.314z"/></svg>
			{/if}
		</span></button>

		<button
			type="button"
			onclick={() => appState.stepForward()}
			disabled={atEnd}
			aria-label="Step forward"
			title={atEnd ? (disabled ? 'Run first to load a trace' : 'Already at the last step') : `Step forward (${shortcuts.stepForward()})`}
			class="btn-chonky h-7.25 w-9 btn-chonky-secondary btn-chonky-square"
		><span>
			<!-- ph:caret-right-fill -->
			<svg xmlns="http://www.w3.org/2000/svg" class="size-4" viewBox="0 0 16 16"><path fill="currentColor" d="M2 8a.75.75 0 0 1 .75-.75h8.787L8.25 4.309a.75.75 0 0 1 1-1.118L14 7.441a.75.75 0 0 1 0 1.118l-4.75 4.25a.75.75 0 1 1-1-1.118l3.287-2.941H2.75A.75.75 0 0 1 2 8"/></svg>
		</span></button>

		<button
			type="button"
			onclick={() => appState.goToEnd()}
			disabled={atEnd}
			aria-label="Jump to end"
			title={atEnd ? (disabled ? 'Run first to load a trace' : 'Already at the last step') : `Jump to end (${shortcuts.goToEnd()})`}
			class="btn-chonky h-7.25 w-9 btn-chonky-secondary btn-chonky-square"
		><span>
			<!-- ph:skip-forward-fill -->
			<svg xmlns="http://www.w3.org/2000/svg" class="size-4" viewBox="0 0 16 16"><path fill="currentColor" d="M3.47 3.22a.75.75 0 0 1 1.06 0l4.25 4.25a.75.75 0 0 1 0 1.06l-4.25 4.25a.75.75 0 0 1-1.06-1.06L7.19 8L3.47 4.28a.75.75 0 0 1 0-1.06m4.5 0a.75.75 0 0 1 1.06 0l4.25 4.25a.75.75 0 0 1 0 1.06l-4.25 4.25a.75.75 0 0 1-1.06-1.06L11.69 8L7.97 4.28a.75.75 0 0 1 0-1.06"/></svg>
		</span></button>
	</div>

	<!-- Step into/out group -->
	<div class="btn-bar mt-px">
		<button
			type="button"
			onclick={() => appState.stepInto()}
			disabled={disabled || !canStepInto()}
			aria-label="Step into"
			title={disabled ? 'Run first to load a trace' : (!canStepInto() ? 'No function call to step into from this line' : 'Step into')}
			class="btn-chonky h-7.25 w-9 btn-chonky-secondary btn-chonky-square"
		><span>
			<!-- ph:arrow-elbow-down-right-fill -->
			<svg xmlns="http://www.w3.org/2000/svg" class="size-4" viewBox="0 0 16 16" version="1.1" xmlns:xlink="http://www.w3.org/1999/xlink" transform="matrix(1,0,0,-1,0,0)"><path fill="currentColor" d="M8.72 1.72a.75.75 0 0 1 1.06 0l3.5 3.5a.75.75 0 0 1 0 1.06l-3.5 3.5a.75.75 0 0 1-1.06-1.06l2.22-2.22H5.75c-.69 0-1.25.56-1.25 1.25v5.5a.75.75 0 0 1-1.5 0v-5.5A2.75 2.75 0 0 1 5.75 5h5.19L8.72 2.78a.75.75 0 0 1 0-1.06"></path></svg>
		</span></button>

		<button
			type="button"
			onclick={() => appState.stepOut()}
			disabled={disabled || !canStepOut()}
			aria-label="Step out"
			title={disabled ? 'Run first to load a trace' : (!canStepOut() ? 'Already in the top-level frame' : 'Step out')}
			class="btn-chonky h-7.25 w-9 btn-chonky-secondary btn-chonky-square"
		><span>
			<!-- ph:arrow-elbow-up-left-fill -->
			<svg xmlns="http://www.w3.org/2000/svg" class="size-4" viewBox="0 0 16 16" version="1.1" xmlns:xlink="http://www.w3.org/1999/xlink" transform="matrix(-1,0,0,1,0,0)"><path fill="currentColor" d="M8.72 1.72a.75.75 0 0 1 1.06 0l3.5 3.5a.75.75 0 0 1 0 1.06l-3.5 3.5a.75.75 0 0 1-1.06-1.06l2.22-2.22H5.75c-.69 0-1.25.56-1.25 1.25v5.5a.75.75 0 0 1-1.5 0v-5.5A2.75 2.75 0 0 1 5.75 5h5.19L8.72 2.78a.75.75 0 0 1 0-1.06"></path></svg>
		</span></button>
	</div>

	<!-- Timeline label -->
	<span class="text-sm text-islands-400 dark:text-islands-500">
		Timeline
	</span>

	<!-- Scrubbar -->
	<div class="flex-1 min-w-[200px]">
		<Scrubbar
			value={stepIndex}
			max={totalSteps - 1}
			{totalSteps}
			onChange={(v) => appState.stepTo(v)}
			{disabled}
		/>
	</div>

	<!-- Step counter -->
	<div class="flex items-baseline gap-2 font-mono text-[12px] text-islands-800 dark:text-islands-200" data-testid="exec-counter">
		<span class="text-[16px] font-medium text-brand-600 dark:text-brand-400">
			{String(disabled ? 0 : stepIndex + 1).padStart(2, '0')}
		</span>
		<span class="text-islands-400 dark:text-islands-500">/</span>
		<span class="text-islands-500 dark:text-islands-400">
			{String(totalSteps).padStart(2, '0')}
		</span>
	</div>

	<!-- Line indicator -->
	<div class="flex items-center gap-1.5 justify-between line-number rounded-sm border border-line px-1.5 font-mono text-sm text-islands-600 dark:text-islands-300">
		<span class="text-islands-400 dark:text-islands-500">Line: </span>
		<span data-testid="exec-line">
			{typeof step?.line === 'number' && step.line > 0 ? String(step.line).padStart(2, '0') : '—'}
		</span>
	</div>

	<!-- Console toggle -->
	<button
		class="btn-chonky btn-chonky-secondary btn-chonky-square mt-px h-7.25 w-8"
		onclick={() => appState.toggleConsole()}
		title="Toggle console"
		aria-label="Toggle console"
	><span>
		<!-- ph:terminal-fill -->
		<svg xmlns="http://www.w3.org/2000/svg" class="size-5" viewBox="0 0 16 16"><path fill="currentColor" d="M4.5 2A2.5 2.5 0 0 0 2 4.5v7A2.5 2.5 0 0 0 4.5 14h7a2.5 2.5 0 0 0 2.5-2.5v-7A2.5 2.5 0 0 0 11.5 2zM3 4.5A1.5 1.5 0 0 1 4.5 3h7A1.5 1.5 0 0 1 13 4.5V8H3z"/></svg>
	</span></button>
</section>


<style>
    .line-number {
				min-width: 10ch;
		}
</style>